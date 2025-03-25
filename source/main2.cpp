#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <utility>
#include <iomanip>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/tokenizer.hpp>

// Structure pour stocker les informations d'un nœud
struct NodeInfo {
    int id;
    double x, y, z;
};

// Structure pour stocker les propriétés des arêtes
struct EdgeInfo {
    double weight;
    bool inPath;  // Pour le path court
};

// Définition du type de graphe
typedef boost::adjacency_list<
    boost::vecS,           // Conteneur pour les arêtes sortantes
    boost::vecS,           // Conteneur pour les sommets
    boost::undirectedS,    // Graphe non orienté
    NodeInfo,              // Structur pour les nœuds
    EdgeInfo        // Struct pour les arêtes
> Graph;

typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

// visiteur pour détecter les cycles
// algorythme depth first car + rapide
class CycleDetector : public boost::dfs_visitor<> {
public:
    CycleDetector(bool& hasCycle) : _hasCycle(hasCycle) {}
    /*CycleDetector(bool& hasCycle) {
        // Initialize the member variable _hasCycle with the value of hasCycle
        _hasCycle = hasCycle;
    }*/
    // Examine les arêtes arrières pour détecter les cycles
    template <class Edge, class Graph>
    void back_edge(Edge, const Graph&) {
        _hasCycle = true;
    }
private:
    bool& _hasCycle;
};

// Fonctions pour charger les données
std::vector<NodeInfo> loadNodes(const std::string& filename) {
    std::vector<NodeInfo> nodes; // on fabrique un vecteur remplie de nodeInfo pour stocker la data
    std::ifstream file(filename);
    
    // verification classique si le fichier est bien là
    if (!file.is_open()) {
        std::cerr << "Erreur lors de l'ouverture du fichier " << filename << std::endl;
        return nodes;
    }
    
    std::string line;
    
    // Ignorer la première ligne (en-têtes)
    std::getline(file, line);
    // Parcour le reste du fichier
    while (std::getline(file, line)) {
        boost::char_separator<char> sep(";"); // defini ; comme separateur
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep); // split la ligne en par ";"
        
        auto tok = tokens.begin(); //pointeur de départ
        if (tok != tokens.end()) {
            NodeInfo node; // creation new node
            node.id = std::stoi(*tok++); // 1er colonne = ID
            if (tok != tokens.end()) node.x = std::stod(*tok++);
            if (tok != tokens.end()) node.y = std::stod(*tok++);
            if (tok != tokens.end()) node.z = std::stod(*tok++);
            
            nodes.push_back(node);
        }
    }
    
    return nodes;
}

// voir loadNodes car c'est pareil
// ! le graph est dejà fait, on ajoute juste arretes
// d'ou le void
void loadEdges(Graph& g, const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Erreur durant l'ouverture du fichier " << filename << std::endl;
        return;
    }
    
    std::string line;
    
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        boost::char_separator<char> sep(";");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        
        auto tok = tokens.begin();
        if (tok != tokens.end()) {
            int source = std::stoi(*tok++) - 1;  // -1 car les ID commencent lign1 dans le fichier csv
            if (tok != tokens.end()) {
                int target = std::stoi(*tok++) - 1;  // mais les indices dans le graphe commencent à 0
                
                // Vérifier si les indices sont valides
                if (source >= 0 && source < boost::num_vertices(g) && 
                    target >= 0 && target < boost::num_vertices(g)) {
                    // Calculer le poids de l'arête (distance euclidienne 3D)
                    double dx = g[source].x - g[target].x;
                    double dy = g[source].y - g[target].y;
                    double dz = g[source].z - g[target].z;
                    // pythogore
                    double weight = std::sqrt(dx*dx + dy*dy + dz*dz);
                    
                    // Ajouter l'arête au graphe avec son poids
                    Edge e;
                    bool inserted;
                    boost::tie(e, inserted) = boost::add_edge(source, target, g);
                    if (inserted) {
                        g[e].weight = weight;
                        g[e].inPath = false;
                    }
                }
            }
        }
    }
}

// Fonction pour vérifier si le graphe est connecté
bool isConnected(const Graph& g) {
    std::vector<int> component(boost::num_vertices(g));
    int num_components = boost::connected_components(g, &component[0]);
    return num_components == 1;
}

