/**
 * @file graph_reports.cpp
 * @brief Implements reporting, export, and analytical output generation
 *        for the Graph class.
 * @author Dimitris B. Kalamaras
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

#include "graph.h"

#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

/**
 * @brief Writes reciprocity report to filename
 * @param fileName
 * @param considerWeights
 */
void Graph::writeReciprocity(const QString fileName, const bool considerWeights)
{
    qDebug() << "Writing reciprocity report to file:" << fileName;

    Q_UNUSED(considerWeights);

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }

    QTextStream outText(&file);

    m_graphReciprocityArc = graphReciprocity();

    int rowCount = 0;
    int progressCounter = 0;
    int N = vertices();
    qreal tiesSym = 0;
    qreal tiesNonSym = 0;
    qreal tiesOutNonSym = 0;
    qreal tiesInNonSym = 0;
    qreal tiesOutNonSymTotalOut = 0;
    qreal tiesInNonSymTotalIn = 0;

    QString pMsg = tr("Writing Reciprocity to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("RECIPROCITY (r) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("Reciprocity, <b>r</b>, is a measure of the likelihood of vertices "
                  "in a directed network to be mutually linked. <br />"
                  "SocNetV supports two different methods to index the degree of "
                  "reciprocity in a social network: <br />"
                  "- The arc reciprocity, which is the fraction of "
                  "reciprocated ties over all actual ties in the network. <br />"
                  "- The dyad reciprocity which is the fraction of "
                  "actor pairs that have reciprocated ties over all "
                  "pairs of actors that have any connection. <br />"
                  "In a directed network, the arc reciprocity measures the proportion "
                  "of directed edges that are bidirectional. If the reciprocity is 1, "
                  "then the adjacency matrix is structurally symmetric. <br />"
                  "Likewise, in a directed network, the dyad reciprocity measures "
                  "the proportion of connected actor dyads that have bidirectional ties "
                  "between them. <br />"
                  "In an undirected graph, all edges are reciprocal. Thus the "
                  "reciprocity of the graph is always 1. <br />"
                  "Reciprocity can be computed on undirected, directed, and weighted graphs.")

            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("r range: ")
            << "</span>"
            << tr("0 &le; r &le; 1")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Arc reciprocity: ")
            << "</span>"
            << tr("%1 / %2 = %3").arg(m_graphReciprocityTiesReciprocated).arg(m_graphReciprocityTiesTotal).arg(m_graphReciprocityArc)
            << "<br />"
            << tr("Of all actual ties in the network, %1% are reciprocated.").arg(m_graphReciprocityArc * 100)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Dyad reciprocity: ")
            << "</span>"
            << tr("%1 / %2 = %3").arg(m_graphReciprocityPairsReciprocated).arg(m_graphReciprocityPairsTotal).arg(m_graphReciprocityDyad)
            << "<br />"
            << tr("Of all pairs of actors that have any ties, %1% have a reciprocated connection.").arg(m_graphReciprocityDyad * 100)
            << "</p>";

    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Reciprocity proportions per actor: ")
            << "</span>"
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Actor")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Symmetric")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nonSymmetric")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nsym out/nsym")
            << "</th>"
            << "<th id=\"col6\" onclick=\"tableSort(results, 5, asc6); asc6*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nsym in/nsym")
            << "</th>"
            << "<th id=\"col7\" onclick=\"tableSort(results, 6, asc7); asc7*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc6 = 1;asc8 = 1;\">"
            << tr("nsym out/out")
            << "</th>"
            << "<th id=\"col8\" onclick=\"tableSort(results, 7, asc8); asc8*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc6 = 1;asc7 = 1;\">"
            << tr("nsym in/in")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody  id=\"results\">";

    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;
        qDebug() << "Graph::writeReciprocity outnon  - innon - rec"
                 << (*it)->outEdgesNonSym()
                 << (*it)->inEdgesNonSym()
                 << (*it)->outEdgesReciprocated();

        // Symmetric: Total number of reciprocated ties involving this actor divided by the number of ties to and from her.
        tiesSym = (qreal)(*it)->outEdgesReciprocated() / (qreal)((*it)->outEdgesCount() + (*it)->inEdgesCount());
        // non Symmetric: One minus symmetric
        tiesNonSym = 1 - tiesSym;
        // nonSym Out/NonSym. Proportion of non-symmetric outgoing ties to the total non-symmetric ties.
        tiesOutNonSym = ((*it)->outEdgesNonSym() || (*it)->inEdgesNonSym()) ? (qreal)(*it)->outEdgesNonSym() / (qreal)((*it)->outEdgesNonSym() + (*it)->inEdgesNonSym()) : 0;
        // nonSym In/NonSym. Proportion of non-symmetric incoming ties to the total non-symmetric ties.
        tiesInNonSym = ((*it)->outEdgesNonSym() || (*it)->inEdgesNonSym()) ? (qreal)(*it)->inEdgesNonSym() / (qreal)((*it)->outEdgesNonSym() + (*it)->inEdgesNonSym()) : 0;
        // nonSym Out/Out. Proportion of non-symmetric outgoing ties to the total outgoing ties.
        tiesOutNonSymTotalOut = ((*it)->outEdgesCount() != 0) ? (qreal)(*it)->outEdgesNonSym() / (qreal)(*it)->outEdgesCount() : 0;
        // nonSym In/In. Proportion of non-symmetric incoming ties to the total incoming ties.
        tiesInNonSymTotalIn = ((*it)->inEdgesCount() != 0) ? (qreal)(*it)->inEdgesNonSym() / (qreal)(*it)->inEdgesCount() : 0;

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                << "<td>"
                << (*it)->number()
                << "</td><td>"
                << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                << "</td><td>"
                << tiesSym
                //<< ((eccentr == 0) ? "\xE2\x88\x9E" : QString::number(eccentr) )
                << "</td><td>"
                << tiesNonSym
                //<< ((eccentr == 0) ? "\xE2\x88\x9E" : QString::number(eccentr) )
                << "</td><td>"
                << tiesOutNonSym
                << "</td><td>"
                << tiesInNonSym
                << "</td><td>"
                << tiesOutNonSymTotalOut
                << "</td><td>"
                << tiesInNonSymTotalIn
                << "</td>"
                << "</tr>";
    }

    outText << "</tbody></table>";

    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("Symmetric ")
            << "</span>"
            << tr("Proportion of reciprocated ties involving the actor to the total incoming and outgoing ties.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("nonSymmetric ")
            << "</span>"
            << tr("One minus symmetric")
            << "<br />"
            << "<span class=\"info\">"
            << tr("nonSym Out/NonSym ")
            << "</span>"
            << tr("Proportion of non-symmetric outgoing ties to the total non-symmetric ties.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("nonSym In/NonSym ")
            << "</span>"
            << tr("Proportion of non-symmetric incoming ties to the total non-symmetric ties.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("nonSym Out/Out ")
            << "</span>"
            << tr("Proportion of non-symmetric outgoing ties to the total outgoing ties.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("nonSym In/In ")
            << "</span>"
            << tr("Proportion of non-symmetric incoming ties to the total incoming ties")
            << "<br/>";
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Reciprocity Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief
 * Writes the matrix of distances to a file
 * @param fn
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeMatrixDistancesPlainText(const QString &fn,
                                          const bool &considerWeights,
                                          const bool &inverseWeights,
                                          const bool &dropIsolates)
{

    qDebug() << "I will write distances matrix (plain-text) to file:" << fn << "First compute the distances matrix...";

    graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);

    qDebug() << "Writing distances matrix (plain-text) to file:" << fn;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }
    QTextStream outText(&file);

    outText.setRealNumberPrecision(m_reportsRealPrecision);
    outText << "-Social Network Visualizer " << VERSION << "\n";
    outText << tr("Network name: ") << getName() << "\n\n";
    outText << "Distance matrix: \n";

    outText << DM;

    file.close();
}

/**
 * @brief Writes the shortest paths matrix to a file
 * Each SIGMA(i,j) element is the number of shortest paths (geodesics) from i and j
 * @param fn
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::writeMatrixShortestPathsPlainText(const QString &fn,
                                              const bool &considerWeights,
                                              const bool &inverseWeights)
{

    qDebug() << "I will write shortest paths matrix (plain-text) to file:" << fn << "First compute the shortest paths matrix...";

    graphMatrixShortestPathsCreate(considerWeights, inverseWeights, false);

    qDebug() << "Writing shortest paths matrix (plain-text) to file:" << fn;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer " << VERSION << "- \n";
    outText << tr("Network name: ") << getName() << " \n\n";
    outText << "Shortest paths matrix: \n";

    outText << SIGMA;

    file.close();
}

/**
 * @brief Writes the Eccentricity report to file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeEccentricity(const QString fileName, const bool considerWeights,
                              const bool inverseWeights, const bool dropIsolates)
{

    qDebug() << "Writing Eccentricity report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    if (!calculatedCentralities)
    {
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);
    }

    int progressCounter = 0;
    int rowCount = 0;
    int N = vertices();
    qreal eccentr = 0;

    QString pMsg = tr("Writing Eccentricity scores to file. \nPlease wait...");
    progressStatus(pMsg);

    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("ECCENTRICITY (e) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The eccentricity <em>e</em> measures how far, at most, is each "
                  " node from every other node. <br />"
                  "In a connected graph, the eccentricity <em>e</em> of a vertex "
                  "is the maximum geodesic distance between that vertex and all other vertices. <br />"
                  "In a disconnected graph, the eccentricity <em>e</em> of all vertices "
                  "is considered to be infinite.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("e range: ")
            << "</span>"
            << tr("1 &le; e &le; \xE2\x88\x9E")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;\">"
            << tr("Actor")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc2 = 1; asc1 = 1;asc4 = 1;\">"
            << tr("e")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody  id=\"results\">";

    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);
        rowCount++;
        eccentr = (*it)->eccentricity();
        qDebug() << "Graph::writeEccentricity() - actor "
                 << (*it)->number()
                 << "eccentricity"
                 << eccentr;

        if (!(*it)->isEnabled())
        {
            qDebug() << "Graph::writeEccentricity() - actor disabled. SKIP.";
            continue; // do not print disabled nodes
        }

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                << "<td>"
                << (*it)->number()
                << "</td><td>"
                << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                << "</td><td>"
                << ((eccentr == 0 || eccentr == RAND_MAX) ? "\xE2\x88\x9E" : QString::number(eccentr))
                << "</td>"
                << "</tr>";
    }

    outText << "</tbody></table>";

    if (minEccentricity == maxEccentricity)
    {
        outText << "<p>"
                << tr("All nodes have the same eccentricity.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max e (Graph Diameter) = ")
                << "</span>"
                << maxEccentricity << " (node " << maxNodeEccentricity << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min e (Graph Radius) = ")
                << "</span>"
                << minEccentricity << " (node " << minNodeEccentricity << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("e classes = ")
                << "</span>"
                << classesEccentricity
                << "</p>";
    }

    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("e = 1 ")
            << "</span>"
            << tr("when the node is connected to all others (star node).")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("e > 1 ")
            << "</span>"
            << tr("when the node is not directly connected to all others. "
                  "Larger eccentricity means the actor is farther from others.")
            << "<br />"
            << "<span class=\"info\">"
            << tr("e = \xE2\x88\x9E ")
            << "</span>"
            << tr("there is no path from that node to one or more other nodes.")
            << "<br/>";
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Eccentricity Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the information centralities to file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::writeCentralityInformation(const QString fileName,
                                       const bool considerWeights,
                                       const bool inverseWeights)
{

    qDebug() << "Writing Information Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }

    QTextStream outText(&file);

    centralityInformation(considerWeights, inverseWeights);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::IC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    bool dropIsolates = true; // by default IC needs to exclude isolates

    int rowCount = 0;
    int N = vertices(dropIsolates, false, true);

    int progressCounter = 0;

    QString pMsg = tr("Writing Information Centralities to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("INFORMATION CENTRALITY (IC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The IC index, introduced by Stephenson and Zelen (1991), measures the "
                  "information flow through all paths between actors weighted by "
                  "strength of tie and distance.")
            << "<br />"
            << tr("IC' is the standardized index (IC divided by the sumIC).")
            << "<br />"
            << tr("Warning: To compute this index, SocNetV drops all isolated "
                  "nodes and symmetrizes (if needed) the adjacency matrix. <br />"
                  "Read the Manual for more.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("IC range: ")
            << "</span>"
            << tr("0 &le; IC &le; \xE2\x88\x9E")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("IC' range: ")
            << "</span>"
            << tr("0 &le; IC' &le; 1")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("IC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("IC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%IC")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if ((*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {

            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->IC()
                    << "</td><td>"
                    << (*it)->SIC()
                    << "</td><td>"
                    << (100 * ((*it)->SIC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minIC == maxIC)
    {
        outText << "<p>"
                << tr("All nodes have the same IC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max IC' = ")
                << "</span>"
                << maxIC << " (node " << maxNodeIC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min IC' = ")
                << "</span>"
                << minIC << " (node " << minNodeIC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("IC classes = ")
                << "</span>"
                << classesIC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("IC' Sum = ")
            << "</span>"
            << sumIC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("IC' Mean = ")
            << "</span>"
            << meanIC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("IC' Variance = ")
            << "</span>"
            << varianceIC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("IC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<h2>";
    outText << tr("GROUP INFORMATION CENTRALIZATION (GIC)")
            << "</h2>";

    outText << "<p>"
            << tr("Since there is no way to compute Group Information Centralization, <br />"
                  "you can use Variance as a general centralization index. <br /><br />")
            << "<span class=\"info\">"
            << tr("Variance = ")
            << "</span>"
            << varianceIC
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("Variance = 0, when all nodes have the same IC value, i.e. a "
                  "complete or a circle graph). <br />")
            << tr("Larger values of variance suggest larger variability between the "
                  "IC' values. <br />")
            << "(Wasserman & Faust, formula 5.20, p. 197)\n\n"
            << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Information Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the eigenvector centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityEigenvector(const QString fileName,
                                       const bool &considerWeights,
                                       const bool &inverseWeights,
                                       const bool &dropIsolates)
{

    qDebug() << "Writing Eigenvector Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    centralityEigenvector(considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::EVC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Eigenvector Centrality scores to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("EIGENVECTOR CENTRALITY (EVC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The Eigenvector Centrality of each node is the i<sub>th</sub> element of "
                  "the leading eigenvector of the adjacency matrix, that is the "
                  "eigenvector corresponding to the largest positive eigenvalue. <br />"
                  "Proposed by Bonacich (1972), the Eigenvector Centrality is "
                  "an extension of the simpler Degree Centrality because it gives "
                  "each actor a score proportional to the scores of its neighbors. "
                  "Thus, a node may have high EVC score if it has lots of ties or "
                  "it has ties to other nodes with high EVC. <br />"
                  "The eigenvector centralities are also known as Gould indices.")
            << "<br />"
            << tr("EVC' is the scaled EVC (EVC divided by max EVC).")
            << "<br />"
            << tr("EVC'' is the standardized index (EVC divided by the sum of all EVCs).")
            << "<br />"
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("EVC range: ")
            << "</span>"
            << tr("0 &le; EVC &lt; 1 (The eigenvector has unit euclidean length) ")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("EVC' range: ")
            << "</span>"
            << tr("0 &le; EVC' &le; 1")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("EVC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("EVC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc6 = 1;\">"
            << tr("EVC''")
            << "</th>"
            << "<th id=\"col6\" onclick=\"tableSort(results, 5, asc6); asc6 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("%EVC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                << "<td>"
                << (*it)->number()
                << "</td><td>"
                << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                << "</td><td>"
                << (*it)->EVC()
                << "</td><td>"
                << (*it)->SEVC()
                << "</td><td>"
                << (*it)->EVC() / (sumEVC ? sumEVC : 1)
                << "</td><td>"
                << (100 * ((*it)->SEVC()))
                << "</td>"
                << "</tr>";
    }

    outText << "</tbody></table>";

    if (minEVC == maxEVC)
    {
        outText << "<p>"
                << tr("All nodes have the same EVC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max EVC = ")
                << "</span>"
                << maxEVC << " (node " << maxNodeEVC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min EVC = ")
                << "</span>"
                << minEVC << " (node " << minNodeEVC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("EVC classes = ")
                << "</span>"
                << classesEVC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("EVC Sum = ")
            << "</span>"
            << sumEVC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("EVC Mean = ")
            << "</span>"
            << meanEVC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("EVC Variance = ")
            << "</span>"
            << varianceEVC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("EVC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<h2>";
    outText << tr("GROUP EIGENVECTOR CENTRALIZATION (GEC)")
            << "</h2>";

    outText << "<p>"
            << tr("Since there is no way to compute Group Eigenvector Centralization, <br />"
                  "you can use Variance as a general centralization index. <br /><br />")
            << "<span class=\"info\">"
            << tr("Variance = ")
            << "</span>"
            << varianceEVC
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("Variance = 0, when all nodes have the same EVC value, i.e. a "
                  "complete or a circle graph). <br />")
            << tr("Larger values of variance suggest larger variability between the "
                  "EVC' values. <br />")
            << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Eigenvector Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the Degree Centrality to a file
 * @param fileName
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::writeCentralityDegree(const QString fileName,
                                  const bool considerWeights,
                                  const bool dropIsolates)
{

    qDebug() << "Writing Degree Centrality report to file:" << fileName
             << "considerWeights:" << considerWeights
             << "dropIsolates:" << dropIsolates;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    centralityDegree(considerWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::DC, m_reportsChartType, distImageFileName);

    qreal maxIndexDC = vertices(dropIsolates) - 1.0;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;
    VList::const_iterator it;

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    QString pMsg = tr("Writing out-Degree Centralities. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << "<h1>";
    outText << tr("DEGREE CENTRALITY (DC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("In undirected networks, the DC index is the sum of edges attached to a node u. <br />"
                  "In directed networks, the index is the sum of outbound arcs from node u "
                  "to all adjacent nodes (also called \"outDegree Centrality\"). <br />"
                  "If the network is weighted, the DC score is the sum of weights of outbound "
                  "edges from node u to all adjacent nodes.<br />"
                  "Note: To compute inDegree Centrality, use the Degree Prestige measure.")
            << "<br />"
            << tr("DC' is the standardized index (DC divided by N-1 (non-valued nets) or by sumDC (valued nets).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DC range: ")
            << "</span>"
            << tr("0 &le; DC &le; ");
    if (considerWeights)
        outText << infinity;
    else
        outText << maxIndexDC;
    outText << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DC' range: ")
            << "</span>"
            << tr("0 &le; DC' &le; 1")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("DC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("DC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%DC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        rowCount++;

        progressUpdate(++progressCounter);

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->DC()
                    << "</td><td>"
                    << (*it)->SDC()
                    << "</td><td>"
                    << (100 * ((*it)->SDC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSDC == maxSDC)
    {
        outText << "<p>"
                << tr("All nodes have the same DC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("DC Sum = ")
                << "</span>"
                << sumDC
                << "</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max DC' = ")
                << "</span>"
                << maxSDC << " (node " << maxNodeSDC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min DC' = ")
                << "</span>"
                << minSDC << " (node " << minNodeSDC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("DC' classes = ")
                << "</span>"
                << classesSDC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("DC' Sum = ")
            << "</span>"
            << sumSDC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("DC' Mean = ")
            << "</span>"
            << meanSDC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("DC' Variance = ")
            << "</span>"
            << varianceSDC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("DC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights)
    {
        outText << "<h2>";
        outText << tr("GROUP DEGREE CENTRALIZATION (GDC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GDC = ")
                << "</span>"
                << groupDC
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GDC range: ")
                << "</span>"
                << " 0 &le; GDC &le; 1"
                << "</p>";

        outText << "<p class=\"description\">"
                << tr("GDC = 0, when all out-degrees are equal (i.e. regular lattice).")
                << "<br />"
                << tr("GDC = 1, when one node completely dominates or overshadows the other nodes.")
                << "<br />"
                << "(Wasserman & Faust, formula 5.5, p. 177)"
                << "<br />"
                << "(Wasserman & Faust, p. 101)"
                << "</p>";
    }
    else
        outText << "<p class=\"description\">"
                << tr("Because this graph is weighted, we cannot compute Group Centralization")
                << "<br />"
                << tr("You can use variance as a group-level centralization measure.")
                << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Degree Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the closeness centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityCloseness(const QString fileName,
                                     const bool considerWeights,
                                     const bool inverseWeights,
                                     const bool dropIsolates)
{

    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug() << "Writing closeness Centrality report to file:" << fileName
             << "considerWeights" << considerWeights
             << "inverseWeights" << inverseWeights
             << "dropIsolates" << dropIsolates
             << "m_reportsLabelLength" << m_reportsLabelLength;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::CC, m_reportsChartType, distImageFileName);

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Closeness Centrality scores to file. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("CLOSENESS CENTRALITY (CC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The CC index is the inverted sum of geodesic distances "
                  "from each node u to all other nodes. ")
            << "<br />"
            << tr("Note: The CC index considers outbound arcs only and "
                  "isolate nodes are dropped by default. ")
            << "<br />"
            << tr("Read the Manual for more.")
            << "<br />"
            << tr("CC' is the standardized index (CC multiplied by (N-1 minus isolates)).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CC range: ")
            << "</span>"
            << tr("0 &le; CC &le; ") << 1.0 / maxIndexCC
            << tr(" ( 1 / Number of node pairs excluding u)")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CC' range: ")
            << "</span>"
            << tr("0 &le; CC' &le; 1  (CC'=1 when a node is the center of a star graph)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("CC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("CC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%CC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    VList::const_iterator it;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->CC()
                    << "</td><td>"
                    << (*it)->SCC()
                    << "</td><td>"
                    << (100 * ((*it)->SCC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSCC == maxSCC)
    {
        outText << "<p>"
                << tr("All nodes have the same CC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("CC Sum = ")
                << "</span>"
                << sumCC
                << "</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max CC' = ")
                << "</span>"
                << maxSCC << " (node " << maxNodeSCC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min CC' = ")
                << "</span>"
                << minSCC << " (node " << minNodeSCC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("CC' classes = ")
                << "</span>"
                << classesSCC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("CC' Sum = ")
            << "</span>"
            << sumSCC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("CC' Mean = ")
            << "</span>"
            << meanSCC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("CC' Variance = ")
            << "</span>"
            << varianceSCC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("CC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights)
    {
        outText << "<h2>";
        outText << tr("GROUP CLOSENESS CENTRALIZATION (GCC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GCC = ")
                << "</span>"
                << groupCC
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GCC range: ")
                << "</span>"
                << " 0 &le; GCC &le; 1"
                << "</p>";

        outText << "<p class=\"description\">"
                << tr("GCC = 0, when the lengths of the geodesics are all equal, "
                      "i.e. a complete or a circle graph.")
                << "<br />"
                << tr("GCC = 1, when one node has geodesics of length 1 to all the "
                      "other nodes, and the other nodes have geodesics of length 2. "
                      "to the remaining (N-2) nodes.")
                << "<br />"
                << tr("This is exactly the situation realised by a star graph.")
                << "<br />"
                << "(Wasserman & Faust, formula 5.9, p. 186-187)"
                << "</p>";
    }
    else
        outText << "<p class=\"description\">"
                << tr("Because this graph is weighted, we cannot compute Group Centralization")
                << "<br />"
                << tr("You can use variance as a group-level centralization measure.")
                << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Closeness Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the "improved" closeness centrality indices to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityClosenessInfluenceRange(const QString fileName,
                                                   const bool considerWeights,
                                                   const bool inverseWeights,
                                                   const bool dropIsolates)
{

    qDebug() << "Writing IR Closeness Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    centralityClosenessIR(considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::IRCC, m_reportsChartType, distImageFileName);

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Influence Range Centrality scores. \n"
                      "Please wait");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("INFLUENCE RANGE CLOSENESS CENTRALITY (IRCC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The IRCC index of a node u is the ratio of the fraction of nodes "
                  "reachable by node u to the average distance of these nodes from u  "
                  "(Wasserman & Faust, formula 5.22, p. 201)<br />"
                  "Thus, this measure is similar to Closeness Centrality "
                  "but it counts only outbound distances from each actor to other reachable nodes. <br />"
                  "This measure is useful for directed networks which are "
                  "not strongly connected (thus the ordinary CC index cannot be computed).<br />"
                  "In undirected networks, the IRCC has the same properties and yields "
                  "the same results as the ordinary Closeness Centrality.<br />"
                  "Read the Manual for more. ")

            << "<br />"
            << tr("IRCC is standardized.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("IRCC range: ")
            << "</span>"
            << tr("0 &le; IRCC &le; 1  (IRCC is a ratio)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("IRCC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%IRCC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    VList::const_iterator it;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->IRCC()
                    << "</td><td>"
                    << (100 * ((*it)->SIRCC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minIRCC == maxIRCC)
    {
        outText << "<p>"
                << tr("All nodes have the same IRCC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max IRCC = ")
                << "</span>"
                << maxIRCC << " (node " << maxNodeIRCC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min IRCC = ")
                << "</span>"
                << minIRCC << " (node " << minNodeIRCC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("IRCC classes = ")
                << "</span>"
                << classesIRCC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("IRCC Sum = ")
            << "</span>"
            << sumIRCC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("IRCC Mean = ")
            << "</span>"
            << meanIRCC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("IRCC Variance = ")
            << "</span>"
            << varianceIRCC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("IRCC DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Influence Range Closeness Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes Betweenness centralities to file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityBetweenness(const QString fileName,
                                       const bool considerWeights,
                                       const bool inverseWeights,
                                       const bool dropIsolates)
{

    qDebug() << "Writing Betweenness Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::BC, m_reportsChartType, distImageFileName);

    int rowCount = 0, progressCounter = 0;
    int N = vertices();

    QString pMsg = tr("Writing Betweenness Centrality scores to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("BETWEENNESS CENTRALITY (BC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The BC index of a node u is the sum of &delta;<sub>(s,t,u)</sub> for all s,t &isin; V ")
            << tr("where &delta;<sub>(s,t,u)</sub> is the ratio of all geodesics between "
                  "s and t which run through u. ")
            << "<br />"
            << tr("Read the Manual for more.")
            << "<br />"
            << tr("BC' is the standardized index (BC divided by (N-1)(N-2)/2 in symmetric nets or (N-1)(N-2) otherwise.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("BC range: ")
            << "</span>"
            << tr("0 &le; BC &le; ") << maxIndexBC
            << tr(" (Number of pairs of nodes excluding u)")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("BC' range: ")
            << "</span>"
            << tr("0 &le; BC' &le; 1  (BC'=1 when the node falls on all geodesics)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("BC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("BC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%BC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    VList::const_iterator it;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        progressUpdate(++progressCounter);
        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->BC()
                    << "</td><td>"
                    << (*it)->SBC()
                    << "</td><td>"
                    << (100 * ((*it)->SBC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSBC == maxSBC)
    {
        outText << "<p>"
                << tr("All nodes have the same BC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("BC Sum = ")
                << "</span>"
                << sumBC
                << "</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max BC' = ")
                << "</span>"
                << maxSBC << " (node " << maxNodeSBC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min BC' = ")
                << "</span>"
                << minSBC << " (node " << minNodeSBC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("BC' classes = ")
                << "</span>"
                << classesSBC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("BC' Sum = ")
            << "</span>"
            << sumSBC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("BC' Mean = ")
            << "</span>"
            << meanSBC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("BC' Variance = ")
            << "</span>"
            << varianceSBC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("BC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights)
    {
        outText << "<h2>";
        outText << tr("GROUP BETWEENNESS CENTRALIZATION (GBC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GBC = ")
                << "</span>"
                << groupSBC
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GBC range: ")
                << "</span>"
                << " 0 &le; GBC &le; 1"
                << "</p>";

        outText << "<p class=\"description\">"
                << tr("GBC = 0, when all the nodes have exactly the same betweenness index.")
                << "<br />"
                << tr("GBC = 1, when one node falls on all other geodesics between "
                      "all the remaining (N-1) nodes. ")
                << "<br />"
                << tr("This is exactly the situation realised by a star graph.")
                << "<br />"
                << "(Wasserman & Faust, formula 5.13, p. 192)"
                << "</p>";
    }
    else
        outText << "<p class=\"description\">"
                << tr("Because this graph is weighted, we cannot compute Group Centralization")
                << "<br />"
                << tr("You can use variance as a group-level centralization measure.")
                << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Betweenness Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the Stress centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityStress(const QString fileName,
                                  const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool dropIsolates)
{

    qDebug() << "Writing Stress Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::SC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Stress Centralities. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("STRESS CENTRALITY (SC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The SC index of each node u is the sum of &sigma;<sub>(s,t,u)</sub>): <br />"
                  "the number of geodesics from s to t through u.")
            << "<br />"
            << tr("SC' is the standardized index (SC divided by sumSC).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("SC range: ")
            << "</span>"
            << tr("0 &le; SC &le; ") << maxIndexSC
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("SC' range: ")
            << "</span>"
            << tr("0 &le; SC' &le; 1  (SC'=1 when the node falls on all geodesics)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("SC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("SC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%SC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->SC()
                    << "</td><td>"
                    << (*it)->SSC()
                    << "</td><td>"
                    << (100 * ((*it)->SSC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSSC == maxSSC)
    {
        outText << "<p>"
                << tr("All nodes have the same SC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("SC Sum = ")
                << "</span>"
                << sumSC
                << "</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max SC' = ")
                << "</span>"
                << maxSSC << " (node " << maxNodeSSC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min SC' = ")
                << "</span>"
                << minSSC << " (node " << minNodeSSC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("BC classes = ")
                << "</span>"
                << classesSSC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("SC' Sum = ")
            << "</span>"
            << sumSSC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("SC' Mean = ")
            << "</span>"
            << meanSSC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("SC' Variance = ")
            << "</span>"
            << varianceSSC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("SC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<p class=\"small\">";
    outText << tr("Stress Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the Eccentricity centralities (aka Harary Graph Centrality) to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityEccentricity(const QString fileName,
                                        const bool considerWeights,
                                        const bool inverseWeights,
                                        const bool dropIsolates)
{

    qDebug() << "Writing Eccentricity Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::EC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Eccentricity Centralities to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("ECCENTRICITY CENTRALITY (EC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The EC score of a node u is the inverse maximum geodesic distance "
                  "from u to all other nodes in the network.")
            << "<br />"
            << tr("This index is also known as <em>Harary Graph Centrality</em>. ")
            << tr("EC is standardized.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("EC range: ")
            << "</span>"
            << tr("0 &le; EC &le; 1 ")
            << tr(" (EC=1 when the actor has ties to all other nodes)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("EC=EC'")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%EC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->EC()
                    << "</td><td>"
                    << (100 * ((*it)->SEC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minEC == maxEC)
    {
        outText << "<p>"
                << tr("All nodes have the same EC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max EC = ")
                << "</span>"
                << maxEC << " (node " << maxNodeEC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min EC = ")
                << "</span>"
                << minEC << " (node " << minNodeEC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("EC classes = ")
                << "</span>"
                << classesEC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("EC Sum = ")
            << "</span>"
            << sumEC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("EC Mean = ")
            << "</span>"
            << meanEC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("EC Variance = ")
            << "</span>"
            << varianceEC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("EC DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Eccentricity Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes Power Centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityPower(const QString fileName,
                                 const bool considerWeights,
                                 const bool inverseWeights,
                                 const bool dropIsolates)
{

    qDebug() << "Writing Power Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::PC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Gil-Schmidt Power Centralities to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("POWER CENTRALITY (PC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The PC index, introduced by Gil and Schmidt, of a node u is the sum of the sizes of all Nth-order "
                  "neighbourhoods with weight 1/n.")
            << "<br />"
            << tr("PC' is the standardized index: The PC score divided by the total number "
                  "of nodes in the same component minus 1")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PC range: ")
            << "</span>"
            << tr("0 &le; PC &le; ") << maxIndexPC
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PC' range: ")
            << "</span>"
            << tr("0 &le; PC' &le; 1  (PC'=1 when the node is connected to all (star).)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("PC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%PC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->PC()
                    << "</td><td>"
                    << (*it)->SPC()
                    << "</td><td>"
                    << (100 * ((*it)->SPC()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSPC == maxSPC)
    {
        outText << "<p>"
                << tr("All nodes have the same PC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("PC Sum = ")
                << "</span>"
                << sumPC
                << "</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PC' = ")
                << "</span>"
                << maxSPC << " (node " << maxNodeSPC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min PC' = ")
                << "</span>"
                << minSPC << " (node " << minNodeSPC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("PC classes = ")
                << "</span>"
                << classesSPC
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PC' Sum = ")
            << "</span>"
            << sumSPC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PC' Mean = ")
            << "</span>"
            << meanSPC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PC' Variance = ")
            << "</span>"
            << varianceSPC
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("PC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights)
    {
        outText << "<h2>";
        outText << tr("GROUP POWER CENTRALIZATION (GPC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GPC = ")
                << "</span>"
                << groupSPC
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GPC range: ")
                << "</span>"
                << " 0 &le; GPC &le; 1"
                << "</p>";

        outText << "<p class=\"description\">"
                << tr("GPC = 0, when all in-degrees are equal (i.e. regular lattice).")
                << "<br />"
                << tr("GPC = 1, when one node is linked to all other nodes (i.e. star). ")
                << "<br />"
                << "</p>";
    }
    else
        outText << "<p class=\"description\">"
                << tr("Because this graph is weighted, we cannot compute Group Centralization")
                << "<br />"
                << tr("Use mean or variance instead.")
                << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Power Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the Degree Prestige of each node to a file
 * @param fileName
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::writePrestigeDegree(const QString fileName,
                                const bool considerWeights,
                                const bool dropIsolates)
{

    qDebug() << "Writing Degree Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    prestigeDegree(considerWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::DP, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int N = vertices();

    qreal maxIndexDP = N - 1.0;

    int rowCount = 0;
    int progressCounter = 0;

    QString pMsg = tr("Writing Degree Prestige (in-Degree) scores to file. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("DEGREE PRESTIGE (DP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The DP index, also known as InDegree Centrality, of a node u "
                  "is the sum of inbound edges to that node from all adjacent nodes. <br />"
                  "If the network is weighted, DP is the sum of inbound arc "
                  "weights (Indegree) to node u from all adjacent nodes. ")
            << "<br />"
            << tr("DP' is the standardized index (DP divided by N-1).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DP range: ")
            << "</span>"
            << tr("0 &le; DP &le; ");
    if (considerWeights)
        outText << infinity;
    else
        outText << maxIndexDP;
    outText << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DP' range: ")
            << "</span>"
            << tr("0 &le; DP' &le; 1")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("DP")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("DP'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%DP'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->DP()
                    << "</td><td>"
                    << (*it)->SDP()
                    << "</td><td>"
                    << (100 * ((*it)->SDP()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minSDP == maxSDP)
    {
        outText << "<p>"
                << tr("All nodes have the same DP score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("DP Sum = ")
                << "</span>"
                << sumDP
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("Max DP' = ")
                << "</span>"
                << maxSDP << " (node " << maxNodeDP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min DP' = ")
                << "</span>"
                << minSDP << " (node " << minNodeDP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("DP' classes = ")
                << "</span>"
                << classesSDP
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("DP' Sum = ")
            << "</span>"
            << sumSDP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("DP' Mean = ")
            << "</span>"
            << meanSDP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("DP' Variance = ")
            << "</span>"
            << varianceSDP
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("DP' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights)
    {
        outText << "<h2>";
        outText << tr("GROUP DEGREE PRESTIGE (GDP)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GDP = ")
                << "</span>"
                << groupDP
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GDP range: ")
                << "</span>"
                << " 0 &le; GDP &le; 1"
                << "</p>";

        outText << "<p class=\"description\">"
                << tr("GDP = 0, when all in-degrees are equal (i.e. regular lattice).")
                << "<br />"
                << tr("GDP = 1, when one node is chosen by all other nodes (i.e. star). ")
                << "<br />"
                << tr("This is exactly the situation realised by a star graph.")
                << "<br />"
                << "(Wasserman & Faust, p. 203)"
                << "</p>";
    }
    else
        outText << "<p class=\"description\">"
                << tr("Because this graph is weighted, we cannot compute Group Centralization")
                << "<br />"
                << tr("You can use variance as a group-level centralization measure.")
                << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Degree Prestige report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief
 * Writes the proximity prestige indices to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writePrestigeProximity(const QString fileName,
                                   const bool considerWeights,
                                   const bool inverseWeights,
                                   const bool dropIsolates)
{

    qDebug() << "Writing Proximity Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    prestigeProximity(considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::PP, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Proximity Prestige scores to file. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("PROXIMITY PRESTIGE (PP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The PP index of a node u is the ratio of the proportion of "
                  "nodes who can reach u to the average distance these nodes are from u "
                  "(Wasserman & Faust, formula 5.25, p. 204)<br />"
                  "Thus, it is similar to Closeness Centrality but it counts "
                  "only inbound distances to each actor, thus it is a measure of actor prestige. <br />"
                  "This metric is useful for directed networks which are "
                  "not strongly connected (thus the ordinary CC index cannot be computed).<br />"
                  "In undirected networks, the PP has the same properties and yields "
                  "the same results as Closeness Centrality.<br />"
                  "Read the Manual for more. <br />")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PP range: ")
            << "</span>"
            << tr("0 &le; PP &le; 1 (PP is a ratio)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PP=PP'")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%PP")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->PP()
                    << "</td><td>"
                    << (100 * ((*it)->SPP()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minPP == maxPP)
    {
        outText << "<p>"
                << tr("All nodes have the same PP score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PP = ")
                << "</span>"
                << maxPP << " (node " << maxNodePP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min PP = ")
                << "</span>"
                << minPP << " (node " << minNodePP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("PP classes = ")
                << "</span>"
                << classesPP
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PP Sum = ")
            << "</span>"
            << sumPP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PP Mean = ")
            << "</span>"
            << meanPP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PP Variance = ")
            << "</span>"
            << variancePP
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("PP DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Proximity Prestige report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes the PageRank scores of vertices to a file
 * @param fileName
 * @param dropIsolates
 */
void Graph::writePrestigePageRank(const QString fileName,
                                  const bool dropIsolates)
{

    qDebug() << "Writing PageRank Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    prestigePageRank(dropIsolates);

    QString distImageFileName;

    if (m_reportsChartType != ChartType::None)
    {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                            QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";
    }

    prominenceDistribution(IndexType::PRP, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing PageRank scores to file. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("PAGERANK PRESTIGE (PRP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The PRP is an importance ranking index for each node based on the structure "
                  "of its incoming links/edges and the rank of the nodes linking to it. <br />"

                  "For each node u the algorithm counts all inbound links (edges) to it, but it "
                  "normalizes each inbound link from a node v by the outDegree of v. <br />"

                  "The PR values correspond to the principal eigenvector of the normalized link matrix.<br />"

                  "Note: In weighted relations, each backlink to a node u from another node v is considered "
                  "to have weight=1 but it is normalized by the sum of outbound edge weights of v. "
                  "Therefore, nodes with high outLink weights give smaller percentage of their PR to node u.")
            << "<br />"
            << tr("PRP' is the scaled PRP (PRP divided by max PRP).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PRP range: ")
            << "</span>"
            << tr("(1-d)/N = ") << ((1 - d_factor) / N) << tr(" &le; PRP  ")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PRP' range: ")
            << "</span>"
            << tr("0 &le; PRP' &le; 1")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PRP")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("PRP'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%PRP'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated())
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td><td>"
                    << "--"
                    << "</td>"
                    << "</tr>";
        }
        else
        {
            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                    << "<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                    << "</td><td>"
                    << (*it)->PRP()
                    << "</td><td>"
                    << (*it)->SPRP()
                    << "</td><td>"
                    << (100 * ((*it)->SPRP()))
                    << "</td>"
                    << "</tr>";
        }
    }

    outText << "</tbody></table>";

    if (minPRP == maxPRP)
    {
        outText << "<p>"
                << tr("All nodes have the same PRP score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PRP = ")
                << "</span>"
                << maxPRP << " (node " << maxNodePRP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min PRP = ")
                << "</span>"
                << minPRP << " (node " << minNodePRP << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("PRP classes = ")
                << "</span>"
                << classesPRP
                << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PRP Sum = ")
            << "</span>"
            << sumPRP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PRP Mean = ")
            << "</span>"
            << meanPRP
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PRP Variance = ")
            << "</span>"
            << variancePRP
            << "<br/>";
    outText << "</p>";

    if (m_reportsChartType != ChartType::None)
    {
        outText << "<h2>";
        outText << tr("PRP' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("PageRank Prestige report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Graph::writeWalksTotalMatrixPlainText
 * Writes the total number of walks matrix
 * @param fn
 * @param netName
 * @param length
 */
void Graph::writeWalksTotalMatrixPlainText(const QString &fn)
{

    qDebug() << "I will write (plain-text) total walks to file:" << fn;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer " << VERSION << "\n";
    outText << tr("Network name: ") << getName() << "\n"
            << "\n";
    outText << "Total number of walks of any length less than or equal to " << vertices() - 1
            << " between each pair of nodes \n\n";
    outText << "Warning: Walk counts consider unordered pairs of nodes\n\n";

    int N = vertices();

    graphWalksMatrixCreate(N, 0, true);

    outText << XSM;

    file.close();
}

/**
 * @brief Graph::writeWalksOfLengthMatrixPlainText
 * @param fn
 * @param length
 */
void Graph::writeWalksOfLengthMatrixPlainText(const QString &fn, const int &length)
{

    qDebug() << "I will write (plain-text) walks of length:" << length
             << "to file:" << fn;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }
    progressStatus(tr("Writing Walks matrix to file:") + fn);

    QTextStream outText(&file);

    outText << "-Social Network Visualizer " << VERSION << "- \n";
    outText << "Network name: " << getName() << " \n";
    outText << "Number of walks of length " << length << " between each pair of nodes \n\n";

    int N = vertices();
    graphWalksMatrixCreate(N, length, true);

    outText << XM;

    file.close();
}

/**
 * @brief Writes the walks of given length matrix to a file in HTML.
 * If length = 0, it writes the Total Walks matrix.
 * @param fn
 * @param length
 * @param simpler
 */
void Graph::writeMatrixWalks(const QString &fn,
                             const int &length,
                             const bool &simpler)
{

    qDebug() << "I will write walks of length:" << length
             << "to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(simpler);

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }

    int N = vertices();

    progressStatus(tr("Computing Walks..."));
    graphWalksMatrixCreate(N, length, true);
    if (progressCanceled())
    {
        file.close();
        return;
    }
    QTextStream outText(&file);

    outText << htmlHead;

    outText << "<h1>";

    if (length > 0)
    {
        outText << tr("WALKS OF LENGTH %1 MATRIX").arg(length);
    }
    else
    {
        outText << tr("TOTAL WALKS MATRIX");
    }

    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    if (length > 0)
    {
        outText << "<p class=\"description\">"
                << tr("The Walks of length %1 matrix is a NxN matrix "
                      "where each element (i,j) is the number of walks of "
                      "length %1 between actor i and actor j, "
                      "or 0 if no walk exists. <br />"
                      "A walk is a sequence of edges and vertices, where each edge's "
                      "endpoints are the two vertices adjacent to it. In a walk, "
                      "vertices and edges may repeat. <br />"
                      "Warning: Walks count unordered pairs of nodes. ")
                       .arg(length)
                << "</p>";
    }
    else
    {
        outText << "<p class=\"description\">"
                << tr("The Total Walks matrix of a social network is a NxN matrix "
                      "where each element (i,j) is the total number of walks of any "
                      "length (less than or equal to %1) between actor i and actor j, "
                      "or 0 if no walk exists. <br />"
                      "A walk is a sequence of edges and vertices, where each edge's "
                      "endpoints are the two vertices adjacent to it. In a walk, "
                      "vertices and edges may repeat. <br />"
                      "Warning: Walks count unordered pairs of nodes. ")
                       .arg(N - 1)
                << "</p>";
    }

    progressStatus(tr("Writing Walks matrix to file:") + fn);
    qDebug() << "Graph::writeMatrixWalks() - Writing XM to file";

    if (length > 0)
    {
        writeMatrixHTMLTable(outText, XM, true);
    }
    else
    {
        writeMatrixHTMLTable(outText, XSM, true);
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Walks report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();
}

/**
    Writes the reachability matrix X^R of the graph to a file
*/
void Graph::writeReachabilityMatrixPlainText(const QString &fn, const bool &dropIsolates)
{

    qDebug() << "Writing Reachability Matrix plain text to file:" << fn;

    QFile file(fn);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer " << VERSION << "\n";
    outText << tr("Network name: ") << getName() << "\n\n";
    outText << tr("Reachability Matrix (XR)") << "\n";
    outText << tr("Two nodes are reachable if there is a walk between them (their geodesic distance is non-zero).") << "\n";
    outText << tr("If nodes i and j are reachable then XR(i,j)=1 otherwise XR(i,j)=0.") << "\n\n";

    graphDistancesGeodesic(false, false, false, dropIsolates);

    outText << XRM;

    file.close();
}

/**
 * @brief Graph::writeClusteringCoefficient
 * Writes the clustering coefficients to a file
 * @param fileName
 * @param considerWeights
 */
void Graph::writeClusteringCoefficient(const QString fileName,
                                       const bool considerWeights)
{

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    VList::const_iterator it;

    averageCLC = clusteringCoefficient(true);
    if (progressCanceled())
    {
        file.close();
        return;
    }
    QString pMsg = tr("Writing Clustering Coefficients to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("CLUSTERING COEFFICIENT (CLC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The local Clustering Coefficient, introduced by Watts and Strogatz (1998) "
                  "quantifies how close each node and its neighbors are to being a complete subgraph (clique).")
            << "<br />"
            << tr("For each node <em>u</em>, the local CLC score is the proportion of actual links between "
                  "its neighbors divided by the number of links that could possibly exist between them. <br />"
                  "The CLC index is used to characterize the transitivity of a network. A value close to one "
                  "indicates that the node is involved in many transitive relations. "
                  "CLC' is the normalized CLC, divided by maximum CLC found in this network.")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CLC range: ")
            << "</span>"
            << tr("0 &le; CLC &le; 1 ")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CLC range: ")
            << "</span>"
            << tr("0 &le; CLC' &le; 1 ")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            << "<tr>"
            << "<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            << "<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("CLC")
            << "</th>"
            << "<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("CLC'")
            << "</th>"
            << "<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%CLC'")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody id=\"results\">";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            file.close();
            progressFinish();
            return;
        }

        rowCount++;

        outText << Qt::fixed;

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                << "<td>"
                << (*it)->number()
                << "</td><td>"
                << ((!((*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-")
                << "</td><td>"
                << (*it)->CLC()
                << "</td><td>"
                << (*it)->CLC() / maxCLC
                << "</td><td>"
                << 100 * (*it)->CLC() / maxCLC
                << "</td>"
                << "</tr>";
    }

    outText << "</tbody></table>";

    if (minCLC == maxCLC)
    {
        outText << "<p>"
                << tr("All nodes have the same local CLC score.")
                << "</p>";
    }
    else
    {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max CLC = ")
                << "</span>"
                << maxCLC << " (node " << maxNodeCLC << ")"
                << "<br />"
                << "<span class=\"info\">"
                << tr("Min CLC = ")
                << "</span>"
                << minCLC << " (node " << minNodeCLC << ")"
                << "<br />"
                << "</p>";
    }

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CLC Mean = ")
            << "</span>"
            << averageCLC
            << "<br/>"
            << "<span class=\"info\">"
            << tr("CLC Variance = ")
            << "</span>"
            << varianceCLC
            << "<br/>";
    outText << "</p>";

    outText << "<h2>";
    outText << tr("GROUP / NETWORK AVERAGE CLUSTERING COEFFICIENT (GCLC)")
            << "</h2>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("GCLC = ")
            << "</span>"
            << averageCLC
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("Range: 0 < GCLC < 1 <br/ >")
            << tr("GCLC = 0, when there are no cliques (i.e. acyclic tree). <br />")
            << tr("GCLC = 1, when every node and its neighborhood are complete cliques.")
            << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Clustering Coefficient report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

// Writes the triad census to a file
void Graph::writeTriadCensus(const QString fileName,
                             const bool considerWeights)
{

    qDebug() << "Graph::writeTriadCensus()";

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }

    QTextStream outText(&file);

    progressStatus((tr("Computing triad census. Please wait....")));

    if (!calculatedTriad)
    {
        if (!graphTriadCensus())
        {
            qDebug() << "Error in graphTriadCensus(). Exiting...";
            file.close();
            return;
        }
        if (progressCanceled())
        {
            file.close();
            return;
        }
    }

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QList<QString> triadTypes;
    triadTypes << "003";
    triadTypes << "012";
    triadTypes << "102";
    triadTypes << "021D";
    triadTypes << "021U";
    triadTypes << "021C";
    triadTypes << "111D";
    triadTypes << "111U";
    triadTypes << "030T";
    triadTypes << "030C";
    triadTypes << "201";
    triadTypes << "120D";
    triadTypes << "120U";
    triadTypes << "120C";
    triadTypes << "210";
    triadTypes << "300";

    QString pMsg = tr("Writing Triad Census to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(16, pMsg);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("TRIAD CENSUS (TRC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("A Triad Census counts all the different types (classes) of observed triads within a network. <br />"
                  "The triad types are coded and labeled according to their number of mutual, asymmetric and non-existent (null) dyads. <br />"
                  "SocNetV follows the M-A-N labeling scheme, as described by Holland, Leinhardt and Davis in their studies. <br />"
                  "In the M-A-N scheme, each triad type has a label with four characters: <br />")
            << tr("- The first character is the number of mutual (M) dyads in the triad. Possible values: 0, 1, 2, 3.<br />"
                  "- The second character is the number of asymmetric (A) dyads in the triad. Possible values: 0, 1, 2, 3.<br />"
                  "- The third character is the number of null (N) dyads in the triad. Possible values: 0, 1, 2, 3.<br />"
                  "- The fourth character is inferred from features or the nature of the triad, i.e. presence of cycle or transitivity. "
                  "Possible values: none, D (\"Down\"), U (\"Up\"), C (\"Cyclic\"), T (\"Transitive\")")
            << "<br /></p>";

    outText << "<table class=\"stripes\">";

    outText << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("Type")
            << "</th><th>"
            << tr("Census")
            //            << "</th><th>"
            //            << tr("Expected Value")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody>";

    for (int i = 0; i <= 15; i++)
    {

        progressUpdate(++progressCounter);

        rowCount = i + 1;
        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">"
                << "<td>"
                << triadTypes[i]
                << "</td><td>"
                << triadTypeFreqs[i]
                << "</td>"
                << "</tr>";
    }

    outText << "</tbody></table>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Triad Census report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Calls graphCliques() to compute all cliques (maximal connected subgraphs) of the network.
 * Then writes the results into a file, along with the Actor by clique analysis,
 * the Co-membership matrix and the Hierarchical clustering of overlap matrix
 * @param fileName
 * @param considerWeights
 */
bool Graph::writeCliqueCensus(const QString &fileName,
                              const bool considerWeights)
{

    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug() << "Graph::writeCliqueCensus() ";

    Q_UNUSED(considerWeights);

    QString varLocation = "Both";
    bool dendrogram = true;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return false;
    }

    int N = vertices();
    int cliqueCounter = 0;
    int rowCounter = 0;
    int cliqueSize = 0;
    int actor2 = 0, actor1 = 0, index1 = 0, index2 = 0;
    qreal numerator = 0;
    QString listString;

    VList::const_iterator it, it2;

    QString pMsg = tr("Computing Clique Census and writing it to a file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(2 * N, pMsg);

    // compute clique census
    pMsg = tr("Computing Clique Census. Please wait..");
    progressStatus(pMsg);
    qDebug() << "Graph::writeCliqueCensus() - calling graphCliques";

    csRecDepth = 0;

    // Call graphCliques() to compute all cliques (maximal connected subgraphs) of the network.
    graphCliques();
    if (progressCanceled())
    {
        file.close();
        progressFinish();
        return false;
    }
    pMsg = tr("Writing Clique Census to file. Please wait..");
    progressStatus(pMsg);

    QTextStream outText(&file);

    outText << htmlHead;
    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("CLIQUE CENSUS (CLQs) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("A clique is the largest subgroup of actors in the social network who are all "
                  "directly connected to each other (maximal complete subgraph). <br />"
                  "SocNetV applies the Bron–Kerbosch algorithm to produce a census of all maximal cliques "
                  "in the network and reports some useful statistics such as disaggregation by vertex "
                  "and co-membership information. <br />")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Maximal Cliques found: ")
            << "</span>"
            << m_cliques.size()
            << "</p>";

    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("Clique No")
            << "</th><th>"
            << tr("Clique members")
            << "</th>"
            << "</tr>"
            << "</thead>"
            << "<tbody>";

    foreach (QList<int> clique, m_cliques)
    {

        ++cliqueCounter;

        outText << "<tr class=" << ((cliqueCounter % 2 == 0) ? "even" : "odd") << ">";

        listString.truncate(0);

        while (!clique.empty())
        {
            listString += QString::number(clique.takeFirst());
            if (!clique.empty())
                listString += " ";
        }
        outText << "<td>"
                << cliqueCounter
                << "</td><td>"
                << listString
                << "</td>"
                << "</tr>";
    }
    outText << "</tbody></table>";

    /**
     * Write the actor by clique analysis matrix.
     * For each actor-clique pair, we compute the proportion of clique members adjacent
     */
    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Actor by clique analysis: ")
            << "</span>"
            << tr("Proportion of clique members adjacent")
            << "</p>";

    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("<sub>Actor</sub>/<sup>Clique</sup>")
            << "</th>";

    for (int listIndex = 0; listIndex < cliqueCounter; listIndex++)
    {
        outText << "<th>"
                << listIndex + 1
                << "</th>";
    }

    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    rowCounter = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        rowCounter++;
        actor1 = (*it)->number();
        outText << "<tr class=" << ((rowCounter % 2 == 0) ? "even" : "odd") << ">"
                << "<td class=\"header\">"
                << actor1
                << "</td>";

        foreach (QList<int> clique, m_cliques)
        {
            numerator = 0;

            if (clique.contains(actor1))
            {
                outText << "<td>"
                        << "1.000"
                        << "</td>";
            }
            else
            {
                cliqueSize = clique.size();
                while (!clique.empty())
                {
                    actor2 = clique.takeFirst();
                    if (edgeExists(actor1, actor2))
                    {
                        numerator++;
                    }
                }
                outText << "<td>"
                        << Qt::fixed << (numerator / (qreal)cliqueSize)
                        << "</td>";
            }
        }
        outText << "</tr>";
    }
    outText << "</tbody></table>";

    /**
     * Write the actor by actor analysis matrix.
     * For each pair, we compute their clique co-membership
     */

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Actor by actor analysis: ")
            << "</span>"
            << tr(" Co-membership matrix")
            << "</p>";

    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("<sub>Actor</sub>/<sup>Actor</sup>")
            << "</th>";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        actor1 = (*it)->number();
        outText << "<th>"
                << actor1
                << "</th>";
    }

    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    rowCounter = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        actor1 = (*it)->number();
        index1 = vpos[actor1];
        rowCounter++;
        outText << "<tr class=" << ((rowCounter % 2 == 0) ? "even" : "odd") << ">"
                << "<td class=\"header\">"
                << actor1
                << "</td>";

        for (it2 = m_graph.cbegin(); it2 != m_graph.cend(); ++it2)
        {
            actor2 = (*it2)->number();
            index2 = vpos[actor2];
            outText << "<td>"
                    << qSetRealNumberPrecision(0) << CLQM.item(index1, index2)
                    << "</td>";
        }
        outText << "</tr>";
    }

    outText << "</tbody></table>";

    /**
     * Write the Hierarchical clustering of overlap matrix
     */

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Hierarchical clustering of overlap matrix: ")
            << "</span>"
            << tr("Actors")
            << "</p>";

    pMsg = tr("Computing HCA for Cliques. Please wait..");
    progressStatus(pMsg);
    if (!graphClusteringHierarchical(CLQM,
                                     varLocation,
                                     graphMetricStrToType("Euclidean"),
                                     Clustering::Complete_Linkage,
                                     false,
                                     true,
                                     true,
                                     false,
                                     true))
    {
        file.close();
        progressStatus("Error completing HCA analysis");
        progressFinish();
        return false;
    }

    pMsg = tr("Writing HCA for Cliques. Please wait..");
    progressStatus(pMsg);

    writeClusteringHierarchicalResultsToStream(outText, N, dendrogram);

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Clique by clique analysis: ")
            << "</span>"
            << tr("Co-membership matrix")
            << "</p>";

    progressUpdate(2 * N);

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Hierarchical clustering of overlap matrix: ")
            << "</span>"
            << tr("Clique")
            << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Clique Census Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();

    return true;
}

/**
 * @brief Writes Hierarchical Clustering Analysis to a given file
 * @param fileName
 * @param matrix
 * @param similarityMeasure
 * @param method
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
bool Graph::writeClusteringHierarchical(const QString &fileName,
                                        const QString &varLocation,
                                        const QString &matrix,
                                        const QString &metric,
                                        const QString &method,
                                        const bool &diagonal,
                                        const bool &dendrogram,
                                        const bool &considerWeights,
                                        const bool &inverseWeights,
                                        const bool &dropIsolates)
{

    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug() << "Graph::writeClusteringHierarchical() - matrix:"
             << matrix
             << "varLocation" << varLocation
             << "metric" << metric
             << "method" << method
             << "considerWeights:" << considerWeights
             << "inverseWeights:" << inverseWeights
             << "dropIsolates:" << dropIsolates;

    int N = vertices(dropIsolates, false, true);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return false;
    }

    progressStatus(tr("Computing hierarchical clustering. Please wait... "));

    Matrix STR_EQUIV;

    switch (graphMatrixStrToType(matrix))
    {
    case MATRIX_ADJACENCY:
        createMatrixAdjacency(dropIsolates);
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        STR_EQUIV = AM;
        break;
    case MATRIX_DISTANCES:
        graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        STR_EQUIV = DM;
        break;
    default:
        break;
    }

    if (!graphClusteringHierarchical(STR_EQUIV,
                                     varLocation,
                                     graphMetricStrToType(metric),
                                     graphClusteringMethodStrToType(method),
                                     diagonal,
                                     dendrogram,
                                     considerWeights,
                                     inverseWeights,
                                     dropIsolates))
    {
        qDebug() << "Graph::writeClusteringHierarchical() - HCA failed. Returning...";
        progressStatus("Error completing HCA analysis");
        progressFinish();
        return false;
    }

    QTextStream outText(&file);

    QString pMsg = tr("Writing Hierarchical Cluster Analysis to file. \nPlease wait... ");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);
    outText.reset();

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("HIERARCHICAL CLUSTERING (HCA)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Input matrix: ")
            << "</span>"
            << matrix
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Distance/dissimilarity metric: ")
            << "</span>"
            << metric
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Clustering method/criterion: ")
            << "</span>"
            << method
            << "</p>";

    outText << "<p>&nbsp;</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Analysis results")
            << "</span>"
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Structural Equivalence Matrix: ")
            << "</span>"
            << "</p>";

    progressUpdate(N / 3);
    if (progressCanceled())
    {
        file.close();
        progressFinish();
        return false;
    }
    writeMatrixHTMLTable(outText, STR_EQUIV, true, false, false, dropIsolates);

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Hierarchical Clustering of Equivalence Matrix: ")
            << "</span>"
            << "</p>";

    progressUpdate(2 * N / 3);
    if (progressCanceled())
    {
        file.close();
        progressFinish();
        return false;
    }
    writeClusteringHierarchicalResultsToStream(outText, N, dendrogram);

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Hierarchical Cluster Analysis report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();
    qDebug() << "Graph::writeClusteringHierarchical() - finished";

    progressUpdate(N);
    progressFinish();

    return true;
}

/**
 * @brief Writes Hierarchical Clustering results to given output stream
 * Before running this method, the method Graph::graphClusteringHierarchical()
 * must execute and return true. Otherwise, the result is unpredictable...
 * @param outText
 * @param N
 * @param dendrogram
 */
void Graph::writeClusteringHierarchicalResultsToStream(QTextStream &outText,
                                                       const int N,
                                                       const bool &dendrogram)
{

    qDebug() << "Writing Hierarchical Clustering results to stream. "
             << "N" << N
             << "dendrogram" << dendrogram;

    QMap<int, V_int>::const_iterator it;
    qreal level;

    outText << "<pre>";
    outText << "Seq" << "\t" << "Level" << "\t" << "Actors" << "\n";

    for (it = m_clustersPerSequence.constBegin(); it != m_clustersPerSequence.constEnd(); ++it)
    {
        level = m_clusteringLevel.at(it.key() - 1);
        outText << it.key() << "\t"
                << level << "\t";

        foreach (int item, it.value())
        {
            outText << item << " ";
        }
        outText << "\n";
    }
    outText << Qt::reset << "</pre>";

    if (dendrogram)
    {

        qDebug() << "Writing SVG dendrogram...";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("Clustering Dendrogram (SVG)")
                << "</span>"
                << "</p>";

        int diagramMaxWidth = 1000;
        int diagramPaddingLeft = 30;
        int diagramPaddingTop = 30;
        int rowHeight = 15;
        int rowPaddingLeft = 5;

        int headerHeight = 10;
        int headerTextSize = 9;
        int actorTextSize = 12;
        int legendTextSize = 7;

        int maxSVGWidth = diagramMaxWidth + diagramPaddingLeft + rowPaddingLeft;
        int maxSVGHeight = 2 * diagramPaddingTop + (rowHeight * N);

        QMap<QString, QPoint> clusterEndPoint;
        QPoint endPoint1, endPoint2, endPointLevel;

        QMap<int, V_str>::const_iterator pit; // cluster names pair iterator

        int actorNumber;

        qreal maxLevelValue;
        QString clusterName;
        QList<qreal> legendLevelsDone;

        it = m_clustersPerSequence.constEnd();
        it--;

        maxLevelValue = m_clusteringLevel.last();

        qDebug() << "m_clustersPerSequence" << m_clustersPerSequence
                 << "\n"
                 << "maxLevelValue" << maxLevelValue
                 << "\n"
                 << "m_clusterPairNamesPerSeq" << m_clusterPairNamesPerSeq << "\n"
                 << "m_clustersByName" << m_clustersByName;

        outText << "<div class=\"dendrogram\">";

        outText << "<svg class=\"dendrosvg SocNetV-v" << VERSION
                << "\" width=\"" << maxSVGWidth
                << "\" height=\"" << maxSVGHeight
                << "\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

        // print a legend on top
        outText << "<text font-size=\"" << headerTextSize
                << "\" class=\"header\" x=\"" << 0
                << "\" y=\"" << headerHeight
                << "\">" << "Actor"
                << "</text>";
        outText << "<text font-size=\"" << headerTextSize
                << "\" class=\"header\" x=\"" << diagramMaxWidth / 2
                << "\" y=\"" << headerHeight
                << "\">" << "Clusterings"
                << "</text>";

        // print actor numbers
        // and compute initial cluster end points for them.
        for (int i = 0; i < it.value().size(); ++i)
        {

            actorNumber = it.value().at(i);
            clusterEndPoint[QString::number(actorNumber)] = QPoint(diagramPaddingLeft, diagramPaddingTop + rowHeight * (i));

            outText << "<g class=\"row row-" << i << "\">";
            outText << "<text class=\"actor\" font-size=\"" << actorTextSize
                    << "\" x=\"" << rowPaddingLeft
                    << "\" y=\"" << diagramPaddingTop + (rowHeight * (i)) + actorTextSize / 3
                    << "\">" << actorNumber
                    << "</text>";

            outText << "</g>"; // end actor name

        } // end for rows

        // begin drawing clustering paths/lines
        for (pit = m_clusterPairNamesPerSeq.constBegin(); pit != m_clusterPairNamesPerSeq.constEnd(); ++pit)
        {
            level = m_clusteringLevel.at(pit.key() - 1);
            qDebug() << "seq" << pit.key()
                     << "level" << level
                     << "cluster pair" << pit.value();

            for (int i = 0; i < pit.value().size(); ++i)
            {

                clusterName = pit.value().at(i);
                qDebug() << "clusterName" << clusterName;

                if (i == 0)
                {
                    endPoint1 = clusterEndPoint.value(clusterName, QPoint());
                    qDebug() << "endPoint1" << endPoint1;
                }
                else
                {
                    endPoint2 = clusterEndPoint.value(clusterName, QPoint());
                    qDebug() << "endPoint2" << endPoint2;
                }
            }

            if (endPoint1.isNull() || endPoint2.isNull())
            {
                continue;
            }

            // compute and save new endPoint
            endPointLevel = QPoint(ceil(diagramPaddingLeft + diagramMaxWidth * (level / maxLevelValue)),
                                   ceil(endPoint1.y() + endPoint2.y()) / 2);

            clusterEndPoint.insert("c" + QString::number(pit.key()), endPointLevel);

            qDebug() << "(pit.key() / maxLevelValue)" << (diagramPaddingLeft + level / maxLevelValue)
                     << "endPointLevel" << endPointLevel;

            // print path
            outText << "<path d=\"M "
                    << endPoint1.x()
                    << " " << endPoint1.y()
                    << " L "
                    << endPointLevel.x()
                    << " "
                    << endPoint1.y()
                    << " L "
                    << endPointLevel.x()
                    << " "
                    << endPoint2.y()
                    << " L "
                    << endPoint2.x()
                    << " "
                    << endPoint2.y()
                    << "\" stroke=\"red\" "
                       "stroke-linecap=\"round\" stroke-width=\"1\" fill=\"none\"/>"; // stroke-dasharray=\"5,5\"

            // print level vertical dashed line
            outText << "<path d=\"M "
                    << endPointLevel.x()
                    << " "
                    << diagramPaddingTop - 10
                    << " L "
                    << endPointLevel.x()
                    << " "
                    << diagramPaddingTop + rowHeight * (N)-10
                    << "\" stroke=\"#999\" "
                       "stroke-linecap=\"round\" stroke-dasharray=\"1,2\" stroke-width=\"0.4\" fill=\"none\"/>";

            // print legend
            if (!legendLevelsDone.contains(level))
            {
                outText << "<text class=\"legend\"  writing-mode=\"tb-rl\" "
                           "glyph-orientation-vertical=\"90\" "
                           "font-size=\""
                        << legendTextSize
                        << "\" x=\"" << diagramPaddingLeft + diagramMaxWidth * (level / maxLevelValue) - 5
                        << "\" y=\""
                        << diagramPaddingTop + rowHeight * (N)
                        << "\" >" << Qt::fixed << level << "</text>";
                legendLevelsDone.append(level);
            }
        }

        outText << "</svg>"; // end dendrogram svg

        outText << "</div>"; // end dendrogram div

    } // end if dendrogram
}

/**
 * @brief Writes similarity matrix based on a matching measure to given file
 * @param fileName
 * @param measure
 * @param matrix
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 */
void Graph::writeMatrixSimilarityMatchingPlain(const QString fileName,
                                               const int &measure,
                                               const QString &matrix,
                                               const QString &varLocation,
                                               const bool &diagonal,
                                               const bool &considerWeights)
{

    Q_UNUSED(considerWeights);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    progressStatus((tr("Examining pair-wise similarity of actors...")));

    Matrix SCM;
    if (matrix == "Adjacency")
    {
        createMatrixAdjacency();
        createMatrixSimilarityMatching(AM, SCM, measure, varLocation, diagonal, considerWeights);
    }
    else if (matrix == "Distances")
    {
        graphDistancesGeodesic();
        createMatrixSimilarityMatching(DM, SCM, measure, varLocation, diagonal, considerWeights);
    }
    else
    {
        return;
    }

    progressStatus(tr("Writing similarity coefficients to file: ") + fileName);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << tr("SIMILARITY MATRIX: MATCHING COEFFICIENTS (SMMC)") << "\n\n";

    outText << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Network name: ") << Qt::reset << getName() << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Input matrix: ") << Qt::reset << matrix << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Variables in: ") << Qt::reset << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation) << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Matching measure: ") << Qt::reset;

    outText << graphMetricTypeToString(measure);

    outText << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Diagonal: \t") << Qt::reset << ((diagonal) ? "Included" : "Not included") << "\n\n";

    outText << tr("Analysis results") << "\n\n";
    if (measure == METRIC_HAMMING_DISTANCE)
        outText << tr("SMMC range: 0 < C") << "\n\n";
    else
        outText << tr("SMMC range: 0 < C < 1") << "\n\n";

    outText << SCM;

    outText << "\n";

    if (measure == METRIC_HAMMING_DISTANCE)
    {
        outText << tr("SMMC = 0, when two actors are absolutely similar (no tie/distance differences).") << "\n";
        outText << tr(
            "SMMC > 0, when two actors have some differences in their ties/distances, \n"
            "i.e. SMMC = 3 means the two actors have 3 differences in their tie/distance profiles to other actors.");
    }
    else
    {
        outText << tr("SMMC = 0, when there is no tie profile similarity at all.") << "\n";
        outText << tr(
            "SMMC > 0, when two actors have some matches in their ties/distances, \n"
            "i.e. SMMC = 1 means the two actors have their ties to other actors exactly the same all the time.");
    }

    outText << "\n\n";

    outText << tr("Similarity Matrix by Matching Measure Report,\n");
    outText << tr("Created by SocNetV ") << VERSION << ": "
            << actualDateTime.currentDateTime()
                   .toString(QString("ddd, dd.MMM.yyyy hh:mm:ss"))
            << "\n\n";

    file.close();
}

/**
 * @brief Writes dissimilarity matrix based on a metric/measure to given html file
 * @param fileName
 * @param measure
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 */
void Graph::writeMatrixDissimilarities(const QString fileName,
                                       const QString &metricStr,
                                       const QString &varLocation,
                                       const bool &diagonal,
                                       const bool &considerWeights)
{

    qDebug() << "Graph::writeMatrixDissimilarities()"
             << "metric" << metricStr
             << "varLocation" << varLocation
             << "diagonal" << diagonal;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file for writing. Abort.";
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    Matrix DSM;
    int N = vertices();

    createMatrixAdjacency();

    progressStatus((tr("Examining pair-wise tie profile dissimilarities of actors...")));

    int metric = graphMetricStrToType(metricStr);
    createMatrixDissimilarities(AM, DSM, metric, varLocation, diagonal, considerWeights);

    progressStatus(tr("Writing tie profile dissimilarities to file: ") + fileName);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("DISSIMILARITIES MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            << "</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Metric: ")
            << "</span>"
            << metricStr
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            << "</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Range: ")
            << "</span>";

    if (metric == METRIC_JACCARD_INDEX)
        outText << tr("0 &lt; C &lt; 1");
    else
        outText << tr("0 &lt; C ");
    outText << "</p>";

    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Analysis results ")
            << "</span>"
            << "</p>";

    writeMatrixHTMLTable(outText, DSM, true);
    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("DSM = 0 ")
            << "</span>"
            << tr("when two actors have no tie profile dissimilarities. The actors have the same ties to all others.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("DSM &gt; 0 ")
            << "</span>"
            << tr("when the two actors have differences in their ties to other actors.");
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Dissimilarity Matrix Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();
}

/**
 * @brief Writes similarity matrix based on a matching measure to given html file
 * @param fileName
 * @param measure
 * @param matrix
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 */
void Graph::writeMatrixSimilarityMatching(const QString fileName,
                                          const QString &measure,
                                          const QString &matrix,
                                          const QString &varLocation,
                                          const bool &diagonal,
                                          const bool &considerWeights)
{

    qDebug() << "Writing similarity matrix to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    int measureInt = graphMetricStrToType(measure);

    Q_UNUSED(considerWeights);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    progressStatus((tr("Examining pair-wise similarity of actors...")));

    Matrix SCM;
    int N = vertices();

    if (matrix == "Adjacency")
    {
        createMatrixAdjacency();
        createMatrixSimilarityMatching(AM, SCM, measureInt,
                                       varLocation, diagonal, considerWeights);
    }
    else if (matrix == "Distances")
    {
        graphDistancesGeodesic();
        createMatrixSimilarityMatching(DM, SCM, measureInt,
                                       varLocation, diagonal, considerWeights);
    }
    else
    {
        return;
    }

    QString pMsg = tr("Writing Similarity coefficients to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(1, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("SIMILARITY MATRIX: MATCHING COEFFICIENTS (SMMC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Input matrix: ")
            << "</span>"
            << matrix
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            << "</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Matching measure: ")
            << "</span>"
            << measure
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            << "</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("SMMC range: ")
            << "</span>";

    if (measureInt == METRIC_HAMMING_DISTANCE)
        outText << tr("0 &lt; C");
    else
        outText << tr("0 &lt; C &lt; 1");
    outText << "</p>";

    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Analysis results ")
            << "</span>"
            << "</p>";

    progressUpdate(0);

    writeMatrixHTMLTable(outText, SCM, true);

    outText << "<p class=\"description\">";
    if (measureInt == METRIC_HAMMING_DISTANCE)
    {
        outText << "<span class=\"info\">"
                << tr("SMMC = 0 ")
                << "</span>"
                << tr("when two actors are absolutely similar (no tie/distance differences).")
                << "<br/>"
                << "<span class=\"info\">"
                << tr("SMMC &gt; 0 ")
                << "</span>"
                << tr("when two actors have some differences in their ties/distances, "
                      "i.e. SMMC = 3 means the two actors have 3 differences in their tie/distance profiles to other actors.");
    }
    else
    {
        outText << "<span class=\"info\">"
                << tr("SMMC = 0 ")
                << "</span>"
                << tr("when there is no tie profile similarity at all.")
                << "<br/>"
                << "<span class=\"info\">"
                << tr("SMMC &gt; 0 ")
                << "</span>"
                << tr("when two actors have some matches in their ties/distances, "
                      "i.e. SMMC = 1 means the two actors have their ties to other actors exactly the same all the time.");
    }
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Similarity Matrix by Matching Measure Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressUpdate(1);
    progressFinish();
}

/**
 * @brief Calls Graph::createMatrixSimilarityPearson() and
 * writes Pearson Correlation Coefficients to given file
 * @param fileName
 * @param considerWeights
 */
void Graph::writeMatrixSimilarityPearson(const QString fileName,
                                         const bool considerWeights,
                                         const QString &matrix,
                                         const QString &varLocation,
                                         const bool &diagonal)
{

    qDebug() << "Writing Pearson Correlation coefficients to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    progressStatus((tr("Calculating Pearson Correlations...")));

    Matrix PCC;
    int N = vertices();

    if (matrix == "Adjacency")
    {
        createMatrixAdjacency();
        createMatrixSimilarityPearson(AM, PCC, varLocation, diagonal);
    }
    else if (matrix == "Distances")
    {
        graphDistancesGeodesic();
        createMatrixSimilarityPearson(DM, PCC, varLocation, diagonal);
    }
    else
    {
        return;
    }

    progressStatus(tr("Writing Pearson coefficients to file: ") + fileName);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("PEARSON CORRELATION COEFFICIENTS (PCC) MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Input matrix: ")
            << "</span>"
            << matrix
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            << "</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            << "</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PCC range: ")
            << "</span>"
            << "-1 &lt; C &lt; 1"
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << "<br />"
            << tr("Analysis results ")
            << "</span>"
            << "</p>";

    writeMatrixHTMLTable(outText, PCC, true);

    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("PCC = 0 ")
            << "</span>"
            << tr("when there is no correlation at all.")
            << "<br/>"
            << "<span class=\"info\">"
            << tr("PCC &gt; 0 ")
            << "</span>"
            << tr("when there is positive correlation, "
                  "i.e. +1 means actors with same patterns of ties/distances.")
            << "<br />"
            << "<span class=\"info\">"
            << tr("PCC &lt; 0 ")
            << "</span>"
            << tr("when there is negative correlation, "
                  "i.e. -1 for actors with exactly opposite patterns of ties.")
            << "<br/>";
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Pearson Correlation Coefficients Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();
}

/**
 * @brief
 * Calls Graph::graphSimilariyPearsonCorrelationCoefficients() and
 * writes Pearson Correlation Coefficients to given file
 * @param fileName
 * @param considerWeights
 */
void Graph::writeMatrixSimilarityPearsonPlainText(const QString fileName,
                                                  const bool considerWeights,
                                                  const QString &matrix,
                                                  const QString &varLocation,
                                                  const bool &diagonal)
{
    qDebug() << "Writing Pearson Correlation coefficients (plain text) to file:" << fileName;

    Q_UNUSED(considerWeights);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    progressStatus((tr("Calculating Pearson Correlations...")));

    Matrix PCC;
    if (matrix == "Adjacency")
    {
        createMatrixAdjacency();
        createMatrixSimilarityPearson(AM, PCC, varLocation, diagonal);
    }
    else if (matrix == "Distances")
    {
        graphDistancesGeodesic();
        createMatrixSimilarityPearson(DM, PCC, varLocation, diagonal);
    }
    else
    {
        return;
    }

    progressStatus(tr("Writing Pearson coefficients to file: ") + fileName);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << tr("PEARSON CORRELATION COEFFICIENTS (PCC) MATRIX") << "\n\n";

    outText << tr("Network name: ") << getName() << "\n"
            << tr("Input matrix: ") << matrix << "\n"
            << tr("Variables in: ") << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "\n\n";
    outText << tr("Analysis results") << "\n\n";

    outText << tr("PCC range: -1 < C < 1") << "\n";

    outText << PCC;

    outText << "\n";
    outText << tr("PCC = 0, when there is no correlation at all.\n");
    outText << tr(
        "PCC > 0, when there is positive correlation, i.e. +1 means actors with same patterns of ties/distances.\n");
    outText << tr(
        "PCC < 0, when there is negative correlation, i.e. -1 for actors with exactly opposite patterns of ties.\n");
    outText << "\n\n";
    outText << tr("Pearson Correlation Coefficients Report,\n");
    outText << tr("Created by SocNetV ") << VERSION << ": "
            << actualDateTime.currentDateTime()
                   .toString(QString("ddd, dd.MMM.yyyy hh:mm:ss"))
            << "\n\n";

    file.close();
}

/**
 * @brief Writes a "famous" dataset to the given file
 * Datasets are hardcoded! They are exported in the given fileName...
 *
 * TODO: Move all these datasets to a separate class
 *
 * @param fileName
 */
void Graph::writeDataSetToFile(const QString dir, const QString fileName)
{

    qDebug() << "Writing famous dataset to file:" << dir + fileName;

    QFile file(dir + fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        progressStatus(tr("Error. Could not write to ") + fileName);
        return;
    }
    QTextStream outText(&file);

    QString datasetDescription = QString();

    QFile virtualFile(":/data/" + fileName);
    if (virtualFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        outText << virtualFile.readAll();
    }
    else
    {
        qWarning() << "Could not load resource: " << fileName;
        return;
    }
    virtualFile.close();

    qDebug() << "		... writing dataset ";
    if (fileName == "Campnet.paj")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr("Campnet dataset\n\n"
                                "The dataset is the interactions among 18 people, "
                                "including 4 instructors, "
                                "participating in a 3-week workshop. \n"
                                "Each person was asked to rank everyone else in terms of "
                                "how much time they spent with them.\n"
                                "This dataset shows only top 3 choices for each respondent"
                                "(week 2 and week 3). Thus, there is a 1 for xij "
                                "if person i listed person j as one of their top 3 interactors.\n\n"

                                "The Camp data were collected by Steve Borgatti, "
                                "Russ Bernard and Bert Pelto in 1992 at the NSF Summer "
                                "Institute for Ethnographic Research Methods.\n "
                                "During the 3-week workshop, all the participants and "
                                "instructors were housed at the same motel and spent "
                                "a great deal of time together. \n"
                                "The participants were all faculty in Anthropology "
                                "except Holly, who was a PhD student. ");
    }
    if (fileName == "Herschel_Graph.paj")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr("Herschel graph \n\n"
                                "The Herschel graph is the smallest nonhamiltonian "
                                "polyhedral graph. \n"
                                "It is the unique such graph on 11 nodes, "
                                "and has 18 edges.");
    }
    else if (fileName == "Krackhardt_High-tech_managers.paj")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr("High-tech Managers\n\n"
                                "Krackhardt's High-tech Managers is a famous social network "
                                "of 21 managers of a high-tech US company. \n\n"
                                "The company manufactured high-tech equipment "
                                "and had just over 100 employees with 21 managers. "
                                "David Krackhardt collected the data to assess the effects "
                                "of a recent management intervention program. \n\n"
                                "The network consists of 3 relations:\n"
                                "- Advice\n"
                                "- Friendship\n"
                                "- Reports To\n"
                                "Each manager was asked to whom do you go to for advice and who is your friend. "
                                "Data for the \"whom do you report\" relation was taken from company documents. \n\n"
                                "This data is used by Wasserman and Faust in their seminal network analysis book.\n\n"
                                "Krackhardt D. (1987). Cognitive social structures. Social Networks, 9, 104-134.");
    }

    else if (fileName == "Padgett_Florentine_Families.paj")
    {
        datasetDescription = tr("Padgett's Florentine_Families\n\n"
                                "This famous data set includes 16 families who were fighting "
                                "each other to gain political control of the city of Florence "
                                "circa 1430. Among the 16 families, the Medicis and the Strozzis "
                                "were the two most prominent with factions formed around them.\n\n"

                                "The data set is actually a subset of the original data on social "
                                "relations among 116 Renaissance Florentine Families collected "
                                "by John Padgett. This subset was used by Breiger & Pattison (1986) "
                                "in their paper about local role analysis.\n\n"

                                "Padgett researched historical documents to code two relations: "
                                "Business ties (loans, credits, partnerships)\n"
                                "Marrital ties (marriage alliances).\n\n"

                                "Breiger R. and Pattison P. (1986). Cumulated social roles: The "
                                "duality of persons and their algebras. Social Networks, 8, 215-256. ");
    }
    else if (fileName == "Zachary_Karate_Club.dl")
    {
        datasetDescription = tr("Zachary Karate Club \n\n"
                                "The Zachary Karate Club is a well-known social network of 34 members"
                                " of a university karate club studied by Wayne W. Zachary from 1970 to 1972.\n\n"
                                "During the study, disputes among two members led to club splitting into two groups. "
                                "Zachary documented 78 ties between members who interacted outside the club and "
                                "used the collected data and an information flow model to explain the split-up. \n\n"
                                "There are two relations (matrices) in this network:"
                                "The ZACHE relation represents the presence or absence of ties among the actors. "
                                "The ZACHC relation indicates the relative strength of their associations "
                                "(number of situations in and outside the club in which interactions occurred).\n\n"
                                "Zachary W. (1977). An information flow model for conflict and fission in small groups. "
                                "Journal of Anthropological Research, 33, 452-473. ");
    }
    else if (fileName == "Galaskiewicz_CEOs_and_clubs_affiliation_network_data.2sm")
    {
        datasetDescription = tr("Galaskiewicz's CEOs and Clubs\n\n"
                                "The affiliation network of the chief executive officers "
                                "and their spouses from 26 corporations and banks in 15 clubs, "
                                "corporate and cultural boards. "
                                "Membership was during the period 1978-1981\n\n"
                                "This is a 26x15 affiliation matrix, where the rows "
                                "correspond to the 26 CEOs and the columns to the 15 clubs. \n\n"
                                "This data  was originally collected by Galaskiewicz (1985) "
                                "and is used by Wasserman and Faust in Social Network Analysis: Methods and Applications (1994).\n\n"
                                "Galaskiewicz, J. (1985). Social Organization of an Urban Grants Economy. New York: Academic Press. ");
        outText << "0 0 1 1 0 0 0 0 1 0 0 0 0 0 0" << "\n"
                << "0 0 1 0 1 0 1 0 0 0 0 0 0 0 0" << "\n"
                << "0 0 1 0 0 0 0 0 0 0 0 1 0 0 0" << "\n"
                << "0 1 1 0 0 0 0 0 0 0 0 0 0 0 1" << "\n"
                << "0 0 1 0 0 0 0 0 0 0 0 0 1 1 0" << "\n"
                << "0 1 1 0 0 0 0 0 0 0 0 0 0 1 0" << "\n"
                << "0 0 1 1 0 0 0 0 0 1 1 0 0 0 0" << "\n"
                << "0 0 0 1 0 0 1 0 0 1 0 0 0 0 0" << "\n"
                << "1 0 0 1 0 0 0 1 0 1 0 0 0 0 0" << "\n"
                << "0 0 1 0 0 0 0 0 1 0 0 0 0 0 0" << "\n"
                << "0 1 1 0 0 0 0 0 1 0 0 0 0 0 0" << "\n"
                << "0 0 0 1 0 0 1 0 0 0 0 0 0 0 0" << "\n"
                << "0 0 1 1 1 0 0 0 1 0 0 0 0 0 0" << "\n"
                << "0 1 1 1 0 0 0 0 0 0 1 1 1 0 1" << "\n"
                << "0 1 1 0 0 1 0 0 0 0 0 0 1 0 1" << "\n"
                << "0 1 1 0 0 1 0 1 0 0 0 0 0 1 0" << "\n"
                << "0 1 1 0 1 0 0 0 0 0 1 1 0 0 1" << "\n"
                << "0 0 0 1 0 0 0 0 1 0 0 1 1 0 1" << "\n"
                << "1 0 1 1 0 0 1 0 1 0 0 0 0 0 0" << "\n"
                << "0 1 1 1 0 0 0 0 0 0 1 0 0 0 1" << "\n"
                << "0 0 1 1 0 0 0 1 0 0 0 0 0 0 0" << "\n"
                << "0 0 1 0 0 0 0 1 0 0 0 0 0 0 1" << "\n"
                << "0 1 1 0 0 1 0 0 0 0 0 0 0 0 1" << "\n"
                << "1 0 1 1 0 1 0 0 0 0 0 0 0 0 1" << "\n"
                << "0 1 1 0 0 0 0 0 0 0 0 0 1 0 0" << "\n"
                << "0 1 1 0 0 0 0 0 0 0 0 1 0 0 0";
    }
    else if (fileName == "Thurman_Office_Networks_Coalitions.dl")
    {
        datasetDescription = tr("Thurman's Office Networks and Coalitions\n\n"
                                "In the late 70s, B. Thurman spent 16 months "
                                "observing the interactions among employees in "
                                "the overseas office of a large international "
                                "corporation. \n"
                                "During this time, two major disputes erupted "
                                "in a subgroup of fifteen people. \n"
                                "Thurman analyzed the outcome of these disputes "
                                "in terms of the network of formal and informal "
                                "associations among those involved.\n"
                                "\n"
                                "This labeled dataset contains two relations (15x15 matrices): \n"
                                "THURA is a 15x15 non-symmetric, binary matrix showing "
                                "the formal organizational chart of the employees.\n\n"
                                "THURM is a 15x15 symmetric binary matrix which "
                                "shows the actors linked by multiplex ties. \n\n"
                                "Thurman B. (1979). In the office: Networks and coalitions. Social Networks, 2, 47-63");
    }
    else if (fileName == "Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl")
    {
        datasetDescription = tr("Corporate Interlocks in Netherlands\n\n"
                                "A 16x16 symmetric, binary matrix."
                                "This data represent corporate interlocks among "
                                "the major business entities in the Netherlands. "
                                "The data were gathered during a 6-year research "
                                "project which was concluded in 1976 in nine "
                                "European countries and the USA \n\n"
                                "Stokman F., Wasseur F. and Elsas D. (1985). "
                                "The Dutch network: "
                                "Types of interlocks and network structure. "
                                "In F. Stokman, R. Ziegler & J. Scott (eds), "
                                "Networks of corporate power. Cambridge: Polity Press, 1985");
    }

    else if (fileName == "Stokman_Ziegler_Corporate_Interlocks_West_Germany.dl")
    {
        datasetDescription = tr("Corporate Interlocks in West Germany\n\n"
                                "A 15x15 symmetric, binary matrix."
                                "This data represent corporate interlocks among "
                                "the major business entities in the West Germany. "
                                "The data were gathered during a 6-year research "
                                "project which was concluded in 1976 in nine "
                                "European countries and the USA \n\n"
                                "Ziegler R., Bender R. and Biehler H. (1985). "
                                "Industry and banking in the German corporate "
                                "network. "
                                "In F. Stokman, R. Ziegler & J. Scott (eds), "
                                "Networks of corporate  power. Cambridge: Polity Press, 1985. ");
    }

    else if (fileName == "Bernard_Killworth_Fraternity.dl")
    {
        datasetDescription =
            tr("Bernard and Killworth Fraternity\n\n"
               "Bernard & Killworth recorded the interactions among students living in a fraternity at "
               "a West Virginia college. Subjects had been residents in the fraternity from 3 months to 3 years. "
               "This network dataset contains two relations: \n\n"
               "The BKFRAB relation is symmetric and valued. It counts the number of times a pair of subjects were "
               "seen in conversation by an unobtrusive observer (observation time: 21 hours a day, for five days). \n\n"
               "The BKFRAC relation is non-symmetric and valued. Contains rankings made by the subjects themselves of "
               "how frequently they interacted with other subjects in the observation week. \n\n"
               "Knoke D. and Wood J. (1981). Organized for action: Commitment in voluntary associations. "
               "New Brunswick, NJ: Rutgers University Press. Knoke D. and Kuklinski J. (1982). "
               "Network analysis, Beverly Hills, CA: Sage");
    }
    else if (fileName == "Freeman_EIES_networks_32actors.dl")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr(
            "Freeman's EIES Networks\n\n"
            "This data comes from an early experiment on computer mediated communication. \n"
            "Fifty academics were allowed to contact each other via an "
            "Electronic Information Exchange System (EIES). "
            "The data collected consisted of all messages sent plus acquaintance "
            "relationships at two time periods.\n\n"

            "The data includes the 32 actors who completed the study and \n"
            "the following three 32x32 relations: \n\n"
            "TIME_1 non-symmetric, valued\n"
            "TIME_2 non-symmetric, valued\n"
            "NUMBER_OF_MESSAGES non-symmetric, valued\n\n"

            "TIME_1 and TIME_2 give the acquaintance information at the beginning "
            "and end of the study. This is coded as follows: \n"
            "4 = close personal fiend, \n"
            "3 = friend, \n"
            "2= person I've met, \n"
            "1 = person I've heard of but not met, and \n"
            "0 = person unknown to me (or no reply). \n\n"
            "NUMBER_OF MESSAGES is the total number of messages person i \n"
            "sent to j over the entire period of the study. ");
    }
    else if (fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time_1.dl")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr("Freeman's EIES network (Acquaintanceship) at time 1");
    }
    else if (fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time_2.dl")
    {
        qDebug() << "		... to  " << fileName;
        datasetDescription = tr("Freeman's EIES network (Acquaintanceship) at time 2");
    }
    else if (fileName == "Freeman_EIES_network_48actors_Messages.dl")
    {
        datasetDescription = tr("Freeman's EIES network (Messages)");
        qDebug() << "		... to  " << fileName;
    }
    else if (fileName == "Freeman_34_possible_graphs_with_N_5_multirelational.paj")
    {
        datasetDescription = tr("Freeman's 34 possible graphs of N=5\n\n"
                                "This data comes from Freeman's (1979) seminal paper "
                                "\"Centrality in social networks\".\n"
                                "It illustrates all 34 possible graphs of five nodes. \n"
                                "Freeman used them to calculate and compare the three measures "
                                "of Centrality: Degree, Betweenness and Closeness. \n"
                                "Use Relation buttons on the toolbar to move between the graphs.");
    }
    else if (fileName == "Mexican_Power_Network_1940s.lst")
    {
        datasetDescription = tr("Mexican Power Network in the 1940s\n\n");
    }
    else if (fileName == "Knoke_Bureaucracies_Network.pajek")
    {
        datasetDescription = tr("Knoke Bureaucracies\n\n"
                                "In 1978, Knoke & Wood collected data from workers at 95 organizations in Indianapolis. "
                                "Respondents indicated with which other organizations their own organization had any "
                                "of 13 different types of relationships. \n"
                                "Knoke and Kuklinski (1982) selected a subset of 10 organizations and two relationships: "
                                "information exchange and money exchange.\n"
                                "This dataset is directed and not symmetric.\n"
                                "Information exchange is recorded in KNOKI relation while money exchange in KNOKM .");
    }
    else if (fileName == "Stephenson_Zelen_40_AIDS_patients_sex_contact.paj")
    {
        qDebug() << "Stephenson_Zelen_40_AIDS_patiens";
        datasetDescription = tr("Stephenson & Zelen's AIDS patients network (sex contact)\n\n"
                                "The data described by Auerbach et al. (1984) and Klovdahl (1985) consists of information on 40 homosexual men diagnosed with AIDS. "
                                "Initially, 19 men residing in the Los Angeles and Orange County area were interviewed about their previous sexual contacts. "
                                "This information led to the subsequent identification of an additional 21 sexual partners in San Francisco, New York and other parts of the United States. "
                                "All 40 homosexual men were linked to each other through sexual contact.");
    }
    else if (fileName == "Stephenson_Zelen_5actors_6edges_IC_test_dataset.paj")
    {
        qDebug() << "Stephenson_Zelen_5actors_6edges_IC_test_dataset.paj";
    }

    else if (fileName == "Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj")
    {
        datasetDescription = tr("Galada baboon colony network (H22a) \n\n"
                                "A network of the Galada baboon colony, as described by Dunbar and Dunbar (1975). This is the first set of observations (H22a) and was made on 12 baboons.\n\n"
                                "The lines connecting two points (baboons) represent nonagonistic interactions (generally grooming behavior) and the frequency of such interactions is recorded by the edge weight. "
                                "Data derived from Stephenson & Zelen seminal 1989 paper where they introduced Information Centrality.");
    }
    else if (fileName == "Wasserman_Faust_7actors_star_circle_line_graphs.paj")
    {
        qDebug() << "Wasserman_Faust_7actors_star_circle_line_graphs.paj";
        datasetDescription = tr("Wasserman & Faust's 7 actors graphs\n\n");
    }
    else if (fileName == "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek")
    {
        datasetDescription = tr("Wasserman & Faust's Countries Trade Data (manufactured goods)\n\n");
        qDebug() << "		Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek written... ";
    }
    else if (fileName == "Petersen_Graph.paj")
    {
        qDebug() << "		Petersen_Graph.paj written... ";
        datasetDescription = tr("This data set is just a famous non-planar mathematical graph, \n"
                                "named after Julius Petersen, who constructed it in 1898.\n"
                                "The Petersen graph is undirected with 10 vertices and 15 edges \n"
                                "and the smallest bridgeless cubic graph with no three-edge-coloring.\n"
                                "This small graph serves as a useful example and counterexample \n"
                                "for many problems in graph theory. ");
    }
    file.close();
    if (!datasetDescription.isEmpty())
    {
        emit signalDatasetDescription(datasetDescription);
    }
}

/**
    Writes the specified matrix of social network to file fn
*/
bool Graph::writeMatrix(const QString &fn,
                        const int &matrix,
                        const bool &considerWeights,
                        const bool &inverseWeights,
                        const bool &dropIsolates,
                        const QString &varLocation,
                        const bool &simpler)
{

    qDebug() << "Writing specified matrix:" << matrix << "to file:" << fn << " -- dropIsolates:" << dropIsolates;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(simpler);

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fn;
        progressStatus(tr("Error. Could not write to ") + fn);
        return false;
    }

    bool inverseResult = false;
    int N = vertices();

    switch (matrix)
    {
    case MATRIX_ADJACENCY:
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Adjacency recomputed. Writing Adjacency Matrix..."));
        break;
    case MATRIX_LAPLACIAN:
        progressStatus(tr("Need to recompute Adjacency Matrix. Please wait..."));
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Adjacency recomputed. Writing Laplacian Matrix..."));
        break;
    case MATRIX_DEGREE:
        progressStatus(tr("Need to recompute Adjacency Matrix. Please wait..."));
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Adjacency recomputed. Writing Degree Matrix..."));
        break;
    case MATRIX_DISTANCES:
        graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Distances recomputed. Writing Distances Matrix..."));
        break;
    case MATRIX_GEODESICS:
        graphMatrixShortestPathsCreate(considerWeights, inverseWeights, false);
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Distances recomputed. Writing Shortest Paths Matrix..."));
        break;
    case MATRIX_ADJACENCY_INVERSE:
        progressStatus(tr("Computing Inverse Adjacency Matrix. Please wait..."));
        inverseResult = createMatrixAdjacencyInverse(QString("lu"));
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Inverse Adjacency Matrix computed. Writing Matrix..."));
        break;
    case MATRIX_REACHABILITY:
        createMatrixReachability();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Writing Reachability Matrix..."));
        break;

    case MATRIX_ADJACENCY_TRANSPOSE:
        progressStatus(tr("Need to recompute Adjacency Matrix. Please wait..."));
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Adjacency recomputed. Writing Adjacency Matrix..."));
        break;
    case MATRIX_COCITATION:
        progressStatus(tr("Need to recompute Adjacency Matrix. Please wait..."));
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Adjacency recomputed. Writing Adjacency Matrix..."));
        break;
    case MATRIX_DISTANCES_HAMMING:
    case MATRIX_DISTANCES_JACCARD:
    case MATRIX_DISTANCES_MANHATTAN:
    case MATRIX_DISTANCES_EUCLIDEAN:
        progressStatus(tr("Need to recompute tie profile distances. Please wait..."));
        createMatrixAdjacency();
        if (progressCanceled())
        {
            file.close();
            return false;
        }
        progressStatus(tr("Tie profile distances recomputed. Writing matrix..."));
        break;

    default:
        break;
    }

    QTextStream outText(&file);

    outText << htmlHead;

    outText << "<h1>";

    switch (matrix)
    {
    case MATRIX_ADJACENCY:
        outText << tr("ADJACENCY MATRIX REPORT");
        break;
    case MATRIX_LAPLACIAN:
        outText << tr("LAPLACIAN MATRIX REPORT");
        break;
    case MATRIX_DEGREE:
        outText << tr("DEGREE MATRIX REPORT");
        break;
    case MATRIX_DISTANCES:
        outText << tr("DISTANCES MATRIX REPORT");
        break;
    case MATRIX_GEODESICS:
        outText << tr("SHORTEST PATHS (GEODESICS) MATRIX REPORT");
        break;
    case MATRIX_ADJACENCY_INVERSE:
        outText << tr("INVERSE ADJACENCY MATRIX REPORT");
        break;
    case MATRIX_REACHABILITY:
        outText << tr("REACHABILITY MATRIX REPORT");
        break;
    case MATRIX_ADJACENCY_TRANSPOSE:
        outText << tr("TRANSPOSE OF ADJACENCY MATRIX REPORT");
        break;
    case MATRIX_COCITATION:
        outText << tr("COCITATION MATRIX REPORT");
        break;
    case MATRIX_DISTANCES_EUCLIDEAN:
        outText << tr("EUCLIDEAN DISTANCE MATRIX REPORT");
        break;
    case MATRIX_DISTANCES_HAMMING:
        outText << tr("HAMMING DISTANCE MATRIX REPORT");
        break;
    case MATRIX_DISTANCES_JACCARD:
        outText << tr("JACCARD DISTANCE MATRIX REPORT");
        break;
    case MATRIX_DISTANCES_MANHATTAN:
        outText << tr("MANHATTAN DISTANCE MATRIX REPORT");
        break;
    default:
        break;
    }

    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    switch (matrix)
    {
    case MATRIX_ADJACENCY:
        outText << "<p class=\"description\">"
                << tr("The adjacency matrix, AM, of a social network is a NxN matrix ")
                << tr("where each element (i,j) is the value of the edge from "
                      "actor i to actor j, or 0 if no edge exists.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, AM, true, false, false);
        break;
    case MATRIX_LAPLACIAN:
        outText << "<p class=\"description\">"
                << tr("The laplacian matrix L of a social network is a NxN matrix ")
                << tr("with L = D - A, where D the degree matrix and A the "
                      "adjacency matrix. ")
                << "<br />"
                << tr("The elements of L are: "
                      "<br />"
                      "- L<sub>i,j</sub> = d<sub>i</sub>, if i = j, <br />"
                      "- L<sub>i,j</sub> = -1,  if i &ne; j and there is an edge (i,j)<br />"
                      "- and all other elements zero.<br />")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, AM.laplacianMatrix(), true, false, false);
        break;
    case MATRIX_DEGREE:
        outText << "<p class=\"description\">"
                << tr("The degree matrix D of a social network is a NxN matrix ")
                << tr("where each element (i,i) is the degree of actor i "
                      "and all other elements are zero.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, AM.degreeMatrix(), true, false, false);
        break;
    case MATRIX_DISTANCES:
        outText << "<p class=\"description\">"
                << tr("The distance matrix of a social network is a NxN matrix "
                      "where each element (i,j) is the geodesic distance "
                      "(length of shortest path) from actor i to actor j, "
                      "or infinity if no shortest path exists.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, DM, true);
        break;
    case MATRIX_GEODESICS:
        outText << "<p class=\"description\">"
                << tr("The geodesics matrix of a social network is a NxN matrix ")
                << tr("where each element (i,j) is the number of shortest paths"
                      "(geodesics) from actor i to actor j, "
                      "or infinity if no shortest path exists.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, SIGMA, true);
        break;

    case MATRIX_ADJACENCY_INVERSE:
        if (!inverseResult)
        {
            outText << "<p class=\"description\">"
                    << tr("The adjacency matrix is singular.")
                    << "<br />"
                    << "</p>";
        }
        else
        {
            writeMatrixHTMLTable(outText, invAM, true);
        }
        break;
    case MATRIX_REACHABILITY:
        outText << "<p class=\"description\">"
                << tr("The reachability matrix R of a social network is a NxN matrix "
                      "where each element R(i,j) is 1 if actors j is reachable from i "
                      "otherwise 0. <br />"
                      "Two nodes are reachable if there is a walk between them "
                      "(their geodesic distance is non-zero). <br />"
                      "Essentially the reachability matrix is a dichotomized "
                      "geodesics matrix.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, XRM, true);
        break;

    case MATRIX_ADJACENCY_TRANSPOSE:
        outText << "<p class=\"description\">"
                << tr("The adjacency matrix AM of a social network is a NxN matrix "
                      "where each element (i,j) is the value of the edge from "
                      "actor i to actor j, or 0 if no edge exists. ")
                << "<br />"
                << tr("This is the transpose of the adjacency matrix, AM<sup>T</sup>, "
                      "a matrix whose (i,j) element is the (j,i) element of AM.")
                << "</p>";

        writeMatrixHTMLTable(outText, AM.transpose(), true);
        break;

    case MATRIX_COCITATION:
        outText << "<p class=\"description\">"
                << tr("The Cocitation matrix, C = A<sup>T</sup> * A, is a "
                      "NxN matrix where each element (i,j) is the number of "
                      "actors that have outbound ties/links to both actors i and j.")
                << "<br />"
                << tr("The diagonal elements, C<sub>ii</sub>, of the Cocitation "
                      "matrix are equal to the number of inbound edges of i (inDegree).")
                << "<br />"
                << tr("C is a symmetric matrix.")
                << "</p>";
        writeMatrixHTMLTable(outText, AM.cocitationMatrix(), true);
        break;

    case MATRIX_DISTANCES_EUCLIDEAN:
        outText << "<p class=\"description\">"
                << tr("The Euclidean distances matrix is a "
                      "NxN matrix where each element (i,j) is the Euclidean distance"
                      "of the tie profiles between actors i and j, namely the "
                      "square root of the sum of their squared differences.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_EUCLIDEAN_DISTANCE, varLocation, false, true),
                             true, false, false);
        break;
    case MATRIX_DISTANCES_HAMMING:
        outText << "<p class=\"description\">"
                << tr("The Hamming distances matrix is a "
                      "NxN matrix where each element (i,j) is the Hamming distance"
                      "of the tie profiles between actors i and j, namely the "
                      "number of different ties to other actors.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_HAMMING_DISTANCE, varLocation, false, true),
                             true, false, false);
        break;
    case MATRIX_DISTANCES_JACCARD:
        outText << "<p class=\"description\">"
                << tr("The Jaccard distances matrix is a "
                      "NxN matrix where each element (i,j) is the Jaccard distance"
                      "of the tie profiles between actors i and j.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_JACCARD_INDEX, "Rows", false, true),
                             true, false, false);

        break;
    case MATRIX_DISTANCES_MANHATTAN:
        outText << "<p class=\"description\">"
                << tr("The Manhattan distances matrix is a "
                      "NxN matrix where each element (i,j) is the Manhattan distance"
                      "of the tie profiles between actors i and j, namely  the "
                      "sum of their absolute differences.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_MANHATTAN_DISTANCE, varLocation, false, true),
                             true, false, false);
        break;
    case MATRIX_DISTANCES_CHEBYSHEV:
        outText << "<p class=\"description\">"
                << tr("The Chebyshev distances matrix is a "
                      "NxN matrix where each element (i,j) is the Chebyshev distance"
                      "of the tie profiles between actors i and j, namely the greatest of their differences.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_CHEBYSHEV_MAXIMUM, varLocation, false, true),
                             true, false, false);
        break;

    default:
        break;
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Matrix report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();
    return true;
}

/**
 * @brief Writes the matrix M as HTML <table> to specified text stream outText
 * It is the same as Matrix::printHTMLTable except that
 * this method omits disabled vertices, thus the table header is correct
 * @param outText
 * @param M
 * @param markDiag
 * @param plain
 * @param printInfinity
 */
void Graph::writeMatrixHTMLTable(QTextStream &outText,
                                 Matrix &M,
                                 const bool &markDiag,
                                 const bool &plain,
                                 const bool &printInfinity,
                                 const bool &dropIsolates)
{

    Q_UNUSED(plain);

    qDebug() << "Graph::writeMatrixHTMLTable() -"
             << "markDiag" << markDiag
             << "plain" << plain
             << " dropIsolates " << dropIsolates;

    int rowCount = 0, i = 0, j = 0;
    int N = vertices();
    qreal maxVal, minVal, element;
    bool hasRealNumbers = false;

    VList::const_iterator it, jt;

    QString pMsg = tr("Writing matrix to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    M.findMinMaxValues(minVal, maxVal, hasRealNumbers);

    outText << ((hasRealNumbers) ? qSetRealNumberPrecision(3) : qSetRealNumberPrecision(0));

    qDebug() << "Graph::writeMatrixHTMLTable() - minVal" << minVal
             << "maxVal" << maxVal << "hasRealNumbers" << hasRealNumbers;

    outText << "<table  border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"stripes\">"
            << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("<sub>Actor</sup>/<sup>Actor</sup>")
            << "</th>";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (!(*it)->isEnabled() || (dropIsolates && (*it)->isIsolated()))
        {
            continue;
        }
        outText << "<th>"
                << (*it)->number()
                << "</th>";
    }
    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (!(*it)->isEnabled() || (dropIsolates && (*it)->isIsolated()))
        {
            continue;
        }

        rowCount++;

        progressUpdate(rowCount);

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">";

        outText << "<td class=\"header\">"
                << (*it)->number()
                << "</td>";

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            if (!(*jt)->isEnabled() || (dropIsolates && (*jt)->isIsolated()))
            {
                continue;
            }
            outText << Qt::fixed << Qt::right;

            outText << "<td" << ((markDiag && (*it)->number() == (*jt)->number()) ? " class=\"diag\">" : ">");

            element = M.item(i, j);

            qDebug() << "Graph::writeMatrixHTMLTable() - M(" << i << "," << j << ") =" << M.item(i, j);

            if ((element == RAND_MAX) && printInfinity)
            {
                // print inf symbol instead of RAND_MAX (distances matrix).
                outText << infinity;
            }
            else
            {
                outText << element;
            }

            outText << "</td>";
            j++;
        }
        outText << "</tr>";
        i++;
        j = 0;
    }
    outText << "</tbody></table>";

    outText << qSetFieldWidth(0) << "\n";

    outText << "<p>"
            << "<span class=\"info\">"
            << ("Values: ")
            << "</span>"
            << ((hasRealNumbers) ? ("real numbers (printed decimals 3)") : ("integers only"))
            << "<br />"
            << "<span class=\"info\">"
            << ("- Max value: ")
            << "</span>"
            << ((maxVal == RAND_MAX) ? ((printInfinity) ? infinity : QString::number(maxVal)) +
                                           " (=not connected nodes, in distance matrix)"
                                     : QString::number(maxVal))
            << "<br />"
            << "<span class=\"info\">"
            << ("- Min value: ")
            << "</span>"
            << ((minVal == RAND_MAX) ? ((printInfinity) ? infinity : QString::number(minVal)) +
                                           +" (usually denotes unconnected nodes, in distance matrix)"
                                     : QString::number(minVal))
            << "</p>";

    progressFinish();
}

/**
    Exports the adjacency matrix to a given textstream
*/
void Graph::writeMatrixAdjacencyTo(QTextStream &os,
                                   const bool &saveEdgeWeights)
{
    qDebug("Graph: adjacencyMatrix(), writing matrix with %i vertices", vertices());
    VList::const_iterator it, it1;
    qreal weight = RAND_MAX;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {
            if (!(*it1)->isEnabled())
                continue;
            if ((weight = edgeExists((*it)->number(), (*it1)->number())) != 0)
            {
                // os << static_cast<int> (weight) << " ";
                os << ((saveEdgeWeights) ? weight : 1) << " ";
            }
            else
                os << "0 ";
        }
        os << "\n";
    }
}

/**
    Writes the adjacency matrix of G to a specified file fn
*/
void Graph::writeMatrixAdjacency(const QString fn,
                                 const bool &markDiag)
{

    qDebug() << "Writing adjacency matrix to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fn;
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }

    QTextStream outText(&file);

    int sum = 0;
    qreal weight = 0;
    int rowCount = 0;
    int N = vertices();

    VList::const_iterator it, it1;

    QString pMsg = tr("Writing Adjacency Matrix to file. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("ADJACENCY MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The adjacency matrix of a social network is a NxN matrix ")
            << tr("where each element (i,j) is the value of the edge from "
                  "actor i to actor j, or 0 if no edge exists.")
            << "<br />"
            << "</p>";

    outText << "<table  border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"stripes\">"
            << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("<sub>Actor</sup>/<sup>Actor</sup>")
            << "</th>";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        outText << "<th>"
                << (*it)->number()
                << "</th>";
    }
    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        rowCount++;

        progressUpdate(rowCount);

        if (!(*it)->isEnabled())
            continue;

        outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">";

        outText << "<td class=\"header\">"
                << (*it)->number()
                << "</td>";

        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {

            if (!(*it1)->isEnabled())
                continue;

            outText << "<td" << ((markDiag && (*it)->number() == (*it1)->number()) ? " class=\"diag\">" : ">");
            if ((weight = edgeExists((*it)->number(), (*it1)->number())) != 0)
            {
                sum++;
                outText << (weight);
            }
            else
            {
                outText << 0;
            }
            outText << "</td>";
        }
        outText << "</tr>";
    }
    outText << "</tbody></table>";

    qDebug("Graph: Found a total of %i edge", sum);
    if (sum != edgesEnabled())
        qDebug("Error in edge count found!!!");
    else
        qDebug("Edge count OK!");

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Adjacency matrix report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Writes a visual representation of the adjacency matrix of the graph to the specified file
 *
 * The resulting matrix HAS NO spaces between elements.
 *
 * @param fn
 * @param simpler
 */
void Graph::writeMatrixAdjacencyPlot(const QString fn,
                                     const bool &simpler)
{

    qDebug() << "Writing adjacency matrix plot to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fn;
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }
    QTextStream outText(&file);

    VList::const_iterator it, it1;
    int sum = 0;
    int rowCount = 0;
    int N = vertices();
    qreal weight = 0;
    int progressCounter = 0;
    QString pMsg = tr("Plotting Adjacency Matrix. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    if (!simpler)
    {
        outText << htmlHead;
    }
    else
        outText << htmlHeadLight;

    outText << "<h1>";
    outText << tr("ADJACENCY MATRIX PLOT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            << "</span>"
            << getName()
            << "<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            << "</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("This a plot of the network's adjacency matrix, a NxN matrix ")
            << tr("where each element (i,j) is filled if there is an edge from "
                  "actor i to actor j, or not filled if no edge exists.")
            << "<br />"
            << "</p>";

    if (!simpler)
    {

        outText << "<table class=\"plot\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">";

        outText << "<tbody>";

        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {

            progressUpdate(++progressCounter);

            if (!(*it)->isEnabled())
            {
                continue;
            }

            rowCount++;

            outText << "<tr class=" << ((rowCount % 2 == 0) ? "even" : "odd") << ">";

            for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
            {

                if (!(*it1)->isEnabled())
                    continue;

                if ((weight = edgeExists((*it)->number(), (*it1)->number())) != 0)
                {
                    sum++;
                    outText << "<td class=\"filled\">"
                            << QString("\xe2\x96\xa0")
                            << "</td>";
                }
                else
                {
                    outText << "<td>"
                            //   << "&nbsp;&nbsp;"
                            << QString("\xe2\x96\xa1")
                            << "</td>";
                }
            }
            outText << "</tr>";
        }
        outText << "</tbody></table>";
    }
    else
    {
        outText << "<p class=\"pre\">";
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {

            progressUpdate(++progressCounter);

            if (!(*it)->isEnabled())
            {
                continue;
            }

            rowCount++;

            for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
            {

                if (!(*it1)->isEnabled())
                    continue;

                if ((weight = edgeExists((*it)->number(), (*it1)->number())) != 0)
                {
                    sum++;
                    outText << QString("\xe2\x96\xa0") << " ";
                }
                else
                {
                    outText << QString("\xe2\x96\xa1") << " ";
                }
            }
            outText << "<br>" << "\n";
        }
        outText << "</p>";
    }
    qDebug("Graph: Found a total of %i edge", sum);
    if (sum != edgesEnabled())
        qDebug("Error in edge count found!!!");
    else
        qDebug("Edge count OK!");

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Adjacency matrix plot, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
                   .arg(VERSION)
                   .arg(actualDateTime.currentDateTime().toString(QString("ddd, dd.MMM.yyyy hh:mm:ss")));
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg(computationTimer.elapsed());
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    progressFinish();
}

/**
 * @brief Computes the Degree matrix of the graph and writes it to given file
 * @param fn
 */
void Graph::writeMatrixDegreeText(const QString &fn)
{

    qDebug() << "Writing degree matrix to file:" << fn;

    createMatrixAdjacency();

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fn;
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }
    QTextStream outText(&file);

    outText << AM.degreeMatrix();

    file.close();
}

/**
 * @brief Computes the Laplacian matrix of the graph and writes it to given file
 * @param fn
 */
void Graph::writeMatrixLaplacianPlainText(const QString &fn)
{

    qDebug() << "Writing Laplacian matrix to file:" << fn;

    createMatrixAdjacency();

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fn;
        progressStatus(tr("Error. Could not write to ") + fn);
        return;
    }
    QTextStream outText(&file);

    outText << AM.laplacianMatrix();

    file.close();
}
