/**
 * @file graph.cpp
 * @brief Implements the Graph class for managing network structures, nodes, and edges in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 * @website http://dimitris.apeiro.gr
 */

#include "graph.h"

#include <QtGlobal>
#include <QFile>
#include <QDir>
#include <QtMath>
#include <QPointF>
#include <QDebug>
#include <QHash>
#include <QColor>
#include <QFileInfo>
#include <QDateTime> // used in exporting centrality files

#include <QPixmap>
#include <QElapsedTimer>
#include <QStringEncoder>

#include <cstdlib> //allows the use of RAND_MAX macro

#include <queue> //for BFS queue Q
#include <ctime> // for randomizeThings

#include "engine/distance_engine.h"
#include "engine/graph_distance_progress_sink.h"

/**
 * @brief Constructs a Graph
 */
Graph::Graph(const int &reserveVerticesSize, const int &reserveEdgesPerVertexSize)
{

    qRegisterMetaType<MyEdge>("MyEdge");

    qRegisterMetaType<NetworkRequestType>("NetworkRequestType");

    m_totalVertices = 0;
    m_totalEdges = 0;

    // We do init these two vars here, because they only get their values
    // on MW::resizeEvent which might happen after we have started creating
    // nodes.
    // For instance, this happens when we load a network from command line...
    canvasWidth = 700;
    canvasHeight = 600;

    order = true; // returns true if the indexes of the list is ordered.

    // Use the given vertices estimate to allocate memory
    // to prevent reallocations and memory fragmentation.
    if (reserveVerticesSize > 0)
    {
        qDebug() << "Graph reserving this vertices estimate:" << reserveVerticesSize;
        m_graph.reserve(reserveVerticesSize);
    }
    // Store the
    m_reserveEdgesPerVertexSize = reserveEdgesPerVertexSize;

    m_graphModStatus = ModStatus::NewNet;
    m_graphName = "";
    m_curRelation = 0;
    m_fileFormat = FileType::NOT_SAVED;

    m_graphIsDirected = true;
    m_graphIsWeighted = false;
    m_graphIsConnected = true; // empty/null graph is considered connected
    m_graphIsSymmetric = true;

    m_graphDensity = -1;
    m_fileName = "";

    calculatedGraphReciprocity = false;
    calculatedGraphSymmetry = false;
    calculatedGraphWeighted = false;
    calculatedGraphDensity = false;
    calculatedEdges = false;
    calculatedVertices = false;
    calculatedVerticesList = false;
    calculatedVerticesSet = false;
    calculatedAdjacencyMatrix = false;

    calculatedDistances = false;
    calculatedIsolates = false;
    calculatedDP = false;
    calculatedDC = false;
    calculatedIC = false;
    calculatedEVC = false;
    calculatedCentralities = false;
    calculatedIRCC = false;
    calculatedPP = false;
    calculatedPRP = false;
    calculatedTriad = false;

    m_reportsDataDir = "";
    m_reportsRealPrecision = 6;
    m_reportsLabelLength = 8;
    m_reportsChartType = ChartType::Spline;

    m_vertexClicked = 0;
    m_clickedEdge.source = 0;
    m_clickedEdge.target = 0;

    file_parser = 0;

    web_crawler = 0;

    m_graphFileFormatExportSupported << FileType::GRAPHML
                                     << FileType::PAJEK
                                     << FileType::ADJACENCY;

    randomizeThings();

    htmlHead = QString("<!DOCTYPE html>"
                       "<html>"
                       "<head>"
                       "<meta name=\"qrichtext\" content=\"1\" />"
                       "<meta charset=\"utf-8\" />"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />"
                       "<meta name=\"generator\" content=\"SocNetV v%1\" />"
                       "<meta name=\"keywords\" content=\"Social Network Visualizer, SocNetV, report\" />"
                       "<meta name=\"description\" content=\"Social Network Visualizer (SocNetV) report\" />"
                       "<style type=\"text/css\">"
                       "body {font-family:'monospace'; font-size:12px; font-weight:400; font-style:normal;}"
                       "body.waiting * { cursor: progress; }"
                       "p, li { white-space: normal; }"
                       "p {margin:10px 0;-qt-block-indent:0; text-indent:0px;}"
                       "table {margin: 20px 5px; white-space: nowrap; border-spacing: 0px; "
                       "border-collapse: separate;font-size: 10px;}"
                       "table tr {white-space: normal;}"
                       "table th {cursor:pointer;text-align:center;font-weight: bold;"
                       "background: #000; color: #fff; vertical-align: bottom; font-size:12px; padding: 3px 6px;}"
                       "table td {text-align:center; padding: 0.2em 1em;}"
                       "span.header, table td.header {background:#000; color:#fff; font-size:12px; padding: 3px 6px;}"
                       "table td.diag {background:#aaa;}"
                       "table.stripes th {}"
                       "table.sortable th::after {content: \"\\2195\"; font-size: 16px;color: #777;}"
                       "table.sortable th.desc::after {content: \"\\2193\"; color: #fff;}"
                       "table.sortable th.asc::after {content: \"\\2191\"; color: #fff;}"
                       "table.stripes tr.odd  { background: #ddd;}"
                       "table.stripes tr:odd  { background: #ddd;}"
                       "table.stripes tr.even { background: #fff;}"
                       "table.stripes tr:even { background: #fff;}"
                       "table.plot {}"
                       "table.plot th {}"
                       "table.plot td {text-align: center; padding: 0px 3px;"
                       "border-collapse: collapse; border-spacing: 0; }"
                       "table.plot td.filled {background: #000;}"
                       ".pre {margin-top:0px; margin-bottom:0px;font-size:1px; line-height: 100%; white-space: nowrap; }"
                       ".description {font-style: italic;color: #666;max-width: 100%;}"
                       ".info {font-weight: bold;color: #333;}"
                       ".small {font-style: italic;color: #333; font-size: 90%;}"
                       ".dendrogram .row { clear:both; height: 16px; margin: 2px 0px; overflow:hidden; }"
                       ".dendrogram .cluster-levels {float:left; min-width: 3%; text-align:right;}"

                       "</style>"
                       "<script type=\"text/javascript\">\n"
                       "var mytable, asc1=1, asc2=1,asc3=1,asc4=1;asc5=1;asc6=1;\n"
                       "window.onload = function () {\n"
                       "mytable = document.getElementById(\"results\");\n"
                       "}\n"
                       "function tableSort(tbody, col, asc) {\n"
                       " document.getElementById(\"socnetv-report\").classList.toggle('waiting'); \n"
                       " var rows = tbody.rows, \n"
                       " rlen = rows.length, \n"
                       " arr = new Array(),\n"
                       " i, j, cells, clen;\n"
                       " clen = rows[0].cells.length;\n"
                       "  for (j = 0; j < clen; j++) {\n"
                       "    document.getElementById(\"col\"+(j+1).toString()).classList.remove('desc'); \n"
                       "    document.getElementById(\"col\"+(j+1).toString()).classList.remove('asc'); \n"
                       "   if ( j == col ) {\n"
                       "    if (  asc > 0 ) { document.getElementById(\"col\"+(j+1).toString()).classList.add('asc'); }"
                       "    else { document.getElementById(\"col\"+(j+1).toString()).classList.add('desc'); }"
                       "   }"

                       "  }\n"
                       " // fill the array with values from the table\n"
                       " for (i = 0; i < rlen; i++) {\n"
                       "  cells = rows[i].cells;\n"
                       "  clen = cells.length;\n"
                       "  arr[i] = new Array();\n"
                       "  for (j = 0; j < clen; j++) {\n"
                       "   arr[i][j] = cells[j].innerHTML; \n"
                       "  }\n"
                       " }\n"
                       " // sort the array by the specified column (col) and order (asc)\n"
                       " arr.sort(function (a, b) {\n"
                       "  var retval=0;\n"
                       "  var fA=parseFloat(a[col]);\n"
                       "  var fB=parseFloat(b[col]);\n"
                       "  if(a[col] != b[col]) {\n"
                       "    if((fA==a[col]) && (fB==b[col]) ){ retval=( fA > fB ) ? asc : -1*asc; } //numerical\n"
                       "    else { retval = (a[col] > b[col]) ? asc : -1 * asc; }\n"
                       "   }"
                       "   return retval; \n"
                       " });\n"
                       " // replace existing rows with new rows created from the sorted array\n"
                       " for (i = 0; i < rlen; i++) {\n"
                       "  rows[i].innerHTML = \"<td>\" + arr[i].join(\"</td><td>\") + \"</td>\";\n"
                       "  }\n"
                       " document.getElementById(\"socnetv-report\").classList.toggle('waiting'); \n"
                       "}\n"
                       "</script>"
                       "</head>"
                       "<body id=\"socnetv-report\">")
                   .arg(VERSION);

    htmlHeadLight = QString("<!DOCTYPE html>"
                            "<html>"
                            "<head>"
                            "<meta name=\"qrichtext\" content=\"1\" />"
                            "<meta charset=\"utf-8\" />"
                            "<meta name=\"generator\" content=\"SocNetV v%1\" />"
                            "<meta name=\"keywords\" content=\"Social Network Visualizer, SocNetV, report\" />"
                            "<meta name=\"description\" content=\"Social Network Visualizer (SocNetV) report\" />"
                            "<style type=\"text/css\">"
                            "body { font-size:12px;white-space: nowrap; }"
                            "p, li { white-space: normal; }"
                            "p {margin:10px 0;-qt-block-indent:0; text-indent:0px;}"
                            ".pre {margin:0px; font-size:1px; line-height: 100%; white-space: nowrap; }"
                            ".description {font-style: italic;color: #666;}"
                            ".info {font-weight: bold;color: #333;}"
                            ".small {font-style: italic;color: #333; font-size: 90%;}"
                            "</style>"
                            "</head>"
                            "<body>")
                        .arg(VERSION);

    htmlEnd = "</body></html>";
}

/**
 * @brief Destroys the Graph object
 */
Graph::~Graph()
{
    qDebug() << "Graph destructing (because app exit?)...Calling clear()";
    this->clear("exit");
    delete file_parser;
}

/**
 * @brief Clears all vertices
 * @param reason
 */
