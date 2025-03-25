# GraphCPP
Ce projet utilise la Boost Graph Library (BGL) pour effectuer diverses analyses de graphes, telles que la vérification de la connectivité, la détection de cycles et le calcul des plus courts chemins. Il intègre également Graphviz pour la visualisation des structures et des résultats des graphes.

## Fonctionnalités principales

- **Connectivité des graphes** : Vérification de la connectivité d'un graphe.
- **Détection de cycles** : Identification des cycles dans les graphes orientés et non orientés.
- **Calcul des plus courts chemins** : Recherche du plus court chemin entre deux nœuds.
- **Visualisation des graphes** : Visualisation des graphes à l’aide de Graphviz.
- **Mesure des performances** : Temps d'exécution des tâches de traitement de graphes.

## Technologies utilisées

- **C++**
- **Boost Graph Library (BGL)**
- **Graphviz**

## installation
```bash
git clone https://github.com/Eratosteme/GraphCPP.git
```
## compilation
#### modifier le path de la lybrairy Boost dans makefile (version par default, la librairy est dans un folder à coter du folder git)
```bash
make
```
## utilisation (paramètres optionelles)
```bash
./source/graph_analysis [node_file] [edges_file] [output_csv] [output_graph]
```
