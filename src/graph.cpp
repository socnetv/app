/******************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt
 
                         graph.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2023 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : https://socnetv.org

*******************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

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
#include <QDateTime> 	// used in exporting centrality files
#include <QAbstractSeries>
#include <QSplineSeries>
#include <QAreaSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPixmap>
#include <QElapsedTimer>
#include <QStringEncoder>
#include <QtCharts/QChart>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>
#include <QNetworkReply>

#include <cstdlib>		//allows the use of RAND_MAX macro 

#include <queue>		//for BFS queue Q
#include <ctime>        // for randomizeThings



/**
 * @brief Constructs a Graph
 */
Graph::Graph(const int &reserveVerticesSize, const int &reserveEdgesPerVertexSize) {

    qRegisterMetaType<MyEdge>("MyEdge");

    qRegisterMetaType<NetworkRequestType>("NetworkRequestType");

    m_totalVertices=0;
    m_totalEdges=0;

    // We do init these two vars here, because they only get their values
    // on MW::resizeEvent which might happen after we have started creating
    // nodes.
    // For instance, this happens when we load a network from command line...
    canvasWidth = 700;
    canvasHeight = 600;

    order=true;		//returns true if the indexes of the list is ordered.

    // Use the given vertices estimate to allocate memory
    // to prevent reallocations and memory fragmentation.
    if ( reserveVerticesSize > 0 ) {
        qDebug() << "Graph reserving this vertices estimate:" << reserveVerticesSize;
        m_graph.reserve(reserveVerticesSize);
    }
    // Store the
    m_reserveEdgesPerVertexSize = reserveEdgesPerVertexSize;

    m_graphModStatus=ModStatus::NewNet;
    m_graphName="";
    m_curRelation=0;
    m_fileFormat=FileType::NOT_SAVED;

    m_graphIsDirected=true;
    m_graphIsWeighted=false;
    m_graphIsConnected=true; // empty/null graph is considered connected
    m_graphIsSymmetric=true;

    m_graphDensity = -1;
    m_fileName ="";

    calculatedGraphReciprocity = false;
    calculatedGraphSymmetry = false;
    calculatedGraphWeighted = false;
    calculatedGraphDensity = false;
    calculatedEdges = false;
    calculatedVertices=false;
    calculatedVerticesList = false;
    calculatedVerticesSet = false;
    calculatedAdjacencyMatrix=false;

    calculatedDistances=false;
    calculatedIsolates = false;
    calculatedDP=false;
    calculatedDC=false;
    calculatedIC=false;
    calculatedEVC=false;
    calculatedCentralities=false;
    calculatedIRCC=false;
    calculatedPP=false;
    calculatedPRP=false;
    calculatedTriad=false;

    m_reportsDataDir = "";
    m_reportsRealPrecision = 6;
    m_reportsLabelLength = 8;
    m_reportsChartType  = ChartType::Spline;

    m_vertexClicked = 0;
    m_clickedEdge.source=0;
    m_clickedEdge.target=0;

    file_parser = 0;

    web_crawler = 0;

    m_graphFileFormatExportSupported<< FileType::GRAPHML
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
                       "<body id=\"socnetv-report\">").arg(VERSION);


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
                       "<body>").arg(VERSION);

    htmlEnd = "</body></html>";
}

/**
 * @brief Destroys the Graph object
 */
Graph::~Graph() {
    qDebug()<<"Graph destructing (because app exit?)...Calling clear()";
    this->clear("exit");
    delete file_parser;
}

/**
 * @brief Clears all vertices
 * @param reason
 */
void Graph::clear(const QString &reason) {
    qDebug()<< "Clearing graph, vertices and data structures... Reason:" << reason;

    qDebug()<< "Asking parser and crawler threads to terminate...";

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

    if ( DM.size() > 0) {
        qDebug() << "clearing DM matrix";
        DM.clear();
    }
    if ( SIGMA.size() > 0) {
        qDebug() << "clearing SIGMA matrix";
        SIGMA.clear();
    }
    if ( sumM.size() > 0) {
        qDebug() << "clearing sumM";
        sumM.clear();
    }
    if ( invAM.size() > 0) {
        qDebug() << "clearing invAM";
        invAM.clear();
    }
    if ( AM.size() > 0) {
        qDebug() << "clearing AM";
        AM.clear();
    }
    if ( invM.size() > 0) {
        qDebug() << "clearing invM";
        invM.clear();
    }
    if ( XM.size() > 0) {
        qDebug() << "clearing XM";
        XM.clear();
    }
    if ( XSM.size() > 0) {
        qDebug() << "clearing XSM";
        XSM.clear();
    }
    if ( XRM.size() > 0) {
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

    //clear relations
    relationsClear();

    // add a default relation, only if we are not closing
    if ( reason != "exit") {
        relationAdd(tr(("unnamed")));
    }

    m_fileFormat=FileType::NOT_SAVED;
    m_graphName="";
    m_fileName ="";

    m_totalVertices=0;
    m_totalEdges=0;

    outboundEdgesVert=0;
    inboundEdgesVert=0;
    reciprocalEdgesVert=0;

    m_vertexClicked = 0;
    m_clickedEdge.source=0;
    m_clickedEdge.target=0;

    order=true;		//returns true if the vpositions of the list is ordered.

    m_graphIsDirected=true;
    m_graphIsWeighted=false;
    m_graphIsConnected=true; // empty/null graph is considered connected.
    m_graphIsSymmetric=true;

    m_graphDensity = -1;
    m_graphDiameter=0;

    m_graphAverageDistance=0;
    m_graphSumDistance = 0;

    m_graphGeodesicsCount = 0; //non zero distances

    calculatedGraphReciprocity = false;
    calculatedGraphSymmetry = false;
    calculatedGraphWeighted = false;
    calculatedGraphDensity = false;
    calculatedEdges = false;
    calculatedVertices=false;
    calculatedVerticesList = false;
    calculatedVerticesSet = false;
    calculatedAdjacencyMatrix=false;

    calculatedDistances=false;
    calculatedIsolates = false;

    calculatedCentralities=false;
    calculatedDP=false;
    calculatedDC=false;
    calculatedIC=false;
    calculatedEVC=false;
    calculatedIRCC=false;
    calculatedPP=false;
    calculatedPRP=false;
    calculatedTriad=false;

    m_graphModStatus=ModStatus::NewNet;

    m_graphHasVertexCustomIcons = false;

    //    if ( urlQueue->size() > 0 ) {
    //        urlQueue->clear();
    //    }

    if ( reason != "exit") {
        qDebug()<< "Finished clearing graph data. Changing graph modification status to" << m_graphModStatus;
        setModStatus(m_graphModStatus,true);
    }
    qDebug()<< "Finished clearing graph data and structures.";
}



/**
 * @brief Sets the size of the canvas
 *
 * Called when the MW is resized to update canvasWidth/canvasHeight, and node positions
 *
 * @param w
 * @param h
 */
void Graph::canvasSizeSet(const int &width, const int &height){

    qreal fX =  (static_cast <qreal> (width)) / canvasWidth;
    qreal fY =  (static_cast <qreal> (height)) / canvasHeight;
    qreal newX, newY;

    qDebug() << "Canvas was resized: " << width << "x" << height
             << "Adjusting node positions, if any. Please wait...";
    VList::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        newX = (*it)->x() * fX ;
        newY = (*it)->y() * fY ;
        (*it)->setX( newX ) ;
        (*it)->setY( newY );
        emit setNodePos((*it)->number(), newX , newY);
    }
    canvasWidth = width;
    canvasHeight= height;
//    emit statusMessage(tr("Canvas size: (%1, %2)px")
//                       .arg(QString::number(canvasWidth))
//                       .arg(QString::number(canvasHeight))
//                       );

    setModStatus(ModStatus::VertexPositions,false);
    qDebug() << "Finished resizing.";
}

/**
 * @brief Gets the max radius of the canvas
 * @return double
 */
double Graph::canvasMaxRadius () const {
    return ( canvasHeight < canvasWidth ) ? canvasHeight / 2.0 -30 : canvasWidth/2.0 - 30;
}

/**
 * @brief Gets the min dimensions of the canvas
 * @return qreal
 */
qreal Graph::canvasMinDimension() const {
    return ( canvasHeight < canvasWidth ) ? canvasHeight-30 : canvasWidth-30;
}

/**
 * @brief Checks if x is visible inside the canvas usable area and if not returns an adjusted x-coordinate
 * @param x
 * @return double
 */
double Graph::canvasVisibleX(const double &x)  const {
    return qMin (canvasWidth - 50.0 , qMax (50.0, x));
}

/**
 * @brief Checks if y is visible inside the canvas usable area and if not returns an adjusted y-coordinate
 * @param y
 * @return double
 */
double Graph::canvasVisibleY(const double &y) const {
    return qMin ( canvasHeight - 50.0 , qMax (50.0 , y ) );
}

/**
 * @brief Returns a random x-coordinate adjusted to be visible inside the canvas usable area
 * @return double
 */
double Graph::canvasRandomX()  const {
    qreal randX = static_cast <qreal> ( rand() % static_cast <int> (canvasWidth) );
    return qMin ( canvasWidth - 30.0 , qMax ( 30.0 , randX ) );
}

/**
 * @brief Returns a random y-coordinate adjusted to be visible inside the canvas usable area
 * @return double
 */
double Graph::canvasRandomY() const {
    qreal randY = static_cast <qreal> ( rand() % static_cast <int> (canvasHeight) );
    return qMin ( canvasHeight - 30.0 , qMax (30.0 , randY ) );
}


/**
 * @brief Changes the current relation, and optionally emits signals to MW/GW (default: true)
 *
 * Forces all enabled vertices to disable edges in the old relation and enable edges of the new relation
 *
 * Then, if updateUI==true (default), it emits signals to MW and GW
 * to update the MW UI and toggle the edges on the GW, respectivelly.
 *
 * Called from Parser, Graph methods and when the user selects a relation in the MW combo box.
 *
 * @param relNum int
 * @param updateUI bool
 */
void Graph::relationSet(int relNum, const bool &updateUI){

    qDebug() << "++ Request to change graph to relation:" << relNum
             << " - current relation:" << m_curRelation
              << "updateUI:" << updateUI;

    //
    // Perform checks for requested new relation number
    //
    if (m_curRelation == relNum ) {
        // Same as current, don't do nothing
        qDebug() << "++ Requested relation is the current one - END";
        return;
    }

    if ( relNum < 0) {
        // negative, don't do nothing
        qDebug() << "++ Requested relation is negative - END ";
        return;
    }
    else if (relNum==RAND_MAX) {
        // Set relation to the last existing relation
        relNum=relations() -1;
    }
    else if (relNum > relations() -1 ) {
        // Invalid relation, abort
        qDebug() << "++ Invalid relation - END ";
        return;
    }

    //
    // Force enabled vertices to disable all edges
    // in the old relation and enable edges in the new relation
    //
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        qDebug() << "++ changing relation of vertex"
                 << (*it)->number()
                 << "to" << relNum;
        if ( ! (*it)->isEnabled() )
            continue;
        (*it)->setRelation(relNum);
    }

    //
    // now store the selected relation
    //
    m_curRelation = relNum;

    //
    // Check if isWeighted so that multiple-relation networks are properly loaded.
    //
    isWeighted();

    //
    // Check if we need to update the UI
    //
    if (updateUI) {
        qDebug() << "++ Signaling to update UI and GW and setting graph mod status to edge count changed.";
        // Notify MW to change combo box relation name
        emit signalRelationChangedToMW(m_curRelation);
        //notify GW to disable/enable the on screen edges.
        emit signalRelationChangedToGW(m_curRelation);
        // update graph mod status
        setModStatus(ModStatus::EdgeCount);
    }
}


/**
 * @brief Changes graph to previous relation
 */
void Graph::relationPrev(){
    qDebug() << "Changing to the previous relation...";
    int relNum=m_curRelation;
    if (m_curRelation>0){
        --relNum;
        relationSet(relNum);
        //editFilterNodesIsolatesAct->setChecked(false);
    }
}

/**
 * @brief Changes graph to next relation
 */
void Graph::relationNext(){
    qDebug() << "Changing to the next relation...";
    int relNum=m_curRelation;
    if ( relations() >0 && relNum < relations() ){
        ++relNum;
        relationSet(relNum);
        //editFilterNodesIsolatesAct->setChecked(false);
    }
}

/**
 * @brief Adds a new relation to the graph
 *
 * Adds a new relation named relName, emitting signal to MW UI, and
 * optionally changing current graph relation to the new one.
 * Called by file parser and various Graph methods, i.e clear() etc.
 *
 * @param relName
 */
void Graph::relationAdd(const QString &relName, const bool &changeRelation) {

    qDebug() << "Adding new relation named:" << relName;

    // Add the new relation to our relations list
    m_relationsList << relName;

    // Emit signal for the new relation to be added to the MW UI combo...
    emit signalRelationAddToMW(relName);

    // Check if we need to change to the new relation...
    if (changeRelation) {
        relationSet();
    }

    emit statusMessage((tr("Added a new relation named: %1."))
                       .arg( relName ) );
}


/**
 * @brief Gets the current relation number
 *
 * @return int
 */
int Graph::relationCurrent(){
    return m_curRelation;
}

/**
 * @brief Gets the current relation name
 *
 * @return string
 */
QString Graph::relationCurrentName() const{
//    qDebug() << "Returning the current relation name...";
    return m_relationsList.value(m_curRelation);
}



/**
 * @brief Renames current relation to newName, optionally emitting a signal to MW
 * @param newName
 */
void Graph::relationCurrentRename(const QString &newName, const bool &signalMW) {

    //
    // Check if new name is the same
    //
    if ( !m_relationsList.isEmpty() && newName == m_relationsList[m_curRelation] ) {
        qDebug()<< "The new name of the relation is the same as the current name. Nothing to do. Returning.";
        return;
    }

    //
    // Check if new name is empty
    //
    if (newName.isEmpty()) {
        qDebug()<< "The new name of the relation is empty. Nothing to do. Returning.";
        return;
    }

    //
    // Rename current relation to newName
    //
    qDebug()<< "Renaming current relation:"
               <<m_curRelation<< "to:"<<newName
              << " - signalMW:" <<signalMW;

    m_relationsList[m_curRelation] = newName;

    //
    // Check if we need to emit a signal
    //
    if (signalMW){
        emit signalRelationRenamedToMW(newName);
    }

}

/**
 * @brief Overload. Renames current relation to newName, without emitting any signal to MW
 *
 * @param newName
 */
void Graph::relationCurrentRename(const QString &newName) {
    relationCurrentRename(newName, false);
}



/**
 * @brief Returns the count of relationships in this Graph
 *
 * @return int
 */
int Graph::relations(){
    //qDebug () << " relations count " << m_relationsList.size();
    return m_relationsList.size();
}



/**
 * @brief Clears relationships in this Graph
 */
void Graph::relationsClear(){
    int oldRelationsCounter = m_relationsList.size();
    m_relationsList.clear();
    m_curRelation=0;
    qDebug () << "Cleared" << oldRelationsCounter << "relation(s)"
              <<"Emitting signalRelationsClear()";
    emit signalRelationsClear();
}

/**
 * @brief Creates a new vertex
 *
 * Main vertex creation slot, associated with homonymous signal from Parser.
 * Adds a vertex to the Graph and signals drawNode to GW
 * The new vertex has number num and specific color, label, label color, shape and position p.
 *
 * @param num
 * @param size
 * @param nodeColor
 * @param numColor
 * @param numSize
 * @param label
 * @param lColor
 * @param lSize
 * @param p
 * @param nodeShape
 * @param signalMW
 */
void Graph::vertexCreate(const int &number,
                         const int &size,
                         const QString &color,
                         const QString &numColor,
                         const int &numSize,
                         const QString &label,
                         const QString &labelColor,
                         const int &labelSize,
                         const QPointF &p,
                         const QString &shape,
                         const QString &iconPath,
                         const bool &signalMW) {

    qDebug() << "Creating a new vertex:" << number
             << "shape:" << shape
             << "icon:" << iconPath
             << "signalMW:" << signalMW
             << "- Appending the new vertex and signaling to GW to create the node";

    if (order)
        vpos[number]=m_totalVertices;
    else
        vpos[number]=m_graph.size();

    m_graph.append(
                new GraphVertex (
                    this,
                    number,
                    m_curRelation ,
                    size,
                    color,
                    numColor,
                    numSize,
                    label,
                    labelColor,
                    labelSize,
                    p,
                    shape,
                    iconPath,
                    m_reserveEdgesPerVertexSize
                    )
                );

    m_totalVertices++;


    emit signalDrawNode( p,
                         number,
                         size,
                         shape,
                         iconPath,
                         color,
                         numColor,
                         numSize,
                         initVertexNumberDistance,
                         label,
                         labelColor,
                         labelSize,
                         initVertexLabelDistance);

    qDebug() << "Finished creating new vertex:" << number << "Setting graph mod status";

    setModStatus(ModStatus::VertexCount, signalMW);

    //to draw new vertices by user with the same style of the file loaded:
    //save color, size and shape as init values
    initVertexColor=color;
    initVertexSize=size;
    initVertexShape=shape;
    if (shape=="custom"){
        initVertexIconPath=iconPath;
    }
} 



/**
 * @brief Creates a new vertex in the given position
 *
 * Called from GW, with i and p as parameters.
 * Calls the main creation slot with init node values.
 *
 * @param QPointF  The clicked pos of the new node.
 */
void Graph::vertexCreateAtPos(const QPointF &p){
    int i = vertexNumberMax() +1;

    qDebug() << "Creating a new vertex:" << i << " in given position:" << p;

    vertexCreate( i, initVertexSize,  initVertexColor,
                  initVertexNumberColor, initVertexNumberSize,
                  QString(), initVertexLabelColor, initVertexLabelSize,
                  p, initVertexShape, initVertexIconPath,
                  true
                  );

    emit statusMessage(  tr("New node (numbered %1) added at position (%2,%3). Double-click on it to start a new edge from it.")
                         .arg(vertexNumberMax())
                         .arg( p.x() )
                         .arg( p.y() )
                         ) ;

}





/**
 * @brief Creates a new randomly positioned vertex with default values
 *
 * Computes a random position p inside the useable canvas area
 * Then calls the main creation slot with init node values.
 *
 * @param bool
 *
 */
void Graph::vertexCreateAtPosRandom(const bool &signalMW){

    QPointF p;
    p.setX( canvasRandomX());
    p.setY( canvasRandomY() );
    qDebug() << "Creating a new random positioned vertex at:" << p;
    vertexCreate( vertexNumberMax()+1, initVertexSize, initVertexColor,
                  initVertexNumberColor, initVertexNumberSize,
                  QString(), initVertexLabelColor, initVertexLabelSize,
                  p, initVertexShape, initVertexIconPath, signalMW
                  );
}



/**
 * @brief Creates a new randomly positioned vertex with specific number and label.
 * All other values are from the defaults.
 *
 * Called from WebCrawler and Parser with parameters label and i.
 * Computes a random position p the useable canvas area
 * Then calls the main creation slot with init node values.
 *
 * @param i
 * @param label
 * @param signalMW
 */
void Graph::vertexCreateAtPosRandomWithLabel(const int &i,
                                             const QString &label,
                                             const bool &signalMW) {

    qDebug() << "Creates a new randomly positioned vertex:" << i
             << "with label:" << label;
    QPointF p;
    p.setX(canvasRandomX());
    p.setY(canvasRandomY());
    vertexCreate( (i<0)?vertexNumberMax() +1:i, initVertexSize,  initVertexColor,
                  initVertexNumberColor, initVertexNumberSize,
                  label, initVertexLabelColor,  initVertexLabelSize,
                  p, initVertexShape, initVertexIconPath, signalMW
                  );

}



/**
 * @brief Deletes a dummy node
 *
 * This is called from Parser (as pajek) to delete any redundant (dummy) nodes.
 *
 * @param int i number of node
 */
void Graph::vertexRemoveDummyNode(int i){
    qDebug() << "Removing dummy node from graph: " << i;
    vertexRemove(i);
}




/**
 * @brief Returns the number of the last vertex in the graph.
 *
 * @return  int
 */
int Graph::vertexNumberMax() {
    if (m_totalVertices>0)
        return m_graph.back()->number();
    else return 0;
}



/**
 * @brief Returns the number of the first vertex in the graph.
 *
 * @return int
 */
int Graph::vertexNumberMin() {
    if (m_totalVertices>0)
        return m_graph.front()->number();
    else return 0;
}



/**
 * @brief Checks if the given vertex exists in the graph.
 *
 * Returns the vpos or -1
 *
 * Complexity:  O(logN) for vpos retrieval
 *
 * @param vertex number
 * @return vertex pos or -1
 */
int Graph::vertexExists(const int &v1){
//    qDebug () << "Checking if vertex exists, with number:" << v1;
    if ( vpos.contains(v1) ) {
        if (  m_graph[ vpos[v1] ]->number() == v1 ) {
            return vpos[v1];
        }
        else{
            qCritical () << "Error in vpos for vertex number v:" << v1;
        }
    }

    return -1;
}



/**
 * @brief Checks if there is a vertex with a specific label exists in the graph
 *
 * Returns the vpos or -1
 *
 * Complexity:  O(N)
 *
 * @param label
 * @return vpos or -1
 */
int Graph::vertexExists(const QString &label){
    qDebug () << "Checking if vertex exists, with label:" << label.toUtf8()  ;
    VList::const_iterator it;
    int i=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( (*it)->label().contains( label, Qt::CaseInsensitive ) )  {
            return i;
        }
        i++;
    }
    return -1;
}


/**
 * @brief Finds vertices in strList by their number
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByNumber (const QStringList &numList) {
    qDebug() << "Finding vertices by number - searchList:" << numList;
    QString vStr;
    QList<int> foundList;
    QStringList notFound;
    int v=-1;
    bool intOk=false;
    bool searchResult = false;
    for (int i = 0; i < numList.size(); ++i) {
        vStr = numList.at(i);
        v = vStr.toInt(&intOk);
        if (intOk) {
            if ( vertexExists(v) != -1 ) {
                qDebug() << "vertex number" << v
                         << "exists. Adding it to found list";
                foundList << v;
            }
            else {
                qDebug() << "vertex number" << v
                         << "does not exist. Adding it to notFound list";
                notFound << vStr;
            }
        }
        else {
            qDebug() << "cannot read" << vStr;
        }

    }

    if ( !foundList.isEmpty() ) {
        searchResult = true;
        qDebug() << "One or more matching nodes found. Signaling to GW to highlight them...";
        emit statusMessage ( tr("Found %1 matching nodes.").arg(foundList.size()) );
        emit signalNodesFound(foundList);
    }
    else {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage ( tr("Could not find any nodes matching your choices.") );
    }

    return searchResult;

}


/**
 * @brief Finds vertices by their label
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByLabel (const QStringList &labelList) {
    qDebug() << "Finding vertices by label - searchList:" << labelList;

    QString vLabel;
    QList<int> foundList;
    int vFoundPos = -1;
    QStringList notFound;
    bool searchResult = false;
    for (int i = 0; i < labelList.size(); ++i) {
        vLabel = labelList.at(i);

        if ( ( vFoundPos = vertexExists(vLabel) ) != -1 ) {
            qDebug() << "vertex with label" << vLabel
                     << "exists. Adding it to found list";
            foundList << m_graph[ vFoundPos ]->number();
        }
        else {
            qDebug() << "vertex with label" << vLabel
                     << "does not exist. Adding it to notFound list ";
            notFound << vLabel;
        }

    }

    if ( !foundList.isEmpty() ) {
        searchResult = true;
        qDebug() << "One or more matchin nodes found. Signaling to GW to highlight them...";
        emit statusMessage ( tr("Found %1 matching nodes.").arg(foundList.size()) );
        emit signalNodesFound(foundList);
    }
    else {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage ( tr("Could not find any nodes matching your choices.") );
    }

    return searchResult;
}




/**
 * @brief Finds vertices by their index score
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByIndexScore(const int &index, const QStringList &thresholds,
                                   const bool &considerWeights, const bool &inverseWeights, const bool &dropIsolates) {

    qDebug()<<"Finding vertices by index"<< index
           << "threshold list" << thresholds
           << "considerWeights"<<considerWeights
           << "inverseWeights" << inverseWeights
           << "dropIsolates" <<dropIsolates;

    QList<int> foundList;

    bool searchResult = false;

    VList::const_iterator it;

    QString thresholdStr="";

    bool gtThan  = false;
    bool gtEqual = false;
    bool lsThan  = false;
    bool lsEqual = false;
    bool convertedOk=false;
    qreal threshold=0;
    qreal score=0;

    switch (index) {
    case 0: {
        // do nothing
        break;
    }
    case IndexType::DC : {
        centralityDegree(considerWeights, dropIsolates);
        break;
    }
    case IndexType::IRCC : {
        centralityClosenessIR();
        break;
    }
    case IndexType::IC : {
        centralityInformation(considerWeights, inverseWeights);
        break;
    }
    case IndexType::EVC : {
        centralityEigenvector(considerWeights, inverseWeights, dropIsolates);
        break;
    }
    case IndexType::DP : {
        prestigeDegree(considerWeights, dropIsolates);
        break;
    }
    case IndexType::PRP : {
        prestigePageRank(dropIsolates);
        break;
    }
    case IndexType::PP : {
        prestigeProximity(considerWeights, inverseWeights);
        break;
    }
    default:
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);
        break;
    }

    // Parse threshold user input
    for (int i = 0; i < thresholds.size(); ++i) {

        thresholdStr = thresholds.at(i);
        thresholdStr=thresholdStr.simplified();

        gtThan  = false;
        gtEqual = false;
        lsThan  = false;
        lsEqual = false;

        convertedOk=false;

        if (thresholdStr.startsWith(">=") || thresholdStr.startsWith("=>")) {
            gtEqual = true;
            thresholdStr.remove(">=");
            thresholdStr.remove("=>");
            qDebug()<< "thresholdStr starts with >=";
        }
        else if (thresholdStr.startsWith(">")) {
            gtThan = true;
            thresholdStr.remove(">");
            qDebug()<< "thresholdStr starts with > ";
        }
        else if (thresholdStr.startsWith("<=") || thresholdStr.startsWith("=<")) {
            lsEqual = true;
            thresholdStr.remove("<=");
            thresholdStr.remove("=<");
            qDebug()<< "thresholdStr starts with <=";
        }
        else if (thresholdStr.startsWith("<"))  {
            lsThan = true;
            thresholdStr.remove("<");
            qDebug()<< "thresholdStr starts with < ";
        }
        else {
            qDebug()<< "thresholdStr does not start with > or <";
            continue;
        }

        // Parse score threshold
        threshold = thresholdStr.toDouble(&convertedOk);

        if (!convertedOk) {
            qDebug()<< "cannot convert thresholdStr to float";
            continue;
        }
        else {
            qDebug()<< "threshold"<<threshold;
        }

        // Iterate over all vertices and get their scores
        for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

            switch (index) {
            case 0: {
                score=0;
                break;
            }
            case IndexType::DC : {
                score=(*it)->SDC();
                break;
            }
            case IndexType::CC : {
                score=(*it)->SCC();
                break;
            }
            case IndexType::IRCC : {
                score=(*it)->SIRCC();
                break;
            }
            case IndexType::BC : {
                score=(*it)->SBC();
                break;
            }
            case IndexType::SC : {
                score=(*it)->SSC();
                break;
            }
            case IndexType::EC : {
                score=(*it)->SEC();
                break;
            }
            case IndexType::PC : {
                score=(*it)->SPC();
                break;
            }
            case IndexType::IC : {
                score=(*it)->SIC();
                break;
            }
            case IndexType::EVC : {
                score=(*it)->SEVC();
                break;
            }
            case IndexType::DP : {
                score=(*it)->SDP();
                break;
            }
            case IndexType::PRP : {
                score=(*it)->SPRP();
                break;
            }
            case IndexType::PP : {
                score=(*it)->SPP();
                break;
            }
            }

            if (gtThan) {
                if ( score > threshold ) {
                    qDebug() << "matching vertex"<< (*it)->number() << "score"<< score;
                    foundList << (*it)->number();
                }
            }
            else if (gtEqual) {
                if ( score >= threshold ) {
                    qDebug() << "matching vertex"<< (*it)->number() << "score"<< score;
                    foundList << (*it)->number();
                }
            }
            else if (lsThan){
                if ( score < threshold ) {
                    qDebug() << "matching vertex"<< (*it)->number() << "score"<< score;
                    foundList << (*it)->number();
                }
            }
            else if (lsEqual){
                if ( score <= threshold ) {
                    qDebug() << "matching vertex"<< (*it)->number() << "score"<< score;
                    foundList << (*it)->number();
                }
            }

        }
    }


    if ( !foundList.isEmpty() ) {
        searchResult = true;
        qDebug() << "One or more matching nodes found. Signaling to GW to highlight them...";
        emit statusMessage ( tr("Found %1 matching nodes.").arg(foundList.size()) );
        emit signalNodesFound(foundList);
    }
    else {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage ( tr("Could not find any nodes matching your choices.") );
    }

    return searchResult;
}



/**
 * @brief Removes the vertex v1 from the graph
 * First, it removes all edges to doomed from other vertices
 * Then it changes the vpos of all subsequent vertices inside m_graph
 * Finally, it removes the vertex.
 * @param int v1
 */
void Graph::vertexRemove(const int &v1){
    qDebug() << "Removing vertex:"
             << m_graph[ vpos[v1] ]->number()
             << "vpos:" << vpos[v1]
                << "Removing all inbound and outbound edges ";
    int doomedPos=vpos[v1];

    //Remove links to v1 from each other vertex
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if  ( qAbs((*it)->hasEdgeTo(v1) ) > 0) {
            qDebug()<< "another vertex" << (*it)->number()
                    << " has outbound Edge to "<< v1 << ". Removing it.";
            (*it)->removeOutEdge(v1);
        }
        if (  qAbs((*it)->hasEdgeFrom(v1)) > 0 ) {
            qDebug()<< "another vertex" << (*it)->number()
                    << " has inbound Edge from "<< v1 << ". Removing it.";
            (*it)->removeInEdge(v1);
        }
    }

    qDebug()<< "Finished with vertices. "
               "Update the vpos which maps vertices inside m_graph " ;
    int prevIndex=doomedPos;

    qDebug()<< "Updating vpos of all subsequent vertices ";
    H_Int::const_iterator it1=vpos.cbegin();
    while (it1 != vpos.cend()){
        if ( it1.value() > doomedPos ) {
            prevIndex = it1.value();
            qDebug() << "vertex" << it1.key()
                     << "had prevIndex:" << prevIndex
                     << " > doomedPos" << doomedPos
                     << "Setting new vpos. vpos size was: "<< vpos.size();
            vpos.insert( it1.key(), --prevIndex)  ;
            qDebug() << "vertex" << it1.key()
                     << "new vpos:" << vpos.value( it1.key(), -666)
                     << "vpos size now:"<< vpos.size();

        }
        else {
            qDebug() << "vertex" << it1.key() << "with vpos"
                     << it1.value() << " =< doomedPos. CONTINUE";

        }
        ++it1;
    }

    //Now remove vertex Doomed from m_graph
    qDebug()<< "graph vertices=size="<< vertices() << "="
             << m_graph.size() <<  "removing vertex at vpos " << doomedPos ;
    m_graph.removeAt( doomedPos ) ;
    m_totalVertices--;
    qDebug()<< "Now graph vertices=size="<< vertices() << "="
             << m_graph.size() ;

    order=false;

    // Check if this was the clicked vertex and unset it
    if (vertexClicked()==v1) {
       vertexClickedSet(0, QPointF(0,0));
    }

    setModStatus(ModStatus::VertexCount);

    emit signalRemoveNode(v1);
}







/**
 * @brief Toggles the status of all isolated vertices (thos without links)
 *
 * For each isolate vertex in the Graph, emits the setVertexVisibility signal
 *
 * @param toggle
 */
void Graph::vertexIsolatedAllToggle(const bool &toggle){
    qDebug() << "Setting all isolated vertices to" << toggle;

    VList::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( !(*it)->isIsolated() ){
            continue;
        }
        else {
            qDebug() << "vertex" << (*it)->number()
                     << "is isolated. Toggling it and emitting setVertexVisibility signal to GW...";
            (*it)->setEnabled (toggle) ;

            setModStatus(ModStatus::VertexCount);

            emit setVertexVisibility( (*it)->number(), toggle );
        }
    }
}



/**
 * @brief Checks if vertex is isolated
 * @param v1
 * @return
 */
bool Graph::vertexIsolated(const int &v1) const{
    if (  m_graph[ vpos[v1] ]->isIsolated() ) {
        qDebug()<<"vertex:"<< v1 << "is isolated";
        return true;
    }
    qDebug()<<"vertex:"<< v1 << "not isolated";
    return false;
}






/**
 * @brief Changes the position of the given vertex
 *
 * Called from MW/GW when node moves to update its position
 *
 * @param v1
 * @param x
 * @param y
 */
void Graph::vertexPosSet(const int &v1, const int &x, const int &y){

    m_graph[ vpos[v1] ]->setX( x );
    m_graph[ vpos[v1] ]->setY( y );
    setModStatus(ModStatus::VertexPositions,false);
}


/**
 * @brief Returns the position of the given vertex
 * @param v1
 * @return
 */
QPointF Graph::vertexPos(const int &v1) const{
    return m_graph[ vpos[v1] ]->pos();
}



/**
 * @brief Sets the clicked vertex.
 *
 * Signals to MW to show node info on the status bar.
 *
 * @param v1
 * @param p
 */
void Graph::vertexClickedSet(const int &v1, const QPointF &p) {
    qDebug()<<"Setting clicked vertex: " << v1 << "click at " << p;
    m_vertexClicked = v1;
    if (v1 == 0) {
        emit signalNodeClickedInfo(0, p);
    }
    else {
        edgeClickedSet(0,0);
        emit signalNodeClickedInfo( v1,
                                    vertexPos(v1),
                                    vertexLabel(v1),
                                    vertexDegreeIn(v1),
                                    vertexDegreeOut(v1)
                                    );
    }
}


/**
 * @brief Returns the number of the clicked vertex
 * @return  int
 */
int Graph::vertexClicked() const {
    return m_vertexClicked;
}

/**
 * @brief Sets the initial vertex size
 *
 * @param size
 */
void Graph::vertexSizeInit (const int size) {
    initVertexSize=size;
}



/**
 * @brief Changes the size of a vertex v or all vertices if v=0
 *
 * Called from MW (i.e. user changing node properties)
 *
 * @param v
 * @param size
 */
void Graph::vertexSizeSet(const int &v, const int &size) {
    if (v) {
        qDebug()<< "Changing size of vertex" << v <<"new size" << size;
        m_graph[ vpos[v] ]->setSize(size);
        emit setNodeSize(v, size);
    }
    else {
        qDebug()<< "Changing size of all vertices, new size" << size;
        vertexSizeInit(size);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                (*it)->setSize(size) ;
                emit setNodeSize((*it)->number(), size);
            }
        }

    }

    setModStatus(ModStatus::VertexMetadata);

}

/**
 * @brief Returns the size of vertex v
 * @param v
 * @return int
 */
int Graph::vertexSize( const int &v ) const {
    return m_graph[ vpos[v] ]->size();
}



/**
 * @brief Sets the default vertex shape and iconPath
 *
 * @param shape
 * @param iconPath
 */
void Graph::vertexShapeSetDefault(const QString shape, const QString &iconPath) {
    initVertexShape=shape;
    initVertexIconPath=iconPath;
    if ( ! iconPath.isEmpty() ) {
        m_graphHasVertexCustomIcons = true;
    }
    else {
        m_graphHasVertexCustomIcons = false;
    }

}



/**
 * @brief Changes the shape and iconPath of vertex v1, or all vertices if v1=-1
 * @param v1
 * @param shape
 * @param iconPath
 */
void Graph::vertexShapeSet(const int &v1, const QString &shape, const QString &iconPath){

    if ( v1 == -1 ) {
        qDebug() << "Changing shape for all vertices, new shape:" << shape
                 << "iconPath:" << iconPath;
        vertexShapeSetDefault(shape, iconPath);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                (*it)->setShape(shape, iconPath);
                emit setNodeShape((*it)->number(), shape, iconPath);
            }
        }
    }
    else {
        qDebug() << "Changing shape for vertex:" << v1
                 << "new shape:" << shape
                 << "iconPath:" <<iconPath;
        m_graph[ vpos[v1] ]->setShape(shape, iconPath);
        if (shape=="custom") { m_graphHasVertexCustomIcons = true; }
        emit setNodeShape(v1, shape, iconPath);
    }
    setModStatus(ModStatus::VertexMetadata);
}



/**
 * @brief Returns the shape of this vertex
 * @param v1
 * @return
 */
QString Graph::vertexShape(const int &v1){
    return m_graph[ vpos[v1] ]->shape();

}

/**
 * @brief Returns the IconPath of vertex v1
 * @param v1
 * @return
 */
QString Graph::vertexShapeIconPath(const int &v1) {
    return m_graph[ vpos[v1] ]->shapeIconPath();
}





/**
 * @brief Changes the color of vertex v1
 * @param v1
 * @param color
 */
void Graph::vertexColorSet(const int &v1, const QString &color){

    if (v1) {
        qDebug()<< "Setting vertex"<< v1 << "new color"<< color;
        m_graph[ vpos[v1] ]->setColor ( color );
        emit setNodeColor ( m_graph[ vpos[v1] ]->number(), color );
    }
    else {
        qDebug()<< "Setting new color for all vertices:"<< color;
        vertexColorInit(color);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                qDebug() << "for all, setting vertex" << (*it)->number()
                         << " new color" << color;
                (*it)->setColor(color) ;
                emit setNodeColor ( (*it)->number(), color );
            }
        }
    }
    setModStatus(ModStatus::VertexMetadata);
}


/**
 * @brief Graph::vertexColor
 * @param v1
 * @return
 */
QColor Graph::vertexColor(const int &v1) const {
    return  QColor ( m_graph[ vpos[v1] ]->color() ) ;
}


/**
 * @brief Graph::vertexColorInit
 * default vertex color initialization
 * @param color
 */
void Graph::vertexColorInit(const QString &color){
    initVertexColor=color;
}






/**
 * @brief Changes the initial color of the vertex numbers
 * @param color
 */
void Graph::vertexNumberColorInit (const QString &color) {
    initVertexNumberColor = color;
}



/**
 * @brief Graph::vertexColorSet
 * Changes the color of vertex v1
 * @param v1
 * @param color
 */
void Graph::vertexNumberColorSet(const int &v1, const QString &color){
    qDebug()<< "Setting number color for vertex:"<< v1 << "new number color:"<< color;
    if ( v1  ) {
        m_graph[ vpos[v1] ]->setNumberColor ( color );
        emit setNodeNumberColor ( m_graph[ vpos[v1] ]->number(), color );
    }
    else {
        qDebug()<< "Changing color for all node numbers";
        vertexNumberColorInit(color);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                (*it)->setNumberColor(color) ;
                emit setNodeNumberColor ( (*it)->number(), color );
            }
        }
    }
    setModStatus(ModStatus::VertexMetadata);
}



/**
 * @brief Changes the initial size of vertex numbers
 * @param size
 */
void Graph::vertexNumberSizeInit (const int &size) {
    initVertexNumberSize = size;
}



/**
 * @brief Changes the size of vertex v number
 * @param v
 * @param size
 */
void Graph::vertexNumberSizeSet(const int &v, const int &size) {

    if (v) {
        qDebug() << "Changing number size for vertex"<< v <<"new number size" << size;
        m_graph[ vpos[v] ]->setNumberSize (size);
        emit setNodeNumberSize ( m_graph[ vpos[v] ]->number(), size);
    }
    else {
        qDebug() << "Setting new number size for all vertices to:" << size;
        vertexNumberSizeInit(size);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                qDebug() << "for all, setting vertex"<< (*it)->number()
                         << " new number size " << size;
                (*it)->setNumberSize(size) ;
                emit setNodeNumberSize ( (*it)->number(), size);
            }
        }
    }

    setModStatus(ModStatus::MinorOptions);
}



/**
 * @brief Changes the initial distance of vertex numbers
 * @param distance
 */
void Graph::vertexNumberDistanceInit(const int &distance) {
    initVertexNumberDistance = distance;
}


/**
 * @brief Changes the distance.of vertex v number from the vertex
 * @param v
 * @param size
 */
void Graph::vertexNumberDistanceSet(const int &v, const int &newDistance) {
    if (v) {
        qDebug() << "Changing number distance for vertex" << v
                 << "new number distance"
                 << newDistance;

        m_graph[ vpos[v] ]->setNumberDistance (newDistance);
        emit setNodeNumberDistance(v, newDistance);
    }
    else {
        qDebug() << "Changing number distance for all vertices, "
                    "new number distance"
                 << newDistance;
        vertexNumberDistanceInit(newDistance);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                (*it)->setNumberDistance(newDistance) ;
                emit setNodeNumberDistance ( (*it)->number(), newDistance);
            }
        }

    }
    setModStatus(ModStatus::MinorOptions);

}






/**
 * @brief Changes the label of a vertex v1
 * @param v1
 * @param label
 */
void Graph::vertexLabelSet(const int &v1, const QString &label){
    qDebug()<< "Graph::vertexLabelSet() - vertex "<< v1
            << "vpos " << vpos[v1]
               << "new label"<< label;
    m_graph[ vpos[v1] ]->setLabel ( label);
    emit setNodeLabel ( m_graph[ vpos[v1] ]->number(), label);

    setModStatus(ModStatus::VertexMetadata);
}



/**
 * @brief Returns the label of a vertex v1
 * @param v1
 * @return
 */
QString Graph::vertexLabel(const int &v) const{
    return m_graph[ vpos[v] ]->label ();
}




/**
 * @brief Graph::vertexLabelSizeInit
 * Changes the default size of vertex labels
 * @param newSize
 */
void Graph::vertexLabelSizeInit(int newSize) {
    initVertexLabelSize = newSize;
}



/**
 * @brief Changes the label size of vertex v1 or all vertices if v1=0
 * @param v1
 * @param size
 */
void Graph::vertexLabelSizeSet(const int &v1, const int &labelSize) {
    if (v1) {
        qDebug()<< "Changing the label size of vertex"<< v1
                << "new label size:"<< labelSize;
        m_graph[ vpos[v1] ]->setLabelSize ( labelSize );
        emit setNodeLabelSize ( v1, labelSize);
    }
    else {
        qDebug() << "Changing the label size of all vertices, new label size"
                 << labelSize;
        vertexLabelSizeInit(labelSize);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                qDebug() << "Changing label size of all vertices, set vertex"
                            << (*it)->number()
                            << "new label size"
                            << labelSize;
                (*it)->setLabelSize(labelSize) ;
                emit setNodeLabelSize ( (*it)->number(), labelSize);
            }
        }
    }

    setModStatus(ModStatus::MinorOptions);

}





/**
 * @brief Changes the label color of vertex v1 or all vertices if v1 = 0
 * @param v1
 * @param color
 */
void Graph::vertexLabelColorSet(const int &v1, const QString &color){
    if (v1) {
        qDebug() << "Changing the label color of vertex" << v1
                 << "new label color" << color;
        m_graph[ vpos[v1] ]->setLabelColor(color);
        emit setNodeLabelColor(v1, color);
    }
    else {
        qDebug() << "Changing the label color of all vertices, "
                    "new label color" << color;
        vertexLabelColorInit(color);
        VList::const_iterator it;
        for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled() ){
                continue;
            }
            else {
                qDebug() << "Changing the label color of all, set vertex"
                         << v1
                         << "new label color"
                         << color;
                (*it)->setLabelColor(color);
                emit setNodeLabelColor( (*it)->number(), color);
            }
        }
    }
    setModStatus(ModStatus::MinorOptions);


}


/**
 * @brief Graph::vertexLabelColorInit
 * Changes the default vertex label color
 * @param color
 */
void Graph::vertexLabelColorInit(QString color){
    initVertexLabelColor=color;
}




/**
 * @brief Changes the distance.of vertex v label from the vertex
 * @param v
 * @param size
 */
void Graph::vertexLabelDistanceSet(const int &v, const int &newDistance) {
    m_graph[ vpos[v] ]->setLabelDistance (newDistance);

    setModStatus(ModStatus::MinorOptions);
    emit setNodeLabelDistance(v, newDistance);
}


/**
 * @brief Changes the distance of all vertex labels from their vertices
 * @param size
 */
void Graph::vertexLabelDistanceAllSet(const int &newDistance) {
    qDebug() << "Changing the label distance of all vertices to:" << newDistance;
    vertexLabelDistanceInit(newDistance);
    VList::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ){
            continue;
        }
        else {
            qDebug() << "vertex" << (*it)->number()
                     << " new label distance:" << newDistance;
            (*it)->setLabelDistance(newDistance) ;
            emit setNodeLabelDistance ( (*it)->number(), newDistance);
        }
    }

    setModStatus(ModStatus::MinorOptions);
}




/**
 * @brief Changes the default distance of vertex labels
 * @param distance
 */
void Graph::vertexLabelDistanceInit(const int &distance) {
    initVertexLabelDistance = distance;
}


/**
 * @brief Sets custom attributes for a specified vertex.
 *
 * This function assigns a set of custom attributes to a vertex identified by its index.
 * It also updates the modification status to indicate that vertex metadata has been changed.
 *
 * @param v1 The index of the vertex for which custom attributes are being set.
 * @param customAttributes A QHash containing the custom attributes to be set for the vertex.
 *                         The keys and values of the QHash are both QStrings.
 */
void Graph::vertexCustomAttributesSet( const int &v1, const QHash<QString, QString> &customAttributes) {
    m_graph[ vpos[v1] ]->setCustomAttributes(customAttributes);
    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Calls the customAttributes method for a specific vertex in the graph.
 * 
 * This function retrieves the vertex at the position specified by the index `v1`
 * from the `vpos` map and calls its `customAttributes` method.
 * 
 * @param v1 The index of the vertex whose custom attributes are to be accessed.
 */
QHash<QString,QString> Graph::vertexCustomAttributes(const int &v1) const {
    return m_graph[ vpos[v1] ]->customAttributes();
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
                       const bool &signalMW){


    // check whether there is already such an edge
    // (see #713617 - https://bugs.launchpad.net/socnetv/+bug/713617)

    if (edgeExists(v1,v2)){
//        qDebug() << "-- Edge " << v1 << "->" << v2
//                    << " declared previously (already exists) - nothing to do \n\n";

        return false;

    }

    if ( type == EdgeType::Undirected ) {

//        qDebug() <<"-- Creating new UNDIRECTED edge:" << v1 << "-" << v2
//                 << "weight" << weight
//                 << "type" << type
//                 << "label" << label
//                 << "signalMW" << signalMW
//                 << "Signaling to GW...";

        edgeAdd ( v1, v2,
                  weight,
                  type,
                  label,
                  ( (weight==0) ? "blue" :  color  )
                  );


        emit signalDrawEdge(v1, v2, weight, label, ( (weight==0) ? "blue" :  color  ), type,
                            drawArrows, bezier, initEdgeWeightNumbers);

    }
    else if ( edgeExists( v2, v1 ) )  {

//        qDebug() <<"-- Creating new RECIPROCAL edge:" << v1 << "->" << v2
//                 << "weight" << weight
//                 << "type" << type
//                 << "label" << label
//                 << "signalMW" << signalMW
//                 << "Reverse edge exists"
//                 << "Signaling to GW...";

        edgeAdd ( v1,
                  v2,
                  weight,
                  EdgeType::Reciprocated ,
                  label,
                  color);


        emit signalDrawEdge(v1, v2, weight, label, color, EdgeType::Reciprocated,
                            drawArrows, bezier, initEdgeWeightNumbers);
        m_graphIsDirected = true;

    }
    else {

//        qDebug() <<"-- Creating new DIRECTED edge:" << v1 << "->" << v2
//                 << "weight" << weight
//                 << "type" << type
//                 << "label" << label
//                 << "signalMW" << signalMW
//                 << "Signaling to GW...";

        edgeAdd ( v1,
                  v2,
                  weight,
                  EdgeType::Directed,
                  label,
                  ( (weight==0) ? "blue" :  color  )
                  );

        emit signalDrawEdge(v1, v2, weight, label, ( (weight==0) ? "blue" :  color  ), EdgeType::Directed,
                            drawArrows, bezier, initEdgeWeightNumbers);

        m_graphIsDirected = true;
        m_graphIsSymmetric=false;

    }

    // save the edge color so that new edges created when user clicks on the canvas
    // have the same color with those of the file loaded,
    initEdgeColor=color;

    setModStatus(ModStatus::EdgeCount, signalMW);

    return true;
}





/**
 * @brief Called from WebCrawler when it finds an new link
 * Calls edgeCreate() method with initEdgeColor
 * @param source
 * @param target
 */
void Graph::edgeCreateWebCrawler (const int &source, const int &target){
//    qDebug()<< " will create edge from" << source << "to" << target ;
    qreal weight = 1.0;
    bool drawArrows=true;
    bool bezier=false;

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
void Graph::edgeAdd (const int &v1, const int &v2,
                     const qreal &weight,
                     const int &type,
                     const QString &label,
                     const QString &color) {

    int source=vpos[v1];
    int target=vpos[v2];

//    qDebug()<< "Adding new edge from vertex "<< v1 << "["<< source
//            << "] to vertex "<< v2 << "["<< target << "] of weight "<<weight
//            << " and label " << label;

    m_graph [ source ]->addOutEdge(v2, weight, color, label );
    m_graph [ target ]->addInEdge(v1, weight);

    if ( weight != 1 && weight!=0) {
        setWeighted(true);
    }
    if (type == EdgeType::Reciprocated ){
        // make existing reverse edge reciprocal

    }
    else if (type == EdgeType::Undirected){
        // edge undirected, add reverse edge too.
        m_graph [ target ]->addOutEdge(v1, weight );
        m_graph [ source ]->addInEdge(v2, weight);
    }

}


/**
 * @brief Toggles the status of outbound edge source -> target at source vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeOutboundStatusSet(const int &source, const int &target, const bool &toggle) {

    m_graph [ vpos[source] ]->setOutEdgeEnabled(target, toggle);
}



/**
 * @brief Toggles the status of inbound edge target <- source at target vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeInboundStatusSet(const int &target, const int &source, const bool &toggle){

    m_graph [ vpos[target] ]->setInEdgeEnabled(source, toggle);

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
void Graph::edgeRemove (const int &v1,
                        const int &v2,
                        const bool &removeReverse) {
    qDebug ()<< "Graph::edgeRemove() - edge" << v1 << "[" << vpos[v1]
                << "] -->" << v2 << " to be removed. removeReverse:" <<removeReverse;
    m_graph [ vpos[v1] ]->removeOutEdge(v2);
    m_graph [ vpos[v2] ]->removeInEdge(v1);

    if ( isUndirected() || removeReverse ) { // remove reverse edge too
        m_graph [ vpos[v2] ]->removeOutEdge(v1);
        m_graph [ vpos[v1] ]->removeInEdge(v2);
        m_graphIsSymmetric=true;
    }
    else {
        if ( edgeExists(v2,v1) !=0 ) {
            m_graphIsSymmetric=false;
        }
    }

    emit signalRemoveEdge(v1,v2, ( isDirected() || removeReverse ));

    setModStatus(ModStatus::EdgeCount);
}


/**
 * @brief Removes a SelectedEdge
 * @param selectedEdge
 * @param removeReverse
 */
void Graph::edgeRemoveSelected (SelectedEdge &selectedEdge,
                                const bool &removeReverse){
    qDebug()<< "Graph::edgeRemoveSelected()" << selectedEdge;
    edgeRemove( selectedEdge.first, selectedEdge.second, removeReverse);
}


/**
 * @brief Removes all selected edges
 */
void Graph::edgeRemoveSelectedAll() {
    qDebug()<< "Graph::edgeRemoveSelectedAll()";

    foreach (SelectedEdge edgeToRemove, getSelectedEdges()) {
        qDebug() << "Graph::edgeRemoveSelectedAll() - About to remove" << edgeToRemove;
        edgeRemoveSelected( edgeToRemove, true );
    }
}




/**
 * @brief Filters (disables) edges according the specified threshold weight
 *
 *
 * @param m_threshold
 * @param overThreshold
 */
void Graph::edgeFilterByWeight(const qreal m_threshold, const bool overThreshold){
    QString words;

    if (overThreshold) {
        qDebug() << "filtering edges with weight over or equal" << m_threshold ;
        words = "equal or over";
    }
    else{
        qDebug() << "Filtering edges with weight below or equal" << m_threshold ;
        words = "equal or under";
    }

    VList::const_iterator it;

    int source, target=0;
    qreal weight=0, reverseEdgeWeight=0;
    bool preserveReverseEdge=false;
    H_edges::const_iterator  ed;

    // Loop over all vertices
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        source = (*it)->number();

//        if (overThreshold) {
//            qDebug() << "vertex:" << source
//                     << "outedges:" << (*it)->outEdgesEnabledHash()
//                     << "disabling outedges with weights >=" << m_threshold;
//        }
//        else {
//            qDebug() << "vertex:" << source
//                     << "outedges:" << (*it)->outEdgesEnabledHash()
//                     << "disabling outedges with weights <=" << m_threshold;
//        }

        // Loop over all out edges of source
        for ( ed = (*it)->m_outEdges.cbegin(); ed != (*it)->m_outEdges.cend(); ++ed) {

            // Init preserve reserve edge status to false
            preserveReverseEdge = false;

            if ( ed.value().first != m_curRelation ) {
                // This edge does not belong to this relation
                continue;
            }

            target=ed.key();
            weight = ed.value().second.first;

            // Check the filtering type: over or under
            if (overThreshold) {
                // We should disable all edges with weights >= threshold
                if ( weight >= m_threshold ) {
                    // this outedge must be disabled - check reverse edge
                    reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                    if ( reverseEdgeWeight != 0 && reverseEdgeWeight < m_threshold ) {
                        // reverse edge exists and doesn't match. It must be preserved.
                        preserveReverseEdge = true;
                    }
//                    qDebug() << source << "->" << target << "weight:" << weight << "will be disabled - preserveReverseEdge:" << preserveReverseEdge << ". Emitting signal...";
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false) );
                    // Disable the inedge of the target vertex too (needed for inDegree)
//                    qDebug() << "disabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility (m_curRelation, source, target, false, preserveReverseEdge, weight, reverseEdgeWeight);

                }
                else {
//                    qDebug() << source << "->" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true) );
                    // Enable the inedge of the target vertex too (needed for inDegree)
//                    qDebug() << "enabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, true);
                    emit signalSetEdgeVisibility (m_curRelation, source, target, true, preserveReverseEdge);
                }

            }
            else {
                // We should disable all edges <= the threshold
                 if ( weight <= m_threshold ) {
                     // this outedge must be disabled - check reverse edge
                     reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                     if ( reverseEdgeWeight !=0 && reverseEdgeWeight > m_threshold ) {
                         // reverse edge exists and doesn't match. It must be preserved.
                         preserveReverseEdge = true;
                     }
//                    qDebug() << source << "->" << target << "weight:" << weight << "will be disabled - preserveReverseEdge:" << preserveReverseEdge << ". Emitting signal...";
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false) );
                    // Disable the inedge of the target vertex too (needed for inDegree)
//                    qDebug() << "disabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility (m_curRelation, source, target, false,preserveReverseEdge, weight, reverseEdgeWeight);

                }
                else {
//                    qDebug() << source << "->" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true) );
                    // Enable the inedge of the target vertex too (needed for inDegree)
//                    qDebug() << "enabling the inedge of the target vertex: " << target << "<-" << source;
                    this->edgeInboundStatusSet(target, source, true);

                    emit signalSetEdgeVisibility (m_curRelation, source, target, true, preserveReverseEdge);

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
void Graph::edgeFilterByRelation(int relation, bool status){
    qDebug() << "toggling all edges in relation" << relation << "to status" << status ;
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ){
            // Skip if the node is disabled.
            continue;
        }
        (*it)->setEnabledEdgesByRelation( relation, status );
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
void Graph::edgeFilterUnilateral(const bool &toggle) {
    qDebug() << "Toggling unilateral edges:" << toggle ;
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        (*it)->setEnabledUnilateralEdges ( toggle );
    }
    setModStatus(ModStatus::EdgeCount);
    emit statusMessage(tr("Unilateral edges have been temporarily disabled."));
}




/**
 * @brief Sets the clicked edge
 *
 * Parameters are the source and target node of the edge.
 * It emits signal to MW, which displays a relevant  message on the status bar.
 *
 * @param v1
 * @param v2
 */
void Graph::edgeClickedSet(const int &v1, const int &v2, const bool &openMenu) {

    m_clickedEdge.source=v1;
    m_clickedEdge.target=v2;

    if (m_clickedEdge.source == 0 && m_clickedEdge.target==0) {
        emit signalEdgeClicked();
        return;
    }
    qreal weight = m_graph[ vpos[ m_clickedEdge.source] ]->hasEdgeTo(m_clickedEdge.target);
    qDebug() << "Setting clicked edge: "<< v1 << "->" << v2 << "weight:" << weight;

    int type=EdgeType::Directed;
    // Check if the reverse tie exists. If yes, this is a reciprocated edge
    qreal oppositeWeight = edgeExists(m_clickedEdge.target, m_clickedEdge.source, false);
    if ( oppositeWeight ) {
        qDebug() << "Reverse tie"<< v2 << "->" << v2 << "exists. Weight:" << oppositeWeight;
        if ( !isDirected() ) {
            type=EdgeType::Undirected;
        }
        else {
            type=EdgeType::Reciprocated;
        }
    }
    m_clickedEdge.type = type;
    m_clickedEdge.weight = weight;
    m_clickedEdge.rWeight = oppositeWeight;

    emit signalEdgeClicked( m_clickedEdge, openMenu);

}


/**
 * @brief Returns clicked edge
 * @return
 */
MyEdge Graph::edgeClicked() {
    return m_clickedEdge;
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
qreal Graph::edgeExists (const int &v1, const int &v2, const bool &checkReciprocal) {

    edgeWeightTemp = m_graph[ vpos[v1] ]->hasEdgeTo(v2);
//    qDebug() << "Checking if edge exists:" << v1 << "->" << v2 << "=" << edgeWeightTemp  ;

    if (!checkReciprocal){
        return edgeWeightTemp;
    }
    else if (edgeWeightTemp!=0) {
        edgeReverseWeightTemp = m_graph[ vpos[v2] ]->hasEdgeTo(v1);
//        qDebug() << "Checking if reverse edge exists: " << v2 << "->" << v1 << "=" << edgeWeightTemp  ;
        if  ( edgeWeightTemp == edgeReverseWeightTemp  ){
            return edgeWeightTemp;
        }
    }
    return  0;
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
qreal Graph::edgeExistsVirtual (const int &v1, const int &v2) {

    qreal m_weight=RAND_MAX;
    bool edgeStatus=false;
    H_edges::const_iterator it1;
    GraphVertex *source = m_graph[ vpos[v1] ];
    H_edges source_outEdges = source ->m_outEdges;

    it1= source_outEdges.constFind(v2);
    while (it1 != source_outEdges.constEnd() && it1.key() == v2 ) {
        if ( it1.value().first == m_curRelation  ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
            }
        }
        ++it1;
    }

    return  m_weight;
}

/**
 * @brief Returns TRUE if edge(v1, v2) is symmetric, i.e. (v1,v2) == (v2,v1).
 * @param v1
 * @param v2
 * @return
 */
bool Graph::edgeSymmetric(const int &v1, const int &v2){
    if ( ( edgeExists( v1, v2 , true) ) !=0 ) {
        qDebug() << "Edge" << v1 << "->" << v2 << "is symmetric";
        return true;
    }
    else {
        qDebug() << "Edge" << v1 << "->" << v2 << "is not symmetric";
        return false;
    }

}


/**
 * @brief Returns the number |E| of graph - only the enabled edges
 *
 * @return
 */
int Graph::edgesEnabled() {

    int enabledEdges = 0;
    if ( calculatedEdges ) {
        enabledEdges = (( isUndirected() ) ? m_totalEdges / 2 : m_totalEdges);
//        qDebug()<< "Graph unchanged. Returning current enabled edges count:" <<  enabledEdges;
        return enabledEdges;
    }
    // Compute the edge count from scratch
    m_totalEdges = 0;
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        m_totalEdges+=(*it)->outEdgesCount();
    }
    calculatedEdges = true;
    enabledEdges = (( isUndirected() ) ? m_totalEdges / 2 : m_totalEdges);
//    qDebug()<< "Computed enabled edges new count:" <<  enabledEdges;
    return enabledEdges;
}




/**
 * @brief Returns the number of outbound edges (arcs) from vertex v1
 * @param v1
 * @return
 */
int Graph::vertexEdgesOutbound(int v1) {
    qDebug("Graph: vertexEdgesOutbound()");
    return m_graph[ vpos[v1] ]->outEdgesCount();
}


/**
 * @brief Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::vertexEdgesInbound (int v1) {
    qDebug("Graph: vertexEdgesInbound()");
    return m_graph[ vpos[v1] ]->inEdgesCount();
}




/**
 * @brief Changes the weight of the edge from vertex v1 to v2 (and optionally of the reverse edge)
 *
 * @param v1
 * @param v2
 * @param weight
 * @param undirected
 */
void Graph::edgeWeightSet (const int &v1, const int &v2,
                           const qreal &weight, const bool &undirected) {
    qDebug() << "Changing the weight of edge" << v1 << "[" << vpos[v1]
                << "]->" << v2 << "[" << vpos[v2] << "]" << " to new weight " << weight;
    m_graph [ vpos[v1] ]->setOutEdgeWeight(v2, weight);
    if (undirected) {
        qDebug() << "Changing the weight of the reverse edge too";
        m_graph [ vpos[v2] ]->setOutEdgeWeight(v1, weight);
    }
    emit setEdgeWeight(v1,v2, weight);
    setModStatus(ModStatus::EdgeCount);
}





/**
 * @brief Returns the weight of the edge v1->v2
 * @param v1
 * @param v2
 * @return qreal
 */
qreal Graph::edgeWeight (const int &v1, const int &v2) const{
    return m_graph[ vpos[v1] ]->hasEdgeTo(v2);
}




/**
 * @brief Changes the visibility of edge weight numbers
 * @param toggle
 */
void Graph::edgeWeightNumbersVisibilitySet (const bool &toggle) {
    initEdgeWeightNumbers = toggle;
}



/**
 * @brief Saves the default edge color
 * Used by random network creation methods
 * @param color
 */
void Graph::edgeColorInit(const QString &color){
    initEdgeColor=color;
}





/**
 * @brief Changes the color of all enabled edges.
 * @param color
 * @return
 */
bool Graph::edgeColorAllSet(const QString &color, const int &threshold){
    qDebug()<< "Graph::edgeColorAllSet() - new color: " << color;
    int target=0, source=0;
    edgeColorInit(color);
    QHash<int,qreal> enabledOutEdges;
    QHash<int,qreal>::const_iterator it1;
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        source = (*it)->number();
        if ( ! (*it)->isEnabled() )
            continue;
        enabledOutEdges=(*it)->outEdgesEnabledHash();
        it1=enabledOutEdges.cbegin();
        while ( it1!=enabledOutEdges.cend() ){
            target = it1.key();
            if (threshold == 0 ){
                if ( it1.value() == threshold  ) {
                    qDebug() << " Graph::edgeColorAllSet() zero weight threshold "
                             << threshold << " - edge "
                                << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else if (threshold != 0 && threshold != RAND_MAX ) {
                if ( it1.value() <= threshold ) {
                    qDebug() << " Graph::edgeColorAllSet() below weight threshold "
                             << threshold << " - edge "
                                << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else {
                qDebug() << " Graph::edgeColorAllSet() : "
                            << source << "->" << target << " new color " << color;
                (*it)->setOutLinkColor(target, color);
                emit setEdgeColor(source, target, color);

            }
            ++it1;
        }
    }
    //delete enabledOutEdges;

    setModStatus(ModStatus::EdgeMetadata);

    return true;

}




/**
 * @brief Changes the color of edge v1->v2
 * @param v1
 * @param v2
 * @param color
 */
void Graph::edgeColorSet(const int &v1, const int &v2, const QString &color){
    qDebug()<< "Graph::edgeColorSet() - "<< v1 << "->"<< v2
            <<" vpos ("<< vpos[v1]<< "->"<<vpos[v2]<<")"
           <<" new color "<< color;
    m_graph[ vpos[v1] ]->setOutLinkColor(v2, color);
    emit setEdgeColor(v1, v2, color);
    if ( isSymmetric() ) {
        m_graph[ vpos[v2] ]->setOutLinkColor(v1, color);
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
QString Graph::edgeColor (const int &v1, const int &v2){
    return m_graph[ vpos[v1] ]->outLinkColor(v2);
}




/**
 * @brief Changes the label of edge v1->v2
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeLabelSet (const int &v1, const int &v2, const QString &label) {
    qDebug() << "Graph::edgeLabelSet()  " << v1 << "[" << vpos[v1]
                << "]->" << v2 << "[" << vpos[v2] << "]" << " label " << label;
    m_graph[ vpos[v1] ]->setOutEdgeLabel(v2, label);

    emit setEdgeLabel(v1,v2, label);

    setModStatus(ModStatus::EdgeMetadata);
}


/**
 * @brief Returns the label of edge v1->v2
 * @param v1
 * @param v2
 * @return
 */
QString Graph::edgeLabel (const int &v1, const int &v2) const {
    return m_graph [ vpos[v1] ]->outEdgeLabel(v2);
}


/**
 * @brief Toggles the visibility of edge labels.
 * @param toggle
 */
void Graph::edgeLabelsVisibilitySet (const bool &toggle) {
    initEdgeLabels = toggle;
}



/**
 * @brief Returns the outDegree (sum of outbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeOut (int v1) {
    qDebug() << "Returning outDegree of " << v1;
    return m_graph[ vpos[v1] ]->degreeOut();
}



/**
 * @brief Returns the inDegree (sum of inbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeIn (int v1) {
    qDebug() << "Returning inDegree of " << v1;
    return m_graph[ vpos[v1] ]->degreeIn();
}

/**
 * @brief Returns a list of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QList<int> Graph::vertexNeighborhoodList(const int &v1) {
    //qDebug() << "Returning the neighborhood list of " << v1;
    return m_graph[ vpos[v1] ]->neighborhoodList();
}


/**
 * @brief Returns the set of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QSet<int> Graph::vertexNeighborhoodSet(const int &v1) {
    //qDebug()<< "Graph::vertexNeighborhoodList()";
    QList<int> myNeightbors = m_graph[ vpos[v1] ]->neighborhoodList();
    return QSet<int>(myNeightbors.constBegin(),myNeightbors.constEnd());
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
int Graph::vertices(const bool &dropIsolates, const bool &countAll, const bool &recount) {

    if ( m_totalVertices!=0 && calculatedVertices && !recount) {
        qDebug() << "Graph not modified, returning static number: "
                   << m_totalVertices;
        return m_totalVertices;
    }
    m_totalVertices=0;
    VList::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (countAll) {
            ++m_totalVertices;
        }
        else {
            if (dropIsolates && (*it)->isIsolated()){
                qDebug()<< "Skipping isolated vertex:" <<(*it)->number();
                continue;
            }
            if ( !(*it)->isEnabled()) {
                qDebug()<< "Skipping disabled vertex:" <<(*it)->number();
                continue;
            }
            ++m_totalVertices;
        }
    }
    qDebug()<< "Graph size:"<< m_graph.size() << "vertices" << m_totalVertices;
    calculatedVertices=true;
    return m_totalVertices;
}



/**
 * @brief Returns a list of all isolated vertices inside the graph
 *
 * @return QList<int>
 */
QList<int> Graph::verticesListIsolated(){
    if ( calculatedIsolates ){
        qDebug()<< "Graph::verticesListIsolated() - graph not modified and "
                   "already calculated isolates. Returning list as is:"
                <<m_verticesIsolatedList;
        return m_verticesIsolatedList;
    }

    VList::const_iterator it;
    m_verticesIsolatedList.clear();
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        //        if ( ! (*it)->isEnabled() )
        //            continue;
        if ((*it)->isIsolated()) {
            m_verticesIsolatedList << (*it)->number();
            qDebug()<< "Graph::verticesListIsolated() - node " << (*it)->number()
                    << " is isolated. Marking it." ;
        }
    }
    qDebug()<< "Graph::verticesListIsolated() - isolated vertices list:"
            <<m_verticesIsolatedList;
    calculatedIsolates = true;
    return m_verticesIsolatedList ;
}



/**
 * @brief Returns a list of all vertices numbers inside the graph
 *
 * @return QList<int>
 */
QList<int> Graph::verticesList(){
    qDebug()<< "Graph::verticesList()";
    if ( !m_verticesList.isEmpty() && calculatedVerticesList ){
        return m_verticesList;
    }
    VList::const_iterator it;
    m_verticesList.clear();
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() )
            continue;
        m_verticesList << (*it)->number();
    }
    calculatedVerticesList = true;
    return m_verticesList ;
}



/**
 * @brief Returns a QSet of all vertices numbers inside the graph
 * @return
 */
QSet<int> Graph::verticesSet(){
    qDebug()<< "Graph::verticesSet()";
    if ( !m_verticesSet.isEmpty() && calculatedVerticesSet ){
        return m_verticesSet;
    }
    VList::const_iterator it;
    m_verticesSet.clear();
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() )
            continue;
        m_verticesSet << (*it)->number();
    }
    calculatedVerticesSet = true;
    return m_verticesSet ;
}





/**
 * @brief Creates a subgraph (clique, star, cycle, line) with vertices in vList
 * Iff vList is empty, then fallbacks to the m_verticesSelected.
 * @param vList
 */
void Graph::verticesCreateSubgraph(QList<int> vList,
                                   const int &type,
                                   const int &center) {

    if ( relations() == 1 && edgesEnabled()==0 ) {
        QString newRelationName = QString::number ( vList.size() ) + tr("-clique");
        relationCurrentRename(newRelationName, true);
    }

    if (vList.isEmpty()) {
        vList = m_verticesSelected;
    }

    qDebug()<<"Graph::verticesCreateSubgraph() - type:" << type
           << "vList:" << vList;

    int progressCounter = 0;
    QString pMsg = tr("Creating subgraph. \nPlease wait...");
    emit statusMessage( pMsg);
    emit signalProgressBoxCreate(vList.size(),pMsg);


    qreal weight;

    bool drawArrows = isDirected();
    int edgeType = ( isUndirected() ) ?  EdgeType::Undirected : EdgeType::Reciprocated;

    if (type == SUBGRAPH_CLIQUE) {

        for (int i=0; i < vList.size(); ++i ) {

            emit signalProgressBoxUpdate(++progressCounter);

            for (int j=i+1; j < vList.size(); ++j ) {

                if ( ! (weight=edgeExists( vList.value(i), vList.value(j) ) ) ) {

                    if ( (weight=edgeExists( vList.value(j), vList.value(i) ) ) ) {
                        edgeTypeSet( vList.value(j), vList.value(i), weight, edgeType );
                    }
                    else {
                        edgeCreate(vList.value(i),
                                   vList.value(j),
                                   1.0,
                                   initEdgeColor,
                                   EdgeType::Undirected,
                                   drawArrows);
                        edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
                    }

                }
                else {
                    edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
                }
            }
        }


    }
    else if (type == SUBGRAPH_STAR)  {

        for (int j=0; j < vList.size(); ++j ) {

            emit signalProgressBoxUpdate(++progressCounter);

            if ( ! (weight=edgeExists( center, vList.value(j) ) ) ) {
                if ( center == vList.value(j))
                    continue;

                if ( (weight=edgeExists( vList.value(j), center ) ) ) {
                    edgeTypeSet( vList.value(j), center, weight, edgeType );
                }
                else {
                    edgeCreate(center,
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet( center, vList.value(j), weight, edgeType );
                }

            }
            else {
                edgeTypeSet( center, vList.value(j), weight, edgeType );
            }

        }
    }
    else if (type == SUBGRAPH_CYCLE)  {
        int j=0;
        for (int i=0; i < vList.size(); ++i ) {

            emit signalProgressBoxUpdate(++progressCounter);

            j= ( i == vList.size()-1) ? 0:i+1;
            if ( ! (weight=edgeExists( vList.value(i), vList.value(j) ) ) ) {

                if ( (weight=edgeExists( vList.value(j), vList.value(i) ) ) ) {
                    edgeTypeSet( vList.value(j), vList.value(i), weight, edgeType );
                }
                else {
                    edgeCreate(vList.value(i),
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
                }


            }
            else {
                edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
            }

        }


    }
    else if (type == SUBGRAPH_LINE)  {
        int j=0;
        for (int i=0; i < vList.size(); ++i ) {

            emit signalProgressBoxUpdate(++progressCounter);

            if ( i == vList.size()-1 ) break;
            j= i+1;
            if ( ! (weight=edgeExists( vList.value(i), vList.value(j) ) ) ) {

                if ( (weight=edgeExists( vList.value(j), vList.value(i) ) ) ) {
                    edgeTypeSet( vList.value(j), vList.value(i), weight, edgeType );
                }
                else {
                    edgeCreate(vList.value(i),
                               vList.value(j),
                               1.0,
                               initEdgeColor,
                               EdgeType::Undirected,
                               drawArrows);
                    edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
                }


            }
            else {
                edgeTypeSet( vList.value(i), vList.value(j), weight, edgeType );
            }

        }

    }
    else {
        emit signalProgressBoxKill();
        return;
    }
    emit signalProgressBoxKill();

}



/**
 * @brief Returns true if the current graph has no vertices at all
 */
bool Graph::isEmpty() const{
    return m_graph.isEmpty();
}



/**
 * @brief Resets the clicked edge and node
 *
 * Usually, called when the user clicks on an empty space.
 *
 * @param p
 */
void Graph::graphClickedEmptySpace( const QPointF &p) {
    qDebug() << "Click on empty space at" << p << " - resetting clicked edge and node...";
    // Reset clicked vertices
    this->vertexClickedSet(0, p);
    // Reset clicked edges
    this->edgeClickedSet(0,0);

}


/**
 * @brief Sets the user-selected vertices and edges
 *
 * Usually called from GW, it emits selection counts to MW
 *
 * @param selectedVertices
 * @param selectedEdges
 */
void Graph::setSelectionChanged(const QList<int> selectedVertices,
                                  const QList<SelectedEdge> selectedEdges) {

    m_verticesSelected = selectedVertices;
    m_selectedEdges = selectedEdges;

    qDebug() << "Selection changed. Vertices" << m_verticesSelected << "Edges" << m_selectedEdges << "Emitting to MW...";

    emit signalSelectionChanged(m_verticesSelected.size(), m_selectedEdges.size());

}


/**
 * @brief Returns a QList of user-selected vertices
 * @return
 */
QList<int> Graph::getSelectedVertices() const{
    return m_verticesSelected;
}


/**
 * @brief Returns count of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesCount() const{
    return m_verticesSelected.size();
}


/**
 * @brief Returns min of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesMin() const{
    int min = RAND_MAX;
    foreach (int i, m_verticesSelected) {
        if (i < min) min = i;
    }
    return min;
}


/**
 * @brief Returns max of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesMax() const{
    int max = 0;
    foreach (int i, m_verticesSelected) {
        if (i > max ) max = i;
    }
    return max;
}



/**
 * @brief Returns a QList of user-selected edges in pair<int,int>
 * @return
 */
QList<SelectedEdge> Graph::getSelectedEdges() const{
    return m_selectedEdges;
}


/**
 * @brief Returns the count of user-selected edges
 * @return
 */
int Graph::getSelectedEdgesCount() const {
    return m_selectedEdges.size();
}




/**
 * @brief Gets the graph density (if computed) or computes it again.
 *
 * The graph density is the ratio of present edges to total possible edges
 * in the current relation.
 *
 * @return qreal
 */
qreal Graph::graphDensity() {

    if ( calculatedGraphDensity ) {
//        qDebug()<< "Graph not changed and density already computed:"
//                << m_graphDensity;
        return m_graphDensity;
    }

//    qDebug()<< "Computing graph density...";
    int V=vertices();
    if (V!=0 && V!=1) {
        int enabledEdges = edgesEnabled();
        m_graphDensity = (isUndirected()) ?
                    (qreal) 2 * enabledEdges  / (qreal)(V*(V-1.0)) :
                    (qreal) enabledEdges  / (qreal)(V*(V-1.0)) ;
    }
    else {
        m_graphDensity = 0;
    }
    calculatedGraphDensity = true;
    return m_graphDensity ;
}



/**
 * @brief Returns true if the graph is weighted (valued),
 * i.e. if any e in |E| has value not 0 or 1
 *  Complexity: O(n^2)
 * @return
 */
bool Graph::isWeighted(){

    if ( calculatedGraphWeighted ) {
        qDebug()<< "graph not modified. Returning isWeighted: "
                << m_graphIsWeighted;
        return m_graphIsWeighted;
    }

    qreal m_weight=0;
    VList::const_iterator it, it1;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Checking if the graph edges are valued. \nPlease wait...");
    emit statusMessage( pMsg);
    emit signalProgressBoxCreate(N,pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            m_weight = edgeExists ( (*it1)->number(), (*it)->number() ) ;
            if ( m_weight  != 1  && m_weight  != 0 )   {
                setWeighted(true);
                break;
            }
        }
        if (m_graphIsWeighted) {
            break;
        }

    }
    calculatedGraphWeighted = true;
    qDebug()<< "graph is weighted:" << m_graphIsWeighted;

    emit signalProgressBoxKill();

    return m_graphIsWeighted;
}


/**
 * @brief Sets the graph to be weighted ( valued edges ).
 * @param toggle
 */
void Graph::setWeighted(const bool &toggle){
    m_graphIsWeighted = toggle;
}

/**
 * @brief Returns the sum of vertices having edgesOutbound
 * @return
 */
int Graph::verticesWithOutboundEdges(){
    return outboundEdgesVert;
}


/**
 * @brief Returns the sum of vertices having edgesInbound
 * @return
 */
int Graph::verticesWithInboundEdges(){
    return inboundEdgesVert;
}


/**
 * @brief Returns the sum of vertices having reciprocal edges
 * @return
 */
int Graph:: verticesWithReciprocalEdges(){
    return reciprocalEdgesVert;
}





/**
 * @brief called from Graph, when closing network, to terminate all crawler processes
* Also called indirectly when wc_spider finishes
 * @param reason
 */
void Graph::webCrawlTerminateThreads (QString reason){
    qDebug() << "Terminating webCrawler threads - reason " << reason
             << "Checking webcrawlerThread...";

    while (webcrawlerThread.isRunning() ) {
        qDebug() << "webcrawlerThread running. "
                    "Calling webcrawlerThread.quit()";
        webcrawlerThread.requestInterruption();
        webcrawlerThread.quit();
        webcrawlerThread.wait();
    }
}






/**
 * @brief
 * Creates a new WebCrawler, that will parse the downloaded HTML code of each webpage
 * we download. Moves the WebCrawler to a new thread and starts the thread.
 * Then creates the fist node (initial url),
 * and starts the web spider to download the first HTML page.
 * Called by MW with user options.
 * @param startUrl
 * @param urlPatternsIncluded
 * @param urlPatternsExcluded
 * @param linkClasses
 * @param maxNodes
 * @param maxLinksPerPage
 * @param intLinks
 * @param childLinks
 * @param parentLinks
 * @param selfLinks
 * @param extLinksIncluded
 * @param extLinksCrawl
 * @param socialLinks
 * @param delayedRequests
 */
void Graph::startWebCrawler(
        const QUrl &startUrl,
        const QStringList &urlPatternsIncluded,
        const QStringList &urlPatternsExcluded,
        const QStringList &linkClasses,
        const int &maxNodes,
        const int &maxLinksPerPage,
        const bool &intLinks,
        const bool &childLinks,
        const bool &parentLinks,
        const bool &selfLinks,
        const bool &extLinksIncluded,
        const bool &extLinksCrawl,
        const bool &socialLinks,
        const bool &delayedRequests){

    qDebug() << "Setting up a new WebCrawler for url:" << startUrl.toString()
             << "graph thread:" << thread();

    // Rename current relation
    relationCurrentRename(tr("web"), true);

    // Initialize variables
    m_crawler_max_urls = maxNodes;                      // Store maximum urls we'll visit (max nodes in the resulted network)
    m_crawler_visited_urls = 0;                         // Init counter of visited urls

    // Check if we need to add delay between requests
    int delayBetween = 0;
    if ( delayedRequests ) {
        delayBetween = 500;         // half second
    }

    // Create our url queue
    urlQueue = new QQueue<QUrl>;

    // Enqueue the start QUrl
    urlQueue->enqueue(startUrl);

    qDebug() << "Creating new WebCrawler...";

    // Create the WebCrawler
    web_crawler = new WebCrawler(
                urlQueue,
                startUrl,
                urlPatternsIncluded,
                urlPatternsExcluded,
                linkClasses,
                maxNodes,
                maxLinksPerPage,
                intLinks,
                childLinks,
                parentLinks,
                selfLinks,
                extLinksIncluded,
                extLinksCrawl,
                socialLinks,
                delayBetween
                );

    // Just in case, we reach this place and the thread is still running
    if (webcrawlerThread.isRunning()) {
        qDebug() << "webcrawlerThread is already running - calling requestInterruption()...";
        webCrawlTerminateThreads("startWebCrawler() to start a new WebCrawler but webcrawlerThread is running...");
    }

    // Move the crawler to another thread
    web_crawler->moveToThread(&webcrawlerThread);

    qDebug() << "WebCrawler created and moved to its own thread:"
             << web_crawler->thread();

    // Connect signals and slots
    qDebug() << "Connect signals/slots with WebCrawler...";
    connect(this, &Graph::signalWebCrawlParse,
            web_crawler, &WebCrawler::parse);

    connect(web_crawler, &WebCrawler::signalStartSpider,
            this, &Graph::webSpider);

    connect(web_crawler, &WebCrawler::signalCreateNode,
            this, &Graph::vertexCreateAtPosRandomWithLabel);

    connect(web_crawler, &WebCrawler::signalCreateEdge,
            this, &Graph::edgeCreateWebCrawler);

    connect (web_crawler, &WebCrawler::finished,
             this, &Graph::webCrawlTerminateThreads);

    connect(&webcrawlerThread, &QThread::finished,
            web_crawler, &QObject::deleteLater);


    // Start the crawler thread...
    qDebug() << "Starting WebCrawler thread...";
    webcrawlerThread.start();

    // Create the initial vertex for the starting url
    qDebug() << "Creating initial node 1, initialUrlStr:" << startUrl.toString();
    vertexCreateAtPosRandomWithLabel(1, startUrl.toString(), false);

    // Call the spider to download the html code of the starting url .
    qDebug() << "Calling webSpider()...";
    this->webSpider();

    qDebug("web crawler and spider started. See the thread running? ");
}


/**
 * @brief
 * A loop, that takes urls awaiting in front of the urlQueue,
 * and signals to the MW to make the network request
 */
void Graph::webSpider(){

    //repeat while urlQueue has items
    do {

        //  Until we crawl all urls in urlQueue.
        if ( urlQueue->size() == 0 ) {
            qDebug () <<"webSpider - urlQueue is empty. Break for now... "  ;
            break;
        }

        // or until we have reached m_maxNodes
        if (m_crawler_max_urls > 0 && m_crawler_visited_urls == m_crawler_max_urls) {
            qDebug () << "webSpider - reached m_crawler_max_urls. Break." ;
            break;
        }

        // Take the first url awaiting in the queue
        qDebug() << "webSpider - urlQueue size: " << urlQueue->size()
                 << " - Taking the first url from the urlQueue  ";
        QUrl currentUrl = urlQueue->dequeue();

        qDebug() << "webSpider - url to download: "
                 <<  currentUrl
                  << "Increasing m_crawler_visited_urls to:" << m_crawler_visited_urls + 1
                  << "and emitting signal signalNetworkManagerRequest to MW...";

        // Signal MW to make the network request
        emit signalNetworkManagerRequest(currentUrl, NetworkRequestType::Crawler);

        // increase visited urls counter
        m_crawler_visited_urls++;


    } while ( urlQueue->size() );

}

/**
 * @brief
 * Gets the reply of a MW network request made by Web Crawler,
 * and emits that reply as is to the Web Crawler.
 */
void Graph::slotHandleCrawlerRequestReply(){

    qDebug() << "Got reply from MW network manager request. Emitting signal to Web Crawler to parse the reply...";

    // Get network reply from the sender
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    // Emit signal to web crawler to parse the reply
    emit signalWebCrawlParse(reply);

}



/**
 * @brief Gets the arc reciprocity of the graph.
 *
 * Also computes the dyad reciprocity and fills parameters with values.

 * @return qreal
 */
qreal Graph::graphReciprocity(){

    qDebug()<< "Graph::graphReciprocity()";

    if ( calculatedGraphReciprocity ){
        qDebug() << "Graph::graphReciprocity() - graph not modified and "
                    "already calculated reciprocity. Returning previous result: "
                 << m_graphReciprocityArc;
        return m_graphReciprocityArc;
    }

    qDebug() << "Graph::graphReciprocity() - Computing...";

    emit statusMessage ( (tr("Calculating the Arc Reciprocity of the graph...")) );

    m_graphReciprocityArc=0;
    m_graphReciprocityDyad=0;
    m_graphReciprocityTiesReciprocated=0;
    m_graphReciprocityTiesNonSymmetric=0;
    m_graphReciprocityTiesTotal=0;
    m_graphReciprocityPairsReciprocated=0;
    m_graphReciprocityPairsTotal=0;

    qreal weight = 0, reciprocalWeight = 0;

    int y=0, v2=0, v1=0;

    QHash<int,qreal> enabledOutEdges;

    QHash<int,qreal>::const_iterator hit;
    VList::const_iterator it;

    H_StrToBool totalDyads;
    H_StrToBool reciprocatedDyads;
    QString pair, reversePair;

    //initialize counters
    for ( it = m_graph.cbegin(); it != m_graph.cend(); ++it) {
        (*it)->setOutEdgesReciprocated(0);
        (*it)->setOutEdgesNonSym(0);
        (*it)->setInEdgesNonSym(0);
    }

    // Compute "arc" reciprocity
    //  the number of ties that are involved in reciprocal relations
    //  relative to the total number of actual ties (not possible ties)
    for ( it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();

        if ( ! (*it)->isEnabled() )
            continue;

        enabledOutEdges=(*it)->outEdgesEnabledHash();

        hit=enabledOutEdges.cbegin();

        while ( hit!=enabledOutEdges.cend() ){

            v2 = hit.key();
            y=vpos[ v2 ];
            weight = hit.value();
            m_graphReciprocityTiesTotal += weight;

            // Compute "dyad" reciprocity
            pair = QString::number(v1) + ">" + QString::number(v2) ;
            reversePair = QString::number(v2) + ">" + QString::number(v1) ;
            if ( !totalDyads.contains(pair) && !totalDyads.contains(reversePair) ) {
                totalDyads [pair] = true;
            }


            qDebug() << pair
                     << "totalTies" << m_graphReciprocityTiesTotal
                     << "totalDyads" << totalDyads.size();

            if (  (reciprocalWeight = edgeExists(v2, v1) ) == weight) {

                (*it)->setOutEdgesReciprocated(); //increase reciprocated ties for ego
                (*it)->setOutEdgesReciprocated();

                m_graphReciprocityTiesReciprocated  +=reciprocalWeight  ;

                pair = QString::number(v2) + ">" + QString::number(v1) ;
                reversePair = QString::number(v1) + ">" + QString::number(v2) ;
                if ( !reciprocatedDyads.contains(pair) && !reciprocatedDyads.contains(reversePair) ) {
                    reciprocatedDyads [pair] = true;
                }


                qDebug() << pair << "reciprocal!"
                          << "reciprocatedTies" << m_graphReciprocityTiesReciprocated
                          << "reciprocatedDyads" << reciprocatedDyads.size();

            }
            else {
                (*it)->setOutEdgesNonSym();
                m_graph[y]->setInEdgesNonSym();
                m_graphReciprocityTiesNonSymmetric++;
            }

            ++hit;
        }
    }
    //delete enabledOutEdges;

    m_graphReciprocityArc = (qreal) m_graphReciprocityTiesReciprocated / (qreal) m_graphReciprocityTiesTotal;

    m_graphReciprocityPairsReciprocated = reciprocatedDyads.size();
    m_graphReciprocityPairsTotal = totalDyads.size();

    m_graphReciprocityDyad = (qreal) m_graphReciprocityPairsReciprocated / (qreal) m_graphReciprocityPairsTotal;

    qDebug() << "Graph: graphReciprocity() - Finished. Arc reciprocity:"
             << m_graphReciprocityTiesReciprocated
             << "/"
             << m_graphReciprocityTiesTotal << "="  << m_graphReciprocityArc << "\n"
             << m_graphReciprocityPairsReciprocated
             << "/"
             << m_graphReciprocityPairsTotal << "=" << m_graphReciprocityDyad;



    calculatedGraphReciprocity = true;

    return m_graphReciprocityArc;
}








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

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }

    QTextStream outText ( &file );

    m_graphReciprocityArc = graphReciprocity();

    int rowCount=0;
    int progressCounter=0;
    int N = vertices();
    qreal tiesSym=0;
    qreal tiesNonSym=0;
    qreal tiesOutNonSym=0;
    qreal tiesInNonSym=0;
    qreal tiesOutNonSymTotalOut=0;
    qreal tiesInNonSymTotalIn=0;


    QString pMsg = tr("Writing Reciprocity to file. \nPlease wait...");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("RECIPROCITY (r) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; r &le; 1")
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Arc reciprocity: ")
            <<"</span>"
            << tr("%1 / %2 = %3").arg(m_graphReciprocityTiesReciprocated).arg(m_graphReciprocityTiesTotal).arg(m_graphReciprocityArc)
            << "<br />"
            << tr("Of all actual ties in the network, %1% are reciprocated.").arg(m_graphReciprocityArc * 100)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Dyad reciprocity: ")
            <<"</span>"
            << tr("%1 / %2 = %3").arg(m_graphReciprocityPairsReciprocated).arg(m_graphReciprocityPairsTotal).arg(m_graphReciprocityDyad )
            << "<br />"
            << tr("Of all pairs of actors that have any ties, %1% have a reciprocated connection.").arg(m_graphReciprocityDyad * 100)
            << "</p>";



    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Reciprocity proportions per actor: ")
            <<"</span>"
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Actor")
            <<"</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Label")
            <<"</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("Symmetric")
            <<"</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nonSymmetric")
            <<"</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc6 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nsym out/nsym")
            <<"</th>"
            <<"<th id=\"col6\" onclick=\"tableSort(results, 5, asc6); asc6*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc7 = 1;asc8 = 1;\">"
            << tr("nsym in/nsym")
            <<"</th>"
            <<"<th id=\"col7\" onclick=\"tableSort(results, 6, asc7); asc7*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc6 = 1;asc8 = 1;\">"
            << tr("nsym out/out")
            <<"</th>"
            <<"<th id=\"col8\" onclick=\"tableSort(results, 7, asc8); asc8*= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1; asc6 = 1;asc7 = 1;\">"
            << tr("nsym in/in")
            <<"</th>"
            <<"</tr>"
            << "</thead>"
            <<"<tbody  id=\"results\">";



    VList::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;
        qDebug() << "Graph::writeReciprocity outnon  - innon - rec"
                 << (*it)->outEdgesNonSym()
                 << (*it)->inEdgesNonSym()
                 << (*it)->outEdgesReciprocated();

        // Symmetric: Total number of reciprocated ties involving this actor divided by the number of ties to and from her.
        tiesSym =(qreal)   (*it)->outEdgesReciprocated() / (qreal)  ( (*it)->outEdgesCount() + (*it)->inEdgesCount());
        // non Symmetric: One minus symmetric
        tiesNonSym = 1 - tiesSym;
        // nonSym Out/NonSym. Proportion of non-symmetric outgoing ties to the total non-symmetric ties.
        tiesOutNonSym = ((*it)->outEdgesNonSym() || (*it)->inEdgesNonSym()) ? (qreal) (*it)->outEdgesNonSym() / (qreal) ((*it)->outEdgesNonSym() + (*it)->inEdgesNonSym()) : 0;
        // nonSym In/NonSym. Proportion of non-symmetric incoming ties to the total non-symmetric ties.
        tiesInNonSym =  ((*it)->outEdgesNonSym() || (*it)->inEdgesNonSym()) ?  (qreal) (*it)->inEdgesNonSym() / (qreal)  ((*it)->outEdgesNonSym() + (*it)->inEdgesNonSym()) : 0;
        // nonSym Out/Out. Proportion of non-symmetric outgoing ties to the total outgoing ties.
        tiesOutNonSymTotalOut = ( (*it)->outEdgesCount() != 0) ? (qreal)  (*it)->outEdgesNonSym() /(qreal) (*it)->outEdgesCount() : 0;
        // nonSym In/In. Proportion of non-symmetric incoming ties to the total incoming ties.
        tiesInNonSymTotalIn = ( (*it)->inEdgesCount() != 0) ? (qreal)  (*it)->inEdgesNonSym() / (qreal) (*it)->inEdgesCount() : 0;

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                <<"<td>"
                << (*it)->number()
                << "</td><td>"
                << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
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
                <<"</tr>";

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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();
}




/**
 * @brief Returns TRUE if the adjacency matrix of the current relation is symmetric
 * @return bool
 */
bool Graph::isSymmetric(){
    qDebug() << "Graph::isSymmetric() ";

    if ( calculatedGraphSymmetry ){
        qDebug() << "Graph::isSymmetric() - graph not modified and "
                    "already calculated symmetry. Returning previous result: "
                 << m_graphIsSymmetric;
        return m_graphIsSymmetric;
    }
    m_graphIsSymmetric=true;
    int v2=0, v1=0;
    qreal weight = 0;

    QHash<int,qreal> enabledOutEdges;

    QHash<int,qreal>::const_iterator hit;
    VList::const_iterator lit;


    for ( lit = m_graph.cbegin(); lit != m_graph.cend(); ++lit)
    {
        v1 = (*lit)->number();

        if ( ! (*lit)->isEnabled() )
            continue;

        enabledOutEdges=(*lit)->outEdgesEnabledHash();

        hit=enabledOutEdges.cbegin();

        while ( hit!=enabledOutEdges.cend() ){

            v2 = hit.key();
            weight = hit.value();

            if ( edgeExists ( v2, v1 )  != weight) {

                m_graphIsSymmetric=false;
                //                qDebug() <<"Graph::isSymmetric() - "
                //                         << " graph not symmetric because "
                //                         << v1 << "->" << v2 << " weight " << weight
                //                         << " differs from " << v2 << "->" << v1 ;

                break;
            }
            ++hit;
        }
    }
    //delete enabledOutEdges;
    qDebug() << "Graph: isSymmetric() - Finished. Result:"  << m_graphIsSymmetric;
    calculatedGraphSymmetry = true;
    return m_graphIsSymmetric;
}




/**
 * @brief Transforms the graph to symmetric (all edges reciprocal)
 */
void Graph::setSymmetric(){
    qDebug()<< "Tranforming graph to symmetric...";
    VList::const_iterator it;
    int v2=0, v1=0, weight;
    qreal invertWeight=0;
    QHash<int,qreal> enabledOutEdges;
    QHash<int,qreal>::const_iterator it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        v1 = (*it)->number();
//        qDebug() << "iterate over edges of v1 " << v1;
        enabledOutEdges=(*it)->outEdgesEnabledHash();
        it1=enabledOutEdges.cbegin();
        while ( it1!=enabledOutEdges.cend() ){
            v2 = it1.key();
            weight = it1.value();
//            qDebug() << "v1" << v1 << "outLinked to" << v2 << ", weight:" << weight;
            invertWeight = edgeExists(v2,v1);
            if ( invertWeight == 0 ) {
//                qDebug() << "v1" << v1 << "is NOT inLinked from v2" <<  v2  ;
                edgeCreate( v2, v1, weight, initEdgeColor, false, true, false,
                            QString(), false);
            }
            else {
//                qDebug() << "v1" << v1 << "is inLinked from v2" <<  v2  ;
                if (weight!= invertWeight )
                    edgeWeightSet(v2,v1,weight);
            }

            ++it1;
        }
    }
    //delete enabledOutEdges;

    m_graphIsSymmetric=true;

    setModStatus(ModStatus::EdgeCount);
}



/**
 * @brief Creates a new symmetric relation by keeping only strong-ties (mutual links)
 * in the current relation. In the new relation, two actors are connected only if
 * they are mutually connected in the current relation.
 * @param allRelations
 */
void Graph::addRelationSymmetricStrongTies(const bool &allRelations){

    qDebug()<< "Creating new relation using strong ties only."
            << "initial relations"<<relations();

    int y=0, v2=0, v1=0, weight;
    qreal invertWeight=0;

    VList::const_iterator it;

    QHash<int,qreal> outEdgesAll;
    QHash<int,qreal>::const_iterator it1;

    QHash<QString,qreal> *strongTies = new QHash<QString,qreal>;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        v1 = (*it)->number();
        qDebug() << "Graph::addRelationSymmetricStrongTies() - v" << v1
                 << "iterate over outEdges in all relations";
        outEdgesAll=(*it)->outEdgesEnabledHash(allRelations); //outEdgesAllRelationsUniqueHash();
        it1=outEdgesAll.cbegin();
        while ( it1!=outEdgesAll.cend() ){
            v2 = it1.key();
            weight = it1.value();
            y=vpos[ v2 ];
            qDebug() << ""
                     << v1 << "->" << v2 << "=" << weight << "Checking opposite.";
            invertWeight = m_graph[y]->hasEdgeTo( v1,allRelations ) ;
            if ( invertWeight == 0 ) {
                qDebug() << v1 << "<-" <<  v2 << " does not exist. Weak tie. Continue." ;
            }
            else {
                if (!strongTies->contains(QString::number(v1)+"--"+QString::number(v2)) &&
                        !strongTies->contains(QString::number(v2)+"--"+QString::number(v1)) ){
                    qDebug() << v1 << "--" << v2 << " exists. Strong Tie. Adding";
                    strongTies->insert(QString::number(v1)+"--"+QString::number(v2), 1);
                }
                else {
                    qDebug() << v1 << "--" << v2 << " exists. Strong Tie already found. Continue";
                }
            }
            ++it1;
        }
    }


    relationAdd("Strong Ties",true);

    QHash<QString,qreal>::const_iterator it2;
    it2=strongTies->constBegin();
    QStringList vertices;
    qDebug() << "creating strong tie edges...";
    while ( it2!=strongTies->constEnd() ){
        vertices = it2.key().split("--");
//        qDebug() << "tie " <<it2.key()
//                 << "vertices.at(0)" << vertices.at(0)
//                 << "vertices.at(1)" << vertices.at(1);
        v1 = (vertices.at(0)).toInt();
        v2 = (vertices.at(1)).toInt();
//        qDebug() << "calling edgeCreate for" << v1 << "--"<<v2;
        edgeCreate( v1, v2, 1, initEdgeColor, EdgeType::Undirected, true, false,
                    QString(), false);
        ++it2;
    }

    //delete outEdgesAll;
    delete strongTies;
    m_graphIsSymmetric=true;

    setModStatus(ModStatus::EdgeCount);

}





/**
* @brief Creates a new symmetric relation by connecting those actors
* that are cocitated by others.
* In the new relation, an edge will exist between actor i and actor j
* only if C(i,j) > 0, where C the Cocitation Matrix.
* Thus the actor pairs cited by more common neighbors will appear
* with a stronger tie between them than pairs those cited by fewer
* common neighbors. The resulting relation is symmetric.
 */
void Graph::relationAddCocitation(){
    qDebug()<< "Graph::relationAddCocitation()"
            << "initial relations"<<relations();

    int v1=0, v2=0, i=0, j=0, weight;
    bool dropIsolates = false;

    createMatrixAdjacency();

    Matrix *CT = new Matrix (AM.rows(), AM.cols());
    *CT = AM.cocitationMatrix();

    //CT->printMatrixConsole(true);

    VList::const_iterator it, it1;

    relationAdd("Cocitation",true);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() || ( (*it)->isIsolated() && dropIsolates) ) {
            continue;
        }
        v1 = (*it)->number();
        j = 0;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); it1++){
            qDebug()<< "Graph::relationAddCocitation() - (i,j)" << i+1<<j+1;
            if ( ! (*it1)->isEnabled() || ( (*it1)->isIsolated() && dropIsolates) ) {
                continue;
            }
            v2 = (*it1)->number();

            if (v1==v2) {
                j++;
                qDebug()<< "Graph::relationAddCocitation() - skipping self loop" << v1<<v2;
                continue;
            }
            if ( (weight = CT->item(i, j) ) != 0 ) {
                qDebug()<< "Graph::relationAddCocitation() - creating edge"
                        << v1 << "<->" << v2
                        << "because CT(" << i+1 << "," <<  j+1 << ") = " << weight;
                edgeCreate( v1, v2, weight, initEdgeColor,
                            EdgeType::Undirected, true, false,
                            QString(), false);
            }

            j++;
        }
        i++;
    }

    m_graphIsSymmetric=true;

    setModStatus(ModStatus::EdgeCount);
    qDebug()<< "Graph::relationAddCocitation()"
            << "final relations"<<relations();
}


/**
 * @brief Creates a new binary relation in a valued network using edge
 * dichotomization according to the threshold parameter.
 * @param threshold
 */
void Graph::graphDichotomization(const qreal threshold) {
    qDebug()<< "Graph::graphDichotomization()"
            << "initial relations"<<relations();

    int v2 = 0, v1 = 0;
    qreal weight = 0;

    VList::const_iterator it;

    QHash<int,qreal> outEdgesAll;
    QHash<int,qreal>::const_iterator it1;

    QHash<QString,qreal> *binaryTies = new QHash<QString,qreal>;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        v1 = (*it)->number();
        qDebug() << "Graph::graphDichotomization() - v" << v1
                 << "iterate over outEdges in all relations";
        outEdgesAll=(*it)->outEdgesEnabledHash(false);
        it1=outEdgesAll.cbegin();
        while ( it1!=outEdgesAll.cend() ){
            v2 = it1.key();
            weight = it1.value();

            qDebug() << v1 << "->" << v2 << "=" << weight << "Checking opposite.";
            if (weight>threshold) {
                if (!binaryTies->contains(QString::number(v1)+"--"+QString::number(v2) ) ) {
                    qDebug() << v1 << "--" << v2 << " over threshold. Adding";
                    binaryTies->insert(QString::number(v1)+"--"+QString::number(v2), 1);
                }
                else {
                    qDebug() << v1 << "--" << v2 << " exists. Binary Tie already found. Continue";
                }
            }
            ++it1;
        }
    }

    relationAdd("Binary-"+QString::number(threshold),true);

    QHash<QString,qreal>::const_iterator it2;
    it2=binaryTies->constBegin();
    QStringList vertices;
    qDebug() << "creating binary tie edges";
    while ( it2!=binaryTies->constEnd() ){
        vertices = it2.key().split("--");
        qDebug() << "binary tie " <<it2.key()
                 << "vertices.at(0)" << vertices.at(0)
                 << "vertices.at(1)" << vertices.at(1);
        v1 = (vertices.at(0)).toInt();
        v2 = (vertices.at(1)).toInt();
        qDebug() << "calling edgeCreate for"
                 << v1 << "--"<<v2;
        edgeCreate( v1, v2, 1, initEdgeColor, EdgeType::Undirected, true, false,
                    QString(), false);
        ++it2;
    }

    //delete outEdgesAll;
    delete binaryTies;
    m_graphIsSymmetric=true;

    setModStatus(ModStatus::EdgeCount);
    qDebug()<< "final relations"<<relations();

}



/**
 * @brief Toggles the graph directed or undirected
 *
 * @param toggle
 * @param signalMW
 */
void Graph::setDirected(const bool &toggle, const bool &signalMW){

    qDebug() << "Setting graph directed to:" << toggle ;

    if ( !toggle ) {
        setUndirected(true);
    }

    if (toggle == isDirected() ) {
        qDebug() << "Same as now, nothing to do.";
        return;
    }

    m_graphIsDirected = true;

    if (m_graphIsDirected) {
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
void Graph::setUndirected(const bool &toggle, const bool &signalMW){

    qDebug() << "Toggling graph undirected to" << toggle ;

    if ( !toggle ) {
        setDirected(true);
        return;
    }

    if (toggle == isUndirected()) {
        qDebug() << "Same as now, nothing to do.";
        return;
    }

    m_graphIsDirected = false;

    VList::const_iterator it;
    int v2=0, v1=0, weight;
    QHash<int,qreal> enabledOutEdges;
    QHash<int,qreal>::const_iterator it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        v1 = (*it)->number();
        qDebug() << "Iterating over edges of v1 " << v1;
        enabledOutEdges=(*it)->outEdgesEnabledHash();
        it1=enabledOutEdges.cbegin();
        while ( it1!=enabledOutEdges.cend() ){
            v2 = it1.key();
            weight = it1.value();

            qDebug() << "edge" << "v1" << v1 << "->" << v2 << " = " << "weight" << weight;
            edgeTypeSet(v1,v2, weight, EdgeType::Undirected);
            ++it1;
        }
    }
    //delete enabledOutEdges;

    m_graphIsSymmetric=true;

    setModStatus(ModStatus::EdgeCount, signalMW);
}



/**
 * @brief Returns true if graph is directed
 *
 * @return bool
 */
bool Graph::isDirected() {
    qDebug() << "isDirected" << m_graphIsDirected;
    return m_graphIsDirected;
}




/**
 * @brief Returns true if graph is undirected
 *
 * @return bool
 */
bool Graph::isUndirected() {
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
                        const int &dirType) {

    qDebug() << "Changing the direction type of edge: " << v1
             << "->" <<  v2  << "new edgeType:" << dirType;

    if (dirType!=EdgeType::Directed) {

        // check if reverse edge exists
        qreal revEdgeWeight = edgeExists ( v2, v1 ) ;

        if ( revEdgeWeight == 0 ) {
            // Reverse edge does not exist, add it
            qDebug() << "reverse  edge" << v1 << " <- " <<  v2 << " does not exist - Adding it..." ;
            // Note: Even if dirType=EdgeType::Undirected we add the opposite edge as EdgeType::Reciprocated
            edgeAdd(v2,v1, weight, EdgeType::Reciprocated, "", initEdgeColor);
        }
        else {
            // Reverse edge does exist
            if ( dirType == EdgeType::Undirected ) {
                // Make the edge weights equal
                // TOFIX: how do we decide which of the two weights to keep?
                qDebug() << "Graph::edgeTypeSet(): opposite  " << v1
                         << " <- " <<  v2 << " exists - equaling weights." ;
                if ( weight!= revEdgeWeight ) {
                    edgeWeightSet(v2,v1,weight);
                }
            }
            else {
                // if dirType is EdgeType::Reciprocated we don't need  to equalize weights
            }
        }
        emit signalEdgeType( v1, v2, dirType );
    }
}





/**
 * @brief Returns true if vertices v1 and v2 are reachable.
 *
 * @param v1
 * @param v2
 * @return bool
 */
bool Graph::graphReachable(const int &v1, const int &v2) {
    qDebug()<< "Graph::reachable()";
    graphDistancesGeodesic(false);
    return ( m_graph[ vpos[v1] ]->distance( v2) != RAND_MAX ) ? true: false;
}




/**
 * @brief Creates the reachability matrix XRM
 */
void Graph::createMatrixReachability() {
    qDebug() << "Creating the Reachability Matrix...";

    graphDistancesGeodesic(false);

    VList::const_iterator it, jt;

    int N = vertices( false, false, true);

    int progressCounter=0;
    int source = 0 , target = 0;
    int i = 0, j = 0 ;
    int reachVal = 0;

    XRM.resize(N, N);

    QString pMsg = tr("Creating reachability matrix. \nPlease wait ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    qDebug() << "Writing Reachability matrix...";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if  ( ! (*it)->isEnabled()  ) {
            qDebug() << "source vertex" << source << "disabled. SKIP";
            continue;
        }

        qDebug() << "source vertex" << source<< "i" << i;

        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt) {

            target = (*jt)->number();

            if  ( ! (*jt)->isEnabled()  ) {
                qDebug() << "target vertex" << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "target vertex" << target << "j" << j;

            reachVal = ((*it)->distance( target ) != RAND_MAX ) ? 1 : 0;
            qDebug() << "Setting XRM ("<< i <<","<< j << ") =" <<  reachVal;
            XRM.setItem( i, j, reachVal );

            j++;
        }
        j=0;
        i++;
    }

    emit signalProgressBoxKill();


}




/**
 * @brief Returns the geodesic distance (length of shortest path)
 * from vertex v1 to vertex v2
 * @param v1
 * @param v2
 * @param considerWeights
 * @param inverseWeights
 * @return
 */
int Graph::graphDistanceGeodesic(const int &v1, const int &v2,
                                 const bool &considerWeights,
                                 const bool &inverseWeights){
    qDebug() <<"Graph::graphDistanceGeodesic()";
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);
    return m_graph[ vpos[v1] ]->distance(v2);
}




/**
 * @brief Returns the diameter of the graph, aka the largest geodesic distance
 * between any two vertices
 * @param considerWeights
 * @param inverseWeights
 * @return
 */
int Graph::graphDiameter(const bool considerWeights,
                         const bool inverseWeights){
    qDebug () << "Graph::graphDiameter()" ;
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);
    return m_graphDiameter;
}



/**
 * @brief Returns the average distance of the graph
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 * @return
 */
qreal Graph::graphDistanceGeodesicAverage(const bool considerWeights,
                                          const bool inverseWeights,
                                          const bool dropIsolates){



    qDebug() <<"Graph::graphDistanceGeodesicAverage() - Computing distances...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    qDebug() <<"Graph::graphDistanceGeodesicAverage() - "
             << "average distance:"
             << m_graphAverageDistance;

    return m_graphAverageDistance;

}




/**
 * @brief Returns the number of geodesics (shortest-paths) in the graph.
 *
 * @return int
 */
int Graph::getGeodesicsCount()  {
    qDebug()<< "Graph::getGeodesicsCount()";

    graphDistancesGeodesic(false, false,false,false);

    qDebug()<< "Graph::getGeodesicsCount() - geodesics:" << m_graphGeodesicsCount;
    return m_graphGeodesicsCount;


}





/**
 * @brief Checks if the graph is connected, in the sense of a topological space,
 * i.e., there is a path from any vertex to any other vertex in the graph.
 * Called from MW::slotConnectedness()
 * @return bool
 */
bool Graph::isConnected() {

    qDebug() << "Graph::isConnected() ";

    if ( calculatedDistances ) {
        qDebug()<< "Graph::isConnected() - graph unmodified. Returning:"
                << m_graphIsConnected;
        return m_graphIsConnected;
    }

    graphDistancesGeodesic(false, false,false,false);

    return m_graphIsConnected;

}







/**
 * @brief Creates the matrix SIGMA of shortest paths (geodesics) between vertices
 * Each SIGMA(i,j) is the number of shortest paths (geodesics) from i and j
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphMatrixShortestPathsCreate(const bool &considerWeights,
                                           const bool &inverseWeights,
                                           const bool &dropIsolates) {
    qDebug() << "Graph::graphMatrixShortestPathsCreate()";

    graphDistancesGeodesic(false,considerWeights,inverseWeights, dropIsolates);

    VList::const_iterator it, jt;

    int N = vertices( dropIsolates, false, true);

    int progressCounter=0;
    int source = 0 , target = 0;
    int i = 0, j = 0 ;

    qDebug() << "Graph::graphMatrixShortestPathsCreate() - Resizing matrix to hold "
             << N << " vertices";

    SIGMA.resize(N, N);

    QString pMsg = tr("Creating shortest paths matrix. \nPlease wait ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    qDebug() << "Graph::graphMatrixShortestPathsCreate() - Writing shortest paths matrix...";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if  ( (*it)->isIsolated() && dropIsolates ) {
            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "isolated. SKIP";

            continue;
        }

        if  ( ! (*it)->isEnabled()  ) {
            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "disabled. SKIP";
            continue;
        }

        qDebug() << "Graph::graphMatrixShortestPathsCreate() - source" << source
                 << "i" << i;

        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt) {

            target = (*jt)->number();

            if  ( (*jt)->isIsolated() && dropIsolates ) {
                qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "isolated. SKIP";
                continue;
            }

            if  ( ! (*jt)->isEnabled()  ) {
                qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << "target" << target << "j" << j;


            qDebug() << "Graph::graphMatrixShortestPathsCreate() -  setting SIGMA ("
                     << i <<","<< j << ") =" << (*it)->shortestPaths( target )  ;
            SIGMA.setItem( i, j, (*it)->shortestPaths( target ) );
            j++;
        }
        j=0;
        i++;
    }

    emit signalProgressBoxKill();


}



/**
 * @brief Creates the matrix DM of geodesic distances between vertices
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphMatrixDistanceGeodesicCreate(const bool &considerWeights,
                                              const bool &inverseWeights,
                                              const bool &dropIsolates) {
    qDebug() << "Graph::graphMatrixDistanceGeodesicCreate()";


    graphDistancesGeodesic(false,considerWeights,inverseWeights, dropIsolates);

    VList::const_iterator it, jt;

    int N = vertices( dropIsolates, false, true);

    int progressCounter=0;
    int source = 0 , target = 0;
    int i = 0, j = 0 ;

    qDebug() << "Graph::graphMatrixDistanceGeodesicCreate() - "
                "Resizing distance matrix to hold "
             << N << " vertices";

    DM.resize(N, N);

    QString pMsg = tr("Creating geodesic distances matrix. \nPlease wait ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - Writing distances matrix...";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if  ( (*it)->isIsolated() && dropIsolates ) {
            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << source << "isolated. SKIP";

            continue;
        }

        if  ( ! (*it)->isEnabled()  ) {
            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << source << "disabled. SKIP";
            continue;
        }


        qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - source"
                 << source << "i" << i;

        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt) {

            target = (*jt)->number();

            if  ( (*jt)->isIsolated() && dropIsolates ) {
                qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                         << target << "isolated. SKIP";
                continue;
            }

            if  ( ! (*jt)->isEnabled()  ) {
                qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                         << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << "target" << target << "j" << j;


            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() -  setting DM ("
                     << i <<","<< j << ") =" << (*it)->distance( target ) ;
            DM.setItem( i, j, (*it)->distance( target ) );

            j++;
        }
        j=0;
        i++;
    }

    emit signalProgressBoxKill();


}


/**
 * @brief Computes the geodesic distances between all vertices:
  * In the process, it also computes many other centrality/prestige metrics:
 * * The so-called sigma matrix, where the (i,j) element is the number of shortest paths
 *   from vertex i to vertex j, called sigma(i,j).
 * * The Diameter of the graph, m_graphDiameter, which is the length of the longest
 *   shortest path between every (i,j)
 * * The Eccentricity of every node i which is the length of the longest shortest
 *   path from i to every other node j
 * * The InfluenceRange and InfluenceDomain of each node.
 * * The centralities for every u in V (if centralities=true):
 *   - Betweenness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
 *   - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
 *   - Eccentricity: EC(u) =  1/maxDistance(u,t)  for some t in V
 *   - Closeness: CC(u) =  1 / Sum( d(u,t) )  for every  t in V
 *   - Power:
 * @param centralities
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphDistancesGeodesic(const bool &computeCentralities,
                                   const bool &considerWeights,
                                   const bool &inverseWeights,
                                   const bool &dropIsolates) {

    qDebug() << "Graph::graphDistancesGeodesic()"
             << "centralities" << computeCentralities
             << "considerWeights:"<<considerWeights
             << "inverseWeights:"<<inverseWeights
             << "dropIsolates:" << dropIsolates;

    if (computeCentralities) {
        if ( calculatedCentralities ) {
            qDebug() << "Graph::graphDistancesGeodesic() - Centralities calculated. Return.";
            return;
        }
    }
    else if ( calculatedDistances )  {
        qDebug() << "Graph::graphDistancesGeodesic() - graph not modified. Return.";
        return;
    }

    VList::const_iterator it, it1;
    QList<int>::const_iterator it2;

    int w=0, u=0,s=0, si=0, ui=0, wi=0;
    //int i=0;
    int progressCounter=0;

    qDebug() << "Graph::graphDistancesGeodesic() - Recomputing geodesic distances.";


    //drop isolated vertices from calculations (i.e. std C and group C).
    int N = vertices(dropIsolates,false,true);
    int E = edgesEnabled();


    QString pMsg  = tr("Computing geodesic distances. \nPlease wait...");
    emit statusMessage ( pMsg  );
    emit signalProgressBoxCreate(N, pMsg );

    m_graphIsSymmetric = isSymmetric();
    qDebug() << "Graph::graphDistancesGeodesic() - m_graphIsSymmetric"
                << m_graphIsSymmetric ;

    if ( E == 0 ) {

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1) {
                // Set all pair-wise distances to RAND_MAX
                (*it)->setDistance((*it1)->number(), RAND_MAX);
                // Set all pair-wise shortest-path counts (sigmas) to 0
                (*it)->setShortestPaths((*it1)->number(), 0);
            }
        }
        if ( N < 2 ) {
            //singleton graph consisting of a single isolated node
            //is considered connected
            m_graphIsConnected = true;
        }
        else {
            //any non-empty and non-singleton graph with zero edges is disconnected
            m_graphIsConnected = false;
        }

    }
    else {

        qDebug() << "Graph::graphDistancesGeodesic() - Initializing variables";

        qreal distances_sum_for_s = 0, maxEdgeWeightInNetwork=0, tempEdgeWeight=0;
        qreal CC=0, BC=0, SC= 0, eccentricity=0, EC=0, PC=0;
        qreal SCC=0, SBC=0, SSC=0, SEC=0, SPC=0;
        qreal tempVarianceBC=0, tempVarianceSC=0,tempVarianceEC=0;
        qreal tempVarianceCC=0, tempVariancePC=0;
        qreal sigma_u=0, sigma_w=0;
        qreal delta_u=0, delta_w=0;
        qreal d_sw=0, d_su=0;
        qreal pairDistance = 0;

        m_graphIsConnected = true;

        H_f_i::const_iterator hfi ; // for Power Centrality

        qDebug() << "Graph: graphDistancesGeodesic() - initialising centrality variables ";

        maxSCC=0; minSCC=RAND_MAX; nomSCC=0; denomSCC=0; groupCC=0; maxNodeSCC=0;
        minNodeSCC=0; sumSCC=0; sumCC=0;
        discreteCCs.clear(); classesSCC=0;
        maxSBC=0; minSBC=RAND_MAX; nomSBC=0; denomSBC=0; groupSBC=0; maxNodeSBC=0;
        minNodeSBC=0; sumBC=0; sumSBC=0;
        discreteBCs.clear(); classesSBC=0;
        maxSSC=0; minSSC=RAND_MAX; groupSC=0; maxNodeSSC=0;
        minNodeSSC=0;sumSC=0; sumSSC=0;
        discreteSCs.clear(); classesSSC=0;
        maxSPC=0; minSPC=RAND_MAX; nomSPC=0; denomSPC=0; groupSPC=0; maxNodeSPC=0;
        minNodeSPC=0; sumSPC=0;sumPC=0;
        discretePCs.clear(); classesSPC=0;
        maxEccentricity=0; minEccentricity=RAND_MAX; maxNodeEccentricity=0;
        minNodeEccentricity=0; discreteEccentricities.clear();
        classesEccentricity=0;
        maxSPC=0; minSPC=RAND_MAX; maxNodeSPC=0; minNodeSPC=0; sumSPC=0;
        maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0;
        minNodeEC=0; sumEC=0;

        discreteECs.clear(); classesEC=0;

        m_graphDiameter=0;
        calculatedDistances = false;
        m_graphAverageDistance=0;
        m_graphSumDistance = 0;
        m_graphGeodesicsCount = 0; //non zero distances

        // Stores vertex pairs not connected
        // Vertices in keys have
        // Infinite Eccentricity
        // Zero Eccentricity Centrality
        // Zero Closeness Centrality
        m_vertexPairsNotConnected.clear();

        qDebug() << "	m_graphDiameter "<< m_graphDiameter
                 << " m_graphAverageDistance " <<m_graphAverageDistance;
        qDebug() << "	reciprocalEdgesVert "<< reciprocalEdgesVert
                 << " inboundEdgesVert " << inboundEdgesVert
                 << " outboundEdgesVert "<<  outboundEdgesVert;
        qDebug() << "	E " << E <<  " N " << N;


        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1) {
                // All pair-wise distances are set to RAND_MAX by default
                // inside GraphVertex::distance()
                // so we don't need to explicitly set them here.
                // We just clear distance hashmap of each actor.
                (*it)->clearDistance();
                // Set all pair-wise shortest-path counts (sigmas) to 0
                // (*it)->setShortestPaths((*it1)->number(), 0);
                (*it)->clearShortestPaths();

                if (considerWeights && inverseWeights) {
                    // find the max weight in the network.
                    // it will be used for maxCC below
                    tempEdgeWeight = (*it)->hasEdgeTo((*it1)->number());
                    if ( tempEdgeWeight > maxEdgeWeightInNetwork ) {
                        maxEdgeWeightInNetwork = tempEdgeWeight;
                    }
                }

            }

            //Zero centrality scores for each vertex
            if (computeCentralities) {

                qDebug() << " Graph:graphDistancesGeodesic() -"
                            "Initializing actor centrality indices";
                (*it)->setBC( 0.0 );
                (*it)->setSC( 0.0 );
                (*it)->setEccentricity( 0.0 );
                (*it)->setEC( 0.0 );
                (*it)->setCC( 0.0 );
                (*it)->setIRCC( 0.0 );
                (*it)->setPC( 0.0 );


            }

        }


        qDebug() << "Graph: graphDistancesGeodesic() - "
                    " initialising variables for max centrality scores";
        if (m_graphIsSymmetric) {
            maxIndexBC= ( N == 2 ) ? 1 : ( N-1.0 ) * ( N-2.0 ) / 2.0;
            maxIndexSC= ( N == 2 ) ? 1 : ( N-1.0 ) * ( N-2.0 ) / 2.0;
            maxIndexCC=N-1.0;
            maxIndexPC=N-1.0;
            qDebug("############# m_graphIsSymmetric - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }
        else {

            maxIndexBC= ( N == 2 ) ? 1 : ( N-1.0 ) * ( N-2.0 );  // fix N=2 case where maxIndex becomes zero
            maxIndexSC= ( N == 2 ) ? 1 : ( N-1.0 ) * ( N-2.0 );
            maxIndexPC=N-1.0;
            maxIndexCC=N-1.0;
            qDebug("############# NOT SymmetricAdjacencyMatrix - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }

        if (considerWeights && inverseWeights) {
            maxIndexCC = maxIndexCC * (1.0 / maxEdgeWeightInNetwork);
        }

        qDebug() << "*********** MAIN LOOP: "
                    "for every s in V solve the Single Source Shortest Path (SSSP) problem...";
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

            s=(*it)->number();
            si=vpos[s];
            distances_sum_for_s = 0;

            qDebug()<< "***** PHASE 1 (SSSP): "
                    << "Source vertex s" << s << "vpos" << si;

            emit signalProgressBoxUpdate( ++progressCounter );

            if ( ! (*it)->isEnabled() ) {
                qDebug()<< "***** PHASE 1 (SSSP): s" << s  << "disabled. SKIP/CONTINUE";
                continue;
            }


            if (computeCentralities) {

                qDebug()<< "***** PHASE 1 (SSSP): "
                           "Empty Stack which will return vertices in "
                           "order of their (non increasing) distance from s ...";
                //- Complexity linear O(n)
                while ( !Stack.empty() ) {
                    Stack.pop();
                }

                qDebug()<< "***** PHASE 1 (SSSP): "
                           "...and for each vertex: empty list Ps of predecessors";
                for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1) {
                    (*it1)->clearPs();

                }
                sizeOfNthOrderNeighborhood.clear();
            }

            qDebug()<< "***** PHASE 1 (SSSP): "
                       "Call BFS or dijkstra for s"
                     << s << " vpos " << si
                     << " to compute distance and shortest paths to every vertex t" ;

            if (!considerWeights) {
                BFS(s,si,computeCentralities, dropIsolates );
            }
            else {
                dijkstra(s, si,computeCentralities, inverseWeights, dropIsolates);
            }

            qDebug()<< "***** PHASE 1 (SSSP): "
                       "FINISHED BFS / DIJKSTRA ALGORITHM. "
                   "Continuing to calculate centralities";

            if (computeCentralities) {



                qDebug()<< "***** PHASE 2 (CENTRALITIES): "
                           "s" << s << "vpos" << si << "CC" << CC;


                // Compute Power Centrality
                // In = [ 1/(N-1) ] * ( Nd1 + Nd2 * 1/2 + ... + Ndi * 1/i )
                // where
                // Ndi (sizeOfNthOrderNeighborhood) is the number of nodes at distance i from this node.
                // N is the sum Nd0 + Nd1 + Nd2 + ... + Ndi, that is the amount of nodes in the same component as the current node

                sizeOfComponent = 1;
                PC=0;
                hfi = sizeOfNthOrderNeighborhood.constBegin();
                //FIXME do we need to check for disabled nodes somewhere?
                while (hfi != sizeOfNthOrderNeighborhood.constEnd()) {
                    qDebug() << " sizeOfNthOrderNeighborhood.value("<< hfi.key() <<")"
                                 << hfi.value();
                    PC += ( 1.0 / hfi.key() ) * hfi.value();
                    sizeOfComponent += hfi.value();
                    ++hfi;
                }

                (*it)->setPC( PC );
                sumPC += PC;
                if ( sizeOfComponent != 1 )
                    SPC = ( 1.0/(sizeOfComponent-1.0) ) * PC;
                else
                    SPC = 0;

                (*it)->setSPC( SPC );	//Set std PC

                sumSPC += SPC;   //add to sumSPC -- used later to compute mean and variance

                qDebug()<< "***** PHASE 2 (CENTRALITIES): "
                           "s" << s << "vpos" << si << "PC" << PC;


                // Compute Betweenness Centrality

                qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                           "Start back propagation of dependencies." <<
                           "Set dependency delta[u]=0 on each vertex";

                for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
                    (*it1)->setDelta(0.0);

                    // compute sum of distances from current vertex to every other vertex

                    distances_sum_for_s += (*it)->distance( (*it1)->number() ) ;
                    qDebug() << "    Compute Centralities: "
                               "For CC: sum of distances. distance(" << (*it)->number()
                             << "," <<  (*it1)->number() << ") = " << (*it)->distance( (*it1)->number() )
                             << "new sum of distances for s =" << distances_sum_for_s;

                }
                qDebug() << "    Compute Centralities: "
                          "For CC: total sum of distances for s =" << distances_sum_for_s;

                m_graphSumDistance += distances_sum_for_s;

                // Compute Closeness Centrality
                if ( distances_sum_for_s != 0 && distances_sum_for_s < RAND_MAX)  {
                    // Connected actor:
                    // There is a path from this actor to all others
                    // Invert the sum of distances and set it as CC
                    CC=1.0/distances_sum_for_s;
                }
                else {
                    // Not connected actor. Cases:
                    // a) Isolated: The actor has no outbound links
                    // b) Disconnected graph: There is no path from this actor
                    // to some of the other actors, which means her distance to
                    // them is infinite
                    // For these two cases, set CC as zero.
                    CC=0;
                }
                (*it)->setCC( CC );



                qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                           "Visit all vertices in reverse order of their discovery (from s = "
                         << s
                         << " ) to sum dependencies. Initial Stack size " << Stack.size();

                while ( !Stack.empty() ) {
                    w=Stack.top();
                    wi=vpos[w];

                    qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                               "Stack top is vertex w " << w
                            << "This is the furthest vertex from s. Popping it.";

                    Stack.pop();
                    QList<int> lst=m_graph[wi]->Ps();

                    qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                               "preLOOP: Size of predecessors list Ps[w]"<< lst.size();

                    qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                               "LOOP over every vertex u in Ps of w"<<w;

                    if (lst.size() > 0) // just in case...do a sanity check
                        for ( it2=lst.cbegin(); it2 != lst.cend(); it2++ ){
                            u=(*it2);
                            ui=vpos[u];
                            sigma_u=m_graph[si]->shortestPaths(u);
                            sigma_w=m_graph[si]->shortestPaths(w);
                            delta_u=m_graph[ui]->delta();
                            delta_w=m_graph[wi]->delta();

                            qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                                       "Selecting Ps[w] element u"<< u
                                    << "with delta_u" << delta_u
                                    << "sigma(s,u)"<< sigma_u
                                    << "sigma(s,w)" << sigma_w
                                    << "delta_w"<< delta_w;

                            if ( m_graph[si]->shortestPaths(w) > 0 ) {
                                //delta[u]=delta[u]+(1+delta[w])*(sigma[u]/sigma[w]) ;
                                d_su=delta_u + ( 1.0 + delta_w ) * ( (qreal) sigma_u / (qreal)sigma_w);
                            }
                            else {
                                d_su=delta_u;
                                qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                                           "zero shortest paths from s to w - "
                                           "using SAME DELTA for vertex u";
                            }
                            qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                                       "Assigning new delta d_su"
                                    << d_su
                                    << " to u" << u;

                            m_graph[ui]->setDelta( d_su);

                        } // end for

                    qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                               "Adding delta_w to BC of w";

                    if  (w!=s) {

                        qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                                   "w!=s. For this furthest vertex we need to add its new delta"
                                 << delta_w
                                 << "to old BC index:"
                                 <<  m_graph[wi]->BC();

                        d_sw = m_graph[wi]->BC() + delta_w;

                        qDebug()<< "***** PHASE 2 (BC/ACCUMULATION): "
                                   "s" << s << "vpos" << si << "BC = d_sw" << d_sw;

                        m_graph[wi]->setBC (d_sw);

                    } // END if

                } // END while stack

            } // END if computeCentralities

        }     // END for SSSP problem


        qDebug() << "*********** MAIN LOOP (SSSP problem): FINISHED.";


        // check if there are disconnected nodes
        // and get the distance sums
        qDebug() << "Checking if there are disconnected nodes";

        m_graphIsConnected = true;

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

            if ( ! (*it)->isEnabled() ) {
                qDebug()<< "actor i" <<  (*it)->number() << "disabled. SKIP/CONTINUE";
                continue;
            }

            pairDistance = 0;

            for ( it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

                if ( ! (*it1)->isEnabled() ) {
                    qDebug()<< "   actor j" <<  (*it1)->number() << "disabled. SKIP/CONTINUE";
                    continue;
                }
                if (  (*it1)->number() == (*it)->number() ) {
                    qDebug()<< "   == actor j" <<  (*it1)->number() << "SKIP/CONTINUE";
                    continue;
                }

                pairDistance = (*it)->distance ( (*it1)->number() );
                
                if ( pairDistance == RAND_MAX) {
                    m_vertexPairsNotConnected.insert((*it)->number(), (*it1)->number());
                    (*it)->setEccentricity( RAND_MAX );
                    m_graphIsConnected = false;

                    qDebug()<< "actor i" <<  (*it)->number()
                            << "has infinite eccentricity. "
                               "There is no path from it to actor j"
                            << (*it1)->number();

                }
                else {

                    qDebug()<< "actor i" <<  (*it)->number()
                            <<"distanceSum" << (*it)->distanceSum();
                    (*it)->setDistanceSum( (*it)->distanceSum() + pairDistance);

                }
            } // end for
            
            qDebug()<< "actor i" <<  (*it)->number()
                    <<"Final distanceSum" << (*it)->distanceSum();


            if (computeCentralities) {

                // Compute Eccentricity (max geodesic distance)
                eccentricity = (*it)->eccentricity();
                
                qDebug() << "actor"
                         << (*it)->number()
                         << "eccentricity" << eccentricity;
                
                if ( eccentricity != RAND_MAX ) {

                    //Find min/max Eccentricity
                    minmax( eccentricity, (*it), maxEccentricity, minEccentricity,
                            maxNodeEccentricity, minNodeEccentricity) ;
                    resolveClasses(eccentricity, discreteEccentricities,
                                   classesEccentricity ,(*it)->number() );

                    //Eccentricity Centrality is the inverted Eccentricity
                    EC=1.0 / eccentricity;
                    (*it)->setEC( EC ); //Set Eccentricity Centrality
                    (*it)->setSEC( EC ); //Set std EC = EC
                    sumEC+=EC;  //set sum EC

                    qDebug()<< "actor i" <<  (*it)->number()
                            << "EC"
                            << EC;
                }
                else {

                    EC=0;
                    (*it)->setEC( EC );     //Set Eccentricity Centrality
                    (*it)->setSEC( EC );    //Set std EC = EC
                    sumEC+=EC;  //set sum EC

                    qDebug()<< "actor i" <<  (*it)->number()
                            << "EC=0 (disconnected graph)";

                }
                
            } // end if compute centralities

        } // end for disconnected checking

        // Compute average path length...
        if (m_vertexPairsNotConnected.size()==0) {

            m_graphAverageDistance = m_graphSumDistance / ( N * ( N-1.0 ) );
            qDebug() <<"Graph::graphDistancesGeodesic() - Average distance:"
                    << m_graphAverageDistance ;

        }
        else {

            //TODO In not connected nets, it would be nice to ask the user what to do
            // with unconnected pairs (make M or drop (default?)
            qDebug() <<"Graph::graphDistancesGeodesic() - Average distance:"
                    << m_graphAverageDistance ;
            m_graphAverageDistance = m_graphSumDistance / m_graphGeodesicsCount;


        }


        if (computeCentralities) {

            qDebug() << "Graph: graphDistancesGeodesic() - "
                        "Computing centralities...";
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ){
                    qDebug() << "vertex " << (*it)->number()
                             << " isolated, continue. ";
                    continue;
                }

                // Compute classes and min/maxEC
                SEC=(*it)->SEC();
                resolveClasses(SEC, discreteECs, classesEC,(*it)->number() );
                minmax( SEC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;

                // Compute classes and min/maxSPC
                SPC = (*it)->SPC();  //same as PC
                resolveClasses(SPC, discretePCs, classesSPC,(*it)->number() );
                minmax( SPC, (*it), maxSPC, minSPC, maxNodeSPC, minNodeSPC) ;

                // Compute std BC, classes and min/maxSBC
                if (m_graphIsSymmetric) {
                    qDebug()<< "Betweenness centrality must be divided by"
                            <<" two if the graph is undirected";
                    (*it)->setBC ( (*it)->BC()/2.0);
                }
                BC=(*it)->BC();
                sumBC+=BC;
                SBC = BC/maxIndexBC;
                (*it)->setSBC( SBC );
                resolveClasses(SBC, discreteBCs, classesSBC);
                sumSBC+=SBC;
                minmax( SBC, (*it), maxSBC, minSBC, maxNodeSBC, minNodeSBC) ;

                // Compute std CC, classes and min/maxSCC
                CC = (*it)->CC();
                sumCC+=CC;
                SCC = maxIndexCC * CC;
                (*it)->setSCC (  SCC );
                resolveClasses(SCC, discreteCCs, classesSCC,(*it)->number() );
                sumSCC+=SCC;
                minmax( SCC, (*it), maxSCC, minSCC, maxNodeSCC, minNodeSCC) ;

                //prepare to compute stdSC
                SC=(*it)->SC();
                if (m_graphIsSymmetric){
                    (*it)->setSC(SC/2.0);
                    SC=(*it)->SC();
                    qDebug() << "SC of " <<(*it)->number()
                             << "  divided by 2 (because the graph is symmetric) "
                             << (*it)->SC();
                }
                sumSC+=SC;

                qDebug() << "vertex " << (*it)->number() << " - "
                         << " EC: "<< (*it)->EC()
                         << " CC: "<< (*it)->CC()
                         << " BC: "<< (*it)->BC()
                         << " SC: "<< (*it)->SC()
                         << " PC: "<< (*it)->PC();
            } // end for

            qDebug() << "Graph: graphDistancesGeodesic() -"
                        "Computing mean centrality values...";

            // Compute mean values and prepare to compute variances
            meanSBC = sumSBC /(qreal) N ;
            varianceSBC=0;
            tempVarianceBC=0;

            meanSCC = sumSCC /(qreal) N ;
            varianceSCC=0;
            tempVarianceCC=0;

            meanSPC = sumSPC /(qreal) N ;
            varianceSPC=0;
            tempVariancePC=0;

            meanEC = sumEC /(qreal) N ;
            varianceEC=0;
            tempVarianceEC=0;

            qDebug() << "Graph: graphDistancesGeodesic() - "
                        "Computing std centralities ...";

            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ) {
                    continue;
                }
                // Compute std SC, classes and min/maxSSC
                SC=(*it)->SC();
                SSC=SC/sumSC;
                (*it)->setSSC(SSC);
                resolveClasses(SSC, discreteSCs, classesSSC);
                sumSSC+=SSC;
                minmax( SSC, (*it), maxSSC, minSSC, maxNodeSSC, minNodeSSC) ;

                //Compute numerator of groupSBC
                SBC=(*it)->SBC();
                nomSBC +=(maxSBC - SBC );

                //calculate BC variance
                tempVarianceBC = (  SBC  -  meanSBC  ) ;
                tempVarianceBC *=tempVarianceBC;
                varianceSBC  += tempVarianceBC;

                //Compute numerator of groupCC
                nomSCC += maxSCC- (*it)->SCC();

                //calculate CC variance
                tempVarianceCC = (  (*it)->SCC()  -  meanSCC  ) ;
                tempVarianceCC *=tempVarianceCC;
                varianceSCC  += tempVarianceCC;

                //Compute numerator of groupSPC
                SPC=(*it)->SPC();
                nomSPC +=(maxSPC - SPC );

                //calculate PC variance
                tempVariancePC = (  (*it)->SPC()  -  meanSPC  ) ;
                tempVariancePC *=tempVariancePC;
                varianceSPC  += tempVariancePC;


                //calculate EC variance
                tempVarianceEC = (  (*it)->EC()  -  meanEC  ) ;
                tempVarianceEC *=tempVarianceEC;
                varianceEC  += tempVarianceEC;


            } // end for

            //compute final variances
            varianceSBC  /=  (qreal) N;
            varianceSCC  /=  (qreal) N;
            varianceSPC  /=  (qreal) N;

            varianceEC  /=  (qreal) N;


            // calculate SC mean value and prepare to compute variance
            meanSSC = sumSSC /(qreal) N ;
            varianceSSC=0;
            tempVarianceSC=0;
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ){
                    continue;
                }
                tempVarianceSC = (  (*it)->SSC()  -  meanSSC  ) ;
                tempVarianceSC *=tempVarianceSC;
                varianceSSC  += tempVarianceSC;
            }
            //calculate final SC variance
            varianceSSC  /=  (qreal) N;

            denomSPC = (  (N-2.0) ) / (2.0 );   //only for connected nets
            if (N < 3 )
                denomSPC = N-1.0;
            //what if the net is disconnected (isolates exist) ?
            groupSPC = nomSPC/denomSPC;

            denomSCC = ( ( N-1.0) * (N-2.0) ) / (2.0 * N -3.0);
            if (N < 3 )
                denomSCC = N-1.0;

            groupCC = nomSCC/denomSCC;	//Calculate group Closeness centrality

            //nomSBC*=2.0;
            //            denomSBC =   (N-1.0) *  (N-1.0) * (N-2.0);
            denomSBC =   (N-1.0) ;  // Wasserman&Faust - formula 5.14
            groupSBC=nomSBC/denomSBC;		//Calculate group Betweenness centrality

            calculatedCentralities=true;

        }  // END if computeCentralities

    }  // END else (aka E!=0)



    calculatedDistances=true;

    qDebug() << "Graph::graphDistancesGeodesic()- FINISHED computing distances";


    emit signalProgressBoxKill();

}





/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

    INPUT:
        a 'source' vertex with vpos s and a boolean computeCentralities.
        (Implicitly, BFS uses the m_graph structure)

    OUTPUT:
        For every vertex t: d(s, t) is set to the distance of each t from s
        For every vertex t: s(s, t) is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then BFS does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list , thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from Q is pushed to a stack Stack

*/ 
void Graph::BFS(const int &s, const int &si,  const bool &computeCentralities,
                const bool &dropIsolates){
    Q_UNUSED(dropIsolates);

    qDebug()<< "BFS:";
    int u=0, ui=0 ,w=0, wi=0;
    int dist_u=0, temp=0, dist_w=0;
    int relation=0;
    //int  weight=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1;


    //set distance of s from s equal to 0
    m_graph[si]->setDistance(s,0);

    //set sigma of s from s equal to 1
    m_graph[si]->setShortestPaths(s,1);

    //    qDebug("BFS: Construct a queue Q of integers and push source vertex s=%i to Q as initial vertex", s);
    queue<int> Q;

    Q.push(s);

    qDebug()<< "BFS: LOOP: While Q not empty ";
    while ( !Q.empty() ) {

        u=Q.front(); Q.pop();
        ui=vpos[u];
        qDebug()<< "BFS: Dequeue: first element of Q is u"<<u<< "vpos"<< ui;

        if ( ! m_graph [ ui ]->isEnabled() ) {
            continue ;
        }

        if (computeCentralities){
            qDebug()<< "BFS: Compute centralities: Pushing u" << u
                    << "to global Stack ";
            Stack.push(u);
        }
        qDebug() << "BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u";
        it1=m_graph [ ui ]->m_outEdges.cbegin();
        while ( it1!=m_graph [ ui ]->m_outEdges.cend() ){
            relation = it1.value().first;
            if ( relation != relationCurrent() )  {
                ++it1;
                continue;
            }
            edgeStatus=it1.value().second.second;
            if ( edgeStatus != true)   {
                ++it1;
                continue;
            }
            w = it1.key();
            //  weight = it1.value().second.first;
            wi=vpos[ w ];
            qDebug("BFS: u=%i is connected with node w=%i of vpos wi=%i. ", u, w, wi);

            qDebug("BFS: Start path discovery");

            //if distance (s,w) is infinite, w found for the first time.
            if ( m_graph [ si ]->distance( w ) == RAND_MAX ) {

                qDebug("BFS: First time visiting w=%i. Enqueuing w to the end of Q", w);

                Q.push(w);

                qDebug()<<"BFS: First check if distance(s,u) = infinite and set it to zero";

                dist_u=m_graph [ si ]->distance( u );
                dist_w = dist_u + 1;

                qDebug() << "BFS: Setting dist_w = d ( s" << s << ", w"<<w
                         << ") equal to dist_u=d(s,u) plus 1. New dist_w" << dist_w ;
                ;
                m_graph[si]->setDistance(w,dist_w);


                m_graphSumDistance += dist_w;
                m_graphGeodesicsCount++;


                qDebug()<< "== BFS  - d("
                        << s <<"," << w
                        <<")=" <<
                          m_graph[si]->distance(w) ;

                if (computeCentralities){
                    qDebug()<<"BFS: Calculate PC: store the number of nodes at distance "
                           << dist_w << "from s";

                    sizeOfNthOrderNeighborhood.insert(
                                dist_w,
                                sizeOfNthOrderNeighborhood.value(dist_w,0)+1
                                );
                    qDebug()<<"BFS: Calculate CC: the sum of distances (will invert it l8r)";
                    m_graph [si]->setCC (m_graph [si]->CC() + dist_w);

                    qDebug()<<"BFS: Calculate Eccentricity: the maximum distance ";
                    if (m_graph [si]->eccentricity() < dist_w )
                        m_graph [si]->setEccentricity(dist_w);

                }
                //                qDebug("BFS: Checking m_graphDiameter");
                if ( dist_w > m_graphDiameter){
                    m_graphDiameter=dist_w;
                    //                    qDebug() << "BFS: new m_graphDiameter = " <<  m_graphDiameter ;
                }
            }

            qDebug()<< "BFS: Start path counting";

            //Is edge (u,w) on a shortest path from s to w via u?

            if ( m_graph[si]->distance(w) == m_graph[si]->distance(u) + 1) {

                temp=m_graph[si]->shortestPaths(w)+m_graph[si]->shortestPaths(u);

                qDebug()<< "BFS: Found a NEW SHORTEST PATH from s" << s
                        << "to w"<< w << "via u"<< u
                        << "Setting Sigma(s, w)"<< temp;
                if (s!=w) {
                    m_graph[si]->setShortestPaths(w, temp);
                }
                if (computeCentralities){
                    qDebug()<< "BFS/SC: Computing centralities: Computing SC ";
                    if ( s!=w && s != u && u!=w ) {
                        qDebug() << "BFS: setSC of u="<<u<<" to "<<m_graph[ui]->SC()+1;
                        m_graph[ui]->setSC( m_graph[ui]->SC()+1 );
                    }
                    else {
                        //                        qDebug() << "BFS/SC: skipping setSC of u, because s="
                        //                                 <<s<<" w="<< w << " u="<< u;
                    }
                    //                    qDebug() << "BFS/SC: SC is " << m_graph[u]->SC();
                    qDebug() << "BFS: appending u"<< u << " to list Ps[w=" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[wi]->appendToPs(u);
                }
            }
            ++it1;
        }

    }
}




/**
*	Dijkstra's algorithm for solving the SSSP problem in weighted graphs (directed or not).
*   It uses a min-priority queue prQ to provide constant time lookup of the minimum
*   distance. The priority queue is implemented with std::priority_queue

    INPUT:
        a 'source' vertex with vpos s and a boolean computeCentralities.
        (Implicitly, the algorithm uses the m_graph structure)

    OUTPUT:
        For every vertex t: d(s, t) is set to the distance of each t from s
        For every vertex t: s(s, t) is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then it does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from prQ is pushed to a stack Stack

*/
void Graph::dijkstra(const int &s, const int &si,
                     const bool &computeCentralities,
                     const bool &inverseWeights,
                     const bool &dropIsolates){

    Q_UNUSED(dropIsolates);

    int u=0,ui=0, w=0, wi=0, v=0, sp_w=0;
    int relation=0;
    qreal  weight=0, dist_u=0,  dist_w=0, cur_dist_w=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1;
    VList::const_iterator it;

    // Construct a priority queue where we will store discovered vertices along with their distances from source
    qDebug() << "### dijkstra: Construct a priority queue prQ to store discovered vertices-distances from source";

    // TODO: Check prQ functionality in weighted graphs, where edge weight denotes value (not cost)
    priority_queue<GraphDistance, vector<GraphDistance>, GraphDistancesCompare> prQ;

    // This is used to not allow duplicates in the priority queue (@see issue #123)
    QSet<int> visited_vertices;

    //set d( s, s ) = 0
    m_graph[si]->setDistance(s,0);

    //set sp ( s , s ) = 1
    m_graph[si]->setShortestPaths(s,1);


    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
        v=vpos[ (*it)->number() ];
        if (v != s ){
            // NOTE: d(i,j) init to RAND_MAX already done in graphDistancesGeodesic
            //            qDebug() << " push " << v << " to prQ with infinite distance from s";
            //            prQ.push(GraphDistance(v,RAND_MAX));

            //TODO // Previous node in optimal path from source
            //    previous[v]  := undefined
        }
    }
    qDebug() << "### dijkstra: push s" << s << "to prQ with 0 distance from s";
    //Note: without it the priority prQ would pop arbitrary node at first loop
    prQ.push(GraphDistance(s,0));

    qDebug() << "### dijkstra: LOOP: While prQ not empty ";
    while ( !prQ.empty() ) {

        qDebug()<< "    *** dijkstra: prQ size: "<< prQ.size();

        // Get the first vertex in the priority queue
        u=prQ.top().target;
        // Get the vertex index
        ui=vpos[u];

        // Pop it
        qDebug()<< "    *** dijkstra: first vertex in prQ is u"<< u << "vpos" << ui
                << ". It has minimum distance from s " << s << "=" << prQ.top().distance <<" Popping it from the queue.";
        prQ.pop();

        if ( visited_vertices.contains(u) ) {
            qDebug()<< "    *** dijkstra: vertex already visited. Skipping!";
            continue ;
        }
        // Add it to visited
        visited_vertices.insert(u);

        // Skip if that vertex is disabled
        if ( ! m_graph [ ui ]->isEnabled() ) {
            qDebug()<< "    *** dijkstra: vertex disabled. Skipping!";
            continue ;
        }

        // Check if we need to compute centralities
        if (computeCentralities){

            qDebug()<< "    *** dijkstra: Compute centralities: pushing u ="
                    << u
                    << " to global Stack ";

            Stack.push(u);
        }

        // LOOP over every edge of u
        qDebug() << "    --- dijkstra: LOOP over every edge of u ("<< u <<", w ) e E... ";
        it1=m_graph [ ui ]->m_outEdges.cbegin();
        while ( it1!=m_graph [ ui ]->m_outEdges.cend() ) {

            // Skip if the edge is not of the current relation
            relation = it1.value().first;
            if ( relation != relationCurrent() )  {
                ++it1;
                continue;
            }
            // Skip if the edge is disabled
            edgeStatus=it1.value().second.second;
            if ( edgeStatus != true)   {
                ++it1;
                continue;
            }

            // Get the target vertex of this edge and its index
            w = it1.key();
            wi=vpos[ w ];

            // Get the edge weight
            weight = it1.value().second.first;

            qDebug()<<"    --- dijkstra: edge (u, w) = ("<< u << ","<< w << ") =" << weight;

            // Invert edge weight if the user told us to do so
            if (inverseWeights) {
                weight = 1.0 / weight;
                qDebug () << "    --- dijkstra: inverting weight to " << weight;
            }

            // Start path discovery
            qDebug() <<"    --- dijkstra: Start path discovery";

            // Get the distance of u from source
            dist_u=m_graph [ si ]->distance( u );

            // If dist_u not finite, this means that dist_w also not finite
            if (dist_u == RAND_MAX || dist_u < 0) {
                dist_w = RAND_MAX;
                qDebug() << "    --- dijkstra: dist_w = RAND_MAX " << RAND_MAX;

            }
            else {
                // dist_u finite, therefore dist_w is (dist_u + edge weight)
                dist_w = dist_u + weight;
                qDebug() << "    --- dijkstra: dist_w = dist_u + weight = "
                         << dist_u << "+" << weight <<  "=" <<dist_w ;
            }

            // Get the currently computed distance of w from source
            cur_dist_w = m_graph [ si ]->distance( w );

            qDebug() << "    --- dijkstra: RELAXATION: check if dist_w =" << dist_w
                     <<  "  shorter than current d(s=" << s <<",w="<<w <<")="
                      << cur_dist_w;

            if ( ( dist_w == cur_dist_w ) &&  dist_w < RAND_MAX ) {

                qDebug() <<"    --- dijkstra: dist_w : " << dist_w
                        <<  " ==  current d(s,w) : " << cur_dist_w ;

                sp_w = m_graph[si]->shortestPaths(w) + m_graph[si]->shortestPaths(u);

                // WRONG! We do not know for sure that we are in a shortest path!!!
                qDebug() <<"    --- dijkstra: (POSSIBLE BUG?) Found ANOTHER SP from s ="
                        << s
                        << " to w=" << w << " via u="<< u
                        << " - Setting Sigma(s, w) = "<< sp_w;

                if (s!=w) {
                    m_graph[si]->setShortestPaths(w, sp_w);
                }

                if (computeCentralities){

                    if ( s!=w && s != u && u!=w ) {

                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                    "setSC of u" << u
                                 <<"to" << m_graph[ui]->SC()+1;

                        m_graph[ui]->setSC(m_graph[ui]->SC()+1);
                    }
                    else {
                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                    "Skipping setSC of u, because s="
                                 <<s<<" w="<< w << " u="<< u;
                    }
                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "SC is " << m_graph[ui]->SC();

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "Appending u="<< u << " to list Ps[w =" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[wi]->appendToPs(u);
                }
            }

            else if (dist_w > 0 && dist_w < cur_dist_w  ) {

                qDebug() <<"    --- dijkstra: dist_w " << dist_w
                        <<  " <  current d(s,w) =" << cur_dist_w
                         << " Pushing w" << w<< "to prQ with distance"<< dist_w << "from s"<<s;

                // FIXME: w might have been already visited?
                // If so, we might use QMap<int> which is sorted (minimum)
                // and also provides contain()
                prQ.push(GraphDistance(w,dist_w));

                m_graph[si]->setDistance(w,dist_w);

                m_graphGeodesicsCount++;

                qDebug() << "    --- dijkstra: "
                            "Set d ( s=" << s << ", w="<< w
                         << " ) = "<< dist_w << "="<< m_graph[si]->distance(w);

                if ( dist_w > m_graphDiameter){
                    m_graphDiameter=dist_w;
                    qDebug() << "    --- dijkstra: "
                                "New graph diameter =" << m_graphDiameter ;
                }

                if (s!=w) {
                    qDebug() << "    --- dijkstra: "
                              "Found NEW shortest path from s =" << s
                             << " to w =" << w << " via u ="<< u
                             << " - Setting Sigma(s, w) = 1 ";
                    m_graph[si]->setShortestPaths(w, 1);
                }


                if (computeCentralities){

                    sizeOfNthOrderNeighborhood.insert(
                                dist_w,
                                sizeOfNthOrderNeighborhood.value(dist_w,0)+1
                                );

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                              "For PC: sizeOfNthOrderNeighborhood: number of nodes at distance "
                           << dist_w << "from s is "
                           <<  sizeOfNthOrderNeighborhood.value(dist_w,0);

                    if (m_graph [si]->eccentricity() < dist_w ) {
                        m_graph [si]->setEccentricity(dist_w);
                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                  "For EC: max distance ="
                                  <<  m_graph [si]->eccentricity();
                    }

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "Appending u="<< u << " to list Ps[w =" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[wi]->appendToPs(u);

                }



            }
            else {
                qDebug() << "    --- dijkstra: "
                            "NOT a new SP";
            }

            ++it1;

        } // END loop for every outEdge of u

        qDebug() << "    --- dijkstra: LOOP END over every edge ("<< u <<", w ) e E... ";

    } // END loop while prQ not empty

    qDebug() << "### dijkstra: LOOP END. prQ is empty - Returning.";
}







/**
 * @brief Computes minimum and maximum centralities during graphDistancesGeodesic()
 * @param C
 * @param v
 * @param max
 * @param min
 * @param maxNode
 * @param minNode
 */
void Graph::minmax(qreal C, GraphVertex *v, qreal &max, qreal &min, int &maxNode, int &minNode) {
    qDebug() << "MINMAX C = " <<  C << "  max = " << max << "  min = " << min << " name = " <<  v->number();
    if (C > max ) {
        max=C;
        maxNode=v->number();
    }
    if (C < min ) {
        min=C;
        minNode=v->number();
    }
}





/**
 * @brief Checks if score C is a new prominence class
 * If yes, it stores that number in a QHash<QString,int> type where the score is the key.
 * If no, increases the frequency of this prominence score by 1
 * Called from graphDistancesGeodesic()
 * @param C
 * @param discreteClasses
 * @param classes
 */
void Graph::resolveClasses(qreal C, H_StrToInt &discreteClasses, int &classes){
    int frq = 0;
    H_StrToInt::iterator it2;
    it2 = discreteClasses.find(QString::number(C));    //Amort. O(1) complexity
    if (it2 == discreteClasses.end() )	{
        classes++;
        discreteClasses.insert(QString::number(C), 1);
    }
    else {
        frq = it2.value() ;
        discreteClasses.insert(QString::number(C), frq + 1);
    }
}



/**
 * @brief Overloaded method. It only adds displaying current vertex for debugging purposes.
 * @param C
 * @param discreteClasses
 * @param classes
 * @param vertex
 */
void Graph::resolveClasses(qreal C, H_StrToInt &discreteClasses, int &classes, int vertex){
    int frq = 0;
    H_StrToInt::iterator it2;
    Q_UNUSED(vertex);
    it2 = discreteClasses.find(QString::number(C));    //Amort. O(1) complexity
    if (it2 == discreteClasses.end() )	{
        classes++;
        discreteClasses.insert(QString::number(C), 1);
    }
    else {
        frq = it2.value() ;
        discreteClasses.insert(QString::number(C), frq + 1);
    }
}







/**
 * @brief
 * Writes the matrix of distances to a file
 * @param fn
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeMatrixDistancesPlainText (const QString &fn,
                                           const bool &considerWeights,
                                           const bool &inverseWeights,
                                           const bool &dropIsolates) {

    qDebug() << "I will write distances matrix (plain-text) to file:" << fn << "First compute the distances matrix...";

    graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);

    qDebug() << "Writing distances matrix (plain-text) to file:" << fn;

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    QTextStream outText(&file);

    outText.setRealNumberPrecision(m_reportsRealPrecision);
    outText << "-Social Network Visualizer "<<  VERSION << "\n";
    outText << tr("Network name: ")<< getName()<< "\n\n";
    outText << "Distance matrix: \n";

    outText << DM ;

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
                                              const bool &inverseWeights) {

    qDebug() << "I will write shortest paths matrix (plain-text) to file:" << fn << "First compute the shortest paths matrix...";

    graphMatrixShortestPathsCreate( considerWeights, inverseWeights, false);

    qDebug() << "Writing shortest paths matrix (plain-text) to file:" << fn;

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer "<<  VERSION <<"- \n";
    outText << tr("Network name: ")<< getName() <<" \n\n";
    outText << "Shortest paths matrix: \n";

    outText << SIGMA ;

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

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    if ( !calculatedCentralities  ) {
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);

    }

    int progressCounter=0;
    int rowCount=0;
    int N = vertices();
    qreal eccentr=0;

    QString pMsg = tr("Writing Eccentricity scores to file. \nPlease wait...");
    emit statusMessage ( pMsg );

    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("ECCENTRICITY (e) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("1 &le; e &le; \xE2\x88\x9E")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;\">"
            << tr("Actor")
            << "</th>"
            << "<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc2 = 1; asc1 = 1;asc4 = 1;\">"
            << tr("e")
            << "</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody  id=\"results\">";


    VList::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);
        rowCount++;
        eccentr = (*it)->eccentricity();
        qDebug() << "Graph::writeEccentricity() - actor "
                 << (*it)->number()
                 << "eccentricity"
                 << eccentr;

        if ( ! (*it)->isEnabled() ) {
            qDebug() << "Graph::writeEccentricity() - actor disabled. SKIP.";
            continue; // do not print disabled nodes
        }

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                <<"<td>"
                << (*it)->number()
                << "</td><td>"
                << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                << "</td><td>"
                << ((eccentr == 0 || eccentr == RAND_MAX ) ? "\xE2\x88\x9E" : QString::number(eccentr) )
                << "</td>"
                <<"</tr>";

    }

    outText << "</tbody></table>";

    if ( minEccentricity ==  maxEccentricity) {
        outText << "<p>"
                << tr("All nodes have the same eccentricity.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max e (Graph Diameter) = ")
                <<"</span>"
               << maxEccentricity <<" (node "<< maxNodeEccentricity  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min e (Graph Radius) = ")
               <<"</span>"
               << minEccentricity <<" (node "<< minNodeEccentricity <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("e classes = ")
               <<"</span>"
               << classesEccentricity
               << "</p>";
    }

    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("e = 1 ")
            <<"</span>"
            << tr("when the node is connected to all others (star node).")
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("e > 1 ")
            <<"</span>"
            << tr("when the node is not directly connected to all others. "
                  "Larger eccentricity means the actor is farther from others.")
            <<"<br />"
           << "<span class=\"info\">"
            << tr("e = \xE2\x88\x9E ")
            <<"</span>"
            << tr("there is no path from that node to one or more other nodes.")
            <<"<br/>";
    outText << "</p>";


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Eccentricity Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();
}



/**
 * @brief Computes the Information centrality of each vertex - diagonal included
 *  Note that there is no known generalization of Stephenson&Zelen's theory
 *  for information centrality to directional data
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityInformation(const bool considerWeights,
                                  const bool inverseWeights){

    qDebug()<< "Graph::centralityInformation()";

    if ( calculatedIC ) {
        qDebug()<< "Graph::centralityInformation() - already computed. Return.";
        return;
    }

    discreteICs.clear();
    sumIC=0;
    maxIC=0;
    t_sumIC=0;
    minIC=RAND_MAX;
    classesIC=0;
    varianceIC=0;

    VList::const_iterator it;

    int i=0, j=0;

    qreal m_weight=0, weightSum=1, diagonalEntriesSum=0, rowSum=0;
    qreal IC=0, SIC=0;
    /* Note: isolated nodes must be dropped from the AM
        Otherwise, the SIGMA matrix might be singular, therefore non-invertible. */
    bool dropIsolates=true;
    bool symmetrize=true;
    int n=vertices(dropIsolates,false,true);

    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);

    QString pMsg = tr("Computing Information Centralities. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(n,pMsg);

    WM.resize( n, n );
    invM.resize(n, n);

    for (i=0; i<n; i++){
        weightSum = 1;
        for (j=0; j<n; j++){
            if ( i == j )
                continue;
            m_weight = AM.item(i,j);
            weightSum += m_weight; //sum of weights for all edges incident to i
            WM.setItem(i,j,1-m_weight);
        }
        WM.setItem(i,i,weightSum);
    }

    emit signalProgressBoxUpdate(n/3);
    emit statusMessage ( tr("Computing inverse adjacency matrix. Please wait...") );

    invM.inverse(WM);

    emit statusMessage ( tr("Computing IC scores. Please wait...") );

    emit signalProgressBoxUpdate(2*n/3);

    diagonalEntriesSum = 0;
    rowSum = 0;
    for (j=0; j<n; j++){
        rowSum += invM.item(0,j);
    }
    for (i=0; i<n; i++){
        diagonalEntriesSum  += invM.item(i,i);  // calculate the matrix trace
    }


    i=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( (*it)->isIsolated() ) {
            (*it)->setIC ( 0 );
            continue;
        }
        IC= 1.0 / ( invM.item(i,i) + (diagonalEntriesSum - 2.0 * rowSum) / n );

        (*it)->setIC ( IC );
        t_sumIC += IC;
        i++;
    }
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        IC = (*it)->IC();
        SIC = IC / t_sumIC ;
        (*it)->setSIC( SIC );
        sumIC+=SIC;
        resolveClasses(SIC, discreteICs, classesIC);
        minmax( SIC, (*it), maxIC, minIC, maxNodeIC, minNodeIC) ;
    }

    qreal x=0;
    meanIC = sumIC /static_cast<qreal> (n) ;

    varianceIC=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        x = (  (*it)->SIC()  -  meanIC  ) ;
        x *=x;
        varianceIC  += x;
    }

    varianceIC  /=  (qreal) n;

    calculatedIC = true;

    WM.clear();

    emit signalProgressBoxUpdate(n);
    emit signalProgressBoxKill();
}




/**
 * @brief Writes the information centralities to file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::writeCentralityInformation(const QString fileName,
                                       const bool considerWeights,
                                       const bool inverseWeights){

    qDebug() << "Writing Information Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }

    QTextStream outText ( &file );

    centralityInformation(considerWeights, inverseWeights);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::IC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    bool dropIsolates = true;  // by default IC needs to exclude isolates

    int rowCount=0;
    int N = vertices(dropIsolates, false, true);

    int progressCounter = 0;

    QString pMsg = tr("Writing Information Centralities to file. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("INFORMATION CENTRALITY (IC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The IC index, introduced by Stephenson and Zelen (1991), measures the "
                  "information flow through all paths between actors weighted by "
                  "strength of tie and distance.")
            << "<br />"
            << tr("IC' is the standardized index (IC divided by the sumIC).")
            << "<br />"
            << tr ("Warning: To compute this index, SocNetV drops all isolated "
                  "nodes and symmetrizes (if needed) the adjacency matrix. <br />"
                  "Read the Manual for more.")
            << "</p>";



    outText << "<p>"
            << "<span class=\"info\">"
            << tr("IC range: ")
            <<"</span>"
            << tr("0 &le; IC &le; \xE2\x88\x9E")
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("IC' range: ")
            <<"</span>"
            << tr("0 &le; IC' &le; 1")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("IC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("IC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%IC")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if ((*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {

            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << (*it)->IC()
                   << "</td><td>"
                   << (*it)->SIC()
                   << "</td><td>"
                   << (100* ((*it)->SIC()))
                   << "</td>"
                   <<"</tr>";
        }
    }


    outText << "</tbody></table>";

    if ( minIC ==  maxIC) {
        outText << "<p>"
                << tr("All nodes have the same IC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max IC' = ")
                <<"</span>"
               << maxIC <<" (node "<< maxNodeIC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min IC' = ")
               <<"</span>"
               << minIC <<" (node "<< minNodeIC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("IC classes = ")
               <<"</span>"
               << classesIC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("IC' Sum = ")
            <<"</span>"
            << sumIC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("IC' Mean = ")
            <<"</span>"
            << meanIC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("IC' Variance = ")
            <<"</span>"
            << varianceIC
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
            <<"</span>"
            <<  varianceIC
             << "</p>";


    outText << "<p class=\"description\">"
            << tr("Variance = 0, when all nodes have the same IC value, i.e. a "
                  "complete or a circle graph). <br />")
            << tr("Larger values of variance suggest larger variability between the "
                  "IC' values. <br />")
            <<"(Wasserman & Faust, formula 5.20, p. 197)\n\n"
            << "</p>";


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Information Centrality report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

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
                                       const bool &dropIsolates){

    qDebug() << "Writing Eigenvector Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    centralityEigenvector(considerWeights, inverseWeights,dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::EVC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Eigenvector Centrality scores to file. \nPlease wait...") ;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("EIGENVECTOR CENTRALITY (EVC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; EVC &lt; 1 (The eigenvector has unit euclidean length) ")
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("EVC' range: ")
            <<"</span>"
            << tr("0 &le; EVC' &le; 1")
            << "</p>";



    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("EVC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;asc6 = 1;\">"
            << tr("EVC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc6 = 1;\">"
            << tr("EVC''")
            << "</th>"
            <<"<th id=\"col6\" onclick=\"tableSort(results, 5, asc6); asc6 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("%EVC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";


    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                <<"<td>"
                << (*it)->number()
                << "</td><td>"
                << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                << "</td><td>"
                << (*it)->EVC()
                << "</td><td>"
                << (*it)->SEVC()
                << "</td><td>"
                << (*it)->EVC() / ( sumEVC ? sumEVC : 1)
                << "</td><td>"
                << (100* ((*it)->SEVC()))
                << "</td>"
                <<"</tr>";

    }


    outText << "</tbody></table>";

    if ( minEVC ==  maxEVC) {
        outText << "<p>"
                << tr("All nodes have the same EVC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max EVC = ")
                <<"</span>"
               << maxEVC <<" (node "<< maxNodeEVC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min EVC = ")
               <<"</span>"
               << minEVC <<" (node "<< minNodeEVC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("EVC classes = ")
               <<"</span>"
               << classesEVC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("EVC Sum = ")
            <<"</span>"
            << sumEVC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("EVC Mean = ")
            <<"</span>"
            << meanEVC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("EVC Variance = ")
            <<"</span>"
            << varianceEVC
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
            <<"</span>"
            <<  varianceEVC
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}



/**
 * @brief Computes Eigenvector centrality
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityEigenvector(const bool &considerWeights,
                                  const bool &inverseWeights,
                                  const bool &dropIsolates) {

    if ( calculatedEVC ) {
        qDebug() << "Graph not changed - EVC already computed. Return.";
        return;
    }

    qDebug()<< "(Re)Computing Eigenvector centrality scores...";

    emit statusMessage ( (tr("Calculating EVC scores...")) );

    classesEVC=0;
    discreteEVCs.clear();
    sumEVC=0;
    maxEVC=0;
    minEVC=RAND_MAX;
    varianceEVC=0;
    meanEVC=0;
    VList::const_iterator it;

    bool symmetrize=false;
    bool useDegrees=false;
    int i = 0;
    int N = vertices(dropIsolates);

    qreal *EVC = new (nothrow) qreal [N];
    Q_CHECK_PTR( EVC );
    qreal SEVC = 0;

    createMatrixAdjacency(dropIsolates, considerWeights,
                               inverseWeights, symmetrize);


    QString pMsg = tr("Computing Eigenvector Centrality scores. \nPlease wait...") ;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    if (useDegrees) {

        qDebug() << "Using outDegree for initial EVC vector";

        emit statusMessage(tr("Computing outDegrees. Please wait..."));

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

            if (!(*it)->isIsolated() && dropIsolates) {
                continue;
            }

            EVC[i] = (*it)->degreeOut();

            i++;
        }

    }
    else {
        qDebug() << "Using unit initial EVC vector";
        for (int i = 0 ; i < N ; i++) {
            EVC[i] = 1;
        }

    }

    emit signalProgressBoxUpdate( N / 3);

    AM.powerIteration(EVC, sumEVC, maxEVC, maxNodeEVC,
                      minEVC, minNodeEVC,
                      0.0000001, 500);

    emit signalProgressBoxUpdate(2 * N / 3);

    emit statusMessage(tr("Leading eigenvector computed. "
                          "Analysing centralities. Please wait..."));

    i = 0;

    meanEVC = sumEVC / (qreal) N;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        if (!(*it)->isIsolated() && dropIsolates) {
            continue;
        }

        (*it)->setEVC( EVC[i]);
        if ( maxEVC != 0 ) {
            SEVC = EVC[i] / maxEVC ;
        }
        else {
            SEVC = 0 ;
        }

        (*it)->setSEVC( SEVC );

        resolveClasses(SEVC, discreteEVCs, classesEVC);

        varianceEVC += (EVC[i]-meanEVC) * (EVC[i]-meanEVC) ;

        i++;

    }

    varianceEVC=varianceEVC/(qreal) N;


    // group eigenvector centralization measure is
    // S(cmax - c(vi)) divided by the maximum value possible,
    // where c(vi) is the eigenvector centrality of vertex vi.

    calculatedEVC=true;

    delete [] EVC;

    emit signalProgressBoxUpdate( N );
    emit signalProgressBoxKill();
}






/**
 * @brief Calculates the degree (outDegree) centrality of each vertex - diagonal included
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::centralityDegree(const bool &considerWeights, const bool &dropIsolates){

    if ( calculatedDC ) {
        qDebug() << "Graph not changed - no need to recompute degree centralities. Returning.";
        return;
    }
    qreal DC=0, nom=0, denom=0,  SDC=0;
    qreal weight;
    classesSDC=0;
    discreteSDCs.clear();
    sumSDC=0;
    sumDC=0;
    maxSDC=0;
    minSDC=RAND_MAX;
    varianceSDC=0;
    meanSDC=0;
    int N=vertices(dropIsolates);

    VList::const_iterator it, it1;

    QString pMsg =  tr("Computing out-Degree Centralities for %1 nodes. \nPlease wait...").arg(N);
    qDebug() << pMsg;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    emit signalProgressBoxUpdate(N/3);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        DC=0;

        if ( ! (*it)->isEnabled() || ( dropIsolates && (*it)->isIsolated()) ) {
            continue;
        }

        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

            if ( ! (*it1)->isEnabled() || ( dropIsolates && (*it1)->isIsolated()) ) {
                continue;
            }

            if ( (weight=edgeExists( (*it)->number(), (*it1)->number() ) ) != 0.0  )   {
                if (considerWeights)
                    DC+=weight;
                else
                    DC++;

                //check here if the matrix is symmetric - we need this below
                if (  weight != edgeExists ( (*it1)->number(), (*it)->number()) )
                    m_graphIsSymmetric = false;
            }
        }

        (*it)->setDC ( DC ) ;	//Set OutDegree

        sumDC += DC;          // store sumDC (for std calc below)
    }

    emit signalProgressBoxUpdate(2*N/3);

    // Calculate std Out-Degree, min, max, classes and sumSDC
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        DC= (*it)->DC();
        if (!considerWeights) {
            SDC = ( DC / (N-1.0) );
        }
        else {
            SDC= ( DC / (sumDC) );
        }
        (*it)->setSDC( SDC );		//Set Standard DC

        qDebug() << "vertex"<<  (*it)->number() << "-- DC=" << DC << "SDC=" << SDC ;
        sumSDC+=SDC;

        resolveClasses(SDC, discreteSDCs, classesSDC );

        if (maxSDC < SDC ) {
            maxSDC = SDC ;
            maxNodeSDC=(*it)->number();
        }
        if (minSDC > SDC ) {
            minSDC = SDC ;
            minNodeSDC=(*it)->number();
        }
    }

    if (minSDC == maxSDC)
        maxNodeSDC=-1;

    meanSDC = sumSDC / (qreal) N;

    // Calculate Variance and the Degree Centralization of the whole graph.
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        SDC= (*it)->SDC();
        nom+= (maxSDC-SDC);
        varianceSDC += (SDC-meanSDC) * (SDC-meanSDC) ;


    }
    varianceSDC=varianceSDC/(qreal) N;

    if (m_graphIsSymmetric) {
        // we divide by N-1 because we use std C values
        denom= (N-1.0)*(N-2.0)  / (N-1.0);
    }
    else {
        denom=(N-1.0)*(N-1.0)   / (N-1.0);
    }

    if (N < 3 ) {
        denom = N-1.0;
    }

    if (!considerWeights) {
        groupDC=nom/denom;
    }

    calculatedDC=true;

    emit signalProgressBoxUpdate(N);
    emit signalProgressBoxKill();

}


/**
 * @brief Returns the IndexType of the given prominence index name
 * Called from MW::slotEditNodeFind, MW::slotLayoutRadialByProminenceIndex etc
 * @param prominenceIndexName
 */
int Graph::getProminenceIndexByName(const QString &prominenceIndexName) {

    qDebug()<< "Returning index type for index named: " << prominenceIndexName;

    if ( prominenceIndexName.contains("Degree Centr") ){
        return IndexType::DC;
    }
    else if ( prominenceIndexName.contains("Closeness Centr") &&
              !prominenceIndexName.contains("IR")){
        return IndexType::CC;
    }
    else if ( prominenceIndexName.contains("Influence Range Closeness Centrality")  ||
              prominenceIndexName.contains("IR Closeness")
              ){
        return IndexType::IRCC;
    }
    else if ( prominenceIndexName.contains("Betweenness Centr")){
        return IndexType::BC;
    }
    else if (prominenceIndexName.contains("Stress Centr")){
        return IndexType::SC;
    }
    else if (prominenceIndexName.contains("Eccentricity Centr")){
        return IndexType::EC;
    }
    else if (prominenceIndexName.contains("Power Centr")){
        return IndexType::PC;
    }
    else if (prominenceIndexName.contains("Information Centr")){
        return IndexType::IC;
    }
    else if (prominenceIndexName.contains("Eigenvector Centr")){
        return IndexType::EVC;
    }
    else if (prominenceIndexName.contains("Degree Prestige")){
        return IndexType::DP;
    }
    else if (prominenceIndexName.contains("PageRank Prestige")){
        return IndexType::PRP;
    }
    else if (prominenceIndexName.contains("Proximity Prestige")){
        return IndexType::PP;
    }
    else
        return 0;


}


/**
 * @brief Computes the distribution of a centrality index score.
 * The distribution is stored as Qt Series depending on the SeriesType parameter type
 * It is send to MW through signal/slot
 * @param index
 * @param type
 */
void Graph::prominenceDistribution(const int &index,
                                   const ChartType &type,
                                   const QString &distImageFileName) {

    qDebug() << "Request to compute prominence distribution. "
             << "index" << index
             << "chart type: " << type
             << "distImageFileName" << distImageFileName;

    QString pMsg = tr("Computing Centrality Distribution. \nPlease wait...");
    emit statusMessage(  pMsg );

    H_StrToInt discreteClasses;

    QString seriesName;

    qDebug() << "setting prominence distribution series name and classes...";
    switch (index) {
    case 0: {
        break;
    }
    case IndexType::DC : {
        seriesName = ("(out)Degree");
        discreteClasses = discreteSDCs;
        break;
    }
    case IndexType::CC : {
        seriesName = ("Closeness");
        discreteClasses = discreteCCs;
        break;
    }
    case IndexType::IRCC : {
        seriesName = ("IRCC");
        discreteClasses = discreteIRCCs;
        break;
    }
    case IndexType::BC : {
        seriesName = ("Betweenness");
        discreteClasses = discreteBCs;
        break;
    }
    case IndexType::SC : {
        seriesName = ("Stress");
        discreteClasses = discreteSCs;
        break;
    }
    case IndexType::EC : {
        seriesName = ("Eccentricity");
        discreteClasses = discreteECs;
        break;
    }
    case IndexType::PC : {
        seriesName = ("Power");
        discreteClasses = discretePCs;
        break;
    }
    case IndexType::IC : {
        seriesName = ("Information");
        discreteClasses = discreteICs;
        break;
    }
    case IndexType::EVC : {
        seriesName = ("Eigenvector");
        discreteClasses = discreteEVCs;
        break;
    }
    case IndexType::DP : {
        seriesName = ("Prestige Degree");
        discreteClasses = discreteDPs;
        break;
    }
    case IndexType::PRP : {
        seriesName = ("Pagerank");
        discreteClasses = discretePRPs;
        break;
    }
    case IndexType::PP : {
        seriesName = ("Proximity");
        discreteClasses = discretePPs;
        break;
    }
    }

    qDebug() << "calling the relevant prominence distribution computation method...";
    switch (type) {
    case ChartType::None:
        emit signalPromininenceDistributionChartUpdate(Q_NULLPTR, Q_NULLPTR);
        break;
    case ChartType::Spline:
        emit statusMessage(tr("Creating prominence index distribution line chart..."));
        prominenceDistributionSpline(discreteClasses, seriesName,distImageFileName  );
        break;
    case ChartType::Area:
        emit statusMessage(tr("Creating prominence index distribution area chart..."));
        prominenceDistributionArea(discreteClasses, seriesName, distImageFileName );
        break;
    case ChartType::Bars:
        emit statusMessage(tr("Creating prominence index distribution bar chart..."));
        prominenceDistributionBars(discreteClasses, seriesName, distImageFileName );
        break;
    }


}





/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QSplineSeries series to MW
 * which in turn displays them on a Spline Chart
 * @param index
 * @param series
 */
void Graph::prominenceDistributionSpline(const H_StrToInt &discreteClasses,
                                         const QString &seriesName,
                                         const QString &distImageFileName) {

    qDebug() << "Computing prominence distribution as spline chart...";

    QLineSeries *series = new QLineSeries();
    series->setName (seriesName);
    QValueAxis *axisX = new QValueAxis ();
    QValueAxis *axisY = new QValueAxis ();

    // Used only for the large chart exported to PNG for the HTML report
    QLineSeries *series1 = new QLineSeries();
    series1->setName (seriesName);
    QValueAxis *axisX1 = new QValueAxis ();
    QValueAxis *axisY1 = new QValueAxis ();

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i) {

        qDebug() << "discreteClasses: "
                 << i.key() << ": " << i.value() ;

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));

    }

    unsigned int initialSize = seriesPQ.size();
    qreal min = 0;
    qreal max = 0;
    qreal value = 0;

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty()) {
        qDebug() << "seriesPQ top is:"
                 << seriesPQ.top().value << " : "
                 << seriesPQ.top().frequency;

        value =  seriesPQ.top().value;
        frequency = seriesPQ.top().frequency;

        series->append( value,  frequency );
        series1->append( value,  frequency );

        if ( frequency < minF ) {
            minF = frequency;
        }
        if ( frequency > maxF ) {
            maxF = frequency;
        }

        if ( initialSize == seriesPQ.size() ) {
            min = value;
        }
        if ( seriesPQ.size() == 1 ) {
            max = value;
        }


        seriesPQ.pop();
    }

    axisX->setMin(min);
    axisX->setMax(max);

    axisY->setMin(minF);
    axisY->setMax(maxF+1.0);


    QPen sPen (QColor( "#209fdf" ));
    sPen.setWidthF(0.9);
    QBrush sBrush( QColor( "#ff0000") );

    series->setBrush(sBrush);
    series->setPen(sPen);

    if (!distImageFileName.isEmpty() ) {
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving prominence distribution image to" << distImageFileName ;

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF+1.0);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView( chart );

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        // chartView->show();

        chart->addSeries(series1);

        chart->setTitle(series1->name() + " distribution");
        chart->setTitleFont(QFont("Times",12));

        chart->legend()->hide();

        //chart->createDefaultAxes();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560,1440);
        chartView->resize(2561,1441);

        QPixmap p = chartView->grab();

        p.save( distImageFileName, "PNG");

        chartView->hide();
        // Do not delete the ChartView
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        //        chartView->deleteLater();
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution spline chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}



/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QAreaSeries series to MW
 * which in turn displays them on a Area Chart
 * @param index
 * @param series
 */
void Graph::prominenceDistributionArea(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName) {

    qDebug() << "Computing prominence distribution as area chart...";

    QAreaSeries *series = new QAreaSeries ();
    series->setName (name);
    QLineSeries *upperSeries = new QLineSeries();
    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();

    // Used only for the large chart exported to PNG for the HTML report
    QAreaSeries *series1 = new QAreaSeries ();
    series1->setName (name);
    QValueAxis *axisX1 = new QValueAxis();
    QValueAxis *axisY1 = new QValueAxis();

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i) {

        qDebug() << "discreteClasses: " << i.key() << ": " << i.value() << "\n";

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));

    }

    unsigned int initialSize = seriesPQ.size();
    qreal min = 0;
    qreal max = 0;
    qreal value = 0;

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty()) {

        qDebug() << seriesPQ.top().value << " : "
                 << seriesPQ.top().frequency << "\n";

        value =  seriesPQ.top().value;
        frequency = seriesPQ.top().frequency;

        upperSeries->append( value,  frequency );

        if ( frequency < minF ) {
            minF = frequency;
        }
        if ( frequency > maxF ) {
            maxF = frequency;
        }

        if ( initialSize == seriesPQ.size() ) {
            min = value;
        }
        if ( seriesPQ.size() == 1 ) {
            max = value;
        }


        seriesPQ.pop();
    }

    axisX->setMin(min);
    axisX->setMax(max);

    axisY->setMin(minF);
    axisY->setMax(maxF+1.0);

    series->setUpperSeries(upperSeries);

    QPen sPen (QColor( "#666" ));
    sPen.setWidthF(0.2);
    QBrush sBrush( QColor( "#209fdf") );

    series->setBrush(sBrush);
    series->setPen(sPen);


    if (!distImageFileName.isEmpty() ) {
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving distribution image to" << distImageFileName ;

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF+1.0);

        series1->setUpperSeries(upperSeries);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView( chart );

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        //chartView->show();

        chart->addSeries(series1);

        chart->setTitle(series->name() + " distribution");
        chart->setTitleFont(QFont("Times",12));

        chart->legend()->hide();

        // chart->createDefaultAxes();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560,1440);
        chartView->resize(2561,1441);

        QPixmap p = chartView->grab();

        p.save( distImageFileName, "PNG");

        chartView->hide();

        // Do not delete the ChartView
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        //        chartView->deleteLater();
        delete chartView;

    }

    qDebug() << "emitting signal to MW update the prominence distribution area chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}



/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QBarSeries series (with a QBarSet attached)
 * to MW  which in turn displays them on a Bar Chart
 * @param index
 * @param series
 * @param set
 * @param strX
 */
void Graph::prominenceDistributionBars(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName) {

    qDebug() << "Computing prominence distribution as bar chart...";

    QBarSeries *series = new QBarSeries();
    series->setName (name);
    QBarSet *barSet = new QBarSet("");
    QValueAxis *axisY = new QValueAxis;
    QBarCategoryAxis *axisX = new QBarCategoryAxis();

    // Used only for the large chart exported to PNG for the HTML report
    QBarSeries *series1 = new QBarSeries();
    series1->setName (name);
    QBarSet *barSet1 = new QBarSet("");
    QValueAxis *axisY1 = new QValueAxis;
    QBarCategoryAxis *axisX1 = new QBarCategoryAxis();

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i) {

        qDebug() << "discreteClasses: " << i.key() << ": " << i.value() << "\n";

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));

    }

    unsigned int initialSize = seriesPQ.size();

    QString min = QString();
    QString max = QString();
    QString value = QString();

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty()) {

        value = QString::number(  seriesPQ.top().value, 'f', 6);

        frequency = seriesPQ.top().frequency;

        qDebug() << "value:"<< value << " : "
                 << "frequency:"<< frequency << "\n";

        axisX->append( value );
        barSet->append( frequency );

        if (!distImageFileName.isEmpty()) {
            axisX1->append( value );
            barSet1->append( frequency );
        }

        if ( frequency < minF ) {
            minF = frequency;
        }
        if ( frequency > maxF ) {
            maxF = frequency;
        }

        if ( initialSize == seriesPQ.size() ) {
            min = value;
        }
        if ( seriesPQ.size() == 1 ) {
            max = value;
        }

        seriesPQ.pop();

    } // end while

    axisX->setMin(min);
    axisX->setMax(max);

    axisY->setMin(minF);
    axisY->setMax(maxF+1.0);

    qDebug() << "axisX min: " << axisX->min() << " max: " << axisX->max();

    series->append( barSet );

    QPen sPen (QColor( "#666" ));
    sPen.setWidthF(0.2);
    QBrush sBrush( QColor( "#209fdf") );

    barSet->setBrush(sBrush);
    barSet->setPen(sPen);


    if (!distImageFileName.isEmpty() ) {
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving distribution image to" << distImageFileName ;

        series1->append( barSet1 );


        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF+1.0);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView( chart );

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        //chartView->show();

        chart->addSeries(series1);

        chart->setTitle(series->name() + " distribution");
        chart->setTitleFont(QFont("Times",12));

        chart->legend()->hide();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560,1440);
        chartView->resize(2561,1441);

        QPixmap p = chartView->grab();

        p.save( distImageFileName, "PNG");

        chartView->hide();
        // Do not delete the ChartView if the axes / series are the same!
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution bar chart";
    emit signalPromininenceDistributionChartUpdate(series,
                                                   axisX, min.toDouble(), max.toDouble(),
                                                   axisY, minF, maxF);
}




/**
 * @brief Writes the Degree Centrality to a file
 * @param fileName
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::writeCentralityDegree ( const QString fileName,
                                    const bool considerWeights,
                                    const bool dropIsolates) {

     qDebug() << "Writing Degree Centrality report to file:" << fileName
              << "considerWeights:" << considerWeights
              << "dropIsolates:" <<dropIsolates;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    centralityDegree(considerWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::DC, m_reportsChartType,distImageFileName);

    qreal maxIndexDC=vertices(dropIsolates)-1.0;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;
    VList::const_iterator it;

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    QString pMsg =  tr("Writing out-Degree Centralities. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText << "<h1>";
    outText << tr("DEGREE CENTRALITY (DC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; DC &le; ");
    if (considerWeights) outText<< infinity;
    else outText << maxIndexDC;
    outText << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DC' range: ")
            <<"</span>"
            << tr("0 &le; DC' &le; 1")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("DC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("DC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%DC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";


    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        rowCount++;

        emit signalProgressBoxUpdate(++progressCounter);

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->DC()
                    << "</td><td>"
                    << (*it)->SDC()
                    << "</td><td>"
                    << (100* ((*it)->SDC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minSDC ==  maxSDC) {
        outText << "<p>"
                << tr("All nodes have the same DC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("DC Sum = ")
                <<"</span>"
                << sumDC
                <<"</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max DC' = ")
                <<"</span>"
               << maxSDC <<" (node "<< maxNodeSDC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min DC' = ")
               <<"</span>"
               << minSDC <<" (node "<< minNodeSDC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("DC' classes = ")
               <<"</span>"
               << classesSDC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("DC' Sum = ")
            <<"</span>"
            << sumSDC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("DC' Mean = ")
            <<"</span>"
            << meanSDC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("DC' Variance = ")
            <<"</span>"
            << varianceSDC
            <<"<br/>";
    outText << "</p>";



    if ( m_reportsChartType != ChartType::None ) {
        outText << "<h2>";
        outText << tr("DC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights) {
        outText << "<h2>";
        outText << tr("GROUP DEGREE CENTRALIZATION (GDC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GDC = ")
                <<"</span>"
                <<  groupDC
                 << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GDC range: ")
                <<"</span>"
                <<" 0 &le; GDC &le; 1"
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();
}





/**
 * @brief Writes the closeness centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityCloseness( const QString fileName,
                                      const bool considerWeights,
                                      const bool inverseWeights,
                                      const bool dropIsolates) {

    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug() << "Writing closeness Centrality report to file:" << fileName
             << "considerWeights"<<considerWeights
             << "inverseWeights"<<inverseWeights
             << "dropIsolates" << dropIsolates
             << "m_reportsLabelLength" <<m_reportsLabelLength;

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::CC, m_reportsChartType,distImageFileName);

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Closeness Centrality scores to file. \nPlease wait ...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("CLOSENESS CENTRALITY (CC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";

    outText << "<p class=\"description\">"
            << tr("The CC index is the inverted sum of geodesic distances "
                  "from each node u to all other nodes. " )
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
            <<"</span>"
            << tr("0 &le; CC &le; ")<< 1.0/maxIndexCC
            << tr(" ( 1 / Number of node pairs excluding u)")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CC' range: ")
            <<"</span>"
            << tr("0 &le; CC' &le; 1  (CC'=1 when a node is the center of a star graph)")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("CC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("CC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%CC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";


    VList::const_iterator it;

    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->CC()
                    << "</td><td>"
                    << (*it)->SCC()
                    << "</td><td>"
                    << (100* ((*it)->SCC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minSCC ==  maxSCC) {
        outText << "<p>"
                << tr("All nodes have the same CC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("CC Sum = ")
                <<"</span>"
                << sumCC
                <<"</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max CC' = ")
                <<"</span>"
               << maxSCC <<" (node "<< maxNodeSCC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min CC' = ")
               <<"</span>"
               << minSCC <<" (node "<< minNodeSCC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("CC' classes = ")
               <<"</span>"
               << classesSCC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("CC' Sum = ")
            <<"</span>"
            << sumSCC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("CC' Mean = ")
            <<"</span>"
            << meanSCC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("CC' Variance = ")
            <<"</span>"
            << varianceSCC
            <<"<br/>";
    outText << "</p>";



    if ( m_reportsChartType != ChartType::None ) {
        outText << "<h2>";
        outText << tr("CC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights) {
        outText << "<h2>";
        outText << tr("GROUP CLOSENESS CENTRALIZATION (GCC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GCC = ")
                <<"</span>"
                <<  groupCC
                 << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GCC range: ")
                <<"</span>"
                <<" 0 &le; GCC &le; 1"
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}





/**
 * @brief Computes an "improved" closeness centrality index, IRCC, which can be used
 * on disconnected graphs.
 * IRCC is an improved node-level centrality closeness index which focuses on the
 * influence range of each node (the set of nodes that are reachable from it)
 * For each node v, this index calculates the fraction of nodes in its influence
 * range and divides it by the average distance of those nodes from v,
 * ignoring nodes that are not reachable from it.
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::centralityClosenessIR(const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool dropIsolates){

    if ( calculatedIRCC ) {
        qDebug() << "Graph not changed - no need to recompute IRCC. Returning";
        return;
    }

    qDebug()<< "(Re)Computing IRCC closeness centrality...";

    graphDistancesGeodesic(false,considerWeights,inverseWeights,dropIsolates);

    // calculate centralities
    VList::const_iterator it, jt;
    int progressCounter = 0;
    qreal IRCC=0,SIRCC=0;
    qreal Ji=0;
    qreal dist=0;
    qreal sumD=0;
    qreal averageD=0;
    qreal N=vertices(dropIsolates,false, true);
    classesIRCC=0;
    discreteIRCCs.clear();
    sumIRCC=0;
    maxIRCC=0;
    minIRCC=N-1;
    varianceIRCC=0;
    meanIRCC=0;

    QString pMsg = tr("Computing Influence Range Centrality scores. \n"
                      "Please wait");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    qDebug()<< "dropIsolates"<< dropIsolates;
    qDebug()<< "computing scores for actors: " << N;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        IRCC=0;
        sumD=0;
        Ji = 0;
        if ((*it)->isIsolated()) {
            continue;
        }
        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt){

            if ( (*it)->number() == (*jt)->number() ) {
                continue;
            }
            if ( ! (*jt)->isEnabled() ) {
                continue;
            }

            dist = (*it)->distance( (*jt)->number() );

            if (dist != RAND_MAX ) {
                sumD += dist;
                Ji ++; // compute |Ji|
            }
            qDebug()<< "dist(" << (*it)->number()
                    << ","<< (*jt)->number() << ") =" << dist << "sumD" << sumD << " Ji"<<Ji;

        }

        qDebug()<< "" << (*it)->number()
                << " sumD"<< sumD
                << "distanceSum" << (*it)->distanceSum();

        // sanity check for sumD=0 (=> node is disconnected)
        if (sumD != 0)  {
            averageD = sumD / Ji;
            qDebug()<< "averageD = sumD /  Ji"<<averageD ;
            qDebug()<< "Ji / (N-1)"<< Ji << "/" << N-1;
            IRCC =  ( Ji / (qreal) (N-1) ) / averageD;
            qDebug()<< "[ Ji / (N-1) ] / [ sumD / Ji]" << IRCC ;
        }

        sumIRCC += IRCC;
        (*it)->setIRCC ( IRCC ) ;
        (*it)->setSIRCC ( IRCC ) ;  // IRCC is a ratio, already std
        resolveClasses(IRCC, discreteIRCCs, classesIRCC);
        minmax( IRCC, (*it), maxIRCC, minIRCC, maxNodeIRCC, minNodeIRCC) ;

    }

    meanIRCC = sumIRCC / (qreal) N;

    if (minIRCC == maxIRCC)
        maxNodeIRCC=-1;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! dropIsolates || ! (*it)->isIsolated()  ) {
            SIRCC= (*it)->SIRCC();
            varianceIRCC += (SIRCC-meanIRCC) * (SIRCC-meanIRCC) ;
        }
    }

    varianceIRCC=varianceIRCC/(qreal) N;

    calculatedIRCC=true;

    emit signalProgressBoxKill();

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
                                                   const bool dropIsolates) {

    qDebug() << "Writing IR Closeness Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    centralityClosenessIR(considerWeights,inverseWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::IRCC, m_reportsChartType,distImageFileName);

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Influence Range Centrality scores. \n"
                      "Please wait");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("INFLUENCE RANGE CLOSENESS CENTRALITY (IRCC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; IRCC &le; 1  (IRCC is a ratio)")
            << "</p>";



    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("IRCC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%IRCC'")
            << "</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    VList::const_iterator it;

    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->IRCC()
                    << "</td><td>"
                    << (100* ((*it)->SIRCC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minIRCC ==  maxIRCC) {
        outText << "<p>"
                << tr("All nodes have the same IRCC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max IRCC = ")
                <<"</span>"
               << maxIRCC <<" (node "<< maxNodeIRCC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min IRCC = ")
               <<"</span>"
               << minIRCC <<" (node "<< minNodeIRCC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("IRCC classes = ")
               <<"</span>"
               << classesIRCC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("IRCC Sum = ")
            <<"</span>"
            << sumIRCC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("IRCC Mean = ")
            <<"</span>"
            << meanIRCC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("IRCC Variance = ")
            <<"</span>"
            << varianceIRCC
            <<"<br/>";
    outText << "</p>";

    if ( m_reportsChartType != ChartType::None ) {
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();


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
                                       const bool dropIsolates) {

    qDebug() << "Writing Betweenness Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::BC, m_reportsChartType, distImageFileName);

    int rowCount=0, progressCounter=0;
    int N = vertices();

    QString pMsg =  tr("Writing Betweenness Centrality scores to file. \nPlease wait...");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("BETWEENNESS CENTRALITY (BC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; BC &le; ")<< maxIndexBC
            << tr(" (Number of pairs of nodes excluding u)")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("BC' range: ")
            <<"</span>"
            << tr("0 &le; BC' &le; 1  (BC'=1 when the node falls on all geodesics)")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("BC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("BC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%BC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";


    VList::const_iterator it;

    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){
        emit signalProgressBoxUpdate(++progressCounter);
        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->BC()
                    << "</td><td>"
                    << (*it)->SBC()
                    << "</td><td>"
                    << (100* ((*it)->SBC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minSBC ==  maxSBC) {
        outText << "<p>"
                << tr("All nodes have the same BC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("BC Sum = ")
                <<"</span>"
                << sumBC
                <<"</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max BC' = ")
                <<"</span>"
               << maxSBC <<" (node "<< maxNodeSBC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min BC' = ")
               <<"</span>"
               << minSBC <<" (node "<< minNodeSBC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("BC' classes = ")
               <<"</span>"
               << classesSBC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("BC' Sum = ")
            <<"</span>"
            << sumSBC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("BC' Mean = ")
            <<"</span>"
            << meanSBC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("BC' Variance = ")
            <<"</span>"
            << varianceSBC
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
        outText << "<h2>";
        outText << tr("BC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights) {
        outText << "<h2>";
        outText << tr("GROUP BETWEENNESS CENTRALIZATION (GBC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GBC = ")
                <<"</span>"
                <<  groupSBC
                 << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GBC range: ")
                <<"</span>"
                <<" 0 &le; GBC &le; 1"
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}




/**
 * @brief Writes the Stress centralities to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writeCentralityStress( const QString fileName,
                                   const bool considerWeights,
                                   const bool inverseWeights,
                                   const bool dropIsolates) {

    qDebug() << "Writing Stress Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    graphDistancesGeodesic(true, considerWeights, inverseWeights,dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::SC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg =  tr("Writing Stress Centralities. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("STRESS CENTRALITY (SC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; SC &le; ")<< maxIndexSC
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("SC' range: ")
            <<"</span>"
            << tr("0 &le; SC' &le; 1  (SC'=1 when the node falls on all geodesics)")
            << "</p>";

    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("SC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("SC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%SC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << (*it)->SC()
                   << "</td><td>"
                   << (*it)->SSC()
                   << "</td><td>"
                   << (100* ((*it)->SSC()))
                   << "</td>"
                   <<"</tr>";
        }
    }

    outText << "</tbody></table>";

    if ( minSSC ==  maxSSC) {
        outText << "<p>"
                << tr("All nodes have the same SC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("SC Sum = ")
                <<"</span>"
                << sumSC
                <<"</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max SC' = ")
                <<"</span>"
               << maxSSC <<" (node "<< maxNodeSSC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min SC' = ")
               <<"</span>"
               << minSSC <<" (node "<< minNodeSSC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("BC classes = ")
               <<"</span>"
               << classesSSC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("SC' Sum = ")
            <<"</span>"
            << sumSSC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("SC' Mean = ")
            <<"</span>"
            << meanSSC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("SC' Variance = ")
            <<"</span>"
            << varianceSSC
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

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
                                        const bool dropIsolates) {

    qDebug() << "Writing Eccentricity Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    graphDistancesGeodesic(true, considerWeights, inverseWeights,dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::EC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Eccentricity Centralities to file. \nPlease wait...") ;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("ECCENTRICITY CENTRALITY (EC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; EC &le; 1 ")
            << tr(" (EC=1 when the actor has ties to all other nodes)")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("EC=EC'")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%EC'")
            << "</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->EC()
                    << "</td><td>"
                    << (100* ((*it)->SEC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minEC ==  maxEC) {
        outText << "<p>"
                << tr("All nodes have the same EC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max EC = ")
                <<"</span>"
               << maxEC <<" (node "<< maxNodeEC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min EC = ")
               <<"</span>"
               << minEC <<" (node "<< minNodeEC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("EC classes = ")
               <<"</span>"
               << classesEC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("EC Sum = ")
            <<"</span>"
            << sumEC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("EC Mean = ")
            <<"</span>"
            << meanEC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("EC Variance = ")
            <<"</span>"
            << varianceEC
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;


    file.close();

    emit signalProgressBoxKill();

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
                                 const bool dropIsolates) {

    qDebug() << "Writing Power Centrality report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    graphDistancesGeodesic(true, considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::PC, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Gil-Schmidt Power Centralities to file. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("POWER CENTRALITY (PC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; PC &le; ")<< maxIndexPC
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PC' range: ")
            <<"</span>"
            << tr("0 &le; PC' &le; 1  (PC'=1 when the node is connected to all (star).)")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("PC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%PC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";


    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->PC()
                    << "</td><td>"
                    << (*it)->SPC()
                    << "</td><td>"
                    << (100* ((*it)->SPC()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minSPC ==  maxSPC) {
        outText << "<p>"
                << tr("All nodes have the same PC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("PC Sum = ")
                <<"</span>"
                << sumPC
                <<"</p>";

        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PC' = ")
                <<"</span>"
               << maxSPC <<" (node "<< maxNodeSPC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min PC' = ")
               <<"</span>"
               << minSPC <<" (node "<< minNodeSPC <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("PC classes = ")
               <<"</span>"
               << classesSPC
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PC' Sum = ")
            <<"</span>"
            << sumSPC
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("PC' Mean = ")
            <<"</span>"
            << meanSPC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("PC' Variance = ")
            <<"</span>"
            << varianceSPC
            <<"<br/>";
    outText << "</p>";



    if ( m_reportsChartType != ChartType::None ) {
        outText << "<h2>";
        outText << tr("PC' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }


    if (!considerWeights) {
        outText << "<h2>";
        outText << tr("GROUP POWER CENTRALIZATION (GPC)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GPC = ")
                <<"</span>"
                <<  groupSPC
                 << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GPC range: ")
                <<"</span>"
                <<" 0 &le; GPC &le; 1"
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;


    file.close();

    emit signalProgressBoxKill();

}







/**
 * @brief Computes the Degree Prestige (in-degree) of each vertex - diagonal included
*	Also the mean value and the variance of the in-degrees.
 * @param weights
 * @param dropIsolates
 */
void Graph::prestigeDegree(const bool &considerWeights, const bool &dropIsolates){

    if ( calculatedDP ) {
        qDebug() << "Graph not changed - no need to recompute Degree Prestige scores. Returning";
        return;
    }

    qDebug()<< "(Re)Computing Degree Prestige scores...";

    int N=vertices(dropIsolates);
    int v2=0, v1=0;
    int progressCounter = 0;

    VList::const_iterator it;

    QHash<int,qreal> *enabledInEdges = new QHash<int,qreal>;
    QHash<int,qreal>::const_iterator hit;

    qreal DP=0, SDP=0, nom=0, denom=0;
    qreal weight;

    classesSDP=0;
    sumSDP=0;
    sumDP=0;
    maxSDP=0;
    minSDP=N-1;
    discreteDPs.clear();
    varianceSDP=0;
    meanSDP=0;
    m_graphIsSymmetric = true;


    QString pMsg = tr("Computing Degree Prestige (in-Degree). \n Please wait ...");
    emit statusMessage(  pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    qDebug()<< "vertices"
            << N
            <<"graph modified. Recomputing...";

    for ( it = m_graph.cbegin(); it != m_graph.cend(); ++it)  {

        emit signalProgressBoxUpdate(++progressCounter);

        v1 = (*it)->number();
        qDebug()<< "computing DP for vertex" << v1 ;

        DP=0;

        if ( ! (*it)->isEnabled() ) {
            qDebug()<< "vertex disabled. Continue.";
            continue;
        }

        qDebug() << "Iterate over inbound edges of "
                 << v1 ;


        enabledInEdges=(*it)->inEdgesEnabledHash();

        hit=enabledInEdges->cbegin();

        while ( hit!=enabledInEdges->cend() ){

            v2 = hit.key();

            qDebug() << "inbound edge from" << v2;

            if (  ! edgeExists ( v2, v1)  ) {
                //sanity check
                qDebug() << "Cannot verify inbound edge"
                         << v2 << "CONTINUE" ;
                ++hit;
                continue;
            }

            weight = hit.value();

            if (considerWeights) {
                DP+=weight;
            }
            else {
                DP++;
            }
            if (  edgeExists ( v1, v2)  != weight) {
                m_graphIsSymmetric=false;
            }
            ++hit;
        }

        (*it)->setDP ( DP ) ;		//Set DP
        sumDP += DP;

        qDebug() << "vertex " <<  (*it)->number()
                 << " DP "  << DP;

    }


    // Calculate std DP, min,max, mean
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        DP= (*it)->DP();

        if (!considerWeights) {
            SDP=( DP / (N-1.0) );		//Set Standard InDegree
        }
        else {
            SDP =( DP / (sumDP) );
        }
        (*it)->setSDP( SDP );
        sumSDP += SDP;

        qDebug() << "vertex " <<  (*it)->number() << " DP  "
                 << DP << " SDP " << (*it)->SDP ();

        resolveClasses(SDP, discreteDPs, classesSDP);

        qDebug("DP classes = %i ", classesSDP);

        if (maxSDP < SDP ) {
            maxSDP = SDP ;
            maxNodeDP=(*it)->number();
        }
        if (minSDP > SDP ) {
            minSDP = SDP ;
            minNodeDP=(*it)->number();
        }

    }

    if (minSDP == maxSDP)
        maxNodeDP=-1;

    meanSDP = sumSDP / (qreal) N;

    qDebug("Graph: sumSDP = %f, meanSDP = %f", sumSDP, meanSDP);

    // Calculate Variance and the Degree Prestigation of the whole graph. :)
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        SDP= (*it)->SDP();
        nom+= maxSDP-SDP;
        varianceSDP += (SDP-meanSDP) * (SDP-meanSDP) ;
    }
    varianceSDP=varianceSDP/(qreal) N;

    if (m_graphIsSymmetric)
        denom=(N-1.0)*(N-2.0);
    else
        denom=(N-1.0)*(N-1.0);
    if (N < 3 )
        denom = N-1.0;


    if (!considerWeights) {
        groupDP=nom/denom;
        qDebug("Graph: varianceSDP = %f, groupDP = %f", varianceSDP, groupDP);
    }

    delete enabledInEdges;
    calculatedDP=true;

    emit signalProgressBoxKill();

}



/**
 * @brief Writes the Degree Prestige of each node to a file
 * @param fileName
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::writePrestigeDegree (const QString fileName,
                                 const bool considerWeights,
                                 const bool dropIsolates) {

    qDebug() << "Writing Degree Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    prestigeDegree(considerWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::DP, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int N = vertices();

    qreal maxIndexDP=N-1.0;

    int rowCount=0;
    int progressCounter = 0;

    QString pMsg = tr("Writing Degree Prestige (in-Degree) scores to file. \nPlease wait ...");
    emit statusMessage(  pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("DEGREE PRESTIGE (DP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
           << tr("0 &le; DP &le; ");
    if (considerWeights) outText<< infinity;
    else outText << maxIndexDP;
    outText << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("DP' range: ")
            <<"</span>"
            << tr("0 &le; DP' &le; 1")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("DP")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("DP'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%DP'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";




    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate( ++progressCounter );

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->DP()
                    << "</td><td>"
                    << (*it)->SDP()
                    << "</td><td>"
                    << (100* ((*it)->SDP()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minSDP ==  maxSDP) {
        outText << "<p>"
                << tr("All nodes have the same DP score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText  << "<span class=\"info\">"
                 << tr("DP Sum = ")
                 <<"</span>"
                << sumDP
                << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("Max DP' = ")
                <<"</span>"
               << maxSDP <<" (node "<< maxNodeDP  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min DP' = ")
               <<"</span>"
              << minSDP <<" (node "<< minNodeDP <<  ")"
              << "<br />"
              << "<span class=\"info\">"
              << tr("DP' classes = ")
              <<"</span>"
             << classesSDP
             << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("DP' Sum = ")
            <<"</span>"
            << sumSDP
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("DP' Mean = ")
            <<"</span>"
            << meanSDP
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("DP' Variance = ")
            <<"</span>"
            << varianceSDP
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
        outText << "<h2>";
        outText << tr("DP' DISTRIBUTION")
                << "</h2>";
        outText << "<p>";
        outText << "<img style=\"width:100%;\" src=\""
                << distImageFileName
                << "\" />";
    }

    if (!considerWeights) {
        outText << "<h2>";
        outText << tr("GROUP DEGREE PRESTIGE (GDP)")
                << "</h2>";
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("GDP = ")
                <<"</span>"
                <<  groupDP
                 << "</p>";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("GDP range: ")
                <<"</span>"
                <<" 0 &le; GDP &le; 1"
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}




/**
 * @brief Computes Proximity Prestige of each vertex
 * Also the mean value and the variance of it..
 */
void Graph::prestigeProximity( const bool considerWeights,
                               const bool inverseWeights,
                               const bool dropIsolates){
    if ( calculatedPP ) {
        qDebug() << "Graph not changed - no need to recompute proximity prestige. Returning";
        return;
    }

    qDebug()<< "(Re)Computing Proximity prestige scores...";

    graphDistancesGeodesic(false,considerWeights, inverseWeights,inverseWeights);

    // calculate centralities
    VList::const_iterator it, jt;
    qreal PP=0;
    qreal dist=0;
    qreal Ii=0;
    qreal V=vertices(dropIsolates);
    classesPP=0;
    discretePPs.clear();
    sumPP=0;
    maxPP=0;
    minPP=V-1;
    variancePP=0;
    meanPP=0;

    int progressCounter = 0;

    QString pMsg = tr("Computing Proximity Prestige scores. \nPlease wait ...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(V,pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        PP=0;
        Ii = 0;

        if ((*it)->isIsolated()){
            continue;
        }

        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt){

            if ( (*it)->number() == (*jt)->number() ) {
                continue;
            }
            if ( ! (*jt)->isEnabled() ) {
                continue;
            }

            dist = (*jt)->distance( (*it)->number() );

            if (dist != RAND_MAX ) {
                PP += dist;
                Ii ++; // compute |Ii|
            }
        }

        qDebug()<< "vertex"
                << (*it)->number()
                << "actors in influence domain Ii" << Ii
                << "actors in network"<< (V-1)
                << "fraction of actors who reach i |Ii|/(V-1)="  << Ii/ (V-1)
                << "distance to actors in Ii" << PP
                << "average distance to actors in Ii" << PP/ Ii
                << "PP= "
                << Ii / (V-1) << " / " << PP / Ii << " = " << ( Ii / (V-1) ) / ( PP / Ii  );


        // sanity check for PP=0 (=> node is disconnected)
        if (PP != 0)  {
            PP /= Ii;
            PP =  ( Ii / (V-1) ) / PP;
        }
        sumPP += PP;

        (*it)->setPP ( PP ) ;
        (*it)->setSPP ( PP ) ; // PP is already stdized

        resolveClasses(PP, discretePPs, classesPP);

        //qDebug("PP classes = %i ", classesPP);
        if (maxPP < PP ) {
            maxPP = PP ;
            maxNodePP=(*it)->number();
        }
        if (minPP > PP ) {
            minPP = PP ;
            minNodePP=(*it)->number();
        }

    }

    if (minPP == maxPP)
        maxNodePP=-1;

    meanPP = sumPP / V;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        PP= (*it)->PP();
        variancePP += (PP-meanPP) * (PP-meanPP) ;
    }

    variancePP=variancePP/ V;

    qDebug() << "sumPP = " << sumPP
             << " meanPP = " << meanPP
             << " variancePP " << variancePP;

    calculatedPP=true;

    emit signalProgressBoxKill();

}




/**
 * @brief
 * Writes the proximity prestige indices to a file
 * @param fileName
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::writePrestigeProximity( const QString fileName,
                                    const bool considerWeights,
                                    const bool inverseWeights,
                                    const bool dropIsolates) {

    qDebug() << "Writing Proximity Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    prestigeProximity(considerWeights, inverseWeights, dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::PP, m_reportsChartType, distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing Proximity Prestige scores to file. \nPlease wait ...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    outText << htmlHead;

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("PROXIMITY PRESTIGE (PP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; PP &le; 1 (PP is a ratio)")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PP=PP'")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("%PP")
            << "</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->PP()
                    << "</td><td>"
                    << (100* ((*it)->SPP()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minPP ==  maxPP) {
        outText << "<p>"
                << tr("All nodes have the same PP score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PP = ")
                <<"</span>"
               << maxPP <<" (node "<< maxNodePP  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min PP = ")
               <<"</span>"
               << minPP <<" (node "<< minNodePP <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("PP classes = ")
               <<"</span>"
               << classesPP
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PP Sum = ")
            <<"</span>"
            << sumPP
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("PP Mean = ")
            <<"</span>"
            << meanPP
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("PP Variance = ")
            <<"</span>"
            << variancePP
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}





/**
 * @brief Calculates the PageRank Prestige of each vertex
 * @param dropIsolates
 */
void Graph::prestigePageRank(const bool &dropIsolates){

    if ( calculatedPRP ) {
        qDebug() << "Graph not changed - no need to recompute Pagerank scores. Return ";
        return;
    }

    qDebug()<< "(Re)Computing PageRank prestige scores...";

    discretePRPs.clear();
    sumPRP=0;
    t_sumPRP=0;
    maxPRP=0;
    minPRP=RAND_MAX;
    classesPRP=0;
    variancePRP=0;
    // The parameter d is a damping factor which can be set between 0 and 1.
    // Google creators set d to 0.85.
    d_factor = 0.85;

    qreal PRP=0, oldPRP = 0;
    qreal SPRP=0;
    int iterations = 1; // a counter
    int referrer;
    qreal delta = 0.00001; // The delta where we will stop the iterative calculation
    qreal maxDelta = RAND_MAX;
    qreal sumInLinksPR = 0;  // temporary var for inlinks sum PR
    qreal transferedPRP = 0;
    qreal inLinks = 0;       // temporary var
    qreal outLinks = 0;       // temporary var
    qreal t_variance=0;
    int N =  vertices(dropIsolates) ;

    VList::const_iterator it;
    H_edges::const_iterator jt;

    int relation=0;
    bool edgeStatus=false;


    QString pMsg = tr("Computing PageRank Prestige scores. \nPlease wait ...");
    emit statusMessage( pMsg ) ;
    emit signalProgressBoxCreate(N,pMsg);


    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        // At first, PR scores have probability distribution
        // from 0 to 1, so each one is set to 1/N
        (*it)->setPRP( 1.0 / (qreal) N );

        // compute inEdgesCount() to warm up inEdgesConst for everyone
        inLinks = (*it)->inEdgesCount();
        outLinks = (*it)->outEdgesCount();
        qDebug() << "node "
                 << (*it)->number() << " PR = " << (*it)->PRP()
                 << " inLinks (set const): " << inLinks
                 << " outLinks (set const): " << outLinks;
    }

    if ( edgesEnabled() == 0 ) {
        qDebug()<< "all vertices are isolated and of equal PR. Stop";
        return;
    }

    emit signalProgressBoxUpdate( N / 3);

    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta) {

        qDebug()<< "ITERATION : " << iterations;

        sumPRP=0;
        maxDelta = 0;
        maxPRP=0;
        minPRP=RAND_MAX;
        maxNodePRP = 0;
        minNodePRP = 0;

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
        {
            sumInLinksPR = 0;
            oldPRP = (*it)->PRP();

            qDebug() << "computing PR for node: "
                     << (*it)->number()  << " current PR " << oldPRP;

            if ( (*it)->isIsolated() ) {
                // isolates have constant PR = 1/N
                qDebug() << "isolated - CONTINUE ";
                continue;
            }

            jt=(*it)->m_inEdges.cbegin();

            qDebug() << "Iterate over inEdges of "
                     << (*it)->number() ;

            while ( jt != (*it)->m_inEdges.cend() )
            {
                relation = jt.value().first;
                if ( relation != relationCurrent() ){
                    ++jt;
                    continue;
                }
                edgeStatus=jt.value().second.second;
                if ( edgeStatus != true){
                    ++jt;
                    continue;
                }

                referrer = jt.key();

                qDebug() << "Node " << (*it)->number()
                         << " inLinked from neighbor " << referrer  << " vpos "
                         << vpos[referrer];

                if ( edgeExists( referrer , (*it)->number() ) )
                {
                    inLinks = m_graph[ vpos[referrer] ]->inEdgesCountConst();
                    outLinks = m_graph[ vpos[referrer] ]->outEdgesCountConst();

                    PRP =  m_graph[ vpos[referrer] ]->PRP();

                    transferedPRP = (outLinks != 0 ) ? ( PRP / outLinks ) : PRP;

                    qDebug()<< "neighbor " << referrer
                            << " has PR = " << PRP
                            << " and outLinks = " << outLinks
                            << "  will transfer " << transferedPRP ;

                    sumInLinksPR +=  transferedPRP;

                }
                ++jt;
            }

            PRP = (1-d_factor) / (qreal) N + d_factor * sumInLinksPR;

            (*it)->setPRP ( PRP );

            sumPRP+=PRP;

            qDebug() << "Node "
                     << (*it)->number()
                     << " new PR = " << PRP
                     << " old PR was = " << oldPRP
                     << " diff = " << fabs(PRP - oldPRP);

            // calculate diff from last PageRank value for this vertex
            // and set it to minDelta if the latter is bigger.

            if ( maxDelta < fabs(PRP - oldPRP) ) {
                maxDelta = fabs(PRP - oldPRP);
                qDebug()<< "Setting new maxDelta = "
                        <<  maxDelta;
            }

        }

        // normalize in every iteration

        qDebug() << "sumPRP for this iteration " <<
                    sumPRP;

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
            PRP = (*it)->PRP();

            if ( PRP > maxPRP ) {
                maxPRP = PRP;
                maxNodePRP=(*it)->number();
            }
            if ( PRP < minPRP ) {
                minPRP = PRP;
                minNodePRP=(*it)->number();
            }

        }
        iterations++;
    }


    emit signalProgressBoxUpdate( 2* N / 3);

    if (N != 0 ) {
        meanPRP = sumPRP / (qreal) N ;
    }
    else {
        meanPRP = SPRP;
    }

    qDebug() << "sumPRP = " << sumPRP << "  N = " << N
             << "  meanPRP = " << meanPRP;


    // calculate std and min/max PRPs
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
        if (dropIsolates && (*it)->isIsolated()) {
            continue;
        }

        PRP=(*it)->PRP();

        resolveClasses(PRP,discretePRPs,classesPRP);

        SPRP = PRP / maxPRP ;
        (*it)->setSPRP( SPRP );

        qDebug()<< "vertex: " <<  (*it)->number()
                << " PR = " << PRP << " standard PR = " << SPRP
                << " t_sumPRP " << t_sumPRP;

        t_variance = ( PRP  - meanPRP  ) ;
        t_variance *=t_variance;
        qDebug() << "PRP " <<  (*it)->PRP() << "  t_variance "
                 << PRP - meanPRP  << " t_variance^2" << t_variance ;
        variancePRP  += t_variance;

    }




    qDebug() << "PRP' Variance   " << variancePRP   << " N " << N ;
    variancePRP  = variancePRP  / (qreal) N;
    qDebug() << "PRP' Variance: " << variancePRP   ;

    calculatedPRP= true;

    emit signalProgressBoxUpdate( N);
    emit signalProgressBoxKill();


    return;

}



/**
 * @brief Writes the PageRank scores of vertices to a file
 * @param fileName
 * @param dropIsolates
 */
void Graph::writePrestigePageRank(const QString fileName,
                                  const bool dropIsolates){

    qDebug() << "Writing PageRank Prestige report to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    prestigePageRank(dropIsolates);

    QString distImageFileName ;

    if ( m_reportsChartType != ChartType::None ) {

        distImageFileName = QFileInfo(fileName).canonicalPath() +
                QDir::separator() + QFileInfo(fileName).completeBaseName() + ".png";

    }

    prominenceDistribution(IndexType::PRP,m_reportsChartType,distImageFileName);

    VList::const_iterator it;

    int rowCount=0;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Writing PageRank scores to file. \nPlease wait ...");
    emit statusMessage( pMsg ) ;
    emit signalProgressBoxCreate(N,pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("PAGERANK PRESTIGE (PRP)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
                  "Therefore, nodes with high outLink weights give smaller percentage of their PR to node u."
                  )
            << "<br />"
            << tr("PRP' is the scaled PRP (PRP divided by max PRP).")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PRP range: ")
            <<"</span>"
            << tr("(1-d)/N = ") << ( ( 1- d_factor ) / N ) << tr(" &le; PRP  ")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PRP' range: ")
            <<"</span>"
            << tr("0 &le; PRP' &le; 1")
            << "</p>";



    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("PRP")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("PRP'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%PRP'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";



    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){

        emit signalProgressBoxUpdate( ++progressCounter );

        rowCount++;

        outText << Qt::fixed;

        if (dropIsolates && (*it)->isIsolated()) {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                   << (*it)->number()
                   << "</td><td>"
                   << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td><td>"
                   << "--"
                   << "</td>"
                   <<"</tr>";
        }
        else {
            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                    <<"<td>"
                    << (*it)->number()
                    << "</td><td>"
                    << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                    << "</td><td>"
                    << (*it)->PRP()
                    << "</td><td>"
                    << (*it)->SPRP()
                    << "</td><td>"
                    << (100* ((*it)->SPRP()))
                    << "</td>"
                    <<"</tr>";

        }

    }

    outText << "</tbody></table>";

    if ( minPRP ==  maxPRP) {
        outText << "<p>"
                << tr("All nodes have the same PRP score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max PRP = ")
                <<"</span>"
               << maxPRP <<" (node "<< maxNodePRP  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min PRP = ")
               <<"</span>"
               << minPRP <<" (node "<< minNodePRP <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("PRP classes = ")
               <<"</span>"
               << classesPRP
               << "</p>";
    }

    outText << "<p>";
    outText << "<span class=\"info\">"
            << tr("PRP Sum = ")
            <<"</span>"
            << sumPRP
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("PRP Mean = ")
            <<"</span>"
            << meanPRP
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("PRP Variance = ")
            <<"</span>"
            << variancePRP
            <<"<br/>";
    outText << "</p>";


    if ( m_reportsChartType != ChartType::None ) {
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

}







/**
 * @brief Adds a little universal randomness :)
 */
void Graph::randomizeThings()   {
    time_t now;				/* define 'now'. time_t is probably a typedef	*/
    now = time((time_t *)NULL);		/* Get the system time and put it
                     * into 'now' as 'calender time' the number of seconds since  1/1/1970   	*/

    srand( (unsigned int ) now);
}




/**
 * @brief Creates an erdos-renyi random network according to the given model
 * @param vert
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 */
void Graph::randomNetErdosCreate(const int &N,
                                 const QString &model,
                                 const int &m,
                                 const qreal &p,
                                 const QString &mode,
                                 const bool &diag)
{
    qDebug() << "Creating an erdos-renyi random network, vertices " << N
             << " model " << model
             << " edges " << m
             << " edge probability " << p
             << " graph mode " << mode
             << " diag " << diag;

    if (mode=="graph") {
        setDirected(false);
    }
    vpos.reserve(N);

    randomizeThings();

    int progressCounter=0;
    int edgeCount = 0;

    qDebug() << "Creating nodes...";

    QString pMsg  = tr( "Creating Erdos-Renyi Random Network. \n"
                               " Please wait..." );

    emit signalProgressBoxCreate( (m != 0 ? m:N), pMsg );

    for (int i=0; i< N ; i++)
    {
        int x=canvasRandomX();
        int y=canvasRandomY();

//        qDebug() << "creating new node" << i+1 << "at" << x << y;

        vertexCreate(
                    i+1, initVertexSize, initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape, initVertexIconPath, false
                    );
    }

    qDebug() << "Creating edges...";

    if ( model == "G(n,p)")
    {
        qDebug() << "G(n,p) model...";
        for (int i=0;i<N; i++) {
            for (int j=0; j<N; j++) {

                //                qDebug() << "Bernoulli trial "
                //                       << "for edge " <<  i+1 << "->" << j+1;

                if (!diag && i==j) {
//                    qDebug()<< " skip because " << i+1 << " = " << j+1 << " and diag " << diag;
                    continue;
                }
                if ( ( rand() % 100 + 1 ) / 100.0 < p )    {
                    edgeCount ++ ;

                    if (mode == "graph") {
//                        qDebug() << " create undirected Edge no " << edgeCount;
                        edgeCreate(i+1, j+1, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                    }
                    else {
//                        qDebug() << " create directed Edge no "<< edgeCount;
                        edgeCreate(i+1, j+1, 1, initEdgeColor,
                                   EdgeType::Directed, true, false,
                                   QString(), false);
                    }
                }
                //                else
                //                    qDebug() << "do not create Edge";
            }

            emit signalProgressBoxUpdate(++progressCounter );

        }

    }
    else
    {
        qDebug() << "G(n,M) model...";
        int source = 0, target = 0 ;
        do {
            source =  rand() % N + 1;
            target =  rand() % N + 1;
//            qDebug() << "random pair " << " " << source << " , " << target ;
            if (!diag && source == target ) {
//                qDebug() << "skip self loop pair ";
                continue;
            }
            if ( edgeExists(source, target) ) {
//                qDebug() << "skip pair - exists";
                continue;
            }
            edgeCount ++;
            if (mode == "graph") {
//                qDebug() << "create " << " undirected Edge no " << edgeCount;
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Undirected, false, false,
                           QString(), false);
            }
            else {
//                qDebug() << "create " << " directed Edge no " << edgeCount;
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Directed, true, false,
                           QString(), false);
            }
            emit signalProgressBoxUpdate(++progressCounter );
        } while ( edgeCount != m );

    }

    relationCurrentRename(tr("erdos-renyi"), true);

    emit signalProgressBoxUpdate((m != 0 ? m:N));
    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexEdgeCount);
}







/**
 * @brief Creates a scale-free random-network network
 * @param N
 * @param power
 * @param m0
 * @param m
 * @param alpha
 * @param mode
 */
void Graph::randomNetScaleFreeCreate (const int &N,
                                      const int &power,
                                      const int &m0,
                                      const int &m,
                                      const qreal &alpha,
                                      const QString &mode)
{
    qDebug() << "Graph::randomNetScaleFreeCreate() - max nodes n" << N
             << "power" << power
             <<"edges added in every round m" <<m
            <<"alpha" <<alpha
           <<"mode"<<mode;

    randomizeThings();

    if (mode=="graph") {
        setDirected(false);
    }

    int x=0;
    int y=0;
    int newEdges = 0;
    double sumDegrees=0;
    double k_j;
    double x0 = canvasWidth/2.0;
    double y0 =canvasHeight/2.0;
    double radius = canvasMaxRadius();
    double rad= (2.0* M_PI/ N );
    double  prob_j = 0, prob=0;
    int progressCounter=0;

    vpos.reserve( N );

    qDebug() << "Graph::randomNetScaleFreeCreate() - "
             << "Create initial connected net of m0 nodes";

    QString pMsg = tr ("Creating Scale-Free Random Network. \n"
                  "Please wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg );


    for (int i=0; i< m0 ; ++i) {
        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);

        qDebug() << "Graph::randomNetScaleFreeCreate() - "
                    << " initial node i " << i+1 << " pos " << x << "," << y;
        vertexCreate(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape, initVertexIconPath, false
                    );
    }

    for (int i=0; i < m0; ++i){
        qDebug() << "Graph::randomNetScaleFreeCreate() - "
                   << " Creating all edges for initial node i " << i+1;
        for (int j=i+1; j< m0  ; ++j) {
            qDebug() << "Graph::randomNetScaleFreeCreate() ---- "
                        "Creating initial edge " << i+1 << " <->" << j+1;
            edgeCreate (i+1, j+1, 1, initEdgeColor,
                        EdgeType::Undirected, false, false,
                        QString(), false);
        }
        emit signalProgressBoxUpdate( ++progressCounter );
    }

    qDebug()<< "Graph::randomNetScaleFreeCreate() - @@@@ "
               << " start network growth to " << N
               << " nodes with preferential attachment" ;

    for (int i= m0 ; i < N ; ++i) {

        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);

        qDebug() << "Graph::randomNetScaleFreeCreate() - ++++"
                    << " adding new node i " << i+1
                    << " pos " << x << "," << y ;

        vertexCreate(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape,initVertexIconPath, false
                    );

        emit signalProgressBoxUpdate( ++progressCounter );

        // need to multiply by 2, since we have a undirected graph
        // and edgesEnabled reports edges/2
        sumDegrees =  2 * edgesEnabled();

        newEdges = 0;

        qDebug()<< "Graph::randomNetScaleFreeCreate() - repeat until we reach"
                << m << "new edges for node" <<i+1;
        for (;;)
        {	//do until we create m new edges

            for (int j=0; j < i  ; ++j) {
                qDebug() << "Graph::randomNetScaleFreeCreate() - "
                           << " preferential attachment test of new node i "
                           << i+1
                           << " with node j " << j+1
                           << " - newEdges " << newEdges ;

                if (newEdges == m)
                    break;

                k_j = vertexDegreeIn(j+1);
                k_j = pow ( k_j , power );
                if (sumDegrees < 1 )
                    prob_j = 1; // always create edge if no other edge exist
                else
                    prob_j = ( alpha + k_j  ) / sumDegrees ;

                prob  = ( rand() % 100 + 1 ) / 100.0;

                qDebug() << "Graph::randomNetScaleFreeCreate() - "
                            << " Edge probability with old node "
                            << j+1 << " is: alpha + k_j ^ power " << alpha + k_j
                            << " / sumDegrees " << sumDegrees
                            << " = prob_j " << prob_j
                            << " prob " << prob ;

                if ( prob  <=  prob_j )  {
                    if ( mode == "graph") {
                        qDebug() << "Graph::randomNetScaleFreeCreate()  <----->"
                                    "Creating pref.att. undirected edge "
                                 <<  i+1 << " <->" << j+1;
                        edgeCreate (i+1, j+1, 1, initEdgeColor,
                                    EdgeType::Undirected, false, false,
                                    QString(), false);
                        newEdges ++;

                    }
                    else {
                        qDebug() << "Graph::randomNetScaleFreeCreate()  ----->"
                                    "Creating pref.att. directed edge "
                                 <<  i+1 << " <->" << j+1;
                        edgeCreate (i+1, j+1, 1, initEdgeColor,
                                    EdgeType::Directed, true, false,
                                    QString(), false);
                        newEdges ++;

                    }
                }
            }
            if ( newEdges == m )
                break;
        }
        qDebug()<< "Graph::randomNetScaleFreeCreate() - " << m << "edges reached "
                "for node" << i+1;
    }

    relationCurrentRename(tr("scale-free"),true);

    qDebug() << "Graph::randomNetScaleFreeCreate() - finished.";

    setModStatus(ModStatus::VertexEdgeCount);

    emit signalProgressBoxKill();

    layoutVertexSizeByIndegree();


}



/**
 * @brief Creates a small world random network
 * @param vert
 * @param degree
 * @param beta
 */
void Graph::randomNetSmallWorldCreate (const int &N, const int &degree,
                                       const double &beta, const QString &mode)
{
    qDebug() << "Creating small-world randome network. Vertices: " << N
             << "degree: " << degree
             << "beta: " << beta
             << "mode: " << mode
             << "First creating a ring lattice";

    if (mode=="graph") {
        setDirected(false);
    }

    randomNetRingLatticeCreate(N, degree, true);

    QString pMsg  = tr("Creating Small-World Random Network. \n"
                       "Please wait ...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N, pMsg);

    qDebug("******** Graph: REWIRING starts...");

    int candidate=0;
    int progressCounter=1;

    for (int i=1;i<N; i++) {
        for (int j=i+1;j<N; j++) {
//            qDebug()<<">>>>> REWIRING: Check if  "<< i << " is linked to " << j;
            if ( edgeExists(i, j) ) {
//                qDebug()<<">>>>> REWIRING: They're linked. Do a random REWIRING "
//                          "Experiment between "<< i<< " and " << j
//                       << " Beta parameter is " << beta;
                if (rand() % 100 < (beta * 100))  {
//                    qDebug(">>>>> REWIRING: We'l break this edge!");
                    edgeRemove(i, j, true);
//                    qDebug()<<">>>>> REWIRING: OK. Let's create a new edge!";
                    for (;;) {	//do until we create a new edge
                        candidate=rand() % (N+1) ;		//pick another vertex.
                        if (candidate == 0 || candidate == i) continue;
//                        qDebug()<<">>>>> REWIRING: Candidate: "<< candidate;
                        //Only if differs from i and hasnot edge with it
//                        qDebug("<---->Random New Edge Experiment between %i and %i:", i, candidate);
                        if (  edgeExists(i, candidate) != 0)
                            continue;
                        if (rand() % 100 > 0.5) {
//                            qDebug() << "Creating new edge";
                            edgeCreate(i, candidate, 1, initEdgeColor,
                                       EdgeType::Undirected, false, false,
                                       QString(), false);
                            break;
                        }
                    }
                }
                else  qDebug("Will not break link!");
            }
        }
        emit signalProgressBoxUpdate( ++progressCounter );
    }

    relationCurrentRename(tr("small-world"), true);


    emit signalProgressBoxKill();

    layoutVertexSizeByIndegree();

    setModStatus(ModStatus::VertexEdgeCount);
}






/**
 * @brief Creates a random network where nodes have the same degree.
 * @param vert
 * @param degree
 */
void Graph::randomNetRegularCreate(const int &N,
                                   const int &degree,
                                   const QString &mode, const bool &diag){
    qDebug() << "Creating d-regular random network...";
    Q_UNUSED(diag);

    if (mode=="graph") {
        setDirected(false);
    }

    int x = 0, y = 0 ;
    qreal progressCounter=0;
    qreal progressFraction =(isUndirected()) ? 2/(qreal) degree : 1/(qreal) degree;

    int target = 0;

    QList<QString> m_edges;
    QStringList firstEdgeVertices, secondEdgeVertices, m_edge;
    QString firstEdge, secondEdge;


    randomizeThings();
    vpos.reserve(N);

    QString pMsg = tr( "Creating pseudo-random d-regular network. \n"
                       "Please wait..." );
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate (N, pMsg );

    qDebug()<< " creating vertices";

    for (int i=0; i< N ; i++) {
        x=canvasRandomX();
        y=canvasRandomY();

//        qDebug() << " creating new vertex at "
//                    << x << "," << y;

        vertexCreate(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape,initVertexIconPath, false
                    );
    }


    qDebug()<< " Creating initial edges";

    if (mode=="graph") {
        for (int i=0;i<N; i++){
            qDebug()<< " "
                       "Creating undirected edges for node  "<< i+1;
            for (int j=0; j< degree/2 ; j++) {
                target = i + j+1 ;
                if ( target > (N-1))
                    target = target-N;
                qDebug()<< " undirected edge "
                        << i+1 << "<->"<< target+1;
                m_edges.append(QString::number(i+1)+"->"+QString::number(target+1));

            }
        }

    }
    else {
        for (int i=0;i<N; i++){
//            qDebug()<< "Creating directed edges for node  "<< i+1;
            for (int j=0; j< degree ; j++) {
                target = i + j+1 ;
                if ( target > (N-1))
                    target = target-N;
//                qDebug()<< " directed edge "<< i+1 << "->"<< target+1;
                m_edges.append(QString::number(i+1)+"->"+QString::number(target+1));

            }
        }

    }
    qDebug()<< "Edge list count:" << m_edges.size()
            << "Now reordering all edges in pairs...";

    //take randomly two edges, of different vertices and combine their source
    //and target vertices to two different edges
    for (int i = 1 ; i< m_edges.size(); ++i) {

        firstEdgeVertices.clear();
        secondEdgeVertices.clear();
        firstEdgeVertices << "";
        firstEdgeVertices << "";
        secondEdgeVertices << "";
        secondEdgeVertices << "";
        while (firstEdgeVertices[0] == firstEdgeVertices[1] ||
               firstEdgeVertices[0] == secondEdgeVertices[0] ||
               firstEdgeVertices[0] == secondEdgeVertices[1] ||
               firstEdgeVertices[1] == secondEdgeVertices[0] ||
               firstEdgeVertices[1] == secondEdgeVertices[1] ||
               secondEdgeVertices[0] == secondEdgeVertices[1] ||
               m_edges.contains( firstEdgeVertices[0] + "->" + secondEdgeVertices[1] ) ||
               m_edges.contains( secondEdgeVertices[0] + "->" + firstEdgeVertices[1] ) ||
               (isUndirected() && m_edges.contains( secondEdgeVertices[1] + "->" + firstEdgeVertices[0]) )||
               (isUndirected() && m_edges.contains( firstEdgeVertices[1] + "->" + secondEdgeVertices[0] ) ) ) {

            firstEdge = m_edges.at(rand() % m_edges.size()) ;
            firstEdgeVertices = firstEdge.split("->");
            secondEdge = m_edges.at(rand() % m_edges.size()) ;
            secondEdgeVertices = secondEdge.split("->");
//            qDebug()<< " firstEdgeVertices:"
//                    << firstEdgeVertices
//                    << " secondEdgeVertices:" << secondEdgeVertices;
        }
//        qDebug()<< " removing edges:" <<firstEdge << secondEdge;
        m_edges.removeAll(firstEdge);
        m_edges.removeAll(secondEdge);

//        qDebug()<< " 2 edges deleted for reordering:"
//                << firstEdgeVertices[0] << "->" << firstEdgeVertices[1]
//                << "and"
//                << secondEdgeVertices[0] << "->" << secondEdgeVertices[1]
//                << "edge list count:" << m_edges.size();

        m_edges.append( firstEdgeVertices[0]+"->"+secondEdgeVertices[1]);
        m_edges.append(secondEdgeVertices[0]+"->"+firstEdgeVertices[1]);

//        qDebug()<< " 2 new edges added:"
//                << firstEdgeVertices[0] << "->" << secondEdgeVertices[1]
//                <<"and"
//                << secondEdgeVertices[0]<<"->"<<firstEdgeVertices[1]
//                << "final edge list count:" << m_edges.size();
    }


    //
    // draw edges
    //
    for (int i = 0; i < m_edges.size(); ++i) {

        m_edge = m_edges.at(i).split("->");

//        qDebug() << "Drawing undirected Edge" <<
//                 << m_edge[0].toInt(0) << "<->" << m_edge[1].toInt(0);

        edgeCreate(m_edge[0].toInt(0), m_edge[1].toInt(0), 1,
                initEdgeColor,
                (isUndirected()) ? EdgeType::Undirected : EdgeType::Directed,
                (isUndirected()) ? false:true,
                false,
                QString(), false);

        progressCounter +=progressFraction;

        if ( fmod ( progressCounter, 1.0) == 0) {
            emit signalProgressBoxUpdate( (int) progressCounter );
        }

    }

    relationCurrentRename(tr("d-regular"), true);

    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexEdgeCount);

}




/**
 * @brief Creates a random ring lattice network.
 * @param vert
 * @param degree
 * @param x0
 * @param y0
 * @param radius
 * @param updateProgress
 */
void Graph::randomNetRingLatticeCreate(const int &N, const int &degree,
                                       const bool updateProgress)
{
    qDebug()<< "Creating ring lattice random network...";
    int x=0;
    int y=0;
    int progressCounter=0;

    double x0 = canvasWidth/2.0;
    double y0 =canvasHeight/2.0;
    double radius = canvasMaxRadius();
    double rad= (2.0* M_PI/ N );

    setDirected(false);

    randomizeThings();

    vpos.reserve(N);

    QString pMsg  = tr( "Creating ring-lattice network. \n"
                        "Please wait..." );
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate (N, pMsg );

    for (int i=0; i< N ; i++) {
        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);
//        qDebug() << "creating new vertex " << i+1;
        vertexCreate( i+1,initVertexSize,initVertexColor,
                      initVertexNumberColor, initVertexNumberSize,
                      QString::number (i+1), initVertexLabelColor,  initVertexLabelSize,
                      QPoint(x, y), initVertexShape, initVertexIconPath, false);

    }
    int target = 0;
    for (int i=0;i<N; i++){
//        qDebug("Creating links for node %i = ", i+1);
        for (int j=0; j< degree/2 ; j++) {
            target = i + j+1 ;
            if ( target > (N-1))
                target = target-N;
//            qDebug("Creating Link between %i  and %i", i+1, target+1);
            edgeCreate(i+1, target+1, 1, initEdgeColor,
                       EdgeType::Undirected, false, false,
                       QString(), false);
        }
        if (updateProgress) {
            emit signalProgressBoxUpdate( ++progressCounter );
        }
    }

    if (updateProgress) {
        relationCurrentRename(tr("ring-lattice"), true);
        emit signalProgressBoxKill();
    }

    setModStatus(ModStatus::VertexEdgeCount, updateProgress);

}







/**
 * @brief Creates a lattice network
 * @param N
 * @param length
 * @param dimension
 * @param neighborhood
 * @param mode
 * @param diag
 */
void Graph::randomNetLatticeCreate(const int &N,
                                   const int &length,
                                   const int &dimension,
                                   const int &neighborhoodLength,
                                   const QString &mode,
                                   const bool &circular){
    qDebug() << "Creating lattice network...";
    Q_UNUSED(circular);
    Q_UNUSED(dimension);
    if (mode=="graph") {
        setDirected(false);
    }

    int x = 0;
    int y = 0;
    int nCount = 0;
    double nodeHPadding = 0;
    double nodeVPadding = 0;
    double canvasPadding = 100;
    qreal progressCounter=0;
    qreal progressFraction =0;

    int target = 0;

    QList<QString> latticeEdges;
    QStringList m_edge;
    QString edge;
    QString oppEdge;


    randomizeThings();

    vpos.reserve(N);

    QString pMsg = tr( "Creating lattice network. \n"
                       "Please wait..." );
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate (N, pMsg );


    // create vertices

    qDebug()<< "creating vertices";

    nCount = 0;
    canvasPadding = 20;
    nodeHPadding= ( canvasWidth )  / (double) ( length + 2);
    nodeVPadding= ( canvasHeight ) / (double) ( length + 2);

    qDebug()<< "canvasPadding" << canvasPadding
            << "nodeHPadding"<<nodeHPadding;

    for (int i=0; i < length ; i++) {

        // compute vertical pos
        y =  canvasPadding + nodeVPadding * (i+1) ;

        for (int j=0; j < length ; j++) {
            nCount ++ ;
            // compute horizontal pos
            x = canvasPadding + nodeHPadding * (j+1) ;

//            qDebug() << "creating new vertex at"
//                     << x << "," << y;

            // create vertex
            vertexCreate(
                        nCount, initVertexSize,
                        initVertexColor,
                        initVertexNumberColor,
                        initVertexNumberSize,
                        QString::number (nCount),
                        initVertexLabelColor,
                        initVertexLabelSize,
                        QPoint(x, y),
                        initVertexShape,
                        initVertexIconPath,
                        false
                        );
        }
    }

    //
    // compute and then draw edges
    //

    qDebug()<< "Computing edges";

    if (mode=="graph") {

        // undirected graph

        for (int i=1;i<=N; i++){

//            qDebug()<< "Creating undirected edges for node  "<< i;

            for (int j=1; j< neighborhoodLength+1 ; j++) {

                for (int p = 0; p < 2; p++ ) {

                    for (int q = 0; q < 2; q++ ) {

                        target = i + pow((-1), p) * j * pow (length, q);



                        if ( i % length == 0  && target == i + 1) {
//                            qDebug()<< i << "<->"<< target << "OOB RIGHT";
                            continue;
                        }
                        if ( i % length == 1  && target == i - 1) {
//                            qDebug()<< i << "<->"<< target << "OOB LEFT";
                            continue;
                        }
                        if ( target > N  ) {
//                            qDebug()<< i << "<->"<< target << "OOB DOWN";
                            target = target % N ;
                            continue;
                        }

                        if ( target < 1 ) {
//                            qDebug()<< i << "<->"<< target << "OOB UP";
                            target =  N - target ;
                            continue;
                        }
//                        qDebug()<< i << "<->"<< target << "OK";

                        edge = QString::number(i)+"<->"+QString::number(target);
                        oppEdge = QString::number(i)+"<->"+QString::number(target);

                        if ( !latticeEdges.contains(edge) && !latticeEdges.contains(oppEdge) ) {
                            latticeEdges.append(QString::number(i)+"<->"+QString::number(target));
                        }

                    }
                }


                //// up
                // target = i-j*length;
                //// pre
                //target = i-j;

                //// same
                // i

                //// next
                //target = i+j;

                //// down
                //target = i+j*length;


            }
        }

    }

    else {

        // directed graph

    }

    //
    // draw edges
    //

    qDebug()<< "drawing edges";

    for (int i = 0; i < latticeEdges.size(); ++i) {

        m_edge = latticeEdges.at(i).split("<->");

//        qDebug() << "Drawing undirected Edge no" << i + 1 << ":"
//                 << m_edge[0].toInt(0) << "<->" << m_edge[1].toInt(0);

        edgeCreate(m_edge[0].toInt(0), m_edge[1].toInt(0), 1,
                initEdgeColor,
                (isUndirected()) ? EdgeType::Undirected : EdgeType::Directed,
                (isUndirected()) ? false:true,
                false,
                QString(), false);

        progressCounter +=progressFraction;

    }

    relationCurrentRename(tr("lattice"), true);

    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexEdgeCount);

}




/**
 * @brief Calculates and returns the number of walks of a given length between v1 and v2
 * @param v1
 * @param v2
 * @param length
 * @return
 */
int Graph::walksBetween(int v1, int v2, int length) {
    graphWalksMatrixCreate(length);
    return XM.item(v1-1,v2-1);
}




/**
 * @brief Computes either the "Walks of given length" or the "Total Walks" matrix.
 * If length>0, it computes the Walks of given length matrix, XM=AM^l
 * where each element (i,j) denotes the number of walks of length l between vertex i and j.
 * If length=0, it computes the Total Walks matrix, XSM=Sum{AM^n} where each (i,j)
 * denotes the total number of walks of any length between vertices i and j.
 * NOTE: In the latter case, this function is VERY SLOW on large networks (n>50),
 * since it will calculate all powers of the sociomatrix up to n-1 in order to find out all
 * possible walks.
 * @param length
 * @param updateProgress
 */
void Graph::graphWalksMatrixCreate(const int &N,
                                   const int &length,
                                   const bool &updateProgress) {

    bool dropIsolates=false;
    bool considerWeights=true;
    bool inverseWeights=false;
    bool symmetrize=false;


    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);

    if (length>0) {
        qDebug()<< "Graph::graphWalksMatrixCreate() - "
                   "Calculating sociomatrix power"  << length;

        QString pMsg = tr("Computing walks of length %1. \nPlease wait...").arg(length) ;
        emit statusMessage( pMsg  );
        if (updateProgress) {
            emit signalProgressBoxCreate(length,pMsg);
        }

        XM = AM.pow(length, false);

        if (updateProgress) {
            emit signalProgressBoxUpdate (length);
        }



    }
    else {
        qDebug()<< "Graph::graphWalksMatrixCreate() - "
                   "Calculating all sociomatrix powers up to"  << N-1;

        XM = AM;   // XM will be the product matrix

        XSM = AM;  // XSM is the sum of product matrices

        QString pMsg = tr("Computing sociomatrix powers up to %1. \nPlease wait...").arg(N-1) ;
        emit statusMessage( pMsg  );
        if (updateProgress) {
            emit signalProgressBoxCreate(N-1,pMsg);
        }


        for (int i=2; i <= (N-1) ; ++i) {

            emit statusMessage(tr("Computing all sociomatrix powers up to %1. "
                                 "Now computing A^%2. Please wait...").arg(N-1).arg(i));

            XM*=AM;
            //           qDebug() << "Graph::graphWalksMatrixCreate() i"<<i <<"XM=AM^i";
            //           XM.printMatrixConsole();


            XSM+=XM; // XSM becomes XSM+XM
            //           qDebug() << "Graph::graphWalksMatrixCreate() i"<<i <<"XSM=";
            //           XSM.printMatrixConsole();

            if (updateProgress) {
                emit signalProgressBoxUpdate (i);
            }

        }

        if (updateProgress) {
            emit signalProgressBoxUpdate (N-1);
        }

    }

    if (updateProgress) {
        emit signalProgressBoxKill();
    }
    //    qDebug()<< "AM + AM = ";
    //    (AM+AM).printMatrixConsole(true);

    //    qDebug()<< "AM += AM = ";
    //    AM+=AM;
    //    (AM).printMatrixConsole(true);


    //    qDebug()<< "XSM.product (AM,AM) ";
    //    XSM.product (AM, AM);
    //    (XSM).printMatrixConsole(true);

    //    qDebug()<< "XSM = AM * AM ";
    //    XSM = AM * AM;
    //    (XSM).printMatrixConsole(true);


}



/**
 * @brief Graph::writeWalksTotalMatrixPlainText
 * Writes the total number of walks matrix
 * @param fn
 * @param netName
 * @param length
 */
void Graph::writeWalksTotalMatrixPlainText(const QString &fn){

    qDebug()<<"I will write (plain-text) total walks to file:" << fn;

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer "<<  VERSION << "\n";
    outText << tr("Network name: ")<< getName()<< "\n"<< "\n";
    outText << "Total number of walks of any length less than or equal to "<< vertices()-1
            <<" between each pair of nodes \n\n";
    outText << "Warning: Walk counts consider unordered pairs of nodes\n\n";

    int N = vertices();

    graphWalksMatrixCreate( N, 0, true);

    outText << XSM ;

    file.close();

}


/**
 * @brief Graph::writeWalksOfLengthMatrixPlainText
 * @param fn
 * @param length
 */
void Graph::writeWalksOfLengthMatrixPlainText(const QString &fn, const int &length){

    qDebug()<<"I will write (plain-text) walks of length:" << length
           << "to file:" << fn;

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    emit statusMessage ( tr("Writing Walks matrix to file:") + fn );

    QTextStream outText(&file);

    outText << "-Social Network Visualizer "<<  VERSION <<"- \n";
    outText << "Network name: "<< getName()<<" \n";
    outText << "Number of walks of length "<< length <<" between each pair of nodes \n\n";

    int N = vertices();
    graphWalksMatrixCreate( N, length, true);

    outText << XM ;

    file.close();
}



/**
 * @brief Calls graphWalksMatrixCreate() to compute walks and writes the
 * Walks of given length matrix to a file in HTML.
 * If length = 0, it writes the Total Walks matrix.
 * @param fn
 * @param length
 * @param simpler
 */
void Graph::writeMatrixWalks (const QString &fn,
                              const int &length,
                              const bool &simpler) {

    qDebug()<<"I will write walks of length:" << length
           << "to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(simpler);

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    int N = vertices();

    emit statusMessage(tr("Computing Walks..."));
    graphWalksMatrixCreate(N, length, true);

    QTextStream outText( &file );

    outText << htmlHead;

    outText << "<h1>";

    if (length>0) {
        outText << tr("WALKS OF LENGTH %1 MATRIX").arg(length);
    }
    else {
        outText << tr("TOTAL WALKS MATRIX");
    }

    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    if (length>0) {
        outText << "<p class=\"description\">"
                << tr("The Walks of length %1 matrix is a NxN matrix "
                      "where each element (i,j) is the number of walks of "
                      "length %1 between actor i and actor j, "
                      "or 0 if no walk exists. <br />"
                      "A walk is a sequence of edges and vertices, where each edge's "
                      "endpoints are the two vertices adjacent to it. In a walk, "
                      "vertices and edges may repeat. <br />"
                      "Warning: Walks count unordered pairs of nodes. ").arg(length)
                << "</p>";
    }
    else {
        outText << "<p class=\"description\">"
                << tr("The Total Walks matrix of a social network is a NxN matrix "
                      "where each element (i,j) is the total number of walks of any "
                      "length (less than or equal to %1) between actor i and actor j, "
                      "or 0 if no walk exists. <br />"
                      "A walk is a sequence of edges and vertices, where each edge's "
                      "endpoints are the two vertices adjacent to it. In a walk, "
                      "vertices and edges may repeat. <br />"
                      "Warning: Walks count unordered pairs of nodes. ").arg(N-1)
                << "</p>";
    }

    emit statusMessage ( tr("Writing Walks matrix to file:") + fn );
    qDebug()<<"Graph::writeMatrixWalks() - Writing XM to file";


    if (length > 0) {
        //XM.printHTMLTable(outText);
        writeMatrixHTMLTable(outText,XM,true);
    }
    else {
        writeMatrixHTMLTable(outText,XSM,true);
        //XSM.printHTMLTable(outText);
    }

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Walks report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();
}







/**
 * @brief Returns the influence range of vertex v1, namely the set of nodes who are
 *  reachable from v1 (See Wasserman and Faust, pp.200-201, based on Lin, 1976).
 * The Influence Range of vertex v can also be defined as:
 * Ji = Sum [ D(v,j), iff D(v,j) != inf  ] for every j in V, where j!=v and D the distance matrix
 *  This function is for digraphs only
 * @param v1
 * @return
 */
QList<int> Graph::vertexinfluenceRange(int v1){

    qDebug() << "Graph::vertexinfluenceRange() - vertex:"<< v1;

    graphDistancesGeodesic(false);

    VList::const_iterator jt;

    int N = vertices( false, false, true);

    int progressCounter=0;
    int target = 0;

    influenceRanges.clear();
    influenceRanges.reserve(N);

    QString pMsg = tr("Creating Influence Range List. \nPlease wait ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);


    for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt) {

        emit signalProgressBoxUpdate(++progressCounter);

        target = (*jt)->number();

        if  ( ! (*jt)->isEnabled()  ) {
            qDebug() << "Graph::vertexinfluenceRange() - target:"
                     << target << "disabled. SKIP";
            continue;
        }

        if ( graphDistanceGeodesic(v1, target) != RAND_MAX ) {
            qDebug() << "Graph::vertexinfluenceRange() - v1 can reach:" << target;
            influenceRanges.insert(v1,target);

        }
    }

    emit signalProgressBoxKill();

    return influenceRanges.values(v1);

}




/**
 * @brief Returns the influence domain of vertex v1, namely the set of nodes who can
 *  reach v1
 * The Influence Domain Ii of vertex v can also be defined as:
 * Ii = Sum [ D(i,v), iff D(i,v) != inf  ] for every in V, where i!=v and D the distance matrix
 *  This function applies to digraphs only
 * @param v1
 * @return
 */
QList<int> Graph::vertexinfluenceDomain(int v1){
    qDebug() << "Graph::vertexinfluenceDomain() - vertex:"<< v1;

    graphDistancesGeodesic(false);

    VList::const_iterator it;

    int N = vertices( false, false, true);

    int progressCounter=0;
    int source = 0;


    influenceDomains.clear();
    influenceDomains.reserve(N);

    QString pMsg = tr("Creating Influence Domain List. \nPlease wait ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if  ( ! (*it)->isEnabled()  ) {
            qDebug() << "Graph::vertexinfluenceDomain() - "
                     << source << "disabled. SKIP";
            continue;
        }

        if ((*it)->distance( v1 ) != RAND_MAX ) {
            qDebug() << "Graph::vertexinfluenceDomain() - v1 reachable from:" << source;
            influenceDomains.insert(v1,source);

        }
    }

    emit signalProgressBoxKill();

    return influenceDomains.values(v1);

}








/**
    Writes the reachability matrix X^R of the graph to a file
*/
void Graph::writeReachabilityMatrixPlainText(const QString &fn, const bool &dropIsolates) {

    qDebug() << "Writing Reachability Matrix plain text to file:" << fn;

    QFile file (fn);

    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer "<< VERSION << "\n";
    outText << tr("Network name: ")<< getName()<< "\n\n";
    outText << tr("Reachability Matrix (XR)") << "\n";
    outText << tr("Two nodes are reachable if there is a walk between them (their geodesic distance is non-zero).") << "\n";
    outText << tr("If nodes i and j are reachable then XR(i,j)=1 otherwise XR(i,j)=0.") << "\n\n";

    graphDistancesGeodesic(false,false,false,dropIsolates);

    outText << XRM ;

    file.close();
}







/**
 * @brief Graph::writeClusteringCoefficient
 * Writes the clustering coefficients to a file
 * @param fileName
 * @param considerWeights
 */
void Graph::writeClusteringCoefficient( const QString fileName,
                                        const bool considerWeights) {

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    VList::const_iterator it;

    averageCLC= clusteringCoefficient(true);

    QString pMsg = tr("Writing Clustering Coefficients to file. \nPlease wait...");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("CLUSTERING COEFFICIENT (CLC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << tr("0 &le; CLC &le; 1 ")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CLC range: ")
            <<"</span>"
            << tr("0 &le; CLC' &le; 1 ")
            << "</p>";


    outText << "<table class=\"stripes sortable\">";

    outText << "<thead>"
            <<"<tr>"
            <<"<th id=\"col1\" onclick=\"tableSort(results, 0, asc1); asc1 *= -1; asc2 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Node")
            << "</th>"
            <<"<th id=\"col2\" onclick=\"tableSort(results, 1, asc2); asc2 *= -1; asc1 = 1; asc3 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("Label")
            << "</th>"
            <<"<th id=\"col3\" onclick=\"tableSort(results, 2, asc3); asc3 *= -1; asc1 = 1; asc2 = 1;asc4 = 1;asc5 = 1;\">"
            << tr("CLC")
            << "</th>"
            <<"<th id=\"col4\" onclick=\"tableSort(results, 3, asc4); asc4 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc5 = 1;\">"
            << tr("CLC'")
            << "</th>"
            <<"<th id=\"col5\" onclick=\"tableSort(results, 4, asc5); asc5 *= -1; asc1 = 1; asc2 = 1;asc3 = 1;asc4 = 1;\">"
            << tr("%CLC'")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody id=\"results\">";





    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        emit signalProgressBoxUpdate( ++progressCounter );

        rowCount++;

        outText << Qt::fixed;

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                <<"<td>"
                << (*it)->number()
                << "</td><td>"
                << ( (! ( (*it)->label().simplified()).isEmpty()) ? (*it)->label().simplified().left(m_reportsLabelLength) : "-" )
                << "</td><td>"
                << (*it)->CLC()
                << "</td><td>"
                << (*it)->CLC() / maxCLC
                << "</td><td>"
                << 100 * (*it)->CLC() / maxCLC
                << "</td>"
                <<"</tr>";

    }

    outText << "</tbody></table>";

    if ( minCLC ==  maxCLC) {
        outText << "<p>"
                << tr("All nodes have the same local CLC score.")
                << "</p>";
    }
    else {
        outText << "<p>";
        outText << "<span class=\"info\">"
                << tr("Max CLC = ")
                <<"</span>"
               << maxCLC <<" (node "<< maxNodeCLC  <<  ")"
               << "<br />"
               << "<span class=\"info\">"
               << tr("Min CLC = ")
               <<"</span>"
               << minCLC <<" (node "<< minNodeCLC <<  ")"
               << "<br />"
               << "</p>";
    }

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("CLC Mean = ")
            <<"</span>"
            << averageCLC
            <<"<br/>"
            << "<span class=\"info\">"
            << tr("CLC Variance = ")
            <<"</span>"
            << varianceCLC
            <<"<br/>";
    outText << "</p>";


    outText << "<h2>";
    outText << tr("GROUP / NETWORK AVERAGE CLUSTERING COEFFICIENT (GCLC)")
            << "</h2>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("GCLC = ")
            <<"</span>"
            <<  averageCLC
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
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();
}




//Writes the triad census to a file
void Graph::writeTriadCensus( const QString fileName,
                              const bool considerWeights) {

    qDebug() << "Graph::writeTriadCensus()";

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }

    QTextStream outText ( &file );

    emit statusMessage ( (tr("Computing triad census. Please wait....")) );

    if (  !calculatedTriad) {
        if (!graphTriadCensus()){
            qDebug() << "Error in graphTriadCensus(). Exiting...";
            file.close();
            return;
        }
    }

    int rowCount = 0;
    int N = vertices();
    int progressCounter = 0;

    QList<QString> triadTypes;
    triadTypes << "003" ;
    triadTypes << "012" ;
    triadTypes << "102" ;
    triadTypes << "021D";
    triadTypes << "021U";
    triadTypes << "021C";
    triadTypes << "111D";
    triadTypes << "111U";
    triadTypes << "030T";
    triadTypes << "030C";
    triadTypes << "201" ;
    triadTypes << "120D";
    triadTypes << "120U";
    triadTypes << "120C";
    triadTypes << "210" ;
    triadTypes << "300" ;


    QString pMsg = tr("Writing Triad Census to file. \nPlease wait...") ;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(16,pMsg);


    outText << htmlHead;

    outText << "<h1>";
    outText << tr("TRIAD CENSUS (TRC) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"<tr>"
            <<"<th>"
            << tr("Type")
            << "</th><th>"
            << tr("Census")
               //            << "</th><th>"
               //            << tr("Expected Value")
            <<"</th>"
           <<"</tr>"
          << "</thead>"
          <<"<tbody>";


    for (int i = 0 ; i<=15 ; i++) {

        emit signalProgressBoxUpdate(++progressCounter);

        rowCount = i + 1;
        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">"
                <<"<td>"
                << triadTypes[i]
                   << "</td><td>"
                << triadTypeFreqs[i]
                   << "</td>"
                <<"</tr>";
    }

    outText << "</tbody></table>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Triad Census report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();
}




/**
 * @brief Graph::writeCliqueCensus
 * Calls graphCliques() to compute all cliques (maximal connected subgraphs) of the network.
 * Then writes the results into a file, along with the Actor by clique analysis,
 * the Co-membership matrix and the Hierarchical clustering of overlap matrix
 * @param fileName
 * @param considerWeights
 */
bool Graph::writeCliqueCensus(const QString &fileName,
                              const bool considerWeights) {

    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug()<< "Graph::writeCliqueCensus() ";

    Q_UNUSED(considerWeights);

    QString varLocation = "Both";
    bool dendrogram = true;

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return false;
    }

    int N = vertices();
    int cliqueCounter=0;
    int rowCounter = 0;
    int cliqueSize = 0;
    int actor2 = 0, actor1=0, index1=0, index2=0;
    qreal numerator = 0;
    QString listString;

    VList::const_iterator it, it2;

    QString pMsg = tr("Computing Clique Census and writing it to a file. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(2*N,pMsg);

    // compute clique census
    pMsg = tr("Computing Clique Census. Please wait..") ;
    emit statusMessage ( pMsg );
    qDebug() << "Graph::writeCliqueCensus() - calling graphCliques";

    csRecDepth = 0;

    // Call graphCliques() to compute all cliques (maximal connected subgraphs) of the network.
    graphCliques();

    pMsg = tr("Writing Clique Census to file. Please wait..") ;
    emit statusMessage ( pMsg );


    QTextStream outText ( &file );

    outText << htmlHead;
    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << "<h1>";
    outText << tr("CLIQUE CENSUS (CLQs) REPORT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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
            <<"</span>"
            << m_cliques.size()
            << "</p>";

    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            <<"<tr>"
            <<"<th>"
            << tr("Clique No")
            << "</th><th>"
            << tr("Clique members")
            << "</th>"
            <<"</tr>"
          << "</thead>"
          <<"<tbody>";

    foreach (QList<int> clique, m_cliques) {

        ++cliqueCounter;

        outText << "<tr class=" << ((cliqueCounter%2==0) ? "even" :"odd" )<< ">";

        listString.truncate(0);

        while (!clique.empty()) {
            listString += QString::number (clique.takeFirst());
            if (!clique.empty()) listString += " ";
        }
        outText <<"<td>"
                << cliqueCounter
                << "</td><td>"
                << listString
                << "</td>"
                <<"</tr>";

    }
    outText << "</tbody></table>";

    /**
     * Write the actor by clique analysis matrix.
     * For each actor-clique pair, we compute the proportion of clique members adjacent
     */
    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Actor by clique analysis: ")
            <<"</span>"
            << tr("Proportion of clique members adjacent")
            << "</p>";


    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            <<"<tr>"
            <<"<th>"
            << tr("<sub>Actor</sub>/<sup>Clique</sup>")
            << "</th>";


    for (int listIndex=0; listIndex<cliqueCounter; listIndex++ ) {
        outText << "<th>"
                 << listIndex+1
                 << "</th>";
    }

    outText <<"</tr>"
           << "</thead>"
           <<"<tbody>";

    rowCounter = 0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        rowCounter++;
        actor1 = (*it)->number();
        outText << "<tr class=" << ((rowCounter%2==0) ? "even" :"odd" )<< ">"
                <<"<td class=\"header\">"
                << actor1
                <<"</td>";

        foreach (QList<int> clique, m_cliques) {
            numerator = 0;

            if (clique.contains( actor1 )){
                outText <<"<td>"
                        << "1.000"
                        <<"</td>";
            }
            else {
                cliqueSize = clique.size();
                while (!clique.empty()) {
                    actor2 = clique.takeFirst();
                    if (  edgeExists( actor1, actor2) ) {
                        numerator++;
                    }

                }
                outText <<"<td>"
                        << Qt::fixed << (numerator/(qreal) cliqueSize)
                        <<"</td>";

            }
        }
        outText <<"</tr>";


    }
    outText << "</tbody></table>";

    /**
     * Write the actor by actor analysis matrix.
     * For each pair, we compute their clique co-membership
     */

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Actor by actor analysis: ")
            <<"</span>"
            << tr(" Co-membership matrix")
            << "</p>";


    outText << "<table class=\"stripes\">";
    outText << "<thead>"
            <<"<tr>"
            <<"<th>"
            << tr("<sub>Actor</sub>/<sup>Actor</sup>")
            << "</th>";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        actor1 = (*it)->number();
        outText << "<th>"
                << actor1
                << "</th>";
    }

    outText <<"</tr>"
           << "</thead>"
           <<"<tbody>";

    rowCounter=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        actor1 = (*it)->number();
        index1 = vpos[actor1];
        rowCounter++;
        outText << "<tr class=" << ((rowCounter%2==0) ? "even" :"odd" )<< ">"
                <<"<td class=\"header\">"
                << actor1
                <<"</td>";

        for (it2=m_graph.cbegin(); it2!=m_graph.cend(); ++it2){
            actor2 =  (*it2)->number();
            index2 = vpos[actor2];
            outText <<"<td>"
                    << qSetRealNumberPrecision(0)<< CLQM.item(index1, index2)
                    <<"</td>";
        }
        outText <<"</tr>";
    }

    outText << "</tbody></table>";

    /**
     * Write the Hierarchical clustering of overlap matrix
     */


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Hierarchical clustering of overlap matrix: ")
            <<"</span>"
            << tr("Actors")
            << "</p>";

    pMsg = tr("Computing HCA for Cliques. Please wait..") ;
    emit statusMessage ( pMsg );
    if (! graphClusteringHierarchical(CLQM,
                                      varLocation,
                                      graphMetricStrToType("Euclidean"),
                                      Clustering::Complete_Linkage,
                                      false,
                                      true,
                                      true,
                                      false,
                                      true) ) {
        file.close();
        emit statusMessage( "Error completing HCA analysis");
        emit signalProgressBoxKill();
        return false;
    }

    pMsg = tr("Writing HCA for Cliques. Please wait..") ;
    emit statusMessage ( pMsg );

    writeClusteringHierarchicalResultsToStream(outText, N, dendrogram);




    outText << "<p>"
           << "<span class=\"info\">"
           << tr("Clique by clique analysis: ")
           <<"</span>"
           << tr("Co-membership matrix")
           << "</p>";


    emit signalProgressBoxUpdate(2 * N);

    outText << "<p>"
           << "<span class=\"info\">"
           << tr("Hierarchical clustering of overlap matrix: ")
           <<"</span>"
           << tr("Clique")
           << "</p>";


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Clique Census Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxKill();

    return true;
}



/**
 * @brief Called from Graph::graphCliques to add a new clique (list of vertices)
 * Adds clique info to each clique member and updates co-membership matrix CLQM .
 * @param list
 * @return
 */
void Graph:: graphCliqueAdd(const QList<int> &clique){

    m_cliques.insert(clique.size(), clique);

    qDebug() << "Graph::graphCliqueAdd() - Added clique:"
             << clique
             << "of size"
             << clique.size()
             << "Total cliques:"
             << m_cliques.size();
    int index1=0, index2=0, cliqueCount=0;
    foreach (int actor1, clique) {
        index1 = vpos[actor1];
        qDebug() << "Graph::graphCliqueAdd() - Updating cliques in actor1:"
                << actor1
                << "vpos:"
                << index1;
        m_graph[ index1 ]->cliqueAdd(clique);
        foreach (int actor2, clique) {
            index2 = vpos[actor2];
            cliqueCount = CLQM.item(index1, index2);
            CLQM.setItem( index1, index2, ( cliqueCount + 1)  );
            qDebug() << "Graph::graphCliqueAdd() - Updated co-membership matrix CLQM"
                    << "actor1:"
                    << actor1
                    << "actor2:"
                    << actor2
                    <<"old matrix element: ("
                    << index1<<","<<index2 <<")="<<cliqueCount
                    <<" -- updated to:"
                    << CLQM.item(index1, index2);
        }
    }

}


/**
 * @brief Finds all maximal cliques in an undirected (?) graph.
 * Implements the Bron–Kerbosch algorithm, a recursive backtracking algorithm
 * that searches for all maximal cliques in a given graph G.
 * Given three sets R, P, and X, the algorithm finds the maximal cliques that
 * include all of the vertices in R, some of the vertices in P, and none of
 * the vertices in X.
 * In each call to the algorithm, P and X are disjoint sets whose union consists
 * of those vertices that form cliques when added to R.
 * In other words, P ∪ X is the set of vertices which are joined to every element of R.
 * When P and X are both empty there are no further elements that can be added to R,
 * so R is a maximal clique and the algorithm outputs R.
 * The recursion is initiated by setting R and X to be the empty set and P to be
 * the vertex set of the graph.
 * Within each recursive call, the algorithm considers the vertices in P in turn.
 * if there are no vertices, it either reports R as a maximal clique (if X is empty),
 * or backtracks.
 * For each vertex v chosen from P, it makes a recursive call in which v is added to R
 * and in which P and X are restricted to the neighbor set NBS(v) of v,
 * which finds and reports all clique extensions of R that contain v.
 * Then, it moves v from P to X to exclude it from consideration in future cliques
 * and continues with the next vertex in P.
 * @param R
 * @param P
 * @param X
 */
void Graph::graphCliques(QSet<int> R, QSet<int> P, QSet<int> X) {

    csRecDepth ++ ;

    qDebug () << "Graph::graphCliques() - STARTS HERE. csRecDepth:"
              << csRecDepth
              << " - Check if we are at initialization step";

    QList<int> myNeightbors;

    if (R.isEmpty() && P.isEmpty() && X.isEmpty()){

        int V = vertices() ;
        P.reserve( V );
        R.reserve( V );
        X.reserve( V );
        P=verticesSet();

        qDebug() << "Graph::graphCliques() - initialization step. R, X empty and P=V(G): "
                 << P;

        CLQM.zeroMatrix(V,V);  //co-membership matrix CLQM

        m_cliques.clear();

        VList::const_iterator it;
        int vertex=0;
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)     {
            vertex = (*it)->number();

            myNeightbors = (*it)->neighborhoodList();
            neighboursHash[ vertex ] = QSet<int>(myNeightbors.constBegin(),myNeightbors.constEnd());

            qDebug() << "Graph::graphCliques() - initialization step. NeighborhoodList of v"
                     << vertex
                     << ": "
                     <<neighboursHash[ vertex ] ;
            (*it)->clearCliques();
        }
    }

    qDebug() << "Graph::graphCliques() - check if P and X are both empty (which would mean we have a clique in R)...";

    if (P.isEmpty() && X.isEmpty()) {

        qDebug() << "Graph::graphCliques() - P and X are both empty. MAXIMAL clique R=" << R;

        QList<int> clique = R.values();

        graphCliqueAdd(clique);

        csRecDepth -- ;

        return;

    }

    int v;

    QSet<int> NBS;

    QSet<int> Rnext, Pnext, Xnext;

    QSet<int>::iterator i = P.begin();

    int counter = 0;

    // Loop over vertices in P, randomly

    qDebug() << "Graph::graphCliques() - Start looping over vertices in P (randomly)";

    while( i != P.end()) {

        counter ++ ;

        v = *i;

        qDebug() << "Graph::graphCliques() - CURRENT v:" << v
                 << " P:" << P << " P.count=" <<P.size()
                 << " R:" << R
                 << " X:" << X ;

        NBS = neighboursHash[ v ];

        if (NBS.size() == 1 && NBS.contains(v)) {

            qDebug() << "Graph::graphCliques() - v:" << v
                     << "has only a tie to itself";

            //graphCliques( R, P, X );

            ++i;

            continue;

        }

        Rnext = R;
        Rnext << v ;
        Pnext = P&NBS;
        Xnext = X&NBS;

        qDebug() << "Graph::graphCliques() - v:" << v
                 << "RECURSIVE CALL to graphCliques ( R ⋃ {v}, P ⋂ NB(v), X ⋂ NBS(v) )"
                    << "\n" << "NBS(v):" << NBS
                    << "\n" << "Rnext = R ⋃ {v}:" << Rnext
                    << "\n" << "Pnext = P ⋂ NBS(v):" << Pnext
                    << "\n" << "Xnext = X ⋂ NBS(v):" << Xnext;

        if (csRecDepth==1) {
            emit signalProgressBoxUpdate(counter);
            emit statusMessage ( tr("Finding cliques: Recursive backtracking for actor ") + QString::number(v));
        }

        // find all clique extensions of R that contain v
        try {
            graphCliques( Rnext, Pnext, Xnext );
        } catch (...) {
            qDebug() << "Graph::graphCliques() - ERROR";
            return;
        }


        // Set P = P \ v
        i=P.erase(i);    //P-=v;

        // Set X = X + v
        X.insert(v);

        qDebug() << "Graph::graphCliques() - v:" << v
                 << "RETURNED from recursive call - recDepth: "
                 << csRecDepth
                 << " Moved v:"<<  v
                 <<" from P to X to be excluded in the future"
                 << " P=" << P << " P.count:" <<P.size()
                 << " R=" << R << " R.count:" <<R.size()
                 << " X=" << X << " X.count:" <<X.size()
                 << " Continuing with next v in P";
        //++i;

    } //end while loop

    qDebug() << "Graph::graphCliques() - FINISHED loop over P:" << P
             << "at csRecDepth:" <<  csRecDepth;

    csRecDepth -- ;

}



/**
    Returns the number of maximal cliques which include a given actor
*/	
int Graph::graphCliquesContaining(const int &actor, const int &size){
    qDebug() << "*** Graph::graphCliquesContaining(" <<  actor << ")";
    int cliqueCounter = 0;
    foreach (QList<int> clique, m_cliques) {
        if ( size!=0  )  {
            if ( clique.size() != size)
                continue;
        }
        if (clique.contains( actor )){
            cliqueCounter++;
        }
    }
    return cliqueCounter;

}



/**
 * @brief Graph::graphCliquesOfSize
 * Returns the number of maximal cliques of a given size
 * @param size
 * @return
 */
int Graph::graphCliquesOfSize(const int &size){
    qDebug() << "Graph::graphCliquesOfSize()";

    return m_cliques.values(size).size();

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
                                        const bool &dropIsolates) {


    QElapsedTimer computationTimer;
    computationTimer.start();

    qDebug()<< "Graph::writeClusteringHierarchical() - matrix:"
            << matrix
            << "varLocation" << varLocation
            << "metric" << metric
            << "method" << method
            << "considerWeights:"<<considerWeights
            << "inverseWeights:"<<inverseWeights
            << "dropIsolates:" << dropIsolates;

    int N = vertices(dropIsolates,false, true);

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return false;
    }

    emit statusMessage ( tr("Computing hierarchical clustering. Please wait... "));

    Matrix STR_EQUIV;

    switch (graphMatrixStrToType(matrix)) {
    case MATRIX_ADJACENCY:
        createMatrixAdjacency(dropIsolates);
        STR_EQUIV=AM;
        break;
    case MATRIX_DISTANCES:
        graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);
        STR_EQUIV=DM;
        break;
    default:
        break;
    }

    if (! graphClusteringHierarchical(STR_EQUIV,
                                      varLocation,
                                      graphMetricStrToType(metric),
                                      graphClusteringMethodStrToType(method),
                                      diagonal,
                                      dendrogram,
                                      considerWeights,
                                      inverseWeights,
                                      dropIsolates) ) {
        qDebug()<< "Graph::writeClusteringHierarchical() - HCA failed. Returning...";
        emit statusMessage( "Error completing HCA analysis");
        emit signalProgressBoxKill();
        return false;
    }

    QTextStream outText ( &file );

    QString pMsg = tr("Writing Hierarchical Cluster Analysis to file. \nPlease wait... ");
    emit statusMessage ( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);
    outText.reset();

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("HIERARCHICAL CLUSTERING (HCA)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
           << getName()
           <<"<br />"
          << "<span class=\"info\">"
          << tr("Actors: ")
          <<"</span>"
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
            <<"</span>"
           << metric
           << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Clustering method/criterion: ")
            <<"</span>"
           << method
           << "</p>";

    outText << "<p>&nbsp;</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Analysis results")
            <<"</span>"
           << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Structural Equivalence Matrix: ")
            <<"</span>"
           << "</p>";

    emit signalProgressBoxUpdate( N /3);

    writeMatrixHTMLTable(outText,STR_EQUIV,true,false,false, dropIsolates);
    //STR_EQUIV.printHTMLTable(outText,true,false);

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Hierarchical Clustering of Equivalence Matrix: ")
            <<"</span>"
           << "</p>";


    emit signalProgressBoxUpdate( 2* N /3);
    writeClusteringHierarchicalResultsToStream(outText, N, dendrogram);


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Hierarchical Cluster Analysis report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();
    qDebug()<< "Graph::writeClusteringHierarchical() - finished";

    emit signalProgressBoxUpdate( N);
    emit signalProgressBoxKill();

    return true;

}


/**
 * @brief Writes Hierarchical Clustering results to given output stream
 * Before running this methos, the method Graph::graphClusteringHierarchical()
 * must execute and return true. Otherwise, the result is unpredictable...
 * @param outText
 * @param N
 * @param dendrogram
 */
void Graph::writeClusteringHierarchicalResultsToStream(QTextStream& outText,
                                                       const int N,
                                                       const bool &dendrogram) {


    qDebug()<<"Writing Hierarchical Clustering results to stream. "
           << "N" << N
           << "dendrogram" << dendrogram;

    QMap<int, V_int>::const_iterator it;
    qreal level;

    outText << "<pre>";
    outText <<"Seq" << "\t"<<"Level" << "\t"<< "Actors" << "\n";

    for ( it= m_clustersPerSequence.constBegin() ; it != m_clustersPerSequence.constEnd(); ++it) {
        level = m_clusteringLevel.at (it.key() -1 );
        outText <<it.key()<< "\t"
                << level << "\t" ;

        foreach (int item, it.value() ) {
            outText << item << " " ;
        }
        outText << "\n";

    }
    outText << Qt::reset << "</pre>";

    if (dendrogram) {

        qDebug()<< "Writing SVG dendrogram...";

        outText << "<p>"
                << "<span class=\"info\">"
                << tr("Clustering Dendrogram (SVG)")
                <<"</span>"
               << "</p>";

        int diagramMaxWidth = 1000;
        int diagramPaddingLeft=30;
        int diagramPaddingTop =30;
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

        QMap<int, V_str>::const_iterator pit; //cluster names pair iterator

        int actorNumber;

        qreal maxLevelValue;
        QString clusterName;
        QList<qreal> legendLevelsDone;

        it = m_clustersPerSequence.constEnd();
        it--;

        maxLevelValue = m_clusteringLevel.last() ;

        qDebug() << "m_clustersPerSequence"<<m_clustersPerSequence
               << "\n"
               << "maxLevelValue"<<maxLevelValue
               << "\n"
               << "m_clusterPairNamesPerSeq" << m_clusterPairNamesPerSeq << "\n"
               << "m_clustersByName" << m_clustersByName;


        outText << "<div class=\"dendrogram\">";

        outText << "<svg class=\"dendrosvg SocNetV-v"<< VERSION
                <<"\" width=\""<< maxSVGWidth
               <<"\" height=\"" <<maxSVGHeight
              << "\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

        // print a legend on top
        outText << "<text font-size=\""<< headerTextSize
                << "\" class=\"header\" x=\"" << 0
                <<"\" y=\"" << headerHeight
               <<"\">" << "Actor"
              <<"</text>";
        outText << "<text font-size=\""<< headerTextSize
                << "\" class=\"header\" x=\"" << diagramMaxWidth / 2
                <<"\" y=\"" << headerHeight
               <<"\">" << "Clusterings"
              <<"</text>";

        // print actor numbers
        // and compute initial cluster end points for them.
        for ( int i=0; i < it.value().size() ; ++i ) {

            actorNumber = it.value().at(i);
            clusterEndPoint[QString::number(actorNumber)] = QPoint(diagramPaddingLeft,diagramPaddingTop+rowHeight*(i));

            outText << "<g class=\"row row-" << i << "\">";
            outText << "<text class=\"actor\" font-size=\""<< actorTextSize
                    << "\" x=\"" << rowPaddingLeft
                    <<"\" y=\"" << diagramPaddingTop + (rowHeight*(i)) + actorTextSize / 3
                   <<"\">" << actorNumber
                  <<"</text>";

            outText << "</g>";    // end actor name

        }                         // end for rows


        // begin drawing clustering paths/lines
        for ( pit= m_clusterPairNamesPerSeq.constBegin() ; pit != m_clusterPairNamesPerSeq.constEnd(); ++pit) {
            level = m_clusteringLevel.at ( pit.key() - 1);
            qDebug() << "seq" <<pit.key()
                     << "level" << level
                     << "cluster pair" << pit.value();

            for ( int i=0; i < pit.value().size() ; ++i ) {

                clusterName = pit.value().at(i);
                qDebug() << "clusterName" <<clusterName;

                if (i==0) {
                    endPoint1 = clusterEndPoint.value(clusterName, QPoint());
                    qDebug() << "endPoint1" <<endPoint1;
                }
                else {
                    endPoint2 = clusterEndPoint.value(clusterName, QPoint());
                    qDebug() << "endPoint2" <<endPoint2;
                }

            }

            if (endPoint1.isNull() || endPoint2.isNull()) {
                continue;
            }

            // compute and save new endPoint
            endPointLevel = QPoint ( ceil(diagramPaddingLeft + diagramMaxWidth * ( level / maxLevelValue)),
                                     ceil(endPoint1.y() + endPoint2.y())/2);

            clusterEndPoint.insert("c"+QString::number(pit.key()), endPointLevel);

            qDebug()<<"(pit.key() / maxLevelValue)" << ( diagramPaddingLeft + level / maxLevelValue)
                   <<"endPointLevel" <<endPointLevel ;

            // print path
            outText << "<path d=\"M "
                    << endPoint1.x()
                    << " " <<endPoint1.y()
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
                       "stroke-linecap=\"round\" stroke-width=\"1\" fill=\"none\"/>"; //stroke-dasharray=\"5,5\"


            // print level vertical dashed line
            outText << "<path d=\"M "
                    << endPointLevel.x()
                    << " "
                    << diagramPaddingTop - 10
                    << " L "
                    << endPointLevel.x()
                    << " "
                    << diagramPaddingTop  + rowHeight*(N) -10
                    << "\" stroke=\"#999\" "
                       "stroke-linecap=\"round\" stroke-dasharray=\"1,2\" stroke-width=\"0.4\" fill=\"none\"/>";


            //print legend
            if (!legendLevelsDone.contains(level)) {
                outText << "<text class=\"legend\"  writing-mode=\"tb-rl\" "
                           "glyph-orientation-vertical=\"90\" "
                           "font-size=\""
                        << legendTextSize
                        << "\" x=\"" << diagramPaddingLeft  + diagramMaxWidth * (level / maxLevelValue) - 5
                        << "\" y=\""
                        << diagramPaddingTop  + rowHeight*(N)
                        << "\" >" << Qt::fixed << level <<"</text>";
                legendLevelsDone.append(level);
            }

        }

        outText << "</svg>";        //end dendrogram svg

        outText << "</div>";        //end dendrogram div

    }       // end if dendrogram

}


/**
 * @brief Performs an hierarchical clustering process (Johnson, 1967) on a given
 * NxN distance/dissimilarity matrix. The input matrix can be the
 * the adjacency matrix, the geodesic distance matrix or a derived from them
 * dissimilarities matrix using a user-specified metric, i.e. euclidean distance.
 * The method parameter defines how to compute distances (similarities) between
 * a new cluster the old clusters. Valid values can be:
 * - Clustering::Single_Linkage: "single-link" or "connectedness" or "minimum"
 * - Clustering::Complete_Linkage: "complete-link" or "diameter" or "maximum"
 * - Clustering::Average_Linkage: "average-link" or UPGMA
 * @param matrix
 * @param metric
 * @param method
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
bool Graph::graphClusteringHierarchical(Matrix &STR_EQUIV,
                                        const QString &varLocation,
                                        const int &metric,
                                        const int &method,
                                        const bool &diagonal,
                                        const bool &diagram,
                                        const bool &considerWeights,
                                        const bool &inverseWeights,
                                        const bool &dropIsolates) {

    Q_UNUSED (inverseWeights);

    qDebug() << "Graph::graphClusteringHierarchical() - "
             << "metric" << metric
             << "method" << graphClusteringMethodTypeToString(method)
             << "diagonal" << diagonal
             << "diagram" << diagram
             << "dropIsolates" << dropIsolates;

    qDebug() << "Graph::graphClusteringHierarchical() - STR_EQUIV matrix:";
    //STR_EQUIV.printMatrixConsole(true);

    qreal min=RAND_MAX;
    qreal max=0;
    int imin, jmin, imax, jmax, mergedClusterIndex, deletedClusterIndex ;
    qreal distanceNewCluster;

    // temporarily stores cluster members at each clustering level
    QList<int> clusteredItems;

    // maps original and clustered items per their DSM matrix index
    // so that we know that at Level X the matrix index 0 corresponds to the cluster i.e. { 1,2,4}
    QMap <int, V_int> m_clustersIndex;
    QMap<int,V_int>::iterator it;
    QMap<int,V_int>::iterator prev;

    QMap<QString,V_int>::const_iterator sit;

    // variables for diagram computation
    QList<QString> clusterPairNames;
    QString cluster1, cluster2;

    Matrix DSM;  //dissimilarities matrix. Note: will be destroyed in the end.

    // TODO: needs fix when distances matrix with -1 (infinity) elements is used.

    // compute, if needed, the dissimilarities matrix
    switch (metric) {
    case METRIC_NONE:
        DSM=STR_EQUIV;
        break;
    case METRIC_JACCARD_INDEX:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric,varLocation,diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_MANHATTAN_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric,varLocation,diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_HAMMING_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric,varLocation,diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_EUCLIDEAN_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric,varLocation,diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_CHEBYSHEV_MAXIMUM:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric,varLocation,diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    default:
        break;
    }


    int N = DSM.rows();


    qDebug() << "Graph::graphClusteringHierarchical() -"
             << "initial matrix DSM contents:";
    //DSM.printMatrixConsole();

    if (DSM.illDefined()) {
        //        DSM.clear();
        //        STR_EQUIV.clear();
        emit statusMessage("ERROR computing dissimilarities matrix");
        return false;
    }

    clusteredItems.reserve(N);
    if (diagram) {
        clusterPairNames.reserve(N);
    }


    m_clustersIndex.clear();

    m_clustersPerSequence.clear();
    m_clusteringLevel.clear();

    m_clustersByName.clear();
    m_clusterPairNamesPerSeq.clear();

    //
    //Step 1: Assign each of the N items to its own cluster.
    //        We have N unit clusters
    //
    int clustersLeft = N;
    int seq = 1 ; //clustering stage/level sequence number

    VList::const_iterator vit;
    int i = 0;
    for ( vit=m_graph.cbegin(); vit!=m_graph.cend(); ++vit){
        //        if (dropIsolates) {
        //            if ((*vit)->isIsolated()) {
        //                continue;
        //            }
        //        }
        //         if (!(*vit)->isEnabled()) {
        //            continue;
        //         }

        if ((*vit)->isEnabled() && ( ! (*vit)->isIsolated() ) ) {
            clusteredItems.clear();
            clusteredItems << (*vit)->number();
            m_clustersIndex[i] = clusteredItems;
            if (diagram) {
                m_clustersByName.insert(QString::number(i+1),clusteredItems );
            }
            i++;
        }


    }


    QString pMsg=tr("Computing Hierarchical Clustering. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    while (clustersLeft > 1)
    {

        emit signalProgressBoxUpdate(seq);

        qDebug()<<"matrix DSM contents now:";
        //DSM.printMatrixConsole();

        //
        //Step 2. Find the most similar pair of clusters.
        //        Merge them into a single new cluster.
        //
        DSM.NeighboursNearestFarthest(min, max, imin, jmin, imax, jmax);
        mergedClusterIndex = (imin < jmin ) ? imin : jmin;
        deletedClusterIndex =  (mergedClusterIndex  == imin ) ? jmin : imin;

        m_clusteringLevel << min;

        clusteredItems.clear();
        clusteredItems = m_clustersIndex[mergedClusterIndex] + m_clustersIndex[deletedClusterIndex] ;

        qDebug() << "Graph::graphClusteringHierarchical() -"
                 << "level"<< min
                 << "seq" << seq
                 <<"clusteredItems in level"  <<clusteredItems;

        m_clustersPerSequence.insert( seq, clusteredItems);

        if (diagram) {

            cluster1.clear();
            cluster2.clear();
            clusterPairNames.clear();

            for ( sit= m_clustersByName.constBegin() ; sit != m_clustersByName.constEnd(); ++sit) {
                if (sit.value() == m_clustersIndex[mergedClusterIndex] ) {
                    cluster1 = sit.key();
                }
                else if (sit.value() == m_clustersIndex[deletedClusterIndex] ) {
                    cluster2 = sit.key();
                }
            }
            if (cluster1.isNull() && m_clustersIndex[mergedClusterIndex].size() == 1) {
                cluster1 = QString::number( m_clustersIndex[mergedClusterIndex].first() );
            }
            if (cluster2.isNull() && m_clustersIndex[deletedClusterIndex].size() == 1) {
                cluster1 = QString::number( m_clustersIndex[deletedClusterIndex].first() );
            }

            clusterPairNames.append(cluster1);
            clusterPairNames.append(cluster2);

            m_clusterPairNamesPerSeq.insert(seq, clusterPairNames);

            m_clustersByName.insert("c"+QString::number(seq),clusteredItems );

            qDebug() << "Computing diagram variables..."<< "\n"
                     << "cluster1"<< cluster1 << "\n"
                     << "cluster2"<< cluster2 << "\n"
                     << "clusterPairNames" << clusterPairNames << "\n"
                     << "m_clusterPairNamesPerSeq" << m_clusterPairNamesPerSeq << "\n"
                     << "m_clustersByName" <<m_clustersByName;


        } //end if diagram


        // map new cluster to a matrix index
        m_clustersIndex[mergedClusterIndex] = clusteredItems ;

        qDebug() << "  Clustering seq:"
                 << seq << "\n"
                 << "  Level:" << min << "\n"
                 << "  Neareast neighbors: ("<< imin+1 <<","<<jmin+1<<")"
                 << "Minimum/distance:" << min << "\n"
                 << "  Farthest neighbors: ("<< imax+1 <<","<<jmax+1<<")"
                 << "Maximum/distance:" << max << "\n"
                 << "  Merge nearest neighbors into a single new cluster:"
                 << mergedClusterIndex +1 << "\n"
                 << "  m_clustersPerSequence" << m_clustersPerSequence;

        qDebug() << "  Remove key"<< deletedClusterIndex
                 << "and shift next values to left... " ;

        it = m_clustersIndex.find(deletedClusterIndex);

        while (it != m_clustersIndex.end()) {
            prev = it;
            ++it;
            if ( it != m_clustersIndex.end() ) {
                prev.value() = it.value() ;
                //qDebug() << "  key now"<< prev.key() << ": " << prev.value() ;
            }
        }
        m_clustersIndex.erase(--it); //erase the last element in map

        qDebug() << "Finished. " << "\n"
                 << "  m_clustersIndex now" <<m_clustersIndex << "\n"
                 << "  Compute distances "
                    "between the new cluster and the old ones";

        //
        //Step 3. Compute distances (or similarities) between
        //        the single new cluster and the old clusters
        //
        int j = mergedClusterIndex ;

        qDebug() << "j = mergedClusterIndex " << mergedClusterIndex +1;

        for (int i = 0 ; i< clustersLeft; i ++ ) {
            if (i == deletedClusterIndex  ) {
                //                qDebug() << "Graph::graphClusteringHierarchical() -"
                //                          <<"SKIP this as it is one of the merged clusters.";
                continue;

            }

            switch (method) {
            case Clustering::Single_Linkage: //"single-linkage":
                if (i==j) {
                    distanceNewCluster = 0;
                }
                else {
                    distanceNewCluster= (DSM.item(i,imin) < DSM.item(i,jmin) ) ? DSM.item(i,imin) : DSM.item(i,jmin);
                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                        << "  DSM("<<i+1<<","<<imin+1<<") ="<< DSM.item(i,imin)
                        << "  DSM("<<i+1<<","<<jmin+1<<") ="<< DSM.item(i,jmin)
                        << " ? minimum DSM("<<i+1<<","<<j+1<<" ="<<distanceNewCluster;
                break;

            case Clustering::Complete_Linkage: // "complete-linkage":
                if (i==j) {
                    distanceNewCluster = 0;
                }
                else {
                    distanceNewCluster= (DSM.item(i,imin) > DSM.item(i,jmin) ) ? DSM.item(i,imin) : DSM.item(i,jmin);

                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                        << "  DSM("<<i+1<<","<<imin+1<<") ="<< DSM.item(i,imin)
                        << "  DSM("<<i+1<<","<<jmin+1<<") ="<< DSM.item(i,jmin)
                        << " ? maximum DSM("<<i+1<<","<<j+1<<" ="<<distanceNewCluster;
                break;

            case Clustering::Average_Linkage: //mean or "average-linkage" or UPGMA
                if (i==j) {
                    distanceNewCluster = 0;
                }
                else {
                    distanceNewCluster= ( DSM.item(i,imin)  + DSM.item(i,jmin) ) / 2;
                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                        << "  DSM("<<i+1<<","<<imin+1<<") ="<< DSM.item(i,imin)
                        << "  DSM("<<i+1<<","<<jmin+1<<") ="<< DSM.item(i,jmin)
                        << " ? average DSM("<<i+1<<","<<j+1<<" ="<<distanceNewCluster;
                break;

            default:
                distanceNewCluster= (DSM.item(i,imin) < DSM.item(i,jmin) ) ? DSM.item(i,imin) : DSM.item(i,jmin);
                break;
            }

            DSM.setItem(i, j, distanceNewCluster);
            DSM.setItem(j, i, distanceNewCluster);

            // DSM.setItem(deletedClusterIndex, j, RAND_MAX);
            // DSM.setItem(j, deletedClusterIndex, RAND_MAX);


        }


        qDebug() << "Graph::graphClusteringHierarchical() - Finished."
                 << "Resizing old DSM matrix";
        //DSM.printMatrixConsole();
        DSM.deleteRowColumn(deletedClusterIndex);

        clustersLeft --;
        seq ++;

        //
        //Step 4. Repeat steps 2 and 3 until all remaining items/clusters
        //        are clustered into a single cluster of size N
        //

    }   // end while clustersLeft

    clusteredItems.clear();
    m_clustersIndex.clear();

    qDebug()<< "m_clustersByName" <<m_clustersByName;

    emit signalProgressBoxKill();

    return true;
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
                                               const bool &considerWeights) {

    Q_UNUSED(considerWeights);

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Examining pair-wise similarity of actors...")) );

    Matrix SCM;
    if (matrix == "Adjacency") {
        createMatrixAdjacency();
        createMatrixSimilarityMatching(AM, SCM, measure, varLocation, diagonal, considerWeights);
    }
    else if (matrix == "Distances") {
        graphDistancesGeodesic();
        createMatrixSimilarityMatching(DM, SCM, measure, varLocation, diagonal, considerWeights);
    }
    else {
        return;
    }

    emit statusMessage ( tr("Writing similarity coefficients to file: ")
                         + fileName );

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << tr("SIMILARITY MATRIX: MATCHING COEFFICIENTS (SMMC)") << "\n\n";

    outText << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Network name: ")<< Qt::reset << getName()<< "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Input matrix: ")<< Qt::reset << matrix << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Variables in: ") << Qt::reset << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)  << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Matching measure: ") << Qt::reset ;


    outText << graphMetricTypeToString(measure);

    outText << "\n"
            << qSetPadChar('.') << qSetFieldWidth(20) << Qt::left
            << tr("Diagonal: \t") << Qt::reset << ((diagonal) ? "Included" : "Not included") << "\n\n";

    outText << tr("Analysis results") << "\n\n";
    if (measure==METRIC_HAMMING_DISTANCE)
        outText << tr("SMMC range: 0 < C") << "\n\n";
    else
        outText << tr("SMMC range: 0 < C < 1") << "\n\n";

    outText << SCM;

    outText << "\n";

    if (measure==METRIC_HAMMING_DISTANCE) {
        outText << tr("SMMC = 0, when two actors are absolutely similar (no tie/distance differences).")<<"\n";
        outText << tr(
                       "SMMC > 0, when two actors have some differences in their ties/distances, \n"
          "i.e. SMMC = 3 means the two actors have 3 differences in their tie/distance profiles to other actors.");
    }
    else {
        outText << tr("SMMC = 0, when there is no tie profile similarity at all.")<<"\n";
        outText << tr(
                       "SMMC > 0, when two actors have some matches in their ties/distances, \n"
          "i.e. SMMC = 1 means the two actors have their ties to other actors exactly the same all the time.");
    }



    outText << "\n\n";

    outText << tr("Similarity Matrix by Matching Measure Report,\n");
    outText << tr("Created by SocNetV ") << VERSION << ": "
            << actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

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
                                       const bool &considerWeights) {

    qDebug()<< "Graph::writeMatrixDissimilarities()"
            << "metric" << metricStr
            << "varLocation" << varLocation
            << "diagonal"<<diagonal;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open file for writing. Abort.";
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    Matrix DSM;
    int N = vertices();

    createMatrixAdjacency();

    emit statusMessage ( (tr("Examining pair-wise tie profile dissimilarities of actors...")) );

    int metric = graphMetricStrToType( metricStr );
    createMatrixDissimilarities(AM,DSM, metric, varLocation,diagonal, considerWeights);

    emit statusMessage ( tr("Writing tie profile dissimilarities to file: ")
                         + fileName );

    outText.setRealNumberPrecision(m_reportsRealPrecision);


    outText << htmlHead;

    outText << "<h1>";
    outText << tr("DISSIMILARITIES MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            <<"</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Metric: ")
            << "</span>"
            <<  metricStr
             << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            <<"</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Range: ")
            <<"</span>";

    if (metric==METRIC_JACCARD_INDEX)
        outText << tr("0 &lt; C &lt; 1") ;
    else
        outText << tr("0 &lt; C ") ;
    outText << "</p>";

    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Analysis results ")
            <<"</span>"
            << "</p>";

    //DSM.printHTMLTable(outText);
    writeMatrixHTMLTable(outText,DSM, true);
    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("DSM = 0 ")
            <<"</span>"
           << tr("when two actors have no tie profile dissimilarities. The actors have the same ties to all others.")
           <<"<br/>"
          << "<span class=\"info\">"
          << tr("DSM &gt; 0 ")
          <<"</span>"
         << tr("when the two actors have differences in their ties to other actors.");
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Dissimilarity Matrix Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

}



/**
 * @brief Calls Matrix:distancesMatrix to compute the dissimilarities matrix DSM
 * of the variables (rows, columns, both) in given input matrix using the
 * user defined metric
 * @param INPUT_MATRIX
 * @param DSM
 * @param metric
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 */
void Graph::createMatrixDissimilarities(Matrix &INPUT_MATRIX,
                                             Matrix &DSM,
                                             const int &metric,
                                             const QString &varLocation,
                                             const bool &diagonal,
                                             const bool &considerWeights){
    qDebug()<<"Graph::createMatrixDissimilarities() -metric" << metric;

    DSM = INPUT_MATRIX.distancesMatrix(metric, varLocation, diagonal, considerWeights);

    //    qDebug()<<"Graph::createMatrixDissimilarities() - matrix DSM:";
    //DSM.printMatrixConsole(true);
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
                                          const bool &considerWeights) {

    qDebug () << "Writing similarity matrix to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    int measureInt = graphMetricStrToType( measure );

    Q_UNUSED(considerWeights);

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Examining pair-wise similarity of actors...")) );

    Matrix SCM;
    int N = vertices();

    if (matrix == "Adjacency") {
        createMatrixAdjacency();
        createMatrixSimilarityMatching(AM, SCM, measureInt ,
                                            varLocation, diagonal, considerWeights);
    }
    else if (matrix == "Distances") {
        graphDistancesGeodesic();
        createMatrixSimilarityMatching(DM, SCM, measureInt,
                                            varLocation, diagonal, considerWeights);
    }
    else {
        return;
    }

    QString pMsg = tr("Writing Similarity coefficients to file. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(1, pMsg);

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("SIMILARITY MATRIX: MATCHING COEFFICIENTS (SMMC)");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Input matrix: ")
            <<"</span>"
            << matrix
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            <<"</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Matching measure: ")
            << "</span>"
            <<  measure
             << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            <<"</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("SMMC range: ")
            <<"</span>";

    if (measureInt==METRIC_HAMMING_DISTANCE)
        outText << tr("0 &lt; C") ;
    else
        outText << tr("0 &lt; C &lt; 1") ;
    outText << "</p>";

    outText << "<p>"
            << "<br />"
            << "<span class=\"info\">"
            << tr("Analysis results ")
            <<"</span>"
            << "</p>";

    emit signalProgressBoxUpdate(0);
    //SCM.printHTMLTable(outText);
    writeMatrixHTMLTable(outText,SCM, true);

    outText << "<p class=\"description\">";
    if (measureInt==METRIC_HAMMING_DISTANCE) {
        outText << "<span class=\"info\">"
                << tr("SMMC = 0 ")
                <<"</span>"
                << tr("when two actors are absolutely similar (no tie/distance differences).")
                <<"<br/>"
                << "<span class=\"info\">"
                << tr("SMMC &gt; 0 ")
                <<"</span>"
                << tr("when two actors have some differences in their ties/distances, "
                      "i.e. SMMC = 3 means the two actors have 3 differences in their tie/distance profiles to other actors.");
    }
    else {
        outText << "<span class=\"info\">"
                << tr("SMMC = 0 ")
                <<"</span>"
                << tr("when there is no tie profile similarity at all.")
                <<"<br/>"
                << "<span class=\"info\">"
                << tr("SMMC &gt; 0 ")
                <<"</span>"
                << tr("when two actors have some matches in their ties/distances, "
                      "i.e. SMMC = 1 means the two actors have their ties to other actors exactly the same all the time.");
    }
    outText << "</p>";

    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Similarity Matrix by Matching Measure Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();

    emit signalProgressBoxUpdate(1);
    emit signalProgressBoxKill();
}



/**
 * @brief Calls Matrix:similarityMatrix to compute the similarity matrix SCM
 * of the variables (rows, columns, both) in given input matrix using the
 * selected matching measure.
 *
 * @param AM
 * @param SCM
 * @param rows
 */
void Graph::createMatrixSimilarityMatching (Matrix &AM,
                                                 Matrix &SCM,
                                                 const int &measure,
                                                 const QString &varLocation,
                                                 const bool &diagonal,
                                                 const bool &considerWeights){
    qDebug()<<"Graph::createMatrixSimilarityMatching()";

    QString pMsg = tr ("Computing Similarity coefficients matrix. \nPlease wait...");
    emit signalProgressBoxCreate(1, pMsg);
    SCM.similarityMatrix(AM, measure, varLocation, diagonal, considerWeights);
    emit signalProgressBoxUpdate(1);
    emit signalProgressBoxKill();
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
                                         const bool &diagonal) {

    qDebug () << "Writing Pearson Correlation coefficients to file:" << fileName;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating Pearson Correlations...")) );

    Matrix PCC;
    int N = vertices();

    if (matrix == "Adjacency") {
        createMatrixAdjacency();
        createMatrixSimilarityPearson(AM, PCC, varLocation,diagonal);
    }
    else if (matrix == "Distances") {
        graphDistancesGeodesic();
        createMatrixSimilarityPearson(DM, PCC, varLocation,diagonal);
    }
    else {
        return;
    }


    emit statusMessage ( tr("Writing Pearson coefficients to file: ")
                         + fileName );

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("PEARSON CORRELATION COEFFICIENTS (PCC) MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Input matrix: ")
            <<"</span>"
            << matrix
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Variables in: ")
            <<"</span>"
            << ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Diagonal: ")
            <<"</span>"
            << ((diagonal) ? "Included" : "Not included")
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("PCC range: ")
            <<"</span>"
            << "-1 &lt; C &lt; 1"
            << "</p>";

    outText << "<p>"
            << "<span class=\"info\">"
            << "<br />"
            << tr("Analysis results ")
            <<"</span>"
            << "</p>";


    //PCC.printHTMLTable(outText);
    writeMatrixHTMLTable(outText,PCC, true);

    outText << "<p class=\"description\">";
    outText << "<span class=\"info\">"
            << tr("PCC = 0 ")
            <<"</span>"
            << tr("when there is no correlation at all.")
            <<"<br/>"
           << "<span class=\"info\">"
            << tr("PCC &gt; 0 ")
            <<"</span>"
            << tr("when there is positive correlation, "
                  "i.e. +1 means actors with same patterns of ties/distances.")
            <<"<br />"
           << "<span class=\"info\">"
            << tr("PCC &lt; 0 ")
            <<"</span>"
            << tr("when there is negative correlation, "
                  "i.e. -1 for actors with exactly opposite patterns of ties.")
            <<"<br/>";
    outText << "</p>";



    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Pearson Correlation Coefficients Report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
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
    qDebug () << "Writing Pearson Correlation coefficients (plain text) to file:" << fileName;

    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating Pearson Correlations...")) );

    Matrix PCC;
    if (matrix == "Adjacency") {
        createMatrixAdjacency();
        createMatrixSimilarityPearson(AM, PCC, varLocation,diagonal);
    }
    else if (matrix == "Distances") {
        graphDistancesGeodesic();
        createMatrixSimilarityPearson(DM, PCC, varLocation,diagonal);
    }
    else {
        return;
    }


    emit statusMessage ( tr("Writing Pearson coefficients to file: ")
                         + fileName );

    outText.setRealNumberPrecision(m_reportsRealPrecision);

    outText << tr("PEARSON CORRELATION COEFFICIENTS (PCC) MATRIX") << "\n\n";

    outText << tr("Network name: ")<< getName()<< "\n"
            << tr("Input matrix: ")<< matrix << "\n"
            << tr("Variables in: ")<< ((varLocation != "Rows" && varLocation != "Columns") ? "Concatenated rows + columns " : varLocation)
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
    outText <<"\n\n" ;
    outText << tr("Pearson Correlation Coefficients Report,\n");
    outText << tr("Created by SocNetV ") << VERSION << ": "
            << actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

    file.close();
}


/**
 * @brief
 * The Pearson product-moment correlation coefficient (PPMCC, PCC or Pearson's r)
 * is a measure of the linear dependence between two variables X and Y.
 *
 * As a normalized version of the covariance, the PPMCC is computed with the formula:
 *  r =\frac{\sum ^n _{i=1}(x_i - \bar{x})(y_i - \bar{y})}{\sqrt{\sum ^n _{i=1}(x_i - \bar{x})^2} \sqrt{\sum ^n _{i=1}(y_i - \bar{y})^2}}
 *
 * It gives a value between +1 and −1 inclusive, where 1 is total positive linear
 * correlation, 0 is no linear correlation, and −1 is total negative linear correlation.
 *
 * In SNA, Pearson correlations can be used to track the similarity between actors,
 * in terms of structural equivalence.
 *
 * This method creates an actor by actor NxN matrix PCC where the (i,j) element
 * is the Pearson correlation coefficient of actor i and actor j.
 * If the input matrix is the adjacency matrix, the PCC of two nodes measures
 * how related (similar, inverse or not related at all) their patterns of ties tend to be.
 * A positive value means there is strong linear association of the two actors,
 * while a negative value means the inverse. For instance a value of -1 means
 * the two actors have exactly opposite ties to other actors, while a value of 1
 * means the actors have identical patterns of ties to other actors
 * (they are connected to the same actors).
 *
 * The correlation measure of similarity is particularly useful when the data on ties are valued

 * @param AM
 * @param PCC
 * @param rows
 */
void Graph::createMatrixSimilarityPearson (Matrix &AM,
                                                Matrix &PCC,
                                                const QString &varLocation,
                                                const bool &diagonal){
    qDebug()<<"Graph::createMatrixSimilarityPearson()";


    PCC.pearsonCorrelationCoefficients(AM, varLocation,diagonal);

    qDebug()<<"Graph::createMatrixSimilarityPearson() - matrix PCC";
    //PCC.printMatrixConsole(true);

}



/**
    Returns the number of triples of vertex v1
    A triple Υ at a vertex v is a path of length two for which v is the center vertex.
*/
qreal Graph::numberOfTriples(int v1){
    qreal totalDegree=0;
    if (isSymmetric()){
        totalDegree=vertexEdgesOutbound(v1);
        return totalDegree * (totalDegree -1.0) / 2.0;
    }
    totalDegree=vertexEdgesOutbound(v1) + vertexEdgesInbound(v1);  //FIXEM
    return	totalDegree * (totalDegree -1.0);
}




/**
 * @brief  Returns the local clustering coefficient (CLUCOF) of a vertex v1
 * @param v1
 * @return
 */
qreal Graph::clusteringCoefficientLocal(const int &v1){
    if ( !isModified() && (m_graph[ vpos [v1] ]->hasCLC() ) )  {
        qreal clucof=m_graph[ vpos [v1] ]->CLC();
        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") - "
                 << " Not modified. Returning previous clucof = " << clucof;
        return clucof;
    }

    qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") - "
            << " Graph changed or clucof not calculated.";

    bool isSymmetric = false;

    if ( this->isSymmetric() ) {
        isSymmetric = true;
    }
    else {
        isSymmetric = false;
    }

    qreal clucof=0, denom = 0 , nom = 0;
    int u1 = 0 , u2 = 0, k = 0;

    H_StrToBool neighborhoodEdges;
    neighborhoodEdges.clear();

    qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") - vertex " << v1
             << "[" << vpos[v1] << "] "
             << " Checking adjacent edges " ;

    QHash<int,qreal> reciprocalEdges ;
    reciprocalEdges = m_graph [ vpos[v1] ]->reciprocalEdgesHash();

    QHash<int,qreal>::const_iterator it1;
    QHash<int,qreal>::const_iterator it2;

    it1=reciprocalEdges.cbegin();

    while ( it1 != reciprocalEdges.cend() )
    {
        u1 = it1.key();

        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                 << v1
                 << "<->"
                 << u1
                 << "[" << vpos[u1] << "] exists"
                 << "weight " << it1.value();

        if ( v1 == u1 ) {
            qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                     << "v1 == u1 - CONTINUE";
            ++it1;
            continue;
        }

        it2=reciprocalEdges.cbegin();
        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                 << "Checking if neighbor" << u1
                 << "is connected to other neighbors of" << v1;

        while ( it2 != reciprocalEdges.cend() ){

            u2 = it2.key();

            qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                     << "Other neighbor" << u2
                     << "Check if there is an edge"
                     << u1
                     << "[" << vpos[u1] << "]"
                        << "->" << u2 ;

            if ( u1 == u2 ) {
                qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                         << "u1 == u2 - CONTINUE";
                ++it2;
                continue;
            }


            if ( edgeExists( u1, u2 ) != 0 )
            {
                qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                         << "Connected neighbors: "
                         << u1 << "->" << u2;

                QString edge = QString::number(u1) + "->" + QString::number(u2);
                QString revedge = QString::number(u2) + "->" + QString::number(u1);

                if ( isSymmetric ) {
                    if ( ! neighborhoodEdges.contains(edge) &&
                         ! neighborhoodEdges.contains(revedge)  )    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                                 << "Edge added to neighborhoodEdges : " << edge;

                    }
                    else {
                        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                                 << "Edge not added, discovered previously : " << edge;
                    }

                }
                else {
                    if ( ! neighborhoodEdges.contains(edge) ) {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                                 << "Edge added to neighborhoodEdges : " << edge;
                    }
                    else {
                        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                                 << "Edge not added, discovered previously : " << edge;
                    }

                }

            }

            ++it2;
        }
        ++it1;
    }

    nom=neighborhoodEdges.size();

    qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
            << "neighborhoodEdges.size() =" <<  nom;

    if ( nom == 0)
        return 0;	//stop if we're at a leaf.

    if ( isSymmetric ){
        k=reciprocalEdges.size();  //k_{i} is the number of neighbours of a vertex
        denom =	k * (k -1.0) / 2.0;

        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
                    << "Symmetric graph. "
                    << "Max edges in neighborhood" << denom ;

    }
    else {
        // fixme : normally we should have a special method
        // to compute the number of vertices k_i = |N_i|, in the neighborhood N_i
        k=reciprocalEdges.size();
        denom = k * (k -1.0);

        qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") - "
                    << "Not symmetric graph. "
                    << "Max edges in neighborhood" << denom ;
    }

    clucof = nom / denom;

    qDebug() << "Graph::clusteringCoefficientLocal("<< v1 << ") -"
             << "CLUCOF = "<< clucof;

    m_graph[ vpos [v1] ]->setCLC(clucof);

    reciprocalEdges.clear();
    neighborhoodEdges.clear();
    return clucof;
}


/**
 * @brief Computes local clustering coefficients and returns
 * the network average Clustering Coefficient
 * @param updateProgress
 * @return
 */
qreal Graph::clusteringCoefficient (const bool updateProgress){
    qDebug()<< "Graph::clusteringCoefficient()";
    averageCLC=0;
    varianceCLC=0;
    maxCLC=0; minCLC=1;
    qreal temp=0;
    qreal x=0;
    qreal N = vertices();
    int progressCounter = 0;
    VList::const_iterator vertex;

    QString pMsg = tr("Computing Clustering Coefficient. \n"
                      "Please wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N,pMsg);

    for ( vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex) {

        if (updateProgress) {
            emit signalProgressBoxUpdate(++progressCounter);
        }

        temp = clusteringCoefficientLocal( (*vertex)->number() );

        if (temp > maxCLC)  {
            maxCLC = temp;
            maxNodeCLC = (*vertex)->number();
        }
        if ( temp < minCLC ) {
            minNodeCLC = (*vertex)->number();
            minCLC= temp;
        }
        averageCLC += temp;

    }

    averageCLC = averageCLC / N ;

    qDebug() << "Graph::clusteringCoefficient() network average " << averageCLC;

    for ( vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex) {
        x = (  (*vertex)->CLC()  -  averageCLC  ) ;
        x *=x;
        varianceCLC  += x;

    }

    varianceIC  /=  N;

    if (updateProgress) {
        emit signalProgressBoxKill();
    }

    return averageCLC;
}




/**
 * @brief Graph::graphTriadCensus
 *  Conducts a triad census and updates QList::triadTypeFreqs,
 * 		which is the list carrying all triad type frequencies
 *  Complexity:O(n!)
 * @return
 */
bool Graph::graphTriadCensus(){
    int mut=0, asy=0, nul =0;
    int temp_mut=0, temp_asy=0, temp_nul =0, counter_021=0;
    int ver1, ver2, ver3;
    int N = vertices();
    int progressCounter = 0;

    VList::const_iterator v1;
    VList::const_iterator v2;
    VList::const_iterator v3;

    qDebug() << "Graph::graphTriadCensus()";
    /*
     * QList::triadTypeFreqs stores triad type frequencies with the following order:
     * 0	1	2	3		4	5	6	7	8		9	10	11	12		13	14	15
     * 003 012 102	021D 021U 021C 111D	111U 030T 030C 201 	120D 120U 120C 210 300
    */

    for (int i = 0; i <= 15; ++i) {
        triadTypeFreqs.append(0);
        qDebug() << " initializing triadTypeFreqs[" << i << "] = "<< triadTypeFreqs[i];
    }


    QString pMsg = tr("Computing Triad Census. \nPlease wait...") ;
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); v1++) {

        emit signalProgressBoxUpdate( ++progressCounter );

        for (v2=(v1+1); v2!=m_graph.cend(); v2++) {

            ver1=(*v1)->number();
            ver2=(*v2)->number();

            temp_mut=0, temp_asy=0, temp_nul =0;

            if ( (*v1)->hasEdgeTo( ver2 ) ) {
                if ( (*v2)->hasEdgeTo( ver1 ) )
                    temp_mut++;
                else
                    temp_asy++;
            }
            else if ( (*v2)->hasEdgeTo( ver1 )  )
                temp_asy++;
            else
                temp_nul++;

            for (v3=(v2+1); v3!=m_graph.cend(); v3++){

                mut = temp_mut ;
                asy = temp_asy ;
                nul = temp_nul ;

                ver3=(*v3)->number();

                if ( (*v1)->hasEdgeTo( ver3 ) ) {
                    if ( (*v3)->hasEdgeTo( ver1 ) )
                        mut++;
                    else
                        asy++;
                }
                else if ( (*v3)->hasEdgeTo( ver1 )  )
                    asy++;
                else
                    nul++;

                if ( (*v2)->hasEdgeTo( ver3 ) ) {
                    if ( (*v3)->hasEdgeTo( ver2 ) )
                        mut++;
                    else
                        asy++;
                }
                else if ( (*v3)->hasEdgeTo( ver2 )  )
                    asy++;
                else
                    nul++;

                qDebug()<< "triad of ("<< ver1 << ","<< ver2 << ","<< ver3
                        << ") = ("	<<mut<<","<< asy<<","<<nul<<")";
                triadType_examine_MAN_label(mut, asy, nul, (*v1), (*v2),  (*v3) ) ;

                if ( mut==3 && asy==0 && nul==0 ){
                    counter_021++;
                }
            } // end 3rd for

        }// end 2rd for

    }// end 1rd for
    qDebug() << " ****** 003 COUNTER: "<< counter_021;

    calculatedTriad=true;

    emit signalProgressBoxKill();

    return true;
}




/** 
    Examines the triad type (in Mutual-Asymmetric-Null label format)
    and increases by one the proper frequency element
    inside QList::triadTypeFreqs
*/
void Graph::triadType_examine_MAN_label(int mut, int asy, int nul,
                                        GraphVertex* vert1,
                                        GraphVertex* vert2,
                                        GraphVertex* vert3
                                        ) 	{
    VList m_triad;
    bool isDown=false, isUp=false, isCycle=false, isTrans=false;
    bool isOutLinked=false, isInLinked=false;

    qDebug () << "Graph::triadType_examine_MAN_label() "
        << " adding ("<< vert1->number() << ","<< vert2->number()
        << ","<< vert3->number() << ") to m_triad ";

    m_triad<<vert1<<vert2<<vert3;

    switch (mut){
    case 0:
        switch (asy){
        case 0:	//"003";
            triadTypeFreqs[0] ++;
            break;
        case 1:	 //"012";
            triadTypeFreqs[1] ++;
            break;
        case 2:
            // "021?" - find out!
            //	qDebug() << "triad vertices: ( "<< vert1->number() << ", "<< vert2->number()<< ", "<< vert3->number()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (GraphVertex *source, m_triad)  {
                //qDebug() << "  vertex " << source->number() ;
                isOutLinked=false; isInLinked=false;

                foreach (GraphVertex *target, m_triad)  	{
                    if ( source->number() == target->number() )
                        continue;

                    if ( source->hasEdgeTo(target->number()) ){
                        if ( isOutLinked ){
                            triadTypeFreqs[3] ++;//"021D"
                            break;
                        }
                        else if (isInLinked){
                            triadTypeFreqs[5] ++;//"021C"
                            break;
                        }
                        else{
                            isOutLinked=true;
                        }
                    }
                    else if( target->hasEdgeTo(source->number()) ){
                        //	qDebug() << "    vertex " << source->number()  << " is IN linked from " <<target->number();
                        if ( isInLinked ){
                            triadTypeFreqs[4] ++;//"021U"
                            break;
                        }
                        else if (isOutLinked){
                            triadTypeFreqs[5] ++;//"021C"
                            break;
                        }
                        else{
                            isInLinked=true;
                        }
                    }
                }
            }
            break;
        case 3:
            qDebug() << "triad vertices: ( "<< vert1->number() << ", "<< vert2->number()<< ", "<< vert3->number()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            isTrans=false;
            foreach (GraphVertex *source, m_triad)  {
                qDebug() << "  vertex " << source->number() ;

                isOutLinked=false;

                foreach (GraphVertex *target, m_triad)  	{
                    if ( source->number() == target->number() )
                        continue;

                    if ( source->hasEdgeTo(target->number()) ){

                        if ( isOutLinked ){
                            triadTypeFreqs[8] ++;//"030T"
                            isTrans=true;
                            break;
                        }
                        else{
                            isOutLinked=true;
                        }
                    }
                }
            }
            if ( ! isTrans ) {//"030C"
                triadTypeFreqs[9] ++;
            }
            break;
        }
        break;

    case 1:
        switch (asy){
        case 0:	//"102";
            triadTypeFreqs[2] ++;
            break;
        case 1:
            isUp=false;
            //qDebug() << "triad vertices: ( "<< vert1->number() << ", "<< vert2->number()<< ", "<< vert3->number()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (GraphVertex *source, m_triad)  {
                //	qDebug() << "  vertex " << source->number() ;

                isInLinked=false;

                foreach (GraphVertex *target, m_triad)  	{
                    if ( source->number() == target->number() )
                        continue;

                    if ( target->hasEdgeTo(source->number()) ){

                        if ( isInLinked ){
                            triadTypeFreqs[6] ++;//"030T"
                            isUp=true;
                            break;
                        }
                        else{
                            isInLinked=true;
                        }
                    }
                }
            }
            if ( ! isUp ) {//"111U"
                triadTypeFreqs[7] ++;
            }
            break;
        case 2:
            isDown=false; isUp=false; isCycle=true;
            qDebug() << "triad vertices: ( "<< vert1->number() << ", "
                     << vert2->number()<< ", "<< vert3->number()<< " ) = ("
                     <<mut<<","<< asy<<","<<nul<<")";

            foreach (GraphVertex *source, m_triad)  {
                //qDebug() << "  vertex " << source->number() ;
                isOutLinked=false; isInLinked=false;

                foreach (GraphVertex *target, m_triad)  	{
                    if ( source->number() == target->number() )
                        continue;

                    if ( source->hasEdgeTo(target->number()) ){
                        if (target->hasEdgeTo(source->number() ) ){
                            isInLinked=true;
                            isOutLinked=true;
                            continue;
                        }
                        else if ( isOutLinked && !isInLinked ){
                            triadTypeFreqs[11] ++;//"120D"
                            isDown=true;
                            isCycle=false;
                            break;
                        }
                        else{
                            isOutLinked=true;
                        }
                    }
                    else if( target->hasEdgeTo(source->number()) ){
                        //	qDebug() << "    vertex " << source->number()  << " is IN linked from " <<target->number();
                        if (source->hasEdgeTo(target->number())){
                            isOutLinked=true;
                            isInLinked=true;
                            continue;
                        }
                        else if ( isInLinked && !isOutLinked ){
                            triadTypeFreqs[12] ++;//"120U"
                            isUp=true;
                            isCycle=false;
                            break;
                        }
                        else{
                            isInLinked=true;
                        }
                    }
                }
                if (isUp || isDown)
                    break;
            }
            if ( isCycle ) {	//"120C"
                triadTypeFreqs[13] ++;
            }
            break;
        case 3:
            // nothing here!
            break;
        }

        break;
    case 2:
        switch (asy){
        case 0:	// "201"
            triadTypeFreqs[10] ++;
            break;
        case 1:	// "210"
            triadTypeFreqs[14] ++;
            break;
        }
        break;
    case 3:	// "300"
        if (asy==0 && nul==0)
            triadTypeFreqs[15] ++;
        break;
    }


}





/** 
    Calculates and returns x! factorial...
    used in (n 2)p edges calculation
*/
int Graph:: factorial(int x) {
    int tmp;
    if(x <= 1) return 1;
    tmp = x * factorial(x - 1);
    return tmp;
}



/**
 * @brief Returns the name of the current graph
 *
 * If graph name is empty, then returns current relation name.
 * If no relation exists, returns "noname"
 *
 * @return QString
 */
QString Graph::getName() const {
    if (m_graphName.isEmpty() ) {
        if ( !( relationCurrentName().isEmpty()) )  {
            return relationCurrentName();
        }
        else {
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
void Graph::setName(const QString &graphName){
    qDebug()<< "Setting graph name to:" << graphName;
    m_graphName = graphName;
}




/**
 * @brief Returns the file name of the current graph, if any.
 *
 * @return QString
 */
QString Graph::getFileName() const {
    return m_fileName;

}


/**
 * @brief Sets the file name of the current graph
 *
 * @param fileName
 */
void Graph::setFileName(const QString &fileName){
    qDebug()<< "Setting graph filename to:" << fileName;
    m_fileName = fileName;
}


/**
 * @brief Returns the format of the last file opened
 *
 * @return int
  */
int Graph::getFileFormat() const {
    return m_fileFormat;
}

void Graph::setFileFormat(const int &fileFormat) {
    qDebug()<< "Setting graph file format to:" << fileFormat;
    m_fileFormat = fileFormat;
}

/**
 * @brief Returns true if the fileFormat is supported for saving
 * @param fileFormat
 * @return
 */
bool Graph::isFileFormatExportSupported(const int &fileFormat) const {
    if (m_graphFileFormatExportSupported.contains(fileFormat)) {
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
void Graph::setModStatus(const int &graphNewStatus, const bool &signalMW){

    if ( m_graphModStatus == ModStatus::NewNet && isEmpty()) {
        // New network, no vertices. Don't change status.

        qDebug()<<"This is a empty new network. Will not change status.";

        emit signalGraphModified(isDirected(),
                                 0,
                                 0,
                                 0,
                                 false);

        return;
    }

    else if ( graphNewStatus == ModStatus::NewNet ) {

        qDebug()<<"This is a new network. Setting graph as new...";

        m_graphModStatus=graphNewStatus;

        emit signalGraphModified(isDirected(),
                                 m_totalVertices,
                                 edgesEnabled(),
                                 graphDensity(),
                                 false);

        return;
    }
    else if ( graphNewStatus == ModStatus::SavedUnchanged ) {

        // this is called after loading or saving a file

        qDebug()<<"Setting graph as saved/unchanged...";

        m_graphModStatus=graphNewStatus;

        emit signalGraphSavedStatus(true);

        return;

    }
    else if ( graphNewStatus > ModStatus::MajorChanges ) {

        // This is called from any method that alters the graph structure,
        // thus all prior computations are invalidated

//        qDebug()<<"Major changes, invalidating computations, setting graph as changed...";

        m_graphModStatus=graphNewStatus;

        // Init all calculated* flags to false,
        // to force all relevant methods to recompute
        calculatedGraphReciprocity = false;
        calculatedGraphSymmetry = false;
        calculatedGraphWeighted = false;
        calculatedGraphDensity = false;
        calculatedEdges = false;
        calculatedVertices = false;
        calculatedVerticesList=false;
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
        calculatedEVC=false;
        calculatedPRP = false;

        if (signalMW) {

//            qDebug() << "signaling to MW that the graph is modified...";

            emit signalGraphModified(isDirected(),
                                     m_totalVertices,
                                     edgesEnabled(),
                                     graphDensity(),
                                     true);
            return;
        }

    }
    else if ( graphNewStatus > ModStatus::MinorOptions) {

        // this is called from Graph methods that inflict minor changes,
        // i.e. changing vertex positions, labels, etc

        if ( m_graphModStatus < ModStatus::MajorChanges) {
            //  Do not change status if current status is > MajorChanges
            m_graphModStatus = graphNewStatus;
        }
//        qDebug()<<"minor changes but needs saving...";
        emit signalGraphSavedStatus(false);
        return;
    }
    else {
        qCritical()<<"Strange. I should not reach this code...";
        m_graphModStatus=graphNewStatus;
    }




}


/**
 * @brief Returns true of graph is modified (edges/vertices added/removed)
 * @return
 */
bool Graph::isModified() const {
    if ( m_graphModStatus > ModStatus::MajorChanges ){
        qDebug()<<"Graph::isModified() - isModified: true";
        return true;
    }
    qDebug()<<"Graph::isModified() - isModified: false";
    return false;
}




/**
 * @brief Returns true if a graph has been loaded from a file.
 * @return
 */
bool Graph::isLoaded() const {
    if ( !getFileName().isEmpty() && getFileFormat() != FileType::UNRECOGNIZED ) {
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
bool Graph::isSaved() const {
    if ( m_graphModStatus == ModStatus::NewNet ) {
        qDebug() << "isSaved: true (new net)";
        return true;
    }
    else if ( m_graphModStatus == ModStatus::SavedUnchanged  ) {
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
                     const bool sm_has_labels){

    qDebug() << "Loading the file:" << fileName;

    qDebug() << "First, clearing current relations...";
    relationsClear();

    qDebug() << "Next, creating new file_parser -- we are on thread:" << this->thread();
    file_parser = new Parser();

    qDebug () << " moving parser to her own new thread...";
    file_parser->moveToThread(&file_parserThread);

    qDebug () << "file_parser thread now: " << file_parser->thread();

    qDebug () << "connecting file_parser signals...";

    connect(&file_parserThread, &QThread::finished,
            file_parser, &QObject::deleteLater);

    connect(file_parser, &Parser::signalAddNewRelation,
            this, &Graph::relationAdd);

    connect (file_parser, &Parser::signalSetRelation,
             this, &Graph::relationSet) ;

    connect (file_parser, &Parser::signalCreateNode,
             this, &Graph::vertexCreate );

    connect (file_parser, &Parser::signalCreateNodeAtPosRandom,
             this, &Graph::vertexCreateAtPosRandom);

    connect (file_parser, &Parser::signalCreateNodeAtPosRandomWithLabel,
             this, &Graph::vertexCreateAtPosRandomWithLabel);

    connect (file_parser, &Parser::signalCreateEdge,
             this, &Graph::edgeCreate);

    connect(file_parser, &Parser::signalFileLoaded,
            this, &Graph::graphFileLoaded);

    connect (file_parser, SIGNAL(removeDummyNode(int)),
            this, SLOT (vertexRemoveDummyNode(int)));

    connect (file_parser, &Parser::finished,
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
                sm_has_labels
                );


}


/**
 * @brief Graph::graphLoadedTerminateParserThreads
 * @param reason
 */
void Graph::graphLoadedTerminateParserThreads(QString reason) {
    qDebug() << "Terminating parser threads - reason " << reason
             <<" Checking if file_parserThread is running...";
    if (file_parserThread.isRunning() ) {
        qDebug() << "deleting file_parser pointer";
        delete file_parser;
        file_parser = 0;  // see why here: https://goo.gl/tQxpGA

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
void Graph::graphFileLoaded (const int &fileType,
                             const QString &fileName,
                             const QString &netName,
                             const int &totalNodes,
                             const int &totalLinks,
                             const int &edgeDirType,
                             const qint64 &elapsedTime,
                             const QString &message)
{
    if ( fileType == FileType::UNRECOGNIZED ) {
        qDebug() << "Could not load file. Signaling to MW the error message...";
        // Emit with error message.
        emit signalGraphLoaded (fileType,
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

    if ( edgeDirType == EdgeType::Directed ) {
        this->setDirected(true);
    }
    else {
        this->setDirected(false);
    }

    setFileFormat(fileType);

    setModStatus(ModStatus::SavedUnchanged);

    qDebug() << "Signaling to MW...";

    emit signalGraphLoaded (fileType,
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
                      const int &fileType ,
                      const bool &saveEdgeWeights,
                      const bool &saveZeroWeightEdges)
{
    qDebug() << "Saving current graph to file named:" << fileName;
    bool saved = false;
    switch (fileType) {
    case FileType::PAJEK : {
        saved=saveToPajekFormat(fileName, getName(), canvasWidth, canvasHeight) ;
        break;
    }
    case FileType::ADJACENCY: {
        saved=saveToAdjacencyFormat(fileName, saveEdgeWeights) ;
        break;
    }
    case FileType::GRAPHVIZ: {
        saved=saveToDotFormat(fileName);
        break;
    }
    case FileType::GRAPHML: {
        saved=saveToGraphMLFormat(fileName, saveZeroWeightEdges);
        break;
    }
    default: {
        setFileFormat(FileType::UNRECOGNIZED);
        break;
    }
    };
    if (saved) {
        setModStatus(ModStatus::SavedUnchanged);
    }
    else {
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
bool Graph::saveToPajekFormat (const QString &fileName, \
                                    QString networkName,
                                    int maxWidth, int maxHeight
                                    )
{

    qDebug () << "Saving graph to Pajek-formatted file:" << fileName;

    qreal weight=0;
    QFileInfo fileInfo (fileName);
    QString fileNameNoPath = fileInfo.fileName();

    networkName  = (networkName == "") ? getName().toHtmlEscaped(): networkName;
    networkName  = (networkName == "unnamed") ? fileNameNoPath.toHtmlEscaped().left(fileNameNoPath.lastIndexOf('.')): networkName;

    qDebug () << "networkName:" << networkName;

    maxWidth = (maxWidth == 0) ? canvasWidth:maxWidth ;
    maxHeight= (maxHeight== 0) ? canvasHeight:maxHeight;


    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return false;
    }
    QTextStream t( &f );

    t<<"*Network "<<networkName<<"\n";

    t<<"*Vertices "<< vertices()<<"\n";
    VList::const_iterator it;
    VList::const_iterator jt;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        qDebug()<<" Name x "<<  (*it)->number()  ;
        t<<(*it)->number()  <<" "<<"\""<<(*it)->label()<<"\"" ;
        t << " ic ";
        t<<  (*it)->colorToPajek();
        qDebug()<<" Coordinates x " << (*it)->x()<< " "<<maxWidth<<" y " << (*it)->y()<< " "<<maxHeight;
        t << "\t\t" <<(*it)->x()/(maxWidth)<<" \t"<<(*it)->y()/(maxHeight);
        t << "\t"<<(*it)->shape();
        t<<"\n";
    }

    t<<"*Arcs \n";
    qDebug()<< "Graph::saveToPajekFormat: Arcs";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveToPajekFormat:  it=" << (*it)->number() << ", jt=" << (*jt)->number() ;
            if  ( (weight=edgeExists ( (*it)->number(), (*jt)->number())) !=0
                  &&   ( edgeExists ((*jt)->number(), (*it)->number())) != weight
                  )
            {
                qDebug()<<"Graph::saveToPajekFormat  weight "<< weight
                       << " color "<<  (*it)->outLinkColor( (*jt)->number() ) ;
                t << (*it)->number() <<" "<<(*jt)->number()<< " "<<weight;
                //FIXME bug in outLinkColor() when we remove then add many nodes from the end
                t<< " c "<< (*it)->outLinkColor( (*jt)->number() );
                t <<"\n";
            }

        }
    }

    t<<"*Edges \n";
    qDebug() << "Graph::saveToPajekFormat: Edges";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveToPajekFormat:  it=" <<  (*it)->number() << ", jt=" <<(*jt)->number() ;
            if  ( ( weight=edgeExists((*it)->number(), (*jt)->number(), true) )!=0 )  {
                if ( (*it)->number() > (*jt)->number() )
                    continue;
                t << (*it)->number() <<" "<<(*jt)->number()<< " "<<weight;
                t << " c "<< (*it)->outLinkColor( (*jt)->number() );
                t <<"\n";
            }
        }
    }
    f.close();

    // Store the filename
    setFileName(fileName);

    // Store the file format
    setFileFormat(FileType::PAJEK);

    emit statusMessage (tr( "File %1 saved" ).arg( fileNameNoPath ));
    return true;



}


/**
 * @brief Saves the active graph to an adjacency-formatted file
 *
 * @param fileName
 *
 * @return bool
 */
bool Graph::saveToAdjacencyFormat (const QString &fileName,
                                        const bool &saveEdgeWeights){

    qDebug () << "Saving graph to adjacency-formatted file:" << fileName;

    QFile file( fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return false;
    }
    QTextStream outText( &file );

    writeMatrixAdjacencyTo(outText, saveEdgeWeights);

    file.close();

    // Store the filename
    setFileName(fileName);

    // Store the file format
    setFileFormat(FileType::ADJACENCY);

    QString fileNameNoPath=fileName.split("/").last();
    emit statusMessage (QString( tr("Adjacency matrix-formatted network saved into file %1") ).arg( fileNameNoPath ));
    return true;
}



/**
 * @brief TODO Saves the active graph to a UCINET-formatted file
 *
 * @param fileName
 *
 * @return bool
 */
bool Graph::saveToDotFormat (QString fileName){
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
bool Graph::saveToGraphMLFormat (const QString &fileName,
                                 const bool &saveZeroWeightEdges,
                                 QString networkName,
                                      int maxWidth,
                                      int maxHeight) {

    qDebug () << "Saving graph to GraphML-formatted file:" << fileName;

    qreal weight=0;
    int source=0, target=0, edgeCount=0, m_size=1, m_labelSize;
    QString m_color, m_labelColor, m_label;
    bool openToken;

    QFileInfo fileInfo (fileName);
    QString fileNameNoPath = fileInfo.fileName();

    QString saveDirPath= fileInfo.canonicalPath();

    QString iconsSubDir = fileInfo.baseName() + "_" + fileInfo.suffix() +"_images";
    QString iconsDirPath = saveDirPath + "/" + iconsSubDir;

    QDir saveDir(saveDirPath);
    qreal rel_coord_x = 0;
    qreal rel_coord_y = 0;

    // Check if there are nodes with custom icons in the network
    if ( graphHasVertexCustomIcons()) {
        qDebug () << "Custom node icons exist."
                  <<  "Creating images subdir" << iconsDirPath;
        // There are custom node icons in this net.
        // We need to save these custom icons to a folder
        // Create a subdir inside the directory where the actual network file
        // is about to be saved. All custom icons will be copied one-by-one there.
        if ( saveDir.mkpath( iconsDirPath ) ){
            qDebug () << "created icons subdir"
                      << iconsDirPath;
        }
        else {
            qDebug () << "ERROR creating subdir!";
        }
    }
    else {
        qDebug () << "No custom node icons. Nothing to do";
    }

    QString iconPath = QString();
    QString iconFileName = QString ();
    QString copyIconFileNamePath = QString();

    networkName  = (networkName == "") ? getName().toHtmlEscaped(): networkName;
    networkName  = (networkName == "unnamed") ? fileNameNoPath.toHtmlEscaped().left(fileNameNoPath.lastIndexOf('.')): networkName;
    qDebug () << "file:" << fileName.toUtf8() << "networkName"<< networkName;

    maxWidth = (maxWidth == 0) ? (int)canvasWidth:maxWidth ;
    maxHeight= (maxHeight== 0) ? (int)canvasHeight:maxHeight;

    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return false;
    }
    QTextStream outText( &f );
    QString outTextEncoding = QStringEncoder(outText.encoding()).name();

    qDebug () << "Using default codec for saving stream:"<< outTextEncoding;

    qDebug()<< " writing xml version...";
    outText << "<?xml version=\"1.0\" encoding=\"" << outTextEncoding << "\"?> \n";
    outText << " <!-- Created by SocNetV "<<  VERSION << " -->\n" ;
    outText << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" "
               "      xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance \" "
               "      xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
               "      http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
               "\n";

    qDebug()<< "writing keys...";

    outText <<	"  <key id=\"d0\" for=\"node\" attr.name=\"label\" attr.type=\"string\"> \n"
                "    <default>" "</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d1\" for=\"node\" attr.name=\"x_coordinate\" attr.type=\"double\"> \n"
                "    <default>" << "0.0" << "</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d2\" for=\"node\" attr.name=\"y_coordinate\" attr.type=\"double\"> \n"
                "    <default>" << "0.0" << "</default> \n"
                "  </key> \n";
    outText <<	"  <key id=\"d3\" for=\"node\" attr.name=\"size\" attr.type=\"double\"> \n"
                "    <default>"<< initVertexSize << "</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d4\" for=\"node\" attr.name=\"color\" attr.type=\"string\"> \n"
                "    <default>" << initVertexColor << "</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d5\" for=\"node\" attr.name=\"shape\" attr.type=\"string\"> \n"
                "    <default>" << initVertexShape << "</default> \n"
                "  </key> \n";

    // Check if there are nodes with custom icons in this network
    if ( graphHasVertexCustomIcons()) {
        // There are custom icons, so we will copy the default custom icon
        // to the subdir we created earlier
        iconPath = initVertexIconPath;
        iconFileName = QFileInfo(iconPath).fileName();
        copyIconFileNamePath = iconsDirPath + "/" + iconFileName;
        if ( ! QFile(copyIconFileNamePath).exists() ) {
            if  ( QFile::copy(iconPath, copyIconFileNamePath) )  {
                qDebug () << "default iconFile saved to:" << copyIconFileNamePath;
            }
            else {
                qDebug () << "ERROR saving default iconFile to:" << copyIconFileNamePath;
            }

        }
        else {
            qDebug () << "default iconFile already exists in:" << copyIconFileNamePath;
        }
        // And we write a new key (id 51) in our graphml for this default custom icon
        outText <<	"  <key id=\"d51\" for=\"node\" attr.name=\"custom-icon\" attr.type=\"string\"> \n"
                    "    <default>" << iconsSubDir + "/"+ iconFileName << "</default> \n"
                                                                          "  </key> \n";
    } // end check if custom icons exist

    outText <<	"  <key id=\"d6\" for=\"node\" attr.name=\"label.color\" attr.type=\"string\"> \n"
                "    <default>" << initVertexLabelColor << "</default> \n"
                "  </key> \n";
    outText <<	"  <key id=\"d7\" for=\"node\" attr.name=\"label.size\" attr.type=\"string\"> \n"
                "    <default>" << initVertexLabelSize << "</default> \n"
                "  </key> \n";
    outText <<	"  <key id=\"d8\" for=\"edge\" attr.name=\"weight\" attr.type=\"double\"> \n"
                "    <default>1.0</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d9\" for=\"edge\" attr.name=\"color\" attr.type=\"string\"> \n"
                "    <default>" << initEdgeColor << "</default> \n"
                "  </key> \n";

    outText <<	"  <key id=\"d10\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"> \n"
                "    <default>" << ""<< "</default> \n"
                "  </key> \n";

    VList::const_iterator it;
    VList::const_iterator jt;
    QString  relationName;
    int relationPrevious = relationCurrent();
    for (int i = 0; i < relations(); ++i) {
        relationName = (m_relationsList.at(i).simplified()).remove("\"");
        relationSet( i , false);
        qDebug()<< "writing graph tag. Relation:" << relationName ;

        if (isUndirected())
            outText << "  <graph id=\""
                    << (( relations()==1 ) ? networkName : relationName)
                    << "\" edgedefault=\"undirected\"> \n";
        else
            outText << "  <graph id=\""
                    << (( relations()==1) ? networkName : relationName )
                    << "\" edgedefault=\"directed\"> \n";

        qDebug()<< "writing nodes data...";
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            if ( ! (*it)->isEnabled () )
                continue;
//            qDebug() << "Node id:" <<  (*it)->number();
            outText << "    <node id=\"" << (*it)->number() << "\"> \n";
            m_color = (*it)->color();
            m_size = (*it)->size() ;
            m_labelSize=(*it)->labelSize() ;
            m_labelColor=(*it)->labelColor() ;
            m_label=(*it)->label();
            m_label = htmlEscaped(m_label);


            outText << "      <data key=\"d0\">" << m_label <<"</data>\n";

            rel_coord_x = (*it)->x()/(maxWidth);
            rel_coord_y = (*it)->y()/(maxHeight);

//            qDebug()<<"Rel coordinates: "
//                   << rel_coord_x
//                   << ","
//                   << rel_coord_y;

            outText << "      <data key=\"d1\">" << rel_coord_x <<"</data>\n";
            outText << "      <data key=\"d2\">" << rel_coord_y <<"</data>\n";

            if (  initVertexSize != m_size ) {
                outText << "      <data key=\"d3\">" << m_size  <<"</data>\n";
            }

            if (  QString::compare ( initVertexColor, m_color,  Qt::CaseInsensitive) != 0) {
                outText << "      <data key=\"d4\">" << m_color <<"</data>\n";
            }

            outText << "      <data key=\"d5\">" << (*it)->shape() <<"</data>\n";

            if ((*it)->shape() == "custom" ) {
                iconPath = (*it)->shapeIconPath();
                iconFileName = QFileInfo(iconPath).fileName();
                copyIconFileNamePath = iconsDirPath + "/" + iconFileName;
                if ( ! QFile(copyIconFileNamePath).exists() ) {
                    if  ( QFile::copy(iconPath, copyIconFileNamePath) )  {
                        qDebug () << "iconFile for node:" << (*it)->number()
                                  << "saved to:" << copyIconFileNamePath;
                    }
                    else {
                        qDebug () << "ERROR saving iconFile for" << (*it)->number()
                                  << "saved to: " << copyIconFileNamePath;
                    }

                }
                else {
                    qDebug () << "iconFile for node:" << (*it)->number()
                              << "already exists in:" << copyIconFileNamePath;
                }
                outText << "      <data key=\"d51\">" << iconsSubDir + "/"+ iconFileName <<"</data>\n";
            }

            if (  QString::compare ( initVertexLabelColor, m_labelColor,  Qt::CaseInsensitive) != 0) {
                outText << "      <data key=\"d6\">" << m_labelColor <<"</data>\n";
            }

            if (  initVertexLabelSize != m_labelSize ) {
                outText << "      <data key=\"d7\">" << m_labelSize <<"</data>\n";
            }

            outText << "    </node>\n";

        }

        qDebug() << "writing edges data...";
        edgeCount=0;
        if (isDirected()) {
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
            {
                for (jt=m_graph.cbegin(); jt!=m_graph.cend(); jt++)
                {
                    source=(*it)->number();
                    target=(*jt)->number();
                    m_label = "";

                    // Check if user opted to save zero-weight edges
                    if (saveZeroWeightEdges) {
                        weight= this->edgeExistsVirtual( source,target ) ;
                    }
                    else {
                        weight= this->edgeExists( source,target ) ;
                    }

                    if  ( ( !saveZeroWeightEdges && weight != 0 ) || (saveZeroWeightEdges && weight != RAND_MAX) )
                    {
                        ++edgeCount;
                        m_color = (*it)->outLinkColor( target );
                        m_label = edgeLabel(source, target);
                        m_label=htmlEscaped(m_label);
//                        qDebug()<< "edge no:"
//                                << edgeCount
//                                << "from n1=" << source << "to n2=" << target
//                                << "with weight" << weight
//                                << "and color" << m_color.toUtf8() ;
                        outText << "    <edge id=\""<< "e"+QString::number(edgeCount)
                                << "\" directed=\"" << "true" << "\" source=\"" << source
                                << "\" target=\"" << target << "\"";

                        openToken = true;
                        if ( weight != 0 ||  (saveZeroWeightEdges && weight != RAND_MAX) ) {
                            outText << "> \n";
                            outText << "      <data key=\"d8\">" << weight<<"</data>" <<" \n";
                            openToken=false;
                        }
                        if (  QString::compare ( initEdgeColor, m_color,  Qt::CaseInsensitive) != 0) {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d9\">" << m_color <<"</data>" <<" \n";
                            openToken=false;
                        }
                        if (  !m_label.isEmpty()) {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d10\">" << m_label<<"</data>" <<" \n";
                            openToken=false;
                        }

                        if (openToken)
                            outText << "/> \n";
                        else
                            outText << "    </edge>\n";

                    }

                }
            }
        }
        else {
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
            {
                for (jt=it; jt!=m_graph.end(); jt++)
                {
                    source=(*it)->number();
                    target=(*jt)->number();
                    m_label = "";

                    // Check if user opted to save zero-weight edges
                    if (saveZeroWeightEdges) {
                        weight= this->edgeExistsVirtual( source,target ) ;
                    }
                    else {
                        weight= this->edgeExists( source,target ) ;
                    }

                    if  ( ( !saveZeroWeightEdges && weight != 0 ) || (saveZeroWeightEdges && weight != RAND_MAX) )
                    {
                        ++edgeCount;
                        m_color = (*it)->outLinkColor( target );
                        m_label = edgeLabel(source, target);
                        m_label=htmlEscaped(m_label);
//                        qDebug()<< "edge no"
//                                << edgeCount
//                                << "from n1=" << source << "to n2=" << target
//                                << "with weight" << weight
//                                << "and color" << m_color.toUtf8() ;
                        outText << "    <edge id=\""<< "e"+QString::number(edgeCount)
                                << "\" directed=\"" << "false" << "\" source=\"" << source
                                << "\" target=\"" << target << "\"";

                        openToken = true;
                        if ( weight !=0  || (saveZeroWeightEdges && weight != RAND_MAX)  ) {
                            outText << "> \n";
                            outText << "      <data key=\"d8\">" << weight<<"</data>" <<" \n";
                            openToken=false;
                        }
                        if (  QString::compare ( initEdgeColor, m_color,  Qt::CaseInsensitive) != 0) {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d9\">" << m_color <<"</data>" <<" \n";
                            openToken=false;
                        }
                        if (  !m_label.isEmpty()) {
                            if (openToken)
                                outText << "> \n";
                            outText << "      <data key=\"d10\">" << m_label<<"</data>" <<" \n";
                            openToken=false;
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

    emit statusMessage( tr( "File %1 saved" ).arg( fileNameNoPath ) );

    return true;
}


/**
 * @brief Sets the directory where reports are saved
 * This is used when exporting prominence distribution images to be used in
 * HTML reports.
 * @param dir
 */
void Graph::setReportsDataDir(const QString &dir) {
    m_reportsDataDir = dir;

}


/**
 * @brief Sets the precision (number of fraction digits) the app will use
 * when writing real numbers in reports.
 * @param precision
 */
void Graph::setReportsRealNumberPrecision(const int &precision) {
    m_reportsRealPrecision = precision;
}


/**
 * @brief Sets the length of labels in reports
 * @param length
 */
void Graph::setReportsLabelLength(const int &length){
    m_reportsLabelLength = length;
}


/**
 * @brief Sets the chart type in reports
 * @param type
 */
void Graph::setReportsChartType(const int &type){
    qDebug()<<"Graph::setReportsChartType() - type:" << type;
    if ( type == -1 ) {
        m_reportsChartType = ChartType::None;
    }
    else if ( type == 0 ) {
        m_reportsChartType = ChartType::Spline;
    }
    else if ( type == 1 ) {
        m_reportsChartType = ChartType::Area;
    }
    else if ( type == 2 ) {
        m_reportsChartType = ChartType::Bars;
    }
}


/**
 * @brief Writes a "famous" dataset to the given file
 * Datasets are hardcoded! They are exported in the given fileName...
 *
 * TODO: Move all these datasets to a separate class
 *
 * @param fileName
 */
void Graph::writeDataSetToFile (const QString dir, const QString fileName) {

    qDebug() << "Writing famous dataset to file:" << dir+fileName;

    QFile file( dir+fileName );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fileName;
        emit statusMessage ( tr("Error. Could not write to ") + fileName );
        return;
    }
    QTextStream outText( &file );

    QString datasetDescription=QString();
    qDebug()<< "		... writing dataset ";
    if ( fileName == "Campnet.paj") {
        qDebug()<< "		... to  " << fileName;
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
        outText << "*Network Campnet" << "\n" <<
                   "*Vertices 18" << "\n" <<
                   "1 \"HOLLY\" ic RGBF1F5D5		0.63046 	0.575472	circle" << "\n" <<
                   "2 \"BRAZEY\" ic RGBF1F5D5		0.0991736 	0.511006	circle" << "\n" <<
                   "3 \"CAROL\" ic RGBF1F5D5		0.576151 	0.43239		circle" << "\n" <<
                   "4 \"PAM\" ic RGBF1F5D5		0.726092 	0.371069	circle" << "\n" <<
                   "5 \"PAT\" ic RGBF1F5D5		0.709563 	0.5		circle" << "\n" <<
                   "6 \"JENNIE\" ic RGBF1F5D5		0.876033 	0.482704	circle" << "\n" <<
                   "7 \"PAULINE\" ic RGBF1F5D5	0.619835 	0.286164	circle" << "\n" <<
                   "8 \"ANN\" ic RGBF1F5D5		0.864227 	0.309748	circle" << "\n" <<
                   "9 \"MICHAEL\" ic RGBF1F5D5	0.489965 	0.638365	box" << "\n" <<
                   "10 \"BILL\" ic RGBF1F5D5		0.475797 	0.805031	box" << "\n" <<
                   "11 \"LEE\" ic RGBF1F5D5		0.0885478 	0.267296	box" << "\n" <<
                   "12 \"DON\" ic RGBF1F5D5		0.645809 	0.778302	box" << "\n" <<
                   "13 \"JOHN\" ic RGBF1F5D5		0.453365 	0.290881	box" << "\n" <<
                   "14 \"HARRY\" ic RGBF1F5D5		0.593861 	0.669811	box" << "\n" <<
                   "15 \"GERY\" ic RGBF1F5D5		0.362456 	0.539308	box" << "\n" <<
                   "16 \"STEVE\" ic RGBF1F5D5		0.230224 	0.5		box" << "\n" <<
                   "17 \"BERT\" ic RGBF1F5D5		0.218418 	0.245283	box" << "\n" <<
                   "18 \"RUSS\" ic RGBF1F5D5		0.323495 	0.29717		box" << "\n" <<
                   "*Arcs " << "\n" <<
                   "1 4 1 c black" << "\n" <<
                   "2 16 1 c black" << "\n" <<
                   "2 17 1 c black" << "\n" <<
                   "3 4 1 c black" << "\n" <<
                   "7 5 1 c black" << "\n" <<
                   "8 7 1 c black" << "\n" <<
                   "9 1 1 c black" << "\n" <<
                   "10 9 1 c black" << "\n" <<
                   "10 12 1 c black" << "\n" <<
                   "10 14 1 c black" << "\n" <<
                   "13 7 1 c black" << "\n" <<
                   "13 15 1 c black" << "\n" <<
                   "13 18 1 c black" << "\n" <<
                   "14 1 1 c black" << "\n" <<
                   "15 9 1 c black" << "\n" <<
                   "15 16 1 c black" << "\n" <<
                   "*Edges " << "\n" <<
                   "1 4 1 c black" << "\n" <<
                   "1 5 1 c black" << "\n" <<
                   "1 12 1 c black" << "\n" <<
                   "2 11 1 c black" << "\n" <<
                   "2 16 1 c black" << "\n" <<
                   "2 17 1 c black" << "\n" <<
                   "3 4 1 c black" << "\n" <<
                   "3 5 1 c black" << "\n" <<
                   "3 7 1 c black" << "\n" <<
                   "4 6 1 c black" << "\n" <<
                   "4 7 1 c black" << "\n" <<
                   "4 8 1 c black" << "\n" <<
                   "5 6 1 c black" << "\n" <<
                   "6 8 1 c black" << "\n" <<
                   "9 12 1 c black" << "\n" <<
                   "9 14 1 c black" << "\n" <<
                   "10 12 1 c black" << "\n" <<
                   "10 14 1 c black" << "\n" <<
                   "11 16 1 c black" << "\n" <<
                   "11 17 1 c black" << "\n" <<
                   "12 14 1 c black" << "\n" <<
                   "13 15 1 c black" << "\n" <<
                   "13 18 1 c black" << "\n" <<
                   "15 16 1 c black" << "\n" <<
                   "15 18 1 c black" << "\n" <<
                   "16 17 1 c black" << "\n" <<
                   "16 18 1 c black" << "\n" <<
                   "17 18 1 c black";
    }
    if ( fileName == "Herschel_Graph.paj") {
        qDebug()<< "		... to  " << fileName;
        datasetDescription = tr("Herschel graph \n\n"
                                "The Herschel graph is the smallest nonhamiltonian "
                                "polyhedral graph. \n"
                                "It is the unique such graph on 11 nodes, "
                                "and has 18 edges.");
        outText << "*Network Herschel_Graph" << "\n" <<
                   "*Vertices 11" << "\n" <<
                   "1 \"1\" ic red	0.48225  0.411308 circle" << "\n" <<
                   "2 \"2\" ic red	0.652297 0.591389 circle" << "\n" <<
                   "3 \"3\" ic red	0.479571 0.762504 circle"<< "\n" <<
                   "4 \"4\" ic red	0.849224 0.41395 circle"<< "\n" <<
                   "5 \"5\" ic red  0.48196  0.06	circle"<< "\n" <<
                   "6 \"6\" ic red	0.148625 0.413208 circle"<< "\n" <<
                   "7 \"7\" ic red	0.654193 0.198133 circle"<< "\n" <<
                   "8 \"8\" ic red	0.268771 0.593206 circle"<< "\n" <<
                   "9 \"9\" ic red	0.272785 0.19606	circle"<< "\n" <<
                   "10 \"10\" ic red 0.834746 0.0533333 circle"<< "\n" <<
                   "11 \"11\" ic red 0.134137 0.761837 circle"<< "\n" <<
                   "*Arcs "<< "\n" <<
                   "*Edges "<< "\n" <<
                   "1 3 1 c #616161"<< "\n" <<
                   "1 4 1 c #616161"<< "\n" <<
                   "1 5 1 c #616161"<< "\n" <<
                   "1 6 1 c #616161"<< "\n" <<
                   "2 3 1 c #616161"<< "\n" <<
                   "2 4 1 c #616161"<< "\n" <<
                   "2 7 1 c #616161"<< "\n" <<
                   "2 8 1 c #616161"<< "\n" <<
                   "3 11 1 c #616161"<< "\n" <<
                   "4 10 1 c #616161"<< "\n" <<
                   "5 9 1 c #616161"<< "\n" <<
                   "5 10 1 c #616161"<< "\n" <<
                   "6 9 1 c #616161"<< "\n" <<
                   "6 11 1 c #616161"<< "\n" <<
                   "7 9 1 c #616161"<< "\n" <<
                   "7 10 1 c #616161"<< "\n" <<
                   "8 9 1 c #616161"<< "\n" <<
                   "8 11 1 c #616161";
    }
    else if ( fileName == "Krackhardt_High-tech_managers.paj" ) {
        qDebug()<< "		... to  " << fileName;
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
        outText << "*Network  Krackhardt's High-tech managers"<< "\n" <<
                   "*Vertices      21"<< "\n" <<
                   "1 \"v1\"       0.6226    0.7207" << "\n" <<
                   "2 \"v2\"       0.6000    0.5533" << "\n" <<
                   "3 \"v3\"       0.6722    0.3928" << "\n" <<
                   "4 \"v4\"       0.7646    0.6000" << "\n" <<
                   "5 \"v5\"       0.3518    0.4775" << "\n" <<
                   "6 \"v6\"       0.7583    0.0784" << "\n" <<
                   "7 \"v7\"       0.6692    0.2475" << "\n" <<
                   "8 \"v8\"       0.7349    0.5030" << "\n" <<
                   "9 \"v9\"       0.5325    0.3892" << "\n" <<
                   "10 \"v10\"      0.5846    0.6311" << "\n" <<
                   "11 \"v11\"      0.4600    0.4733" << "\n" <<
                   "12 \"v12\"      0.8855    0.2566" << "\n" <<
                   "13 \"v13\"      0.1145    0.4786" << "\n" <<
                   "14 \"v14\"      0.3838    0.3270" << "\n" <<
                   "15 \"v15\"      0.5349    0.4455" << "\n" <<
                   "16 \"v16\"      0.6117    0.9216" << "\n" <<
                   "17 \"v17\"      0.7041    0.4144" << "\n" <<
                   "18 \"v18\"      0.4864    0.5808" << "\n" <<
                   "19 \"v19\"      0.5728    0.4802" << "\n" <<
                   "20 \"v20\"      0.6640    0.5041" << "\n" <<
                   "21 \"v21\"      0.7846    0.3329" << "\n" <<
                   "*Matrix :1 gives_advice_to"<< "\n" <<
                   "0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0 0 1" << "\n" <<
                   "0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "1 1 0 1 0 1 1 1 1 1 1 1 0 1 0 0 1 1 0 1 1" << "\n" <<
                   "1 1 0 0 0 1 0 1 0 1 1 1 0 0 0 1 1 1 0 1 1" << "\n" <<
                   "1 1 0 0 0 1 1 1 0 1 1 0 1 1 0 1 1 1 1 1 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "0 1 0 0 0 1 0 0 0 0 1 1 0 1 0 0 1 1 0 0 1" << "\n" <<
                   "0 1 0 1 0 1 1 0 0 1 1 0 0 0 0 0 0 1 0 0 1" << "\n" <<
                   "1 1 0 0 0 1 1 1 0 1 1 1 0 1 0 1 1 1 0 0 1" << "\n" <<
                   "1 1 1 1 1 0 0 1 0 0 1 0 1 0 1 1 1 1 1 1 0" << "\n" <<
                   "1 1 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "1 1 0 0 1 0 0 0 1 0 0 0 0 1 0 0 0 1 0 0 0" << "\n" <<
                   "0 1 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 0 0 1" << "\n" <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1" << "\n" <<
                   "1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                   "1 1 0 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "1 1 1 1 1 0 1 1 1 1 1 0 1 1 1 1 0 0 1 1 1" << "\n" <<
                   "1 1 1 0 1 0 1 0 0 1 1 0 0 1 1 0 0 1 0 1 0" << "\n" <<
                   "1 1 0 0 0 1 0 1 0 0 1 1 0 1 1 1 1 1 0 0 1" << "\n" <<
                   "0 1 1 1 0 1 1 1 0 0 0 1 0 1 0 0 1 1 0 1 0"<< "\n" <<
                   "*Matrix :2 is_friend_of" <<"\n" <<
                   "0 1 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 0 0" << "\n" <<
                   "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0" << "\n" <<
                   "1 1 0 0 0 0 0 1 0 0 0 1 0 0 0 1 1 0 0 0 0" << "\n" <<
                   "0 1 0 0 0 0 0 0 1 0 1 0 0 1 0 0 1 0 1 0 1" << "\n" <<
                   "0 1 0 0 0 0 1 0 1 0 0 1 0 0 0 0 1 0 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 1 0 1 0 0 1 1 0 0 1 0 0 0 1 0 0 0 1 0" << "\n" <<
                   "1 1 1 1 1 0 0 1 1 0 0 1 1 0 1 0 1 1 1 0 0" << "\n" <<
                   "1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1" << "\n" <<
                   "0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0" << "\n" <<
                   "1 0 1 0 1 1 0 0 1 0 1 0 0 1 0 0 0 0 1 0 0" << "\n" <<
                   "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 0 0 1 1 1" << "\n" <<
                   "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "1 1 1 0 1 0 0 0 0 0 1 1 0 1 1 0 0 0 0 1 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                   "0 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 1 0 0 0" << "\n" <<
                   "*Matrix :3 reports_to" <<"\n" <<
                   "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
    }


    else if (fileName == "Padgett_Florentine_Families.paj"){
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
                                "duality of persons and their algebras. Social Networks, 8, 215-256. "
                             "");
        outText<< "*Network Padgett's Florentine Families" << "\n" <<
                  "*Vertices      16" << "\n" <<
                  "1 \"Acciaiuoli\"         0.2024    0.1006" << "\n" <<
                  "2 \"Albizzi\"            0.3882    0.4754" << "\n" <<
                  "3 \"Barbadori\"          0.1633    0.7413" << "\n" <<
                  "4 \"Bischeri\"           0.6521    0.5605" << "\n" <<
                  "5 \"Castellani\"         0.6178    0.9114" << "\n" <<
                  "6 \"Ginori\"             0.3018    0.5976" << "\n" <<
                  "7 \"Guadagni\"           0.5219    0.5006" << "\n" <<
                  "8 \"Lamberteschi\"       0.4533    0.6299" << "\n" <<
                  "9 \"Medici\"             0.2876    0.3521" << "\n" <<
                  "10 \"Pazzi\"              0.0793    0.2587" << "\n" <<
                  "11 \"Peruzzi\"            0.6509    0.7365" << "\n" <<
                  "12 \"Pucci\"              0.4083    0.1186" << "\n" <<
                  "13 \"Ridolfi\"            0.6308    0.2060" << "\n" <<
                  "14 \"Salviati\"           0.0734    0.4455" << "\n" <<
                  "15 \"Strozzi\"            0.8639    0.5832" << "\n" <<
                  "16 \"Tornabuoni\"         0.5633    0.3713" << "\n" <<
                  "*Matrix 1: \"Marital\""<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 1 1 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 1 0 0 0 1 0 0 0 1 0"<< "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 1 0 0 0 1 0"<< "\n" <<
                  "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1"<< "\n" <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "1 1 1 0 0 0 0 0 0 0 0 0 1 1 0 1"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0"<< "\n" <<
                  "0 0 0 1 1 0 0 0 0 0 0 0 0 0 1 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 1 1"<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 1 1 0 0 0 0 0 1 0 1 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 1 0 1 0 0 0 1 0 0 0"<< "\n" <<
                  "*Matrix 2: \"Business\""<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 1 1 0 0 1 0 1 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 0"<< "\n" <<
                  "0 0 1 0 0 0 0 1 0 0 1 0 0 0 0 0"<< "\n" <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 1 1 0 1 0 0 0 1 0 0 0 0 0"<< "\n" <<
                  "0 0 1 0 0 1 0 0 0 1 0 0 0 1 0 1"<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 1 1 1 0 0 1 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<< "\n" <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0";

    }
    else if (fileName == "Zachary_Karate_Club.dl"){
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
        outText<< "DL"<< "\n" <<
                  "N=34 NM=2"<< "\n" <<
                  "FORMAT = FULLMATRIX DIAGONAL PRESENT"<< "\n" <<
                  "LEVEL LABELS:"<< "\n" <<
                  "ZACHE"<< "\n" <<
                  "ZACHC"<< "\n" <<
                  "DATA:"<< "\n" <<
                  "0 1 1 1 1 1 1 1 1 0 1 1 1 1 0 0 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 1 0 0" << "\n" <<
                  "1 0 1 1 0 0 0 1 0 0 0 0 0 1 0 0 0 1 0 1 0 1 0 0 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                  "1 1 0 1 0 0 0 1 1 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 0" << "\n" <<
                  "1 1 1 0 0 0 0 1 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 0 0 0 1 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "1 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 0 0 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 0 0 1 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 1 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 1 0 0 0 0 0 1 1" << "\n" <<
                  "0 1 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << "\n" <<
                  "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 1 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 1 1 0 0 1 0 1 0 1 1 0 0 0 0 0 1 1 1 0 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 1 1 0 0 0 1 1 1 0 0 1 1 1 0 1 1 0 0 1 1 1 1 1 1 1 0" << "\n" <<
                  "0 4 5 3 3 3 3 2 2 0 2 3 1 3 0 0 0 2 0 2 0 2 0 0 0 0 0 0 0 0 0 2 0 0" << "\n" <<
                  "4 0 6 3 0 0 0 4 0 0 0 0 0 5 0 0 0 1 0 2 0 2 0 0 0 0 0 0 0 0 2 0 0 0" << "\n" <<
                  "5 6 0 3 0 0 0 4 5 1 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 2 2 0 0 0 2 0" << "\n" <<
                  "3 3 3 0 0 0 0 3 0 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "3 0 0 0 0 0 2 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "3 0 0 0 0 0 5 0 0 0 3 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "3 0 0 0 2 5 0 0 0 0 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "2 4 4 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "2 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 0 3 4" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2" << "\n" <<
                  "2 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "1 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "3 5 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 2" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 4" << "\n" <<
                  "0 0 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 2" << "\n" <<
                  "2 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 1" << "\n" <<
                  "2 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 3" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 0 4 0 3 0 0 5 4" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 3 0 0 0 2 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 2 0 0 0 0 0 0 7 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4 0 0 0 2" << "\n" <<
                  "0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4 3 0 0 0 0 0 0 0 0 4" << "\n" <<
                  "0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 2" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 0 0 4 0 0 0 0 0 4 2" << "\n" <<
                  "0 2 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 3" << "\n" <<
                  "2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 7 0 0 2 0 0 0 4 4" << "\n" <<
                  "0 0 2 0 0 0 0 0 3 0 0 0 0 0 3 3 0 0 1 0 3 0 2 5 0 0 0 0 0 4 3 4 0 5" << "\n" <<
                  "0 0 0 0 0 0 0 0 4 2 0 0 0 3 2 4 0 0 2 1 1 0 3 4 0 0 2 4 2 2 3 4 5 0";
    }
    else if (fileName == "Galaskiewicz_CEOs_and_clubs_affiliation_network_data.2sm"){
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
        outText<< "0 0 1 1 0 0 0 0 1 0 0 0 0 0 0" << "\n" <<
                  "0 0 1 0 1 0 1 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 1 0 0 0" << "\n" <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 1 1 0" << "\n" <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 0 1 0" << "\n" <<
                  "0 0 1 1 0 0 0 0 0 1 1 0 0 0 0" << "\n" <<
                  "0 0 0 1 0 0 1 0 0 1 0 0 0 0 0" << "\n" <<
                  "1 0 0 1 0 0 0 1 0 1 0 0 0 0 0" << "\n" <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 0" << "\n" <<
                  "0 1 1 0 0 0 0 0 1 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 1 0 0 1 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 1 1 1 0 0 0 1 0 0 0 0 0 0" << "\n" <<
                  "0 1 1 1 0 0 0 0 0 0 1 1 1 0 1" << "\n" <<
                  "0 1 1 0 0 1 0 0 0 0 0 0 1 0 1" << "\n" <<
                  "0 1 1 0 0 1 0 1 0 0 0 0 0 1 0" << "\n" <<
                  "0 1 1 0 1 0 0 0 0 0 1 1 0 0 1" << "\n" <<
                  "0 0 0 1 0 0 0 0 1 0 0 1 1 0 1" << "\n" <<
                  "1 0 1 1 0 0 1 0 1 0 0 0 0 0 0" << "\n" <<
                  "0 1 1 1 0 0 0 0 0 0 1 0 0 0 1" << "\n" <<
                  "0 0 1 1 0 0 0 1 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 1 0 0 0 0 1 0 0 0 0 0 0 1" << "\n" <<
                  "0 1 1 0 0 1 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "1 0 1 1 0 1 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 1 0 0" << "\n" <<
                  "0 1 1 0 0 0 0 0 0 0 0 1 0 0 0";
    }
    else if (fileName== "Thurman_Office_Networks_Coalitions.dl" ) {
        datasetDescription=tr("Thurman's Office Networks and Coalitions\n\n"
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
        outText << "DL"<<"\n"<<
                   "N=15 NM=2"<<"\n"<<
                   "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<"\n"<<
                   "ROW LABELS:"<<"\n"<<
                   "ANN"<<"\n"<<
                   "AMY"<<"\n"<<
                   "KATY"<<"\n"<<
                   "BILL"<<"\n"<<
                   "PETE"<<"\n"<<
                   "TINA"<<"\n"<<
                   "ANDY"<<"\n"<<
                   "LISA"<<"\n"<<
                   "PRESIDENT"<<"\n"<<
                   "MINNA"<<"\n"<<
                   "MARY"<<"\n"<<
                   "EMMA"<<"\n"<<
                   "ROSE"<<"\n"<<
                   "MIKE"<<"\n"<<
                   "PEG"<<"\n"<<
                   "COLUMN LABELS:"<<"\n"<<
                   "ANN"<<"\n"<<
                   "AMY"<<"\n"<<
                   "KATY"<<"\n"<<
                   "BILL"<<"\n"<<
                   "PETE"<<"\n"<<
                   "TINA"<<"\n"<<
                   "ANDY"<<"\n"<<
                   "LISA"<<"\n"<<
                   "PRESIDENT"<<"\n"<<
                   "MINNA"<<"\n"<<
                   "MARY"<<"\n"<<
                   "EMMA"<<"\n"<<
                   "ROSE"<<"\n"<<
                   "MIKE"<<"\n"<<
                   "PEG"<<"\n"<<
                   "LEVEL LABELS:"<<"\n"<<
                   "THURA"<<"\n"<<
                   "THURM"<<"\n"<<
                   "DATA:"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 1 1 1 1 0 1 1 1 0 1 1 1 1 1 1"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1"<<"\n"<<
                   " 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 1 0 0 0 0 1 0 1 1 1"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 1 1 0 1 1 0 1 1 0 1 0 1 0 0"<<"\n"<<
                   " 1 0 1 1 1 1 0 1 0 0 0 0 0 0 0"<<"\n"<<
                   " 1 1 0 0 1 1 0 1 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 1 0 0 0 0 1 0 0 1 0 0 0 0 0"<<"\n"<<
                   " 1 1 1 0 0 1 1 1 1 0 0 1 0 0 0"<<"\n"<<
                   " 1 1 1 0 1 0 0 1 0 0 0 0 0 0 0"<<"\n"<<
                   " 0 0 0 1 1 0 0 0 0 1 0 0 0 0 0"<<"\n"<<
                   " 1 1 1 0 1 1 0 0 1 0 0 1 0 0 0"<<"\n"<<
                   " 1 0 0 0 1 0 0 1 0 0 0 1 0 0 0"<<"\n"<<
                   " 0 0 0 1 0 0 1 0 0 0 0 1 0 0 0"<<"\n"<<
                   " 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0"<<"\n"<<
                   " 0 0 0 0 1 0 0 1 1 1 1 0 1 1 1"<<"\n"<<
                   " 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0";
    }
    else if (fileName == "Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl" ) {
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
        outText << "DL"<<"\n"<<
                   "N=16"<<"\n"<<
                   "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<"\n"<<
                   "ROW LABELS:"<<"\n"<<
                   "ABN"<<"\n"<<
                   "AMRO"<<"\n"<<
                   "ENNIA"<<"\n"<<
                   "NS"<<"\n"<<
                   "BUHRT"<<"\n"<<
                   "AGO"<<"\n"<<
                   "AKZO"<<"\n"<<
                   "NB"<<"\n"<<
                   "SHV"<<"\n"<<
                   "FGH"<<"\n"<<
                   "HEINK"<<"\n"<<
                   "PHLPS"<<"\n"<<
                   "NATND"<<"\n"<<
                   "OGEM"<<"\n"<<
                   "RSV"<<"\n"<<
                   "NSU"<<"\n"<<
                   "COLUMN LABELS:"<<"\n"<<
                   "ABN"<<"\n"<<
                   "AMRO"<<"\n"<<
                   "ENNIA"<<"\n"<<
                   "NS"<<"\n"<<
                   "BUHRT"<<"\n"<<
                   "AGO"<<"\n"<<
                   "AKZO"<<"\n"<<
                   "NB"<<"\n"<<
                   "SHV"<<"\n"<<
                   "FGH"<<"\n"<<
                   "HEINK"<<"\n"<<
                   "PHLPS"<<"\n"<<
                   "NATND"<<"\n"<<
                   "OGEM"<<"\n"<<
                   "RSV"<<"\n"<<
                   "NSU"<<"\n"<<
                   "DATA:"<<"\n"<<
                   " 0 0 0 1 2 1 2 1 1 1 2 1 4 0 0 0"<<"\n"<<
                   " 0 0 3 2 1 2 1 2 2 0 3 1 2 1 2 0"<<"\n"<<
                   " 0 3 0 3 1 0 1 0 1 0 0 0 0 1 1 0"<<"\n"<<
                   " 1 2 3 0 0 0 1 1 2 0 0 0 1 0 2 0"<<"\n"<<
                   " 2 1 1 0 0 0 1 0 0 1 0 0 0 1 0 0"<<"\n"<<
                   " 1 2 0 0 0 0 0 2 1 0 1 1 0 0 0 0"<<"\n"<<
                   " 2 1 1 1 1 0 0 1 2 1 0 1 1 0 2 0"<<"\n"<<
                   " 1 2 0 1 0 2 1 0 1 0 1 1 1 0 0 0"<<"\n"<<
                   " 1 2 1 2 0 1 2 1 0 0 0 0 1 0 1 0"<<"\n"<<
                   " 1 0 0 0 1 0 1 0 0 0 0 1 0 1 0 0"<<"\n"<<
                   " 2 3 0 0 0 1 0 1 0 0 0 1 0 1 1 0"<<"\n"<<
                   " 1 1 0 0 0 1 1 1 0 1 1 0 1 0 1 0"<<"\n"<<
                   " 4 2 0 1 0 0 1 1 1 0 0 1 0 0 0 0"<<"\n"<<
                   " 0 1 1 0 1 0 0 0 0 1 1 0 0 0 1 0"<<"\n"<<
                   " 0 2 1 2 0 0 2 0 1 0 1 1 0 1 0 0"<<"\n"<<
                   " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";

    }

    else if (fileName == "Stokman_Ziegler_Corporate_Interlocks_West_Germany.dl" ) {
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
        outText <<"DL"<<"\n"<<
                  "N=15"<<"\n"<<
                  "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<"\n"<<
                  "ROW LABELS:"<<"\n"<<
                  "VAG"<<"\n"<<
                  "DEUBK"<<"\n"<<
                  "ALINZ"<<"\n"<<
                  "SIEMN"<<"\n"<<
                  "RUHRK"<<"\n"<<
                  "DIMLR"<<"\n"<<
                  "HAPAG"<<"\n"<<
                  "KRUPP"<<"\n"<<
                  "RWE"<<"\n"<<
                  "KREDT"<<"\n"<<
                  "THYSN"<<"\n"<<
                  "MANES"<<"\n"<<
                  "DRESB"<<"\n"<<
                  "KARST"<<"\n"<<
                  "VEBA"<<"\n"<<
                  "COLUMN LABELS:"<<"\n"<<
                  "VAG"<<"\n"<<
                  "DEUBK"<<"\n"<<
                  "ALINZ"<<"\n"<<
                  "SIEMN"<<"\n"<<
                  "RUHRK"<<"\n"<<
                  "DIMLR"<<"\n"<<
                  "HAPAG"<<"\n"<<
                  "KRUPP"<<"\n"<<
                  "RWE"<<"\n"<<
                  "KREDT"<<"\n"<<
                  "THYSN"<<"\n"<<
                  "MANES"<<"\n"<<
                  "DRESB"<<"\n"<<
                  "KARST"<<"\n"<<
                  "VEBA"<<"\n"<<
                  "DATA:"<<"\n"<<
                  " 0 2 1 0 2 0 0 2 2 2 2 1 1 1 0"<<"\n"<<
                  " 2 0 3 3 1 4 2 0 2 1 1 2 0 2 0"<<"\n"<<
                  " 1 3 0 6 1 2 2 1 2 0 2 2 1 1 0"<<"\n"<<
                  " 0 3 6 0 2 2 1 0 0 0 4 3 1 0 0"<<"\n"<<
                  " 2 1 1 2 0 1 1 2 1 1 2 1 1 0 0"<<"\n"<<
                  " 0 4 2 2 1 0 1 2 2 0 2 0 1 0 0"<<"\n"<<
                  " 0 2 2 1 1 1 0 1 1 0 1 0 2 1 0"<<"\n"<<
                  " 2 0 1 0 2 2 1 0 2 1 2 0 2 0 0"<<"\n"<<
                  " 2 2 2 0 1 2 1 2 0 3 3 0 1 1 0"<<"\n"<<
                  " 2 1 0 0 1 0 0 1 3 0 3 1 0 1 0"<<"\n"<<
                  " 2 1 2 4 2 2 1 2 3 3 0 0 1 0 0"<<"\n"<<
                  " 1 2 2 3 1 0 0 0 0 1 0 0 0 0 0"<<"\n"<<
                  " 1 0 1 1 1 1 2 2 1 0 1 0 0 1 0"<<"\n"<<
                  " 1 2 1 0 0 0 1 0 1 1 0 0 1 0 0"<<"\n"<<
                  " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";

    }


    else if (fileName == "Bernard_Killworth_Fraternity.dl"){
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
        outText << "DL"<<"\n"<<
                   "N=58 NM=2"<<"\n"<<
                   "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<"\n"<<
                   "LEVEL LABELS:"<<"\n"<<
                   "BKFRAB"<<"\n"<<
                   "BKFRAC"<<"\n"<<
                   "DATA:"<<"\n"<<
                   "0  0  2  1  0  0  2  0  0  0  1  1  2  0  0  0  1  0  1  0  0  1  0  0  0  0"<<"\n"<<
                   "0  0  2  1  1  1  0  2  1  2  0  0  0  0  1  0  0  0  0  0  0  0  1  0  0  0"<<"\n"<<
                   "0  1  1  4  1  1"<<"\n"<<
                   "0  0 10  0  0  2  1  0  2  0  0  0  6  2  0  1  0  0  0  1  0 10  2  0  4  0"<<"\n"<<
                   "3  0  1  1  0  0  0  0  5  1  0  4  0  0  0  0  0  1  1  0  0  5  3  0  0  0"<<"\n"<<
                   "0  1  0  1  4  0"<<"\n"<<
                   "2 10  0  6 11 14 15  4 12  0  5  4  3  8 10  8 11  0  2 19  2 15  1  2  6  1"<<"\n"<<
                   "5  0 12  5  4  0  1  4 15  3  1  3  6  0  2  3  0  9  8  2  1  3  6  2  0  2"<<"\n"<<
                   "2 16  4  5 19  1"<<"\n"<<
                   "1  0  6  0  2  3  9  1  8  0  0  5  0  0  2  4  3  2  2  6  0  1  1  3  1  0"<<"\n"<<
                   "5  1  1  3  0  1  1  4  1  0  1  3  2  0  1  0  0  1  1  1  1  2  1  3  0  0"<<"\n"<<
                   "2  1  2  2  3  5"<<"\n"<<
                   "0  0 11  2  0  2  8  1  1  1  0  0  2  0  1  1  0  0  0  3  0  0  0  0  0  0"<<"\n"<<
                   "8  0  1  5  0  0  1  0  0  0  0  0  9  2  1  0  1  8 25  0  0  0  0  0  0  0"<<"\n"<<
                   "1  2  0  0  4  0"<<"\n"<<
                   "0  2 14  3  2  0 30  2  8  0  4  4  1  6  2 14  9  0  1 51  0  3  2  1  0  1"<<"\n"<<
                   "6  0  3 11  2  0 15  5  3  1  0  2  2  1  3  1  0  3  2  2  6  1  3  4  0  2"<<"\n"<<
                   "8  9  3  2 18  2"<<"\n"<<
                   "2  1 15  9  8 30  0 10  4  2  7  3  0 12  9 10  9  2  3 40  2  2  5  2  0  1"<<"\n"<<
                   "19  1 10 14  5  3 14  7  7  5  3  4  5  7  8  5  0  2  4  7  3  7  7  2  0  0"<<"\n"<<
                   "6  5 14 16 20  4"<<"\n"<<
                   "0  0  4  1  1  2 10  0  3  0  2  0  1  3  3  3  5  0  0  6  1  0  2  3  0  1"<<"\n"<<
                   "6  0  2  0  9  1  0  1  2  4  2  5  1  0  3  5  0  0  5  0  1  3  1  1  0  1"<<"\n"<<
                   "2  5  0  2  4  2"<<"\n"<<
                   "0  2 12  8  1  8  4  3  0  0  5  5  2  2  4  5  6  1  0  5  0  5  0  3  3  3"<<"\n"<<
                   "3  1  2  3  1  0  2  4  4  3  5  1  2  0  1  1  1  2  0  0  4  0  1  4  0  6"<<"\n"<<
                   "1  4  3  2  7  1"<<"\n"<<
                   "0  0  0  0  1  0  2  0  0  0  0  0  0  0  1  2  0  0  0  0  0  0  0  0  0  0"<<"\n"<<
                   "6  0  1  0  1  0  0  0  0  0  0  1  2  2  0  0  0  0  0  1  0  0  0  0  0  0"<<"\n"<<
                   "0  1  0  0  0  0"<<"\n"<<
                   "1  0  5  0  0  4  7  2  5  0  0  0  0  1  3  3  5  3  0  7  4  1  0  3  0  0"<<"\n"<<
                   "4  0  5  1  3  0  0  2  2  3  5  3  2  0  0  1  0  2  1  4  5  2  1  0  0  0"<<"\n"<<
                   "0  4  6  6 12  0"<<"\n"<<
                   "1  0  4  5  0  4  3  0  5  0  0  0  0  0  0  0  0  0  0  3  0  1  0  1  1  0"<<"\n"<<
                   "0  0  2  0  2  0  1  2  3  2  2  1  0  0  0  1  0  1  1  1  0  0  1  2  0  0"<<"\n"<<
                   "1  2  0  7  3  3"<<"\n"<<
                   "2  6  3  0  2  1  0  1  2  0  0  0  0  2  1  3  3  0  1  0  0  6  2  0  0  0"<<"\n"<<
                   "3  0  1  0  0  0  1  1  1  0  0  1  1  1  1  1  1  0  2  1  0  0  2  0  0  0"<<"\n"<<
                   "2  4  1  0  0  0"<<"\n"<<
                   "0  2  8  0  0  6 12  3  2  0  1  0  2  0  3  8 11  1  4  8  0  1  0  0  1  1"<<"\n"<<
                   "4  0  8  4  6  0  3  1  5  1  1  0  0  0  1  3  0  2  2  1  1  1  0  0  0  0"<<"\n"<<
                   "1  0  2  1  5  1"<<"\n"<<
                   "0  0 10  2  1  2  9  3  4  1  3  0  1  3  0  9 14  0  6  9  0  2  1  2  1  0"<<"\n"<<
                   "4  0  3  0  2  1  1  4  2  3  0  6  1  0  7  1  0  7  1  1  0  0  1  1  0  0"<<"\n"<<
                   "7  6  4  9  4  0"<<"\n"<<
                   "0  1  8  4  1 14 10  3  5  2  3  0  3  8  9  0 26  3  1 12  0  2  0  0  1  0"<<"\n"<<
                   "7  0  5  6  5  4  2  2  2  2  0  4  4  0  2  5  1  3  2  1  1  4  0  2  0  0"<<"\n"<<
                   "8  4  2  0 11  3"<<"\n"<<
                   "1  0 11  3  0  9  9  5  6  0  5  0  3 11 14 26  0  3  0  9  0  1  0  0  1  0"<<"\n"<<
                   "5  0  5  2  2  4  2  1  4  2  0  1  1  1  2  3  0  3  1  0  0  3  1  2  0  0"<<"\n"<<
                   "7  7  4  0 11  0"<<"\n"<<
                   "0  0  0  2  0  0  2  0  1  0  3  0  0  1  0  3  3  0  0  0  3  0  0  0  0  0"<<"\n"<<
                   "0  0  1  0  0  3  0  1  1  1  1  0  1  0  0  0  0  1  0  2  0  2  0  0  0  0"<<"\n"<<
                   "0  0  2  1  0  1"<<"\n"<<
                   "1  0  2  2  0  1  3  0  0  0  0  0  1  4  6  1  0  0  0  5  0  0  2  1  3  0"<<"\n"<<
                   "0  0  0  1  1  0  0  1  1  1  1  2  0  1 14  1  0  1  0  0  1  0  3  0  0  0"<<"\n"<<
                   "1  0  0  3  1  2"<<"\n"<<
                   "0  1 19  6  3 51 40  6  5  0  7  3  0  8  9 12  9  0  5  0  3  2  3  2  1  1"<<"\n"<<
                   "7  1 10  6  6  1 13 12  9  2  1  6  2  1 10  4  0  2  2  1  2  1  6  1  0  0"<<"\n"<<
                   "12 17 11  9 23  5"<<"\n"<<
                   "0  0  2  0  0  0  2  1  0  0  4  0  0  0  0  0  0  3  0  3  0  0  1  0  0  0"<<"\n"<<
                   "0  0  2  0  2  0  0  1  1  1  0  1  0  0  1  1  0  0  0  5  0  1  1  0  0  0"<<"\n"<<
                   "0  1  2  4  2  1"<<"\n"<<
                   "1 10 15  1  0  3  2  0  5  0  1  1  6  1  2  2  1  0  0  2  0  0  1  1  7  2"<<"\n"<<
                   "1  0  3  1  0  0  0  0  1  1  1  0  2  0  0  0  0  1  0  3  0  0  2  1  0  0"<<"\n"<<
                   "0  2  1  1  3  0"<<"\n"<<
                   "0  2  1  1  0  2  5  2  0  0  0  0  2  0  1  0  0  0  2  3  1  1  0  0  1  0"<<"\n"<<
                   "1  0  2  0  2  0  3  1  2  1  2  2  2  1  7  1  0  1  2  0  2  0 11  1  1  0"<<"\n"<<
                   "1  4  1  2  3  1"<<"\n"<<
                   "0  0  2  3  0  1  2  3  3  0  3  1  0  0  2  0  0  0  1  2  0  1  0  0  0  1"<<"\n"<<
                   "0  0  1  1  1  0  0  2  1  1  0  2  0  0  0  0  0  1  0  1  0  1  0  0  0  0"<<"\n"<<
                   "0  0  0  2  1  1"<<"\n"<<
                   "0  4  6  1  0  0  0  0  3  0  0  1  0  1  1  1  1  0  3  1  0  7  1  0  0  0"<<"\n"<<
                   "0  0  3  1  0  0  0  0  3  0  1  1  0  0  4  0  0  1  0  0  0  0  0  0  0  0"<<"\n"<<
                   "2  1  1  1  5  0"<<"\n"<<
                   "0  0  1  0  0  1  1  1  3  0  0  0  0  1  0  0  0  0  0  1  0  2  0  1  0  0"<<"\n"<<
                   "1  0  0  1  0  0  0  0  1  0  0  1  3  0  0  0  0  0  1  0  0  1  2  0  0  2"<<"\n"<<
                   "0  1  1  1  2  0"<<"\n"<<
                   "0  3  5  5  8  6 19  6  3  6  4  0  3  4  4  7  5  0  0  7  0  1  1  0  0  1"<<"\n"<<
                   "0  0  6  6  2  1  1  4  0  1  0  2  4  0  3  2  1  1  4  1  0  5  2  0  0  0"<<"\n"<<
                   "1  2  2  4  6  2"<<"\n"<<
                   "0  0  0  1  0  0  1  0  1  0  0  0  0  0  0  0  0  0  0  1  0  0  0  0  0  0"<<"\n"<<
                   "0  0  0  0  0  0  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0"<<"\n"<<
                   "1  0  0  0  0  0"<<"\n"<<
                   "2  1 12  1  1  3 10  2  2  1  5  2  1  8  3  5  5  1  0 10  2  3  2  1  3  0"<<"\n"<<
                   "6  0  0  3  1  0  0  0 20  2  2  3  3  2  1  2  0  3  3  0  1  1  1  1  0  0"<<"\n"<<
                   "0  7  1  2 10  1"<<"\n"<<
                   "1  1  5  3  5 11 14  0  3  0  1  0  0  4  0  6  2  0  1  6  0  1  0  1  1  1"<<"\n"<<
                   "6  0  3  0  3  0  1  0  6  1  1  1  3  1  4  1  2  0  1  0  5  1  3  1  0  0"<<"\n"<<
                   "3  2  1  6 10  2"<<"\n"<<
                   "1  0  4  0  0  2  5  9  1  1  3  2  0  6  2  5  2  0  1  6  2  0  2  1  0  0"<<"\n"<<
                   "2  0  1  3  0  4  0  3  1  3  0  1  0  1  3  3  0  0  1  3  0  2  1  0  0  0"<<"\n"<<
                   "1  4  1  1  3  2"<<"\n"<<
                   "1  0  0  1  0  0  3  1  0  0  0  0  0  0  1  4  4  3  0  1  0  0  0  0  0  0"<<"\n"<<
                   "1  0  0  0  4  0  0  2  0  0  0  0  0  0  1  0  0  0  0  3  0  6  0  0  0  0"<<"\n"<<
                   "0  0  0  0  0  1"<<"\n"<<
                   "0  0  1  1  1 15 14  0  2  0  0  1  1  3  1  2  2  0  0 13  0  0  3  0  0  0"<<"\n"<<
                   "1  0  0  1  0  0  0  1  1  1  0  0  0  3  1  0  0  0  0  0  0  0  1  0  0  2"<<"\n"<<
                   "8  1  0  1  3  0"<<"\n"<<
                   "2  0  4  4  0  5  7  1  4  0  2  2  1  1  4  2  1  1  1 12  1  0  1  2  0  0"<<"\n"<<
                   "4  1  0  0  3  2  1  0  3  1  0  0  1  1  2  1  0  0  0  3  2  2  1  3  0  0"<<"\n"<<
                   "2  4  3  4  3  6"<<"\n"<<
                   "1  5 15  1  0  3  7  2  4  0  2  3  1  5  2  2  4  1  1  9  1  1  2  1  3  1"<<"\n"<<
                   "0  0 20  6  1  0  1  3  0  2  1  3  2  2  3  4  2  2  0  0  1  0  6  1  0  0"<<"\n"<<
                   "1 12  2  3  6  2"<<"\n"<<
                   "2  1  3  0  0  1  5  4  3  0  3  2  0  1  3  2  2  1  1  2  1  1  1  1  0  0"<<"\n"<<
                   "1  0  2  1  3  0  1  1  2  0  0  0  1  0  1  2  0  1  0  3  0  0  3  0  0  0"<<"\n"<<
                   "1  0  2 10  1  1"<<"\n"<<
                   "0  0  1  1  0  0  3  2  5  0  5  2  0  1  0  0  0  1  1  1  0  1  2  0  1  0"<<"\n"<<
                   "0  0  2  1  0  0  0  0  1  0  0  0  3  0  1  0  0  0  1  0  4  0  2  0  1  0"<<"\n"<<
                   "2  1  0  1  3  0"<<"\n"<<
                   "0  4  3  3  0  2  4  5  1  1  3  1  1  0  6  4  1  0  2  6  1  0  2  2  1  1"<<"\n"<<
                   "2  0  3  1  1  0  0  0  3  0  0  0  0  1  2  1  0  0  1  0  2  0  0  1  0  0"<<"\n"<<
                   "1  6  1  1  4  2"<<"\n"<<
                   "0  0  6  2  9  2  5  1  2  2  2  0  1  0  1  4  1  1  0  2  0  2  2  0  0  3"<<"\n"<<
                   "4  0  3  3  0  0  0  1  2  1  3  0  0  1  0  0  0  4  9  2  1  2  5  4  3  0"<<"\n"<<
                   "0  2  2  1  2  0"<<"\n"<<
                   "0  0  0  0  2  1  7  0  0  2  0  0  1  0  0  0  1  0  1  1  0  0  1  0  0  0"<<"\n"<<
                   "0  0  2  1  1  0  3  1  2  0  0  1  1  0  0  0  0  0  0  0  1  0  0  0  0  0"<<"\n"<<
                   "1  2  0  0  2  0"<<"\n"<<
                   "1  0  2  1  1  3  8  3  1  0  0  0  1  1  7  2  2  0 14 10  1  0  7  0  4  0"<<"\n"<<
                   "3  0  1  4  3  1  1  2  3  1  1  2  0  0  0  1  1  1  1  0  0  0  9  0  0  0"<<"\n"<<
                   "4  1  1  5  1  2"<<"\n"<<
                   "0  0  3  0  0  1  5  5  1  0  1  1  1  3  1  5  3  0  1  4  1  0  1  0  0  0"<<"\n"<<
                   "2  0  2  1  3  0  0  1  4  2  0  1  0  0  1  0  1  1  1  1  1  1  1  0  0  0"<<"\n"<<
                   "2  1  1  0  3  1"<<"\n"<<
                   "0  0  0  0  1  0  0  0  1  0  0  0  1  0  0  1  0  0  0  0  0  0  0  0  0  0"<<"\n"<<
                   "1  0  0  2  0  0  0  0  2  0  0  0  0  0  1  1  0  0  0  0  0  0  0  0  0  0"<<"\n"<<
                   "0  0  1  0  0  0"<<"\n"<<
                   "0  1  9  1  8  3  2  0  2  0  2  1  0  2  7  3  3  1  1  2  0  1  1  1  1  0"<<"\n"<<
                   "1  0  3  0  0  0  0  0  2  1  0  0  4  0  1  1  0  0  2  0  1  0  2  1  0  0"<<"\n"<<
                   "0  2  1  2  3  0"<<"\n"<<
                   "0  1  8  1 25  2  4  5  0  0  1  1  2  2  1  2  1  0  0  2  0  0  2  0  0  1"<<"\n"<<
                   "4  0  3  1  1  0  0  0  0  0  1  1  9  0  1  1  0  2  0  0  1  2  4  1  1  0"<<"\n"<<
                   "0  4  0  0  1  0"<<"\n"<<
                   "0  0  2  1  0  2  7  0  0  1  4  1  1  1  1  1  0  2  0  1  5  3  0  1  0  0"<<"\n"<<
                   "1  0  0  0  3  3  0  3  0  3  0  0  2  0  0  1  0  0  0  0  0  5  1  0  0  0"<<"\n"<<
                   "0  0  1  2  4  1"<<"\n"<<
                   "0  0  1  1  0  6  3  1  4  0  5  0  0  1  0  1  0  0  1  2  0  0  2  0  0  0"<<"\n"<<
                   "0  0  1  5  0  0  0  2  1  0  4  2  1  1  0  1  0  1  1  0  0  1  2  0  2  0"<<"\n"<<
                   "3  0  0  2  6  1"<<"\n"<<
                   "0  5  3  2  0  1  7  3  0  0  2  0  0  1  0  4  3  2  0  1  1  0  0  1  0  1"<<"\n"<<
                   "5  0  1  1  2  6  0  2  0  0  0  0  2  0  0  1  0  0  2  5  1  0  3  2  0  0"<<"\n"<<
                   "0  2  1  0  2  0"<<"\n"<<
                   "1  3  6  1  0  3  7  1  1  0  1  1  2  0  1  0  1  0  3  6  1  2 11  0  0  2"<<"\n"<<
                   "2  0  1  3  1  0  1  1  6  3  2  0  5  0  9  1  0  2  4  1  2  3  0  4  0  1"<<"\n"<<
                   "4  4  2  2  3  1"<<"\n"<<
                   "0  0  2  3  0  4  2  1  4  0  0  2  0  0  1  2  2  0  0  1  0  1  1  0  0  0"<<"\n"<<
                   "0  0  1  1  0  0  0  3  1  0  0  1  4  0  0  0  0  1  1  0  0  2  4  0  1  0"<<"\n"<<
                   "0  1  1  1  0  3"<<"\n"<<
                   "0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0  0  0"<<"\n"<<
                   "0  0  0  0  0  0  0  0  0  0  1  0  3  0  0  0  0  0  1  0  2  0  0  1  0  0"<<"\n"<<
                   "0  0  0  0  0  0"<<"\n"<<
                   "0  0  2  0  0  2  0  1  6  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  2"<<"\n"<<
                   "0  0  0  0  0  0  2  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0  0  0"<<"\n"<<
                   "0  0  0  0  0  0"<<"\n"<<
                   "0  0  2  2  1  8  6  2  1  0  0  1  2  1  7  8  7  0  1 12  0  0  1  0  2  0"<<"\n"<<
                   "1  1  0  3  1  0  8  2  1  1  2  1  0  1  4  2  0  0  0  0  3  0  4  0  0  0"<<"\n"<<
                   "0  5  1  2  4  3"<<"\n"<<
                   "1  1 16  1  2  9  5  5  4  1  4  2  4  0  6  4  7  0  0 17  1  2  4  0  1  1"<<"\n"<<
                   "2  0  7  2  4  0  1  4 12  0  1  6  2  2  1  1  0  2  4  0  0  2  4  1  0  0"<<"\n"<<
                   "5  0  5  3 10  0"<<"\n"<<
                   "1  0  4  2  0  3 14  0  3  0  6  0  1  2  4  2  4  2  0 11  2  1  1  0  1  1"<<"\n"<<
                   "2  0  1  1  1  0  0  3  2  2  0  1  2  0  1  1  1  1  0  1  0  1  2  1  0  0"<<"\n"<<
                   "1  5  0 12  7  1"<<"\n"<<
                   "4  1  5  2  0  2 16  2  2  0  6  7  0  1  9  0  0  1  3  9  4  1  2  2  1  1"<<"\n"<<
                   "4  0  2  6  1  0  1  4  3 10  1  1  1  0  5  0  0  2  0  2  2  0  2  1  0  0"<<"\n"<<
                   "2  3 12  0 12  0"<<"\n"<<
                   "1  4 19  3  4 18 20  4  7  0 12  3  0  5  4 11 11  0  1 23  2  3  3  1  5  2"<<"\n"<<
                   "6  0 10 10  3  0  3  3  6  1  3  4  2  2  1  3  0  3  1  4  6  2  3  0  0  0"<<"\n"<<
                   "4 10  7 12  0  1"<<"\n"<<
                   "1  0  1  5  0  2  4  2  1  0  0  3  0  1  0  3  0  1  2  5  1  0  1  1  0  0"<<"\n"<<
                   "2  0  1  2  2  1  0  6  2  1  0  2  0  0  2  1  0  0  0  1  1  0  1  3  0  0"<<"\n"<<
                   "3  0  1  0  1  0"<<"\n"<<
                   "0 4 4 5 4 4 5 5 4 5 5 4 4 5 5 4 4 5 5 5 5 4 5 4 4 5 5 4 4 5 5 5 4 4 4 5 4 5 4 4"<<"\n"<<
                   "5 4 5 5 4 5 5 5 4 5 4 5 4 4 5 5 5 5"<<"\n"<<
                   "3 0 2 3 4 2 2 2 3 2 3 3 5 4 3 3 3 2 3 3 2 5 4 2 5 3 2 2 3 4 4 2 4 2 5 3 3 5 3 2"<<"\n"<<
                   "3 3 3 2 4 2 4 3 3 3 2 3 4 4 2 3 2 2"<<"\n"<<
                   "2 2 0 4 5 4 4 1 4 3 3 3 3 3 4 4 4 2 2 5 2 5 3 1 4 3 3 2 4 5 4 2 3 3 5 1 2 3 3 1"<<"\n"<<
                   "2 4 4 3 4 2 2 2 2 2 1 3 2 5 3 2 5 2"<<"\n"<<
                   "4 4 5 0 5 5 4 3 5 4 4 5 3 4 4 4 4 3 5 5 3 4 5 5 4 4 5 4 4 4 5 4 5 5 4 3 3 4 4 3"<<"\n"<<
                   "5 4 4 4 4 3 4 4 5 4 2 4 5 5 4 4 5 5"<<"\n"<<
                   "2 3 5 5 0 3 2 3 3 4 4 1 4 2 4 2 2 1 2 2 1 3 4 1 2 5 4 1 3 1 3 1 1 1 1 1 2 2 5 4"<<"\n"<<
                   "2 2 2 5 5 1 3 2 5 2 3 2 1 2 2 2 4 1"<<"\n"<<
                   "3 2 5 5 2 0 5 3 4 3 4 3 3 5 4 5 5 2 2 5 3 2 3 1 1 1 3 1 4 4 4 1 4 3 3 2 1 2 2 2"<<"\n"<<
                   "2 3 1 2 2 3 5 3 3 3 1 2 4 4 3 3 5 4"<<"\n"<<
                   "2 1 3 4 2 5 0 2 2 2 2 2 1 5 5 5 4 3 3 5 3 2 2 2 2 2 3 1 4 3 3 2 5 4 3 4 2 2 2 2"<<"\n"<<
                   "3 2 2 3 2 2 2 2 3 3 1 2 3 3 3 4 5 3"<<"\n"<<
                   "5 3 3 3 2 2 3 0 3 3 4 3 3 3 3 3 2 3 3 3 2 2 3 5 2 2 3 2 2 3 5 4 3 5 3 4 3 3 3 2"<<"\n"<<
                   "3 2 2 3 2 3 2 3 4 3 2 2 3 2 2 3 3 4"<<"\n"<<
                   "2 2 5 4 2 3 3 2 0 1 3 5 2 3 3 3 3 1 3 4 2 4 3 3 4 5 2 1 3 3 2 2 3 3 4 3 3 2 2 2"<<"\n"<<
                   "2 2 2 2 2 2 4 2 3 2 1 5 2 4 2 3 4 4"<<"\n"<<
                   "3 3 5 4 4 4 3 3 2 0 3 4 2 4 5 4 4 1 2 4 2 4 2 2 3 2 5 1 5 2 3 1 2 3 4 1 1 2 4 5"<<"\n"<<
                   "3 3 2 1 3 4 1 3 2 4 1 2 2 5 4 1 3 3"<<"\n"<<
                   "3 3 3 3 3 3 3 3 3 3 0 3 3 3 3 3 3 3 3 3 5 3 3 3 3 3 3 3 3 3 5 5 3 3 3 3 3 3 3 3"<<"\n"<<
                   "3 3 3 3 3 5 3 5 3 3 2 3 3 3 3 3 3 3"<<"\n"<<
                   "2 3 4 4 2 3 2 2 5 3 3 0 2 3 2 3 3 2 3 4 2 4 2 2 3 3 3 1 2 2 3 1 2 4 3 4 2 2 4 2"<<"\n"<<
                   "2 2 3 2 1 3 1 2 3 2 2 2 2 4 2 4 3 4"<<"\n"<<
                   "2 5 4 2 4 3 2 2 2 2 2 2 0 3 2 3 2 2 2 3 1 5 3 2 5 2 3 2 4 3 3 2 3 3 3 3 2 4 4 5"<<"\n"<<
                   "2 4 3 4 4 2 3 2 3 3 3 2 3 4 3 3 3 2"<<"\n"<<
                   "3 2 5 4 3 5 5 2 4 3 3 3 3 0 4 5 4 3 4 4 3 3 2 2 3 3 4 2 5 3 4 3 4 3 4 3 2 3 4 3"<<"\n"<<
                   "4 4 3 4 4 3 3 3 3 3 2 2 4 4 4 4 5 3"<<"\n"<<
                   "2 1 3 3 4 4 5 2 3 2 2 1 1 3 0 4 5 1 5 2 2 2 3 2 1 1 4 1 3 2 3 3 3 4 4 3 1 4 2 1"<<"\n"<<
                   "5 3 1 5 3 2 2 1 4 1 1 1 3 3 3 5 5 5"<<"\n"<<
                   "3 3 4 4 3 5 5 2 3 2 3 3 4 4 5 0 5 3 5 5 2 3 2 2 2 1 2 2 4 4 3 2 3 3 3 3 2 3 4 3"<<"\n"<<
                   "4 5 3 3 4 4 3 4 3 3 2 1 5 4 2 4 3 3"<<"\n"<<
                   "2 2 5 3 3 4 4 2 3 3 3 3 2 4 5 5 0 2 4 5 2 3 2 2 3 1 2 3 4 3 2 2 4 2 5 2 1 4 3 2"<<"\n"<<
                   "3 5 3 5 3 2 2 3 3 3 1 1 4 5 3 4 5 3"<<"\n"<<
                   "5 3 2 5 3 3 5 5 3 2 5 5 4 4 3 5 4 0 2 1 5 3 3 3 3 2 2 1 2 5 5 5 3 5 4 4 2 2 3 3"<<"\n"<<
                   "3 3 3 5 5 5 2 5 3 5 2 1 2 3 4 5 5 5"<<"\n"<<
                   "3 3 4 4 2 2 3 2 4 3 3 3 3 4 5 5 5 2 0 4 2 4 5 4 5 2 3 3 4 4 3 3 4 4 2 4 4 3 1 2"<<"\n"<<
                   "5 4 3 3 3 2 4 2 5 3 3 2 3 3 3 4 4 3"<<"\n"<<
                   "3 2 4 4 2 5 5 2 3 3 3 3 2 4 3 4 3 2 3 0 2 3 3 1 1 1 2 2 3 3 3 2 5 3 3 3 2 2 2 2"<<"\n"<<
                   "3 2 2 3 2 2 1 2 3 3 1 1 5 5 3 2 5 3"<<"\n"<<
                   "3 1 2 2 1 2 3 1 2 2 5 2 1 2 2 1 1 4 1 3 0 1 2 2 2 1 1 1 1 2 5 4 1 2 2 2 1 1 1 1"<<"\n"<<
                   "2 1 1 3 1 5 2 5 3 4 1 1 1 2 2 2 3 2"<<"\n"<<
                   "2 5 5 3 3 3 2 2 4 2 2 3 4 3 3 2 3 2 3 4 3 0 3 3 5 3 3 1 4 3 3 3 3 3 4 3 2 3 3 3"<<"\n"<<
                   "4 3 3 3 3 4 3 3 4 3 1 3 2 4 3 3 4 3"<<"\n"<<
                   "2 3 4 4 5 3 4 3 4 2 2 3 2 4 4 2 2 2 5 3 1 2 0 2 4 3 2 1 3 1 2 1 4 5 2 1 4 4 5 2"<<"\n"<<
                   "5 2 1 5 5 1 5 1 5 1 4 2 5 2 3 2 4 5"<<"\n"<<
                   "4 1 2 5 2 2 3 5 4 3 4 3 2 2 3 4 2 2 4 2 1 3 2 0 2 4 4 1 1 2 5 2 2 5 2 4 1 1 1 1"<<"\n"<<
                   "4 1 2 3 1 4 4 2 2 5 1 3 1 1 3 4 2 5"<<"\n"<<
                   "2 4 5 3 3 2 2 2 3 1 3 4 4 2 2 1 3 1 4 2 2 5 4 2 0 3 2 1 3 4 2 1 2 3 3 2 2 2 3 3"<<"\n"<<
                   "2 3 4 2 2 1 1 2 3 2 1 2 2 2 1 3 4 2"<<"\n"<<
                   "3 3 4 4 4 2 2 2 5 2 3 3 2 2 2 2 2 2 3 1 2 4 3 3 4 0 2 2 2 2 2 2 2 3 4 2 4 2 2 2"<<"\n"<<
                   "2 4 4 3 4 2 3 4 3 3 3 5 2 3 2 2 3 2"<<"\n"<<
                   "4 2 5 5 5 3 5 3 1 5 5 3 1 4 4 2 1 2 1 5 2 4 3 4 1 1 0 1 5 4 5 3 4 5 4 3 1 2 1 2"<<"\n"<<
                   "1 4 1 2 3 1 1 4 2 4 1 1 4 1 2 5 5 3"<<"\n"<<
                   "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 0 3 3 3 3 3 3 3 3 3 3 3 3"<<"\n"<<
                   "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3"<<"\n"<<
                   "2 3 5 3 3 3 4 2 3 4 3 2 3 5 4 4 4 1 3 3 1 4 3 1 4 1 5 2 0 2 3 2 3 2 5 3 2 4 4 3"<<"\n"<<
                   "2 4 4 3 2 1 2 1 2 1 1 2 3 5 3 3 5 2"<<"\n"<<
                   "5 2 5 5 1 3 2 3 3 1 5 1 2 2 3 3 1 1 2 5 2 2 1 3 3 1 3 1 3 0 2 2 2 3 4 2 4 3 1 1"<<"\n"<<
                   "3 4 3 3 1 1 5 2 1 2 1 2 3 3 3 3 5 3"<<"\n"<<
                   "4 2 4 4 2 4 5 5 3 3 5 3 2 3 4 3 3 3 3 4 4 3 4 5 3 2 4 2 4 2 0 5 3 4 4 4 3 3 2 2"<<"\n"<<
                   "4 4 2 3 2 4 3 4 3 4 2 4 3 4 3 4 4 3"<<"\n"<<
                   "5 3 4 4 3 4 4 4 3 3 5 3 2 4 5 4 4 5 4 5 5 4 3 4 3 2 4 2 2 5 5 0 5 5 4 3 3 3 2 2"<<"\n"<<
                   "3 3 2 4 2 5 3 5 5 5 2 2 4 4 3 4 5 5"<<"\n"<<
                   "3 3 3 5 2 5 5 2 3 2 3 4 3 4 4 4 3 3 4 5 2 2 4 2 3 3 3 4 3 3 2 3 0 4 4 3 2 3 2 3"<<"\n"<<
                   "3 3 4 2 2 2 3 2 4 3 2 3 5 4 4 4 4 4"<<"\n"<<
                   "4 3 4 5 3 4 4 4 4 3 3 3 2 2 3 3 3 4 5 5 3 2 5 5 3 4 4 2 3 4 4 4 5 0 3 4 4 3 4 3"<<"\n"<<
                   "5 3 3 3 2 4 3 3 5 5 2 4 5 3 4 5 4 5"<<"\n"<<
                   "2 4 5 3 3 4 4 2 3 4 3 4 3 3 4 2 5 2 4 4 2 4 3 2 5 3 3 2 5 4 3 2 4 2 0 2 2 4 4 3"<<"\n"<<
                   "4 4 5 3 2 1 4 1 3 3 2 2 2 5 2 3 4 2"<<"\n"<<
                   "5 1 2 5 1 4 5 1 3 3 5 5 2 5 3 4 2 3 5 5 5 2 1 1 1 1 3 3 3 2 2 2 5 4 4 0 1 1 1 1"<<"\n"<<
                   "5 3 4 4 4 5 4 5 1 2 1 2 1 3 1 5 5 4"<<"\n"<<
                   "2 3 3 4 2 4 3 3 4 3 3 4 3 3 3 3 2 2 5 3 2 3 5 2 4 4 3 2 2 3 3 2 3 4 4 2 0 4 5 2"<<"\n"<<
                   "4 3 3 4 2 2 5 2 5 3 5 3 3 3 3 2 3 3"<<"\n"<<
                   "2 5 3 3 2 3 2 2 3 3 3 3 3 3 4 4 4 2 4 4 2 2 3 2 3 2 2 3 4 3 3 3 4 2 5 3 3 0 3 2"<<"\n"<<
                   "3 4 4 3 2 3 4 3 3 3 2 2 4 4 2 3 4 3"<<"\n"<<
                   "2 2 3 4 5 3 3 2 3 4 4 5 4 4 3 5 3 2 2 4 2 5 5 2 5 4 4 2 5 2 2 2 2 3 5 2 5 2 0 3"<<"\n"<<
                   "2 5 5 3 5 2 5 2 5 3 5 2 3 4 3 3 4 3"<<"\n"<<
                   "2 3 3 1 4 3 4 2 1 2 1 2 5 3 3 3 2 2 4 4 1 2 1 2 3 2 2 1 4 1 1 1 2 3 3 3 2 2 3 0"<<"\n"<<
                   "2 3 2 3 3 1 1 1 1 2 1 5 2 4 2 2 4 2"<<"\n"<<
                   "3 2 2 5 3 4 4 2 2 4 2 2 2 4 4 3 3 2 5 5 2 4 5 2 3 2 2 2 3 4 3 2 4 3 4 4 3 3 3 2"<<"\n"<<
                   "0 2 4 3 4 2 3 2 5 3 4 3 5 2 2 4 4 5"<<"\n"<<
                   "2 1 4 1 2 3 3 2 1 1 3 1 4 4 3 5 5 1 3 3 1 2 1 1 2 2 2 1 4 3 3 1 2 2 5 1 1 4 2 1"<<"\n"<<
                   "2 0 5 2 1 1 2 1 2 1 1 1 4 4 2 3 3 1"<<"\n"<<
                   "2 3 4 3 2 2 2 1 3 2 2 2 2 3 2 3 2 1 4 2 1 2 2 2 5 4 2 2 3 3 2 1 2 1 5 3 2 3 3 2"<<"\n"<<
                   "3 4 0 1 2 1 3 1 2 1 1 2 3 4 3 3 2 1"<<"\n"<<
                   "3 2 4 4 4 3 3 2 2 1 4 2 3 4 5 3 5 5 3 3 5 4 4 2 2 2 2 4 3 2 3 2 3 2 2 3 4 3 3 2"<<"\n"<<
                   "1 3 3 0 3 3 5 3 4 2 1 1 3 2 2 3 4 2"<<"\n"<<
                   "2 3 4 4 5 2 1 1 1 3 4 3 2 4 4 4 2 1 1 1 1 2 4 1 2 3 3 1 1 1 3 1 1 1 1 1 1 1 5 3"<<"\n"<<
                   "2 1 1 3 0 1 1 1 4 1 1 1 1 1 1 1 4 1"<<"\n"<<
                   "4 2 3 1 1 2 3 3 3 4 4 3 2 3 3 3 2 4 3 4 4 3 3 2 1 1 3 1 4 3 4 5 1 2 3 4 1 2 3 2"<<"\n"<<
                   "1 1 1 4 1 0 1 5 3 3 1 2 1 3 3 4 4 1"<<"\n"<<
                   "3 3 3 3 4 4 2 2 5 1 5 3 2 2 3 3 3 1 5 3 1 3 5 2 3 3 1 2 3 5 3 2 2 2 4 3 4 3 5 2"<<"\n"<<
                   "3 2 3 5 2 1 0 1 4 2 5 2 3 1 4 3 5 1"<<"\n"<<
                   "4 4 3 3 3 3 3 3 3 4 5 3 3 2 3 4 3 4 1 4 5 3 3 1 3 4 4 2 3 3 4 5 2 2 2 3 2 2 3 2"<<"\n"<<
                   "2 3 2 3 2 5 2 0 3 4 1 2 2 3 3 3 3 2"<<"\n"<<
                   "4 4 2 5 4 4 3 2 4 3 3 4 1 4 4 4 4 1 5 4 1 3 5 3 3 3 3 2 3 2 4 3 4 5 4 3 4 3 4 3"<<"\n"<<
                   "5 2 2 5 5 2 4 2 0 5 2 3 4 2 2 3 4 5"<<"\n"<<
                   "5 4 2 5 3 4 4 4 4 5 5 3 3 3 4 3 2 2 2 5 4 3 3 4 3 3 4 3 2 3 3 2 3 5 3 2 3 3 3 3"<<"\n"<<
                   "2 3 3 4 3 5 3 5 4 0 1 3 3 1 3 5 5 5"<<"\n"<<
                   "2 2 2 3 4 2 2 2 2 5 2 2 3 3 2 2 2 2 3 1 1 1 5 1 1 4 2 1 2 2 2 2 2 1 3 1 5 2 5 2"<<"\n"<<
                   "3 1 2 2 2 1 5 1 3 2 0 1 2 2 2 1 2 1"<<"\n"<<
                   "2 2 3 3 2 2 4 2 4 2 3 4 2 3 3 2 2 3 3 2 4 4 3 3 3 4 3 2 2 2 2 2 3 3 3 2 3 2 2 4"<<"\n"<<
                   "2 2 3 3 3 3 3 3 3 3 2 0 3 3 2 3 4 3"<<"\n"<<
                   "2 2 3 5 2 5 5 1 2 1 3 2 2 4 3 4 4 1 4 5 1 1 4 1 3 1 1 5 3 5 2 2 5 4 3 1 1 3 2 1"<<"\n"<<
                   "4 4 3 1 1 1 1 1 4 2 1 1 0 3 1 2 3 4"<<"\n"<<
                   "2 3 5 3 3 5 3 2 4 3 2 4 4 2 2 3 3 1 2 5 2 5 2 1 5 2 4 3 5 3 2 1 4 2 5 3 2 4 3 4"<<"\n"<<
                   "2 4 4 3 3 2 2 2 3 4 2 2 4 0 3 4 5 3"<<"\n"<<
                   "2 2 3 2 2 3 4 2 2 3 4 2 3 3 3 2 2 4 1 5 4 3 2 2 5 1 3 1 4 2 4 1 3 3 3 2 1 2 1 1"<<"\n"<<
                   "1 2 2 2 2 3 4 2 2 4 1 1 2 5 0 2 2 2"<<"\n"<<
                   "3 1 3 4 2 5 5 3 5 3 4 5 2 4 5 5 5 1 5 3 3 3 3 3 4 2 4 1 4 4 3 3 4 4 5 5 3 1 2 4"<<"\n"<<
                   "4 3 4 4 2 5 3 5 3 5 1 4 3 5 4 0 5 5"<<"\n"<<
                   "2 1 5 5 5 3 4 1 4 1 5 2 2 3 3 4 3 1 2 5 3 2 2 1 1 1 4 1 5 5 1 1 3 2 5 5 1 2 3 2"<<"\n"<<
                   "1 3 2 3 4 2 5 1 3 2 1 2 3 3 1 5 0 5"<<"\n"<<
                   "3 2 2 5 2 4 2 3 3 3 3 4 1 3 3 3 2 3 3 2 2 2 4 4 2 2 3 1 2 3 3 2 2 5 3 2 2 2 2 2"<<"\n"<<
                   "4 2 2 2 2 2 2 2 5 4 1 3 3 3 3 3 3 0";

    }
    else if ( fileName == "Freeman_EIES_networks_32actors.dl" ) {
        qDebug()<< "		... to  " << fileName;
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
        outText <<"DL"<<"\n"<<
                  "N=32 NM=3"<<"\n"<<
                  "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<"\n"<<
                  "ROW LABELS:"<<"\n"<<
                  "\"1\""<<"\n"<<
                  "\"2\""<<"\n"<<
                  "\"3\""<<"\n"<<
                  "\"6\""<<"\n"<<
                  "\"8\""<<"\n"<<
                  "\"10\""<<"\n"<<
                  "\"11\""<<"\n"<<
                  "\"13\""<<"\n"<<
                  "\"14\""<<"\n"<<
                  "\"18\""<<"\n"<<
                  "\"19\""<<"\n"<<
                  "\"20\""<<"\n"<<
                  "\"21\""<<"\n"<<
                  "\"22\""<<"\n"<<
                  "\"23\""<<"\n"<<
                  "\"24\""<<"\n"<<
                  "\"25\""<<"\n"<<
                  "\"26\""<<"\n"<<
                  "\"27\""<<"\n"<<
                  "\"32\""<<"\n"<<
                  "\"33\""<<"\n"<<
                  "\"35\""<<"\n"<<
                  "\"36\""<<"\n"<<
                  "\"37\""<<"\n"<<
                  "\"38\""<<"\n"<<
                  "\"39\""<<"\n"<<
                  "\"40\""<<"\n"<<
                  "\"41\""<<"\n"<<
                  "\"42\""<<"\n"<<
                  "\"43\""<<"\n"<<
                  "\"44\""<<"\n"<<
                  "\"45\""<<"\n"<<
                  "COLUMN LABELS:"<<"\n"<<
                  "\"1\""<<"\n"<<
                  "\"2\""<<"\n"<<
                  "\"3\""<<"\n"<<
                  "\"6\""<<"\n"<<
                  "\"8\""<<"\n"<<
                  "\"10\""<<"\n"<<
                  "\"11\""<<"\n"<<
                  "\"13\""<<"\n"<<
                  "\"14\""<<"\n"<<
                  "\"18\""<<"\n"<<
                  "\"19\""<<"\n"<<
                  "\"20\""<<"\n"<<
                  "\"21\""<<"\n"<<
                  "\"22\""<<"\n"<<
                  "\"23\""<<"\n"<<
                  "\"24\""<<"\n"<<
                  "\"25\""<<"\n"<<
                  "\"26\""<<"\n"<<
                  "\"27\""<<"\n"<<
                  "\"32\""<<"\n"<<
                  "\"33\""<<"\n"<<
                  "\"35\""<<"\n"<<
                  "\"36\""<<"\n"<<
                  "\"37\""<<"\n"<<
                  "\"38\""<<"\n"<<
                  "\"39\""<<"\n"<<
                  "\"40\""<<"\n"<<
                  "\"41\""<<"\n"<<
                  "\"42\""<<"\n"<<
                  "\"43\""<<"\n"<<
                  "\"44\""<<"\n"<<
                  "\"45\""<<"\n"<<
                  "LEVEL LABELS:"<<"\n"<<
                  "\"TIME_1\""<<"\n"<<
                  "\"TIME_2\""<<"\n"<<
                  "\"NUMBER_OF_MESSAGES\""<<"\n"<<
                  "DATA:"<<"\n"<<
                  "0 4 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 2 2 2 2 2 2 2 3 2 2 2 2 4 2"<<"\n"<<
                  "4 0 2 0 1 0 3 3 4 1 3 0 2 2 2 3 2 0 1 2 3 2 0 2 0 0 2 1 2 3 4 4"<<"\n"<<
                  "3 1 0 4 1 0 0 2 0 2 4 4 0 4 1 2 2 2 1 2 2 2 4 2 0 2 0 1 1 1 0 0"<<"\n"<<
                  "2 0 2 0 2 0 0 2 2 2 2 2 2 2 2 1 0 0 4 2 2 2 2 2 2 0 2 2 2 0 2 0"<<"\n"<<
                  "3 0 0 2 0 0 0 2 3 2 2 1 0 2 1 2 2 0 1 2 2 2 0 2 1 0 1 2 2 0 2 2"<<"\n"<<
                  "3 0 0 0 0 0 0 2 0 0 0 0 0 2 0 1 0 0 2 0 1 0 0 0 0 0 2 0 2 0 2 0"<<"\n"<<
                  "3 2 1 0 0 0 0 2 2 0 1 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0"<<"\n"<<
                  "2 2 2 2 2 0 0 0 1 0 2 0 2 2 2 2 2 0 1 2 2 1 1 2 2 0 2 0 2 2 0 0"<<"\n"<<
                  "3 4 0 0 2 0 0 2 0 0 1 0 2 1 0 0 0 0 0 0 1 3 0 0 0 0 3 0 0 0 0 4"<<"\n"<<
                  "2 1 3 3 2 0 1 2 2 0 2 3 0 1 2 2 2 0 2 3 2 2 4 2 2 0 0 2 2 2 0 0"<<"\n"<<
                  "1 3 2 1 1 0 0 3 1 1 0 0 0 2 1 2 2 0 1 2 2 2 1 2 2 0 2 1 1 0 1 0"<<"\n"<<
                  "1 0 1 2 0 0 0 1 0 3 0 0 0 2 0 1 0 0 2 2 2 2 0 0 2 0 0 0 2 2 0 0"<<"\n"<<
                  "3 3 1 2 1 0 3 3 2 1 1 0 0 1 1 1 0 0 2 1 1 1 1 0 0 2 4 2 2 2 3 3"<<"\n"<<
                  "3 2 4 2 3 0 0 3 2 1 2 3 1 0 3 4 3 2 3 3 3 4 3 3 3 2 1 2 4 3 2 0"<<"\n"<<
                  "3 2 2 3 1 0 1 2 2 2 2 1 0 3 0 2 2 0 2 1 2 1 2 2 2 0 0 0 3 0 2 0"<<"\n"<<
                  "2 2 2 1 3 0 0 3 1 0 2 0 0 3 2 0 3 0 1 2 4 3 0 3 2 0 0 0 2 0 0 0"<<"\n"<<
                  "3 2 3 0 2 0 0 3 2 1 2 0 0 3 2 2 0 0 1 3 3 3 0 2 0 0 0 1 1 0 2 0"<<"\n"<<
                  "4 1 2 0 0 0 0 0 0 0 2 0 0 2 1 0 0 0 1 0 0 0 0 1 0 2 2 1 2 2 4 0"<<"\n"<<
                  "2 0 2 4 1 0 0 2 0 2 0 2 0 2 2 1 0 0 0 1 2 3 2 2 2 2 0 2 2 1 2 0"<<"\n"<<
                  "2 2 2 2 2 0 0 2 0 3 2 2 0 3 1 2 2 0 2 0 3 4 2 3 3 0 0 2 3 1 0 0"<<"\n"<<
                  "3 3 2 2 2 0 0 3 1 2 3 2 0 2 3 4 3 0 2 2 0 3 2 2 3 0 1 2 2 1 0 1"<<"\n"<<
                  "2 2 2 3 0 0 0 2 3 2 2 0 0 3 0 3 2 0 3 3 3 0 0 4 2 0 0 2 4 0 0 0"<<"\n"<<
                  "2 0 4 3 0 0 0 0 0 4 0 1 0 2 1 1 0 0 2 2 2 1 0 1 2 0 0 1 2 0 0 0"<<"\n"<<
                  "2 2 2 2 2 0 0 3 2 2 2 2 0 3 2 3 2 0 3 3 3 4 2 0 3 0 2 2 4 0 0 0"<<"\n"<<
                  "2 2 2 2 1 0 0 2 0 3 2 2 0 3 2 3 0 0 2 4 3 3 3 4 0 0 0 1 0 0 0 0"<<"\n"<<
                  "4 1 2 1 1 0 1 1 0 1 1 1 2 2 1 1 0 3 2 1 1 2 1 2 1 0 0 2 2 0 3 0"<<"\n"<<
                  "2 2 1 2 1 0 0 2 2 1 1 0 4 1 1 1 1 0 1 1 1 0 0 0 0 0 0 0 0 2 0 0"<<"\n"<<
                  "3 2 0 3 0 0 0 0 0 1 1 0 1 2 2 2 0 0 3 2 2 3 0 2 1 2 1 0 2 0 2 0"<<"\n"<<
                  "2 2 2 2 2 0 0 2 0 2 0 2 0 3 2 2 0 2 2 2 2 4 2 3 0 2 2 2 0 2 2 0"<<"\n"<<
                  "3 4 1 0 0 0 0 4 0 2 0 0 2 2 0 2 2 2 2 2 2 2 0 0 0 0 2 1 2 0 0 2"<<"\n"<<
                  "4 4 2 2 2 2 1 2 2 0 2 0 2 2 2 1 2 3 2 0 1 2 2 2 0 2 2 2 2 2 0 0"<<"\n"<<
                  "3 3 0 1 2 0 0 3 4 0 1 0 2 1 0 1 0 0 1 1 1 0 0 0 0 0 2 0 0 3 3 0"<<"\n"<<
                  "0 4 2 2 2 2 2 3 3 2 3 2 3 2 2 2 2 3 2 2 2 2 2 2 2 3 2 2 3 2 4 3"<<"\n"<<
                  "4 0 2 2 1 2 2 3 4 2 3 0 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 4 4"<<"\n"<<
                  "3 1 0 4 1 0 0 2 0 2 4 4 0 4 1 2 2 2 1 2 2 2 4 2 0 2 0 1 1 1 0 0"<<"\n"<<
                  "2 2 2 0 2 2 0 2 2 3 2 2 1 2 2 2 0 2 4 2 2 2 2 2 2 2 2 2 2 2 2 0"<<"\n"<<
                  "3 0 0 2 0 0 0 2 3 2 2 1 0 2 1 2 2 0 1 2 2 2 0 2 1 0 1 2 2 0 2 2"<<"\n"<<
                  "4 2 0 0 0 0 0 3 0 2 2 0 0 2 2 2 0 0 2 0 2 0 0 3 0 2 2 2 3 0 4 2"<<"\n"<<
                  "3 2 1 0 0 0 0 2 2 0 1 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0"<<"\n"<<
                  "3 2 2 2 2 2 1 0 1 2 4 1 2 2 2 2 2 2 2 2 2 2 2 2 2 0 2 1 2 2 2 4"<<"\n"<<
                  "3 4 0 0 2 0 0 2 0 0 2 0 2 1 0 1 2 0 0 0 2 2 0 0 0 0 3 0 1 0 2 4"<<"\n"<<
                  "3 0 2 3 2 0 1 2 1 0 2 3 2 1 2 2 2 2 2 4 2 2 4 2 2 0 2 2 3 2 2 1"<<"\n"<<
                  "3 2 2 2 2 2 0 4 2 2 0 0 2 2 2 2 2 2 2 2 2 2 1 2 2 0 2 2 2 0 3 3"<<"\n"<<
                  "2 0 1 2 0 0 1 1 0 3 0 0 0 2 1 1 0 0 2 3 2 1 1 1 2 0 0 1 0 1 2 2"<<"\n"<<
                  "3 3 1 2 1 0 3 3 2 1 2 0 0 1 1 2 0 2 2 1 2 2 1 0 0 2 4 2 2 2 3 3"<<"\n"<<
                  "3 2 4 3 3 0 0 3 0 2 2 3 2 0 3 4 4 2 3 3 3 4 3 3 3 2 2 3 4 3 3 2"<<"\n"<<
                  "3 2 2 3 1 0 0 2 2 2 2 2 0 3 0 2 2 0 2 1 2 2 2 2 2 0 1 0 3 0 3 1"<<"\n"<<
                  "2 2 2 2 3 2 0 3 1 2 2 0 0 3 2 0 2 0 2 2 4 3 0 2 2 2 1 1 2 0 2 2"<<"\n"<<
                  "3 2 3 1 2 0 0 3 2 1 3 1 1 3 2 3 0 1 1 3 3 3 0 2 0 1 2 1 2 2 2 2"<<"\n"<<
                  "4 2 2 0 0 0 1 2 0 0 2 0 1 2 0 0 0 0 0 0 0 0 0 0 0 2 2 0 2 2 4 1"<<"\n"<<
                  "2 0 2 4 1 0 0 2 0 2 0 2 0 2 2 1 0 0 0 2 2 3 2 2 2 2 0 2 2 1 2 0"<<"\n"<<
                  "2 2 2 2 2 0 0 2 0 3 2 2 0 3 1 2 2 0 2 0 3 4 2 3 3 0 0 2 3 1 0 0"<<"\n"<<
                  "3 3 2 2 2 0 0 3 1 2 3 2 0 2 3 4 3 0 2 2 0 3 2 2 3 0 1 2 2 1 0 1"<<"\n"<<
                  "2 2 2 3 0 0 0 2 3 2 2 0 0 3 2 3 2 0 3 3 3 0 0 4 2 0 0 2 4 0 0 2"<<"\n"<<
                  "3 2 4 3 0 0 0 2 0 4 0 1 0 3 1 1 0 0 3 2 1 1 0 2 2 0 0 2 3 2 2 0"<<"\n"<<
                  "3 2 2 2 3 2 0 3 2 2 3 2 2 3 2 3 2 2 2 3 3 4 2 0 3 0 2 3 3 2 2 2"<<"\n"<<
                  "2 2 2 3 1 0 0 3 0 3 2 2 0 3 2 3 0 0 2 3 3 3 3 3 0 0 0 1 2 0 0 0"<<"\n"<<
                  "4 1 2 1 1 0 1 1 0 1 1 1 2 2 1 1 0 3 2 1 1 2 1 2 1 0 0 2 2 0 3 0"<<"\n"<<
                  "3 2 2 2 2 2 0 3 3 2 2 0 4 1 2 2 2 2 2 1 2 2 1 2 0 0 0 0 2 2 2 2"<<"\n"<<
                  "3 2 0 3 0 0 0 2 0 1 1 0 2 2 2 2 0 0 3 2 2 3 0 2 1 2 1 0 2 0 2 2"<<"\n"<<
                  "3 2 2 3 2 2 0 3 0 3 2 3 2 4 3 2 2 2 2 3 2 4 2 4 0 2 2 2 0 2 3 2"<<"\n"<<
                  "3 3 1 2 0 2 0 3 0 2 2 0 2 2 0 2 2 2 2 2 2 2 0 2 2 0 3 2 3 0 3 3"<<"\n"<<
                  "4 4 2 2 2 3 2 2 2 2 3 2 3 2 3 2 2 3 2 2 2 2 2 2 2 2 2 2 3 2 0 4"<<"\n"<<
                  "4 4 0 2 2 2 0 4 4 2 3 0 2 1 0 3 2 0 0 1 2 3 1 1 0 2 2 1 3 2 4 0"<<"\n"<<
                  "24 488  28  65  20  65  45 346  82  52 177  28  24  49  81  77  77  73  33  31  22  46  31 128  38  89  95  25 388  71 212 185"<<"\n"<<
                  "364   6  17  17  15   0  30  20  35  20  22  15  15  15  15  50  25   8   0  15  15  15  15   0  15  15  10  24  89  23 163  39"<<"\n"<<
                  "4   5   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0"<<"\n"<<
                  "52  30   0   4   0   2   0  32  21  34   9   0   0   0   0   5   4   2  35   0   0   0   0  12   0   0  12   5  20   4  19  33"<<"\n"<<
                  "26   4   4   4   0   4   8   4   4   4   4   4   4   4   4   4   4   4   4   0   4   8   4  14   4   0   4   0   4   7   4   4"<<"\n"<<
                  "72  23   0   2   0  34   0  16   0   7  15   0   0   0   8   7   6   0   0   0   0   0   0  14   0   0   7   3  34   3  22   0"<<"\n"<<
                  "14   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   6   0"<<"\n"<<
                  "239  82   5  37   3  34   5  10  12  18 164  18   0   0   0  30  53  27  20   4   0   5   4  55   0   9  34   0 146 216  88 288"<<"\n"<<
                  "24  25   0   2   0   0   0   8  16   0  15   0  10   0   0   0   5   0   0   0   0   0   0   0   0   0  15   0  10   0  30  44"<<"\n"<<
                  "43  15   0  32   0  12   0  14   0   5  25   2   0   0   0  10  10   0  20  15   0   5  20  29   0   4  10   0  47   6  22  19"<<"\n"<<
                  "178  36   0  11   0  19  10 172  39  28  29   0   4   0   0  23  15  24   0   0   8   0   0  29  10  11  22   0  46   0 119  34"<<"\n"<<
                  "0   5   0   0   0   0   0   5   0   0   0   3   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0  53   0   5   9"<<"\n"<<
                  "5   0   0   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   5   0"<<"\n"<<
                  "12   0   9   0   0   0   0   0   0   0   0   0   0   2   0  12   0   0   5   0   0   0   0   0   0   0   0   0  35   0   8   0"<<"\n"<<
                  "120   0   0   0   0   4   0   0   0   0   0   5   0   0  78   0   0   0   0   0   0   0   0   0   0   0   8   0  58   0  32   0"<<"\n"<<
                  "58  25   0  10   0   0   0  20   0   5  10   0   0   5   0  15  10   0   0   0   5   0   0   5   0   0   0   0  35   0  10   0"<<"\n"<<
                  "63  18   9   7   0   6   0  36   0   5   9   5   0   5   0   5   0   0   0   5   2   0   0   0   0   0  15   0  10   9  15   9"<<"\n"<<
                  "58   8   5   4   0   0   0   4   0   5  18   0   0   0   0   0   0   4   0   0   0   0   0   0   0   0  20   0   8  10  48   0"<<"\n"<<
                  "5   5   0  25   0   0   0  10   0   0   0   0   0   5   0   0   0   0   5   0   0   0   5   0   0   0   0   0   0   0  10   0"<<"\n"<<
                  "0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   4   0   0   0   0   0   0   0   4   0   0   0"<<"\n"<<
                  "9   0   0   0   0   0   0   0   0   0   3   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   5   0   0   0"<<"\n"<<
                  "10   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0  40   0   0   0   0  15   0   0   5"<<"\n"<<
                  "5   5   5   0   0   0   0   0   0  19   0   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0  14   0   5   0"<<"\n"<<
                  "89  17   4  14  14  18   8  41   4  19  31   4   4   9   4  14   4   9   4   4   4  58   4   5  18  14   9   4 156   4  56  10"<<"\n"<<
                  "32   5   0   0   0   0   0   0   0   0   0   0   0  15   0   0   0   0   0   0   0  10   0  23  10   0   0   0   0   9  15   0"<<"\n"<<
                  "35   5   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0  10   0  13   0"<<"\n"<<
                  "50  28   0  13   0   0   0  19  29   5   8   0  33   0   4   0  10  15   0   0   0   0   0  10   0   0   0   3  32   0  13  33"<<"\n"<<
                  "9   6   0   0   0   3   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   3   0   0   0   6"<<"\n"<<
                  "559 132   5  24  21  29   0 155  15  98  69  89  37  76  80  63  15   4   9  18  43 108  29 218   0  15  66   0   6  14  91 126"<<"\n"<<
                  "39  21   0   6   3   3   0 140   0   7   0   2   0   0   0   0   9   5   0   0   0   0   0   0   0   0   2   0  18   2  20   8"<<"\n"<<
                  "82 125  10  22  10  15  18  70  35  23 114  20  16  15  24  30  28  49  30   5   5  15   8  53  25   8  21   8  65  28   0  67"<<"\n"<<
                  "239  99   0  27   3   0   0 268 101  18  35   4   0   0   0   0   7   0   0   0   0  14   0   5   0   0  50   6  71   7 107 219";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time-1.dl" ) {
        qDebug()<< "		... to  " << fileName;
        datasetDescription = tr("Freeman's EIES network (Acquaintanceship)");
        outText <<
                   "dl"<<"\n"<<
                   "N=48"<<"\n"<<
                   "format=edgelist1"<<"\n"<<
                   "data:"<<"\n"<<
                   "1 2 4"<<"\n"<<
                   "1 3 2"<<"\n"<<
                   "1 6 2"<<"\n"<<
                   "1 8 2"<<"\n"<<
                   "1 10 2"<<"\n"<<
                   "1 11 2"<<"\n"<<
                   "1 13 2"<<"\n"<<
                   "1 14 2"<<"\n"<<
                   "1 18 2"<<"\n"<<
                   "1 19 2"<<"\n"<<
                   "1 20 2"<<"\n"<<
                   "1 21 2"<<"\n"<<
                   "1 22 2"<<"\n"<<
                   "1 23 2"<<"\n"<<
                   "1 24 2"<<"\n"<<
                   "1 25 2"<<"\n"<<
                   "1 26 3"<<"\n"<<
                   "1 27 2"<<"\n"<<
                   "1 31 2"<<"\n"<<
                   "1 32 2"<<"\n"<<
                   "1 33 2"<<"\n"<<
                   "1 35 2"<<"\n"<<
                   "1 36 2"<<"\n"<<
                   "1 37 2"<<"\n"<<
                   "1 38 2"<<"\n"<<
                   "1 39 3"<<"\n"<<
                   "1 40 2"<<"\n"<<
                   "1 41 2"<<"\n"<<
                   "1 42 2"<<"\n"<<
                   "1 43 2"<<"\n"<<
                   "1 44 4"<<"\n"<<
                   "1 45 2"<<"\n"<<
                   "1 46 2"<<"\n"<<
                   "2 1 4"<<"\n"<<
                   "2 3 2"<<"\n"<<
                   "2 8 1"<<"\n"<<
                   "2 11 3"<<"\n"<<
                   "2 13 3"<<"\n"<<
                   "2 14 4"<<"\n"<<
                   "2 18 1"<<"\n"<<
                   "2 19 3"<<"\n"<<
                   "2 21 2"<<"\n"<<
                   "2 22 2"<<"\n"<<
                   "2 23 2"<<"\n"<<
                   "2 24 3"<<"\n"<<
                   "2 25 2"<<"\n"<<
                   "2 27 1"<<"\n"<<
                   "2 32 2"<<"\n"<<
                   "2 33 3"<<"\n"<<
                   "2 35 2"<<"\n"<<
                   "2 37 2"<<"\n"<<
                   "2 40 2"<<"\n"<<
                   "2 41 1"<<"\n"<<
                   "2 42 2"<<"\n"<<
                   "2 43 3"<<"\n"<<
                   "2 44 4"<<"\n"<<
                   "2 45 4"<<"\n"<<
                   "2 46 2"<<"\n"<<
                   "3 1 3"<<"\n"<<
                   "3 2 1"<<"\n"<<
                   "3 6 4"<<"\n"<<
                   "3 8 1"<<"\n"<<
                   "3 13 2"<<"\n"<<
                   "3 18 2"<<"\n"<<
                   "3 19 4"<<"\n"<<
                   "3 20 4"<<"\n"<<
                   "3 22 4"<<"\n"<<
                   "3 23 1"<<"\n"<<
                   "3 24 2"<<"\n"<<
                   "3 25 2"<<"\n"<<
                   "3 26 2"<<"\n"<<
                   "3 27 1"<<"\n"<<
                   "3 31 1"<<"\n"<<
                   "3 32 2"<<"\n"<<
                   "3 33 2"<<"\n"<<
                   "3 35 2"<<"\n"<<
                   "3 36 4"<<"\n"<<
                   "3 37 2"<<"\n"<<
                   "3 39 2"<<"\n"<<
                   "3 41 1"<<"\n"<<
                   "3 42 1"<<"\n"<<
                   "3 43 1"<<"\n"<<
                   "6 1 2"<<"\n"<<
                   "6 3 2"<<"\n"<<
                   "6 8 2"<<"\n"<<
                   "6 13 2"<<"\n"<<
                   "6 14 2"<<"\n"<<
                   "6 18 2"<<"\n"<<
                   "6 19 2"<<"\n"<<
                   "6 20 2"<<"\n"<<
                   "6 21 2"<<"\n"<<
                   "6 22 2"<<"\n"<<
                   "6 23 2"<<"\n"<<
                   "6 24 1"<<"\n"<<
                   "6 27 4"<<"\n"<<
                   "6 31 1"<<"\n"<<
                   "6 32 2"<<"\n"<<
                   "6 33 2"<<"\n"<<
                   "6 35 2"<<"\n"<<
                   "6 36 2"<<"\n"<<
                   "6 37 2"<<"\n"<<
                   "6 38 2"<<"\n"<<
                   "6 40 2"<<"\n"<<
                   "6 41 2"<<"\n"<<
                   "6 42 2"<<"\n"<<
                   "6 44 2"<<"\n"<<
                   "8 1 3"<<"\n"<<
                   "8 6 2"<<"\n"<<
                   "8 13 2"<<"\n"<<
                   "8 14 3"<<"\n"<<
                   "8 18 2"<<"\n"<<
                   "8 19 2"<<"\n"<<
                   "8 20 1"<<"\n"<<
                   "8 22 2"<<"\n"<<
                   "8 23 1"<<"\n"<<
                   "8 24 2"<<"\n"<<
                   "8 25 2"<<"\n"<<
                   "8 27 1"<<"\n"<<
                   "8 32 2"<<"\n"<<
                   "8 33 2"<<"\n"<<
                   "8 35 2"<<"\n"<<
                   "8 37 2"<<"\n"<<
                   "8 38 1"<<"\n"<<
                   "8 40 1"<<"\n"<<
                   "8 41 2"<<"\n"<<
                   "8 42 2"<<"\n"<<
                   "8 44 2"<<"\n"<<
                   "8 45 2"<<"\n"<<
                   "10 1 3"<<"\n"<<
                   "10 13 2"<<"\n"<<
                   "10 22 2"<<"\n"<<
                   "10 24 1"<<"\n"<<
                   "10 27 2"<<"\n"<<
                   "10 33 1"<<"\n"<<
                   "10 40 2"<<"\n"<<
                   "10 42 2"<<"\n"<<
                   "10 44 2"<<"\n"<<
                   "11 1 3"<<"\n"<<
                   "11 2 2"<<"\n"<<
                   "11 3 1"<<"\n"<<
                   "11 13 2"<<"\n"<<
                   "11 14 2"<<"\n"<<
                   "11 19 1"<<"\n"<<
                   "11 21 3"<<"\n"<<
                   "11 41 2"<<"\n"<<
                   "13 1 2"<<"\n"<<
                   "13 2 2"<<"\n"<<
                   "13 3 2"<<"\n"<<
                   "13 6 2"<<"\n"<<
                   "13 8 2"<<"\n"<<
                   "13 14 1"<<"\n"<<
                   "13 19 2"<<"\n"<<
                   "13 21 2"<<"\n"<<
                   "13 22 2"<<"\n"<<
                   "13 23 2"<<"\n"<<
                   "13 24 2"<<"\n"<<
                   "13 25 2"<<"\n"<<
                   "13 27 1"<<"\n"<<
                   "13 32 2"<<"\n"<<
                   "13 33 2"<<"\n"<<
                   "13 35 1"<<"\n"<<
                   "13 36 1"<<"\n"<<
                   "13 37 2"<<"\n"<<
                   "13 38 2"<<"\n"<<
                   "13 40 2"<<"\n"<<
                   "13 42 2"<<"\n"<<
                   "13 43 2"<<"\n"<<
                   "14 1 3"<<"\n"<<
                   "14 2 4"<<"\n"<<
                   "14 8 2"<<"\n"<<
                   "14 13 2"<<"\n"<<
                   "14 19 1"<<"\n"<<
                   "14 21 2"<<"\n"<<
                   "14 22 1"<<"\n"<<
                   "14 33 1"<<"\n"<<
                   "14 35 3"<<"\n"<<
                   "14 40 3"<<"\n"<<
                   "14 45 4"<<"\n"<<
                   "18 1 2"<<"\n"<<
                   "18 2 1"<<"\n"<<
                   "18 3 3"<<"\n"<<
                   "18 6 3"<<"\n"<<
                   "18 8 2"<<"\n"<<
                   "18 11 1"<<"\n"<<
                   "18 13 2"<<"\n"<<
                   "18 14 2"<<"\n"<<
                   "18 19 2"<<"\n"<<
                   "18 20 3"<<"\n"<<
                   "18 22 1"<<"\n"<<
                   "18 23 2"<<"\n"<<
                   "18 24 2"<<"\n"<<
                   "18 25 2"<<"\n"<<
                   "18 27 2"<<"\n"<<
                   "18 31 2"<<"\n"<<
                   "18 32 3"<<"\n"<<
                   "18 33 2"<<"\n"<<
                   "18 35 2"<<"\n"<<
                   "18 36 4"<<"\n"<<
                   "18 37 2"<<"\n"<<
                   "18 38 2"<<"\n"<<
                   "18 41 2"<<"\n"<<
                   "18 42 2"<<"\n"<<
                   "18 43 2"<<"\n"<<
                   "19 1 1"<<"\n"<<
                   "19 2 3"<<"\n"<<
                   "19 3 2"<<"\n"<<
                   "19 6 1"<<"\n"<<
                   "19 8 1"<<"\n"<<
                   "19 13 3"<<"\n"<<
                   "19 14 1"<<"\n"<<
                   "19 18 1"<<"\n"<<
                   "19 22 2"<<"\n"<<
                   "19 23 1"<<"\n"<<
                   "19 24 2"<<"\n"<<
                   "19 25 2"<<"\n"<<
                   "19 27 1"<<"\n"<<
                   "19 31 2"<<"\n"<<
                   "19 32 2"<<"\n"<<
                   "19 33 2"<<"\n"<<
                   "19 35 2"<<"\n"<<
                   "19 36 1"<<"\n"<<
                   "19 37 2"<<"\n"<<
                   "19 38 2"<<"\n"<<
                   "19 40 2"<<"\n"<<
                   "19 41 1"<<"\n"<<
                   "19 42 1"<<"\n"<<
                   "19 44 1"<<"\n"<<
                   "20 1 1"<<"\n"<<
                   "20 3 1"<<"\n"<<
                   "20 6 2"<<"\n"<<
                   "20 13 1"<<"\n"<<
                   "20 18 3"<<"\n"<<
                   "20 22 2"<<"\n"<<
                   "20 24 1"<<"\n"<<
                   "20 27 2"<<"\n"<<
                   "20 32 2"<<"\n"<<
                   "20 33 2"<<"\n"<<
                   "20 35 2"<<"\n"<<
                   "20 38 2"<<"\n"<<
                   "20 42 2"<<"\n"<<
                   "20 43 2"<<"\n"<<
                   "21 1 3"<<"\n"<<
                   "21 2 3"<<"\n"<<
                   "21 3 1"<<"\n"<<
                   "21 6 2"<<"\n"<<
                   "21 8 1"<<"\n"<<
                   "21 11 3"<<"\n"<<
                   "21 13 3"<<"\n"<<
                   "21 14 2"<<"\n"<<
                   "21 18 1"<<"\n"<<
                   "21 19 1"<<"\n"<<
                   "21 22 1"<<"\n"<<
                   "21 23 1"<<"\n"<<
                   "21 24 1"<<"\n"<<
                   "21 27 2"<<"\n"<<
                   "21 31 1"<<"\n"<<
                   "21 32 1"<<"\n"<<
                   "21 33 1"<<"\n"<<
                   "21 35 1"<<"\n"<<
                   "21 36 1"<<"\n"<<
                   "21 39 2"<<"\n"<<
                   "21 40 4"<<"\n"<<
                   "21 41 2"<<"\n"<<
                   "21 42 2"<<"\n"<<
                   "21 43 2"<<"\n"<<
                   "21 44 3"<<"\n"<<
                   "21 45 3"<<"\n"<<
                   "22 1 3"<<"\n"<<
                   "22 2 2"<<"\n"<<
                   "22 3 4"<<"\n"<<
                   "22 6 2"<<"\n"<<
                   "22 8 3"<<"\n"<<
                   "22 13 3"<<"\n"<<
                   "22 14 2"<<"\n"<<
                   "22 18 1"<<"\n"<<
                   "22 19 2"<<"\n"<<
                   "22 20 3"<<"\n"<<
                   "22 21 1"<<"\n"<<
                   "22 23 3"<<"\n"<<
                   "22 24 4"<<"\n"<<
                   "22 25 3"<<"\n"<<
                   "22 26 2"<<"\n"<<
                   "22 27 3"<<"\n"<<
                   "22 31 2"<<"\n"<<
                   "22 32 3"<<"\n"<<
                   "22 33 3"<<"\n"<<
                   "22 35 4"<<"\n"<<
                   "22 36 3"<<"\n"<<
                   "22 37 3"<<"\n"<<
                   "22 38 3"<<"\n"<<
                   "22 39 2"<<"\n"<<
                   "22 40 1"<<"\n"<<
                   "22 41 2"<<"\n"<<
                   "22 42 4"<<"\n"<<
                   "22 43 3"<<"\n"<<
                   "22 44 2"<<"\n"<<
                   "22 46 1"<<"\n"<<
                   "23 1 3"<<"\n"<<
                   "23 2 2"<<"\n"<<
                   "23 3 2"<<"\n"<<
                   "23 6 3"<<"\n"<<
                   "23 8 1"<<"\n"<<
                   "23 11 1"<<"\n"<<
                   "23 13 2"<<"\n"<<
                   "23 14 2"<<"\n"<<
                   "23 18 2"<<"\n"<<
                   "23 19 2"<<"\n"<<
                   "23 20 1"<<"\n"<<
                   "23 22 3"<<"\n"<<
                   "23 24 2"<<"\n"<<
                   "23 25 2"<<"\n"<<
                   "23 27 2"<<"\n"<<
                   "23 31 4"<<"\n"<<
                   "23 32 1"<<"\n"<<
                   "23 33 2"<<"\n"<<
                   "23 35 1"<<"\n"<<
                   "23 36 2"<<"\n"<<
                   "23 37 2"<<"\n"<<
                   "23 38 2"<<"\n"<<
                   "23 42 3"<<"\n"<<
                   "23 44 2"<<"\n"<<
                   "23 46 1"<<"\n"<<
                   "24 1 2"<<"\n"<<
                   "24 2 2"<<"\n"<<
                   "24 3 2"<<"\n"<<
                   "24 6 1"<<"\n"<<
                   "24 8 3"<<"\n"<<
                   "24 13 3"<<"\n"<<
                   "24 14 1"<<"\n"<<
                   "24 19 2"<<"\n"<<
                   "24 22 3"<<"\n"<<
                   "24 23 2"<<"\n"<<
                   "24 25 3"<<"\n"<<
                   "24 27 1"<<"\n"<<
                   "24 31 2"<<"\n"<<
                   "24 32 2"<<"\n"<<
                   "24 33 4"<<"\n"<<
                   "24 35 3"<<"\n"<<
                   "24 37 3"<<"\n"<<
                   "24 38 2"<<"\n"<<
                   "24 42 2"<<"\n"<<
                   "25 1 3"<<"\n"<<
                   "25 2 2"<<"\n"<<
                   "25 3 3"<<"\n"<<
                   "25 8 2"<<"\n"<<
                   "25 13 3"<<"\n"<<
                   "25 14 2"<<"\n"<<
                   "25 18 1"<<"\n"<<
                   "25 19 2"<<"\n"<<
                   "25 22 3"<<"\n"<<
                   "25 23 2"<<"\n"<<
                   "25 24 2"<<"\n"<<
                   "25 27 1"<<"\n"<<
                   "25 32 3"<<"\n"<<
                   "25 33 3"<<"\n"<<
                   "25 35 3"<<"\n"<<
                   "25 37 2"<<"\n"<<
                   "25 41 1"<<"\n"<<
                   "25 42 1"<<"\n"<<
                   "25 44 2"<<"\n"<<
                   "25 46 1"<<"\n"<<
                   "26 1 4"<<"\n"<<
                   "26 2 1"<<"\n"<<
                   "26 3 2"<<"\n"<<
                   "26 19 2"<<"\n"<<
                   "26 22 2"<<"\n"<<
                   "26 23 1"<<"\n"<<
                   "26 27 1"<<"\n"<<
                   "26 37 1"<<"\n"<<
                   "26 39 2"<<"\n"<<
                   "26 40 2"<<"\n"<<
                   "26 41 1"<<"\n"<<
                   "26 42 2"<<"\n"<<
                   "26 43 2"<<"\n"<<
                   "26 44 4"<<"\n"<<
                   "26 46 2"<<"\n"<<
                   "27 1 2"<<"\n"<<
                   "27 3 2"<<"\n"<<
                   "27 6 4"<<"\n"<<
                   "27 8 1"<<"\n"<<
                   "27 13 2"<<"\n"<<
                   "27 18 2"<<"\n"<<
                   "27 20 2"<<"\n"<<
                   "27 22 2"<<"\n"<<
                   "27 23 2"<<"\n"<<
                   "27 24 1"<<"\n"<<
                   "27 32 1"<<"\n"<<
                   "27 33 2"<<"\n"<<
                   "27 35 3"<<"\n"<<
                   "27 36 2"<<"\n"<<
                   "27 37 2"<<"\n"<<
                   "27 38 2"<<"\n"<<
                   "27 39 2"<<"\n"<<
                   "27 41 2"<<"\n"<<
                   "27 42 2"<<"\n"<<
                   "27 43 1"<<"\n"<<
                   "27 44 2"<<"\n"<<
                   "27 46 2"<<"\n"<<
                   "31 1 1"<<"\n"<<
                   "31 3 2"<<"\n"<<
                   "31 6 1"<<"\n"<<
                   "31 8 1"<<"\n"<<
                   "31 18 2"<<"\n"<<
                   "31 19 2"<<"\n"<<
                   "31 20 2"<<"\n"<<
                   "31 22 2"<<"\n"<<
                   "31 23 2"<<"\n"<<
                   "31 24 2"<<"\n"<<
                   "31 32 1"<<"\n"<<
                   "31 35 3"<<"\n"<<
                   "31 36 1"<<"\n"<<
                   "31 37 3"<<"\n"<<
                   "31 38 2"<<"\n"<<
                   "31 42 1"<<"\n"<<
                   "32 1 2"<<"\n"<<
                   "32 2 2"<<"\n"<<
                   "32 3 2"<<"\n"<<
                   "32 6 2"<<"\n"<<
                   "32 8 2"<<"\n"<<
                   "32 13 2"<<"\n"<<
                   "32 18 3"<<"\n"<<
                   "32 19 2"<<"\n"<<
                   "32 20 2"<<"\n"<<
                   "32 22 3"<<"\n"<<
                   "32 23 1"<<"\n"<<
                   "32 24 2"<<"\n"<<
                   "32 25 2"<<"\n"<<
                   "32 27 2"<<"\n"<<
                   "32 31 1"<<"\n"<<
                   "32 33 3"<<"\n"<<
                   "32 35 4"<<"\n"<<
                   "32 36 2"<<"\n"<<
                   "32 37 3"<<"\n"<<
                   "32 38 3"<<"\n"<<
                   "32 41 2"<<"\n"<<
                   "32 42 3"<<"\n"<<
                   "32 43 1"<<"\n"<<
                   "33 1 3"<<"\n"<<
                   "33 2 3"<<"\n"<<
                   "33 3 2"<<"\n"<<
                   "33 6 2"<<"\n"<<
                   "33 8 2"<<"\n"<<
                   "33 13 3"<<"\n"<<
                   "33 14 1"<<"\n"<<
                   "33 18 2"<<"\n"<<
                   "33 19 3"<<"\n"<<
                   "33 20 2"<<"\n"<<
                   "33 22 2"<<"\n"<<
                   "33 23 3"<<"\n"<<
                   "33 24 4"<<"\n"<<
                   "33 25 3"<<"\n"<<
                   "33 27 2"<<"\n"<<
                   "33 31 2"<<"\n"<<
                   "33 32 2"<<"\n"<<
                   "33 35 3"<<"\n"<<
                   "33 36 2"<<"\n"<<
                   "33 37 2"<<"\n"<<
                   "33 38 3"<<"\n"<<
                   "33 40 1"<<"\n"<<
                   "33 41 2"<<"\n"<<
                   "33 42 2"<<"\n"<<
                   "33 43 1"<<"\n"<<
                   "33 45 1"<<"\n"<<
                   "35 1 2"<<"\n"<<
                   "35 2 2"<<"\n"<<
                   "35 3 2"<<"\n"<<
                   "35 6 3"<<"\n"<<
                   "35 13 2"<<"\n"<<
                   "35 14 3"<<"\n"<<
                   "35 18 2"<<"\n"<<
                   "35 19 2"<<"\n"<<
                   "35 22 3"<<"\n"<<
                   "35 24 3"<<"\n"<<
                   "35 25 2"<<"\n"<<
                   "35 27 3"<<"\n"<<
                   "35 32 3"<<"\n"<<
                   "35 33 3"<<"\n"<<
                   "35 37 4"<<"\n"<<
                   "35 38 2"<<"\n"<<
                   "35 41 2"<<"\n"<<
                   "35 42 4"<<"\n"<<
                   "36 1 2"<<"\n"<<
                   "36 3 4"<<"\n"<<
                   "36 6 3"<<"\n"<<
                   "36 18 4"<<"\n"<<
                   "36 20 1"<<"\n"<<
                   "36 22 2"<<"\n"<<
                   "36 23 1"<<"\n"<<
                   "36 24 1"<<"\n"<<
                   "36 27 2"<<"\n"<<
                   "36 31 1"<<"\n"<<
                   "36 32 2"<<"\n"<<
                   "36 33 2"<<"\n"<<
                   "36 35 1"<<"\n"<<
                   "36 37 1"<<"\n"<<
                   "36 38 2"<<"\n"<<
                   "36 41 1"<<"\n"<<
                   "36 42 2"<<"\n"<<
                   "37 1 2"<<"\n"<<
                   "37 2 2"<<"\n"<<
                   "37 3 2"<<"\n"<<
                   "37 6 2"<<"\n"<<
                   "37 8 2"<<"\n"<<
                   "37 13 3"<<"\n"<<
                   "37 14 2"<<"\n"<<
                   "37 18 2"<<"\n"<<
                   "37 19 2"<<"\n"<<
                   "37 20 2"<<"\n"<<
                   "37 22 3"<<"\n"<<
                   "37 23 2"<<"\n"<<
                   "37 24 3"<<"\n"<<
                   "37 25 2"<<"\n"<<
                   "37 27 3"<<"\n"<<
                   "37 31 4"<<"\n"<<
                   "37 32 3"<<"\n"<<
                   "37 33 3"<<"\n"<<
                   "37 35 4"<<"\n"<<
                   "37 36 2"<<"\n"<<
                   "37 38 3"<<"\n"<<
                   "37 40 2"<<"\n"<<
                   "37 41 2"<<"\n"<<
                   "37 42 4"<<"\n"<<
                   "38 1 2"<<"\n"<<
                   "38 2 2"<<"\n"<<
                   "38 3 2"<<"\n"<<
                   "38 6 2"<<"\n"<<
                   "38 8 1"<<"\n"<<
                   "38 13 2"<<"\n"<<
                   "38 18 3"<<"\n"<<
                   "38 19 2"<<"\n"<<
                   "38 20 2"<<"\n"<<
                   "38 22 3"<<"\n"<<
                   "38 23 2"<<"\n"<<
                   "38 24 3"<<"\n"<<
                   "38 27 2"<<"\n"<<
                   "38 31 2"<<"\n"<<
                   "38 32 4"<<"\n"<<
                   "38 33 3"<<"\n"<<
                   "38 35 3"<<"\n"<<
                   "38 36 3"<<"\n"<<
                   "38 37 4"<<"\n"<<
                   "38 41 1"<<"\n"<<
                   "39 1 4"<<"\n"<<
                   "39 2 1"<<"\n"<<
                   "39 3 2"<<"\n"<<
                   "39 6 1"<<"\n"<<
                   "39 8 1"<<"\n"<<
                   "39 11 1"<<"\n"<<
                   "39 13 1"<<"\n"<<
                   "39 18 1"<<"\n"<<
                   "39 19 1"<<"\n"<<
                   "39 20 1"<<"\n"<<
                   "39 21 2"<<"\n"<<
                   "39 22 2"<<"\n"<<
                   "39 23 1"<<"\n"<<
                   "39 24 1"<<"\n"<<
                   "39 26 3"<<"\n"<<
                   "39 27 2"<<"\n"<<
                   "39 32 1"<<"\n"<<
                   "39 33 1"<<"\n"<<
                   "39 35 2"<<"\n"<<
                   "39 36 1"<<"\n"<<
                   "39 37 2"<<"\n"<<
                   "39 38 1"<<"\n"<<
                   "39 41 2"<<"\n"<<
                   "39 42 2"<<"\n"<<
                   "39 44 3"<<"\n"<<
                   "39 46 1"<<"\n"<<
                   "40 1 2"<<"\n"<<
                   "40 2 2"<<"\n"<<
                   "40 3 1"<<"\n"<<
                   "40 6 2"<<"\n"<<
                   "40 8 1"<<"\n"<<
                   "40 13 2"<<"\n"<<
                   "40 14 2"<<"\n"<<
                   "40 18 1"<<"\n"<<
                   "40 19 1"<<"\n"<<
                   "40 21 4"<<"\n"<<
                   "40 22 1"<<"\n"<<
                   "40 23 1"<<"\n"<<
                   "40 24 1"<<"\n"<<
                   "40 25 1"<<"\n"<<
                   "40 27 1"<<"\n"<<
                   "40 32 1"<<"\n"<<
                   "40 33 1"<<"\n"<<
                   "40 43 2"<<"\n"<<
                   "41 1 3"<<"\n"<<
                   "41 2 2"<<"\n"<<
                   "41 6 3"<<"\n"<<
                   "41 18 1"<<"\n"<<
                   "41 19 1"<<"\n"<<
                   "41 21 1"<<"\n"<<
                   "41 22 2"<<"\n"<<
                   "41 23 2"<<"\n"<<
                   "41 24 2"<<"\n"<<
                   "41 27 3"<<"\n"<<
                   "41 32 2"<<"\n"<<
                   "41 33 2"<<"\n"<<
                   "41 35 3"<<"\n"<<
                   "41 37 2"<<"\n"<<
                   "41 38 1"<<"\n"<<
                   "41 39 2"<<"\n"<<
                   "41 40 1"<<"\n"<<
                   "41 42 2"<<"\n"<<
                   "41 44 2"<<"\n"<<
                   "42 1 2"<<"\n"<<
                   "42 2 2"<<"\n"<<
                   "42 3 2"<<"\n"<<
                   "42 6 2"<<"\n"<<
                   "42 8 2"<<"\n"<<
                   "42 13 2"<<"\n"<<
                   "42 18 2"<<"\n"<<
                   "42 20 2"<<"\n"<<
                   "42 22 3"<<"\n"<<
                   "42 23 2"<<"\n"<<
                   "42 24 2"<<"\n"<<
                   "42 26 2"<<"\n"<<
                   "42 27 2"<<"\n"<<
                   "42 32 2"<<"\n"<<
                   "42 33 2"<<"\n"<<
                   "42 35 4"<<"\n"<<
                   "42 36 2"<<"\n"<<
                   "42 37 3"<<"\n"<<
                   "42 39 2"<<"\n"<<
                   "42 40 2"<<"\n"<<
                   "42 41 2"<<"\n"<<
                   "42 43 2"<<"\n"<<
                   "42 44 2"<<"\n"<<
                   "42 46 3"<<"\n"<<
                   "43 1 3"<<"\n"<<
                   "43 2 4"<<"\n"<<
                   "43 3 1"<<"\n"<<
                   "43 13 4"<<"\n"<<
                   "43 18 2"<<"\n"<<
                   "43 21 2"<<"\n"<<
                   "43 22 2"<<"\n"<<
                   "43 24 2"<<"\n"<<
                   "43 25 2"<<"\n"<<
                   "43 26 2"<<"\n"<<
                   "43 27 2"<<"\n"<<
                   "43 32 2"<<"\n"<<
                   "43 33 2"<<"\n"<<
                   "43 35 2"<<"\n"<<
                   "43 40 2"<<"\n"<<
                   "43 41 1"<<"\n"<<
                   "43 42 2"<<"\n"<<
                   "43 45 2"<<"\n"<<
                   "43 46 1"<<"\n"<<
                   "44 1 4"<<"\n"<<
                   "44 2 4"<<"\n"<<
                   "44 3 2"<<"\n"<<
                   "44 6 2"<<"\n"<<
                   "44 8 2"<<"\n"<<
                   "44 10 2"<<"\n"<<
                   "44 11 1"<<"\n"<<
                   "44 13 2"<<"\n"<<
                   "44 14 2"<<"\n"<<
                   "44 19 2"<<"\n"<<
                   "44 21 2"<<"\n"<<
                   "44 22 2"<<"\n"<<
                   "44 23 2"<<"\n"<<
                   "44 24 1"<<"\n"<<
                   "44 25 2"<<"\n"<<
                   "44 26 3"<<"\n"<<
                   "44 27 2"<<"\n"<<
                   "44 33 1"<<"\n"<<
                   "44 35 2"<<"\n"<<
                   "44 36 2"<<"\n"<<
                   "44 37 2"<<"\n"<<
                   "44 39 2"<<"\n"<<
                   "44 40 2"<<"\n"<<
                   "44 41 2"<<"\n"<<
                   "44 42 2"<<"\n"<<
                   "44 43 2"<<"\n"<<
                   "44 46 1"<<"\n"<<
                   "45 1 3"<<"\n"<<
                   "45 2 3"<<"\n"<<
                   "45 6 1"<<"\n"<<
                   "45 8 2"<<"\n"<<
                   "45 13 3"<<"\n"<<
                   "45 14 4"<<"\n"<<
                   "45 19 1"<<"\n"<<
                   "45 21 2"<<"\n"<<
                   "45 22 1"<<"\n"<<
                   "45 24 1"<<"\n"<<
                   "45 27 1"<<"\n"<<
                   "45 32 1"<<"\n"<<
                   "45 33 1"<<"\n"<<
                   "45 40 2"<<"\n"<<
                   "45 43 3"<<"\n"<<
                   "45 44 3"<<"\n"<<
                   "45 46 1"<<"\n"<<
                   "46 1 2"<<"\n"<<
                   "46 2 2"<<"\n"<<
                   "46 42 3";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time-2.dl" ) {
        qDebug()<< "		... to  " << fileName;
        datasetDescription = tr("Freeman's EIES network (Acquaintanceship) at time 2");
        outText <<
                   "dl"<<"\n"<<
                   "N=48"<<"\n"<<
                   "format=edgelist1"<<"\n"<<
                   "data:"<<"\n"<<
                   "1 2 4"<<"\n"<<
                   "1 3 2"<<"\n"<<
                   "1 6 2"<<"\n"<<
                   "1 8 2"<<"\n"<<
                   "1 10 2"<<"\n"<<
                   "1 11 2"<<"\n"<<
                   "1 13 3"<<"\n"<<
                   "1 14 3"<<"\n"<<
                   "1 18 2"<<"\n"<<
                   "1 19 3"<<"\n"<<
                   "1 20 2"<<"\n"<<
                   "1 21 3"<<"\n"<<
                   "1 22 2"<<"\n"<<
                   "1 23 2"<<"\n"<<
                   "1 24 2"<<"\n"<<
                   "1 25 2"<<"\n"<<
                   "1 26 3"<<"\n"<<
                   "1 27 2"<<"\n"<<
                   "1 31 2"<<"\n"<<
                   "1 32 2"<<"\n"<<
                   "1 33 2"<<"\n"<<
                   "1 35 2"<<"\n"<<
                   "1 36 2"<<"\n"<<
                   "1 37 2"<<"\n"<<
                   "1 38 2"<<"\n"<<
                   "1 39 3"<<"\n"<<
                   "1 40 2"<<"\n"<<
                   "1 41 2"<<"\n"<<
                   "1 42 3"<<"\n"<<
                   "1 43 2"<<"\n"<<
                   "1 44 4"<<"\n"<<
                   "1 45 3"<<"\n"<<
                   "1 46 3"<<"\n"<<
                   "2 1 4"<<"\n"<<
                   "2 3 2"<<"\n"<<
                   "2 6 2"<<"\n"<<
                   "2 8 1"<<"\n"<<
                   "2 10 2"<<"\n"<<
                   "2 11 2"<<"\n"<<
                   "2 13 3"<<"\n"<<
                   "2 14 4"<<"\n"<<
                   "2 18 2"<<"\n"<<
                   "2 19 3"<<"\n"<<
                   "2 21 2"<<"\n"<<
                   "2 22 2"<<"\n"<<
                   "2 23 2"<<"\n"<<
                   "2 24 2"<<"\n"<<
                   "2 25 2"<<"\n"<<
                   "2 26 2"<<"\n"<<
                   "2 27 2"<<"\n"<<
                   "2 32 2"<<"\n"<<
                   "2 33 2"<<"\n"<<
                   "2 35 2"<<"\n"<<
                   "2 36 2"<<"\n"<<
                   "2 37 2"<<"\n"<<
                   "2 38 2"<<"\n"<<
                   "2 39 2"<<"\n"<<
                   "2 40 2"<<"\n"<<
                   "2 41 2"<<"\n"<<
                   "2 42 2"<<"\n"<<
                   "2 43 3"<<"\n"<<
                   "2 44 4"<<"\n"<<
                   "2 45 4"<<"\n"<<
                   "2 46 2"<<"\n"<<
                   "3 1 3"<<"\n"<<
                   "3 2 1"<<"\n"<<
                   "3 6 4"<<"\n"<<
                   "3 8 1"<<"\n"<<
                   "3 13 2"<<"\n"<<
                   "3 18 2"<<"\n"<<
                   "3 19 4"<<"\n"<<
                   "3 20 4"<<"\n"<<
                   "3 22 4"<<"\n"<<
                   "3 23 1"<<"\n"<<
                   "3 24 2"<<"\n"<<
                   "3 25 2"<<"\n"<<
                   "3 26 2"<<"\n"<<
                   "3 27 1"<<"\n"<<
                   "3 31 1"<<"\n"<<
                   "3 32 2"<<"\n"<<
                   "3 33 2"<<"\n"<<
                   "3 35 2"<<"\n"<<
                   "3 36 4"<<"\n"<<
                   "3 37 2"<<"\n"<<
                   "3 39 2"<<"\n"<<
                   "3 41 1"<<"\n"<<
                   "3 42 1"<<"\n"<<
                   "3 43 1"<<"\n"<<
                   "6 1 2"<<"\n"<<
                   "6 2 2"<<"\n"<<
                   "6 3 2"<<"\n"<<
                   "6 8 2"<<"\n"<<
                   "6 10 2"<<"\n"<<
                   "6 13 2"<<"\n"<<
                   "6 14 2"<<"\n"<<
                   "6 18 3"<<"\n"<<
                   "6 19 2"<<"\n"<<
                   "6 20 2"<<"\n"<<
                   "6 21 1"<<"\n"<<
                   "6 22 2"<<"\n"<<
                   "6 23 2"<<"\n"<<
                   "6 24 2"<<"\n"<<
                   "6 26 2"<<"\n"<<
                   "6 27 4"<<"\n"<<
                   "6 31 1"<<"\n"<<
                   "6 32 2"<<"\n"<<
                   "6 33 2"<<"\n"<<
                   "6 35 2"<<"\n"<<
                   "6 36 2"<<"\n"<<
                   "6 37 2"<<"\n"<<
                   "6 38 2"<<"\n"<<
                   "6 39 2"<<"\n"<<
                   "6 40 2"<<"\n"<<
                   "6 41 2"<<"\n"<<
                   "6 42 2"<<"\n"<<
                   "6 43 2"<<"\n"<<
                   "6 44 2"<<"\n"<<
                   "6 46 2"<<"\n"<<
                   "8 1 3"<<"\n"<<
                   "8 6 2"<<"\n"<<
                   "8 13 2"<<"\n"<<
                   "8 14 3"<<"\n"<<
                   "8 18 2"<<"\n"<<
                   "8 19 2"<<"\n"<<
                   "8 20 1"<<"\n"<<
                   "8 22 2"<<"\n"<<
                   "8 23 1"<<"\n"<<
                   "8 24 2"<<"\n"<<
                   "8 25 2"<<"\n"<<
                   "8 27 1"<<"\n"<<
                   "8 32 2"<<"\n"<<
                   "8 33 2"<<"\n"<<
                   "8 35 2"<<"\n"<<
                   "8 37 2"<<"\n"<<
                   "8 38 1"<<"\n"<<
                   "8 40 1"<<"\n"<<
                   "8 41 2"<<"\n"<<
                   "8 42 2"<<"\n"<<
                   "8 44 2"<<"\n"<<
                   "8 45 2"<<"\n"<<
                   "10 1 4"<<"\n"<<
                   "10 2 2"<<"\n"<<
                   "10 13 3"<<"\n"<<
                   "10 18 2"<<"\n"<<
                   "10 19 2"<<"\n"<<
                   "10 22 2"<<"\n"<<
                   "10 23 2"<<"\n"<<
                   "10 24 2"<<"\n"<<
                   "10 27 2"<<"\n"<<
                   "10 31 2"<<"\n"<<
                   "10 33 2"<<"\n"<<
                   "10 37 3"<<"\n"<<
                   "10 39 2"<<"\n"<<
                   "10 40 2"<<"\n"<<
                   "10 41 2"<<"\n"<<
                   "10 42 3"<<"\n"<<
                   "10 44 4"<<"\n"<<
                   "10 45 2"<<"\n"<<
                   "10 46 3"<<"\n"<<
                   "11 1 3"<<"\n"<<
                   "11 2 2"<<"\n"<<
                   "11 3 1"<<"\n"<<
                   "11 13 2"<<"\n"<<
                   "11 14 2"<<"\n"<<
                   "11 19 1"<<"\n"<<
                   "11 21 3"<<"\n"<<
                   "11 41 2"<<"\n"<<
                   "13 1 3"<<"\n"<<
                   "13 2 2"<<"\n"<<
                   "13 3 2"<<"\n"<<
                   "13 6 2"<<"\n"<<
                   "13 8 2"<<"\n"<<
                   "13 10 2"<<"\n"<<
                   "13 11 1"<<"\n"<<
                   "13 14 1"<<"\n"<<
                   "13 18 2"<<"\n"<<
                   "13 19 4"<<"\n"<<
                   "13 20 1"<<"\n"<<
                   "13 21 2"<<"\n"<<
                   "13 22 2"<<"\n"<<
                   "13 23 2"<<"\n"<<
                   "13 24 2"<<"\n"<<
                   "13 25 2"<<"\n"<<
                   "13 26 2"<<"\n"<<
                   "13 27 2"<<"\n"<<
                   "13 32 2"<<"\n"<<
                   "13 33 2"<<"\n"<<
                   "13 35 2"<<"\n"<<
                   "13 36 2"<<"\n"<<
                   "13 37 2"<<"\n"<<
                   "13 38 2"<<"\n"<<
                   "13 40 2"<<"\n"<<
                   "13 41 1"<<"\n"<<
                   "13 42 2"<<"\n"<<
                   "13 43 2"<<"\n"<<
                   "13 44 2"<<"\n"<<
                   "13 45 4"<<"\n"<<
                   "13 46 2"<<"\n"<<
                   "14 1 3"<<"\n"<<
                   "14 2 4"<<"\n"<<
                   "14 8 2"<<"\n"<<
                   "14 13 2"<<"\n"<<
                   "14 19 2"<<"\n"<<
                   "14 21 2"<<"\n"<<
                   "14 22 1"<<"\n"<<
                   "14 24 1"<<"\n"<<
                   "14 25 2"<<"\n"<<
                   "14 33 2"<<"\n"<<
                   "14 35 2"<<"\n"<<
                   "14 40 3"<<"\n"<<
                   "14 42 1"<<"\n"<<
                   "14 44 2"<<"\n"<<
                   "14 45 4"<<"\n"<<
                   "14 46 2"<<"\n"<<
                   "18 1 3"<<"\n"<<
                   "18 3 2"<<"\n"<<
                   "18 6 3"<<"\n"<<
                   "18 8 2"<<"\n"<<
                   "18 11 1"<<"\n"<<
                   "18 13 2"<<"\n"<<
                   "18 14 1"<<"\n"<<
                   "18 19 2"<<"\n"<<
                   "18 20 3"<<"\n"<<
                   "18 21 2"<<"\n"<<
                   "18 22 1"<<"\n"<<
                   "18 23 2"<<"\n"<<
                   "18 24 2"<<"\n"<<
                   "18 25 2"<<"\n"<<
                   "18 26 2"<<"\n"<<
                   "18 27 2"<<"\n"<<
                   "18 31 2"<<"\n"<<
                   "18 32 4"<<"\n"<<
                   "18 33 2"<<"\n"<<
                   "18 35 2"<<"\n"<<
                   "18 36 4"<<"\n"<<
                   "18 37 2"<<"\n"<<
                   "18 38 2"<<"\n"<<
                   "18 40 2"<<"\n"<<
                   "18 41 2"<<"\n"<<
                   "18 42 3"<<"\n"<<
                   "18 43 2"<<"\n"<<
                   "18 44 2"<<"\n"<<
                   "18 45 1"<<"\n"<<
                   "19 1 3"<<"\n"<<
                   "19 2 2"<<"\n"<<
                   "19 3 2"<<"\n"<<
                   "19 6 2"<<"\n"<<
                   "19 8 2"<<"\n"<<
                   "19 10 2"<<"\n"<<
                   "19 13 4"<<"\n"<<
                   "19 14 2"<<"\n"<<
                   "19 18 2"<<"\n"<<
                   "19 21 2"<<"\n"<<
                   "19 22 2"<<"\n"<<
                   "19 23 2"<<"\n"<<
                   "19 24 2"<<"\n"<<
                   "19 25 2"<<"\n"<<
                   "19 26 2"<<"\n"<<
                   "19 27 2"<<"\n"<<
                   "19 31 2"<<"\n"<<
                   "19 32 2"<<"\n"<<
                   "19 33 2"<<"\n"<<
                   "19 35 2"<<"\n"<<
                   "19 36 1"<<"\n"<<
                   "19 37 2"<<"\n"<<
                   "19 38 2"<<"\n"<<
                   "19 40 2"<<"\n"<<
                   "19 41 2"<<"\n"<<
                   "19 42 2"<<"\n"<<
                   "19 44 3"<<"\n"<<
                   "19 45 3"<<"\n"<<
                   "19 46 2"<<"\n"<<
                   "20 1 2"<<"\n"<<
                   "20 3 1"<<"\n"<<
                   "20 6 2"<<"\n"<<
                   "20 11 1"<<"\n"<<
                   "20 13 1"<<"\n"<<
                   "20 18 3"<<"\n"<<
                   "20 22 2"<<"\n"<<
                   "20 23 1"<<"\n"<<
                   "20 24 1"<<"\n"<<
                   "20 27 2"<<"\n"<<
                   "20 31 2"<<"\n"<<
                   "20 32 3"<<"\n"<<
                   "20 33 2"<<"\n"<<
                   "20 35 1"<<"\n"<<
                   "20 36 1"<<"\n"<<
                   "20 37 1"<<"\n"<<
                   "20 38 2"<<"\n"<<
                   "20 41 1"<<"\n"<<
                   "20 43 1"<<"\n"<<
                   "20 44 2"<<"\n"<<
                   "20 45 2"<<"\n"<<
                   "20 46 2"<<"\n"<<
                   "21 1 3"<<"\n"<<
                   "21 2 3"<<"\n"<<
                   "21 3 1"<<"\n"<<
                   "21 6 2"<<"\n"<<
                   "21 8 1"<<"\n"<<
                   "21 11 3"<<"\n"<<
                   "21 13 3"<<"\n"<<
                   "21 14 2"<<"\n"<<
                   "21 18 1"<<"\n"<<
                   "21 19 2"<<"\n"<<
                   "21 22 1"<<"\n"<<
                   "21 23 1"<<"\n"<<
                   "21 24 2"<<"\n"<<
                   "21 26 2"<<"\n"<<
                   "21 27 2"<<"\n"<<
                   "21 31 1"<<"\n"<<
                   "21 32 1"<<"\n"<<
                   "21 33 2"<<"\n"<<
                   "21 35 2"<<"\n"<<
                   "21 36 1"<<"\n"<<
                   "21 39 2"<<"\n"<<
                   "21 40 4"<<"\n"<<
                   "21 41 2"<<"\n"<<
                   "21 42 2"<<"\n"<<
                   "21 43 2"<<"\n"<<
                   "21 44 3"<<"\n"<<
                   "21 45 3"<<"\n"<<
                   "22 1 3"<<"\n"<<
                   "22 2 2"<<"\n"<<
                   "22 3 4"<<"\n"<<
                   "22 6 3"<<"\n"<<
                   "22 8 3"<<"\n"<<
                   "22 13 3"<<"\n"<<
                   "22 18 2"<<"\n"<<
                   "22 19 2"<<"\n"<<
                   "22 20 3"<<"\n"<<
                   "22 21 2"<<"\n"<<
                   "22 23 3"<<"\n"<<
                   "22 24 4"<<"\n"<<
                   "22 25 4"<<"\n"<<
                   "22 26 2"<<"\n"<<
                   "22 27 3"<<"\n"<<
                   "22 31 3"<<"\n"<<
                   "22 32 3"<<"\n"<<
                   "22 33 3"<<"\n"<<
                   "22 35 4"<<"\n"<<
                   "22 36 3"<<"\n"<<
                   "22 37 3"<<"\n"<<
                   "22 38 3"<<"\n"<<
                   "22 39 2"<<"\n"<<
                   "22 40 2"<<"\n"<<
                   "22 41 3"<<"\n"<<
                   "22 42 4"<<"\n"<<
                   "22 43 3"<<"\n"<<
                   "22 44 3"<<"\n"<<
                   "22 45 2"<<"\n"<<
                   "22 46 2"<<"\n"<<
                   "23 1 3"<<"\n"<<
                   "23 2 2"<<"\n"<<
                   "23 3 2"<<"\n"<<
                   "23 6 3"<<"\n"<<
                   "23 8 1"<<"\n"<<
                   "23 13 2"<<"\n"<<
                   "23 14 2"<<"\n"<<
                   "23 18 2"<<"\n"<<
                   "23 19 2"<<"\n"<<
                   "23 20 2"<<"\n"<<
                   "23 22 3"<<"\n"<<
                   "23 24 2"<<"\n"<<
                   "23 25 2"<<"\n"<<
                   "23 27 2"<<"\n"<<
                   "23 31 4"<<"\n"<<
                   "23 32 1"<<"\n"<<
                   "23 33 2"<<"\n"<<
                   "23 35 2"<<"\n"<<
                   "23 36 2"<<"\n"<<
                   "23 37 2"<<"\n"<<
                   "23 38 2"<<"\n"<<
                   "23 40 1"<<"\n"<<
                   "23 42 3"<<"\n"<<
                   "23 44 3"<<"\n"<<
                   "23 45 1"<<"\n"<<
                   "24 1 2"<<"\n"<<
                   "24 2 2"<<"\n"<<
                   "24 3 2"<<"\n"<<
                   "24 6 2"<<"\n"<<
                   "24 8 3"<<"\n"<<
                   "24 10 2"<<"\n"<<
                   "24 13 3"<<"\n"<<
                   "24 14 1"<<"\n"<<
                   "24 18 2"<<"\n"<<
                   "24 19 2"<<"\n"<<
                   "24 22 3"<<"\n"<<
                   "24 23 2"<<"\n"<<
                   "24 25 2"<<"\n"<<
                   "24 27 2"<<"\n"<<
                   "24 31 2"<<"\n"<<
                   "24 32 2"<<"\n"<<
                   "24 33 4"<<"\n"<<
                   "24 35 3"<<"\n"<<
                   "24 37 2"<<"\n"<<
                   "24 38 2"<<"\n"<<
                   "24 39 2"<<"\n"<<
                   "24 40 1"<<"\n"<<
                   "24 41 1"<<"\n"<<
                   "24 42 2"<<"\n"<<
                   "24 44 2"<<"\n"<<
                   "24 45 2"<<"\n"<<
                   "24 46 2"<<"\n"<<
                   "25 1 3"<<"\n"<<
                   "25 2 2"<<"\n"<<
                   "25 3 3"<<"\n"<<
                   "25 6 1"<<"\n"<<
                   "25 8 2"<<"\n"<<
                   "25 13 3"<<"\n"<<
                   "25 14 2"<<"\n"<<
                   "25 18 1"<<"\n"<<
                   "25 19 3"<<"\n"<<
                   "25 20 1"<<"\n"<<
                   "25 21 1"<<"\n"<<
                   "25 22 3"<<"\n"<<
                   "25 23 2"<<"\n"<<
                   "25 24 3"<<"\n"<<
                   "25 26 1"<<"\n"<<
                   "25 27 1"<<"\n"<<
                   "25 32 3"<<"\n"<<
                   "25 33 3"<<"\n"<<
                   "25 35 3"<<"\n"<<
                   "25 37 2"<<"\n"<<
                   "25 39 1"<<"\n"<<
                   "25 40 2"<<"\n"<<
                   "25 41 1"<<"\n"<<
                   "25 42 2"<<"\n"<<
                   "25 43 2"<<"\n"<<
                   "25 44 2"<<"\n"<<
                   "25 45 2"<<"\n"<<
                   "25 46 1"<<"\n"<<
                   "26 1 4"<<"\n"<<
                   "26 2 2"<<"\n"<<
                   "26 3 2"<<"\n"<<
                   "26 11 1"<<"\n"<<
                   "26 13 2"<<"\n"<<
                   "26 19 2"<<"\n"<<
                   "26 21 1"<<"\n"<<
                   "26 22 2"<<"\n"<<
                   "26 39 2"<<"\n"<<
                   "26 40 2"<<"\n"<<
                   "26 42 2"<<"\n"<<
                   "26 43 2"<<"\n"<<
                   "26 44 4"<<"\n"<<
                   "26 45 1"<<"\n"<<
                   "26 46 2"<<"\n"<<
                   "27 1 2"<<"\n"<<
                   "27 3 2"<<"\n"<<
                   "27 6 4"<<"\n"<<
                   "27 8 1"<<"\n"<<
                   "27 13 2"<<"\n"<<
                   "27 18 2"<<"\n"<<
                   "27 20 2"<<"\n"<<
                   "27 22 2"<<"\n"<<
                   "27 23 2"<<"\n"<<
                   "27 24 1"<<"\n"<<
                   "27 32 2"<<"\n"<<
                   "27 33 2"<<"\n"<<
                   "27 35 3"<<"\n"<<
                   "27 36 2"<<"\n"<<
                   "27 37 2"<<"\n"<<
                   "27 38 2"<<"\n"<<
                   "27 39 2"<<"\n"<<
                   "27 41 2"<<"\n"<<
                   "27 42 2"<<"\n"<<
                   "27 43 1"<<"\n"<<
                   "27 44 2"<<"\n"<<
                   "27 46 2"<<"\n"<<
                   "31 1 1"<<"\n"<<
                   "31 3 2"<<"\n"<<
                   "31 6 1"<<"\n"<<
                   "31 8 1"<<"\n"<<
                   "31 18 2"<<"\n"<<
                   "31 19 2"<<"\n"<<
                   "31 20 2"<<"\n"<<
                   "31 22 2"<<"\n"<<
                   "31 23 2"<<"\n"<<
                   "31 24 2"<<"\n"<<
                   "31 32 1"<<"\n"<<
                   "31 35 3"<<"\n"<<
                   "31 36 1"<<"\n"<<
                   "31 37 3"<<"\n"<<
                   "31 38 2"<<"\n"<<
                   "31 42 1"<<"\n"<<
                   "32 1 2"<<"\n"<<
                   "32 2 2"<<"\n"<<
                   "32 3 2"<<"\n"<<
                   "32 6 2"<<"\n"<<
                   "32 8 2"<<"\n"<<
                   "32 13 2"<<"\n"<<
                   "32 18 3"<<"\n"<<
                   "32 19 2"<<"\n"<<
                   "32 20 2"<<"\n"<<
                   "32 22 3"<<"\n"<<
                   "32 23 1"<<"\n"<<
                   "32 24 2"<<"\n"<<
                   "32 25 2"<<"\n"<<
                   "32 27 2"<<"\n"<<
                   "32 31 1"<<"\n"<<
                   "32 33 3"<<"\n"<<
                   "32 35 4"<<"\n"<<
                   "32 36 2"<<"\n"<<
                   "32 37 3"<<"\n"<<
                   "32 38 3"<<"\n"<<
                   "32 41 2"<<"\n"<<
                   "32 42 3"<<"\n"<<
                   "32 43 1"<<"\n"<<
                   "33 1 3"<<"\n"<<
                   "33 2 3"<<"\n"<<
                   "33 3 2"<<"\n"<<
                   "33 6 2"<<"\n"<<
                   "33 8 2"<<"\n"<<
                   "33 13 3"<<"\n"<<
                   "33 14 1"<<"\n"<<
                   "33 18 2"<<"\n"<<
                   "33 19 3"<<"\n"<<
                   "33 20 2"<<"\n"<<
                   "33 22 2"<<"\n"<<
                   "33 23 3"<<"\n"<<
                   "33 24 4"<<"\n"<<
                   "33 25 3"<<"\n"<<
                   "33 27 2"<<"\n"<<
                   "33 31 2"<<"\n"<<
                   "33 32 2"<<"\n"<<
                   "33 35 3"<<"\n"<<
                   "33 36 2"<<"\n"<<
                   "33 37 2"<<"\n"<<
                   "33 38 3"<<"\n"<<
                   "33 40 1"<<"\n"<<
                   "33 41 2"<<"\n"<<
                   "33 42 2"<<"\n"<<
                   "33 43 1"<<"\n"<<
                   "33 45 1"<<"\n"<<
                   "35 1 2"<<"\n"<<
                   "35 2 2"<<"\n"<<
                   "35 3 2"<<"\n"<<
                   "35 6 3"<<"\n"<<
                   "35 13 2"<<"\n"<<
                   "35 14 3"<<"\n"<<
                   "35 18 2"<<"\n"<<
                   "35 19 2"<<"\n"<<
                   "35 22 3"<<"\n"<<
                   "35 23 2"<<"\n"<<
                   "35 24 3"<<"\n"<<
                   "35 25 2"<<"\n"<<
                   "35 27 3"<<"\n"<<
                   "35 32 3"<<"\n"<<
                   "35 33 3"<<"\n"<<
                   "35 37 4"<<"\n"<<
                   "35 38 2"<<"\n"<<
                   "35 41 2"<<"\n"<<
                   "35 42 4"<<"\n"<<
                   "35 45 2"<<"\n"<<
                   "36 1 3"<<"\n"<<
                   "36 2 2"<<"\n"<<
                   "36 3 4"<<"\n"<<
                   "36 6 3"<<"\n"<<
                   "36 13 2"<<"\n"<<
                   "36 18 4"<<"\n"<<
                   "36 20 1"<<"\n"<<
                   "36 22 3"<<"\n"<<
                   "36 23 1"<<"\n"<<
                   "36 24 1"<<"\n"<<
                   "36 27 3"<<"\n"<<
                   "36 31 1"<<"\n"<<
                   "36 32 2"<<"\n"<<
                   "36 33 1"<<"\n"<<
                   "36 35 1"<<"\n"<<
                   "36 37 2"<<"\n"<<
                   "36 38 2"<<"\n"<<
                   "36 41 2"<<"\n"<<
                   "36 42 3"<<"\n"<<
                   "36 43 2"<<"\n"<<
                   "36 44 2"<<"\n"<<
                   "36 46 2"<<"\n"<<
                   "37 1 3"<<"\n"<<
                   "37 2 2"<<"\n"<<
                   "37 3 2"<<"\n"<<
                   "37 6 2"<<"\n"<<
                   "37 8 3"<<"\n"<<
                   "37 10 2"<<"\n"<<
                   "37 13 3"<<"\n"<<
                   "37 14 2"<<"\n"<<
                   "37 18 2"<<"\n"<<
                   "37 19 3"<<"\n"<<
                   "37 20 2"<<"\n"<<
                   "37 21 2"<<"\n"<<
                   "37 22 3"<<"\n"<<
                   "37 23 2"<<"\n"<<
                   "37 24 3"<<"\n"<<
                   "37 25 2"<<"\n"<<
                   "37 26 2"<<"\n"<<
                   "37 27 2"<<"\n"<<
                   "37 31 4"<<"\n"<<
                   "37 32 3"<<"\n"<<
                   "37 33 3"<<"\n"<<
                   "37 35 4"<<"\n"<<
                   "37 36 2"<<"\n"<<
                   "37 38 3"<<"\n"<<
                   "37 40 2"<<"\n"<<
                   "37 41 3"<<"\n"<<
                   "37 42 3"<<"\n"<<
                   "37 43 2"<<"\n"<<
                   "37 44 2"<<"\n"<<
                   "37 45 2"<<"\n"<<
                   "37 46 2"<<"\n"<<
                   "38 1 2"<<"\n"<<
                   "38 2 2"<<"\n"<<
                   "38 3 2"<<"\n"<<
                   "38 6 3"<<"\n"<<
                   "38 8 1"<<"\n"<<
                   "38 13 3"<<"\n"<<
                   "38 18 3"<<"\n"<<
                   "38 19 2"<<"\n"<<
                   "38 20 2"<<"\n"<<
                   "38 22 3"<<"\n"<<
                   "38 23 2"<<"\n"<<
                   "38 24 3"<<"\n"<<
                   "38 27 2"<<"\n"<<
                   "38 31 3"<<"\n"<<
                   "38 32 3"<<"\n"<<
                   "38 33 3"<<"\n"<<
                   "38 35 3"<<"\n"<<
                   "38 36 3"<<"\n"<<
                   "38 37 3"<<"\n"<<
                   "38 41 1"<<"\n"<<
                   "38 42 2"<<"\n"<<
                   "39 1 4"<<"\n"<<
                   "39 2 1"<<"\n"<<
                   "39 3 2"<<"\n"<<
                   "39 6 1"<<"\n"<<
                   "39 8 1"<<"\n"<<
                   "39 11 1"<<"\n"<<
                   "39 13 1"<<"\n"<<
                   "39 18 1"<<"\n"<<
                   "39 19 1"<<"\n"<<
                   "39 20 1"<<"\n"<<
                   "39 21 2"<<"\n"<<
                   "39 22 2"<<"\n"<<
                   "39 23 1"<<"\n"<<
                   "39 24 1"<<"\n"<<
                   "39 26 3"<<"\n"<<
                   "39 27 2"<<"\n"<<
                   "39 32 1"<<"\n"<<
                   "39 33 1"<<"\n"<<
                   "39 35 2"<<"\n"<<
                   "39 36 1"<<"\n"<<
                   "39 37 2"<<"\n"<<
                   "39 38 1"<<"\n"<<
                   "39 41 2"<<"\n"<<
                   "39 42 2"<<"\n"<<
                   "39 44 3"<<"\n"<<
                   "39 46 1"<<"\n"<<
                   "40 1 3"<<"\n"<<
                   "40 2 2"<<"\n"<<
                   "40 3 2"<<"\n"<<
                   "40 6 2"<<"\n"<<
                   "40 8 2"<<"\n"<<
                   "40 10 2"<<"\n"<<
                   "40 13 3"<<"\n"<<
                   "40 14 3"<<"\n"<<
                   "40 18 2"<<"\n"<<
                   "40 19 2"<<"\n"<<
                   "40 21 4"<<"\n"<<
                   "40 22 1"<<"\n"<<
                   "40 23 2"<<"\n"<<
                   "40 24 2"<<"\n"<<
                   "40 25 2"<<"\n"<<
                   "40 26 2"<<"\n"<<
                   "40 27 2"<<"\n"<<
                   "40 32 1"<<"\n"<<
                   "40 33 2"<<"\n"<<
                   "40 35 2"<<"\n"<<
                   "40 36 1"<<"\n"<<
                   "40 37 2"<<"\n"<<
                   "40 42 2"<<"\n"<<
                   "40 43 2"<<"\n"<<
                   "40 44 2"<<"\n"<<
                   "40 45 2"<<"\n"<<
                   "40 46 2"<<"\n"<<
                   "41 1 3"<<"\n"<<
                   "41 2 2"<<"\n"<<
                   "41 6 3"<<"\n"<<
                   "41 13 2"<<"\n"<<
                   "41 18 1"<<"\n"<<
                   "41 19 1"<<"\n"<<
                   "41 21 2"<<"\n"<<
                   "41 22 2"<<"\n"<<
                   "41 23 2"<<"\n"<<
                   "41 24 2"<<"\n"<<
                   "41 27 3"<<"\n"<<
                   "41 32 2"<<"\n"<<
                   "41 33 2"<<"\n"<<
                   "41 35 3"<<"\n"<<
                   "41 37 2"<<"\n"<<
                   "41 38 1"<<"\n"<<
                   "41 39 2"<<"\n"<<
                   "41 40 1"<<"\n"<<
                   "41 42 2"<<"\n"<<
                   "41 44 2"<<"\n"<<
                   "41 45 2"<<"\n"<<
                   "41 46 2"<<"\n"<<
                   "42 1 3"<<"\n"<<
                   "42 2 2"<<"\n"<<
                   "42 3 2"<<"\n"<<
                   "42 6 3"<<"\n"<<
                   "42 8 2"<<"\n"<<
                   "42 10 2"<<"\n"<<
                   "42 13 3"<<"\n"<<
                   "42 18 3"<<"\n"<<
                   "42 19 2"<<"\n"<<
                   "42 20 3"<<"\n"<<
                   "42 21 2"<<"\n"<<
                   "42 22 4"<<"\n"<<
                   "42 23 3"<<"\n"<<
                   "42 24 2"<<"\n"<<
                   "42 25 2"<<"\n"<<
                   "42 26 2"<<"\n"<<
                   "42 27 2"<<"\n"<<
                   "42 32 3"<<"\n"<<
                   "42 33 2"<<"\n"<<
                   "42 35 4"<<"\n"<<
                   "42 36 2"<<"\n"<<
                   "42 37 4"<<"\n"<<
                   "42 39 2"<<"\n"<<
                   "42 40 2"<<"\n"<<
                   "42 41 2"<<"\n"<<
                   "42 43 2"<<"\n"<<
                   "42 44 3"<<"\n"<<
                   "42 45 2"<<"\n"<<
                   "42 46 4"<<"\n"<<
                   "43 1 3"<<"\n"<<
                   "43 2 3"<<"\n"<<
                   "43 3 1"<<"\n"<<
                   "43 6 2"<<"\n"<<
                   "43 10 2"<<"\n"<<
                   "43 13 3"<<"\n"<<
                   "43 18 2"<<"\n"<<
                   "43 19 2"<<"\n"<<
                   "43 21 2"<<"\n"<<
                   "43 22 2"<<"\n"<<
                   "43 24 2"<<"\n"<<
                   "43 25 2"<<"\n"<<
                   "43 26 2"<<"\n"<<
                   "43 27 2"<<"\n"<<
                   "43 32 2"<<"\n"<<
                   "43 33 2"<<"\n"<<
                   "43 35 2"<<"\n"<<
                   "43 37 2"<<"\n"<<
                   "43 38 2"<<"\n"<<
                   "43 40 3"<<"\n"<<
                   "43 41 2"<<"\n"<<
                   "43 42 3"<<"\n"<<
                   "43 44 3"<<"\n"<<
                   "43 45 3"<<"\n"<<
                   "43 46 2"<<"\n"<<
                   "44 1 4"<<"\n"<<
                   "44 2 4"<<"\n"<<
                   "44 3 2"<<"\n"<<
                   "44 6 2"<<"\n"<<
                   "44 8 2"<<"\n"<<
                   "44 10 3"<<"\n"<<
                   "44 11 2"<<"\n"<<
                   "44 13 2"<<"\n"<<
                   "44 14 2"<<"\n"<<
                   "44 18 2"<<"\n"<<
                   "44 19 3"<<"\n"<<
                   "44 20 2"<<"\n"<<
                   "44 21 3"<<"\n"<<
                   "44 22 2"<<"\n"<<
                   "44 23 3"<<"\n"<<
                   "44 24 2"<<"\n"<<
                   "44 25 2"<<"\n"<<
                   "44 26 3"<<"\n"<<
                   "44 27 2"<<"\n"<<
                   "44 31 2"<<"\n"<<
                   "44 32 2"<<"\n"<<
                   "44 33 2"<<"\n"<<
                   "44 35 2"<<"\n"<<
                   "44 36 2"<<"\n"<<
                   "44 37 2"<<"\n"<<
                   "44 38 2"<<"\n"<<
                   "44 39 2"<<"\n"<<
                   "44 40 2"<<"\n"<<
                   "44 41 2"<<"\n"<<
                   "44 42 3"<<"\n"<<
                   "44 43 2"<<"\n"<<
                   "44 45 4"<<"\n"<<
                   "44 46 3"<<"\n"<<
                   "45 1 4"<<"\n"<<
                   "45 2 4"<<"\n"<<
                   "45 6 2"<<"\n"<<
                   "45 8 2"<<"\n"<<
                   "45 10 2"<<"\n"<<
                   "45 13 4"<<"\n"<<
                   "45 14 4"<<"\n"<<
                   "45 18 2"<<"\n"<<
                   "45 19 3"<<"\n"<<
                   "45 21 2"<<"\n"<<
                   "45 22 1"<<"\n"<<
                   "45 24 3"<<"\n"<<
                   "45 25 2"<<"\n"<<
                   "45 32 1"<<"\n"<<
                   "45 33 2"<<"\n"<<
                   "45 35 3"<<"\n"<<
                   "45 36 1"<<"\n"<<
                   "45 37 1"<<"\n"<<
                   "45 39 2"<<"\n"<<
                   "45 40 2"<<"\n"<<
                   "45 41 1"<<"\n"<<
                   "45 42 3"<<"\n"<<
                   "45 43 2"<<"\n"<<
                   "45 44 4"<<"\n"<<
                   "45 46 3"<<"\n"<<
                   "46 1 3"<<"\n"<<
                   "46 2 2"<<"\n"<<
                   "46 6 1"<<"\n"<<
                   "46 8 1"<<"\n"<<
                   "46 10 2"<<"\n"<<
                   "46 13 3"<<"\n"<<
                   "46 14 2"<<"\n"<<
                   "46 18 2"<<"\n"<<
                   "46 19 2"<<"\n"<<
                   "46 24 2"<<"\n"<<
                   "46 25 2"<<"\n"<<
                   "46 27 1"<<"\n"<<
                   "46 37 2"<<"\n"<<
                   "46 42 4"<<"\n"<<
                   "46 44 3"<<"\n"<<
                   "46 45 3";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Messages.dl" ) {
        datasetDescription = tr("Freeman's EIES network (Messages)");
        qDebug()<< "		... to  " << fileName;
        outText <<
                   "dl"<<"\n"<<
                   "N=32"<<"\n"<<
                   "format=edgelist1"<<"\n"<<
                   "data:"<<"\n"<<
                   "1 1 24"<<"\n"<<
                   "1 2 488"<<"\n"<<
                   "1 3 28"<<"\n"<<
                   "1 4 65"<<"\n"<<
                   "1 5 20"<<"\n"<<
                   "1 6 65"<<"\n"<<
                   "1 7 45"<<"\n"<<
                   "1 8 346"<<"\n"<<
                   "1 9 82"<<"\n"<<
                   "1 10 52"<<"\n"<<
                   "1 11 177"<<"\n"<<
                   "1 12 28"<<"\n"<<
                   "1 13 24"<<"\n"<<
                   "1 14 49"<<"\n"<<
                   "1 15 81"<<"\n"<<
                   "1 16 77"<<"\n"<<
                   "1 17 77"<<"\n"<<
                   "1 18 73"<<"\n"<<
                   "1 19 33"<<"\n"<<
                   "1 20 31"<<"\n"<<
                   "1 21 22"<<"\n"<<
                   "1 22 46"<<"\n"<<
                   "1 23 31"<<"\n"<<
                   "1 24 128"<<"\n"<<
                   "1 25 38"<<"\n"<<
                   "1 26 89"<<"\n"<<
                   "1 27 95"<<"\n"<<
                   "1 28 25"<<"\n"<<
                   "1 29 388"<<"\n"<<
                   "1 30 71"<<"\n"<<
                   "1 31 212"<<"\n"<<
                   "1 32 185"<<"\n"<<
                   "2 1 364"<<"\n"<<
                   "2 2 6"<<"\n"<<
                   "2 3 17"<<"\n"<<
                   "2 4 17"<<"\n"<<
                   "2 5 15"<<"\n"<<
                   "2 7 30"<<"\n"<<
                   "2 8 20"<<"\n"<<
                   "2 9 35"<<"\n"<<
                   "2 10 20"<<"\n"<<
                   "2 11 22"<<"\n"<<
                   "2 12 15"<<"\n"<<
                   "2 13 15"<<"\n"<<
                   "2 14 15"<<"\n"<<
                   "2 15 15"<<"\n"<<
                   "2 16 50"<<"\n"<<
                   "2 17 25"<<"\n"<<
                   "2 18 8"<<"\n"<<
                   "2 20 15"<<"\n"<<
                   "2 21 15"<<"\n"<<
                   "2 22 15"<<"\n"<<
                   "2 23 15"<<"\n"<<
                   "2 25 15"<<"\n"<<
                   "2 26 15"<<"\n"<<
                   "2 27 10"<<"\n"<<
                   "2 28 24"<<"\n"<<
                   "2 29 89"<<"\n"<<
                   "2 30 23"<<"\n"<<
                   "2 31 163"<<"\n"<<
                   "2 32 39"<<"\n"<<
                   "3 1 4"<<"\n"<<
                   "3 2 5"<<"\n"<<
                   "3 8 5"<<"\n"<<
                   "4 1 52"<<"\n"<<
                   "4 2 30"<<"\n"<<
                   "4 4 4"<<"\n"<<
                   "4 6 2"<<"\n"<<
                   "4 8 32"<<"\n"<<
                   "4 9 21"<<"\n"<<
                   "4 10 34"<<"\n"<<
                   "4 11 9"<<"\n"<<
                   "4 16 5"<<"\n"<<
                   "4 17 4"<<"\n"<<
                   "4 18 2"<<"\n"<<
                   "4 19 35"<<"\n"<<
                   "4 24 12"<<"\n"<<
                   "4 27 12"<<"\n"<<
                   "4 28 5"<<"\n"<<
                   "4 29 20"<<"\n"<<
                   "4 30 4"<<"\n"<<
                   "4 31 19"<<"\n"<<
                   "4 32 33"<<"\n"<<
                   "5 1 26"<<"\n"<<
                   "5 2 4"<<"\n"<<
                   "5 3 4"<<"\n"<<
                   "5 4 4"<<"\n"<<
                   "5 6 4"<<"\n"<<
                   "5 7 8"<<"\n"<<
                   "5 8 4"<<"\n"<<
                   "5 9 4"<<"\n"<<
                   "5 10 4"<<"\n"<<
                   "5 11 4"<<"\n"<<
                   "5 12 4"<<"\n"<<
                   "5 13 4"<<"\n"<<
                   "5 14 4"<<"\n"<<
                   "5 15 4"<<"\n"<<
                   "5 16 4"<<"\n"<<
                   "5 17 4"<<"\n"<<
                   "5 18 4"<<"\n"<<
                   "5 19 4"<<"\n"<<
                   "5 21 4"<<"\n"<<
                   "5 22 8"<<"\n"<<
                   "5 23 4"<<"\n"<<
                   "5 24 14"<<"\n"<<
                   "5 25 4"<<"\n"<<
                   "5 27 4"<<"\n"<<
                   "5 29 4"<<"\n"<<
                   "5 30 7"<<"\n"<<
                   "5 31 4"<<"\n"<<
                   "5 32 4"<<"\n"<<
                   "6 1 72"<<"\n"<<
                   "6 2 23"<<"\n"<<
                   "6 4 2"<<"\n"<<
                   "6 6 34"<<"\n"<<
                   "6 8 16"<<"\n"<<
                   "6 10 7"<<"\n"<<
                   "6 11 15"<<"\n"<<
                   "6 15 8"<<"\n"<<
                   "6 16 7"<<"\n"<<
                   "6 17 6"<<"\n"<<
                   "6 24 14"<<"\n"<<
                   "6 27 7"<<"\n"<<
                   "6 28 3"<<"\n"<<
                   "6 29 34"<<"\n"<<
                   "6 30 3"<<"\n"<<
                   "6 31 22"<<"\n"<<
                   "7 1 14"<<"\n"<<
                   "7 31 6"<<"\n"<<
                   "8 1 239"<<"\n"<<
                   "8 2 82"<<"\n"<<
                   "8 3 5"<<"\n"<<
                   "8 4 37"<<"\n"<<
                   "8 5 3"<<"\n"<<
                   "8 6 34"<<"\n"<<
                   "8 7 5"<<"\n"<<
                   "8 8 10"<<"\n"<<
                   "8 9 12"<<"\n"<<
                   "8 10 18"<<"\n"<<
                   "8 11 164"<<"\n"<<
                   "8 12 18"<<"\n"<<
                   "8 16 30"<<"\n"<<
                   "8 17 53"<<"\n"<<
                   "8 18 27"<<"\n"<<
                   "8 19 20"<<"\n"<<
                   "8 20 4"<<"\n"<<
                   "8 22 5"<<"\n"<<
                   "8 23 4"<<"\n"<<
                   "8 24 55"<<"\n"<<
                   "8 26 9"<<"\n"<<
                   "8 27 34"<<"\n"<<
                   "8 29 146"<<"\n"<<
                   "8 30 216"<<"\n"<<
                   "8 31 88"<<"\n"<<
                   "8 32 288"<<"\n"<<
                   "9 1 24"<<"\n"<<
                   "9 2 25"<<"\n"<<
                   "9 4 2"<<"\n"<<
                   "9 8 8"<<"\n"<<
                   "9 9 16"<<"\n"<<
                   "9 11 15"<<"\n"<<
                   "9 13 10"<<"\n"<<
                   "9 17 5"<<"\n"<<
                   "9 27 15"<<"\n"<<
                   "9 29 10"<<"\n"<<
                   "9 31 30"<<"\n"<<
                   "9 32 44"<<"\n"<<
                   "10 1 43"<<"\n"<<
                   "10 2 15"<<"\n"<<
                   "10 4 32"<<"\n"<<
                   "10 6 12"<<"\n"<<
                   "10 8 14"<<"\n"<<
                   "10 10 5"<<"\n"<<
                   "10 11 25"<<"\n"<<
                   "10 12 2"<<"\n"<<
                   "10 16 10"<<"\n"<<
                   "10 17 10"<<"\n"<<
                   "10 19 20"<<"\n"<<
                   "10 20 15"<<"\n"<<
                   "10 22 5"<<"\n"<<
                   "10 23 20"<<"\n"<<
                   "10 24 29"<<"\n"<<
                   "10 26 4"<<"\n"<<
                   "10 27 10"<<"\n"<<
                   "10 29 47"<<"\n"<<
                   "10 30 6"<<"\n"<<
                   "10 31 22"<<"\n"<<
                   "10 32 19"<<"\n"<<
                   "11 1 178"<<"\n"<<
                   "11 2 36"<<"\n"<<
                   "11 4 11"<<"\n"<<
                   "11 6 19"<<"\n"<<
                   "11 7 10"<<"\n"<<
                   "11 8 172"<<"\n"<<
                   "11 9 39"<<"\n"<<
                   "11 10 28"<<"\n"<<
                   "11 11 29"<<"\n"<<
                   "11 13 4"<<"\n"<<
                   "11 16 23"<<"\n"<<
                   "11 17 15"<<"\n"<<
                   "11 18 24"<<"\n"<<
                   "11 21 8"<<"\n"<<
                   "11 24 29"<<"\n"<<
                   "11 25 10"<<"\n"<<
                   "11 26 11"<<"\n"<<
                   "11 27 22"<<"\n"<<
                   "11 29 46"<<"\n"<<
                   "11 31 119"<<"\n"<<
                   "11 32 34"<<"\n"<<
                   "12 2 5"<<"\n"<<
                   "12 8 5"<<"\n"<<
                   "12 12 3"<<"\n"<<
                   "12 19 5"<<"\n"<<
                   "12 29 53"<<"\n"<<
                   "12 31 5"<<"\n"<<
                   "12 32 9"<<"\n"<<
                   "13 1 5"<<"\n"<<
                   "13 11 5"<<"\n"<<
                   "13 31 5"<<"\n"<<
                   "14 1 12"<<"\n"<<
                   "14 3 9"<<"\n"<<
                   "14 14 2"<<"\n"<<
                   "14 16 12"<<"\n"<<
                   "14 19 5"<<"\n"<<
                   "14 29 35"<<"\n"<<
                   "14 31 8"<<"\n"<<
                   "15 1 120"<<"\n"<<
                   "15 6 4"<<"\n"<<
                   "15 12 5"<<"\n"<<
                   "15 15 78"<<"\n"<<
                   "15 27 8"<<"\n"<<
                   "15 29 58"<<"\n"<<
                   "15 31 32"<<"\n"<<
                   "16 1 58"<<"\n"<<
                   "16 2 25"<<"\n"<<
                   "16 4 10"<<"\n"<<
                   "16 8 20"<<"\n"<<
                   "16 10 5"<<"\n"<<
                   "16 11 10"<<"\n"<<
                   "16 14 5"<<"\n"<<
                   "16 16 15"<<"\n"<<
                   "16 17 10"<<"\n"<<
                   "16 21 5"<<"\n"<<
                   "16 24 5"<<"\n"<<
                   "16 29 35"<<"\n"<<
                   "16 31 10"<<"\n"<<
                   "17 1 63"<<"\n"<<
                   "17 2 18"<<"\n"<<
                   "17 3 9"<<"\n"<<
                   "17 4 7"<<"\n"<<
                   "17 6 6"<<"\n"<<
                   "17 8 36"<<"\n"<<
                   "17 10 5"<<"\n"<<
                   "17 11 9"<<"\n"<<
                   "17 12 5"<<"\n"<<
                   "17 14 5"<<"\n"<<
                   "17 16 5"<<"\n"<<
                   "17 20 5"<<"\n"<<
                   "17 21 2"<<"\n"<<
                   "17 27 15"<<"\n"<<
                   "17 29 10"<<"\n"<<
                   "17 30 9"<<"\n"<<
                   "17 31 15"<<"\n"<<
                   "17 32 9"<<"\n"<<
                   "18 1 58"<<"\n"<<
                   "18 2 8"<<"\n"<<
                   "18 3 5"<<"\n"<<
                   "18 4 4"<<"\n"<<
                   "18 8 4"<<"\n"<<
                   "18 10 5"<<"\n"<<
                   "18 11 18"<<"\n"<<
                   "18 18 4"<<"\n"<<
                   "18 27 20"<<"\n"<<
                   "18 29 8"<<"\n"<<
                   "18 30 10"<<"\n"<<
                   "18 31 48"<<"\n"<<
                   "19 1 5"<<"\n"<<
                   "19 2 5"<<"\n"<<
                   "19 4 25"<<"\n"<<
                   "19 8 10"<<"\n"<<
                   "19 14 5"<<"\n"<<
                   "19 19 5"<<"\n"<<
                   "19 23 5"<<"\n"<<
                   "19 31 10"<<"\n"<<
                   "20 21 4"<<"\n"<<
                   "20 29 4"<<"\n"<<
                   "21 1 9"<<"\n"<<
                   "21 11 3"<<"\n"<<
                   "21 16 5"<<"\n"<<
                   "21 29 5"<<"\n"<<
                   "22 1 10"<<"\n"<<
                   "22 24 40"<<"\n"<<
                   "22 29 15"<<"\n"<<
                   "22 32 5"<<"\n"<<
                   "23 1 5"<<"\n"<<
                   "23 2 5"<<"\n"<<
                   "23 3 5"<<"\n"<<
                   "23 10 19"<<"\n"<<
                   "23 19 5"<<"\n"<<
                   "23 29 14"<<"\n"<<
                   "23 31 5"<<"\n"<<
                   "24 1 89"<<"\n"<<
                   "24 2 17"<<"\n"<<
                   "24 3 4"<<"\n"<<
                   "24 4 14"<<"\n"<<
                   "24 5 14"<<"\n"<<
                   "24 6 18"<<"\n"<<
                   "24 7 8"<<"\n"<<
                   "24 8 41"<<"\n"<<
                   "24 9 4"<<"\n"<<
                   "24 10 19"<<"\n"<<
                   "24 11 31"<<"\n"<<
                   "24 12 4"<<"\n"<<
                   "24 13 4"<<"\n"<<
                   "24 14 9"<<"\n"<<
                   "24 15 4"<<"\n"<<
                   "24 16 14"<<"\n"<<
                   "24 17 4"<<"\n"<<
                   "24 18 9"<<"\n"<<
                   "24 19 4"<<"\n"<<
                   "24 20 4"<<"\n"<<
                   "24 21 4"<<"\n"<<
                   "24 22 58"<<"\n"<<
                   "24 23 4"<<"\n"<<
                   "24 24 5"<<"\n"<<
                   "24 25 18"<<"\n"<<
                   "24 26 14"<<"\n"<<
                   "24 27 9"<<"\n"<<
                   "24 28 4"<<"\n"<<
                   "24 29 156"<<"\n"<<
                   "24 30 4"<<"\n"<<
                   "24 31 56"<<"\n"<<
                   "24 32 10"<<"\n"<<
                   "25 1 32"<<"\n"<<
                   "25 2 5"<<"\n"<<
                   "25 14 15"<<"\n"<<
                   "25 22 10"<<"\n"<<
                   "25 24 23"<<"\n"<<
                   "25 25 10"<<"\n"<<
                   "25 30 9"<<"\n"<<
                   "25 31 15"<<"\n"<<
                   "26 1 35"<<"\n"<<
                   "26 2 5"<<"\n"<<
                   "26 10 5"<<"\n"<<
                   "26 29 10"<<"\n"<<
                   "26 31 13"<<"\n"<<
                   "27 1 50"<<"\n"<<
                   "27 2 28"<<"\n"<<
                   "27 4 13"<<"\n"<<
                   "27 8 19"<<"\n"<<
                   "27 9 29"<<"\n"<<
                   "27 10 5"<<"\n"<<
                   "27 11 8"<<"\n"<<
                   "27 13 33"<<"\n"<<
                   "27 15 4"<<"\n"<<
                   "27 17 10"<<"\n"<<
                   "27 18 15"<<"\n"<<
                   "27 24 10"<<"\n"<<
                   "27 28 3"<<"\n"<<
                   "27 29 32"<<"\n"<<
                   "27 31 13"<<"\n"<<
                   "27 32 33"<<"\n"<<
                   "28 1 9"<<"\n"<<
                   "28 2 6"<<"\n"<<
                   "28 6 3"<<"\n"<<
                   "28 28 3"<<"\n"<<
                   "28 32 6"<<"\n"<<
                   "29 1 559"<<"\n"<<
                   "29 2 132"<<"\n"<<
                   "29 3 5"<<"\n"<<
                   "29 4 24"<<"\n"<<
                   "29 5 21"<<"\n"<<
                   "29 6 29"<<"\n"<<
                   "29 8 155"<<"\n"<<
                   "29 9 15"<<"\n"<<
                   "29 10 98"<<"\n"<<
                   "29 11 69"<<"\n"<<
                   "29 12 89"<<"\n"<<
                   "29 13 37"<<"\n"<<
                   "29 14 76"<<"\n"<<
                   "29 15 80"<<"\n"<<
                   "29 16 63"<<"\n"<<
                   "29 17 15"<<"\n"<<
                   "29 18 4"<<"\n"<<
                   "29 19 9"<<"\n"<<
                   "29 20 18"<<"\n"<<
                   "29 21 43"<<"\n"<<
                   "29 22 108"<<"\n"<<
                   "29 23 29"<<"\n"<<
                   "29 24 218"<<"\n"<<
                   "29 26 15"<<"\n"<<
                   "29 27 66"<<"\n"<<
                   "29 29 6"<<"\n"<<
                   "29 30 14"<<"\n"<<
                   "29 31 91"<<"\n"<<
                   "29 32 126"<<"\n"<<
                   "30 1 39"<<"\n"<<
                   "30 2 21"<<"\n"<<
                   "30 4 6"<<"\n"<<
                   "30 5 3"<<"\n"<<
                   "30 6 3"<<"\n"<<
                   "30 8 140"<<"\n"<<
                   "30 10 7"<<"\n"<<
                   "30 12 2"<<"\n"<<
                   "30 17 9"<<"\n"<<
                   "30 18 5"<<"\n"<<
                   "30 27 2"<<"\n"<<
                   "30 29 18"<<"\n"<<
                   "30 30 2"<<"\n"<<
                   "30 31 20"<<"\n"<<
                   "30 32 8"<<"\n"<<
                   "31 1 82"<<"\n"<<
                   "31 2 125"<<"\n"<<
                   "31 3 10"<<"\n"<<
                   "31 4 22"<<"\n"<<
                   "31 5 10"<<"\n"<<
                   "31 6 15"<<"\n"<<
                   "31 7 18"<<"\n"<<
                   "31 8 70"<<"\n"<<
                   "31 9 35"<<"\n"<<
                   "31 10 23"<<"\n"<<
                   "31 11 114"<<"\n"<<
                   "31 12 20"<<"\n"<<
                   "31 13 16"<<"\n"<<
                   "31 14 15"<<"\n"<<
                   "31 15 24"<<"\n"<<
                   "31 16 30"<<"\n"<<
                   "31 17 28"<<"\n"<<
                   "31 18 49"<<"\n"<<
                   "31 19 30"<<"\n"<<
                   "31 20 5"<<"\n"<<
                   "31 21 5"<<"\n"<<
                   "31 22 15"<<"\n"<<
                   "31 23 8"<<"\n"<<
                   "31 24 53"<<"\n"<<
                   "31 25 25"<<"\n"<<
                   "31 26 8"<<"\n"<<
                   "31 27 21"<<"\n"<<
                   "31 28 8"<<"\n"<<
                   "31 29 65"<<"\n"<<
                   "31 30 28"<<"\n"<<
                   "31 32 67"<<"\n"<<
                   "32 1 239"<<"\n"<<
                   "32 2 99"<<"\n"<<
                   "32 4 27"<<"\n"<<
                   "32 5 3"<<"\n"<<
                   "32 8 268"<<"\n"<<
                   "32 9 101"<<"\n"<<
                   "32 10 18"<<"\n"<<
                   "32 11 35"<<"\n"<<
                   "32 12 4"<<"\n"<<
                   "32 17 7"<<"\n"<<
                   "32 22 14"<<"\n"<<
                   "32 24 5"<<"\n"<<
                   "32 27 50"<<"\n"<<
                   "32 28 6"<<"\n"<<
                   "32 29 71"<<"\n"<<
                   "32 30 7"<<"\n"<<
                   "32 31 107"<<"\n"<<
                   "32 32 219";

    }
    else if (fileName=="Freeman_34_possible_graphs_with_N_5_multirelational.paj") {
        datasetDescription=tr("Freeman's 34 possible graphs of N=5\n\n"
                              "This data comes from Freeman's (1979) seminal paper "
                "\"Centrality in social networks\".\n"
                "It illustrates all 34 possible graphs of five nodes. \n"
                "Freeman used them to calculate and compare the three measures "
                 "of Centrality: Degree, Betweenness and Closeness. \n"
               "Use Relation buttons on the toolbar to move between the graphs.");
        outText<< "*Network \"34 possible graphs of N=5\"" << "\n" <<
                  "*Vertices 5" << "\n" <<
                  "1 \"1\" ic red		0.221583 	0.644042	circle" << "\n" <<
                  "2 \"2\" ic red		0.233094 	0.351433	circle" << "\n" <<
                  "3 \"3\" ic red		0.696403 	0.328808	circle" << "\n" <<
                  "4 \"4\" ic red		0.471942 	0.197587	circle" << "\n" <<
                  "5 \"5\" ic red		0.726619 	0.644042	circle" << "\n" <<
                  "*Matrix :1" << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :2" << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :3" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :4" << "\n" <<
                  "0 0 0 0 1 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 0 0 " << "\n" <<
                  "*Matrix :5" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 1 0 0 0" << "\n" <<
                  "*Matrix :6" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 0 1 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :7" << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "1 1 0 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :8" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :9 \"star\"" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 1 1 1 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "*Matrix :10 \"fork\"" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "*Matrix :11 \"chain\"" << "\n" <<
                  "0 1 0 0 1 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 0 0 " << "\n" <<
                  "*Matrix :12" << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "1 1 1 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :13" << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 0 1 " << "\n" <<
                  "1 1 0 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :14" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :15" << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "0 0 0 1 0 " << "\n" <<
                  "*Matrix :16" << "\n" <<
                  "0 1 0 0 0 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "0 1 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :17" << "\n" <<
                  "0 1 0 0 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 0 0 " << "\n" <<
                  "*Matrix :18" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :19" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "1 1 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :20" << "\n" <<
                  "0 1 0 0 1 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 1 0 0 " << "\n" <<
                  "*Matrix :21" << "\n" <<
                  "0 1 0 1 0 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 1 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "0 0 1 1 0 " << "\n" <<
                  "*Matrix :22" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 1 1 0 0" << "\n" <<
                  "*Matrix :23" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "1 1 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :24" << "\n" <<
                  "0 1 0 0 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "0 1 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 1 0 0" << "\n" <<
                  "*Matrix :25" << "\n" <<
                  "0 1 1 0 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "1 1 0 1 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 0 0 0 " << "\n" <<
                  "*Matrix :26 " << "\n" <<
                  "0 1 1 1 0 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "1 1 0 1 0 " << "\n" <<
                  "1 1 1 0 0 " << "\n" <<
                  "0 0 0 0 0 " << "\n" <<
                  "*Matrix :27" << "\n" <<
                  "0 1 0 1 1 " << "\n" <<
                  "1 0 0 1 0 " << "\n" <<
                  "0 0 0 1 1 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "*Matrix :28" << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 0 1 1 1 " << "\n" <<
                  "1 1 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "*Matrix :29" << "\n" <<
                  "0 1 1 0 1 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 1 1 0 0 " << "\n" <<
                  "*Matrix :30" << "\n" <<
                  "0 1 1 1 0 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "1 1 0 1 1 " << "\n" <<
                  "1 1 1 0 0 " << "\n" <<
                  "0 0 1 0 0 " << "\n" <<
                  "*Matrix :31" << "\n" <<
                  "0 1 0 1 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "0 1 0 1 1 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "1 0 1 1 0 " << "\n" <<
                  "*Matrix :32" << "\n" <<
                  "0 1 1 0 1 " << "\n" <<
                  "1 0 1 1 1 " << "\n" <<
                  "1 1 0 1 1 " << "\n" <<
                  "0 1 1 0 0 " << "\n" <<
                  "1 1 1 0 0 " << "\n" <<
                  "*Matrix :33" << "\n" <<
                  "0 1 1 1 1 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "1 0 0 1 1 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "1 1 1 1 0 " << "\n" <<
                  "*Matrix :34" << "\n" <<
                  "0 1 1 1 1 " << "\n" <<
                  "1 0 1 1 1 " << "\n" <<
                  "1 1 0 1 1 " << "\n" <<
                  "1 1 1 0 1 " << "\n" <<
                  "1 1 1 1 0 ";

    }
    else if (fileName == "Mexican_Power_Network_1940s.lst"){
        datasetDescription = tr("Mexican Power Network in the 1940s\n\n"
                                "");
        outText<< "18 8 10 23 21" << "\n" <<
                  "19 11 21" << "\n" <<
                  "29 5 9 10" << "\n" <<
                  "23 8 9 18 11" << "\n" <<
                  "4 7 6 8 20 5 21" << "\n" <<
                  "5 4 29 20 7 6 8 9 26 21" << "\n" <<
                  "6 5 7 4 20 21 8" << "\n" <<
                  "7 4 6 5 8 20 21" << "\n" <<
                  "9 5 8 23 29 20 21 11 10" << "\n" <<
                  "8 18 23 4 5 6 7 21 24 26 25 9 10 37 20" << "\n" <<
                  "10 18 29 8 11 9 20 25 26" << "\n" <<
                  "11 19 23 9 10 25 21 36" << "\n" <<
                  "20 4 5 6 7 8 9 10" << "\n" <<
                  "24 8 26" << "\n" <<
                  "26 5 8 24 10" << "\n" <<
                  "21 19 4 5 6 7 8 9 11 18" << "\n" <<
                  "36 37 11" << "\n" <<
                  "37 8 36" << "\n" <<
                  "25 10 11 8";
    }
    else if (fileName == "Knoke_Bureaucracies_Network.pajek"){
        datasetDescription=tr("Knoke Bureaucracies\n\n"
                              "In 1978, Knoke & Wood collected data from workers at 95 organizations in Indianapolis. "
                              "Respondents indicated with which other organizations their own organization had any "
                              "of 13 different types of relationships. \n"
                              "Knoke and Kuklinski (1982) selected a subset of 10 organizations and two relationships: "
                              "information exchange and money exchange.\n"
                              "This dataset is directed and not symmetric.\n"
                              "Information exchange is recorded in KNOKI relation while money exchange in KNOKM .");

        qDebug()<< "		Knocke_Bureacracies_Information_Exchange_Network.pajek written... ";
        outText<< "*Network knokbur " << "\n" <<
                  "*Vertices 10" << "\n" <<
                  "1 \"COUN\" 0.1000    0.5000    0.5000" << "\n" <<
                  "2 \"COMM\" 0.1764    0.2649    0.5000" << "\n" <<
                  "3 \"EDUC\" 0.3764    0.1196    0.5000" << "\n" <<
                  "4 \"INDU\" 0.6236    0.1196    0.5000" << "\n" <<
                  "5 \"MAYR\" 0.8236    0.2649    0.5000" << "\n" <<
                  "6 \"WRO \" 0.9000    0.5000    0.5000" << "\n" <<
                  "7 \"NEWS\" 0.8236    0.7351    0.5000" << "\n" <<
                  "8 \"UWAY\" 0.6236    0.8804    0.5000" << "\n" <<
                  "9 \"WELF\" 0.3764    0.8804    0.5000" << "\n" <<
                  "10 \"WEST\" 0.1764    0.7351    0.5000" << "\n" <<
                  "*Matrix :1 \"Information exchange\"" << "\n" <<
                  "0 1 0 0 1 0 1 0 1 0 " << "\n" <<
                  "1 0 1 1 1 0 1 1 1 0 " << "\n" <<
                  "0 1 0 1 1 1 1 0 0 1 " << "\n" <<
                  "1 1 0 0 1 0 1 0 0 0 " << "\n" <<
                  "1 1 1 1 0 0 1 1 1 1 " << "\n" <<
                  "0 0 1 0 0 0 1 0 1 0 " << "\n" <<
                  "0 1 0 1 1 0 0 0 0 0 " << "\n" <<
                  "1 1 0 1 1 0 1 0 1 0 " << "\n" <<
                  "0 1 0 0 1 0 1 0 0 0 " << "\n" <<
                  "1 1 1 0 1 0 1 0 0 0 " << "\n" <<
                  "*Matrix :2 \"Money exchange\"" << "\n" <<
                  "0 0 1 0 1 0 0 1 1 1 " << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 " << "\n" <<
                  "0 0 0 0 0 0 0 1 0 0 " << "\n" <<
                  "0 1 1 0 0 0 1 1 1 0 " << "\n" <<
                  "0 1 1 0 0 0 0 1 1 0 " << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 " << "\n" <<
                  "0 1 0 0 0 0 0 1 0 0 " << "\n" <<
                  "0 0 0 0 0 0 0 0 1 1 " << "\n" <<
                  "0 0 1 0 0 0 0 1 0 0 " << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 ";

        qDebug()<< "		Knocke_Bureacracies_Information_Exchange_Network.pajek written... ";
    }
    else if (fileName=="Stephenson&Zelen_40_AIDS_patients_sex_contact.paj"){
        qDebug()<<"Stephenson&Zelen_40_AIDS_patiens";
        datasetDescription=tr("Stephenson & Zelen's AIDS patients network (sex contact)\n\n"
                              "The data described by Auerbach et al. (1984) and Klovdahl (1985) consists of information on 40 homosexual men diagnosed with AIDS. "
                               "Initially, 19 men residing in the Los Angeles and Orange County area were interviewed about their previous sexual contacts. "
                               "This information led to the subsequent identification of an additional 21 sexual partners in San Francisco, New York and other parts of the United States. "
                               "All 40 homosexual men were linked to each other through sexual contact."
                            );

        outText << "*Network Stephenson&Zelen_40_AIDS_patients"<<"\n"<<
                   "*Vertices 40"<<"\n"<<
                   "1 \"1\" ic red		0.15899 	0.150442	circle"<<"\n"<<
                   "2 \"2\" ic red		0.178306 	0.210914	circle"<<"\n"<<
                   "3 \"3\" ic red		0.242199 	0.181416	circle"<<"\n"<<
                   "4 \"4\" ic red		0.31055 	0.182891	circle"<<"\n"<<
                   "5 \"5\" ic red		0.20951 	0.253687	circle"<<"\n"<<
                   "6 \"6\" ic red		0.132244 	0.29351	circle"<<"\n"<<
                   "7 \"7\" ic red		0.0846954 	0.327434	circle"<<"\n"<<
                   "8 \"8\" ic red		0.200594 	0.351032	circle"<<"\n"<<
                   "9 \"9\" ic red		0.170877 	0.412979	circle"<<"\n"<<
                   "10 \"10\" ic red		0.120357 	0.458702	circle"<<"\n"<<
                   "11 \"11\" ic red		0.283804 	0.292035	circle"<<"\n"<<
                   "12 \"12\" ic red		0.329866 	0.244838	circle"<<"\n"<<
                   "13 \"13\" ic red		0.389302 	0.210914	circle"<<"\n"<<
                   "14 \"14\" ic red		0.459138 	0.238938	circle"<<"\n"<<
                   "15 \"15\" ic red		0.497771 	0.294985	circle"<<"\n"<<
                   "16 \"16\" ic red		0.401189 	0.351032	circle"<<"\n"<<
                   "17 \"17\" ic red		0.280832 	0.349558	circle"<<"\n"<<
                   "18 \"18\" ic red		0.251114 	0.482301	circle"<<"\n"<<
                   "19 \"19\" ic red		0.344725 	0.547198	circle"<<"\n"<<
                   "20 \"20\" ic red		0.317979 	0.463127	circle"<<"\n"<<
                   "21 \"21\" ic red		0.401189 	0.449852	circle"<<"\n"<<
                   "22 \"22\" ic red		0.536404 	0.418879	circle"<<"\n"<<
                   "23 \"23\" ic red		0.63893 	0.355457	circle"<<"\n"<<
                   "24 \"24\" ic red		0.658247 	0.268437	circle"<<"\n"<<
                   "25 \"25\" ic red		0.676077 	0.443953	circle"<<"\n"<<
                   "26 \"26\" ic red		0.576523 	0.516224	circle"<<"\n"<<
                   "27 \"27\" ic red		0.468053 	0.511799	circle"<<"\n"<<
                   "28 \"28\" ic red		0.482912 	0.600295	circle"<<"\n"<<
                   "29 \"29\" ic red		0.482912 	0.675516	circle"<<"\n"<<
                   "30 \"30\" ic red		0.423477 	0.728614	circle"<<"\n"<<
                   "31 \"31\" ic red		0.592868 	0.646018	circle"<<"\n"<<
                   "32 \"32\" ic red		0.59584 	0.728614	circle"<<"\n"<<
                   "33 \"33\" ic red		0.594354 	0.792035	circle"<<"\n"<<
                   "34 \"34\" ic red		0.69688 	0.839233	circle"<<"\n"<<
                   "35 \"35\" ic red		0.805349 	0.889381	circle"<<"\n"<<
                   "36 \"36\" ic red		0.710253 	0.669617	circle"<<"\n"<<
                   "37 \"37\" ic red		0.787519 	0.70944	circle"<<"\n"<<
                   "38 \"38\" ic red		0.698366 	0.539823	circle"<<"\n"<<
                   "39 \"39\" ic red		0.808321 	0.466077	circle"<<"\n"<<
                   "40 \"40\" ic red		0.817236 	0.564897	circle"<<"\n"<<
                   "*Edges "<<"\n"<<
                   "1 2 1 c black"<<"\n"<<
                   "2 5 1 c black"<<"\n"<<
                   "3 5 1 c black"<<"\n"<<
                   "4 5 1 c black"<<"\n"<<
                   "5 6 1 c black"<<"\n"<<
                   "5 11 1 c black"<<"\n"<<
                   "7 8 1 c black"<<"\n"<<
                   "8 9 1 c black"<<"\n"<<
                   "8 11 1 c black"<<"\n"<<
                   "9 10 1 c black"<<"\n"<<
                   "11 16 1 c black"<<"\n"<<
                   "12 16 1 c black"<<"\n"<<
                   "13 14 1 c black"<<"\n"<<
                   "14 16 1 c black"<<"\n"<<
                   "15 16 1 c black"<<"\n"<<
                   "16 17 1 c black"<<"\n"<<
                   "16 20 1 c black"<<"\n"<<
                   "16 21 1 c black"<<"\n"<<
                   "16 22 1 c black"<<"\n"<<
                   "18 20 1 c black"<<"\n"<<
                   "19 20 1 c black"<<"\n"<<
                   "19 28 1 c black"<<"\n"<<
                   "22 23 1 c black"<<"\n"<<
                   "22 25 1 c black"<<"\n"<<
                   "22 26 1 c black"<<"\n"<<
                   "23 24 1 c black"<<"\n"<<
                   "26 27 1 c black"<<"\n"<<
                   "26 28 1 c black"<<"\n"<<
                   "26 31 1 c black"<<"\n"<<
                   "26 38 1 c black"<<"\n"<<
                   "28 29 1 c black"<<"\n"<<
                   "29 30 1 c black"<<"\n"<<
                   "31 32 1 c black"<<"\n"<<
                   "31 36 1 c black"<<"\n"<<
                   "32 33 1 c black"<<"\n"<<
                   "32 34 1 c black"<<"\n"<<
                   "33 34 1 c black"<<"\n"<<
                   "34 35 1 c black"<<"\n"<<
                   "36 37 1 c black"<<"\n"<<
                   "38 39 1 c black"<<"\n"<<
                   "38 40 1 c black";

    }
    else if (fileName == "Stephenson&Zelen_5actors_6edges_IC_test_dataset.paj"){
        qDebug() << "Stephenson&Zelen_5actors_6edges_IC_test_dataset.paj";
        outText<<"*Network Stephenson&Zelen_5_actors_6edges"<<"\n"<<
                 "*Vertices 5"<<"\n"<<
                 "1 \"1\" ic red		0.226804 	0.365782	circle"<<"\n"<<
                 "2 \"2\" ic red		0.745214 	0.365782	circle"<<"\n"<<
                 "3 \"3\" ic red		0.758468 	0.724189	circle"<<"\n"<<
                 "4 \"4\" ic red		0.226804 	0.724189	circle"<<"\n"<<
                 "5 \"5\" ic red		0.480118 	0.10472	circle"<<"\n"<<
                 "*Matrix :1 non-weighted"<<"\n"<<
                 "0 1 0 1 1 "<<"\n"<<
                 "1 0 1 0 1 "<<"\n"<<
                 "0 1 0 1 0 "<<"\n"<<
                 "1 0 1 0 0 "<<"\n"<<
                 "1 1 0 0 0"<<"\n"<<
                 "*Matrix :2 weighted"<<"\n"<<
                 "0 2 0 1 5 "<<"\n"<<
                 "2 0 1 0 5 "<<"\n"<<
                 "0 1 0 10 0 "<<"\n"<<
                 "1 0 10 0 0 "<<"\n"<<
                 "5 5 0 0 0 ";

    }

    else if (fileName == "Stephenson&Zelen_Dunbar&Dunbar_Gelada_baboon_colony_H22a_IC.paj"){
        datasetDescription=tr("Galada baboon colony network (H22a) \n\n"
                        "A network of the Galada baboon colony, as described by Dunbar and Dunbar (1975). This is the first set of observations (H22a) and was made on 12 baboons.\n\n"
                        "The lines connecting two points (baboons) represent nonagonistic interactions (generally grooming behavior) and the frequency of such interactions is recorded by the edge weight. "
                        "Data derived from Stephenson & Zelen seminal 1989 paper where they introduced Information Centrality."
                    );

        outText << "*Network Dunbar&Dunbar_Gelada_baboon_colony_H22a"<<"\n"<<
                   "*Vertices 12"<<"\n"<<
                   "1 \"Adult Female\" ic RGB729FCF		0.223061 	0.329258	circle"<<"\n"<<
                   "2 \"2 years Male\" ic RGB729FCF		0.212487 	0.530562	circle"<<"\n"<<
                   "3 \"Adult Female\" ic RGB729FCF		0.426989 	0.427873	circle"<<"\n"<<
                   "4 \"Adult Female\" ic RGB729FCF		0.341893 	0.414018	circle"<<"\n"<<
                   "5 \"Adult Male\" ic RGB729FCF		0.348943 	0.243684	circle"<<"\n"<<
                   "6 \"3 years Female\" ic RGB729FCF		0.475327 	0.271394	circle"<<"\n"<<
                   "7 \"3 years Male\" ic RGB729FCF		0.632931 	0.323553	circle"<<"\n"<<
                   "8 \"Adult Female\" ic RGB729FCF		0.63142 	0.444988	circle"<<"\n"<<
                   "9 \"1 year Male\" ic RGB729FCF		0.571501 	0.554197	circle"<<"\n"<<
                   "10 \"3 years Female\" ic RGB729FCF		0.486908 	0.604727	circle"<<"\n"<<
                   "11 \"2 years Female\" ic RGB729FCF		0.405337 	0.581092	circle"<<"\n"<<
                   "12 \"1 year Male\" ic RGB729FCF		0.331319 	0.550937	circle"<<"\n"<<
                   "*Arcs "<<"\n"<<
                   "*Edges "<<"\n"<<
                   "1 2 11 c #666666"<<"\n"<<
                   "1 4 5 c #666666"<<"\n"<<
                   "1 5 9 c #666666"<<"\n"<<
                   "1 11 2 c #666666"<<"\n"<<
                   "1 12 1 c #666666"<<"\n"<<
                   "3 4 30 c #666666"<<"\n"<<
                   "3 5 4 c #666666"<<"\n"<<
                   "3 6 3 c #666666"<<"\n"<<
                   "3 7 1 c #666666"<<"\n"<<
                   "3 10 8 c #666666"<<"\n"<<
                   "3 11 6 c #666666"<<"\n"<<
                   "3 12 3 c #666666"<<"\n"<<
                   "4 5 10 c #666666"<<"\n"<<
                   "4 6 2 c #666666"<<"\n"<<
                   "4 7 3 c #666666"<<"\n"<<
                   "4 10 1 c #666666"<<"\n"<<
                   "4 12 3 c #666666"<<"\n"<<
                   "5 6 20 c #666666"<<"\n"<<
                   "5 8 3 c #666666"<<"\n"<<
                   "6 10 5 c #666666"<<"\n"<<
                   "6 12 1 c #666666"<<"\n"<<
                   "7 8 17 c #666666"<<"\n"<<
                   "8 9 2 c #666666"<<"\n"<<
                   "8 10 7 c #666666";
    }
    else if (fileName=="Wasserman_Faust_7actors_star_circle_line_graphs.paj") {
        qDebug () << "Wasserman_Faust_7actors_star_circle_line_graphs.paj";
        datasetDescription=tr("Wasserman & Faust's 7 actors graphs\n\n" );
        outText<< "*Network 7actors-wasserman-test-net-all"<<"\n"<<
                  "*Vertices 7"<<"\n"<<
                  "1 \"1\" ic red         0.441826        0.426254        circle"<<"\n"<<
                  "2 \"2\" ic red         0.584683        0.19469 circle"<<"\n"<<
                  "3 \"3\" ic red         0.71134         0.417404        circle"<<"\n"<<
                  "4 \"4\" ic red         0.664212        0.687316        circle"<<"\n"<<
                  "5 \"5\" ic red         0.310751        0.70944 circle"<<"\n"<<
                  "6 \"6\" ic red         0.157585        0.427729        circle"<<"\n"<<
                  "7 \"7\" ic red         0.248895        0.193215        circle"<<"\n"<<
                  "*Matrix :1 star"<<"\n"<<
                  "0 1 1 1 1 1 1 "<<"\n"<<
                  "1 0 0 0 0 0 0 "<<"\n"<<
                  "1 0 0 0 0 0 0 "<<"\n"<<
                  "1 0 0 0 0 0 0 "<<"\n"<<
                  "1 0 0 0 0 0 0 "<<"\n"<<
                  "1 0 0 0 0 0 0 "<<"\n"<<
                  "1 0 0 0 0 0 0"<<"\n"<<
                  "*Matrix :2 circle"<<"\n"<<
                  "0 1 0 0 0 0 1 "<<"\n"<<
                  "1 0 1 0 0 0 0 "<<"\n"<<
                  "0 1 0 1 0 0 0 "<<"\n"<<
                  "0 0 1 0 1 0 0 "<<"\n"<<
                  "0 0 0 1 0 1 0 "<<"\n"<<
                  "0 0 0 0 1 0 1 "<<"\n"<<
                  "1 0 0 0 0 1 0 "<<"\n"<<
                  "*Matrix :3 line"<<"\n"<<
                  "0 1 1 0 0 0 0 "<<"\n"<<
                  "1 0 0 1 0 0 0 "<<"\n"<<
                  "1 0 0 0 1 0 0 "<<"\n"<<
                  "0 1 0 0 0 1 0 "<<"\n"<<
                  "0 0 1 0 0 0 1 "<<"\n"<<
                  "0 0 0 1 0 0 0 "<<"\n"<<
                  "0 0 0 0 1 0 0";
    }
    else if (fileName == "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek"){
        datasetDescription=tr("Wasserman & Faust's Countries Trade Data (manufactured goods)\n\n" );
        qDebug()<< "		Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek written... ";
        outText<< "*Network Countries_Trade_Basic_Manufactured_Goods" << "\n" <<
                  "*Vertices      24" << "\n" <<
                  "1 \"ALG\"     0.5408 0.0347" << "\n" <<
                  "2 \"ARG\"     0.9195 0.1080" << "\n" <<
                  "3 \"BRA\"     0.7626 0.4348" << "\n" <<
                  "4 \"CHI\"     0.5190 0.2900" << "\n" <<
                  "5 \"CZE\"     0.4734 0.5176" << "\n" <<
                  "6 \"ECU\"     0.9669 0.3401" << "\n" <<
                  "7 \"EGY\"     0.1749 0.9478" << "\n" <<
                  "8 \"ETH\"     0.4757 0.9701" << "\n" <<
                  "9 \"FIN\"     0.6789 0.5941" << "\n" <<
                  "10 \"HON\"     0.9499 0.6624" << "\n" <<
                  "11 \"IND\"     0.0638 0.2404" << "\n" <<
                  "12 \"ISR\"     0.6606 0.1142" << "\n" <<
                  "13 \"JAP\"     0.4718 0.4038" << "\n" <<
                  "14 \"LIB\"     0.9210 0.9313" << "\n" <<
                  "15 \"MAD\"     0.7077 0.9150" << "\n" <<
                  "16 \"NZ\"      0.0501 0.6893" << "\n" <<
                  "17 \"PAK\"     0.3653 0.3211" << "\n" <<
                  "18 \"SPA\"     0.6454 0.3687" << "\n" <<
                  "19 \"SWI\"     0.5480 0.7162" << "\n" <<
                  "20 \"SYR\"     0.2465 0.0501" << "\n" <<
                  "21 \"TAI\"     0.3805 0.6520" << "\n" <<
                  "22 \"UK\"      0.5921 0.4555" << "\n" <<
                  "23 \"US\"      0.5464 0.5983" << "\n" <<
                  "24 \"YUG\"     0.3576 0.4845" << "\n" <<
                  "*Matrix :3 \"ws6 - Basic manufactured goods\"" << "\n" <<
                  "0 0 0 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1" << "\n" <<
                  "1 0 1 1 0 1 0 0 1 0 1 1 1 0 0 0 1 1 1 0 1 0 1 0" << "\n" <<
                  "1 1 0 1 1 1 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1" << "\n" <<
                  "1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1" << "\n" <<
                  "1 1 1 1 0 1 1 1 1 1 1 0 1 1 0 1 1 1 1 1 1 1 1 1" << "\n" <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0" << "\n" <<
                  "0 0 0 0 1 0 0 1 1 0 0 0 1 0 0 0 0 1 1 0 0 1 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 0" << "\n" <<
                  "1 1 1 1 1 1 1 1 0 1 1 1 1 0 0 1 1 1 1 1 1 1 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0" << "\n" <<
                  "1 0 0 1 1 0 1 0 1 0 0 0 1 0 0 1 1 1 1 0 1 1 1 1" << "\n" <<
                  "0 1 0 0 0 0 0 1 1 0 0 0 1 0 0 1 0 1 1 0 1 1 1 1" << "\n" <<
                  "1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0" << "\n" <<
                  "1 0 0 1 0 0 1 0 0 0 1 0 1 0 0 0 1 1 0 0 1 1 1 1" << "\n" <<
                  "0 0 0 1 1 0 0 0 1 0 1 0 1 1 0 1 0 1 1 1 1 1 1 0" << "\n" <<
                  "1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1" << "\n" <<
                  "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1" << "\n" <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << "\n" <<
                  "0 0 1 1 0 0 0 0 1 0 1 1 1 0 0 1 1 1 1 1 0 1 1 1" << "\n" <<
                  "1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1" << "\n" <<
                  "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1" << "\n" <<
                  "1 1 0 1 1 0 1 1 1 0 1 1 1 0 0 1 1 1 1 1 1 1 1 0";
        qDebug()<< "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek written... ";
    }
    else if (fileName == "Petersen_Graph.paj") {
        qDebug()<< "		Petersen_Graph.paj written... ";
        datasetDescription=tr("This data set is just a famous non-planar mathematical graph, \n"
                              "named after Julius Petersen, who constructed it in 1898.\n"
                "The Petersen graph is undirected with 10 vertices and 15 edges \n"
                 "and the smallest bridgeless cubic graph with no three-edge-coloring.\n"
                "This small graph serves as a useful example and counterexample \n"
                              "for many problems in graph theory. ");

        outText<< "*Network petersen"<<"\n"<<
                  "*Vertices 10"<<"\n"<<
                  "1 \"blue\" ic RGB5555FF      0.301331  0.398259  circle"<<"\n"<<
                  "2 \"red\" ic red             0.474335  0.238302  circle"<<"\n"<<
                  "3 \"blue\" ic RGB5555FF      0.652082  0.407722  circle"<<"\n"<<
                  "4 \"green\" ic RGB00FF00     0.601418  0.681758  circle"<<"\n"<<
                  "5 \"red\" ic red             0.348936  0.677763  circle"<<"\n"<<
                  "6 \"green\" ic RGB00FF00     0.410646  0.581066  circle"<<"\n"<<
                  "7 \"red\" ic red             0.534221  0.583243  circle"<<"\n"<<
                  "8 \"red\" ic red             0.561787  0.437432  circle"<<"\n"<<
                  "9 \"blue\" ic RGB5555FF      0.475285  0.351469  circle"<<"\n"<<
                  "10 \"green\" ic RGB00FF00    0.38308   0.436344  circle"<<"\n"<<
                  "*Arcs "<<"\n"<<
                  "*Edges "<<"\n"<<
                  "1 2 1 c black"<<"\n"<<
                  "1 5 1 c black"<<"\n"<<
                  "1 10 1 c black"<<"\n"<<
                  "2 3 1 c black"<<"\n"<<
                  "2 9 1 c black"<<"\n"<<
                  "3 4 1 c black"<<"\n"<<
                  "3 8 1 c black"<<"\n"<<
                  "4 5 1 c black"<<"\n"<<
                  "4 7 1 c black"<<"\n"<<
                  "5 6 1 c black"<<"\n"<<
                  "6 8 1 c black"<<"\n"<<
                  "6 9 1 c black"<<"\n"<<
                  "7 9 1 c black"<<"\n"<<
                  "7 10 1 c black"<<"\n"<<
                  "8 10 1 c black";
    }
    file.close();
    if ( !datasetDescription.isEmpty() ) {
        emit signalDatasetDescription(datasetDescription);
    }

}







/**
    Writes the specified matrix of social network to file fn
*/
void Graph::writeMatrix (const QString &fn,
                         const int &matrix,
                         const bool &considerWeights,
                         const bool &inverseWeights,
                         const bool &dropIsolates,
                         const QString &varLocation,
                         const bool &simpler) {

    qDebug() << "Writing specified matrix:" << matrix << "to file:" << fn << " -- dropIsolates:"<< dropIsolates;

    QElapsedTimer computationTimer;
    computationTimer.start();

    Q_UNUSED(simpler);

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    bool inverseResult = false;
    int N = vertices();

    switch (matrix) {
    case MATRIX_ADJACENCY:
        createMatrixAdjacency();
        emit statusMessage ( tr("Adjacency recomputed. Writing Adjacency Matrix...") );
        break;
    case MATRIX_LAPLACIAN:
        emit statusMessage ( tr("Need to recompute Adjacency Matrix. Please wait...") );
        createMatrixAdjacency();
        emit statusMessage ( tr("Adjacency recomputed. Writing Laplacian Matrix...") );
        break;
    case MATRIX_DEGREE:
        emit statusMessage ( tr("Need to recompute Adjacency Matrix. Please wait...") );
        createMatrixAdjacency();
        emit statusMessage ( tr("Adjacency recomputed. Writing Degree Matrix...") );
        break;
    case MATRIX_DISTANCES:
        graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates);
        emit statusMessage ( tr("Distances recomputed. Writing Distances Matrix...") );
        break;
    case MATRIX_GEODESICS:
        graphMatrixShortestPathsCreate(considerWeights, inverseWeights, false);
        emit statusMessage ( tr("Distances recomputed. Writing Shortest Paths Matrix...") );
        break;
    case MATRIX_ADJACENCY_INVERSE:
        emit statusMessage ( tr("Computing Inverse Adjacency Matrix. Please wait...") );
        inverseResult = createMatrixAdjacencyInverse(QString("lu"));
        emit statusMessage ( tr("Inverse Adjacency Matrix computed. Writing Matrix...") );
        break;
    case MATRIX_REACHABILITY:
        createMatrixReachability();
        emit statusMessage ( tr("Writing Reachability Matrix...") );
        break;

    case MATRIX_ADJACENCY_TRANSPOSE:
        emit statusMessage ( tr("Need to recompute Adjacency Matrix. Please wait...") );
        createMatrixAdjacency();
        emit statusMessage ( tr("Adjacency recomputed. Writing Adjacency Matrix...") );
        break;
    case MATRIX_COCITATION:
        emit statusMessage ( tr("Need to recompute Adjacency Matrix. Please wait...") );
        createMatrixAdjacency();
        emit statusMessage ( tr("Adjacency recomputed. Writing Adjacency Matrix...") );
        break;
    case MATRIX_DISTANCES_HAMMING:
    case MATRIX_DISTANCES_JACCARD:
    case MATRIX_DISTANCES_MANHATTAN:
    case MATRIX_DISTANCES_EUCLIDEAN:
        emit statusMessage ( tr("Need to recompute tie profile distances. Please wait...") );
        createMatrixAdjacency();
        emit statusMessage ( tr("Tie profile distances recomputed. Writing matrix...") );
        break;


    default:
        break;
    }


    QTextStream outText( &file );

    outText << htmlHead;

    outText << "<h1>";

    switch (matrix) {
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
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    switch (matrix) {
    case MATRIX_ADJACENCY:
        outText << "<p class=\"description\">"
                << tr("The adjacency matrix, AM, of a social network is a NxN matrix ")
                << tr("where each element (i,j) is the value of the edge from "
                      "actor i to actor j, or 0 if no edge exists.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText, AM , true,false,false);
        //AM.printHTMLTable(outText,true);
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
        //AM.laplacianMatrix().printHTMLTable(outText,true,false,false);
        writeMatrixHTMLTable(outText, AM.laplacianMatrix() , true,false,false);
        break;
    case MATRIX_DEGREE:
        outText << "<p class=\"description\">"
                << tr("The degree matrix D of a social network is a NxN matrix ")
                << tr("where each element (i,i) is the degree of actor i "
                      "and all other elements are zero.")
                << "<br />"
                << "</p>";
        //AM.degreeMatrix().printHTMLTable(outText, true);
        writeMatrixHTMLTable(outText, AM.degreeMatrix() , true,false,false);
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
        //DM.printHTMLTable(outText,true);
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
        //SIGMA.printHTMLTable(outText,true);
        break;

    case MATRIX_ADJACENCY_INVERSE:
        if (!inverseResult) {
            outText << "<p class=\"description\">"
                    << tr("The adjacency matrix is singular.")
                    << "<br />"
                    << "</p>";
        }else {
            writeMatrixHTMLTable(outText, invAM, true);
            //invAM.printHTMLTable(outText,true);
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
        //XRM.printHTMLTable(outText,true);
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

        writeMatrixHTMLTable(outText, AM.transpose() , true);
        //AM.transpose().printHTMLTable(outText,true);
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
        writeMatrixHTMLTable(outText, AM.cocitationMatrix() , true);
        //AM.cocitationMatrix().printHTMLTable(outText,true);
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
                             AM.distancesMatrix(METRIC_EUCLIDEAN_DISTANCE, varLocation, false, true ),
                             true,false,false);
        //AM.distancesMatrix(METRIC_EUCLIDEAN_DISTANCE, varLocation, false, true ).printHTMLTable(outText,true);
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
                             AM.distancesMatrix(METRIC_HAMMING_DISTANCE, varLocation, false, true ),
                             true,false,false);
        //AM.distancesMatrix(METRIC_HAMMING_DISTANCE, varLocation, false, true ).printHTMLTable(outText,true);
        break;
    case MATRIX_DISTANCES_JACCARD:
        outText << "<p class=\"description\">"
                << tr("The Jaccard distances matrix is a "
                      "NxN matrix where each element (i,j) is the Jaccard distance"
                      "of the tie profiles between actors i and j.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_JACCARD_INDEX, "Rows", false, true ),
                             true,false,false);
        //AM.distancesMatrix(METRIC_JACCARD_INDEX, "Rows", false, true ).printHTMLTable(outText,true);

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
                             AM.distancesMatrix(METRIC_MANHATTAN_DISTANCE, varLocation, false, true ),
                             true,false,false);
        //AM.distancesMatrix(METRIC_MANHATTAN_DISTANCE, varLocation, false, true ).printHTMLTable(outText,true);
        break;
    case MATRIX_DISTANCES_CHEBYSHEV:
        outText << "<p class=\"description\">"
                << tr("The Chebyshev distances matrix is a "
                      "NxN matrix where each element (i,j) is the Chebyshev distance"
                      "of the tie profiles between actors i and j, namely the greatest of their differences.")
                << "<br />"
                << "</p>";
        writeMatrixHTMLTable(outText,
                             AM.distancesMatrix(METRIC_CHEBYSHEV_MAXIMUM, varLocation, false, true ),
                             true,false,false);
        //AM.distancesMatrix(METRIC_CHEBYSHEV_MAXIMUM, varLocation, false, true ).printHTMLTable(outText,true);
        break;

    default:
        break;
    }



    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Matrix report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;

    file.close();
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
void Graph::writeMatrixHTMLTable(QTextStream& outText,
                                 Matrix &M,
                                 const bool &markDiag,
                                 const bool &plain,
                                 const bool &printInfinity,
                                 const bool &dropIsolates) {

    Q_UNUSED(plain);

    qDebug () << "Graph::writeMatrixHTMLTable() -"
              << "markDiag" << markDiag
              << "plain" << plain
              << " dropIsolates " << dropIsolates;

    int rowCount=0, i=0, j=0;
    int N = vertices();
    qreal maxVal, minVal, element;
    bool hasRealNumbers=false;

    VList::const_iterator it, jt;

    QString pMsg = tr("Writing matrix to file. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N, pMsg );

    M.findMinMaxValues(minVal, maxVal, hasRealNumbers);

    outText <<  ( (hasRealNumbers) ? qSetRealNumberPrecision(3) : qSetRealNumberPrecision(0) ) ;

    qDebug () << "Graph::writeMatrixHTMLTable() - minVal" << minVal
              << "maxVal" << maxVal << "hasRealNumbers" << hasRealNumbers;

    outText << "<table  border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"stripes\">"
            << "<thead>"
            << "<tr>"
            << "<th>"
            << tr("<sub>Actor</sup>/<sup>Actor</sup>")
            << "</th>";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        if ( ! (*it)->isEnabled() || (dropIsolates && (*it)->isIsolated() ) ) {
            continue;
        }
        outText <<"<th>"
                << (*it)->number()
                << "</th>";
    }
    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        if ( ! (*it)->isEnabled() || (dropIsolates && (*it)->isIsolated() ) ) {
            continue;
        }

        rowCount++;

        emit signalProgressBoxUpdate(rowCount);

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">";

        outText <<"<td class=\"header\">"
               << (*it)->number()
               << "</td>";

        for (jt=m_graph.cbegin(); jt!=m_graph.cend(); ++jt){

            if ( ! (*jt)->isEnabled() || (dropIsolates && (*jt)->isIsolated() ) ) {
                continue;
            }
            outText << Qt::fixed << Qt::right;

            outText <<"<td" << ((markDiag && (*it)->number() ==(*jt)->number() )? " class=\"diag\">" : ">");

            element = M.item(i,j);

            qDebug () << "Graph::writeMatrixHTMLTable() - M(" <<i<<","<<j<<") =" <<  M.item(i,j);

            if ( ( element == RAND_MAX ) && printInfinity) {
                // print inf symbol instead of RAND_MAX (distances matrix).
                outText << infinity;
            }
            else {
                outText << element ;
            }


            outText << "</td>";
            j++;
        }
        outText <<"</tr>";
        i++;
        j=0;
    }
    outText << "</tbody></table>";



    outText << qSetFieldWidth(0) << "\n" ;


    outText << "<p>"
       << "<span class=\"info\">"
       << ("Values: ")
       <<"</span>"
       << ( (hasRealNumbers) ? ("real numbers (printed decimals 3)") : ("integers only" ) )
       << "<br />"
       << "<span class=\"info\">"
       << ("- Max value: ")
       <<"</span>"
       << ( ( maxVal==RAND_MAX ) ?
                ( (printInfinity) ? infinity : QString::number(maxVal) ) +
                " (=not connected nodes, in distance matrix)" : QString::number(maxVal) )
       << "<br />"
       << "<span class=\"info\">"
       << ("- Min value: ")
       <<"</span>"
       << ( ( minVal==RAND_MAX ) ?
                ( (printInfinity) ? infinity : QString::number(minVal) ) +
                + " (usually denotes unconnected nodes, in distance matrix)" : QString::number(minVal ) )
       << "</p>";



    emit signalProgressBoxKill();

}





/** 
    Exports the adjacency matrix to a given textstream
*/
void Graph::writeMatrixAdjacencyTo(QTextStream& os,
                                   const bool &saveEdgeWeights){
    qDebug("Graph: adjacencyMatrix(), writing matrix with %i vertices", vertices());
    VList::const_iterator it, it1;
    qreal weight=RAND_MAX;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight = edgeExists( (*it)->number(), (*it1)->number() )  ) !=0 ) {
                //os << static_cast<int> (weight) << " ";
                os << ((saveEdgeWeights) ? weight : 1 ) << " ";
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
void Graph::writeMatrixAdjacency (const QString fn,
                                  const bool &markDiag) {

    qDebug() << "Writing adjacency matrix to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }

    QTextStream outText( &file );

    int sum=0;
    qreal weight=0;
    int rowCount=0;
    int N = vertices();

    VList::const_iterator it, it1;

    QString pMsg = tr("Writing Adjacency Matrix to file. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N, pMsg );

    outText << htmlHead;

    outText << "<h1>";
    outText << tr("ADJACENCY MATRIX");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
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

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ) continue;
        outText <<"<th>"
                << (*it)->number()
                << "</th>";
    }
    outText << "</tr>"
            << "</thead>"
            << "<tbody>";

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        rowCount++;

        emit signalProgressBoxUpdate(rowCount);

        if ( ! (*it)->isEnabled() ) continue;

        outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">";

        outText <<"<td class=\"header\">"
               << (*it)->number()
               << "</td>";

        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

            if ( ! (*it1)->isEnabled() ) continue;

            outText <<"<td" << ((markDiag && (*it)->number() ==(*it1)->number() )? " class=\"diag\">" : ">");
            if ( (weight =  edgeExists ( (*it)->number(), (*it1)->number() )  )!=0 ) {
                sum++;
                outText << (weight);

            }
            else {
                outText << 0 ;

            }
            outText << "</td>";

        }
        outText <<"</tr>";

    }
    outText << "</tbody></table>";

    qDebug("Graph: Found a total of %i edge",sum);
    if ( sum != edgesEnabled() ) qDebug ("Error in edge count found!!!");
    else qDebug("Edge count OK!");


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Adjacency matrix report, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;


    file.close();

    emit signalProgressBoxKill();

}


/**
 * @brief Writes a visual representation of the adjacency matrix of the graph to the specified file
 *
 * The resulting matrix HAS NO spaces between elements.
 *
 * @param fn
 * @param simpler
 */
void Graph::writeMatrixAdjacencyPlot (const QString fn,
                                      const bool &simpler) {

    qDebug() << "Writing adjacency matrix plot to file:" << fn;

    QElapsedTimer computationTimer;
    computationTimer.start();

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    QTextStream outText( &file );

    VList::const_iterator it, it1;
    int sum=0;
    int rowCount=0;
    int N = vertices();
    qreal weight=0;
    int progressCounter = 0;
    QString pMsg = tr("Plotting Adjacency Matrix. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N, pMsg );

    if (!simpler) {
        outText << htmlHead;
    }
    else
        outText <<  htmlHeadLight;

    outText << "<h1>";
    outText << tr("ADJACENCY MATRIX PLOT");
    outText << "</h1>";

    outText << "<p>"
            << "<span class=\"info\">"
            << tr("Network name: ")
            <<"</span>"
            << getName()
            <<"<br />"
            << "<span class=\"info\">"
            << tr("Actors: ")
            <<"</span>"
            << N
            << "</p>";


    outText << "<p class=\"description\">"
            << tr("This a plot of the network's adjacency matrix, a NxN matrix ")
            << tr("where each element (i,j) is filled if there is an edge from "
                  "actor i to actor j, or not filled if no edge exists.")
            << "<br />"
            << "</p>";


    if (!simpler) {

        outText << "<table class=\"plot\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">";

        outText << "<tbody>";

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

            emit signalProgressBoxUpdate(++progressCounter);

            if ( ! (*it)->isEnabled() ) {
                continue;
            }

            rowCount++;

            outText << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">";

            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

                if ( ! (*it1)->isEnabled() ) continue;

                if ( (weight =  edgeExists ( (*it)->number(), (*it1)->number() )  )!=0 ) {
                    sum++;
                    outText <<"<td class=\"filled\">"
                           << QString("\xe2\x96\xa0")
                           << "</td>";
                }
                else {
                    outText <<"<td>"
                              //   << "&nbsp;&nbsp;"
                           << QString("\xe2\x96\xa1")
                           << "</td>";
                }

            }
            outText <<"</tr>";
        }
        outText << "</tbody></table>";
    }
    else {
        outText << "<p class=\"pre\">";
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

            emit signalProgressBoxUpdate(++progressCounter);

            if ( ! (*it)->isEnabled() ) {
                continue;
            }

            rowCount++;

            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

                if ( ! (*it1)->isEnabled() ) continue;

                if ( (weight =  edgeExists ( (*it)->number(), (*it1)->number() )  )!=0 ) {
                    sum++;
                    outText << QString("\xe2\x96\xa0") << " ";
                }
                else {
                    outText << QString("\xe2\x96\xa1") << " ";

                }

            }
            outText << "<br>"<<"\n";
        }
        outText << "</p>";
    }
    qDebug("Graph: Found a total of %i edge",sum);
    if ( sum != edgesEnabled() ) qDebug ("Error in edge count found!!!");
    else qDebug("Edge count OK!");


    outText << "<p>&nbsp;</p>";
    outText << "<p class=\"small\">";
    outText << tr("Adjacency matrix plot, <br />");
    outText << tr("Created by <a href=\"https://socnetv.org\" target=\"_blank\">Social Network Visualizer</a> v%1: %2")
               .arg(VERSION).arg( actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) ) ;
    outText << "<br />";
    outText << tr("Computation time: %1 msecs").arg( computationTimer.elapsed() );
    outText << "</p>";

    outText << htmlEnd;


    file.close();

    emit signalProgressBoxKill();

}




/**
 * @brief  Creates an adjacency matrix AM
 *  where AM(i,j)=1 if i is connected to j
 *  and AM(i,j)=0 if i not connected to j
 *  Used in Graph::centralityInformation(), Graph::graphWalksMatrixCreate
 *  and Graph::createMatrixAdjacencyInverse()
 * @param dropIsolates
 * @param considerWeights
 * @param inverseWeights
 * @param symmetrize
 */
void Graph::createMatrixAdjacency(const bool dropIsolates,
                                       const bool considerWeights,
                                       const bool inverseWeights,
                                       const bool symmetrize ){
    qDebug() << "Graph::createMatrixAdjacency() "
             << "dropIsolates" << dropIsolates
             << "considerWeights" << considerWeights
             << "inverseWeights" << inverseWeights
             << "symmetrize" << symmetrize;
    qreal m_weight=RAND_MAX;
    int i=0, j=0;
    int N = vertices(dropIsolates,false,true), progressCounter=0;
    VList::const_iterator it, jt;

    qDebug() << "Graph::createMatrixAdjacency() -resizing AM to"<< N;
    AM.resize(N, N);

    QString pMsg = tr ("Creating Adjacency Matrix. \nPlease wait...");
    emit statusMessage (pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        qDebug() << "Graph::createMatrixAdjacency() - i" << i << "name"<< (*it)->number();

        emit signalProgressBoxUpdate(++progressCounter);

        if ( ! (*it)->isEnabled() || ( (*it)->isIsolated() && dropIsolates) ) {
            qDebug() << "Graph::createMatrixAdjacency() - SKIP i" << i << "name"<< (*it)->number();
            continue;
        }

        j=i;

        for (jt=it; jt!=m_graph.end(); jt++){

            qDebug() << "Graph::createMatrixAdjacency() - j" << j << "name"<< (*jt)->number();

            if ( ! (*jt)->isEnabled() || ( (*jt)->isIsolated() && dropIsolates) ) {
                qDebug() << "Graph::createMatrixAdjacency() - SKIP j" << j << "name" << (*jt)->number();
                continue;
            }

            if ( (m_weight = edgeExists ( (*it)->number(), (*jt)->number() )  ) !=0 ) {
                if (!considerWeights) {
                    AM.setItem(i,j, 1 );
                }
                else {
                    if (inverseWeights)
                        AM.setItem(i,j, 1.0 / m_weight );
                    else
                        AM.setItem(i,j, m_weight );
                }
            }
            else{
                AM.setItem(i,j, 0);
            }

            qDebug()<<" AM("<< i << ","<< j << ") = " <<  AM.item(i,j);

            if (i != j ) {
                if ( (m_weight = edgeExists ( (*jt)->number(), (*it)->number() )  ) !=0 ) {
                    if (!considerWeights) {
                        AM.setItem(j,i, 1 );
                    }
                    else {
                        if (inverseWeights)
                            AM.setItem(j,i, 1.0 / m_weight );
                        else
                            AM.setItem(j,i, m_weight );
                    }
                    if (symmetrize && (AM.item(i,j) != AM.item(j,i)) ) {
                        AM.setItem(i,j, AM.item(j,i) );
                    }
                }
                else {
                    AM.setItem(j,i, 0);
                    if (symmetrize && (AM.item(i,j) != AM.item(j,i)) )
                        AM.setItem(j,i, AM.item(i,j) );

                }
                qDebug()<<" AM("<< j << ","<< i << ") = " <<  AM.item(j,i);
            }
            j++;
        }
        i++;
    }

    calculatedAdjacencyMatrix=true;

    emit signalProgressBoxKill();

}

/**
 * @brief Computes the inverse of the current adjacency matrix
 * @param method
 * @return
 */
bool Graph::createMatrixAdjacencyInverse(const QString &method){
    qDebug()<<"Graph::createMatrixAdjacencyInverse() ";

    bool considerWeights=false;
    int i=0, j=0;
    bool isSingular=true;

    bool dropIsolates=true; // always drop isolates else AM will be singular

    int  N = vertices(dropIsolates, false, true);

    createMatrixAdjacency(dropIsolates, considerWeights);

    invAM.resize(N,N);

    if ( method == "gauss") {
        invAM.inverseByGaussJordanElimination(AM);
    }
    else {
        invAM.inverse(AM);
    }

    VList::const_iterator it, it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() || (*it)->isIsolated())
            continue;
        j=0;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() || (*it)->isIsolated() )
                continue;
            if ( invAM.item(i,j) != 0)
                isSingular = false;
            j++;
        }
        i++;
    }

    return !isSingular;
}



/**
 * @brief Calls Graph::createMatrixAdjacencyInverse(method) to compute
 * the inverse of the adjacency matrix and writes it to file fn in HTML notation
 * @param fn
 * @param method
 */
void Graph::writeMatrixAdjacencyInvert(const QString &fn,
                                       const QString &method)
{
    qDebug() << "Writing inverse of the adjacency matrix to file:" << fn;

    int i=0, j=0;
    VList::const_iterator it, it1;
    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    QTextStream outText( &file );

    outText << "-Social Network Visualizer "<<  VERSION << "\n";
    outText << tr("Network name: ")<< getName()<< "\n\n";
    outText << tr("Inverse Matrix:") << "\n";
    if (!createMatrixAdjacencyInverse(method)) {
        outText << "\n"<< " The adjacency matrix is singular.";
        file.close();
        return;
    }
    int isolatedVertices = verticesListIsolated().size();
    if (  isolatedVertices  > 0  )
        outText << "\n"<< "Dropped "<< isolatedVertices
                << " isolated vertices"
                    << "\n\n";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() || (*it)->isIsolated() )
            continue;
        j=0;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() || (*it)->isIsolated() )
                continue;
            outText << invAM.item(i,j)<< " ";
            qDebug() << i << "," << j << " = " << invAM.item(i,j);
            j++;
        }
        i++;
        outText << "\n";
        qDebug() << "\n";
    }

    file.close();
}




/**
 * @brief Computes the Degree matrix of the graph and writes it to given file
 * @param fn
 */
void Graph::writeMatrixDegreeText(const QString &fn) {

    qDebug() << "Writing degree matrix to file:" << fn;

    createMatrixAdjacency();

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    QTextStream outText( &file );

    outText << AM.degreeMatrix();

    file.close();

}




/**
 * @brief Computes the Laplacian matrix of the graph and writes it to given file
 * @param fn
 */
void Graph::writeMatrixLaplacianPlainText(const QString &fn) {

    qDebug() << "Writing Laplacian matrix to file:" << fn;

    createMatrixAdjacency();

    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )  {
        qDebug () << "Could not open (for writing) file:" << fn;
        emit statusMessage ( tr("Error. Could not write to ") + fn );
        return;
    }
    QTextStream outText( &file );

    outText << AM.laplacianMatrix();

    file.close();

}





/**
 * @brief Repositions all nodes on random positions
 * Emits setNodePos(i, x,y) to tell GW that the node item should be moved.
 * @param maxWidth
 * @param maxHeight
 */
void Graph::layoutRandom(){
    qDebug()<< "Graph::layoutRandom() ";
    double new_x=0, new_y=0;
    VList::const_iterator it;

    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Embedding Random Layout. \n"
                     "Please wait...");
    emit statusMessage(  pMsg  );
    emit signalProgressBoxCreate (N,pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        emit signalProgressBoxUpdate (++progressCounter);
        new_x= canvasRandomX();
        new_y= canvasRandomY();
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug()<< "Graph::layoutRandom() - "
                << " vertex " << (*it)->number()
                << " emitting setNodePos to new pos " << new_x << " , "<< new_y;
        emit setNodePos((*it)->number(),  new_x,  new_y);
    }

    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexPositions);
}



/**
 * @brief Repositions all nodes on the periphery of
 * different circles with random radius
 * @param x0
 * @param y0
 * @param maxRadius
 */
void Graph::layoutRadialRandom(const bool &guides){
    qDebug() << "Graph::layoutRadialRandom - ";
    double rad=0, new_radius=0, new_x=0, new_y=0;
    double i=0;
    double x0=canvasWidth/2.0;
    double y0=canvasHeight/2.0;

    double maxRadius = canvasMaxRadius();
    //offset controls how far from the centre the central nodes be positioned
    qreal offset=0.06, randomDecimal=0;
    int vert=vertices();
    int progressCounter=0;
    VList::const_iterator it;


    int N = vertices();


    QString pMsg = tr("Embedding Random Radial layout. \n"
                      "Please wait ....");
    emit statusMessage(  pMsg );
    emit signalProgressBoxCreate(N,pMsg );


    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        randomDecimal = (qreal ) ( rand() % 100 ) / 100.0;
        new_radius=(maxRadius- (randomDecimal - offset)*maxRadius);

        qDebug () << "Vertice " << (*it)->number()
                  << " at x=" << (*it)->x()
                  << ", y= "<< (*it)->y()
                  << ", maxradius " <<  maxRadius
                  << " randomDecimal " << randomDecimal
                  << " new radius " << new_radius;

        //Calculate new position
        rad= (2.0* M_PI/ vert );
        new_x=x0 + new_radius * cos(i * rad);
        new_y=y0 + new_radius * sin(i * rad);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug("Vertice will move to x=%f and y=%f ", new_x, new_y);
        //Move node to new position
        emit setNodePos((*it)->number(),  new_x,  new_y);
        i++;
        if (guides) {
            emit addGuideCircle ( x0, y0, new_radius );
        }
    }

    emit signalProgressBoxKill();
    setModStatus(ModStatus::VertexPositions);
}




/**
 * @brief Repositions all nodes on the periphery of a circle with given radius
 * @param x0
 * @param y0
 * @param maxRadius
 */
void Graph::layoutCircular (const double &x0, const double &y0,
                            const double &newRadius, const bool &guides){
    qDebug() << "Graph::layoutCircular - ";
    double rad=0, new_x=0, new_y=0;
    double i=0;
    VList::const_iterator it;
    int N=vertices();
    int progressCounter=0;

    QString pMsg=tr("Applying circular layout. \nPlease wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(N, pMsg);

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){

        emit signalProgressBoxUpdate(++progressCounter);

        if ( ! (*it)->isEnabled() ) {
            qDebug() << "  vertex i" << (*it)->number() << " disabled. Continue";
            continue;
        }

        //Calculate new position
        rad= (2.0* M_PI/ N );
        new_x=x0 + newRadius * cos(i * rad);
        new_y=y0 + newRadius * sin(i * rad);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug("Vertice will move to x=%f and y=%f ", new_x, new_y);
        //Move node to new position
        emit setNodePos((*it)->number(),  new_x,  new_y);
        i++;

        if (guides) {
            emit addGuideCircle ( x0, y0, newRadius );
        }

    }

    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexPositions);

}






/**
 * @brief Convenience method
 * Changes the size of all nodes to be proportional to their outDegree (Degree Centrality)
 * Calls layoutByProminenceIndex
 */
void Graph::layoutVertexSizeByOutdegree() {

    layoutByProminenceIndex(1,2);
}


/**
 * @brief Convenience method
 * Changes the size of all nodes to be proportional to their InDegree (Degree Prestige)
 * Calls layoutByProminenceIndex
 */
void Graph::layoutVertexSizeByIndegree() {

    layoutByProminenceIndex(10,2);
}





/**
 * @brief Applies a layout according to each actor's prominence index score.
* The layout type can be radial (0), level (1), node sizes (2) or node colors (3), as follows:
* layoutType=0 - Repositions all nodes on the periphery of concentric circles with radius analogous to their prominence index
* layoutType=1 - Repositions all nodes on different top-down levels according to their centrality
* layoutType=2 - Changes node sizes to be proportional to their prominence index score
* layoutType=2 - Changes node colors to reflect their prominence index score (from red to green)
 * @param prominenceIndex, 0-12
 * @param layoutType: 0,1,2,3
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::layoutByProminenceIndex (int prominenceIndex, int layoutType,
                                     const bool &considerWeights,
                                     const bool &inverseWeights,
                                     const bool &dropIsolates) {
    qDebug() << "Applying layout by prominence index:"
                << prominenceIndex
                << "type:" << layoutType;

    double  i=0, std=0, norm=0;
    double new_x=0, new_y=0;
    qreal C=0, maxC=0;
    double x0=0, y0=0, maxRadius=0, new_radius=0, rad=0;
    double maxWidth=0, maxHeight=0;
    qreal offset=0;
    int new_size=0;
    int progressCounter=0;

    int N=vertices();
    VList::const_iterator it;

    QColor new_color;

    switch (layoutType){
    case 0: { // radial
        x0=canvasWidth/2.0;
        y0=canvasHeight/2.0;
        offset=0.06;
        maxRadius=canvasMaxRadius();
        break;
    }
    case 1: { //level
        offset=50;
        maxHeight= canvasHeight-offset;
        maxWidth= canvasWidth-offset;
        break;
    }
    };


    emit statusMessage(tr("Computing centrality/prestige scores. Please wait..."));

    //first compute centrality indices if needed

    if ( prominenceIndex == 0) {
        // do nothing
    }
    else if ( prominenceIndex == IndexType::DC ) {
        centralityDegree(considerWeights, dropIsolates);
    }
    else if ( prominenceIndex == IndexType::IRCC ){
        centralityClosenessIR(considerWeights,inverseWeights, dropIsolates);
    }
    else if ( prominenceIndex == IndexType::IC ) {
        centralityInformation(considerWeights,inverseWeights);
    }
    else if ( prominenceIndex == IndexType::EVC ){
        centralityEigenvector(considerWeights, dropIsolates);
    }
    else if ( prominenceIndex == IndexType::DP ){
        prestigeDegree(considerWeights, dropIsolates);
    }
    else if ( prominenceIndex == IndexType::PRP ) {
        prestigePageRank(dropIsolates);
    }
    else if ( prominenceIndex == IndexType::PP ){
        prestigeProximity(considerWeights, inverseWeights, dropIsolates);
    }
    else{
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);
    }


    QString pMsg;
    switch (layoutType){
    case 0: { // radial
        pMsg = tr("Embedding Radial layout by Prominence Score. \nPlease wait...");
        break;
    }
    case 1: { // level
        pMsg = tr("Embedding Level layout by Prominence Score. \nPlease wait...");
        break;
    }
    case 2: { // node size
        pMsg = tr("Embedding Node Size by Prominence Score layout. \nPlease wait...");
        break;
    }
    case 3: { // node color
        pMsg = tr("Embedding Node Color by Prominence Score layout. \nPlease wait...");
        break;
    }
    }
    emit statusMessage(  pMsg );
    emit signalProgressBoxCreate(N,pMsg);

    for  (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {

        switch (prominenceIndex) {
        case 0: {
            C=0;maxC=0;
            break;
        }
        case IndexType::DC : {
            C=(*it)->SDC();
            std= (*it)->SDC();
            maxC=maxSDC;
            break;
        }
        case IndexType::CC : {
            C=(*it)->CC();
            std= (*it)->SCC();
            maxC=maxSCC;
            break;
        }
        case IndexType::IRCC : {
            C=(*it)->IRCC();
            std= (*it)->SIRCC();
            maxC=maxIRCC;
            break;
        }
        case IndexType::BC : {
            C=(*it)->BC();
            std= (*it)->SBC();
            maxC=maxSBC;
            break;
        }
        case IndexType::SC : {
            C=(*it)->SC();
            std= (*it)->SSC();
            maxC=maxSSC;
            break;
        }
        case IndexType::EC : {
            C=(*it)->EC();
            std= (*it)->SEC();
            maxC=maxEC;
            break;
        }
        case IndexType::PC : {
            C=(*it)->PC();
            std= (*it)->SPC();
            maxC=maxSPC;
            break;
        }
        case IndexType::IC : {
            C=(*it)->IC();
            std= (*it)->SIC();
            maxC=maxIC;
            break;
        }
        case IndexType::EVC : {
            C=(*it)->EVC();
            std= (*it)->SEVC();
            maxC=1;
            break;
        }
        case IndexType::DP : {
            C=(*it)->SDP();
            std= (*it)->SDP();
            maxC=maxSDP;
            break;
        }
        case IndexType::PRP : {
            C=(*it)->PRP();
            std= (*it)->SPRP();
            maxC=1;
            break;
        }
        case IndexType::PP : {
            C=(*it)->PP();
            std= (*it)->SPP();
            maxC=maxPP;
            break;
        }
        };

        norm = std/maxC;

        emit signalProgressBoxUpdate( ++progressCounter);

        switch (layoutType){

        case 0: { // radial

            qDebug () << "vertex" << (*it)->number()
                      << "pos x" << (*it)->x() << "y"<< (*it)->y()
                      << "C" << C << "stdC" << std << "maxC"<< maxC
                      << "norm (std/maxC)" << norm
                      << "maxradius" <<  maxRadius
                      << "newradius" << maxRadius - (norm - offset)*maxRadius;

            //Compute new vertex position
            switch (static_cast<int> (ceil(maxC)) ){

            case 0: {
                qDebug("maxC=0.   Using maxRadius");
                new_radius=maxRadius;
                break;
            }

            default: {
                new_radius=(maxRadius- (norm - offset)*maxRadius);
                break;
            }

            };

            rad= (2.0* M_PI/ N );
            new_x=x0 + new_radius * cos(i * rad);
            new_y=y0 + new_radius * sin(i * rad);

            qDebug() << "Finished calculation. "
                        "new radial pos: x"<< new_x << "y" << new_y;

            //Move vertex to new position
            (*it)->setX( new_x );
            (*it)->setY( new_y );

            emit setNodePos((*it)->number(), new_x, new_y);

            i++;

            emit addGuideCircle ( x0, y0, new_radius );

            break;
        }

        case 1: { // level

            qDebug()<< "vertex" << (*it)->number()
                    << "pos x"<< (*it)->x() << "y"<<  (*it)->y()
                    << "C" << C << "stdC" << std  << "maxC "<<	maxC
                    << "norm (std/maxC)" << norm
                    << "maxWidth " << maxWidth
                    <<" maxHeight "<<maxHeight
                   << "maxHeight-(norm)*maxHeight "
                    <<  maxHeight-(norm)*maxHeight;

            //Compute new vertex position
            switch ( static_cast<int> (ceil(maxC)) ){

            case 0: {
                qDebug("maxC=0.   Using maxHeight");
                new_y=maxHeight;
                break;
            }

            default: {
                new_y=offset/2.0+maxHeight-(norm)*maxHeight;
                break;
            }

            };

            new_x=offset/2.0 + rand() % ( static_cast<int> (maxWidth) );

            qDebug() << "Finished calculation. "
                        "new level pos: x"<< new_x << "y" << new_y;

            //Move vertex to new position
            (*it)->setX( new_x );
            (*it)->setY( new_y );

            emit setNodePos((*it)->number(),  new_x,  new_y);

            i++;

            emit addGuideHLine(new_y);

            break;

        }

        case 2: { // node size

            qDebug () << "vertex" << (*it)->number()
                      << "C=" << C << ", stdC=" << std  << "maxC" << maxC
                      << "initVertexSize " << initVertexSize
                      << "norm (stdC/maxC) " << norm
                      << ", (norm) * initVertexSize " << (norm *initVertexSize);

            //Calculate new node size
            switch (static_cast<int> (ceil(maxC) )){

            case 0: {
                qDebug()<<"maxC=0.   Using initVertexSize";
                new_size=initVertexSize;
                break;
            }

            default: {
                new_size=ceil ( initVertexSize/2.0 + (qreal) initVertexSize * (norm));
                break;
            }

            };

            //set new vertex size and emit signal to change node size
            qDebug() << "Finished calculation. "
                     << "new vertex size "<< new_size << " call setSize()";
            (*it)->setSize(new_size);
            emit setNodeSize((*it)->number(),  new_size);

            break;
        }

        case 3: { // node color

            qDebug () << "vertex" << (*it)->number()
                      << "C=" << C << ", stdC=" << std << "maxC" << maxC
                      << "initVertexColor " << initVertexColor
                      << "norm (stdC/maxC) " << norm;

            //Calculate new node color
            switch (static_cast<int> (ceil(maxC) )){
            case 0: {
                qDebug()<<"maxC=0.   Using initVertexColor";
                new_color = QColor (initVertexColor);
                break;
            }
            default: {
                // H = 0 (red) for most important nodes
                // H = 240 (blue) for least important nodes
                new_color.setHsv( 240 - norm * 240, 255, 255,255 );
                break;
            }
            };

            //change vertex color and emit signal to change node color as well
            qDebug ()<< "new vertex color "<< new_color << " call setSize()";
            (*it)->setColor(new_color.name());

            emit setNodeColor((*it)->number(),  new_color.name());

            break;
        }

        };
    }

    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexPositions);

    prominenceDistribution(prominenceIndex, m_reportsChartType);

}






/**
 * @brief Embeds a Force Directed Placement layout according to the initial Spring Embedder model proposed by Eades.
 * @param maxIterations
 *  The Spring Embedder model (Eades, 1984), part of the Force Directed Placement (FDP) family,
    assigns forces to all vertices and edges, as if nodes were electrically charged particles (Coulomb's law)
    and all edges were springs (i.e. Hooke's law).

    These forces are applied to the nodes iteratively, pulling them closer together or pushing them further apart,
    until the system comes to an equilibrium state (node positions do not change anymore).

    Note that, following Eades, we do not need to have a faithful simulation;
    we can -and we do- apply unrealistic forces in an unrealistic manner.
    For instance, instead of the forces described by Hooke's law,
    we will assume weaker logarithmic forces between far apart vertices...
 */
void Graph::layoutForceDirectedSpringEmbedder(const int maxIterations){

    int iteration = 1 ;
    int progressCounter=0;
    qreal dist = 0;
    qreal f_rep=0, f_att=0;
    QPointF DV;
    qreal c4=0.1; //normalization factor for final displacement


    VList::const_iterator v1;
    VList::const_iterator v2;


    /**
     * compute max spring length as function of canvas area divided by the
     * total vertices area
    */
    qreal V = (qreal) vertices() ;
    qreal naturalLength= computeOptimalDistance(V);
    qDebug() << "\n\n layoutForceDirectedSpringEmbedder() "
             << " vertices " << V
             << " naturalLength " << naturalLength;


    /* apply an initial random layout */
    //layoutCircular(canvasWidth/2.0, canvasHeight/2.0, naturalLength/2.0 ,false);
    layoutRandom();

    QString pMsg  = tr ( "Embedding Eades Spring-Gravitational model. \n"
                         "Please wait ....");
    emit statusMessage( pMsg  );
    emit signalProgressBoxCreate (maxIterations, pMsg );

    for ( iteration=1; iteration <= maxIterations ; iteration++) {

        //setup init disp
        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
            qDebug() << " 0000 s " << (*v1)->number() << " zeroing rx/ry";
        }

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            qDebug() << "*********  Calculate forces for source s  "
                     << (*v1)->number()
                     <<" pos "<< (*v1)->x()<< ", "<< (*v1)->y();

            if ( ! (*v1)->isEnabled() ) {
                qDebug() << "  vertex s disabled. Continue";
                continue;
            }

            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2)
            {
                if ( ! (*v2)->isEnabled() ) {
                    qDebug() << "   t " << (*v1)->number() << " disabled. Continue";
                    continue;
                }

                if (v2 == v1) {
                    qDebug() << "   s==t, continuing";
                    continue;
                }

                DV.setX( (*v2)->x() - (*v1)->x());
                DV.setY( (*v2)->y() - (*v1)->y());

                dist = graphDistanceEuclidean(DV);

                /**
                  *  calculate electric (repulsive) forces between
                  *  all vertices.
                  */
                f_rep = layoutForceDirected_F_rep ("Eades", dist, naturalLength) ;
                (*v1)->disp().rx() += sign( DV.x() ) * f_rep ;
                (*v1)->disp().ry() += sign( DV.y() ) * f_rep  ;
                qDebug() <<"  s = "<< (*v1)->number()
                        <<" pushed away from t = " << (*v2)->number()
                       << " dist " <<dist
                       << " f_rep=" << f_rep
                       << " sign * f_repx " << sign( DV.x() ) * f_rep
                       << " sign * f_repy " << sign( DV.y() ) * f_rep ;

                /**
                * calculate spring forces between adjacent nodes
                * that pull them together (if d > naturalLength)
                * or push them apart (if d < naturalLength)
                */
                if ( this->edgeExists( (*v1)->number(), (*v2)->number()) ) {

                    f_att = layoutForceDirected_F_att ("Eades", dist, naturalLength) ;

                    (*v1)->disp().rx() += sign( DV.x() ) * f_att ;
                    (*v1)->disp().ry() += sign( DV.y() ) * f_att ;
                    (*v2)->disp().rx() -= sign( DV.x() ) * f_att ;
                    (*v2)->disp().ry() -= sign( DV.y() ) * f_att ;

                    qDebug() << "  s= "<<(*v1)->number()
                             << " attracted by t= "<< (*v2)->number()
                             << " dist " <<dist
                             << " f_att="<< f_att
                             << " sdx * f_att " <<sign( DV.x() ) * f_att
                             << " sdy * f_att " <<sign( DV.y() ) * f_att
                             << " disp_s.x="<< (*v2)->disp().rx()
                             << " disp_s.y="<< (*v2)->disp().ry()
                             << " disp_t.x="<< (*v2)->disp().rx()
                             << " disp_t.y="<< (*v2)->disp().ry();

                }  // end if edgeExists

            } //end for v2

            qDebug() << "  >>> final s = "<< (*v1)->number()
                     << " disp_s.x="<< (*v1)->disp().rx()
                     << " disp_s.y="<< (*v1)->disp().ry();

        } // end for v1


        layoutForceDirected_Eades_moveNodes(c4) ;

        emit signalProgressBoxUpdate( ++progressCounter );

    } //end iterations

    emit signalProgressBoxKill();
}



/**
 * @brief Embeds a Force Directed Placement layout according to the Fruchterman-Reingold model.
  *  Fruchterman and Reingold (1991) refined the Spring Embedder model by replacing the forces.
    In this model, "the vertices behave as atomic particles or celestial bodies,
    exerting attractive and repulsive forces on one another." (ibid).
    Again, only vertices that are neighbours attract each other but, unlike Spring Embedder,
    all vertices repel each other.
    These forces induce movement. The algorithm might resemble molecular or planetary simulations,
    sometimes called n-body problems.
 * @param maxIterations
 */
void Graph::layoutForceDirectedFruchtermanReingold(const int maxIterations){
    int progressCounter=0;
    qreal dist = 0;
    qreal f_att, f_rep;
    QPointF DV;   //difference vector

    qreal V = (qreal) vertices() ;
    qreal C=0.9; //this is found experimentally
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    qreal optimalDistance= C * computeOptimalDistance(V);

    VList::const_iterator v1, v2;
    int iteration = 1 ;

    /* apply an initial circular layout */
    //layoutCircular(canvasWidth/2.0, canvasHeight/2.0, optimalDistance/2.0,false);
    //layoutRandom();

    qDebug() << "Graph: layoutForceDirectedFruchtermanReingold() ";
    qDebug () << "Graph: Setting optimalDistance = "<<  optimalDistance
              << "...following Fruchterman-Reingold (1991) formula ";

    qDebug() << "Graph: canvasWidth " << canvasWidth << " canvasHeight " << canvasHeight;


    QString pMsg = tr( "Embedding Fruchterman & Reingold forces model. \n"
                       "Please wait ...");
    emit statusMessage( pMsg );

    emit signalProgressBoxCreate(maxIterations,pMsg );

    for ( iteration=1; iteration <= maxIterations ; iteration++) {

        //setup init disp
        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
            //qDebug() << " 0000 s " << (*v1)->number() << " zeroing rx/ry";
        }

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            //            qDebug() << "*****  Calculate forces for s " << (*v1)->number()
            //                     << " vpos " <<  vpos[(*v1)->number()]
            //                     << " pos "<< (*v1)->x() << ", "<< (*v1)->y();

            if ( ! (*v1)->isEnabled() ) {
                //                qDebug() << "  vertex s " << (*v1)->number() << " disabled. Continue";
                continue;
            }

            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2)
            {
                //                qDebug () << "  t = "<< (*v2)->number()
                //                          << "  pos (" <<  (*v2)->x() << "," << (*v2)->y() << ")";

                if ( ! (*v2)->isEnabled() ) {
                    //                    qDebug()<< " t "<< (*v2)->number()<< " disabled. Continue";
                    continue;
                }

                if (v2 == v1) {
                    //                    qDebug() << "  s==t, continuing";
                    continue;
                }

                DV.setX( (*v2)->x() - (*v1)->x() );
                DV.setY( (*v2)->y() - (*v1)->y() );

                dist = graphDistanceEuclidean( DV );

                //calculate repulsive force from _near_ vertices
                f_rep = layoutForceDirected_F_rep( "FR", dist, optimalDistance);
                (*v1)->disp().rx() += sign( DV.x() ) * f_rep;
                (*v1)->disp().ry() += sign( DV.y() ) * f_rep ;

                //                qDebug()<< " dist( " << (*v1)->number() <<  "," <<  (*v2)->number() <<  " = "
                //                        << dist
                //                        << " f_rep " << f_rep
                //                        << " disp_s.x="<< (*v1)->disp().rx()
                //                        << " disp_s.y="<< (*v1)->disp().ry();

                if ( edgeExists ((*v1)->number(), (*v2)->number()) ) {
                    //calculate attracting force
                    f_att = layoutForceDirected_F_att ("FR", dist, optimalDistance);
                    (*v1)->disp().rx() += sign( DV.x() ) * f_att;
                    (*v1)->disp().ry() += sign( DV.y() ) * f_att;
                    (*v2)->disp().rx() -= sign( DV.x() ) * f_att ;
                    (*v2)->disp().ry() -= sign( DV.y() ) * f_att ;

                    qDebug() << "  s= "<<(*v1)->number()
                             << " attracted by t= "<< (*v2)->number()
                             <<"  optimalDistance =" << optimalDistance
                            << " f_att " << f_att
                            << " disp_s.x="<< (*v1)->disp().rx()
                            << " disp_s.y="<< (*v1)->disp().ry()
                            << " disp_t.x="<< (*v2)->disp().rx()
                            << " disp_t.y="<< (*v2)->disp().ry();
                } //endif

            }//end for v2

        } //end for v1

        // limit the max displacement to the temperature t
        // prevent placement outside of the frame/canvas
        layoutForceDirected_FR_moveNodes( layoutForceDirected_FR_temperature (iteration) );

        emit signalProgressBoxUpdate( ++progressCounter );
    }

    emit signalProgressBoxKill();
}




/**
 * @brief Embeds a Force Directed Placement layout according to the Kamada-Kawai model.
 * In this model, the network is considered to be a dynamic system
 * where every two actors are 'particles' mutually connected by a 'spring'.
 * Each spring has a desirable length, which corresponds to their graph
 * theoretic distance. In this way, the optimal layout of the graph
 * is the state with the minimum imbalance. The degree of
 * imbalance is formulated as the total spring energy:
 * the square summation of the differences between desirable
 * distances and real ones for all pairs of particles
 * Initially, the particles/actors are placed on the vertices of a regular n-polygon
 */

void Graph::layoutForceDirectedKamadaKawai(const int maxIterations,
                                           const bool considerWeights,
                                           const bool inverseWeights,
                                           const bool dropIsolates,
                                           const QString  &initialPositions){

    qDebug()<< "Embedding an FDP layout according to the Kamada-Kawai model, maxIterations:" << maxIterations;

    VList::const_iterator v1, v2;

    int progressCounter=0, minimizationIterations=0;

    int i=0, j=0, m=0, pm=0, pnm=0, pn=0;

    int N=vertices();  // active actors

    qreal K=1;  // constant
    qreal L=0;  // the desirable length of a single edge.
    qreal L0=0; // the length of a side of the display square area
    qreal D=0;  // the graph diameter



    Matrix l; // the original spring length between pairs of particles/actors
    Matrix k; // the strength of the spring between pairs of particles/actors

    Matrix LIN_EQ_COEF(2,2);    // holds the coefficients of set of linear equations 11 and 12
    qreal b[2];                 // holds the right hand vector of linear equations 11 and 12

    qreal partDrvtEx = 0; // partial derivative of E by Xm
    qreal partDrvtEy = 0; // partial derivative of E by Ym
    qreal partDrvtExSec_m = 0; // partial second derivative of E by Xm
    qreal partDrvtEySec_m = 0; // partial second derivative of E by Ym
    qreal partDrvtExEySec_m = 0; // partial second derivative of E by Xm Ym
    qreal partDrvtEyExSec_m = 0; // partial second derivative of E by Ym Xm

    qreal partDrvtEx_m = 0; // cache for partial derivative of E by Xm, for particle with max D_i
    qreal partDrvtEy_m = 0; // cache for partial derivative of E by Ym, for particle with max D_i

    qreal partDrvDenom = 0;
    qreal xm=0,ym=0;
    qreal xi=0,yi=0;
    qreal xpm=0, ypm=0; // cache for pos of particle with max D_i

    qreal dx=0, dy=0;

    qreal epsilon=0.1;
    qreal Delta_m=0;
    qreal Delta_max=epsilon + 0.0001;

    bool couldNotSolveLinearSystem = false;

    // Compute graph-theoretic distances dij for 1 <= i!=j <= n

    qDebug()<< "Compute dij, where (i,j) in E";

    graphMatrixDistanceGeodesicCreate(considerWeights,inverseWeights, dropIsolates);

    // Compute original spring length
    // lij for 1 <= i!=j <= n using the formula:
    // lij = L x dij
    // where L the desirable length of a single edge in the display pane
    // Since we are on a restricted display (a canvas), L depends on the
    // diameter D of the graph and the length L of a side of the display square:
    // L = L0 / D

    qDebug()<< "Compute lij = L x dij. lij will be symmmetric.";

    D = graphDiameter(considerWeights,inverseWeights);
    L0 = canvasMinDimension()-100;
    L = L0 / D;

    qDebug()<< "L="<< L0 << "/" <<D << "=" <<L;

    l.zeroMatrix(DM.rows(), DM.cols());
    l=DM;
    l.multiplyScalar(L);

//    qDebug()<< "Graph::layoutForceDirectedKamadaKawai() - l=" ;
//    l.printMatrixConsole();


    // Compute kij for 1 <= i!=j <= n using the formula:
    // kij = K / dij ^2
    // kij is the strength of the spring between pi and pj, K a constant

    qDebug()<< "Compute kij = K / dij ^2. kij will be symmmetric. ";

    k.zeroMatrix(DM.rows(), DM.cols());

    for (i=0; i<N; i++){
        for (j=0; j<N; j++){
            if ( i == j )
                continue;
            k.setItem(i,j, K / (DM.item(i,j) * DM.item(i,j)));
        }
    }
//    qDebug()<< "Graph::layoutForceDirectedKamadaKawai() - k=" ;
//    k.printMatrixConsole();


    // initialize p1, p2, ... pn
    qDebug()<< "Set particles to initial positions p" ;

    i=0;

    if (initialPositions == "circle") {
        double x0=0, y0=0;
        x0=canvasWidth/2.0;
        y0=canvasHeight/2.0;
        // placing the particles on the vertices of a regular n-polygon
        // circumscribed by a circle whose diameter is L0
        layoutCircular(x0,y0,L0/2,false);
    }
    else if (initialPositions == "random") {
        layoutRandom();
    }


    QString pMsg = tr("Embedding Kamada & Kawai spring model.\n"
                      "Please wait...");
    emit statusMessage( pMsg );
    emit signalProgressBoxCreate(maxIterations, pMsg);

    // while ( max_D_i > e )
    while (Delta_max > epsilon) {

        progressCounter++;

        emit signalProgressBoxUpdate( progressCounter );

        if (progressCounter == maxIterations) {
//            qDebug()<< "Reached maxIterations. BREAK";
            break;
        }


        Delta_max = epsilon;

        // choose particle with largest Delta_m = max Delta_i
        // compute partial derivatives of E by xm and ym for every particle m
        // using equations 7 and 8

        qDebug()<< "Iteration:" << progressCounter
                << "Choose particle with largest Delta_m = max Delta_i ";

        pnm = -1;

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1) {

            pn = (*v1)->number();
            m = vpos[pn];
            xm = (*v1)->x();
            ym = (*v1)->y();

//            qDebug()<< "Compute partial derivatives E for particle" << pn
//                    << " vpos m" <<  m
//                    << " pos"<< xm << ", "<< ym;

            if ( ! (*v1)->isEnabled() ) {

//                qDebug() << "  particle " << pn
//                         << " vpos m " << m << " disabled. Continue";
                continue;
            }

            partDrvtEx = 0;
            partDrvtEy = 0;

            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2) {

                i = vpos[(*v2)->number()];
                xi = (*v2)->x();
                yi = (*v2)->y();

//                qDebug () << "  particle vpos i"<< i
//                          << "  pos (" <<  xi << "," << yi << ")";

                if ( ! (*v2)->isEnabled() ) {
                    qDebug()<< " i "<< (*v2)->number()<< " disabled. Continue";
                    continue;
                }

                if (m == i) {
                    qDebug() << "  m==i, continuing";
                    continue;
                }

                partDrvtEx += k.item(m,i) * ( (xm - xi) - ( l.item(m,i) * (xm - xi) ) / sqrt( (xm - xi) * (xm - xi) + (ym - yi)*(ym - yi) ) );
                partDrvtEy += k.item(m,i) * ( (ym - yi) - ( l.item(m,i) * (ym - yi) ) / sqrt( (xm - xi) * (xm - xi) + (ym - yi)*(ym - yi) ) );


            } // end v2 for loop

            Delta_m = sqrt (partDrvtEx * partDrvtEx + partDrvtEy * partDrvtEy);

//            qDebug()<< "m" << m << " Delta_m" << Delta_m;

            if (Delta_m > Delta_max) {

//                qDebug()<< "m" << m << " Delta_m > Delta_max. Setting new Delta_max = "<< Delta_m;

                Delta_max = Delta_m;
                partDrvtEx_m = partDrvtEx;
                partDrvtEy_m = partDrvtEy;

                pnm = pn ;  // store name of particle satisfying Delta_m = max Delta_i
                pm = m ;  // store vpos of particle satisfying Delta_m = max Delta_i
                xpm = xm; // store particle x pos
                ypm = ym; // store particle y pos
            }

        } // end v1 for loop


        if (pnm < 0) {
//            qDebug () << "No particle left with Delta_m > epsilon -- BREAK";
            break;
        }


        // let pm the particle satisfying  D_m = max_D_i
        m  = pm ;
        xm = xpm;
        ym = ypm;

//        qDebug () << "m"<< m<< "has max Delta_m"<< Delta_max
//                  << " Starting minimizing Delta_m - "
//                << " initial m pos " << xm << ym;


        minimizationIterations=0;

        // while ( D_m > e)
        do {
            if (minimizationIterations > 10) {
                qDebug()<<  "Reached minimizationIterations threshold. BREAK";
                break;
            }
            minimizationIterations++;

//            qDebug () << "Started minimizing Delta_m for m"<< m
//                      << "First compute dx and dy by solving equations 11 and 12 ";

            // compute dx and dy by solving equations 11 and 12

            partDrvtEx=0;
            partDrvtEy=0;
            partDrvtEx_m = 0;
            partDrvtEy_m = 0;
            partDrvtExSec_m=0;
            partDrvtEySec_m=0;
            partDrvtExEySec_m=0;
            partDrvtEyExSec_m=0;

            // first compute coefficients of the linear system equations
            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2) {

                i = vpos[(*v2)->number()];
                xi = (*v2)->x();
                yi = (*v2)->y();

//                qDebug () << "  m"<< m << "  i"<< i
//                          << "  pos_i (" <<  xi << "," << yi << ")";

                if ( ! (*v2)->isEnabled() ) {
                    qDebug()<< " i "<< (*v2)->number()<< " disabled. Continue";
                    continue;
                }

                if (i == m) {
                    qDebug() << "  m==i, continuing";
                    continue;
                }
                partDrvDenom = pow ( sqrt( (xm - xi) * (xm - xi) + (ym - yi)*(ym - yi) ) , 3 );


                partDrvtEx_m += k.item(m,i) * ( (xm - xi) - ( l.item(m,i) * (xm - xi) ) / sqrt( (xm - xi) * (xm - xi) + (ym - yi)*(ym - yi) ) );
                partDrvtEy_m += k.item(m,i) * ( (ym - yi) - ( l.item(m,i) * (ym - yi) ) / sqrt( (xm - xi) * (xm - xi) + (ym - yi)*(ym - yi) ) );

                partDrvtExSec_m   += k.item(m,i) * ( 1 - ( l.item(m,i) * (ym - yi) * (ym - yi) ) / partDrvDenom );

                partDrvtExEySec_m += k.item(m,i) * ( ( l.item(m,i) * (xm - xi) * (ym - yi) ) /   partDrvDenom );

                partDrvtEyExSec_m += k.item(m,i) * (  ( l.item(m,i) * (xm - xi) * (ym - yi) ) /   partDrvDenom );

                partDrvtEySec_m   += k.item(m,i) * ( 1 - ( l.item(m,i) * (xm - xi) * (xm - xi) ) / partDrvDenom );


            } // end v2 for loop

            Delta_m = sqrt (partDrvtEx_m * partDrvtEx_m + partDrvtEy_m * partDrvtEy_m);

//            qDebug () << "m"<< m << " new Delta_m" << Delta_m;

            LIN_EQ_COEF.setItem(0,0,partDrvtExSec_m);
            LIN_EQ_COEF.setItem(0,1,partDrvtExEySec_m);
            LIN_EQ_COEF.setItem(1,0,partDrvtEyExSec_m);
            LIN_EQ_COEF.setItem(1,1,partDrvtEySec_m);
//            qDebug()<< "Jacobian Matrix of coefficients for linear system (eq. 11 & 12) is:";
//            LIN_EQ_COEF.printMatrixConsole();
            b[0] = - partDrvtEx_m;
            b[1] = - partDrvtEy_m;
//            qDebug()<< "right hand vector is: \n"  << b[0] << " \n" << b[1];
//            qDebug()<< "solving linear system...";
            if ( ! LIN_EQ_COEF.solve(b) )  {
                couldNotSolveLinearSystem = true;
                continue;
            }
//            qDebug()<< "solved linear system.";
            dx=b[0];
            dy=b[1];
//            qDebug()<< "Solution \n b[0] = dx =" << dx << "\n b[1] = dy =" << dy;

//            qDebug () << "m"<< m << " current m pos " << xm << ym << " new m pos " << xm +dx << ym+dy;

            if ( (xm + dx) < 50 || (xm + dx) > (canvasWidth-50) ) {
//                qDebug () << "new xm out of canvas, setting random x";
                xm = canvasRandomX();
            }
            else {
                xm = xm + dx;
            }
            if ( (ym + dy) < 50 || (ym + dy) > (canvasHeight-50) ) {
//                qDebug() <<  "new ym out of canvas, setting random y";
                ym = canvasRandomY();
            }
            else {
                ym = ym + dy;
            }

            qDebug () << "m" << m << " new m pos " << xm  << ym;

            // TODO CHECK IF WE HAVE REACHED A FIXED POINT LOOP

        } while (Delta_m > epsilon);

        if (couldNotSolveLinearSystem) {
            qDebug() << "Could not solve linear system for particle " << pnm << "vpos" << m;
        }

        qDebug () << "Finished minimizing Delta_m for particle" << pnm << "vpos" << m
                  << "Minimized Delta_m"<< Delta_m
                  << "moving it to new pos" << xm << ym;

        m_graph[m]->setX(xm);
        m_graph[m]->setY(ym);
        //emit setNodePos( pnm,  xm,  ym);

    } // end while (Delta_max > epsilon) {

    qDebug () << "Delta_max =< epsilon -- RETURN";

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1) {

        emit setNodePos( (*v1)->number(),  (*v1)->pos().x(),  (*v1)->pos().y());
    }
    emit signalProgressBoxKill();

    setModStatus(ModStatus::VertexPositions);


}






/**
 * @brief Reduces the temperature as the layout approaches a better configuration
 * @return qreal temperature
 */
qreal Graph::layoutForceDirected_FR_temperature(const int iteration) const{
    qreal temp=0;

    qreal temperature=5.8309518948453; //limits the displacement of the vertex

    qDebug() << "Graph::layoutForceDirected_FR_temperature(): cool iteration " << iteration;
    // For the temperature (which limits the displacement of each vertex),
    // Fruchterman & Reingold suggested in their paper that it might start
    // at an initial high value (i.e. "one tenth the width of the frame"
    //  and then decay to 0 in an inverse linear fashion
    // They also suggested "a combination of quenching and simmering",
    // which is a high initial temperature and then a constant low one.
    // This is what SocNetV does. In fact after 200 iterations we follow Eades advice
    // and stop movement (temp = 0)
    if (iteration < 10) {
        // quenching: starts at a high temperature ( canvasWidth / 10) and cools steadily and rapidly
        temp =canvasWidth / (iteration + 10.0) ;
    }
    else if (iteration > 200)  { // follow Eades advice...
        temp = 0;
    }
    else {
        // simmering: stay at a constant low temperature
        temp = temperature;
    }

    qDebug() << "Graph::layoutForceDirected_FR_temperature() - iteration " << iteration
             << " temp " << temp;
    return temp;

}

/**
 * @brief Computes Optimal Distance. Used in Spring Embedder and FR algorithms.
 * @return qreal optimalDistance
 */
qreal Graph::computeOptimalDistance(const int &V){
    qreal vertexWidth = (qreal)  2.0 * initVertexSize ;
    qreal screenArea = canvasHeight*canvasWidth;
    qreal vertexArea =  qCeil ( qSqrt( screenArea / V ) ) ;
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    return (vertexWidth + vertexArea);
}






qreal Graph::layoutForceDirected_F_att( const QString model, const qreal &dist,
                                        const qreal &optimalDistance) {
    qreal f_att;
    if (model == "Eades") {
        qreal c_spring=2;
        f_att = c_spring * log10 ( dist / optimalDistance );
    }
    else {   // model->FR
        f_att= ( dist * dist ) / optimalDistance;
    }

    return f_att;
}


qreal Graph::layoutForceDirected_F_rep( const QString model, const qreal &dist,
                                        const qreal &optimalDistance) {
    qreal f_rep;
    if (model == "Eades") {
        if (dist !=0){
            qreal c_rep= 1.0;
            f_rep =  c_rep / (dist * dist);
            if ( dist > (2.0 * optimalDistance)   ) {
                //neglect vertices outside circular area of radius 2 * optimalDistance
                f_rep=0;
            }
        }
        else {
            f_rep = optimalDistance ; //move away
        }

    }
    else {  // model->FR
        // To speed up our algorithm we use the grid-variant algorithm.
        if ( (2.0 * optimalDistance) < dist ) {
            //neglect vertices outside circular area of radius 2*optimalDistance
            f_rep=0;
        }
        else {
            // repelsive forces are computed only for vertices within a circular area
            // of radius 2*optimalDistance
            f_rep = (optimalDistance * optimalDistance  /  dist) ;
        }
    }

    return -f_rep;
}

/**
 * @brief Graph::sign
 * returns the sign of number D as integer (1 or -1)
 * @param D
 * @return
 */
int Graph::sign(const qreal &D) {
    if (D != 0 ) {
        return ( D / qAbs(D) );
    }
    else {
        return 0;
    }
}

/**
 * @brief Graph::compute_angles
 * Computes the two angles of the orthogonal triangle shaped by two points
 * of difference vector DV and distance dist
 * A = 90
 * B = angle1
 * C = angle2
 *
 * @param DV
 * @param dist
 * @param angle1
 * @param angle2
 * @param degrees1
 * @param degrees2
 */
void Graph::compute_angles(const QPointF &DV,
                           const qreal &dist,
                           qreal &angle1,
                           qreal &angle2,
                           qreal &degrees1,
                           qreal &degrees2 )
{
    if ( dist >0 ) {
        angle1 = qAcos( qAbs(DV.x()) / dist );   // radians
        angle2 = (M_PI  / 2.0) -angle1;   // radians (pi/2 -a1)
    }
    else {
        angle1 =0;
        angle2 =0;
    }
    degrees1 = angle1 * 180.0 / M_PI; // convert to degrees
    degrees2 = angle2 * 180.0 / M_PI; // convert to degrees
    qDebug () << "angle1 " <<angle1 << " angle2 "<<angle2
              <<"deg1 " <<degrees1 << " deg2 "<<degrees2
             << " qCos " << qCos(angle1) << " qSin" << qSin(angle2) ;
}


/**
 * @brief Computes the euclideian distance between QPointF a and b
 * @param a
 * @param b
 * @return
 */
qreal Graph::graphDistanceEuclidean (const QPointF & a, const QPointF & b){
    return qSqrt (
                ( b.x() - a.x() ) * (b.x() - a.x() ) +
                ( b.y() - a.y())  * (b.y() - a.y() )
                );
}


/**
 * @brief the euclideian distance of QPointF a (where a is difference vector)
 * @param a
 * @return
 */
qreal Graph::graphDistanceEuclidean (const QPointF & a){
    return qSqrt (
                a.x()  * a.x()  +
                a.y() * a.y()
                );
}



void Graph::layoutForceDirected_Eades_moveNodes(const qreal &c4) {
    qDebug() << "\n *****  layoutForceDirected_Eades_moveNodes() " ;
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    VList::const_iterator v1;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
    {
        // calculate new overall velocity vector
        xvel =  c4 * (*v1)->disp().rx();
        yvel =  c4 * (*v1)->disp().ry();
        qDebug() << " ##### source vertex  " <<  (*v1)->number()
                 << " xvel,yvel = ("<< xvel << ", "<< yvel << ")";

        //fix Qt error a positive QPoint to the floor
        // when we ask for setNodePos to happen.
        if ( xvel < 1 && xvel > 0 )
            xvel = 1 ;
        if ( yvel < 1 && yvel > 0 )
            yvel = 1 ;

        //Move source node to new position according to overall velocity
        newPos = QPointF( (qreal) (*v1)->x() + xvel, (qreal) (*v1)->y() + yvel);

        qDebug() << " source vertex v1 " << (*v1)->number()
                 << " current pos: (" <<  (*v1)->x()
                 << " , " << (*v1)->y()
                 << " Possible new pos (" <<  newPos.x()
                 << " , " <<  newPos.y();

        // check if new pos is out of usable screen and adjust
        newPos.rx() = canvasVisibleX(newPos.x());
        newPos.ry() = canvasVisibleY(newPos.y());

        qDebug() << "  Final new pos (" <<  newPos.x() << ","
                 << newPos.y()<< ")";
        (*v1)->setX(  newPos.x() );
        (*v1)->setY(  newPos.y() );
        emit setNodePos((*v1)->number(),  newPos.x(),  newPos.y());
        //vertexPosSet();

    }

}

/**
 * @brief Graph::layoutForceDirected_FR_moveNodes
 * @param temperature
 */
void Graph::layoutForceDirected_FR_moveNodes(const qreal &temperature) {

    qDebug() << "\n\n *****  layoutForceDirected_FR_moveNodes() \n\n " ;
    qDebug () << " temperature " << temperature;
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    VList::const_iterator v1;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
    {
        // compute the new position
        // limit the maximum displacement to a maximum temperature
        xvel = sign((*v1)->disp().rx()) * qMin( qAbs((*v1)->disp().rx()), temperature) ;
        yvel = sign((*v1)->disp().ry()) * qMin( qAbs((*v1)->disp().ry()), temperature) ;
        newPos = QPointF((*v1)->x()+ xvel, (*v1)->y()+yvel);
        qDebug()<< " source vertex v1 " << (*v1)->number()
                << " current pos: (" << (*v1)->x() << "," << (*v1)->y() << ")"
                << "Possible new pos (" <<  newPos.x() << ","
                << newPos.y()<< ")";

        newPos.rx() = canvasVisibleX (newPos.x());
        newPos.ry() = canvasVisibleY (newPos.y());

        //Move node to new position
        qDebug()<< " final new pos "
                <<  newPos.x() << ","
                << newPos.y()<< ")";
        (*v1)->setX(  newPos.x() );
        (*v1)->setY(  newPos.y() );
        emit setNodePos((*v1)->number(),  newPos.x(),  newPos.y());
    }
}



/**
 * @brief Helper method, return the human readable name of matrix type.
 * @param matrix
 */
QString Graph::graphMatrixTypeToString(const int &matrixType) const {
    QString matrixStr;

    switch (matrixType) {

    case MATRIX_ADJACENCY :
        matrixStr = "Adjacency Matrix" ;
        break;
    case MATRIX_DISTANCES:
        matrixStr = "Distances Matrix" ;
        break;
    case MATRIX_DEGREE:
        matrixStr = "Degree Matrix" ;
        break;
    case MATRIX_LAPLACIAN:
        matrixStr = "Laplacian Matrix" ;
        break;
    case MATRIX_ADJACENCY_INVERSE:
        matrixStr = "Adjacency Inverse" ;
        break;

    case MATRIX_GEODESICS:
        matrixStr = "Geodesics Matrix" ;
        break;
    case MATRIX_REACHABILITY:
        matrixStr = "Reachability Matrix" ;
        break;
    case MATRIX_ADJACENCY_TRANSPOSE:
        matrixStr = "Adjacency Transpose" ;
        break;
    case MATRIX_COCITATION:
        matrixStr = "Cocitation Matrix" ;
        break;
    case MATRIX_DISTANCES_EUCLIDEAN :
        matrixStr = "Euclidean distance matrix";
        break;
    case MATRIX_DISTANCES_MANHATTAN:
        matrixStr = "Manhattan distance matrix";
        break;
    case MATRIX_DISTANCES_JACCARD:
        matrixStr = "Jaccard distance matrix";
        break;
    case MATRIX_DISTANCES_HAMMING:
        matrixStr = "Hamming distance matrix";
        break;
    default:
        matrixStr = "-" ;
        break;
    }
    return matrixStr;
}


/**
 * @brief Helper method, return the matrix type of human readable matrix name .
 * @param matrix
 * @return
 */
int Graph::graphMatrixStrToType(const QString &matrix) const {
    if (matrix.contains("Hamming", Qt::CaseInsensitive)) {
        return MATRIX_DISTANCES_HAMMING;
    }
    else if (matrix.contains("Jaccard", Qt::CaseInsensitive)) {
        return MATRIX_DISTANCES_JACCARD;
    }
    else if (matrix.contains("Manhattan", Qt::CaseInsensitive)) {
        return MATRIX_DISTANCES_MANHATTAN;
    }
    else if (matrix.contains("Euclidean", Qt::CaseInsensitive)) {
        return MATRIX_DISTANCES_EUCLIDEAN;
    }
    else if (matrix.contains("Cocitation", Qt::CaseInsensitive)) {
        return MATRIX_COCITATION;
    }
    else if (matrix.contains("Adjacency Transpose", Qt::CaseInsensitive)) {
        return MATRIX_ADJACENCY_TRANSPOSE;
    }
    else if (matrix.contains("Reachability", Qt::CaseInsensitive)) {
        return MATRIX_REACHABILITY;
    }
    else if (matrix.contains("Geodesics", Qt::CaseInsensitive)) {
        return MATRIX_GEODESICS;
    }
    else if (matrix.contains("Adjacency Inverse", Qt::CaseInsensitive)) {
        return MATRIX_ADJACENCY_INVERSE;
    }
    else if (matrix.contains("Laplacian", Qt::CaseInsensitive)) {
        return MATRIX_LAPLACIAN;
    }
    else if (matrix.contains("Degree", Qt::CaseInsensitive)) {
        return MATRIX_DEGREE;
    }
    else if (matrix.contains("Adjacency", Qt::CaseInsensitive)) {
        return MATRIX_ADJACENCY;
    }
    else if (matrix.contains("Distances", Qt::CaseInsensitive)) {
        return MATRIX_DISTANCES;
    }
    else {
        return -1;
    }
}

/**
 * @brief Helper method, return the human readable name of metric type.
 * @param metric
 */
QString Graph::graphMetricTypeToString(const int &metricType) const {
    QString metricStr;
    switch (metricType) {
    case METRIC_SIMPLE_MATCHING :
        metricStr = "Simple / Exact matching" ;
        break;
    case METRIC_JACCARD_INDEX:
        metricStr = "Jaccard Index" ;
        break;
    case METRIC_HAMMING_DISTANCE:
        metricStr = "Hamming distance" ;
        break;
    case METRIC_COSINE_SIMILARITY:
        metricStr = "Cosine similarity" ;
        break;
    case METRIC_EUCLIDEAN_DISTANCE:
        metricStr = "Euclidean distance" ;
        break;
    case METRIC_MANHATTAN_DISTANCE:
        metricStr = "Manhattan distance" ;
        break;
    case METRIC_PEARSON_COEFFICIENT:
        metricStr = "Pearson Correlation Coefficient" ;
        break;
    case METRIC_CHEBYSHEV_MAXIMUM:
        metricStr = "Chebyshev distance" ;
        break;
    default:
        metricStr = "-" ;
        break;
    }
    return metricStr;
}


/**
 * @brief Helper method, return the identifier of a metric.
 * @param metricStr
 */
int Graph::graphMetricStrToType(const QString &metricStr) const {
    int metric=METRIC_SIMPLE_MATCHING;
    if (metricStr.contains("Simple",Qt::CaseInsensitive))
        metric = METRIC_SIMPLE_MATCHING ;
    else if (metricStr.contains("Jaccard",Qt::CaseInsensitive))
        metric =METRIC_JACCARD_INDEX ;
    else if (metricStr.contains("None",Qt::CaseInsensitive))
        metric =METRIC_NONE;
    else if (metricStr.contains("Hamming",Qt::CaseInsensitive))
        metric =METRIC_HAMMING_DISTANCE;
    else if (metricStr.contains("Cosine",Qt::CaseInsensitive))
        metric =METRIC_COSINE_SIMILARITY;
    else if (metricStr.contains("Euclidean",Qt::CaseInsensitive))
        metric =METRIC_EUCLIDEAN_DISTANCE;
    else if (metricStr.contains("Manhattan",Qt::CaseInsensitive))
        metric =METRIC_MANHATTAN_DISTANCE;
    else if (metricStr.contains("Pearson ",Qt::CaseInsensitive))
        metric = METRIC_PEARSON_COEFFICIENT;
    else if (metricStr.contains("Chebyshev",Qt::CaseInsensitive))
        metric = METRIC_CHEBYSHEV_MAXIMUM;
    return metric;
}



/**
 * @brief  Helper method, return the human readable name of clustering method type.
 * @return
 */
QString Graph::graphClusteringMethodTypeToString(const int &methodType) const {
    QString methodStr;
    switch (methodType) {
    case Clustering::Single_Linkage:
        methodStr = "Single-linkage (minimum)";
        break;
    case Clustering::Complete_Linkage:
        methodStr = "Complete-linkage (maximum)";
        break;
    case Clustering::Average_Linkage:
        methodStr = "Average-linkage (UPGMA)";
        break;
    default:
        break;
    }
    return methodStr;
}


/**
 * @brief Helper method, return clustering method type from the human readable name of it.
 * @param method
 * @return
 */
int Graph::graphClusteringMethodStrToType(const QString &method) const {
    int methodType=Clustering::Average_Linkage;
    if (method.contains("Single", Qt::CaseInsensitive)) {
        methodType = Clustering::Single_Linkage;
    }
    else if (method.contains("Complete", Qt::CaseInsensitive)) {
        methodType = Clustering::Complete_Linkage;
    }
    else if (method.contains("Average", Qt::CaseInsensitive)) {
        methodType = Clustering::Average_Linkage;
    }
    return methodType;
}

/**
 * @brief Helper method, returns a nice qstring where all html special chars are encoded
 * @param str
 * @return
 */
QString Graph::htmlEscaped(QString str) const {
    str=str.simplified();
    if (str.contains('&') ){
        str=str.replace('&',"&amp;");
    }
    if (str.contains('<') ){
        str=str.replace('<',"&lt;");
    }
    if (str.contains('>') ){
        str=str.replace('>',"&gt;");
    }
    if (str.contains('\"') ){
        str=str.replace('\"',"&quot;");
    }
    if (str.contains('\'') ){
        str=str.replace('\'',"&apos;");
    }
    return str;

}