void Graph::clear(const QString &reason)
{
    qDebug() << "Clearing graph, vertices and data structures... Reason:" << reason;

    qDebug() << "Asking parser and crawler threads to terminate...";

    graphLoadedTerminateParserThreads("clear");
    webCrawlTerminateThreads("clear");

    qDeleteAll(m_graph.begin(), m_graph.end());
    m_graph.clear();
    vpos.clear();

    discreteDPs.clear();
    discreteSDCs.clear();
    discreteCCs.clear();
    discreteBCs.clear();
    discreteSCs.clear();
    discreteIRCCs.clear();
    discreteECs.clear();
    discreteEccentricities.clear();
    discretePCs.clear();
    discreteICs.clear();
    discretePRPs.clear();
    discretePPs.clear();
    discreteEVCs.clear();

    if (DM.size() > 0)
    {
        qDebug() << "clearing DM matrix";
        DM.clear();
    }
    if (SIGMA.size() > 0)
    {
        qDebug() << "clearing SIGMA matrix";
        SIGMA.clear();
    }
    if (sumM.size() > 0)
    {
        qDebug() << "clearing sumM";
        sumM.clear();
    }
    if (invAM.size() > 0)
    {
        qDebug() << "clearing invAM";
        invAM.clear();
    }
    if (AM.size() > 0)
    {
        qDebug() << "clearing AM";
        AM.clear();
    }
    if (invM.size() > 0)
    {
        qDebug() << "clearing invM";
        invM.clear();
    }
    if (XM.size() > 0)
    {
        qDebug() << "clearing XM";
        XM.clear();
    }
    if (XSM.size() > 0)
    {
        qDebug() << "clearing XSM";
        XSM.clear();
    }
    if (XRM.size() > 0)
    {
        qDebug() << "clearing XRM";
        XRM.clear();
    }

    m_verticesList.clear();
    m_verticesSet.clear();

    m_verticesIsolatedList.clear();
    m_vertexPairsNotConnected.clear();
    m_vertexPairsUnilaterallyConnected.clear();
    influenceDomains.clear();
    influenceRanges.clear();
    triadTypeFreqs.clear();

    // clear relations
    relationsClear();

    // add a default relation, only if we are not closing
    if (reason != "exit")
    {
        relationAdd(tr(("unnamed")));
    }

    m_fileFormat = FileType::NOT_SAVED;
    m_graphName = "";
    m_fileName = "";

    m_totalVertices = 0;
    m_totalEdges = 0;

    outboundEdgesVert = 0;
    inboundEdgesVert = 0;
    reciprocalEdgesVert = 0;

    m_vertexClicked = 0;
    m_clickedEdge.source = 0;
    m_clickedEdge.target = 0;

    order = true; // returns true if the vpositions of the list is ordered.

    m_graphIsDirected = true;
    m_graphIsWeighted = false;
    m_graphIsConnected = true; // empty/null graph is considered connected.
    m_graphIsSymmetric = true;

    m_graphDensity = -1;
    m_graphDiameter = 0;

    m_graphAverageDistance = 0;
    m_graphSumDistance = 0;

    m_graphGeodesicsCount = 0; // non zero distances

    calculatedGraphReciprocity = false;
    calculatedGraphSymmetry = false;
    calculatedGraphWeighted = false;
    calculatedGraphDensity = false;
    calculatedEdges = false;
    calculatedVertices = false;
    calculatedVerticesList = false;
    calculatedVerticesSet = false;
    calculatedAdjacencyMatrix = false;

    calculatedDistances = false;
    calculatedIsolates = false;

    calculatedCentralities = false;
    calculatedDP = false;
    calculatedDC = false;
    calculatedIC = false;
    calculatedEVC = false;
    calculatedIRCC = false;
    calculatedPP = false;
    calculatedPRP = false;
    calculatedTriad = false;

    m_graphModStatus = ModStatus::NewNet;

    //    if ( urlQueue->size() > 0 ) {
    //        urlQueue->clear();
    //    }

    if (reason != "exit")
    {
        qDebug() << "Finished clearing graph data. Changing graph modification status to" << m_graphModStatus;
        setModStatus(m_graphModStatus, true);
    }
    qDebug() << "Finished clearing graph data and structures.";
}



/**
 * @brief Checks a) if edge exists and b) if the reverse edge exists
 * Calls edgeAdd to add the new edge to the Graph,
 * then emits drawEdge() which calls GW::drawEdge() to draw the new edge.
 * Called from homonymous signal of Parser class.
 * Also called from MW when user clicks on the "add link" button
 * Also called (via MW) from GW when user middle-clicks on two nodes.
 * @param v1
 * @param v2
 * @param weight
 * @param color
 * @param reciprocal
 * @param drawArrows
 * @param bezier
 */
bool Graph::edgeCreate(const int &v1,
                       const int &v2,
                       const qreal &weight,
                       const QString &color,
                       const int &type,
                       const bool &drawArrows,
                       const bool &bezier,
                       const QString &label,
                       const bool &signalMW)
{

    // check whether there is already such an edge
    // (see #713617 - https://bugs.launchpad.net/socnetv/+bug/713617)

    if (edgeExists(v1, v2))
    {
        //        qDebug() << "-- Edge " << v1 << "->" << v2
        //                    << " declared previously (already exists) - nothing to do \n\n";

        return false;
    }

    if (type == EdgeType::Undirected)
    {

        //        qDebug() <<"-- Creating new UNDIRECTED edge:" << v1 << "-" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Signaling to GW...";

        edgeAdd(v1, v2,
                weight,
                type,
                label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label, ((weight == 0) ? "blue" : color), type,
                            drawArrows, bezier, initEdgeWeightNumbers);
    }
    else if (edgeExists(v2, v1))
    {

        //        qDebug() <<"-- Creating new RECIPROCAL edge:" << v1 << "->" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Reverse edge exists"
        //                 << "Signaling to GW...";

        edgeAdd(v1,
                v2,
                weight,
                EdgeType::Reciprocated,
                label,
                color);

        emit signalDrawEdge(v1, v2, weight, label, color, EdgeType::Reciprocated,
                            drawArrows, bezier, initEdgeWeightNumbers);
        m_graphIsDirected = true;
    }
    else
    {

        //        qDebug() <<"-- Creating new DIRECTED edge:" << v1 << "->" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Signaling to GW...";

        edgeAdd(v1,
                v2,
                weight,
                EdgeType::Directed,
                label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label, ((weight == 0) ? "blue" : color), EdgeType::Directed,
                            drawArrows, bezier, initEdgeWeightNumbers);

        m_graphIsDirected = true;
        m_graphIsSymmetric = false;
    }

    // save the edge color so that new edges created when user clicks on the canvas
    // have the same color with those of the file loaded,
    initEdgeColor = color;

    setModStatus(ModStatus::EdgeCount, signalMW);

    return true;
}

/**
 * @brief Called from WebCrawler when it finds an new link
 * Calls edgeCreate() method with initEdgeColor
 * @param source
 * @param target
 */
void Graph::edgeCreateWebCrawler(const int &source, const int &target)
{
    //    qDebug()<< " will create edge from" << source << "to" << target ;
    qreal weight = 1.0;
    bool drawArrows = true;
    bool bezier = false;

    edgeCreate(source, target, weight, initEdgeColor, EdgeType::Directed, drawArrows, bezier);
}

/**
 * @brief Adds a directed edge from v1 to v2
 * If type == EdgeType::Undirected then it also adds the directed edge v2->v1
 * @param v1
 * @param v2
 * @param weight
 * @param label
 * @param color
 * @param type
 */
void Graph::edgeAdd(const int &v1, const int &v2,
                    const qreal &weight,
                    const int &type,
                    const QString &label,
                    const QString &color)
{

    int source = vpos[v1];
    int target = vpos[v2];

    //    qDebug()<< "Adding new edge from vertex "<< v1 << "["<< source
    //            << "] to vertex "<< v2 << "["<< target << "] of weight "<<weight
    //            << " and label " << label;

    m_graph[source]->addOutEdge(v2, weight, color, label);
    m_graph[target]->addInEdge(v1, weight);

    if (weight != 1 && weight != 0)
    {
        setWeighted(true);
    }
    if (type == EdgeType::Reciprocated)
    {
        // make existing reverse edge reciprocal
    }
    else if (type == EdgeType::Undirected)
    {
        // edge undirected, add reverse edge too.
        m_graph[target]->addOutEdge(v1, weight);
        m_graph[source]->addInEdge(v2, weight);
    }
}

/**
 * @brief Toggles the status of outbound edge source -> target at source vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeOutboundStatusSet(const int &source, const int &target, const bool &toggle)
{

    m_graph[vpos[source]]->setOutEdgeEnabled(target, toggle);
}

/**
 * @brief Toggles the status of inbound edge target <- source at target vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeInboundStatusSet(const int &target, const int &source, const bool &toggle)
{

    m_graph[vpos[target]]->setInEdgeEnabled(source, toggle);
}

/**
 * @brief Removes the directed arc v1->v2 or, if the graph is undirected, the edge v1 <->v2
 *
 * Emits signal to GW to delete the graphics item.
 *
 * @param v1
 * @param v2
 * @param removeReverse if true also removes the reverse edge
 */
void Graph::edgeRemove(const int &v1,
                       const int &v2,
                       const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemove() - edge" << v1 << "[" << vpos[v1]
             << "] -->" << v2 << " to be removed. removeReverse:" << removeReverse;
    m_graph[vpos[v1]]->removeOutEdge(v2);
    m_graph[vpos[v2]]->removeInEdge(v1);

    if (isUndirected() || removeReverse)
    { // remove reverse edge too
        m_graph[vpos[v2]]->removeOutEdge(v1);
        m_graph[vpos[v1]]->removeInEdge(v2);
        m_graphIsSymmetric = true;
    }
    else
    {
        if (edgeExists(v2, v1) != 0)
        {
            m_graphIsSymmetric = false;
        }
    }

    emit signalRemoveEdge(v1, v2, (isDirected() || removeReverse));

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Removes a SelectedEdge
 * @param selectedEdge
 * @param removeReverse
 */
void Graph::edgeRemoveSelected(SelectedEdge &selectedEdge,
                               const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemoveSelected()" << selectedEdge;
    edgeRemove(selectedEdge.first, selectedEdge.second, removeReverse);
}

/**
 * @brief Removes all selected edges
 */
void Graph::edgeRemoveSelectedAll()
{
    qDebug() << "Graph::edgeRemoveSelectedAll()";

    foreach (SelectedEdge edgeToRemove, getSelectedEdges())
    {
        qDebug() << "Graph::edgeRemoveSelectedAll() - About to remove" << edgeToRemove;
        edgeRemoveSelected(edgeToRemove, true);
    }
}

/**
 * @brief Filters (disables) edges according the specified threshold weight
 *
 *
 * @param m_threshold
 * @param overThreshold
 */
void Graph::edgeFilterByWeight(const qreal m_threshold, const bool overThreshold)
{
    QString words;

    if (overThreshold)
    {
        qDebug() << "filtering edges with weight over or equal" << m_threshold;
        words = "equal or over";
    }
    else
    {
        qDebug() << "Filtering edges with weight below or equal" << m_threshold;
        words = "equal or under";
    }

    VList::const_iterator it;

    int source, target = 0;
    qreal weight = 0, reverseEdgeWeight = 0;
    bool preserveReverseEdge = false;
    H_edges::const_iterator ed;

    // Loop over all vertices
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        source = (*it)->number();

        // Loop over all out edges of source
        for (ed = (*it)->m_outEdges.cbegin(); ed != (*it)->m_outEdges.cend(); ++ed)
        {

            // Init preserve reserve edge status to false
            preserveReverseEdge = false;

            if (ed.value().first != m_curRelation)
            {
                // This edge does not belong to this relation
                continue;
            }

            target = ed.key();
            weight = ed.value().second.first;

            // Check the filtering type: over or under
            if (overThreshold)
            {
                // We should enable only edges with weight >= threshold
                if (weight < m_threshold)
                {
                    // this outedge must be disabled - check reverse edge
                    reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                    if (reverseEdgeWeight != 0 && reverseEdgeWeight >= m_threshold)
                    {
                        // reverse edge exists and doesn't match. It must be preserved.
                        preserveReverseEdge = true;
                    }
                    //                    qDebug() << source << "->" << target << "weight:" << weight << "will be disabled - preserveReverseEdge:" << preserveReverseEdge << ". Emitting signal...";
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false));
                    // Disable the inedge of the target vertex too (needed for inDegree)
                    //                    qDebug() << "disabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, false, preserveReverseEdge, weight, reverseEdgeWeight);
                }
                else
                {
                    //                    qDebug() << source << "->" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true));
                    // Enable the inedge of the target vertex too (needed for inDegree)
                    //                    qDebug() << "enabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, true);
                    emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverseEdge);
                }
            }
            else
            {
                // We should enable edges with weight <= the threshold
                if (weight > m_threshold)
                {
                    // this outedge must be disabled - check reverse edge
                    reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                    if (reverseEdgeWeight != 0 && reverseEdgeWeight <= m_threshold)
                    {
                        // reverse edge exists and doesn't match. It must be preserved.
                        preserveReverseEdge = true;
                    }
                    //                    qDebug() << source << "->" << target << "weight:" << weight << "will be disabled - preserveReverseEdge:" << preserveReverseEdge << ". Emitting signal...";
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false));
                    // Disable the inedge of the target vertex too (needed for inDegree)
                    //                    qDebug() << "disabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, false, preserveReverseEdge, weight, reverseEdgeWeight);
                }
                else
                {
                    //                    qDebug() << source << "->" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true));
                    // Enable the inedge of the target vertex too (needed for inDegree)
                    //                    qDebug() << "enabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, true);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverseEdge);
                }
            }
        }
    }
    // Update graph mod status
    setModStatus(ModStatus::EdgeCount);
    // Emit a status message
    emit statusMessage(tr("Edges with weight %1 %2 have been filtered.").arg(words).arg(m_threshold));
}

