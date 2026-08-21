/**
 * @file mainwindow_init_panels.cpp
 * @brief Implements MainWindow dock panel construction and window layout.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "texteditor.h"
#include "chart.h"
#include "widgets/filterbarwidget.h"
#include "widgets/graphtablewidget.h"

#include <QtWidgets>
#include <QtCharts>

/**
 * @brief Creates docked panels for instant access to main app functionalities
 * and displaying statistics
 */
void MainWindow::initPanels()
{

    qCDebug(lcMainWindow) << "Initializing panels...";

    //
    // create widgets for the Control Panel
    //

    QString helpMessage = "";

    QLabel *toolBoxNetworkAutoCreateSelectLabel = new QLabel;
    toolBoxNetworkAutoCreateSelectLabel->setText(tr("Auto Create:"));
    toolBoxNetworkAutoCreateSelectLabel->setMinimumWidth(90);
    toolBoxNetworkAutoCreateSelectLabel->setStatusTip(
        tr("Create a network automatically (famous, random, or by using the web crawler)."));
    toolBoxNetworkAutoCreateSelect = new QComboBox;
    toolBoxNetworkAutoCreateSelect->setStatusTip(
        tr("Create a network automatically (famous, random, or by using the web crawler)."));
    helpMessage = tr("<p><b>Auto network creation</b></p> "
                     "<p>Create a new network automatically.</p>"
                     "<p>You may create a random network, recreate famous data-sets "
                     "or use the built-in web crawler to create a network of webpages. </p>");
    toolBoxNetworkAutoCreateSelect->setToolTip(helpMessage);
    toolBoxNetworkAutoCreateSelect->setWhatsThis(helpMessage);

    toolBoxNetworkAutoCreateSelect->setToolTip(helpMessage);
    toolBoxNetworkAutoCreateSelect->setWhatsThis(helpMessage);
    QStringList networkAutoCreateSelectCommands;
    networkAutoCreateSelectCommands << "Select"
                                    << "Famous data sets"
                                    << "Random scale-free"
                                    << "Random small-world"
                                    << "Random Erdős–Rényi"
                                    << "Random lattice"
                                    << "Random d-regular"
                                    << "Random ring-lattice"
                                    << "With Web Crawler";
    toolBoxNetworkAutoCreateSelect->addItems(networkAutoCreateSelectCommands);

    toolBoxNetworkAutoCreateSelect->setMinimumWidth(90);

    QLabel *toolBoxEditNodeSubgraphSelectLabel = new QLabel;
    toolBoxEditNodeSubgraphSelectLabel->setText(tr("Subgraph:"));
    toolBoxEditNodeSubgraphSelectLabel->setMinimumWidth(90);
    toolBoxEditNodeSubgraphSelectLabel->setStatusTip(
        tr("Create a basic subgraph with selected nodes."));
    toolBoxEditNodeSubgraphSelect = new QComboBox;
    toolBoxEditNodeSubgraphSelect->setStatusTip(
        tr("Create a basic subgraph with selected nodes."));
    helpMessage = tr("<p><b>Subgraph creation</b></p> "
                     "<p>Create a basic subgraph from selected nodes.</p>"
                     "<p>Select some nodes with your mouse and then click on one of these"
                     "options to create a basic subgraph with them. </p>"
                     "<p>You can create a star, clique, line, etc subgraph.</p>"
                     "<p>There must be some nodes selected!</p>");
    toolBoxEditNodeSubgraphSelect->setToolTip(helpMessage);
    toolBoxEditNodeSubgraphSelect->setWhatsThis(helpMessage);

    toolBoxEditNodeSubgraphSelectLabel->setToolTip(helpMessage);
    toolBoxEditNodeSubgraphSelectLabel->setWhatsThis(helpMessage);
    QStringList editNodeSubgraphCommands;
    editNodeSubgraphCommands << "Select"
                             << "Clique"
                             << "Star"
                             << "Cycle"
                             << "Line";
    toolBoxEditNodeSubgraphSelect->addItems(editNodeSubgraphCommands);
    toolBoxEditNodeSubgraphSelect->setMinimumWidth(90);

    QLabel *toolBoxEdgeModeSelectLabel = new QLabel;
    toolBoxEdgeModeSelectLabel->setText(tr("Edge Mode:"));
    toolBoxEdgeModeSelectLabel->setMinimumWidth(90);
    toolBoxEditEdgeModeSelect = new QComboBox;
    toolBoxEditEdgeModeSelect->setStatusTip(
        tr("Select the edge mode: directed or undirected."));
    helpMessage = tr("<p><b>Edge mode</b></p>"
                     "<p>In social networks and graphs, edges can be directed or undirected "
                     "(and the corresponding network is called directed or undirected as well).</p>"
                     "<p>This option lets you choose what the kind of edges you want in your network.<p>"
                     "<p>By selecting an option here, all edges of the network will change automatically. <p>"
                     "<p>For instance, if the network is directed and and you select \"undirected\" "
                     "then all the directed edges will become undirected <p>");
    toolBoxEditEdgeModeSelect->setToolTip(helpMessage);
    toolBoxEditEdgeModeSelect->setWhatsThis(helpMessage);
    QStringList edgeModeCommands;
    edgeModeCommands << "Directed"
                     << "Undirected";
    toolBoxEditEdgeModeSelect->addItems(edgeModeCommands);
    toolBoxEditEdgeModeSelect->setMinimumWidth(120);

    QLabel *toolBoxEditEdgeTransformSelectLabel = new QLabel;
    toolBoxEditEdgeTransformSelectLabel->setText(tr("Transform:"));
    toolBoxEditEdgeTransformSelectLabel->setMinimumWidth(90);
    toolBoxEditEdgeTransformSelect = new QComboBox;
    toolBoxEditEdgeTransformSelect->setStatusTip(
        tr("Select a method to transform the network, i.e. transform all directed edges to undirected."));
    helpMessage = tr("<p><b>Transform Network Edges </b></p>"
                     "<p>Select a method to transform network edges. Available methods: </p>"

                     "<p><em>Symmetrize All Edges</em></p>"
                     "<p>Forces all edges in this relation to be reciprocated: "
                     "<p>If there is a directed edge from node A to node B "
                     "then a new directed edge from node B to node A will be "
                     " created, with the same weight. </p>"
                     "<p>The result is a symmetric network.</p>"

                     "<p><em>Symmetrize Edges by Strong Ties:</em></p>"
                     "<p>Creates a new symmetric relation by keeping strong ties only. </p>"
                     "<p>A tie between actors A and B is considered strong if both A -> B and B -> A exist. "
                     "Therefore, in the new relation, a reciprocated edge will be created between actors A and B "
                     "only if both arcs A->B and B->A were present in the current or all relations. </p>"
                     "<p>If the network is multi-relational, it will ask you whether "
                     "ties in the current relation or all relations are to be considered.</p>"

                     "<p><em>Symmetrize Edges by examining Cocitation:</em></p>"
                     "<p>Creates a new symmetric relation by connecting actors "
                     "that are cocitated by others. "
                     "In the new relation, an edge will be created between actor i and "
                     "actor j only if C(i,j) > 0, where C the Cocitation Matrix. </p>"
                     "<p>Thus the actor pairs cited by more common neighbors will appear "
                     "with a stronger tie between them than pairs those cited by fewer "
                     "common neighbors. "
                     "The resulting relation is symmetric.</p>"

                     "<p><em>Dichotomize Edges</em></p>"
                     "<p>Creates a new binary relation in a valued network using "
                     "edge dichotomization according to a given threshold value. "
                     "In the new dichotomized relation, an edge will exist between actor i and "
                     "actor j only if e(i,j) > threshold, where threshold is a user-defined value."
                     "The process is also known as compression and slicing.</p>");
    toolBoxEditEdgeTransformSelect->setToolTip(helpMessage);
    toolBoxEditEdgeTransformSelect->setWhatsThis(helpMessage);

    QStringList edgeTransformCommands;
    edgeTransformCommands << "Select"
                          << "Symmetrize All Ties"
                          << "Symmetrize Strong Ties"
                          << "Cocitation Network"
                          << "Edge Dichotomization";
    toolBoxEditEdgeTransformSelect->addItems(edgeTransformCommands);
    toolBoxEditEdgeTransformSelect->setMinimumWidth(120);

    // create a grid layout for Edit buttons

    QGridLayout *editGrid = new QGridLayout;
    editGrid->addWidget(toolBoxNetworkAutoCreateSelectLabel, 0, 0);
    editGrid->addWidget(toolBoxNetworkAutoCreateSelect, 0, 1);

    editGrid->addWidget(toolBoxEditNodeSubgraphSelectLabel, 1, 0);
    editGrid->addWidget(toolBoxEditNodeSubgraphSelect, 1, 1);
    editGrid->addWidget(toolBoxEdgeModeSelectLabel, 2, 0);
    editGrid->addWidget(toolBoxEditEdgeModeSelect, 2, 1);
    editGrid->addWidget(toolBoxEditEdgeTransformSelectLabel, 3, 0);
    editGrid->addWidget(toolBoxEditEdgeTransformSelect, 3, 1);

    QLabel *toolBoxFilterSelectLabel = new QLabel;
    toolBoxFilterSelectLabel->setText(tr("Filter:"));
    toolBoxFilterSelectLabel->setMinimumWidth(90);
    toolBoxFilterSelect = new QComboBox;
    toolBoxFilterSelect->setStatusTip(
        tr("Select a filter to apply to nodes or edges. Filters are non-destructive and reversible."));
    toolBoxFilterSelect->setToolTip(
        tr("<p><b>Filter Nodes / Edges</b></p>"
           "<p>Apply a non-destructive, reversible filter to the network. Available options:</p>"
           "<p><em>Filter by Centrality</em> — hide nodes below a centrality score threshold.</p>"
           "<p><em>Filter by Attribute</em> — hide nodes or edges whose attribute does not match a condition.</p>"
           "<p><em>Filter Edges by Weight</em> — hide edges below a weight threshold.</p>"
           "<p><em>Restore All Nodes</em> — undo the last node filter.</p>"
           "<p><em>Restore All Edges</em> — undo the last edge filter.</p>"));
    QStringList filterCommands;
    filterCommands << "Select"
                   << "Focus on Node (Ego Network)"
                   << "Focus on Selection"
                   << "Filter Nodes by Centrality"
                   << "Filter Nodes/Edges by Attribute"
                   << "Filter Edges by Weight"
                   << "Restore All Nodes"
                   << "Restore All Edges";
    toolBoxFilterSelect->addItems(filterCommands);
    toolBoxFilterSelect->setMinimumWidth(120);

    editGrid->addWidget(toolBoxFilterSelectLabel, 4, 0);
    editGrid->addWidget(toolBoxFilterSelect, 4, 1);

    editGrid->setSpacing(5);
    editGrid->setContentsMargins(5, 5, 5, 5);

    // create a groupbox "Network" - Inside, display the grid layout of widgets
    QGroupBox *editGroupBox = new QGroupBox(tr("Network"));
    editGroupBox->setLayout(editGrid);
    editGroupBox->setMaximumWidth(255);
    editGroupBox->setMinimumHeight(100);

    // create widgets for the "Analysis" box
    QLabel *toolBoxAnalysisMatricesSelectLabel = new QLabel;
    toolBoxAnalysisMatricesSelectLabel->setText(tr("Matrix:"));
    toolBoxAnalysisMatricesSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisMatricesSelect = new QComboBox;
    toolBoxAnalysisMatricesSelect->setStatusTip(
        tr("Select which matrix to compute and display, based on the "
           "adjacency matrix of the current network."));
    helpMessage = tr("<p><b>Matrix Analysis</b></p>"
                     "<p>Compute and display the adjacency matrix and other matrices "
                     "based on the adjacency matrix of the current network. "
                     "Available options:"
                     "<p><em>Adjacency Matrix</em></p>"
                     "<p><em>Adjacency Matrix Plot</em></p>"
                     "<p><em>Inverse of Adjacency Matrix</em></p>"
                     "<p><em>Transpose of Adjacency Matrix</em></p>"
                     "<p><em>Cocitation Matrix </em></p>"
                     "<p><em>Degree Matrix </em></p>"
                     "<p><em>Laplacian Matrix </em></p>");
    toolBoxAnalysisMatricesSelect->setToolTip(helpMessage);
    toolBoxAnalysisMatricesSelect->setWhatsThis(helpMessage);
    QStringList graphMatricesList;
    graphMatricesList << "Select"
                      << "Adjacency"
                      << "Adjacency Plot"
                      << "Adjacency Inverse"
                      << "Adjacency Transpose"
                      << "Cocitation Matrix"
                      << "Degree Matrix"
                      << "Laplacian Matrix";
    toolBoxAnalysisMatricesSelect->addItems(graphMatricesList);
    toolBoxAnalysisMatricesSelect->setMinimumWidth(120);

    QLabel *toolBoxAnalysisCohesionSelectLabel = new QLabel;
    toolBoxAnalysisCohesionSelectLabel->setText(tr("Cohesion:"));
    toolBoxAnalysisCohesionSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisCohesionSelect = new QComboBox;
    toolBoxAnalysisCohesionSelect->setStatusTip(
        tr("Select a graph-theoretic measure, i.e. distances, walks, graph diameter, eccentricity."));
    helpMessage =
        tr("<p><b>Analyze Cohesion</b></p>"
           "<p><Compute basic graph-theoretic measures. "

           "<p><em>Reciprocity:</em><p>"
           "<p>Measures the likelihood that pairs of nodes in a directed network are mutually linked.</p>"

           "<p><em>Symmetry:</em><p>"
           "<p>Checks if the directed network is symmetric or not.<p>"

           "<p><em>Distances:</em></p>"
           "<p>Computes the matrix of geodesic distances between all pairs of nodes.<p>"

           "<p><em>Average Distance:</em></p>"
           "<p>Computes the average distance between all nodes.<p>"

           "<p><em>Graph Diameter:</em></p>"
           "<p>The maximum distance between any two nodes in the network.</p>"

           "<p><em>Walks:</em></p>"
           "<p>A walk is a sequence of edges and vertices (nodes), where "
           "each edge's endpoints are the two vertices adjacent to it. "
           "In a walk, vertices and edges may repeat."

           "<p><em>Eccentricity:</em></p>"
           "<p>The Eccentricity of each node is how far, at most, is from every other actor in the network.</p>"

           "<p><em>Connectedness:</em></p>"
           "<p>Checks whether the network is connected, i.e. whether a path exists between every "
           "pair of nodes. For directed networks, asks whether to check weak connectivity "
           "(ignoring edge direction) or strong connectivity (respecting it).</p>"

           "<p><em>Node Connectivity:</em></p>"
           "<p>The minimum number of nodes that must be removed to disconnect two chosen actors. "
           "For directed networks, asks whether to respect edge direction (strong) or ignore it "
           "(weak).</p>"

           "<p><em>Graph Connectivity:</em></p>"
           "<p>The network's overall vertex connectivity: the fewest nodes that would need to be "
           "removed to disconnect it at its weakest point. Same weak/strong choice as Node "
           "Connectivity for directed networks.</p>"

           "<p><em>Reachability:</em></p>"
           "<p>Creates a matrix where an element (i,j) = 1 only if the actors i and j are reachable.</p>"

           "<p><em>Clustering Coefficient (CLC):</em></p>"
           "<p>The CLC score of each node  is the proportion of actual links "
           "between its neighbors divided by the number of links that could "
           "possibly exist between them. "
           "Quantifies how close each actor and its neighbors are to form "
           "a complete subgraph (clique)</p>");
    toolBoxAnalysisCohesionSelect->setToolTip(helpMessage);
    toolBoxAnalysisCohesionSelect->setWhatsThis(helpMessage);

    QStringList graphPropertiesList;
    graphPropertiesList << "Select"
                        << "Reciprocity"
                        << "Symmetry"
                        << "Distance"
                        << "Average Distance"
                        << "Geodesic Distribution"
                        << "Distances Matrix"
                        << "Geodesics Matrix"
                        << "Eccentricity"
                        << "Diameter"
                        << "Connectedness"
                        << "Walks of given length"
                        << "Total Walks"
                        << "Reachability Matrix"
                        << "Clustering Coefficient"
                        << "Node Connectivity"
                        << "Graph Connectivity";
    toolBoxAnalysisCohesionSelect->addItems(graphPropertiesList);
    toolBoxAnalysisCohesionSelect->setMinimumWidth(120);

    QLabel *toolBoxAnalysisProminenceSelectLabel = new QLabel;
    toolBoxAnalysisProminenceSelectLabel->setText(tr("Prominence:"));
    toolBoxAnalysisProminenceSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisProminenceSelect = new QComboBox;
    toolBoxAnalysisProminenceSelect->setStatusTip(
        tr("Select a prominence metric to compute for each actor "
           "and the whole network. "));
    helpMessage = tr("<p><b>Prominence Analysis</b></p>"
                     "<p>Compute Centrality and Prestige indices, to measure how "
                     "<em>prominent</em> (important) "
                     "each actor (node) is inside the network. </p>"
                     "<p>Centrality measures quantify how central is each node by examining "
                     "its ties and its geodesic distances (shortest path lengths) to other nodes. "
                     "Most Centrality indices were designed for undirected graphs. </p>"

                     "<p>Prestige indices focus on \"choices received\" to a node. "
                     "These indices measure the nominations or ties to each node from all others (or inLinks). "
                     "Prestige indices are suitable (and can be calculated only) on directed graphs.</p>"

                     "<p>Available measures:</p>"

                     "<p><em>Degree Centrality (DC) </em></p>"
                     "<p>The sum of outbound edges or the sum of weights of outbound "
                     "edges from each node <em>i</em> to all adjacent nodes. Note: This is "
                     "the outDegree Centrality. To compute inDegree Centrality, "
                     "use the Degree Prestige measure.</p>"

                     "<p><em>Closeness Centrality (CC):</em></p>"
                     "The inverted sum of geodesic distances from each node <em>u</em> "
                     "to all other nodes. "

                     "<p><em>IR Closeness Centrality (IRCC):</em></p>"
                     "<p>The ratio of the fraction of nodes reachable by each node <em>u</em> "
                     "to the average distance of these nodes from <em>u</em>.</p>"

                     "<p><em>Betweenness Centrality (BC):</em></p>"
                     "<p>The sum of delta<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "delta<sub>(s,t,u)</sub> is the ratio of all geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Stress Centrality (SC):</em></p>"
                     "<p>The sum of sigma<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "sigma<sub>(s,t,u)</sub> is the number of geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Eccentricity Centrality (EC):</em></p>"
                     "<p>Also known as Harary Graph Centrality. The inverse maximum geodesic distance from node <em>u</em> to "
                     "all other nodes in the network."

                     "<p><em>Power Centrality (PC):</em></p>"
                     "<p>The sum of the sizes of all N<sub>th</sub>-order neighbourhoods "
                     "of node <em>u</em> with weight 1/n.</p>"

                     "<p><em>Information Centrality (IC):</em></p>"
                     "<p>Measures the information flow through all paths between actors weighted by "
                     "strength of tie and distance.</p>"

                     "<p><em>Eigenvector Centrality (EVC):</em></p>"
                     "<p>The EVC score of each node <em>i</em> is the i<sub>th</sub> element of the "
                     "leading eigenvector of the adjacency matrix, that is the "
                     "eigenvector corresponding to the largest positive eigenvalue. "

                     "<p><em>Degree Prestige (DP):</em></p>"
                     "<p>Also known as InDegree Centrality, it is the sum of inbound edges to a node <em>u</em> "
                     "from all adjacent nodes. </p>"

                     "<p><em>PageRank Prestige (PRP):</em></p>"
                     "<p>For each node <em>u</em> counts all inbound links (edges) to it, but "
                     "it normalizes each inbound link from another node <em>v</em> by the outDegree of <em>v</em>. </p>"

                     "<p><em>Proximity Prestige (PP):</em></p>"
                     "<p>The ratio of the proportion of nodes who can reach each node <em>u</em> "
                     "to the average distance these nodes are from it. Similar to Closeness Centrality "
                     "but it counts only inbound distances to each actor, thus it is a measure of actor prestige.</p>");
    toolBoxAnalysisProminenceSelect->setToolTip(helpMessage);
    toolBoxAnalysisProminenceSelect->setWhatsThis(helpMessage);

    // Used in toolBoxAnalysisProminenceSelect and DialogNodeFind
    prominenceIndexList << "Degree Centrality"
                        << "Closeness Centrality"
                        << "IR Closeness Centrality"
                        << "Betweenness Centrality"
                        << "Stress Centrality"
                        << "Eccentricity Centrality"
                        << "Power Centrality"
                        << "Information Centrality"
                        << "Eigenvector Centrality"
                        << "Degree Prestige"
                        << "PageRank Prestige"
                        << "Proximity Prestige"
                        << "Clustering Coefficient";

    // Clustering Coefficient is excluded here: toolBoxAnalysisProminenceSelectChanged()
    // has no case for it (the Cohesion dropdown already offers it, via the same
    // slotAnalyzeClusteringCoefficient() - a 3rd entry point would be pure duplication).
    // prominenceIndexList itself keeps it, for Node Find / layout-by-index / filter-by-
    // centrality, which all handle it correctly via their own dedicated code paths.
    QStringList prominenceCommands;
    prominenceCommands << "Select" << prominenceIndexList;
    prominenceCommands.removeAll("Clustering Coefficient");
    toolBoxAnalysisProminenceSelect->addItems(prominenceCommands);
    toolBoxAnalysisProminenceSelect->setMinimumWidth(120);

    QLabel *toolBoxAnalysisCommunitiesSelectLabel = new QLabel;
    toolBoxAnalysisCommunitiesSelectLabel->setText(tr("Communities:"));
    toolBoxAnalysisCommunitiesSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisCommunitiesSelect = new QComboBox;
    toolBoxAnalysisCommunitiesSelect->setStatusTip(
        tr("Select a community detection measure / cohesive subgroup algorithm, i.e. cliques, triad census etc."));
    helpMessage = tr("<p><b>Community Analysis</b></p>"
                     "<p>Community detection measures and cohesive subgroup algorithms, "
                     "to identify meaningful subgraphs in the graph.</p>"
                     "<p><b>Available measures</b></p>"
                     "<p><em>Clique Census:</em><p>"
                     "<p>Computes aggregate counts of all maximal cliques of actors by size, "
                     " actor by clique analysis, clique co-memberships</p>"
                     "<p><em>Triad Census:</em><p>"
                     "<p>Computes the Holland, Leinhardt and Davis triad census, which "
                     "counts all different classes of triads coded according to their"
                     "number of Mutual, Asymmetric and Non-existest dyads (M-A-N scheme)</p>");
    toolBoxAnalysisCommunitiesSelect->setToolTip(helpMessage);
    toolBoxAnalysisCommunitiesSelect->setWhatsThis(helpMessage);
    QStringList communitiesCommands;
    communitiesCommands << "Select"
                        << "Cliques"
                        << "Triad Census";
    toolBoxAnalysisCommunitiesSelect->addItems(communitiesCommands);
    toolBoxAnalysisCommunitiesSelect->setMinimumWidth(120);

    QLabel *toolBoxAnalysisStrEquivalenceSelectLabel = new QLabel;
    toolBoxAnalysisStrEquivalenceSelectLabel->setText(tr("Equivalence:"));
    toolBoxAnalysisStrEquivalenceSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisStrEquivalenceSelect = new QComboBox;
    toolBoxAnalysisStrEquivalenceSelect->setStatusTip(
        tr("Select a method to measure structural equivalence, "
           "i.e. Pearson Coefficients, tie profile similarities, "
           "hierarchical clustering, etc."));
    helpMessage = tr("<p><b>Structural Equivalence Analysis</b></p>"
                     "<p>Select one of the available structural equivalence "
                     "measures and visualization algorithms. <p>"
                     "<p>Available options</p>"
                     "<p><em>Pearson Coefficients<.em></p>"
                     "<p><em>Tie profile similarities</em></p>"
                     "<p><em>Dissimilarities</em></p>"
                     "<p><em>Hierarchical Clustering Analysis</em></p>");
    toolBoxAnalysisStrEquivalenceSelect->setToolTip(helpMessage);
    toolBoxAnalysisStrEquivalenceSelect->setWhatsThis(helpMessage);
    QStringList connectivityCommands;
    connectivityCommands << "Select"
                         << "Pearson Coefficients"
                         << "Similarities"
                         << "Dissimilarities"
                         << "Hierarchical Clustering";
    toolBoxAnalysisStrEquivalenceSelect->addItems(connectivityCommands);
    toolBoxAnalysisStrEquivalenceSelect->setMinimumWidth(120);

    // create layout for analysis options
    QGridLayout *analysisGrid = new QGridLayout();
    analysisGrid->addWidget(toolBoxAnalysisMatricesSelectLabel, 0, 0);
    analysisGrid->addWidget(toolBoxAnalysisMatricesSelect, 0, 1);
    analysisGrid->addWidget(toolBoxAnalysisCohesionSelectLabel, 1, 0);
    analysisGrid->addWidget(toolBoxAnalysisCohesionSelect, 1, 1);
    analysisGrid->addWidget(toolBoxAnalysisProminenceSelectLabel, 2, 0);
    analysisGrid->addWidget(toolBoxAnalysisProminenceSelect, 2, 1);
    analysisGrid->addWidget(toolBoxAnalysisCommunitiesSelectLabel, 3, 0);
    analysisGrid->addWidget(toolBoxAnalysisCommunitiesSelect, 3, 1);
    analysisGrid->addWidget(toolBoxAnalysisStrEquivalenceSelectLabel, 4, 0);
    analysisGrid->addWidget(toolBoxAnalysisStrEquivalenceSelect, 4, 1);

    analysisGrid->setSpacing(5);
    analysisGrid->setContentsMargins(5, 5, 5, 5);

    // create a box and set the above layout inside
    QGroupBox *analysisBox = new QGroupBox(tr("Analyze"));
    analysisBox->setMinimumHeight(120);
    analysisBox->setMaximumWidth(255);
    analysisBox->setLayout(analysisGrid);

    // create widgets for the "Visualization By Index" box
    QLabel *toolBoxLayoutByIndexSelectLabel = new QLabel;
    toolBoxLayoutByIndexSelectLabel->setText(tr("Index:"));
    toolBoxLayoutByIndexSelectLabel->setMinimumWidth(70);
    toolBoxLayoutByIndexSelect = new QComboBox;
    toolBoxLayoutByIndexSelect->setStatusTip(tr("Select a prominence-based layout model"));
    helpMessage = tr("<p><b>Visualize by prominence index</b></p>"
                     "<p>Apply a prominence-based layout model to the network.</p>"
                     "<p>For instance, you can apply a degree centrality layout. </p>"

                     "<p>Note: For each prominence index, you must select a layout type (below).</p>"

                     "<p>Available measures:</p>"

                     "<p><em>Degree Centrality (DC) </em></p>"
                     "<p>The sum of outbound edges or the sum of weights of outbound "
                     "edges from each node <em>i</em> to all adjacent nodes. Note: This is "
                     "the outDegree Centrality. To compute inDegree Centrality, "
                     "use the Degree Prestige measure.</p>"

                     "<p><em>Closeness Centrality (CC):</em></p>"
                     "The inverted sum of geodesic distances from each node <em>u</em> "
                     "to all other nodes. "

                     "<p><em>IR Closeness Centrality (IRCC):</em></p>"
                     "<p>The ratio of the fraction of nodes reachable by each node <em>u</em> "
                     "to the average distance of these nodes from <em>u</em>.</p>"

                     "<p><em>Betweenness Centrality (BC):</em></p>"
                     "<p>The sum of delta<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "delta<sub>(s,t,u)</sub> is the ratio of all geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Stress Centrality (SC):</em></p>"
                     "<p>The sum of sigma<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "sigma<sub>(s,t,u)</sub> is the number of geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Eccentricity Centrality (EC):</em></p>"
                     "<p>Also known as Harary Graph Centrality. The inverse maximum geodesic distance from node <em>u</em> to "
                     "all other nodes in the network."

                     "<p><em>Power Centrality (PC):</em></p>"
                     "<p>The sum of the sizes of all N<sub>th</sub>-order neighbourhoods "
                     "of node <em>u</em> with weight 1/n.</p>"

                     "<p><em>Information Centrality (IC):</em></p>"
                     "<p>Measures the information flow through all paths between actors weighted by "
                     "strength of tie and distance.</p>"

                     "<p><em>Eigenvector Centrality (EVC):</em></p>"
                     "<p>The EVC score of each node <em>i</em> is the i<sub>th</sub> element of the "
                     "leading eigenvector of the adjacency matrix, that is the "
                     "eigenvector corresponding to the largest positive eigenvalue. "

                     "<p><em>Degree Prestige (DP):</em></p>"
                     "<p>Also known as InDegree Centrality, it is the sum of inbound edges to a node <em>u</em> "
                     "from all adjacent nodes. </p>"

                     "<p><em>PageRank Prestige (PRP):</em></p>"
                     "<p>For each node <em>u</em> counts all inbound links (edges) to it, but "
                     "it normalizes each inbound link from another node <em>v</em> by the outDegree of <em>v</em>. </p>"

                     "<p><em>Proximity Prestige (PP):</em></p>"
                     "<p>The ratio of the proportion of nodes who can reach each node <em>u</em> "
                     "to the average distance these nodes are from it. Similar to Closeness Centrality "
                     "but it counts only inbound distances to each actor, thus it is a measure of actor prestige.</p>");
    toolBoxLayoutByIndexSelect->setToolTip(helpMessage);
    toolBoxLayoutByIndexSelect->setWhatsThis(helpMessage);
    QStringList layoutCommandsList;
    layoutCommandsList << "None" << "Random" << prominenceIndexList;

    toolBoxLayoutByIndexSelect->addItems(layoutCommandsList);
    toolBoxLayoutByIndexSelect->setMinimumWidth(100);

    QLabel *toolBoxLayoutByIndexTypeLabel = new QLabel;
    toolBoxLayoutByIndexTypeLabel->setText(tr("Type:"));
    toolBoxLayoutByIndexTypeLabel->setMinimumWidth(70);
    toolBoxLayoutByIndexTypeSelect = new QComboBox;
    toolBoxLayoutByIndexTypeSelect->setStatusTip(
        tr("Select layout type for the selected model"));
    helpMessage = tr("<p><b>Layout Type</b></p>"
                     "</p>Select a layout type (radial, level, node size or node color) "
                     "for the selected prominence-based model you want to apply to the "
                     "network. Please note that node coloring works only for basic shapes "
                     "(box, circle, etc) not for image icons.</p>");
    toolBoxLayoutByIndexTypeSelect->setToolTip(helpMessage);
    toolBoxLayoutByIndexTypeSelect->setWhatsThis(helpMessage);
    QStringList layoutTypes;
    layoutTypes << "None" << "Radial" << "On Levels" << "Node Size" << "Node Color";
    toolBoxLayoutByIndexTypeSelect->addItems(layoutTypes);
    toolBoxLayoutByIndexTypeSelect->setMinimumWidth(100);

    // create layout for visualisation by index options
    QGridLayout *layoutByIndexGrid = new QGridLayout();
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexSelectLabel, 0, 0);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexSelect, 0, 1);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexTypeLabel, 1, 0);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexTypeSelect, 1, 1);
    layoutByIndexGrid->setSpacing(5);
    layoutByIndexGrid->setContentsMargins(5, 5, 5, 5);

    // create a box and set the above layout inside
    QGroupBox *layoutByIndexBox = new QGroupBox(tr("By Prominence Index"));
    layoutByIndexBox->setMinimumHeight(80);
    helpMessage = tr("<p><b>Visualize by prominence index</b/></p>"
                     "<p>Apply a prominence-based layout model to the network. </p>"
                     "<p>For instance, you can apply a Degree Centrality layout. </p>"
                     "<p>For each prominence index, you must select a layout type:</p>"
                     "<p>Radial, Levels, NodeSize or NodeColor.</p>"
                     "<p>Please note that node coloring works only for basic shapes "
                     "(box, circle, etc) not for image icons.</p>");
    layoutByIndexBox->setToolTip(helpMessage);
    layoutByIndexBox->setMaximumWidth(255);
    layoutByIndexBox->setLayout(layoutByIndexGrid);

    // create widgets for the "Force-Directed Models" Box
    QLabel *toolBoxLayoutForceDirectedSelectLabel = new QLabel;
    toolBoxLayoutForceDirectedSelectLabel->setText(tr("Model:"));
    toolBoxLayoutForceDirectedSelectLabel->setMinimumWidth(70);
    toolBoxLayoutForceDirectedSelect = new QComboBox;
    QStringList modelsList;
    modelsList << tr("None")
               << tr("Kamada-Kawai")
               << tr("Fruchterman-Reingold")
               << tr("Eades Spring Embedder");

    toolBoxLayoutForceDirectedSelect->addItems(modelsList);
    toolBoxLayoutForceDirectedSelect->setMinimumWidth(100);
    toolBoxLayoutForceDirectedSelect->setStatusTip(
        tr("Select a Force-Directed layout model. "));
    helpMessage = tr("<p><b>Visualize by a Force-Directed Placement layout model.</b></p> "
                     "<p>Available models: </p>"

                     "<p><em>Kamada-Kawai</em></p>"
                     "<p>The best variant of the Spring Embedder family of models. "
                     "<p>In this the graph is considered to be a dynamic system where "
                     "every edge is between two actors is a 'spring' of a desirable "
                     "length, which corresponds to their graph theoretic distance. </p>"
                     "<p>In this way, the optimal layout of the graph \n"
                     "is the state with the minimum imbalance. The degree of "
                     "imbalance is formulated as the total spring energy: "
                     "the square summation of the differences between desirable "
                     "distances and real ones for all pairs of vertices.</p>"

                     "<p><em>Fruchterman-Reingold:</em></p>"
                     "<p>In this model, the vertices behave as atomic particles "
                     "or celestial bodies, exerting attractive and repulsive "
                     "forces to each other. Again, only vertices that are "
                     "neighbours  attract each other but, unlike Eades Spring "
                     "Embedder, all vertices repel each other.</p>"

                     "<p><em>Eades Spring Embedder:</em></p>"
                     "<p>A spring-gravitational model, where each node is "
                     "regarded as physical object (ring) repelling all other non-adjacent "
                     "nodes, while springs between connected nodes attract them.</p>"

    );
    toolBoxLayoutForceDirectedSelect->setToolTip(helpMessage);
    toolBoxLayoutForceDirectedSelect->setWhatsThis(helpMessage);

    // create layout for dynamic visualisation
    QGridLayout *layoutForceDirectedGrid = new QGridLayout();
    layoutForceDirectedGrid->addWidget(toolBoxLayoutForceDirectedSelectLabel, 0, 0);
    layoutForceDirectedGrid->addWidget(toolBoxLayoutForceDirectedSelect, 0, 1);
    layoutForceDirectedGrid->setSpacing(5);
    layoutForceDirectedGrid->setContentsMargins(5, 5, 5, 5);

    // create a box for dynamic layout options
    QGroupBox *layoutDynamicBox = new QGroupBox(tr("By Force-Directed Model"));
    layoutDynamicBox->setMinimumHeight(90);
    layoutDynamicBox->setMaximumWidth(255);
    layoutDynamicBox->setLayout(layoutForceDirectedGrid);
    layoutDynamicBox->setContentsMargins(5, 5, 5, 5);

    // Parent box with vertical layout for all layout/visualization boxes
    QVBoxLayout *visualizationBoxLayout = new QVBoxLayout;
    visualizationBoxLayout->addWidget(layoutByIndexBox);
    visualizationBoxLayout->addWidget(layoutDynamicBox);
    visualizationBoxLayout->setContentsMargins(5, 5, 5, 5);

    QGroupBox *visualizationBox = new QGroupBox(tr("Layout"));
    visualizationBox->setMaximumWidth(255);
    visualizationBox->setLayout(visualizationBoxLayout);
    visualizationBox->setContentsMargins(5, 5, 5, 5);

    // Parent box with vertical layout for all boxes of Controls
    QGridLayout *controlGrid = new QGridLayout;
    controlGrid->addWidget(editGroupBox, 0, 0);
    controlGrid->addWidget(analysisBox, 1, 0);
    controlGrid->addWidget(visualizationBox, 2, 0);
    controlGrid->setRowStretch(3, 1); // fix stretch
    controlGrid->setContentsMargins(5, 5, 5, 5);
    // create a box with title
    leftPanel = new QGroupBox(tr("Control Panel"));
    leftPanel->setMinimumWidth(240);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setLayout(controlGrid);

    //
    // Create widgets for Properties/Statistics group/tab
    //

    // Helper: uniform collapsible section toggle button
    auto makeSectionBtn = [this](const QString &title) -> QPushButton *
    {
        QPushButton *btn = new QPushButton(tr("▾ ") + title, this);
        btn->setObjectName("sectionToggleBtn");
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setFlat(true);
        return btn;
    };

    // ── Network section ───────────────────────────────────────────────────────
    m_networkToggleBtn = makeSectionBtn(tr("NETWORK"));

    QLabel *rightPanelNetworkTypeLabel = new QLabel("Type:");
    rightPanelNetworkTypeLabel->setStatusTip(
        tr("The type of the network: directed or undirected. "
           "Toggle the menu option Edit->Edges->Undirected Edges to change it"));
    rightPanelNetworkTypeLabel->setToolTip(
        tr("The loaded network, if any, is directed and \n"
           "any link you add between nodes will be a directed arc.\n"
           "If you want to work with undirected edges and/or \n"
           "transform the loaded network (if any) to undirected \n"
           "toggle the option Edit->Edges->Undirected \n"
           "or press CTRL+E+U"));

    rightPanelNetworkTypeLCD = new QLabel;
    rightPanelNetworkTypeLCD->setAlignment(Qt::AlignRight);
    rightPanelNetworkTypeLCD->setText(tr("Directed"));
    rightPanelNetworkTypeLCD->setStatusTip(
        tr("Directed data mode. "
           "Toggle the menu option Edit->Edges->Undirected Edges to change it"));
    rightPanelNetworkTypeLCD->setToolTip(
        tr("The loaded network, if any, is directed and \n"
           "any link you add between nodes will be a directed arc.\n"
           "If you want to work with undirected edges and/or \n"
           "transform the loaded network (if any) to undirected \n"
           "toggle the option Edit->Edges->Undirected."));

    QLabel *rightPanelNodesLabel = new QLabel(tr("Nodes:"));
    rightPanelNodesLabel->setStatusTip(tr("Each actor in a social network is visualized as a node (aka vertex)."));
    rightPanelNodesLabel->setToolTip(tr("<p><b>Nodes</b></p>"
                                        "<p>Total number of actors (nodes/vertices) in this social network.</p>"));

    rightPanelNodesLCD = new QLabel;
    rightPanelNodesLCD->setAlignment(Qt::AlignRight);
    rightPanelNodesLCD->setStatusTip(tr("The total number of actors (aka nodes or vertices) in the social network."));

    rightPanelEdgesLabel = new QLabel(tr("Arcs:"));
    rightPanelEdgesLabel->setStatusTip(tr("Each link between a pair of actors is visualized as an edge or arc."));
    rightPanelEdgesLabel->setToolTip(tr("<p><b>Edges</b></p>"
                                        "Each link between actors is visualized as an undirected edge or a directed arc."));

    rightPanelEdgesLCD = new QLabel;
    rightPanelEdgesLCD->setAlignment(Qt::AlignRight);
    rightPanelEdgesLCD->setStatusTip(tr("The total number of directed edges in the social network."));

    QLabel *rightPanelDensityLabel = new QLabel(tr("Density:"));
    helpMessage = tr("<p><b>Density</b></p>"
                     "<p>The density <em>d</em> is the ratio of existing edges to all possible edges ( n*(n-1) ).</p>");
    rightPanelDensityLabel->setStatusTip(tr("The density d is the ratio of existing edges to all possible edges"));
    rightPanelDensityLabel->setToolTip(helpMessage);

    rightPanelDensityLCD = new QLabel;
    rightPanelDensityLCD->setAlignment(Qt::AlignRight);
    rightPanelDensityLCD->setStatusTip(tr("The network density, the ratio of existing edges to all possible edges."));

    QGridLayout *networkGrid = new QGridLayout;
    networkGrid->setSpacing(3);
    networkGrid->setContentsMargins(0, 2, 0, 4);
    networkGrid->addWidget(rightPanelNetworkTypeLabel, 0, 0);
    networkGrid->addWidget(rightPanelNetworkTypeLCD, 0, 1);
    networkGrid->addWidget(rightPanelNodesLabel, 1, 0);
    networkGrid->addWidget(rightPanelNodesLCD, 1, 1);
    networkGrid->addWidget(rightPanelEdgesLabel, 2, 0);
    networkGrid->addWidget(rightPanelEdgesLCD, 2, 1);
    networkGrid->addWidget(rightPanelDensityLabel, 3, 0);
    networkGrid->addWidget(rightPanelDensityLCD, 3, 1);
    m_networkSection = new QWidget;
    m_networkSection->setLayout(networkGrid);

    // ── Selection section ─────────────────────────────────────────────────────
    m_selectionToggleBtn = makeSectionBtn(tr("SELECTION"));

    QLabel *rightPanelSelectedNodesLabel = new QLabel(tr("Nodes:"));
    rightPanelSelectedNodesLabel->setStatusTip(tr("Selected nodes."));

    rightPanelSelectedNodesLCD = new QLabel("0");
    rightPanelSelectedNodesLCD->setAlignment(Qt::AlignRight);
    rightPanelSelectedNodesLCD->setStatusTip(tr("The number of selected nodes (vertices)."));

    rightPanelSelectedEdgesLabel = new QLabel(tr("Arcs:"));
    rightPanelSelectedEdgesLabel->setStatusTip(tr("Selected edges."));

    rightPanelSelectedEdgesLCD = new QLabel("0");
    rightPanelSelectedEdgesLCD->setAlignment(Qt::AlignRight);
    rightPanelSelectedEdgesLCD->setStatusTip(tr("The number of selected edges."));

    QGridLayout *selectionGrid = new QGridLayout;
    selectionGrid->setSpacing(3);
    selectionGrid->setContentsMargins(0, 2, 0, 4);
    selectionGrid->addWidget(rightPanelSelectedNodesLabel, 0, 0);
    selectionGrid->addWidget(rightPanelSelectedNodesLCD, 0, 1);
    selectionGrid->addWidget(rightPanelSelectedEdgesLabel, 1, 0);
    selectionGrid->addWidget(rightPanelSelectedEdgesLCD, 1, 1);
    m_selectionSection = new QWidget;
    m_selectionSection->setLayout(selectionGrid);

    // ── Clicked Node section ──────────────────────────────────────────────────
    m_clickedNodeToggleBtn = makeSectionBtn(tr("CLICKED NODE"));

    QLabel *rightPanelClickedNodeLabel = new QLabel(tr("Number:"));
    rightPanelClickedNodeLabel->setToolTip(tr("The node number of the last clicked node."));
    rightPanelClickedNodeLabel->setStatusTip(tr("The node number of the last clicked node. Zero means no node clicked."));

    rightPanelClickedNodeLCD = new QLabel;
    rightPanelClickedNodeLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedNodeLCD->setToolTip(tr("Node number of the last clicked node. Zero when nothing is clicked."));

    rightPanelClickedNodeInDegreeLabel = new QLabel(tr("In-Degree:"));
    rightPanelClickedNodeInDegreeLabel->setToolTip(tr("The inDegree of a node is the sum of all inbound edge weights."));
    rightPanelClickedNodeInDegreeLCD = new QLabel;
    rightPanelClickedNodeInDegreeLCD->setAlignment(Qt::AlignRight);

    rightPanelClickedNodeOutDegreeLabel = new QLabel(tr("Out-Degree:"));
    rightPanelClickedNodeOutDegreeLabel->setToolTip(tr("The outDegree of a node is the sum of all outbound edge weights."));
    rightPanelClickedNodeOutDegreeLCD = new QLabel;
    rightPanelClickedNodeOutDegreeLCD->setAlignment(Qt::AlignRight);

    // Detail rows hidden until a node is clicked
    rightPanelClickedNodeInDegreeLabel->setVisible(false);
    rightPanelClickedNodeInDegreeLCD->setVisible(false);
    rightPanelClickedNodeOutDegreeLabel->setVisible(false);
    rightPanelClickedNodeOutDegreeLCD->setVisible(false);

    QGridLayout *clickedNodeGrid = new QGridLayout;
    clickedNodeGrid->setSpacing(3);
    clickedNodeGrid->setContentsMargins(0, 2, 0, 4);
    clickedNodeGrid->addWidget(rightPanelClickedNodeLabel, 0, 0);
    clickedNodeGrid->addWidget(rightPanelClickedNodeLCD, 0, 1);
    clickedNodeGrid->addWidget(rightPanelClickedNodeInDegreeLabel, 1, 0);
    clickedNodeGrid->addWidget(rightPanelClickedNodeInDegreeLCD, 1, 1);
    clickedNodeGrid->addWidget(rightPanelClickedNodeOutDegreeLabel, 2, 0);
    clickedNodeGrid->addWidget(rightPanelClickedNodeOutDegreeLCD, 2, 1);
    m_clickedNodeSection = new QWidget;
    m_clickedNodeSection->setLayout(clickedNodeGrid);

    // ── Clicked Edge section ──────────────────────────────────────────────────
    m_clickedEdgeToggleBtn = makeSectionBtn(tr("CLICKED EDGE"));

    rightPanelClickedEdgeNameLabel = new QLabel(tr("Name:"));
    rightPanelClickedEdgeNameLabel->setToolTip(tr("The name of the last clicked edge."));
    rightPanelClickedEdgeNameLCD = new QLabel;
    rightPanelClickedEdgeNameLCD->setAlignment(Qt::AlignRight);

    rightPanelClickedEdgeWeightLabel = new QLabel(tr("Weight:"));
    rightPanelClickedEdgeWeightLabel->setStatusTip(tr("The weight of the clicked edge."));
    rightPanelClickedEdgeWeightLCD = new QLabel;
    rightPanelClickedEdgeWeightLCD->setAlignment(Qt::AlignRight);

    rightPanelClickedEdgeReciprocalWeightLabel = new QLabel;
    rightPanelClickedEdgeReciprocalWeightLabel->setToolTip(tr("The weight of the reciprocal edge."));
    rightPanelClickedEdgeReciprocalWeightLCD = new QLabel;
    rightPanelClickedEdgeReciprocalWeightLCD->setAlignment(Qt::AlignRight);

    // Detail rows hidden until an edge is clicked
    rightPanelClickedEdgeWeightLabel->setVisible(false);
    rightPanelClickedEdgeWeightLCD->setVisible(false);
    rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
    rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);

    QGridLayout *clickedEdgeGrid = new QGridLayout;
    clickedEdgeGrid->setSpacing(3);
    clickedEdgeGrid->setContentsMargins(0, 2, 0, 4);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeNameLabel, 0, 0);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeNameLCD, 0, 1);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeWeightLabel, 1, 0);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeWeightLCD, 1, 1);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeReciprocalWeightLabel, 2, 0);
    clickedEdgeGrid->addWidget(rightPanelClickedEdgeReciprocalWeightLCD, 2, 1);
    m_clickedEdgeSection = new QWidget;
    m_clickedEdgeSection->setLayout(clickedEdgeGrid);

    // ── Distribution (chart) section ──────────────────────────────────────────
    m_chartToggleBtn = makeSectionBtn(tr("DISTRIBUTION"));

    miniChart = new Chart(this);
    int chartHeight = 100;
    miniChart->setThemeSmallWidget(chartHeight, chartHeight);

    // ── Outer grid ────────────────────────────────────────────────────────────
    QGridLayout *propertiesGrid = new QGridLayout;
    propertiesGrid->setSpacing(2);
    propertiesGrid->setContentsMargins(4, 4, 4, 4);

    propertiesGrid->addWidget(m_networkToggleBtn, 0, 0);
    propertiesGrid->addWidget(m_networkSection, 1, 0);
    propertiesGrid->addWidget(m_selectionToggleBtn, 2, 0);
    propertiesGrid->addWidget(m_selectionSection, 3, 0);
    propertiesGrid->addWidget(m_clickedNodeToggleBtn, 4, 0);
    propertiesGrid->addWidget(m_clickedNodeSection, 5, 0);
    propertiesGrid->addWidget(m_clickedEdgeToggleBtn, 6, 0);
    propertiesGrid->addWidget(m_clickedEdgeSection, 7, 0);
    propertiesGrid->addWidget(m_chartToggleBtn, 8, 0);
    propertiesGrid->addWidget(miniChart, 9, 0);
    propertiesGrid->setRowMinimumHeight(9, (int)floor(1.5 * chartHeight));
    propertiesGrid->setRowStretch(10, 1);

    // Toggle connections
    auto connectToggle = [](QPushButton *btn, QWidget *section,
                            const QString &title)
    {
        QObject::connect(btn, &QPushButton::clicked, btn, [btn, section, title]()
                         {
            const bool show = !section->isVisible();
            section->setVisible(show);
            btn->setText((show ? QString("▾ ") : QString("▴ ")) + title); });
    };
    connectToggle(m_networkToggleBtn, m_networkSection, tr("NETWORK"));
    connectToggle(m_selectionToggleBtn, m_selectionSection, tr("SELECTION"));
    connectToggle(m_clickedNodeToggleBtn, m_clickedNodeSection, tr("CLICKED NODE"));
    connectToggle(m_clickedEdgeToggleBtn, m_clickedEdgeSection, tr("CLICKED EDGE"));
    connectToggle(m_chartToggleBtn, miniChart, tr("DISTRIBUTION"));

    // Create a panel with title
    rightPanel = new QGroupBox(tr("Statistics Panel"));
    rightPanel->setMinimumWidth(170);
    rightPanel->setMaximumWidth(190);
    rightPanel->setObjectName("rightPanel");
    rightPanel->setLayout(propertiesGrid);

    qCDebug(lcMainWindow) << "Finished panels init.";
}

