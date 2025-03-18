#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/tokenizer.hpp>

// Structure pour stocker les informations d'un nœud
struct NodeInfo {
    int id;
    double x, y, z;
};

// Définition du type de graphe
typedef boost::adjacency_list<
    boost::vecS,           // Conteneur pour les arêtes sortantes
    boost::vecS,           // Conteneur pour les sommets
    boost::undirectedS,    // Graphe non orienté
    NodeInfo               // Structure de données pour les nœuds
> Graph;

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
                    // Ajouter l'arête au graphe
                    boost::add_edge(source, target, g);
                }
            }
        }
    }
}

// Fonction principale pour générer le rapport d'analyse du graphe
void generateGraphReport(const Graph& g) {
    std::cout << "=== RAPPORT D'ANALYSE DU GRAPHE ===" << std::endl;
    std::cout << "Nombre de nœuds: " << boost::num_vertices(g) << std::endl;
    std::cout << "Nombre d'arêtes: " << boost::num_edges(g) << std::endl;
    
    // Calculer et afficher le degré de chaque nœud
    std::cout << "\nDegré de chaque nœud:" << std::endl;
    std::cout << "----------------------" << std::endl;
    
    int max_degree = 0;
    boost::graph_traits<Graph>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi) {
        int degree = boost::degree(*vi, g);
        std::cout << "Nœud " << g[*vi].id << ": " << degree << std::endl;
        
        if (degree > max_degree) {
            max_degree = degree;
        }
    }
    
    // Calculer et afficher le degré du graphe
    std::cout << "\nDegré du graphe: " << max_degree << std::endl;
}

int main(int argc, char* argv[]) {
    // Chemins par défaut des fichiers
    std::string nodes_file = "nodes.csv";
    std::string edges_file = "edges.csv";
    
    // Utiliser les arguments de ligne de commande si fournis
    if (argc > 1) nodes_file = argv[1];
    if (argc > 2) edges_file = argv[2];
    
    // Afficher les informations sur la version de Boost
    std::cout << "Utilisation de Boost version " 
              << BOOST_VERSION / 100000 << "." 
              << BOOST_VERSION / 100 % 1000 << "." 
              << BOOST_VERSION % 100 << std::endl;
    
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
    
    return 0;
}