/**
 * @brief Toggles (enables or disables) all edges of the given relation
 *
 * Calls the homonymous method of GraphVertex class.
 *
 * @param relation
 * @param status
 */
void Graph::edgeFilterByRelation(int relation, bool status)
{
    qDebug() << "toggling all edges in relation" << relation << "to status" << status;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
        {
            // Skip if the node is disabled.
            continue;
        }
        (*it)->setEnabledEdgesByRelation(relation, status);
    }
}

/**
 * @brief Enables or disables unilateral edges in current relationship.
 *
 * If toggle=true, all non-reciprocal edges are disabled, effectively making
 * the network symmetric.
 *
 * @param toggle
 */
void Graph::edgeFilterUnilateral(const bool &toggle)
{
    qDebug() << "Toggling unilateral edges:" << toggle;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        (*it)->setEnabledUnilateralEdges(toggle);
    }
    setModStatus(ModStatus::EdgeCount);
    emit statusMessage(tr("Unilateral edges have been temporarily disabled."));
}

/**
 * @brief Checks if there is an edge from v1 to v2 and returns the weight, if the edge exists.
 *
   Complexity:  O(logN) for vpos retrieval + O(1) for QList index retrieval + O(logN) for checking edge(v2)

 * @param v1
 * @param v2
 * @param reciprocated: if true, checks if the edge is reciprocated (v1<->v2) with the same weight
 * @return zero if edge or reciprocated edge does not exist or non-zero if arc /reciprocated edge exists
 */
qreal Graph::edgeExists(const int &v1, const int &v2, const bool &checkReciprocal)
{

    edgeWeightTemp = m_graph[vpos[v1]]->hasEdgeTo(v2);
    //    qDebug() << "Checking if edge exists:" << v1 << "->" << v2 << "=" << edgeWeightTemp  ;

    if (!checkReciprocal)
    {
        return edgeWeightTemp;
    }
    else if (edgeWeightTemp != 0)
    {
        edgeReverseWeightTemp = m_graph[vpos[v2]]->hasEdgeTo(v1);
        //        qDebug() << "Checking if reverse edge exists: " << v2 << "->" << v1 << "=" << edgeWeightTemp  ;
        if (edgeWeightTemp == edgeReverseWeightTemp)
        {
            return edgeWeightTemp;
        }
    }
    return 0;
}

/**
 * @brief Checks if there is an edge from v1 to v2, even weight = 0 and returns the weight, if the edge exists
 * or RAND_MAX if the edge does not exist at all.
 *
 * This is only used in GraphML saving if the user has selected the Settings option to save zero-weight edges
 *
 * @see https://github.com/socnetv/app/issues/151
 *
 * @param v1
 * @param v2
 */
qreal Graph::edgeExistsVirtual(const int &v1, const int &v2)
{

    qreal m_weight = RAND_MAX;
    bool edgeStatus = false;
    H_edges::const_iterator it1;
    GraphVertex *source = m_graph[vpos[v1]];
    H_edges source_outEdges = source->m_outEdges;

    it1 = source_outEdges.constFind(v2);
    while (it1 != source_outEdges.constEnd() && it1.key() == v2)
    {
        if (it1.value().first == m_curRelation)
        {
            edgeStatus = it1.value().second.second;
            if (edgeStatus == true)
            {
                m_weight = it1.value().second.first;
            }
        }
        ++it1;
    }

    return m_weight;
}

/**
 * @brief Returns TRUE if edge(v1, v2) is symmetric, i.e. (v1,v2) == (v2,v1).
 * @param v1
 * @param v2
 * @return
 */
bool Graph::edgeSymmetric(const int &v1, const int &v2)
{
    if ((edgeExists(v1, v2, true)) != 0)
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is symmetric";
        return true;
    }
    else
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is not symmetric";
        return false;
    }
}

/**
 * @brief Returns the number |E| of graph - only the enabled edges
 *
 * @return
 */
int Graph::edgesEnabled()
{

    int enabledEdges = 0;
    if (calculatedEdges)
    {
        enabledEdges = ((isUndirected()) ? m_totalEdges / 2 : m_totalEdges);
        //        qDebug()<< "Graph unchanged. Returning current enabled edges count:" <<  enabledEdges;
        return enabledEdges;
    }
    // Compute the edge count from scratch
    m_totalEdges = 0;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        m_totalEdges += (*it)->outEdgesCount();
    }
    calculatedEdges = true;
    enabledEdges = ((isUndirected()) ? m_totalEdges / 2 : m_totalEdges);
    //    qDebug()<< "Computed enabled edges new count:" <<  enabledEdges;
    return enabledEdges;
}

/**
 * @brief Returns the number of outbound edges (arcs) from vertex v1
 * @param v1
 * @return
 */
int Graph::vertexEdgesOutbound(int v1)
{
    qDebug("Graph: vertexEdgesOutbound()");
    return m_graph[vpos[v1]]->outEdgesCount();
}

/**
 * @brief Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::vertexEdgesInbound(int v1)
{
    qDebug("Graph: vertexEdgesInbound()");
    return m_graph[vpos[v1]]->inEdgesCount();
}

/**
 * @brief Changes the weight of the edge from vertex v1 to v2 (and optionally of the reverse edge)
 *
 * @param v1
 * @param v2
 * @param weight
 * @param undirected
 */
void Graph::edgeWeightSet(const int &v1, const int &v2,
                          const qreal &weight, const bool &undirected)
{
    qDebug() << "Changing the weight of edge" << v1 << "[" << vpos[v1]
             << "]->" << v2 << "[" << vpos[v2] << "]" << " to new weight " << weight;
    m_graph[vpos[v1]]->setOutEdgeWeight(v2, weight);
    if (undirected)
    {
        qDebug() << "Changing the weight of the reverse edge too";
        m_graph[vpos[v2]]->setOutEdgeWeight(v1, weight);
    }
    emit setEdgeWeight(v1, v2, weight);
    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Returns the weight of the edge v1->v2
 * @param v1
 * @param v2
 * @return qreal
 */
qreal Graph::edgeWeight(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->hasEdgeTo(v2);
}

/**
 * @brief Changes the visibility of edge weight numbers
 * @param toggle
 */
void Graph::edgeWeightNumbersVisibilitySet(const bool &toggle)
{
    initEdgeWeightNumbers = toggle;
}

/**
 * @brief Saves the default edge color
 * Used by random network creation methods
 * @param color
 */
void Graph::edgeColorInit(const QString &color)
{
    initEdgeColor = color;
}

/**
 * @brief Changes the color of all enabled edges.
 * @param color
 * @return
 */
bool Graph::edgeColorAllSet(const QString &color, const int &threshold)
{
    qDebug() << "Graph::edgeColorAllSet() - new color: " << color;
    int target = 0, source = 0;
    edgeColorInit(color);
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        source = (*it)->number();
        if (!(*it)->isEnabled())
            continue;
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            target = it1.key();
            if (threshold == 0)
            {
                if (it1.value() == threshold)
                {
                    qDebug() << " Graph::edgeColorAllSet() zero weight threshold "
                             << threshold << " - edge "
                             << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else if (threshold != 0 && threshold != RAND_MAX)
            {
                if (it1.value() <= threshold)
                {
                    qDebug() << " Graph::edgeColorAllSet() below weight threshold "
                             << threshold << " - edge "
                             << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else
            {
                qDebug() << " Graph::edgeColorAllSet() : "
                         << source << "->" << target << " new color " << color;
                (*it)->setOutLinkColor(target, color);
                emit setEdgeColor(source, target, color);
            }
            ++it1;
        }
    }
    // delete enabledOutEdges;

    setModStatus(ModStatus::EdgeMetadata);

    return true;
}

/**
 * @brief Changes the color of edge v1->v2
 * @param v1
 * @param v2
 * @param color
 */
void Graph::edgeColorSet(const int &v1, const int &v2, const QString &color)
{
    qDebug() << "Graph::edgeColorSet() - " << v1 << "->" << v2
             << " vpos (" << vpos[v1] << "->" << vpos[v2] << ")"
             << " new color " << color;
    m_graph[vpos[v1]]->setOutLinkColor(v2, color);
    emit setEdgeColor(v1, v2, color);
    if (isSymmetric())
    {
        m_graph[vpos[v2]]->setOutLinkColor(v1, color);
        emit setEdgeColor(v2, v1, color);
    }

    setModStatus(ModStatus::EdgeMetadata);
}

/**
 * @brief Returns the color of the directed edge v1->v2
 * @param v1
 * @param v2
 * @return
 */
QString Graph::edgeColor(const int &v1, const int &v2)
{
    return m_graph[vpos[v1]]->outLinkColor(v2);
}

/**
 * @brief Changes the label of edge v1->v2
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeLabelSet(const int &v1, const int &v2, const QString &label)
{
    qDebug() << "Graph::edgeLabelSet()  " << v1 << "[" << vpos[v1]
             << "]->" << v2 << "[" << vpos[v2] << "]" << " label " << label;
    m_graph[vpos[v1]]->setOutEdgeLabel(v2, label);

    emit setEdgeLabel(v1, v2, label);

    setModStatus(ModStatus::EdgeMetadata);
}

/**
 * @brief Returns the label of edge v1->v2
 * @param v1
 * @param v2
 * @return
 */
QString Graph::edgeLabel(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->outEdgeLabel(v2);
}

/**
 * @brief Toggles the visibility of edge labels.
 * @param toggle
 */
void Graph::edgeLabelsVisibilitySet(const bool &toggle)
{
    initEdgeLabels = toggle;
}

/**
 * @brief Returns the outDegree (sum of outbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeOut(int v1)
{
    qDebug() << "Returning outDegree of " << v1;
    return m_graph[vpos[v1]]->degreeOut();
}

/**
 * @brief Returns the inDegree (sum of inbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeIn(int v1)
{
    qDebug() << "Returning inDegree of " << v1;
    return m_graph[vpos[v1]]->degreeIn();
}

/**
 * @brief Returns a list of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QList<int> Graph::vertexNeighborhoodList(const int &v1)
{
    // qDebug() << "Returning the neighborhood list of " << v1;
    return m_graph[vpos[v1]]->neighborhoodList();
}

/**
 * @brief Returns the set of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QSet<int> Graph::vertexNeighborhoodSet(const int &v1)
{
    // qDebug()<< "Graph::vertexNeighborhoodList()";
    QList<int> myNeightbors = m_graph[vpos[v1]]->neighborhoodList();
    return QSet<int>(myNeightbors.constBegin(), myNeightbors.constEnd());
}

/**
 * @brief Gets the number of vertices in the graph
 *
 * If countAll = true, returns |V| where V the set of all (enabled or not) vertices
 * If countAll = false, it skips disabled vertices
 * If countAll = false and dropIsolates = true, it skips both disabled and isolated vertices
 *
 * @param dropIsolates
 * @param countAll
 * @return
 */
int Graph::vertices(const bool &dropIsolates, const bool &countAll, const bool &recount)
{

    if (m_totalVertices != 0 && calculatedVertices && !recount)
    {
        qDebug() << "Graph not modified, returning static number: "
                 << m_totalVertices;
        return m_totalVertices;
    }
    m_totalVertices = 0;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (countAll)
        {
            ++m_totalVertices;
        }
        else
        {
            if (dropIsolates && (*it)->isIsolated())
            {
                qDebug() << "Skipping isolated vertex:" << (*it)->number();
                continue;
            }
            if (!(*it)->isEnabled())
            {
                qDebug() << "Skipping disabled vertex:" << (*it)->number();
                continue;
            }
            ++m_totalVertices;
        }
    }
    qDebug() << "Graph size:" << m_graph.size() << "vertices" << m_totalVertices;
    calculatedVertices = true;
    return m_totalVertices;
}

/**
 * @brief Returns a list of all isolated vertices inside the graph
 *
 * @return QList<int>
 */
QList<int> Graph::verticesListIsolated()
{
    if (calculatedIsolates)
    {
        qDebug() << "Graph::verticesListIsolated() - graph not modified and "
                    "already calculated isolates. Returning list as is:"
                 << m_verticesIsolatedList;
        return m_verticesIsolatedList;
    }

    VList::const_iterator it;
    m_verticesIsolatedList.clear();
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        //        if ( ! (*it)->isEnabled() )
        //            continue;
        if ((*it)->isIsolated())
        {
            m_verticesIsolatedList << (*it)->number();
            qDebug() << "Graph::verticesListIsolated() - node " << (*it)->number()
                     << " is isolated. Marking it.";
        }
    }
    qDebug() << "Graph::verticesListIsolated() - isolated vertices list:"
             << m_verticesIsolatedList;
    calculatedIsolates = true;
    return m_verticesIsolatedList;
}

/**
 * @brief Returns a list of all vertices numbers inside the graph
 *
 * @return QList<int>
 */
QList<int> Graph::verticesList()
{
    qDebug() << "Graph::verticesList()";
    if (!m_verticesList.isEmpty() && calculatedVerticesList)
    {
        return m_verticesList;
    }
    VList::const_iterator it;
    m_verticesList.clear();
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        m_verticesList << (*it)->number();
    }
    calculatedVerticesList = true;
    return m_verticesList;
}