// Fonction pour détecter les cycles dans le graphe
bool hasCycle(const Graph& g) {
    bool cycle_detected = false;
    CycleDetector vis(cycle_detected);
    
    // Vector pour marquer les sommets visités
    std::vector<boost::default_color_type> colorMap(boost::num_vertices(g));
    
    // Recherche en profondeur pour détecter les cycles
    boost::depth_first_search(g, boost::visitor(vis).color_map(boost::make_iterator_property_map(
        colorMap.begin(), boost::get(boost::vertex_index, g))));
    
    return cycle_detected;
}

// Fonction pour calculer le chemin le plus court entre deux nœuds
std::pair<double, std::vector<int>> shortestPath(const Graph& g, int startNodeId, int endNodeId) {
    int start = startNodeId - 1;  // Convertir de l'ID à l'indice (le csv rang1 :/)
    int end = endNodeId - 1;
    
    // Vérifier si les indices sont valides
    if (start < 0 || start >= boost::num_vertices(g) || end < 0 || end >= boost::num_vertices(g)) {
        std::cerr << "Erreur : Indices de nœuds invalides." << std::endl;
        return {-1, {}};
    }
    
    // sommet passé
    std::vector<Vertex> predecessor(boost::num_vertices(g));
    // distances passées
    std::vector<double> distance(boost::num_vertices(g));
    
    // Exécuter l'algorithme de Dijkstra
    // trouver sur internet/ à verifié
    boost::dijkstra_shortest_paths(g, start, 
        boost::predecessor_map(boost::make_iterator_property_map(predecessor.begin(), boost::get(boost::vertex_index, g)))
        .distance_map(boost::make_iterator_property_map(distance.begin(), boost::get(boost::vertex_index, g)))
        .weight_map(boost::get(&EdgeInfo::weight, g)));
    
    // Reconstruire le chemin
    std::vector<int> path;
    
    // Vérifier si un chemin existe
    if (predecessor[end] == end && start != end) {
        return {-1, {}}; // Pas de chemin
    }
    
    // Reconstruire le chemin à partir des prédécesseurs
    for (Vertex v = end; ; v = predecessor[v]) {
        path.push_back(g[v].id);
        if (v == start) break;
    }
    
    // Inverser le chemin pour qu'il soit du début à la fin
    std::reverse(path.begin(), path.end());
    
    return {distance[end], path};
}

// Fonction pour marquer les arêtes du chemin (pour l'illustration)
void markPathEdges(Graph& g, const std::vector<int>& path) {
    // Réinitialiser toutes les arêtes
    boost::graph_traits<Graph>::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = boost::edges(g); ei != ei_end; ++ei) {
        g[*ei].inPath = false;
    }
    
    // Marquer les arêtes du chemin
    for (size_t i = 0; i < path.size() - 1; ++i) {
        int u = path[i] - 1;  // Convertir l'ID en indice
        int v = path[i + 1] - 1;
        
        // Trouver l'arête entre u et v
        Edge e;
        bool exists;
        boost::tie(e, exists) = boost::edge(u, v, g);
        if (exists) {
            g[e].inPath = true;
        }
    }
}

// Fonction pour générer une illustration du graphe (sortie Graphviz DOT)
void generateGraphImage(const Graph& g, const std::string& filename) {
    std::ofstream dotFile(filename);
    std::string pattern = ".png";
    if (!dotFile.is_open()) {
        std::cerr << "Erreur : Impossible de créer le fichier DOT." << std::endl;
        return;
    }
    
    // Écrire l'attribut pour le nœud (afficher l'ID)
    auto vertex_writer = [&g](std::ostream& out, const Vertex& v) {
        out << "[label=\"" << g[v].id << "\"]";
    };
    
    // Écrire les attributs pour l'arête (couleur et poids)
    auto edge_writer = [&g](std::ostream& out, const Edge& e) {
        out << "[label=\"" << std::fixed << std::setprecision(2) << g[e].weight << "\"";
        if (g[e].inPath) {
            out << ", color=red, penwidth=2";
        }
        out << "]";
    };
    
    // Écrire le graphe au format DOT
    boost::write_graphviz(dotFile, g, vertex_writer, edge_writer);
    if (!(filename.find(pattern) != std::string::npos)) { // test si l'utilisateur demande un PNG
    std::cout << "Fichier DOT généré : " << filename << std::endl;
    std::cout << "Pour visualiser le graphe, utilisez Graphviz avec la commande :" << std::endl;
    std::cout << "dot -Tpng " << filename << " -o graph.png" << std::endl;
    } else {
    // Command to generate a PNG from a DOT file
    std::string command = "dot -Tpng " + filename + " -o " + filename;
    int result = system(command.c_str());

    // Check the result for success or failure
    if (result == 0) {
        std::cout << "png successfully!" << std::endl;
    } else {
        std::cerr << "Error making png!" << std::endl;
    }
    std::cout << "PNG réalsier" << std::endl;
    }
}

