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
struct EdgeProperties {
    double weight;
    bool inPath;  // Pour l'illustration de chemin
};

// Définition du type de graphe
typedef boost::adjacency_list<
    boost::vecS,           // Conteneur pour les arêtes sortantes
    boost::vecS,           // Conteneur pour les sommets
    boost::undirectedS,    // Graphe non orienté
    NodeInfo,              // Structure de données pour les nœuds
    EdgeProperties         // Structure de données pour les arêtes
> Graph;

typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

// Visiteur personnalisé pour détecter les cycles
class CycleDetector : public boost::dfs_visitor<> {
public:
    CycleDetector(bool& hasCycle) : _hasCycle(hasCycle) {}

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
    std::vector<NodeInfo> nodes;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier " << filename << std::endl;
        return nodes;
    }
    
    std::string line;
    
    // Ignorer la première ligne (en-têtes)
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        boost::char_separator<char> sep(";");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        
        auto it = tokens.begin();
        if (it != tokens.end()) {
            NodeInfo node;
            node.id = std::stoi(*it++);
            if (it != tokens.end()) node.x = std::stod(*it++);
            if (it != tokens.end()) node.y = std::stod(*it++);
            if (it != tokens.end()) node.z = std::stod(*it++);
            
            nodes.push_back(node);
        }
    }
    
    return nodes;
}

void loadEdges(Graph& g, const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier " << filename << std::endl;
        return;
    }
    
    std::string line;
    
    // Ignorer la première ligne (en-têtes)
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        boost::char_separator<char> sep(";");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        
        auto it = tokens.begin();
        if (it != tokens.end()) {
            int source = std::stoi(*it++) - 1;  // -1 car les ID commencent à 1 dans le fichier
            if (it != tokens.end()) {
                int target = std::stoi(*it++) - 1;  // mais les indices dans le graphe commencent à 0
                
                // Vérifier si les indices sont valides
                if (source >= 0 && source < boost::num_vertices(g) && 
                    target >= 0 && target < boost::num_vertices(g)) {
                    // Calculer le poids de l'arête (distance euclidienne 3D)
                    double dx = g[source].x - g[target].x;
                    double dy = g[source].y - g[target].y;
                    double dz = g[source].z - g[target].z;
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
    int start = startNodeId - 1;  // Convertir de l'ID à l'indice
    int end = endNodeId - 1;
    
    // Vérifier si les indices sont valides
    if (start < 0 || start >= boost::num_vertices(g) || end < 0 || end >= boost::num_vertices(g)) {
        std::cerr << "Erreur : Indices de nœuds invalides." << std::endl;
        return {-1, {}};
    }
    
    // Vecteurs pour stocker les prédécesseurs et les distances
    std::vector<Vertex> predecessor(boost::num_vertices(g));
    std::vector<double> distance(boost::num_vertices(g));
    
    // Exécuter l'algorithme de Dijkstra
    boost::dijkstra_shortest_paths(g, start, 
        boost::predecessor_map(boost::make_iterator_property_map(predecessor.begin(), boost::get(boost::vertex_index, g)))
        .distance_map(boost::make_iterator_property_map(distance.begin(), boost::get(boost::vertex_index, g)))
        .weight_map(boost::get(&EdgeProperties::weight, g)));
    
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
    
    std::cout << "Fichier DOT généré : " << filename << std::endl;
    std::cout << "Pour visualiser le graphe, utilisez Graphviz avec la commande :" << std::endl;
    std::cout << "dot -Tpng " << filename << " -o graph.png" << std::endl;
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
void generateGraphReport(Graph& g) {
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
    
    // ii. Vérifier la connectivité du graphe
    std::cout << "\n== ii. Connectivité du graphe ==" << std::endl;
    bool connected = isConnected(g);
    std::cout << "Le graphe est " << (connected ? "connecté" : "non connecté") << std::endl;
    
    // iii. Détection de cycles
    std::cout << "\n== iii. Détection de cycles ==" << std::endl;
    bool cycle = hasCycle(g);
    std::cout << "Le graphe " << (cycle ? "contient" : "ne contient pas") << " de cycle" << std::endl;
    std::cout << "Explication de l'algorithme de détection de cycles:" << std::endl;
    std::cout << "1. Utilisation d'un parcours en profondeur (DFS) du graphe" << std::endl;
    std::cout << "2. Lors du parcours, nous maintenons un état pour chaque nœud (non visité, en cours, visité)" << std::endl;
    std::cout << "3. Quand nous rencontrons une arête arrière (qui mène à un nœud en cours de visite), un cycle est détecté" << std::endl;
    std::cout << "4. L'implémentation utilise le visiteur boost::dfs_visitor qui détecte les arêtes arrière" << std::endl;
    
    // iv. Exemple de calcul de chemin le plus court
    std::cout << "\n== iv. Exemple de calcul de chemin le plus court ==" << std::endl;
    int start = 1;  // ID du nœud de départ
    int end = 20;   // ID du nœud d'arrivée
    
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
    std::string edges_file = "edges.csv";
    std::string output_csv = "paths.csv";
    std::string output_dot = "graph.dot";
    
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
    
    // Générer le rapport d'analyse
    generateGraphReport(g);
    
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