/**
 * @brief Returns a QSet of all vertices numbers inside the graph
 * @return
 */
QSet<int> Graph::verticesSet()
{
    qDebug() << "Graph::verticesSet()";
    if (!m_verticesSet.isEmpty() && calculatedVerticesSet)
    {
        return m_verticesSet;
    }
    VList::const_iterator it;
    m_verticesSet.clear();
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        m_verticesSet << (*it)->number();
    }
    calculatedVerticesSet = true;
    return m_verticesSet;
}

/**
 * @brief Creates a subgraph (clique, star, cycle, line) with vertices in vList
 * Iff vList is empty, then fallbacks to the m_verticesSelected.
 * @param vList
 */
void Graph::verticesCreateSubgraph(QList<int> vList,
                                   const int &type,
                                   const int &center)
{

    if (relations() == 1 && edgesEnabled() == 0)
    {
        QString newRelationName = QString::number(vList.size()) + tr("-clique");
        relationCurrentRename(newRelationName, true);
    }

    if (vList.isEmpty())
    {
        vList = m_verticesSelected;
    }

    qDebug() << "Graph::verticesCreateSubgraph() - type:" << type
             << "vList:" << vList;

    int progressCounter = 0;
    QString pMsg = tr("Creating subgraph. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(vList.size(), pMsg);

    qreal weight;

    bool drawArrows = isDirected();
    int edgeType = (isUndirected()) ? EdgeType::Undirected : EdgeType::Reciprocated;

    if (type == SUBGRAPH_CLIQUE)
    {

        for (int i = 0; i < vList.size(); ++i)
        {

            emit signalProgressBoxUpdate(++progressCounter);

            for (int j = i + 1; j < vList.size(); ++j)
            {

                if (!(weight = edgeExists(vList.value(i), vList.value(j))))
                {

                    if ((weight = edgeExists(vList.value(j), vList.value(i))))
                    {
                        edgeTypeSet(vList.value(j), vList.value(i), weight, edgeType);
                    }
                    else
                    {
                        edgeCreate(vList.value(i),
                                   vList.value(j),
                                   1.0,
                                   initEdgeColor,
                                   EdgeType::Undirected,
                                   drawArrows);
                        edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
                    }
                }
                else
                {
                    edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
                }
            }
        }
    }
    else if (type == SUBGRAPH_STAR)
    {

        for (int j = 0; j < vList.size(); ++j)
        {

            emit signalProgressBoxUpdate(++progressCounter);

            if (!(weight = edgeExists(center, vList.value(j))))
            {
                if (center == vList.value(j))
                    continue;

                if ((weight = edgeExists(vList.value(j), center)))
                {
                    edgeTypeSet(vList.value(j), center, weight, edgeType);
                }
                else
                {
                    edgeCreate(center,
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet(center, vList.value(j), weight, edgeType);
                }
            }
            else
            {
                edgeTypeSet(center, vList.value(j), weight, edgeType);
            }
        }
    }
    else if (type == SUBGRAPH_CYCLE)
    {
        int j = 0;
        for (int i = 0; i < vList.size(); ++i)
        {

            emit signalProgressBoxUpdate(++progressCounter);

            j = (i == vList.size() - 1) ? 0 : i + 1;
            if (!(weight = edgeExists(vList.value(i), vList.value(j))))
            {

                if ((weight = edgeExists(vList.value(j), vList.value(i))))
                {
                    edgeTypeSet(vList.value(j), vList.value(i), weight, edgeType);
                }
                else
                {
                    edgeCreate(vList.value(i),
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
                }
            }
            else
            {
                edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
            }
        }
    }
    else if (type == SUBGRAPH_LINE)
    {
        int j = 0;
        for (int i = 0; i < vList.size(); ++i)
        {

            emit signalProgressBoxUpdate(++progressCounter);

            if (i == vList.size() - 1)
                break;
            j = i + 1;
            if (!(weight = edgeExists(vList.value(i), vList.value(j))))
            {

                if ((weight = edgeExists(vList.value(j), vList.value(i))))
                {
                    edgeTypeSet(vList.value(j), vList.value(i), weight, edgeType);
                }
                else
                {
                    edgeCreate(vList.value(i),
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
                }
            }
            else
            {
                edgeTypeSet(vList.value(i), vList.value(j), weight, edgeType);
            }
        }
    }
    else
    {
        emit signalProgressBoxKill();
        return;
    }
    emit signalProgressBoxKill();
}



/**
 * @brief Gets the graph density (if computed) or computes it again.
 *
 * The graph density is the ratio of present edges to total possible edges
 * in the current relation.
 *
 * @return qreal
 */
qreal Graph::graphDensity()
{

    if (calculatedGraphDensity)
    {
        //        qDebug()<< "Graph not changed and density already computed:"
        //                << m_graphDensity;
        return m_graphDensity;
    }

    //    qDebug()<< "Computing graph density...";
    int V = vertices();
    if (V != 0 && V != 1)
    {
        int enabledEdges = edgesEnabled();
        m_graphDensity = (isUndirected()) ? (qreal)2 * enabledEdges / (qreal)(V * (V - 1.0)) : (qreal)enabledEdges / (qreal)(V * (V - 1.0));
    }
    else
    {
        m_graphDensity = 0;
    }
    calculatedGraphDensity = true;
    return m_graphDensity;
}

/**
 * @brief Returns true if the graph is weighted (valued),
 * i.e. if any e in |E| has value not 0 or 1
 *  Complexity: O(n^2)
 * @return
 */
bool Graph::isWeighted()
{

    if (calculatedGraphWeighted)
    {
        qDebug() << "graph not modified. Returning isWeighted: "
                 << m_graphIsWeighted;
        return m_graphIsWeighted;
    }

    qreal m_weight = 0;
    VList::const_iterator it, it1;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Checking if the graph edges are valued. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {
            m_weight = edgeExists((*it1)->number(), (*it)->number());
            if (m_weight != 1 && m_weight != 0)
            {
                setWeighted(true);
                break;
            }
        }
        if (m_graphIsWeighted)
        {
            break;
        }
    }
    calculatedGraphWeighted = true;
    qDebug() << "graph is weighted:" << m_graphIsWeighted;

    emit signalProgressBoxKill();

    return m_graphIsWeighted;
}

/**
 * @brief Sets the graph to be weighted ( valued edges ).
 * @param toggle
 */
void Graph::setWeighted(const bool &toggle)
{
    m_graphIsWeighted = toggle;
}

/**
 * @brief Returns the sum of vertices having edgesOutbound
 * @return
 */
int Graph::verticesWithOutboundEdges()
{
    return outboundEdgesVert;
}

/**
 * @brief Returns the sum of vertices having edgesInbound
 * @return
 */
int Graph::verticesWithInboundEdges()
{
    return inboundEdgesVert;
}

/**
 * @brief Returns the sum of vertices having reciprocal edges
 * @return
 */
int Graph::verticesWithReciprocalEdges()
{
    return reciprocalEdgesVert;
}

/**
 * @brief Gets the arc reciprocity of the graph.
 *
 * Also computes the dyad reciprocity and fills parameters with values.

 * @return qreal
 */
qreal Graph::graphReciprocity()
{

    qDebug() << "Graph::graphReciprocity()";

    if (calculatedGraphReciprocity)
    {
        qDebug() << "Graph::graphReciprocity() - graph not modified and "
                    "already calculated reciprocity. Returning previous result: "
                 << m_graphReciprocityArc;
        return m_graphReciprocityArc;
    }

    qDebug() << "Graph::graphReciprocity() - Computing...";

    emit statusMessage((tr("Calculating the Arc Reciprocity of the graph...")));

    m_graphReciprocityArc = 0;
    m_graphReciprocityDyad = 0;
    m_graphReciprocityTiesReciprocated = 0;
    m_graphReciprocityTiesNonSymmetric = 0;
    m_graphReciprocityTiesTotal = 0;
    m_graphReciprocityPairsReciprocated = 0;
    m_graphReciprocityPairsTotal = 0;

    qreal weight = 0, reciprocalWeight = 0;

    int y = 0, v2 = 0, v1 = 0;

    QHash<int, qreal> enabledOutEdges;

    QHash<int, qreal>::const_iterator hit;
    VList::const_iterator it;

    H_StrToBool totalDyads;
    H_StrToBool reciprocatedDyads;
    QString pair, reversePair;

    // initialize counters
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        (*it)->setOutEdgesReciprocated(0);
        (*it)->setOutEdgesNonSym(0);
        (*it)->setInEdgesNonSym(0);
    }

    // Compute "arc" reciprocity
    //  the number of ties that are involved in reciprocal relations
    //  relative to the total number of actual ties (not possible ties)
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();

        if (!(*it)->isEnabled())
            continue;

        enabledOutEdges = (*it)->outEdgesEnabledHash();

        hit = enabledOutEdges.cbegin();

        while (hit != enabledOutEdges.cend())
        {

            v2 = hit.key();
            y = vpos[v2];
            weight = hit.value();
            m_graphReciprocityTiesTotal += weight;

            // Compute "dyad" reciprocity
            pair = QString::number(v1) + ">" + QString::number(v2);
            reversePair = QString::number(v2) + ">" + QString::number(v1);
            if (!totalDyads.contains(pair) && !totalDyads.contains(reversePair))
            {
                totalDyads[pair] = true;
            }

            qDebug() << pair
                     << "totalTies" << m_graphReciprocityTiesTotal
                     << "totalDyads" << totalDyads.size();

            if ((reciprocalWeight = edgeExists(v2, v1)) == weight)
            {

                (*it)->setOutEdgesReciprocated(); // increase reciprocated ties for ego
                (*it)->setOutEdgesReciprocated();

                m_graphReciprocityTiesReciprocated += reciprocalWeight;

                pair = QString::number(v2) + ">" + QString::number(v1);
                reversePair = QString::number(v1) + ">" + QString::number(v2);
                if (!reciprocatedDyads.contains(pair) && !reciprocatedDyads.contains(reversePair))
                {
                    reciprocatedDyads[pair] = true;
                }

                qDebug() << pair << "reciprocal!"
                         << "reciprocatedTies" << m_graphReciprocityTiesReciprocated
                         << "reciprocatedDyads" << reciprocatedDyads.size();
            }
            else
            {
                (*it)->setOutEdgesNonSym();
                m_graph[y]->setInEdgesNonSym();
                m_graphReciprocityTiesNonSymmetric++;
            }

            ++hit;
        }
    }
    // delete enabledOutEdges;

    m_graphReciprocityArc = (qreal)m_graphReciprocityTiesReciprocated / (qreal)m_graphReciprocityTiesTotal;

    m_graphReciprocityPairsReciprocated = reciprocatedDyads.size();
    m_graphReciprocityPairsTotal = totalDyads.size();

    m_graphReciprocityDyad = (qreal)m_graphReciprocityPairsReciprocated / (qreal)m_graphReciprocityPairsTotal;

    qDebug() << "Graph: graphReciprocity() - Finished. Arc reciprocity:"
             << m_graphReciprocityTiesReciprocated
             << "/"
             << m_graphReciprocityTiesTotal << "=" << m_graphReciprocityArc << "\n"
             << m_graphReciprocityPairsReciprocated
             << "/"
             << m_graphReciprocityPairsTotal << "=" << m_graphReciprocityDyad;

    calculatedGraphReciprocity = true;

    return m_graphReciprocityArc;
}

/**
 * @brief Returns TRUE if the adjacency matrix of the current relation is symmetric
 * @return bool
 */
bool Graph::isSymmetric()
{
    qDebug() << "Graph::isSymmetric() ";

    if (calculatedGraphSymmetry)
    {
        qDebug() << "Graph::isSymmetric() - graph not modified and "
                    "already calculated symmetry. Returning previous result: "
                 << m_graphIsSymmetric;
        return m_graphIsSymmetric;
    }
    m_graphIsSymmetric = true;
    int v2 = 0, v1 = 0;
    qreal weight = 0;

    QHash<int, qreal> enabledOutEdges;

    QHash<int, qreal>::const_iterator hit;
    VList::const_iterator lit;

    for (lit = m_graph.cbegin(); lit != m_graph.cend(); ++lit)
    {
        v1 = (*lit)->number();

        if (!(*lit)->isEnabled())
            continue;

        enabledOutEdges = (*lit)->outEdgesEnabledHash();

        hit = enabledOutEdges.cbegin();

        while (hit != enabledOutEdges.cend())
        {

            v2 = hit.key();
            weight = hit.value();

            if (edgeExists(v2, v1) != weight)
            {

                m_graphIsSymmetric = false;
                //                qDebug() <<"Graph::isSymmetric() - "
                //                         << " graph not symmetric because "
                //                         << v1 << "->" << v2 << " weight " << weight
                //                         << " differs from " << v2 << "->" << v1 ;

                break;
            }
            ++hit;
        }
    }
    // delete enabledOutEdges;
    qDebug() << "Graph: isSymmetric() - Finished. Result:" << m_graphIsSymmetric;
    calculatedGraphSymmetry = true;
    return m_graphIsSymmetric;
}

/**
 * @brief Transforms the graph to symmetric (all edges reciprocal)
 */
void Graph::setSymmetric()
{
    qDebug() << "Tranforming graph to symmetric...";
    VList::const_iterator it;
    int v2 = 0, v1 = 0, weight;
    qreal invertWeight = 0;
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        //        qDebug() << "iterate over edges of v1 " << v1;
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            v2 = it1.key();
            weight = it1.value();
            //            qDebug() << "v1" << v1 << "outLinked to" << v2 << ", weight:" << weight;
            invertWeight = edgeExists(v2, v1);
            if (invertWeight == 0)
            {
                //                qDebug() << "v1" << v1 << "is NOT inLinked from v2" <<  v2  ;
                edgeCreate(v2, v1, weight, initEdgeColor, false, true, false,
                           QString(), false);
            }
            else
            {
                //                qDebug() << "v1" << v1 << "is inLinked from v2" <<  v2  ;
                if (weight != invertWeight)
                    edgeWeightSet(v2, v1, weight);
            }

            ++it1;
        }
    }
    // delete enabledOutEdges;

    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Creates a new binary relation in a valued network using edge
 * dichotomization according to the threshold parameter.
 * @param threshold
 */