/**
 * @brief Initializes the application window layout
 *
 * Creates helper widgets and sets the main layout of the MainWindow
 */
void MainWindow::initWindowLayout()
{

    qCDebug(lcMainWindow) << "Initializing window layout...";

    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);
    iconSize.setHeight(16);
    iconSize.setWidth(16);

    //
    // Zoom slider
    //
    zoomInBtn = new QToolButton;
    zoomInBtn->setToolTip(tr("Zoom in the network."));
    zoomInBtn->setStatusTip(tr("Zoom in the network. Or press Cltr and use mouse wheel."));
    zoomInBtn->setWhatsThis(tr("Zoom In.\n\n"
                               "Zooms in the network (Ctrl++)."
                               "You can also press Cltr and use the mouse wheel."));
    zoomInBtn->setAutoRepeat(true);
    zoomInBtn->setAutoRepeatInterval(33);
    zoomInBtn->setAutoRepeatDelay(0);
    zoomInBtn->setIcon(QPixmap(":/images/zoom_in_24px.svg"));
    zoomInBtn->setIconSize(iconSize);

    zoomOutBtn = new QToolButton;
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setToolTip(tr("Zoom out."));
    zoomOutBtn->setStatusTip(tr("Zoom out of the actual network. Or press Cltr and use mouse wheel."));
    zoomOutBtn->setWhatsThis(tr("Zoom out.\n\n"
                                "Zooms out of the actual network. (Ctrl+-)"
                                "You can also press Cltr and use the mouse wheel."));
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setAutoRepeatInterval(33);
    zoomOutBtn->setAutoRepeatDelay(0);
    zoomOutBtn->setIcon(QPixmap(":/images/zoom_out_24px.svg"));
    zoomOutBtn->setIconSize(iconSize);

    zoomSlider = new QSlider;
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(maxZoomIndex);
    zoomSlider->setValue((int)maxZoomIndex / 2.0);
    zoomSlider->setToolTip(tr("Zoom slider: Drag up to zoom in. \n"
                              "Drag down to zoom out. "));
    zoomSlider->setWhatsThis(tr("Zoom slider: Drag up to zoom in. \n"
                                "Drag down to zoom out. "));
    zoomSlider->setTickPosition(QSlider::TicksBothSides);

    // Zoom slider layout
    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
    zoomSliderLayout->addWidget(zoomInBtn);
    zoomSliderLayout->addWidget(zoomSlider);
    zoomSliderLayout->addWidget(zoomOutBtn);
    //
    // Rotate slider
    //
    rotateLeftBtn = new QToolButton;
    rotateLeftBtn->setAutoRepeat(true);
    rotateLeftBtn->setIcon(QPixmap(":/images/rotate_left_48px.svg"));
    rotateLeftBtn->setToolTip(tr("Rotates the canvas counterclockwise"));
    rotateLeftBtn->setStatusTip(tr("Rotate counterclockwise"));
    rotateLeftBtn->setWhatsThis(tr("Rotates the canvas counterclockwise."));
    rotateLeftBtn->setIconSize(iconSize);

    rotateRightBtn = new QToolButton;
    rotateRightBtn->setAutoRepeat(true);
    rotateRightBtn->setIcon(QPixmap(":/images/rotate_right_48px.svg"));
    rotateRightBtn->setToolTip(tr("Rotates the canvas clockwise."));
    rotateRightBtn->setStatusTip(tr("Rotate clockwise"));
    rotateRightBtn->setWhatsThis(tr("Rotates the canvas clockwise."));
    rotateRightBtn->setIconSize(iconSize);

    rotateSlider = new QSlider;
    rotateSlider->setOrientation(Qt::Horizontal);
    rotateSlider->setMinimum(-180);
    rotateSlider->setMaximum(180);
    rotateSlider->setTickInterval(5);
    rotateSlider->setValue(0);
    rotateSlider->setToolTip(tr("Rotate slider: Drag to left to rotate clockwise. \n"
                                "Drag to right to rotate counterclockwise. "));
    rotateSlider->setWhatsThis(tr("Rotate slider: Drag to left to rotate clockwise. "
                                  "Drag to right to rotate counterclockwise. "));
    rotateSlider->setTickPosition(QSlider::TicksBothSides);

    // Rotate slider layout
    QHBoxLayout *rotateSliderLayout = new QHBoxLayout;
    rotateSliderLayout->addWidget(rotateLeftBtn);
    rotateSliderLayout->addWidget(rotateSlider);
    rotateSliderLayout->addWidget(rotateRightBtn);

    resetSlidersBtn = new QToolButton;
    resetSlidersBtn->setText(tr("Reset"));
    resetSlidersBtn->setShortcut(Qt::CTRL | Qt::Key_0);
    resetSlidersBtn->setStatusTip(tr("Reset zoom and rotation to zero (or press Ctrl+0)"));
    resetSlidersBtn->setToolTip(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setWhatsThis(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setIcon(QPixmap(":/images/refresh_48px.svg"));
    resetSlidersBtn->setIconSize(iconSize);
    resetSlidersBtn->setEnabled(true);

    // Filter bar — sits between toolbar and canvas, hidden when no filter is active
    m_filterBar = new FilterBarWidget(this);

    // Wrap filter bar + canvas in a vertical container for grid cell [0,1]
    QVBoxLayout *canvasVBox = new QVBoxLayout;
    canvasVBox->setContentsMargins(0, 0, 0, 0);
    canvasVBox->setSpacing(0);
    canvasVBox->addWidget(m_filterBar);
    canvasVBox->addWidget(graphicsWidget, 1);

    // Wrap the left panel in a scroll area so it scrolls vertically when
    // the bottom dock (Data Table) reduces available height, preventing widget overlap.
    m_leftScrollArea = new QScrollArea;
    m_leftScrollArea->setWidget(leftPanel);
    m_leftScrollArea->setWidgetResizable(true);
    m_leftScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_leftScrollArea->setFrameShape(QFrame::NoFrame);
    m_leftScrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_leftScrollArea->setMinimumWidth(leftPanel->minimumWidth() + 4);

    // Create a layout for the toolbox and the canvas.
    // This will be the layout of our MW central widget
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_leftScrollArea, 0, 0, 2, 1);
    layout->addLayout(canvasVBox, 0, 1);
    layout->addLayout(zoomSliderLayout, 0, 2);
    layout->addWidget(rightPanel, 0, 3, 2, 1);
    layout->addLayout(rotateSliderLayout, 1, 1, 1, 1);
    layout->addWidget(resetSlidersBtn, 1, 2, 1, 1);

    // create a dummy widget, and set the above layout
    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    // now set this as central widget of MW
    setCentralWidget(widget);

    // Data Table dock — docked at the bottom, hidden by default
    m_tableWidget = new GraphTableWidget(this);
    m_tableWidget->setNodeShapeLists(nodeShapeList, iconPathList);
    m_tableDock = new QDockWidget(tr("Data Table"), this);
    m_tableDock->setObjectName("tableDock");
    m_tableDock->setWidget(m_tableWidget);
    m_tableDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    // Minimum height prevents the dock from collapsing to zero when the splitter is dragged
    m_tableWidget->setMinimumHeight(80);
    addDockWidget(Qt::BottomDockWidgetArea, m_tableDock);
    m_tableDock->hide();
    connect(m_tableDock, &QDockWidget::visibilityChanged,
            viewDataTableAct, &QAction::setChecked);

    // set panels visibility
    if (appSettings["showRightPanel"] == "false")
    {
        slotOptionsWindowRightPanelVisibility(false);
    }

    if (appSettings["showLeftPanel"] == "false")
    {
        slotOptionsWindowLeftPanelVisibility(false);
    }

    //
    // Load our default stylesheet, if set in the app settings.
    //
    if (appSettings["useCustomStyleSheet"] == "true")
    {
        slotStyleSheetByName(":/qss/default.qss");
    }

    qCDebug(lcMainWindow) << "Finished window layout init.";
}