// Fonction pour écrire les résultats des chemins les plus courts dans un fichier CSV
void writePathsToCSV(const Graph& g, const std::vector<std::pair<int, int>>& nodePairs, const std::string& filename) {
    std::ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        std::cerr << "Erreur : Impossible de créer le fichier CSV." << std::endl;
        return;
    }
    
    // Écrire l'en-tête du CSV
    csvFile << "SourceNodeID;TargetNodeID;PathLength;Path" << std::endl;
    
    // Pour chaque paire de nœuds
    for (const auto& pair : nodePairs) {
        int source = pair.first;
        int target = pair.second;
        
        // Calculer le chemin le plus court
        auto result = shortestPath(g, source, target);
        double distance = result.first;
        const auto& path = result.second;
        
        // Écrire dans le CSV
        csvFile << source << ";" << target << ";" << std::fixed << std::setprecision(2) << distance << ";";
        
        // Écrire le chemin
        if (path.empty()) {
            csvFile << "No path";
        } else {
            for (size_t i = 0; i < path.size(); ++i) {
                csvFile << path[i];
                if (i < path.size() - 1) {
                    csvFile << "->";
                }
            }
        }
        csvFile << std::endl;
    }
    
    std::cout << "Fichier CSV généré : " << filename << std::endl;
}

// Fonction principale pour générer le rapport d'analyse du graphe
void generateGraphReport(Graph& g, int start, int end) {
    std::cout << "=== RAPPORT D'ANALYSE DU GRAPHE ===" << std::endl;
    std::cout << "Nombre de nœuds: " << boost::num_vertices(g) << std::endl;
    std::cout << "Nombre d'arêtes: " << boost::num_edges(g) << std::endl;
    
    // i. Calculer et afficher le degré de chaque nœud
    std::cout << "\n== i. Degré des nœuds ==" << std::endl;
    
    int max_degree = 0;
    boost::graph_traits<Graph>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi) {
        int degree = boost::degree(*vi, g);
        std::cout << "Nœud " << g[*vi].id << ": " << degree << std::endl;
        
        if (degree > max_degree) {
            max_degree = degree;
        }
    }
    
    // Calculer et afficher le degré du graphe (degré maximum)
    std::cout << "Degré du graphe: " << max_degree << std::endl;
    
    // Vérifiation la connectivité du graphe
    std::cout << "\n== ii. Connectivité du graphe ==" << std::endl;
    bool connected = isConnected(g);
    std::cout << "Le graphe est " << (connected ? "connecté" : "non connecté") << std::endl;
    
    // Détection de cycles
    std::cout << "\n== iii. Détection de cycles ==" << std::endl;
    bool cycle = hasCycle(g);
    std::cout << "Le graphe " << (cycle ? "contient":"ne contient pas") << " de cycle" << std::endl;
    std::cout << "Explication de l'algorithme de détection de cycles:" << std::endl;
    std::cout << "1. Utilisation d'un parcours en profondeur (DFS) du graphe" << std::endl;
    std::cout << "2. Lors du parcours, nous maintenons un état pour chaque nœud (non visité, en cours, visité)" << std::endl;
    std::cout << "3. Quand nous rencontrons une arête arrière (qui mène à un nœud en cours de visite), un cycle est détecté" << std::endl;
    std::cout << "4. L'implémentation utilise le visiteur boost::dfs_visitor qui détecte les arêtes arrière" << std::endl;
    
    // iv. Exemple de calcul de chemin le plus court
    std::cout << "\n== iv. calcul de chemin le plus court ==" << std::endl;
    std::cout <<"    * node de départ: "<< start <<std::endl;
    std::cout <<"    * node d'arrivé: "<< end <<std::endl;
    //int start = 1;  // ID du nœud de départ
    //int end = 20;   // ID du nœud d'arrivée
    
    auto pathResult = shortestPath(g, start, end);
    double pathLength = pathResult.first;
    const auto& path = pathResult.second;
    
    std::cout << "Chemin le plus court de " << start << " à " << end << ": ";
    if (pathLength < 0) {
        std::cout << "Pas de chemin trouvé" << std::endl;
    } else {
        std::cout << "Longueur = " << std::fixed << std::setprecision(2) << pathLength << std::endl;
        std::cout << "Chemin: ";
        for (size_t i = 0; i < path.size(); ++i) {
            std::cout << path[i];
            if (i < path.size() - 1) {
                std::cout << " -> ";
            }
        }
        std::cout << std::endl;
        
        // Marquer les arêtes du chemin pour l'illustration
        markPathEdges(g, path);
    }
}