void Graph::graphDichotomization(const qreal threshold)
{
    qDebug() << "Graph::graphDichotomization()"
             << "initial relations" << relations();

    int v2 = 0, v1 = 0;
    qreal weight = 0;

    VList::const_iterator it;

    QHash<int, qreal> outEdgesAll;
    QHash<int, qreal>::const_iterator it1;

    QHash<QString, qreal> *binaryTies = new QHash<QString, qreal>;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        qDebug() << "Graph::graphDichotomization() - v" << v1
                 << "iterate over outEdges in all relations";
        outEdgesAll = (*it)->outEdgesEnabledHash(false);
        it1 = outEdgesAll.cbegin();
        while (it1 != outEdgesAll.cend())
        {
            v2 = it1.key();
            weight = it1.value();

            qDebug() << v1 << "->" << v2 << "=" << weight << "Checking opposite.";
            if (weight > threshold)
            {
                if (!binaryTies->contains(QString::number(v1) + "--" + QString::number(v2)))
                {
                    qDebug() << v1 << "--" << v2 << " over threshold. Adding";
                    binaryTies->insert(QString::number(v1) + "--" + QString::number(v2), 1);
                }
                else
                {
                    qDebug() << v1 << "--" << v2 << " exists. Binary Tie already found. Continue";
                }
            }
            ++it1;
        }
    }

    relationAdd("Binary-" + QString::number(threshold), true);

    QHash<QString, qreal>::const_iterator it2;
    it2 = binaryTies->constBegin();
    QStringList vertices;
    qDebug() << "creating binary tie edges";
    while (it2 != binaryTies->constEnd())
    {
        vertices = it2.key().split("--");
        qDebug() << "binary tie " << it2.key()
                 << "vertices.at(0)" << vertices.at(0)
                 << "vertices.at(1)" << vertices.at(1);
        v1 = (vertices.at(0)).toInt();
        v2 = (vertices.at(1)).toInt();
        qDebug() << "calling edgeCreate for"
                 << v1 << "--" << v2;
        edgeCreate(v1, v2, 1, initEdgeColor, EdgeType::Undirected, true, false,
                   QString(), false);
        ++it2;
    }

    // delete outEdgesAll;
    delete binaryTies;
    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
    qDebug() << "final relations" << relations();
}

/**
 * @brief Toggles the graph directed or undirected
 *
 * @param toggle
 * @param signalMW
 */
void Graph::setDirected(const bool &toggle, const bool &signalMW)
{

    qDebug() << "Setting graph directed to:" << toggle;

    if (!toggle)
    {
        setUndirected(true);
    }

    if (toggle == isDirected())
    {
        qDebug() << "Same as now, nothing to do.";
        return;
    }

    m_graphIsDirected = true;

    if (m_graphIsDirected)
    {
        setModStatus(ModStatus::EdgeCount, signalMW);
        return;
    }
}

/**
 * @brief Makes the graph undirected or directed.
 *
 * @param toggle
 * @param signalMW
 */
void Graph::setUndirected(const bool &toggle, const bool &signalMW)
{

    qDebug() << "Toggling graph undirected to" << toggle;

    if (!toggle)
    {
        setDirected(true);
        return;
    }

    if (toggle == isUndirected())
    {
        qDebug() << "Same as now, nothing to do.";
        return;
    }

    m_graphIsDirected = false;

    VList::const_iterator it;
    int v2 = 0, v1 = 0, weight;
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        qDebug() << "Iterating over edges of v1 " << v1;
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            v2 = it1.key();
            weight = it1.value();

            qDebug() << "edge" << "v1" << v1 << "->" << v2 << " = " << "weight" << weight;
            edgeTypeSet(v1, v2, weight, EdgeType::Undirected);
            ++it1;
        }
    }
    // delete enabledOutEdges;

    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount, signalMW);
}

/**
 * @brief Returns true if graph is directed
 *
 * @return bool
 */
bool Graph::isDirected()
{
    qDebug() << "isDirected" << m_graphIsDirected;
    return m_graphIsDirected;
}

/**
 * @brief Returns true if graph is undirected
 *
 * @return bool
 */
bool Graph::isUndirected()
{
    //    qDebug() << "isUndirected: " << !m_graphIsDirected;
    return !m_graphIsDirected;
}

/**
 * @brief Changes the direction type of an existing edge
 *
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeTypeSet(const int &v1,
                        const int &v2,
                        const qreal &weight,
                        const int &dirType)
{

    qDebug() << "Changing the direction type of edge: " << v1
             << "->" << v2 << "new edgeType:" << dirType;

    if (dirType != EdgeType::Directed)
    {

        // check if reverse edge exists
        qreal revEdgeWeight = edgeExists(v2, v1);

        if (revEdgeWeight == 0)
        {
            // Reverse edge does not exist, add it
            qDebug() << "reverse  edge" << v1 << " <- " << v2 << " does not exist - Adding it...";
            // Note: Even if dirType=EdgeType::Undirected we add the opposite edge as EdgeType::Reciprocated
            edgeAdd(v2, v1, weight, EdgeType::Reciprocated, "", initEdgeColor);
        }
        else
        {
            // Reverse edge does exist
            if (dirType == EdgeType::Undirected)
            {
                // Make the edge weights equal
                // TOFIX: how do we decide which of the two weights to keep?
                qDebug() << "Graph::edgeTypeSet(): opposite  " << v1
                         << " <- " << v2 << " exists - equaling weights.";
                if (weight != revEdgeWeight)
                {
                    edgeWeightSet(v2, v1, weight);
                }
            }
            else
            {
                // if dirType is EdgeType::Reciprocated we don't need  to equalize weights
            }
        }
        emit signalEdgeType(v1, v2, dirType);
    }
}


/**
 * @brief Returns the name of the current graph
 *
 * If graph name is empty, then returns current relation name.
 * If no relation exists, returns "noname"
 *
 * @return QString
 */
