/**
 * @file graph.cpp
 * @brief Implements the Graph class for managing network structures, nodes, and edges in SocNetV.
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