int main(int argc, char* argv[]) {
    // Chemins par défaut des fichiers
    std::string nodes_file = "nodes.csv";
    std::cout << "Veuillez entrer le nom du fichier nodes : "<<std::endl;
    std::cout << "(ex: nodes.csv)"<<std::endl;
    std::getline(std::cin, nodes_file);
    std::string edges_file = "edges.csv";
    std::cout << "Veuillez entrer le nom du fichier edge : "<<std::endl;
    std::cout << "(ex: edges.csv)"<<std::endl;
    std::getline(std::cin, edges_file);
    std::string output_csv = "paths.csv";// ça on ne demande pas
    std::string output_dot = "graph.png";
    std::cout << "Veuillez entrer le nom du fichier graph : "<<std::endl;
    std::cout << "(ex: graph.dot ou graph.png)"<<std::endl;
    std::getline(std::cin, output_dot);
    
    // Utiliser les arguments de ligne de commande si fournis
    if (argc > 1) nodes_file = argv[1];
    if (argc > 2) edges_file = argv[2];
    if (argc > 3) output_csv = argv[3];
    if (argc > 4) output_dot = argv[4];
    
    // Afficher les informations sur la version de Boost
    std::cout << "Utilisation de Boost version " 
              << BOOST_VERSION / 100000 << "." 
              << BOOST_VERSION / 100 % 1000 << "." 
              << BOOST_VERSION % 100 << std::endl;
    
    // Démarrer le chronomètre
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Charger les nœuds
    std::vector<NodeInfo> nodes = loadNodes(nodes_file);
    
    if (nodes.empty()) {
        std::cerr << "Erreur : Aucun nœud chargé." << std::endl;
        return 1;
    }
    
    // Créer le graphe avec le bon nombre de nœuds
    Graph g(nodes.size());
    
    // Associer les informations de nœuds au graphe
    boost::graph_traits<Graph>::vertex_iterator vi, vi_end;
    int i = 0;
    for (boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi, ++i) {
        g[*vi] = nodes[i];
    }
    
    // Charger les arêtes
    loadEdges(g, edges_file);
    
    int node1, node2; // Variables to store the user's input

    // Prompt the user for the first number
    std::cout << "Selection de 2 nodes pour un calcul de chemin" << std::endl;
    std::cout << "Entrer la node de départ: ";
    std::cin >> node1;

    // Prompt the user for the second number
    std::cout << "Entrer la node d'arrivé': ";
    std::cin >> node2;

    // Générer le rapport d'analyse
    generateGraphReport(g, node1, node2);
    
    // v. Générer une illustration du graphe
    generateGraphImage(g, output_dot);
    
    // vi. Écrire les chemins dans un fichier CSV
    // Liste de paires de nœuds pour le calcul des chemins
    std::vector<std::pair<int, int>> nodePairs = {
        {1, 5}, {1, 10}, {1, 15}, {1, 20},
        {5, 10}, {5, 15}, {5, 20},
        {10, 15}, {10, 20},
        {15, 20}
    };
    
    writePathsToCSV(g, nodePairs, output_csv);
    
    // vii. Afficher le temps de calcul
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n== vii. Temps de calcul ==" << std::endl;
    std::cout << "Temps total d'exécution: " << duration.count() << " ms" << std::endl;
    
    return 0;
}