QString Graph::getName() const
{
    if (m_graphName.isEmpty())
    {
        if (!(relationCurrentName().isEmpty()))
        {
            return relationCurrentName();
        }
        else
        {
            return "noname";
        }
    }
    return m_graphName;
}

/**
 * @brief Sets the name of the current graph
 *
 * @param graphName
 */
void Graph::setName(const QString &graphName)
{
    qDebug() << "Setting graph name to:" << graphName;
    m_graphName = graphName;
}

/**
 * @brief Returns the file name of the current graph, if any.
 *
 * @return QString
 */
QString Graph::getFileName() const
{
    return m_fileName;
}

/**
 * @brief Sets the file name of the current graph
 *
 * @param fileName
 */
void Graph::setFileName(const QString &fileName)
{
    qDebug() << "Setting graph filename to:" << fileName;
    m_fileName = fileName;
}

/**
 * @brief Returns the format of the last file opened
 *
 * @return int
 */
int Graph::getFileFormat() const
{
    return m_fileFormat;
}

void Graph::setFileFormat(const int &fileFormat)
{
    qDebug() << "Setting graph file format to:" << fileFormat;
    m_fileFormat = fileFormat;
}

/**
 * @brief Returns true if the fileFormat is supported for saving
 * @param fileFormat
 * @return
 */
bool Graph::isFileFormatExportSupported(const int &fileFormat) const
{
    if (m_graphFileFormatExportSupported.contains(fileFormat))
    {
        return true;
    }
    return false;
}

/**
 * @brief Sets the graph modification status.
 *
 * If there are major changes or new network, it signals to MW to update the UI.
 *
 * @param int graphNewStatus
 * @param bool signalMW
 */
void Graph::setModStatus(const int &graphNewStatus, const bool &signalMW)
{

    if (m_graphModStatus == ModStatus::NewNet && isEmpty())
    {
        // New network, no vertices. Don't change status.

        qDebug() << "This is a empty new network. Will not change status.";

        emit signalGraphModified(isDirected(),
                                 0,
                                 0,
                                 0,
                                 false);

        return;
    }

    else if (graphNewStatus == ModStatus::NewNet)
    {

        qDebug() << "This is a new network. Setting graph as new...";

        m_graphModStatus = graphNewStatus;

        emit signalGraphModified(isDirected(),
                                 m_totalVertices,
                                 edgesEnabled(),
                                 graphDensity(),
                                 false);

        return;
    }
    else if (graphNewStatus == ModStatus::SavedUnchanged)
    {

        // this is called after loading or saving a file

        qDebug() << "Setting graph as saved/unchanged...";

        m_graphModStatus = graphNewStatus;

        emit signalGraphSavedStatus(true);

        return;
    }
    else if (graphNewStatus > ModStatus::MajorChanges)
    {

        // This is called from any method that alters the graph structure,
        // thus all prior computations are invalidated

        //        qDebug()<<"Major changes, invalidating computations, setting graph as changed...";

        m_graphModStatus = graphNewStatus;

        // Init all calculated* flags to false,
        // to force all relevant methods to recompute
        calculatedGraphReciprocity = false;
        calculatedGraphSymmetry = false;
        calculatedGraphWeighted = false;
        calculatedGraphDensity = false;
        calculatedEdges = false;
        calculatedVertices = false;
        calculatedVerticesList = false;
        calculatedVerticesSet = false;
        calculatedIsolates = false;
        calculatedTriad = false;
        calculatedAdjacencyMatrix = false;
        calculatedDistances = false;
        calculatedCentralities = false;
        calculatedDP = false;
        calculatedDC = false;
        calculatedPP = false;
        calculatedIRCC = false;
        calculatedIC = false;
        calculatedEVC = false;
        calculatedPRP = false;

        if (signalMW)
        {

            //            qDebug() << "signaling to MW that the graph is modified...";

            emit signalGraphModified(isDirected(),
                                     m_totalVertices,
                                     edgesEnabled(),
                                     graphDensity(),
                                     true);
            return;
        }
    }
    else if (graphNewStatus > ModStatus::MinorOptions)
    {

        // this is called from Graph methods that inflict minor changes,
        // i.e. changing vertex positions, labels, etc

        if (m_graphModStatus < ModStatus::MajorChanges)
        {
            //  Do not change status if current status is > MajorChanges
            m_graphModStatus = graphNewStatus;
        }
        //        qDebug()<<"minor changes but needs saving...";
        emit signalGraphSavedStatus(false);
        return;
    }
    else
    {
        qCritical() << "Strange. I should not reach this code...";
        m_graphModStatus = graphNewStatus;
    }
}

/**
 * @brief Returns true of graph is modified (edges/vertices added/removed)
 * @return
 */
bool Graph::isModified() const
{
    if (m_graphModStatus > ModStatus::MajorChanges)
    {
        qDebug() << "Graph::isModified() - isModified: true";
        return true;
    }
    qDebug() << "Graph::isModified() - isModified: false";
    return false;
}

/**
 * @brief Returns true if a graph has been loaded from a file.
 * @return
 */
bool Graph::isLoaded() const
{
    if (!getFileName().isEmpty() && getFileFormat() != FileType::UNRECOGNIZED)
    {
        qDebug() << "isLoaded: true ";
        return true;
    }
    qDebug() << "isLoaded: false ";
    return false;
}

/**
 * @brief Returns true if the graph is saved.
 * @return
 */
bool Graph::isSaved() const
{
    if (m_graphModStatus == ModStatus::NewNet)
    {
        qDebug() << "isSaved: true (new net)";
        return true;
    }
    else if (m_graphModStatus == ModStatus::SavedUnchanged)
    {
        qDebug() << "isSaved: true";
        return true;
    }
    qDebug() << "isSaved: false";
    return false;
}

/**
 * @brief Loads a graph from a given file.
 *
 * It creates a new Parser object, moves it to a another thread,
 * connects signals and slots and calls its run() method.
 *
 * @param fileName
 * @param codecName
 * @param m_showLabels
 * @param maxWidth
 * @param maxHeight
 * @param fileFormat
 * @param sm_two_mode
 * @return
 */
void Graph::loadFile(const QString fileName,
                     const QString codecName,
                     const int fileFormat,
                     const QString delimiter,
                     const int sm_two_mode,
                     const bool sm_has_labels)
{

    qDebug() << "Loading the file:" << fileName;

    qDebug() << "First, clearing current relations...";
    relationsClear();

    qDebug() << "Next, creating new file_parser -- we are on thread:" << this->thread();
    file_parser = new Parser();

    qDebug() << " moving parser to her own new thread...";
    file_parser->moveToThread(&file_parserThread);

    qDebug() << "file_parser thread now: " << file_parser->thread();

    qDebug() << "connecting file_parser signals...";

    connect(&file_parserThread, &QThread::finished,
            file_parser, &QObject::deleteLater);

    connect(file_parser, &Parser::signalAddNewRelation,
            this, &Graph::relationAdd);

    connect(file_parser, &Parser::signalSetRelation,
            this, &Graph::relationSet);

    connect(file_parser, &Parser::signalCreateNode,
            this, &Graph::vertexCreate);

    connect(file_parser, &Parser::signalCreateNodeAtPosRandom,
            this, &Graph::vertexCreateAtPosRandom);

    connect(file_parser, &Parser::signalCreateNodeAtPosRandomWithLabel,
            this, &Graph::vertexCreateAtPosRandomWithLabel);

    connect(file_parser, &Parser::signalCreateEdge,
            this, &Graph::edgeCreate);

    connect(file_parser, &Parser::signalFileLoaded,
            this, &Graph::graphFileLoaded);

    connect(file_parser, SIGNAL(removeDummyNode(int)),
            this, SLOT(vertexRemoveDummyNode(int)));

    connect(file_parser, &Parser::finished,
            this, &Graph::graphLoadedTerminateParserThreads);

    qDebug() << "Starting parser thread...";
    file_parserThread.start();

    qDebug() << "Calling the file_parser to load the file...";
    file_parser->load(
        fileName,
        codecName,
        initVertexSize,
        initVertexColor,
        initVertexShape,
        initVertexNumberColor,
        initVertexNumberSize,
        initVertexLabelColor,
        initVertexLabelSize,
        initEdgeColor,
        canvasWidth,
        canvasHeight,
        fileFormat,
        delimiter,
        sm_two_mode,
        sm_has_labels);
}

/**
 * @brief Graph::graphLoadedTerminateParserThreads
 * @param reason
 */
void Graph::graphLoadedTerminateParserThreads(QString reason)
{
    qDebug() << "Terminating parser threads - reason " << reason
             << " Checking if file_parserThread is running...";
    if (file_parserThread.isRunning())
    {
        qDebug() << "deleting file_parser pointer";
        delete file_parser;
        file_parser = 0; // see why here: https://goo.gl/tQxpGA

        qDebug() << "file_parserThread running."
                    "Calling file_parserThread.quit();";
        file_parserThread.quit();
    }
}

/**
 * @brief Stores loaded file name, graph name, sets edge direction type and signals MW to update the UI
 *
 * Called from Parser when file parsing ends.
 *
 * @param fileType
 * @param fileName
 * @param netName
 * @param totalNodes
 * @param totalLinks
 * @param edgeDirType
 * @param elapsedTime
 * @param message
 */
void Graph::graphFileLoaded(const int &fileType,
                            const QString &fileName,
                            const QString &netName,
                            const int &totalNodes,
                            const int &totalLinks,
                            const int &edgeDirType,
                            const qint64 &elapsedTime,
                            const QString &message)
{
    if (fileType == FileType::UNRECOGNIZED)
    {
        qDebug() << "Could not load file. Signaling to MW the error message...";
        // Emit with error message.
        emit signalGraphLoaded(fileType,
                               QString(),
                               QString(),
                               0,
                               0,
                               0,
                               elapsedTime,
                               message);
        return;
    }

    qDebug() << "Loaded file OK. "
             << "type:" << fileType
             << "filename:" << fileName
             << "nodes:" << totalNodes
             << "links:" << totalLinks
             << "edgeDirType:" << edgeDirType
             << "setting graph as saved/unchanged...";

    setFileName(fileName);

    if (!netName.isEmpty())
        setName(netName);
    else
        setName((fileName.split("/").last()).split("/").first());

    if (edgeDirType == EdgeType::Directed)
    {
        this->setDirected(true);
    }
    else
    {
        this->setDirected(false);
    }

    setFileFormat(fileType);

    setModStatus(ModStatus::SavedUnchanged);

    qDebug() << "Signaling to MW...";

    emit signalGraphLoaded(fileType,
                           fileName,
                           getName(),
                           totalNodes,
                           totalLinks,
                           graphDensity(),
                           elapsedTime,
                           message);
}

/**
 * @brief Saves the current graph to a file.
 *
 * Checks the requested file type and calls the corresponding saveGraphTo...() method
 *
 * @param fileName
 * @param fileType
 *
 */
void Graph::saveToFile(const QString &fileName,
                       const int &fileType,
                       const bool &saveEdgeWeights,
                       const bool &saveZeroWeightEdges)
{
    qDebug() << "Saving current graph to file named:" << fileName;
    bool saved = false;
    switch (fileType)
    {
    case FileType::PAJEK:
    {
        saved = saveToPajekFormat(fileName, getName(), canvasWidth, canvasHeight);
        break;
    }
    case FileType::ADJACENCY:
    {
        saved = saveToAdjacencyFormat(fileName, saveEdgeWeights);
        break;
    }
    case FileType::GRAPHVIZ:
    {
        saved = saveToDotFormat(fileName);
        break;
    }
    case FileType::GRAPHML:
    {
        saved = saveToGraphMLFormat(fileName, saveZeroWeightEdges);
        break;
    }
    default:
    {
        setFileFormat(FileType::UNRECOGNIZED);
        break;
    }
    };
    if (saved)
    {
        setModStatus(ModStatus::SavedUnchanged);
    }
    else
    {
        emit signalGraphSavedStatus(FileType::UNRECOGNIZED);
    }
}

/**
 * @brief Saves the active graph to a Pajek-formatted file
 *
 * Preserves node properties (positions, colours, etc)
 *
 * @param fileName
 * @param networkName
 * @param maxWidth
 * @param maxHeight
 *
 * @return bool
 */
bool Graph::saveToPajekFormat(const QString &fileName,
                              QString networkName,
                              int maxWidth, int maxHeight)
{

    qDebug() << "Saving graph to Pajek-formatted file:" << fileName;

    qreal weight = 0;
    QFileInfo fileInfo(fileName);
    QString fileNameNoPath = fileInfo.fileName();

    networkName = (networkName == "") ? getName().toHtmlEscaped() : networkName;
    networkName = (networkName == "unnamed") ? fileNameNoPath.toHtmlEscaped().left(fileNameNoPath.lastIndexOf('.')) : networkName;

    qDebug() << "networkName:" << networkName;

    maxWidth = (maxWidth == 0) ? canvasWidth : maxWidth;
    maxHeight = (maxHeight == 0) ? canvasHeight : maxHeight;

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        emit statusMessage(tr("Error. Could not write to ") + fileName);
        return false;
    }
    QTextStream t(&f);

    t << "*Network " << networkName << "\n";

    t << "*Vertices " << vertices() << "\n";
    VList::const_iterator it;
    VList::const_iterator jt;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        qDebug() << " Name x " << (*it)->number();
        t << (*it)->number() << " " << "\"" << (*it)->label() << "\"";
        t << " ic ";
        t << (*it)->colorToPajek();
        qDebug() << " Coordinates x " << (*it)->x() << " " << maxWidth << " y " << (*it)->y() << " " << maxHeight;
        t << "\t\t" << (*it)->x() / (maxWidth) << " \t" << (*it)->y() / (maxHeight);
        t << "\t" << (*it)->shape();
        t << "\n";
    }

    t << "*Arcs \n";
    qDebug() << "Graph::saveToPajekFormat: Arcs";
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        for (jt = m_graph.begin(); jt != m_graph.end(); jt++)
        {
            qDebug() << "Graph::saveToPajekFormat:  it=" << (*it)->number() << ", jt=" << (*jt)->number();
            if ((weight = edgeExists((*it)->number(), (*jt)->number())) != 0 && (edgeExists((*jt)->number(), (*it)->number())) != weight)
            {
                qDebug() << "Graph::saveToPajekFormat  weight " << weight
                         << " color " << (*it)->outLinkColor((*jt)->number());
                t << (*it)->number() << " " << (*jt)->number() << " " << weight;
                // FIXME bug in outLinkColor() when we remove then add many nodes from the end
                t << " c " << (*it)->outLinkColor((*jt)->number());
                t << "\n";
            }
        }
    }

    t << "*Edges \n";
    qDebug() << "Graph::saveToPajekFormat: Edges";
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        for (jt = m_graph.begin(); jt != m_graph.end(); jt++)
        {
            qDebug() << "Graph::saveToPajekFormat:  it=" << (*it)->number() << ", jt=" << (*jt)->number();
            if ((weight = edgeExists((*it)->number(), (*jt)->number(), true)) != 0)
            {
                if ((*it)->number() > (*jt)->number())
                    continue;
                t << (*it)->number() << " " << (*jt)->number() << " " << weight;
                t << " c " << (*it)->outLinkColor((*jt)->number());
                t << "\n";
            }
        }
    }
    f.close();

    // Store the filename
    setFileName(fileName);

    // Store the file format
    setFileFormat(FileType::PAJEK);

    emit statusMessage(tr("File %1 saved").arg(fileNameNoPath));
    return true;
}

/**
 * @brief Saves the active graph to an adjacency-formatted file
 *
 * @param fileName
 *
 * @return bool
 */
bool Graph::saveToAdjacencyFormat(const QString &fileName,
                                  const bool &saveEdgeWeights)
{

    qDebug() << "Saving graph to adjacency-formatted file:" << fileName;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        emit statusMessage(tr("Error. Could not write to ") + fileName);
        return false;
    }
    QTextStream outText(&file);

    writeMatrixAdjacencyTo(outText, saveEdgeWeights);

    file.close();

    // Store the filename
    setFileName(fileName);

    // Store the file format
    setFileFormat(FileType::ADJACENCY);

    QString fileNameNoPath = fileName.split("/").last();
    emit statusMessage(QString(tr("Adjacency matrix-formatted network saved into file %1")).arg(fileNameNoPath));
    return true;
}

/**
 * @brief TODO Saves the active graph to a UCINET-formatted file
 *
 * @param fileName
 *
 * @return bool
 */
bool Graph::saveToDotFormat(QString fileName)
{
    Q_UNUSED(fileName);
    return true;
}

/**
 * @brief Saves the current graph to a GraphML-formatted file
 *
 * @param fileName
 * @param networkName
 * @param maxWidth
 * @param maxHeight
 *
 * @return bool
 */
bool Graph::saveToGraphMLFormat(const QString &fileName,
                                const bool &saveZeroWeightEdges,
                                QString networkName,
                                int maxWidth,
                                int maxHeight)
{

    qDebug() << "Saving graph to GraphML-formatted file:" << fileName;

    qreal weight = 0;
    int source = 0, target = 0, edgeCount = 0, m_size = 1, m_labelSize;
    QString m_color, m_labelColor, m_label;
    bool openToken;

    QFileInfo fileInfo(fileName);
    QString fileNameNoPath = fileInfo.fileName();

    QString saveDirPath = fileInfo.canonicalPath();

    QString iconsSubDir = fileInfo.baseName() + "_" + fileInfo.suffix() + "_images";
    QString iconsDirPath = saveDirPath + "/" + iconsSubDir;

    QDir saveDir(saveDirPath);
    qreal rel_coord_x = 0;
    qreal rel_coord_y = 0;

    // Check if there are nodes with custom icons in the network
    if (graphHasVertexCustomIcons())
    {
        qDebug() << "Custom node icons exist."
                 << "Creating images subdir" << iconsDirPath;
        // There are custom node icons in this net.
        // We need to save these custom icons to a folder
        // Create a subdir inside the directory where the actual network file
        // is about to be saved. All custom icons will be copied one-by-one there.
        if (saveDir.mkpath(iconsDirPath))
        {
            qDebug() << "created icons subdir"
                     << iconsDirPath;
        }
        else
        {
            qDebug() << "ERROR creating subdir!";
        }
    }
    else
    {
        qDebug() << "No custom node icons. Nothing to do";
    }

    QString iconPath = QString();
    QString iconFileName = QString();
    QString copyIconFileNamePath = QString();

    // Init custom attributes list and temp hash
    QStringList vertexCustomAttributesList = graphHasVertexCustomAttributes();
    QHash<QString, QString> m_vertexCustomAttributes = QHash<QString, QString>();

    networkName = (networkName == "") ? getName().toHtmlEscaped() : networkName;
    networkName = (networkName == "unnamed") ? fileNameNoPath.toHtmlEscaped().left(fileNameNoPath.lastIndexOf('.')) : networkName;
    qDebug() << "file:" << fileName.toUtf8() << "networkName" << networkName;

    maxWidth = (maxWidth == 0) ? (int)canvasWidth : maxWidth;
    maxHeight = (maxHeight == 0) ? (int)canvasHeight : maxHeight;

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Could not open (for writing) file:" << fileName;
        emit statusMessage(tr("Error. Could not write to ") + fileName);
        return false;
    }
    QTextStream outText(&f);
    QString outTextEncoding = QStringEncoder(outText.encoding()).name();

    qDebug() << "Using default codec for saving stream:" << outTextEncoding;

    qDebug() << " writing xml version...";
    outText << "<?xml version=\"1.0\" encoding=\"" << outTextEncoding << "\"?> \n";
    outText << " <!-- Created by SocNetV " << VERSION << " -->\n";
    outText << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" "
               "      xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance \" "
               "      xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
               "      http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
               "\n";

    qDebug() << "writing keys...";

    outText << "  <key id=\"d0\" for=\"node\" attr.name=\"label\" attr.type=\"string\"> \n"
               "    <default>"
               "</default> \n"
               "  </key> \n";

    outText << "  <key id=\"d1\" for=\"node\" attr.name=\"x_coordinate\" attr.type=\"double\"> \n"
               "    <default>"
            << "0.0" << "</default> \n"
                        "  </key> \n";

    outText << "  <key id=\"d2\" for=\"node\" attr.name=\"y_coordinate\" attr.type=\"double\"> \n"
               "    <default>"
            << "0.0" << "</default> \n"
                        "  </key> \n";
    outText << "  <key id=\"d3\" for=\"node\" attr.name=\"size\" attr.type=\"double\"> \n"
               "    <default>"
            << initVertexSize << "</default> \n"
                                 "  </key> \n";

    outText << "  <key id=\"d4\" for=\"node\" attr.name=\"color\" attr.type=\"string\"> \n"
               "    <default>"
            << initVertexColor << "</default> \n"
                                  "  </key> \n";

    outText << "  <key id=\"d5\" for=\"node\" attr.name=\"shape\" attr.type=\"string\"> \n"
               "    <default>"
            << initVertexShape << "</default> \n"
                                  "  </key> \n";

    // Check if there are nodes with custom icons in this network
    if (graphHasVertexCustomIcons())
    {
        // There are custom icons, so we will copy the default custom icon
        // to the subdir we created earlier
        iconPath = initVertexIconPath;
        iconFileName = QFileInfo(iconPath).fileName();
        copyIconFileNamePath = iconsDirPath + "/" + iconFileName;
        if (!QFile(copyIconFileNamePath).exists())
        {
            if (QFile::copy(iconPath, copyIconFileNamePath))
            {
                qDebug() << "default iconFile saved to:" << copyIconFileNamePath;
            }
            else
            {
                qDebug() << "ERROR saving default iconFile to:" << copyIconFileNamePath;
            }
        }
        else
        {
            qDebug() << "default iconFile already exists in:" << copyIconFileNamePath;
        }
        // And we write a new key (id 51) in our graphml for this default custom icon
        outText << "  <key id=\"d51\" for=\"node\" attr.name=\"custom-icon\" attr.type=\"string\"> \n"
                   "    <default>"
                << iconsSubDir + "/" + iconFileName << "</default> \n"
                                                       "  </key> \n";
    } // end check if custom icons exist

    outText << "  <key id=\"d6\" for=\"node\" attr.name=\"label.color\" attr.type=\"string\"> \n"
               "    <default>"
            << initVertexLabelColor << "</default> \n"
                                       "  </key> \n";
    outText << "  <key id=\"d7\" for=\"node\" attr.name=\"label.size\" attr.type=\"string\"> \n"
               "    <default>"
            << initVertexLabelSize << "</default> \n"
                                      "  </key> \n";
    outText << "  <key id=\"d8\" for=\"edge\" attr.name=\"weight\" attr.type=\"double\"> \n"
               "    <default>1.0</default> \n"
               "  </key> \n";

    outText << "  <key id=\"d9\" for=\"edge\" attr.name=\"color\" attr.type=\"string\"> \n"
               "    <default>"
            << initEdgeColor << "</default> \n"
                                "  </key> \n";

    outText << "  <key id=\"d10\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"> \n"
               "    <default>"
            << "" << "</default> \n"
                     "  </key> \n";

    // Save custom attributes defaults, if any.
    if (!vertexCustomAttributesList.isEmpty())
    {
        qDebug() << "saving defaults for vertexCustomAttributesList:" << vertexCustomAttributesList;
        QString customVertexAttrId;
        for (qsizetype i = 0; i < vertexCustomAttributesList.size(); ++i)
        {
            customVertexAttrId = 'd' + QString::number(1000 + i);
            qDebug() << "customVertexAttrId:" << customVertexAttrId
                     << "customVertexAttr" << vertexCustomAttributesList.at(i);
            outText << "  <key id=\"" + customVertexAttrId + "\" for=\"node\" attr.name=\"" + vertexCustomAttributesList.at(i) + "\" attr.type=\"string\"> \n"
                                                                                                                                 "    <default></default> \n"
                                                                                                                                 "  </key> \n";
        }
    }

    VList::const_iterator it;
    VList::const_iterator jt;
    QString relationName;
    int relationPrevious = relationCurrent();
    for (int i = 0; i < relations(); ++i)
    {
        relationName = (m_relationsList.at(i).simplified()).remove("\"");
        relationSet(i, false);
        qDebug() << "writing graph tag. Relation:" << relationName;

        if (isUndirected())
            outText << "  <graph id=\""
                    << ((relations() == 1) ? networkName : relationName)
                    << "\" edgedefault=\"undirected\"> \n";
        else
            outText << "  <graph id=\""
                    << ((relations() == 1) ? networkName : relationName)
                    << "\" edgedefault=\"directed\"> \n";

        qDebug() << "writing nodes data...";
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
                continue;
            qDebug() << "Node id:" << (*it)->number();
            outText << "    <node id=\"" << (*it)->number() << "\"> \n";
            m_color = (*it)->color();
            m_size = (*it)->size();
            m_labelSize = (*it)->labelSize();
            m_labelColor = (*it)->labelColor();
            m_label = (*it)->label();
            m_label = htmlEscaped(m_label);
            m_vertexCustomAttributes = (*it)->customAttributes();
            outText << "      <data key=\"d0\">" << m_label << "</data>\n";

            rel_coord_x = (*it)->x() / (maxWidth);
            rel_coord_y = (*it)->y() / (maxHeight);

            //            qDebug()<<"Rel coordinates: "
            //                   << rel_coord_x
            //                   << ","
            //                   << rel_coord_y;

            outText << "      <data key=\"d1\">" << rel_coord_x << "</data>\n";
            outText << "      <data key=\"d2\">" << rel_coord_y << "</data>\n";

            if (initVertexSize != m_size)
            {
                outText << "      <data key=\"d3\">" << m_size << "</data>\n";
            }

            if (QString::compare(initVertexColor, m_color, Qt::CaseInsensitive) != 0)
            {
                outText << "      <data key=\"d4\">" << m_color << "</data>\n";
            }

            outText << "      <data key=\"d5\">" << (*it)->shape() << "</data>\n";

            if ((*it)->shape() == "custom")
            {
                iconPath = (*it)->shapeIconPath();
                iconFileName = QFileInfo(iconPath).fileName();
                copyIconFileNamePath = iconsDirPath + "/" + iconFileName;
                if (!QFile(copyIconFileNamePath).exists())
                {
                    if (QFile::copy(iconPath, copyIconFileNamePath))
                    {
                        qDebug() << "iconFile for node:" << (*it)->number()
                                 << "saved to:" << copyIconFileNamePath;
                    }
                    else
                    {
                        qDebug() << "ERROR saving iconFile for" << (*it)->number()
                                 << "saved to: " << copyIconFileNamePath;
                    }
                }
                else
                {
                    qDebug() << "iconFile for node:" << (*it)->number()
                             << "already exists in:" << copyIconFileNamePath;
                }
                outText << "      <data key=\"d51\">" << iconsSubDir + "/" + iconFileName << "</data>\n";
            }

            if (QString::compare(initVertexLabelColor, m_labelColor, Qt::CaseInsensitive) != 0)
            {
                outText << "      <data key=\"d6\">" << m_labelColor << "</data>\n";
            }

            if (initVertexLabelSize != m_labelSize)
            {
                outText << "      <data key=\"d7\">" << m_labelSize << "</data>\n";
            }

            qDebug() << "m_vertexCustomAttributes:" << m_vertexCustomAttributes;
            // TODO: TEST ME
            if (!m_vertexCustomAttributes.isEmpty())
            {
                QString customVertexAttrId;
                QHashIterator<QString, QString> i(m_vertexCustomAttributes);
                int customAttrCount = 0;
                for (auto cit = m_vertexCustomAttributes.cbegin(), end = m_vertexCustomAttributes.cend(); cit != end; ++cit)
                {
                    customVertexAttrId = 'd' + QString::number(1000 + customAttrCount);
                    outText << "      <data key=\"" + customVertexAttrId + "\">" << cit.value() << "</data>\n";
                    customAttrCount++;
                }
            }

            outText << "    </node>\n";
        }

        qDebug() << "writing edges data...";
        edgeCount = 0;
        if (isDirected())
        {
            for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
            {
                for (jt = m_graph.cbegin(); jt != m_graph.cend(); jt++)
                {
                    source = (*it)->number();
                    target = (*jt)->number();
                    m_label = "";

                    // Check if user opted to save zero-weight edges
                    if (saveZeroWeightEdges)
                    {
                        weight = this->edgeExistsVirtual(source, target);
                    }
                    else
                    {
                        weight = this->edgeExists(source, target);
                    }

                    if ((!saveZeroWeightEdges && weight != 0) || (saveZeroWeightEdges && weight != RAND_MAX))
                    {
                        ++edgeCount;
                        m_color = (*it)->outLinkColor(target);
                        m_label = edgeLabel(source, target);
                        m_label = htmlEscaped(m_label);
                        //                        qDebug()<< "edge no:"
                        //                                << edgeCount
                        //                                << "from n1=" << source << "to n2=" << target
                        //                                << "with weight" << weight
                        //                                << "and color" << m_color.toUtf8() ;
                        outText << "    <edge id=\"" << "e" + QString::number(edgeCount)
                                << "\" directed=\"" << "true" << "\" source=\"" << source
                                << "\" target=\"" << target << "\"";

                        openToken = true;
                        if (weight != 0 || (saveZeroWeightEdges && weight != RAND_MAX))
                        {
                            outText << "> \n";
                            outText << "      <data key=\"d8\">" << weight << "</data>" << " \n";
                            openToken = false;
                        }
                        if (QString::compare(initEdgeColor, m_color, Qt::CaseInsensitive) != 0)
                        {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d9\">" << m_color << "</data>" << " \n";
                            openToken = false;
                        }
                        if (!m_label.isEmpty())
                        {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d10\">" << m_label << "</data>" << " \n";
                            openToken = false;
                        }

                        if (openToken)
                            outText << "/> \n";
                        else
                            outText << "    </edge>\n";
                    }
                }
            }
        }
        else
        {
            for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
            {
                for (jt = it; jt != m_graph.end(); jt++)
                {
                    source = (*it)->number();
                    target = (*jt)->number();
                    m_label = "";

                    // Check if user opted to save zero-weight edges
                    if (saveZeroWeightEdges)
                    {
                        weight = this->edgeExistsVirtual(source, target);
                    }
                    else
                    {
                        weight = this->edgeExists(source, target);
                    }

                    if ((!saveZeroWeightEdges && weight != 0) || (saveZeroWeightEdges && weight != RAND_MAX))
                    {
                        ++edgeCount;
                        m_color = (*it)->outLinkColor(target);
                        m_label = edgeLabel(source, target);
                        m_label = htmlEscaped(m_label);
                        //                        qDebug()<< "edge no"
                        //                                << edgeCount
                        //                                << "from n1=" << source << "to n2=" << target
                        //                                << "with weight" << weight
                        //                                << "and color" << m_color.toUtf8() ;
                        outText << "    <edge id=\"" << "e" + QString::number(edgeCount)
                                << "\" directed=\"" << "false" << "\" source=\"" << source
                                << "\" target=\"" << target << "\"";

                        openToken = true;
                        if (weight != 0 || (saveZeroWeightEdges && weight != RAND_MAX))
                        {
                            outText << "> \n";
                            outText << "      <data key=\"d8\">" << weight << "</data>" << " \n";
                            openToken = false;
                        }
                        if (QString::compare(initEdgeColor, m_color, Qt::CaseInsensitive) != 0)
                        {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d9\">" << m_color << "</data>" << " \n";
                            openToken = false;
                        }
                        if (!m_label.isEmpty())
                        {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d10\">" << m_label << "</data>" << " \n";
                            openToken = false;
                        }
                        if (openToken)
                            outText << "/> \n";
                        else
                            outText << "    </edge>\n";
                    }
                }
            }
        }

        outText << "  </graph>\n";
    }
    outText << "</graphml>\n";

    f.close();
    relationSet(relationPrevious, false);

    // Store the filename
    setFileName(fileName);

    // Store the file format
    setFileFormat(FileType::GRAPHML);

    emit statusMessage(tr("File %1 saved").arg(fileNameNoPath));

    return true;
}

/**
 * @brief Sets the directory where reports are saved
 * This is used when exporting prominence distribution images to be used in
 * HTML reports.
 * @param dir
 */
void Graph::setReportsDataDir(const QString &dir)
{
    m_reportsDataDir = dir;
}

/**
 * @brief Sets the precision (number of fraction digits) the app will use
 * when writing real numbers in reports.
 * @param precision
 */
void Graph::setReportsRealNumberPrecision(const int &precision)
{
    m_reportsRealPrecision = precision;
}

/**
 * @brief Sets the length of labels in reports
 * @param length
 */
void Graph::setReportsLabelLength(const int &length)
{
    m_reportsLabelLength = length;
}

/**
 * @brief Sets the chart type in reports
 * @param type
 */
void Graph::setReportsChartType(const int &type)
{
    qDebug() << "Graph::setReportsChartType() - type:" << type;
    if (type == -1)
    {
        m_reportsChartType = ChartType::None;
    }
    else if (type == 0)
    {
        m_reportsChartType = ChartType::Spline;
    }
    else if (type == 1)
    {
        m_reportsChartType = ChartType::Area;
    }
    else if (type == 2)
    {
        m_reportsChartType = ChartType::Bars;
    }
}
