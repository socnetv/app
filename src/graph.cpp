/******************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9-dev
 Written in Qt
 
                         graph.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net
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
#include <QFile>
#include <QtMath>
#include <QPointF>
#include <QDebug>		//used for qDebug messages
#include <QHash>
#include <QColor>
#include <QTextCodec>

#include <cstdlib>		//allows the use of RAND_MAX macro 
#include <math.h>
#include <queue>		//for BFS queue Q
#include <ctime>        // for makeThingsLookRandom



static qreal Pi = 3.14159265;

Graph::Graph() {
    m_totalVertices=0;
    outboundEdgesVert=0;
    inboundEdgesVert=0;
    reciprocalEdgesVert=0;
    order=true;		//returns true if the indexes of the list is ordered.
    graphModified=false;
    m_undirected=false;
    symmetricAdjacencyMatrix=true;
    adjacencyMatrixCreated=false;
    reachabilityMatrixCreated=false;
    distanceMatrixCreated=false;
    calculatedDP=false;
    calculatedDC=false;
    calculatedIC=false;
    calculatedCentralities=false;
    calculatedIRCC=false;
    calculatedPP=false;
    calculatedPRP=false;
    calculatedTriad=false;
    m_precision = 3;
    m_curRelation=0;
    dynamicMovement=false;
    timerId=0;
    layoutType=0;

    file_parser = 0;
    wc_parser = 0;
    wc_spider = 0;

 //   edgesHash.reserve(40000);
    influenceDomains.reserve(1000);
    influenceRanges.reserve(1000);





}


/**
 * @brief Graph::changeRelation
 * Called from MW and Parser
 * @param relation
 */
void Graph::changeRelation(int relation){
    qDebug() << "\n \n \n Graph::changeRelation(int) to relation " << relation
             << " current relation is " << m_curRelation << "\n\n\n";
    if (m_curRelation == relation ) {
        qDebug() << "Graph::changeRelation(int) - same relation - END";
        return;
    }
    if ( relation < 0) {
        qDebug() << "Graph::changeRelation(int) - negative relation - END ";
        return;
    }
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() )
            continue;
       (*it)->changeRelation(relation);
    }
    m_curRelation = relation;
    emit relationChanged(m_curRelation);
    graphModified=true;
    emit graphChanged();
}



/**
 * @brief Graph::addRelationFromUser
 * Called from MW to add a relation and change to that new relation
 * @param newRelation
 */
void Graph::addRelationFromUser(QString newRelation){
    m_relationsList << newRelation;
    qDebug() << "\n\nGraph::addRelationFromUser(string) " << newRelation
                << " total relations now " << relations() << "\n\n";

}

/**
 * @brief Graph::addRelationFromGraph
 * Called when creating random networks
 * emits addRelationToMW
 * @param newRelation
 */
void Graph::addRelationFromGraph(QString newRelation) {
    qDebug() << "Graph::addRelationFromGraph(string) " << newRelation;
    m_relationsList << newRelation;
    emit addRelationToMW(newRelation);
}

/**
 * @brief Graph::addRelationFromParser
 * Called by file parser to add a new relation
 * emits addRelationToMW
 * @param newRelation
 */
void Graph::addRelationFromParser(QString newRelation) {
    qDebug() << "Graph::addRelationFromParser(string) " << newRelation;
    m_relationsList << newRelation;
    emit addRelationToMW(newRelation);
}

/**
 * @brief Graph::currentRelation
 * @return int current relation index
 */
int Graph::currentRelation(){
    return m_curRelation;
}


int Graph::relations(){
    //qDebug () << " relations count " << m_relationsList.count();
    return m_relationsList.count();
}


/**
    main node creation slot, associated with homonymous signal from Parser.
    Adds a Vertex to the Graph and calls addNode of GraphicsWidget
    p holds the desired position of the new node.
    The new Vertex is named i and stores its color, label, label color, shape and position p.
*/
void Graph::createVertex(int i, int size, QString nodeColor, QString numColor,
                         int numSize, QString label, QString lColor, int lSize,
                         QPointF p, QString nodeShape, bool signalMW){
    int value = 1;
    addVertex(i, value, size,  nodeColor, numColor, numSize, label, lColor, lSize, p, nodeShape);
    emit drawNode( i, size,  nodeColor, numColor, numSize, label, lColor, lSize,
                   p, nodeShape, initShowLabels, initNumbersInsideNodes, true);
    if (signalMW)
        emit graphChanged();
    //draw new user-clicked nodes with the same color with that of the file loaded
    initVertexColor=nodeColor;
    initVertexShape=nodeShape;
    initVertexSize=size;
} 



/**
    auxilliary node creation slot.
    Called from GW, with i and p as parameters.
    p holds the desired position of the new node.
    Calls the main creation slot with init node values.
*/
void Graph::createVertex(int i, QPointF p){
    if ( i < 0 )  i = lastVertexNumber() +1;
    qDebug() << "Graph::createVertex() " << i << " fixed coords.";
    createVertex(	i, initVertexSize,  initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number(i), initVertexLabelColor, initVertexLabelSize,
                    p, initVertexShape, true
                    );
}




/**
    second auxilliary node creation slot.
    Called from MW only with parameter i.
    Calculates a random position p from canvasWidth and Height.
    Then calls the main creation slot with init node values.
*/
void Graph::createVertex(int i, int cWidth, int cHeight){
    if ( i < 0 )  i = lastVertexNumber() +1;
    qDebug() << "Graph::createVertex() " << i << " random coords.";
    QPointF p;
    p.setX(rand()%cWidth);
    p.setY(rand()%cHeight);
    createVertex(	i, initVertexSize, initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number(i), initVertexLabelColor, initVertexLabelSize,
                    p, initVertexShape, true
                    );
}



/**
    third auxilliary node creation slot.
    Called from WebCrawler with parameter i.
    Calculates a random position p from canvasWidth and Height.
    Then calls the main creation slot with init node values.
*/

void Graph::createVertexWebCrawler(QString label, int i) {
    if ( i < 0 )  i = lastVertexNumber() +1;
    qDebug() << "Graph::createVertexWebCrawler() " << i << " rand coords with label";
    QPointF p;
    p.setX(rand()%canvasWidth);
    p.setY(rand()%canvasHeight);
    createVertex(	i, initVertexSize,  initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    label, initVertexLabelColor,  initVertexLabelSize,
                    p, initVertexShape, true
                    );

}


void Graph::setCanvasDimensions(int w, int h){
    qDebug() << "Graph:: setCanvasDimensions() to " << w << " " << h ;
    canvasWidth = w;
    canvasHeight= h;
}


/**
 * @brief Graph::createEdge
    Called from homonymous signal of Parser class.
    Adds an Edge to the Graph, then emits drawEdge() which calls
    GraphicsWidget::addEdge() to draw the new edge.
    Also called from MW when user clicks on the "add link" button
    Alse called from GW (via createEdge() below) when user middle-clicks.
 * @param v1
 * @param v2
 * @param weight
 * @param color
 * @param reciprocal
 * @param drawArrows
 * @param bezier
 */
void Graph::createEdge(int v1, int v2, float weight, QString color,
                       int reciprocal=0, bool drawArrows=true, bool bezier=false){
    qDebug()<<"\n\nGraph::createEdge() " << v1 << " -> " << v2
           << " weight " << weight
              << " reciprocal " << reciprocal;
    // check whether there is already such an edge
    // (see #713617 - https://bugs.launchpad.net/socnetv/+bug/713617)
    if (!hasArc(v1,v2)){
        if ( reciprocal == 2) {
            qDebug()<<"  Creating edge as RECIPROCAL - emitting drawEdge signal to GW";
            addEdge ( v1, v2, weight, color, reciprocal);
            emit drawEdge(v1, v2, weight, reciprocal, drawArrows, color, bezier);
        }
        else if (this->hasArc( v2, v1) )  {
            qDebug()<<". Opposite arc exists. "
                   << "  Emitting drawEdgeReciprocal to GW ";
            reciprocal = 1;
            addEdge ( v1, v2, weight, color, reciprocal);
            emit drawEdgeReciprocal(v2, v1);
        }
        else {
            qDebug()<<"  Opposite arc does not exist. Emitting drawEdge to GW...";
            reciprocal = 0;
            addEdge ( v1, v2, weight, color, reciprocal);
            emit drawEdge(v1, v2, weight, reciprocal, drawArrows, color, bezier);
        }
    }
    else {
        qDebug() << "n\nGraph::createEdge() - edge " << v1 << " -> " << v2
                 << " declared previously (exists) - nothing to do \n\n";
    }
    //draw new edges the same color with those of the file loaded,
    // on user clicks on the canvas
    initEdgeColor=color;
    emit graphChanged();
}


/**
    Called (via MW::addLink()) from GW when user middle-clicks on two nodes.
    Calls the above createEdge() method with initEdgeColor to set the default edge color.
*/
void Graph::createEdge(int v1, int v2, float weight, int reciprocal=0,
                       bool drawArrows=true, bool bezier=false){
    qDebug()<< " Graph::createEdge() - " << v1<< " -> " << v2 ;
    createEdge(v1, v2, (float) weight, initEdgeColor, reciprocal,
               drawArrows, bezier);
}


/**
    Called from WebCrawler when it finds an new link
    Calls the above createEdge() method with initEdgeColor
*/
void Graph::createEdgeWebCrawler (int source, int target){
    qDebug()<< " Graph::createEdgeWebCrawler() - from " << source << " to " << target ;
    float weight = 1.0;
    bool reciprocal=false;
    bool drawArrows=true;
    bool bezier=false;

    createEdge(source, target, weight, initEdgeColor, reciprocal, drawArrows, bezier);
}


/**
 * @brief Graph::removeDummyNode
 * This is called from loadPajek method of Parser in order to delete any
 * redundant (dummy) nodes.
 * @param i
 */
void Graph::removeDummyNode(int i){
    qDebug("**Graph: RemoveDummyNode %i", i);
    removeVertex(i);
    //	emit selectedVertex(i);

}



/**
 * @brief Graph::addVertex
 * Adds a Vertex named v1, valued val, sized nszm colored nc, labeled nl,
 * labelColored lc, shaped nsp, at point p.
 * This method is called by createVertex() method
 * @param v1
 * @param val
 * @param size
 * @param color
 * @param numColor
 * @param numSize
 * @param label
 * @param labelColor
 * @param labelSize
 * @param p
 * @param shape
 */
void Graph::addVertex (
        int v1, int val, int size, QString color,
        QString numColor, int numSize,
        QString label, QString labelColor, int labelSize,
        QPointF p, QString shape
        ){

    qDebug() << "Graph::addVertex() ";
    if (order)
        index[v1]=m_totalVertices;
    else
        index[v1]=m_graph.size();

    m_graph.append(
                new Vertex
                (this, v1, val, size, color, numColor, numSize, label,
                 labelColor, labelSize, p, shape
                 )
                );
    m_totalVertices++;

//    qDebug() << "Graph: addVertex(): Vertex named " << m_graph.back()->name()
//             << " appended with index= "<<index[v1]
//             << " Now, m_graph size " << m_graph.size()
//             << ". New vertex position: " << p.x() << "," << p.y();
    graphModified=true;
}




/**
    updates MW  with the file type (0=nofile, 1=Pajek, 2=Adjacency etc)
*/
void Graph::setFileType (
        int type, QString networkName, int aNodes, int totalLinks, bool undirected)
{
    qDebug("Graph: setFileType %i", type);
    m_undirected = undirected;
    emit signalFileType (type, networkName, aNodes, totalLinks, m_undirected);
    qDebug ()<< "Graph::setFileType()  -check parser if running...";

}



/**	Returns the name of the last vertex.
    Used by slotRemoveNode of MW
*/
int Graph::lastVertexNumber() {
    if (m_totalVertices>0)
        return m_graph.back()->name();
    else return 0;
}


/**	Returns the name of the first vertex. 
    Used by slotRemoveNode of MW
*/
int Graph::firstVertexNumber() {
    if (m_totalVertices>0)
        return m_graph.front()->name();
    else return 0;
}



/**	Removes the vertex named Doomed from the graph 
    First, it removes all edges to Doomed from other vertices
    Then it changes the index of all subsequent vertices inside m_graph
    Finally, it removes the vertex.
*/
void Graph::removeVertex(long int Doomed){
    qDebug() << "Graph: removeVertex - Doomed: "
             << m_graph[ index[Doomed] ]->name()
             << "  indexOfDoomed= " << index[Doomed] ;
    long int indexOfDoomed=index[Doomed];

    //Remove links to Doomed from each other vertex
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if  ( (*it)->hasEdgeTo(Doomed) != 0) {
            qDebug()<< "Graph: Vertex " << (*it)->name()
                    << " is linked to doomed "<< Doomed << " and has "
                    << (*it)->outEdges() << " and " <<  (*it)->outDegree() ;
            if ( (*it)->outEdges() == 1 && (*it)->hasEdgeFrom(Doomed) != 0 )	{
                qDebug() << "Graph: decreasing reciprocalEdgesVert";
                (*it)->setReciprocalLinked(false);
            }
            (*it)->removeEdgeTo(Doomed) ;
        }
        if (  (*it)->hasEdgeFrom(Doomed) != 0 ) {
            (*it)->removeEdgeFrom(Doomed);
        }
    }

    qDebug()<< "Graph: Finished with vertices. Update the index which maps vertices inside m_graph " ;
    long int prevIndex=indexOfDoomed;

    qDebug () << " Updating index of all subsequent vertices ";
    H_Int::const_iterator it1=index.cbegin();
    while (it1 != index.cend()){
        if ( it1.value() > indexOfDoomed ) {
            prevIndex = it1.value();
            qDebug() << "Graph::removeVertex - vertex " << it1.key()
                     << " had prevIndex: " << prevIndex
                     << " > indexOfDoomed " << indexOfDoomed
                     << " Setting new index. Index size was: "<< index.size();
            index.insert( it1.key(), --prevIndex)  ;
            qDebug() << "Graph::removeVertex - vertex " << it1.key()
                     << " new index: " << index.value( it1.key(), -666)
                     << " Index size now: "<< index.size();

        }
        else {
            qDebug() << "Graph::removeVertex " << it1.key() << " with index "
                     << it1.value() << " < indexOfDoomed. CONTINUE";

        }
        ++it1;
    }

    //Now remove vertex Doomed from m_graph
    qDebug() << "Graph: graph vertices=size="<< vertices() << "="
             << m_graph.size() <<  " removing vertex at index " << indexOfDoomed ;
    m_graph.removeAt( indexOfDoomed ) ;
    m_totalVertices--;
    qDebug() << "Graph: Now graph vertices=size="<< vertices() << "="
             << m_graph.size() <<  " total edges now  " << totalEdges();

    order=false;
    graphModified=true;

    emit graphChanged();
    emit eraseNode(Doomed);
}



/**	Creates an edge between v1 and v2
*/
void Graph::addEdge (int v1, int v2, float weight, QString color, int reciprocal) {

    int source=index[v1];
    int target=index[v2];

    qDebug()<< "Graph: addEdge() from vertex "<< v1 << "["<< source
            << "] to vertex "<< v2 << "["<< target << "] of weight "<<weight;

    m_graph [ source ]->addEdgeTo(v2, weight );
    m_graph [ target ]->addEdgeFrom(v1, weight);
    m_totalEdges++;

    if (reciprocal == 1){
        m_graph [ source ]->setReciprocalLinked(true);
        m_graph [ target ]->setReciprocalLinked(true);
    }
    else if (reciprocal == 2){
        m_graph [ source ]->setReciprocalLinked(true);
        m_graph [ target ]->setReciprocalLinked(true);
        m_graph [ target ]->addEdgeTo(v1, weight );
        m_graph [ source ]->addEdgeFrom(target, weight);
        m_totalEdges++;
    }


    qDebug()<<"Graph: addEdge() now a("<< v1 << ","<< v2<< ") = " << weight
           << " with color "<<  color
           <<" . Storing edge color..." << ". Total Links " <<m_totalEdges;
    m_graph[ source]->setOutLinkColor(v2, color);

    graphModified=true;
}




/**	
    Change edge (arc) weight between v1 and v2
*/
void Graph::setArcWeight (const long &v1, const long &v2, const float &weight) {
    qDebug() << "Graph::setArcWeight between " << v1 << "[" << index[v1]
                << "] and " << v2 << "[" << index[v2] << "]" << " = " << weight;
    m_graph [ index[v1] ]->changeOutEdgeWeight(v2, weight);
    graphModified=true;
    emit graphChanged();

}


/** 	Removes the edge (arc) between v1 and v2
*/
void Graph::removeEdge (int v1, int v2) {	
    qDebug ()<< "\n\n Graph::removeEdge() edge from " << v1 << " index " << index[v1]
                << " to " << v2 << " to be removed from graph";
    m_graph [ index[v1] ]->removeEdgeTo(v2);
    m_graph [ index[v2] ]->removeEdgeFrom(v1);
    qDebug()<< "Graph: removeEdge between " << v1 << " i " << index[v1]
               << " and " << v2 << " i "<< index[v2]
               << "  NOW vertex v1 reports edge weight "
               << m_graph [ index[v1] ]->hasEdgeTo(v2) ;
    if ( this->hasArc(v2,v1) !=0)
        symmetricAdjacencyMatrix=false;

    m_totalEdges--;
    if (m_totalEdges<0) //crazy check :)
        m_totalEdges=0;
    graphModified=true;

    emit eraseEdge(v1,v2);
    emit graphChanged();
}





//Called by MW to start a web crawler...
void Graph::webCrawl( QString seed, int maxNodes, int maxRecursion,
                      bool extLinks, bool intLinks){

    qDebug() << "Graph::webCrawl() - seed " << seed ;
    //WebCrawler *crawler = new WebCrawler;

    qDebug() << "Graph::webCrawl() Creating wc_spider & wc_parser objects";
    WebCrawler_Parser *wc_parser = new WebCrawler_Parser(seed, maxNodes,
                                                         maxRecursion,
                                                         extLinks,
                                                         intLinks);
    WebCrawler_Spider *wc_spider = new WebCrawler_Spider (seed, maxNodes,
                                                          maxRecursion,
                                                          extLinks, intLinks);

    qDebug() << "Graph::webCrawl()  Moving parser & spider to new QThreads!";
    qDebug () << " graph thread  " << this->thread();
    qDebug () << " wc_parser thread  " << wc_parser->thread();
    qDebug () << " wc_spider thread  " << wc_spider->thread();
    wc_parser->moveToThread(&wc_parserThread);
    wc_spider->moveToThread(&wc_spiderThread);
    qDebug () << " graph thread is " << this->thread();
    qDebug () << " wc_parser thread now " << wc_parser->thread();
    qDebug () << " wc_spider thread now " << wc_spider->thread();


    qDebug() << "Graph::webCrawl()  Connecting signals from/to parser & spider";
    connect(&wc_parserThread, &QThread::finished,
            wc_parser, &QObject::deleteLater);

    connect(&wc_spiderThread, &QThread::finished,
            wc_spider, &QObject::deleteLater);

    connect(this, &Graph::operateSpider,
             wc_spider, &WebCrawler_Spider::get);

    connect(wc_parser, &WebCrawler_Parser::signalCreateNode,
            this, &Graph::createVertexWebCrawler);

    connect(wc_parser, &WebCrawler_Parser::signalCreateEdge,
            this, &Graph::createEdgeWebCrawler);

    connect (wc_spider, &WebCrawler_Spider::finished,
             this, &Graph::terminateCrawlerThreads);

    connect (wc_parser, &WebCrawler_Parser::finished,
             this, &Graph::terminateCrawlerThreads);

    connect (wc_spider, &WebCrawler_Spider::parse,
                 wc_parser, &WebCrawler_Parser::parse );

    connect (wc_parser, &WebCrawler_Parser::startSpider,
             wc_spider, &WebCrawler_Spider::get );


    qDebug() << "Graph::webCrawl()  Starting parser & spider QThreads!";
    wc_parserThread.start();
    wc_spiderThread.start();

    qDebug() << "Graph::webCrawl()  Creating initial node 1, url: " << seed;
    createVertexWebCrawler(seed, 1);

    qDebug() << "Graph::webCrawl()  calling spider get() for that url!";
    emit operateSpider();

    qDebug("Graph::webCrawl() - reach the end - See the threads running? ");
}


//called from Graph, when closing network, to terminate all processes
//also called indirectly when wc_spider finishes
void Graph::terminateCrawlerThreads (QString reason){
    qDebug() << "Graph::terminateCrawlerThreads() - reason " << reason;
    qDebug() << "Graph::terminateCrawlerThreads()  check if wc_parserThread is running...";
    if (wc_parserThread.isRunning() ) {
         qDebug() << "Graph::terminateCrawlerThreads()  parser thread quit";
        wc_parserThread.quit();
        qDebug() << "Graph::terminateCrawlerThreads() - deleting wc_parser pointer";
        delete wc_parser;
        wc_parser = 0;  // see why here: https://goo.gl/tQxpGA

    }
    qDebug() << "Graph::terminateCrawlerThreads()  check if wc_spiderThread is running...";
    if (wc_spiderThread.isRunning() ) {
        qDebug() << "Graph::terminateCrawlerThreads()  spider thread quit";
        wc_spiderThread.quit();
        qDebug() << "Graph::terminateCrawlerThreads() - deleting wc_spider pointer";
        delete wc_spider;
        wc_spider= 0;  // see why here: https://goo.gl/tQxpGA

        emit signalNodeSizesByInDegree(true);
     }




}



/**
    Called from filterOrphanNodes via MainWindow  to filter nodes with no links
    For each orphan Vertex in the Graph, emits the filterVertex()
*/
void Graph::filterIsolateVertices(bool filterFlag){
    qDebug() << "*** Graph::filterIsolateVertices() "
                << " setting all isolate nodes to " << filterFlag;

    QList<Vertex*>::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( !(*it)->isIsolated() ){
            continue;
        }
        else {
            qDebug() << "Graph::filterOrphanNodes() Vertex " << (*it)->name()
                     << " isolate. Toggling it and emitting setVertexVisibility signal to GW...";
            (*it)->setEnabled (filterFlag) ;
            graphModified=true;
            emit graphChanged();
            emit setVertexVisibility( (*it)-> name(), filterFlag );
        }
    }
}




/**
    Called from filterEdgesDialog via MainWindow
    to filter edges over or under a specified weight (m_threshold)
    For each Vertex in the Graph, calls the homonymous method of Vertex class.
*/
void Graph::filterEdgesByWeight(float m_threshold, bool overThreshold){
    if (overThreshold)
        qDebug() << "Graph: filterEdgesByWeight() over " << m_threshold ;
    else
        qDebug() << "Graph: filterEdgesByWeight()  below "<< m_threshold ;

    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
            (*it)->filterEdgesByWeight ( m_threshold, overThreshold );
    }
    graphModified=true;
    emit graphChanged();
    emit statusMessage("Edges have been filtered.");
}



/**
 * @brief Graph::filterEdgesByRelation
 * Not called by Called from MW to filter out all edges of a given relation
 * calls the homonymous method of Vertex class.
 * @param relation
  */
void Graph::filterEdgesByRelation(int relation, bool status){
    qDebug() << "Graph::filterEdgesByRelation() " ;
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() )
            continue;
       (*it)->filterEdgesByRelation ( relation, status );
    }
}



void Graph::slotSetEdgeVisibility ( int relation,  int source, int target, bool visible) {
    //qDebug() << "Graph: slotSetEdgeVisibility  - emitting signal to GW";
    emit setEdgeVisibility ( relation, source, target, visible);
}


/**	Checks if there is a specific vertex in the graph
    Returns the index or -1
    Complexity:  O(logN) for index retrieval
*/
int Graph::hasVertex(long int num){
    qDebug () << "Graph: hasVertex() v: " << num <<  " with index " << index[num]  << " named " << m_graph[ index[num]] ->name();
    if (  m_graph[ index[num]] ->name() == num)
        return index[num];
    else
        return -1;
}



/**	Checks if there is a vertex with a specific label in the graph
    Returns the index or -1
    Complexity:  O(N)
*/
int Graph::hasVertex(QString label){			
    qDebug ()<<"Graph: hasVertex( "<< label.toUtf8() <<" ) ?" ;
    QList<Vertex*>::const_iterator it;
    int i=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( (*it) ->label() == label)  {
            qDebug()<< "Graph: hasVertex() at pos %i" << i;
            return i;
        }
        i++;
    }
    qDebug("Graph: hasVertex() NO - returning -1");
    return -1;
}




void Graph::setInitVertexSize (const long int size) {
    initVertexSize=size;
}


//Changes the size.of vertex v 
void Graph::setVertexSize(const long int &v, const int &size) {
    m_graph[ index[v] ]->setSize(size);
    graphModified=true;
    emit graphChanged();
    emit setNodeSize(v, size);
}

int Graph::vertexSize(const long &v ) {
    return m_graph[ index[v] ]-> size();
}

//Changes the size.of all vertices
void Graph::setAllVerticesSize(const int &size) {
    setInitVertexSize(size);
    QList<Vertex*>::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ){
            continue;
        }
        else {
            (*it)->setSize(size) ;
            emit setNodeSize((*it)->name(), size);
        }
    }
    graphModified=true;
    emit graphChanged();
}


void Graph::setInitVertexShape(const QString shape) {
    initVertexShape=shape;
}

//Changes the shape.of vertex v 
void Graph::setVertexShape(const int v1, const QString shape){
    m_graph[ index[v1] ]->setShape(shape);
    emit setNodeShape( v1, shape);
    graphModified=true;
    emit graphChanged();
}


//returns the shape of this vertex
QString Graph::vertexShape(const int &v1){
    return m_graph[ index[v1] ]->shape();

}

void Graph::setAllVerticesShape(const QString shape) {
    setInitVertexShape(shape);
    QList<Vertex*>::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ){
            continue;
        }
        else {
            (*it)->setShape(shape);
            emit setNodeShape((*it)->name(), shape);
        }
    }
    graphModified=true;
    emit graphChanged();
}


//Changes the initial color of vertices numbers 
void Graph::setInitVertexNumberColor (QString color) {
    initVertexNumberColor = color;
}

//Changes the initial size of vertices numbers 
void Graph::setInitVertexNumberSize (int size) {
    initVertexNumberSize = size;
}



/**Changes the label.of vertex v  */
void Graph::setVertexLabel(int v1, QString label){
    qDebug()<< "Graph: setVertexLabel for "<< v1 << ", index " << index[v1]<< " with label"<< label;
    m_graph[ index[v1] ]->setLabel ( label);
    emit setNodeLabel ( m_graph[ index[v1] ]-> name(), label);
    graphModified=true;
    emit graphChanged();
}



//Changes the init size of new vertices labels
void Graph::setInitVertexLabelSize(int newSize) {
    initVertexLabelSize = newSize;
}


//Changes the size of a vertex label
void Graph::setVertexLabelSize(int v1, int newSize) {
    qDebug()<< "Graph: setVertexLabelSize for "<< v1 << ", index " << index[v1]<< " with size "<< newSize;
    m_graph[ index[v1] ] -> setLabelSize ( newSize );
    graphModified=true;
    emit graphChanged();

}



//Changes the shape.of vertex v 
void Graph::setVertexLabelColor(int v1, QString color){
    m_graph[ index[v1] ]->setLabelColor(color);
    graphModified=true;
    emit graphChanged();
}


void Graph::setInitVertexLabelColor(QString color){
    initVertexLabelColor=color;
}


QString Graph::vertexLabel(const long int &v1){
    return m_graph[ index[v1] ]->label ();
}


/**
    Changes the color of vertex v1
*/
void Graph::setVertexColor(const long int &v1, const QString &color){
    qDebug()<< "Graph: setVertexColor for "<< v1 << ", index " << index[v1]<< " with color "<< color;
    m_graph[ index[v1] ]->setColor ( color );
    emit setNodeColor ( m_graph[ index[v1] ]-> name(), color );
    graphModified=true;
    emit graphChanged();
}

QColor Graph::vertexColor(const long int &v1){
    return  QColor ( m_graph[ index[v1] ] -> color() ) ;
}

void Graph::setInitVertexColor(const QString &color){
    initVertexColor=color;
}


void Graph::setAllVerticesColor(const QString &color) {
    qDebug() << "*** Graph::setAllVerticesColor() "
                << " to " << color;
    setInitVertexColor(color);
    QList<Vertex*>::const_iterator it;
    for ( it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ){
            continue;
        }
        else {
            qDebug() << "Graph::setAllVerticesColor() Vertex " << (*it)->name()
                     << " new color " << color;
            (*it)->setColor(color) ;
            emit setNodeColor ( (*it)-> name(), color );
        }
    }
    graphModified=true;
    emit graphChanged();

}



void Graph::setInitEdgeColor(const QString &color){
    initEdgeColor=color;
}




//Returns the edgeColor
QString Graph::edgeColor (const long &v1, const long &v2){
    return m_graph[ index[v1] ]->outLinkColor(v2);
}


/**
    Changes the color of all edges.
*/
bool Graph::setAllEdgesColor(const QString &color){
    qDebug()<< "\n\nGraph::setAllEdgesColor()" << color;
    int target=0, source=0;
    setInitEdgeColor(color);
    QHash<int,float> *enabledOutEdges = new QHash<int,float>;
    QHash<int,float>::const_iterator it1;
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        //updateProgressDialog(++count);
        source = (*it)->name();
        if ( ! (*it)->isEnabled() )
            continue;
        enabledOutEdges=(*it)->returnEnabledOutEdges();
        it1=enabledOutEdges->cbegin();
        while ( it1!=enabledOutEdges->cend() ){
            target = it1.key();
            qDebug() << "=== Graph::setAllEdgesColor() : "
                        << source << "->" << target << " new color " << color;
            (*it)->setOutLinkColor(target, color);
            emit setLinkColor(source, target, color);
            ++it1;
        }
    }
    delete enabledOutEdges;
    graphModified=true;
    emit graphChanged();
    return true;

}


/**
    Changes the color of edge (s,t).
*/
void Graph::setEdgeColor(const long &v1, const long &v2, const QString &color){
    qDebug()<< "\n\n === Graph::setEdgeColor() "<< v1 << " -> "<< v2
            <<" with index ("<< index[v1]<< " -> "<<index[v2]<<")"
           <<" new color "<< color;
    m_graph[ index[v1] ]->setOutLinkColor(v2, color);
    emit setLinkColor(v1, v2, color);
    if (isSymmetric()) {
        m_graph[ index[v2] ]->setOutLinkColor(v1, color);
        emit setLinkColor(v2, v1, color);
    }
    graphModified=true;
    emit graphChanged();
}




/**	Checks if there is a directed edge (arc) from v1 to v2
    Complexity:  O(logN) for index retrieval + O(1) for QList index retrieval + O(logN) for checking edge(v2)
*/
float Graph::hasArc (const long int &v1, const long int &v2) {
    //qDebug() << "Graph::hasArc() " << v1 << " -> " << v2 << " ? " ;
    return m_graph[ index[v1] ]->hasEdgeTo(v2);
}

/**	Checks if there is a edge between v1 and v2 (both arcs exist)
*/
bool Graph::hasEdge (const int &v1, const long &v2) {
    qDebug() << "Graph::hasEdge() " << v1 << " <-> " << v2 << " ? " ;
    return ( ( m_graph[ index[v1] ]->hasEdgeTo(v2) != 0 )
            && ( m_graph[ index[v2] ]->hasEdgeTo(v1) != 0) ) ? true: false;
}


void Graph::edges(){
    H_edges::const_iterator it1;
    QList<Vertex*>::const_iterator it;
    int  relation=0,source=0, target=0, w=0;
    float weight=0;
    bool edgeStatus=false;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
        if ( ! (*it)->isEnabled() )
            continue ;
        source = index[ (*it)->name() ];
        it1=m_graph [ source ] ->m_outEdges.cbegin();
        while ( it1!=m_graph [ source ] -> m_outEdges.cend() ){
            relation = it1.value().first;
            if ( relation != currentRelation() )  {
                ++it1;
                continue;
            }
            edgeStatus=it1.value().second.second;
            if ( edgeStatus != true)   {
                ++it1;
                continue;
            }
            target = it1.key();
            w=index[ target ];
            weight = it1.value().second.first;
            qDebug("u=%i is connected with node %i of index w=%i. ", source, target, w);
            ++it1;
        }
    }

}

/**
    Called from MainWindow
*/
void Graph::updateVertCoords(int v1, int  x, int y){
    //qDebug("Graph: updateVertCoords() for %i with index %i with %i, %i", v1, index[v1], x,y);
    m_graph[ index[v1] ]->setX( x );
    m_graph[ index[v1] ]->setY( y );
    graphModified=true;
}



/**
 * @brief Graph::outboundEdges
 * *Returns the number of outbound edges (arcs) from vertex v1
 * @param v1
 * @return
 */
int Graph::outboundEdges(int v1) {
    qDebug("Graph: outboundEdges()");
    return m_graph[ index[v1] ]->outEdges();
}


/**
 * @brief Graph::inboundEdges
 * Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::inboundEdges (int v1) {
    qDebug("Graph: inboundEdges()");
    return m_graph[ index[v1] ]->inEdges();
}




/**
 * @brief Graph::outDegree
 * Returns the outDegree (sum of outEdges weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::outDegree (int v1) {
    qDebug("Graph: outDegree()");
    return m_graph[ index[v1] ]->outDegree();
}


/**
    Returns the inDegree (sum of inEdges weights) of vertex v1
*/
int Graph::inDegree (int v1) {
    qDebug("Graph: inDegree()");
    return m_graph[ index[v1] ]-> inDegree();
}



/** 
    Returns |E| of graph
*/
int Graph::totalEdges () {
    qDebug("Graph: totalEdges()");
    int tEdges=0;
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        tEdges+=(*it)->outEdges();
    }
    qDebug() << "Graph: m_totalEdges = " << m_totalEdges << ", tEdges=" <<  tEdges;
    return tEdges;
}


/**	
    Returns |V| of graph
*/
int Graph::vertices(const bool dropIsolates, const bool countAll) {
    qDebug("Graph: vertices()");
    m_totalVertices=0;
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (countAll) {
            ++m_totalVertices;
        }
        else {
            if (dropIsolates && (*it)->isIsolated())
                continue;
                ++m_totalVertices;
        }
    }
    return m_totalVertices;
}

/**
    Returns a list of all isolated vertices inside the graph
*/
QList<int> Graph::verticesIsolated(){
    qDebug()<< "Graph::verticesIsolated()";
    if (!graphModified){
        return m_isolatedVerticesList;
    }
    QList<Vertex*>::const_iterator it;
    m_isolatedVerticesList.clear();
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
//        if ( ! (*it)->isEnabled() )
//            continue;
        if ((*it)->isIsolated()) {
            m_isolatedVerticesList << (*it)->name();
            qDebug()<< "Graph::verticesIsolated() - node " << (*it)->name()
                    << " is isolated. Marking it." ;
        }
    }
    return m_isolatedVerticesList ;
}


/**
 *  Returns ratio of present edges to total possible edges.
 */
float Graph::density() {
    qDebug("Graph: density()");
    int vert=vertices();
    if (vert!=0 && vert!=1)
        return  (float) totalEdges() / (float)(vert*(vert-1.0));
    else return 0;
}


/**
 *  Checks if the graph is weighted, i.e. if any e in |E| has value > 1
 *  O(n^2)
 */
bool Graph::isWeighted(){
    qDebug("Graph: isWeighted()");
    QList<Vertex*>::const_iterator it, it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
       for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ( this->hasArc ( (*it1)->name(), (*it)->name() ) )  > 1  )   {
                qDebug("Graph: isWeighted: TRUE");
                return true;
            }
        }
    }
    qDebug("Graph: isWeighted: FALSE");
    return false;
}


/**
    Returns the sum of vertices having outboundEdges
*/
int Graph::verticesWithOutboundEdges(){
    return outboundEdgesVert;
}

/**
    Returns the sum of vertices having inboundEdges
*/
int Graph::verticesWithInboundEdges(){
    return inboundEdgesVert;
}


/**
    Returns the sum of vertices having reciprocal edges
*/
int Graph:: verticesWithReciprocalEdges(){
    return reciprocalEdgesVert;
}

/** 
    Clears all vertices
*/
void Graph::clear() {
   qDebug("Graph::clear() m_graph reports size %i", m_graph.size());
    qDeleteAll(m_graph.begin(), m_graph.end());
    m_graph.clear();
    index.clear();
    //clear relations
    m_relationsList.clear();
    m_curRelation=0;

    discreteDPs.clear(); discreteDCs.clear(); discreteCCs.clear();
    discreteBCs.clear(); discreteSCs.clear(); discreteIRCCs.clear();
    discreteECs.clear(); discreteEccentricities.clear();
    discretePCs.clear(); discreteICs.clear();  discretePRPs.clear();
    discretePPs.clear();
    if ( DM.size() > 0) {
        qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        DM.clear();
    }
    if ( TM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        TM.clear();
    }
    if ( sumM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        sumM.clear();
    }
    if ( invAM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        invAM.clear();
    }
    if ( AM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        AM.clear();
    }
    if ( invM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        invM.clear();
    }
    if ( XM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        XM.clear();
    }
    if ( XSM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        XSM.clear();
    }
    if ( XRM.size() > 0) {
                qDebug() << "\n\n\n\n Graph::clear()  clearing DM\n\n\n";
        XRM.clear();
    }


    m_isolatedVerticesList.clear();
    disconnectedVertices.clear();
    unilaterallyConnectedVertices.clear();
    influenceDomains.clear();
    influenceRanges.clear();
    triadTypeFreqs.clear();

    m_totalVertices=0;
    m_totalEdges=0;
    outboundEdgesVert=0;
    inboundEdgesVert=0;
    reciprocalEdgesVert=0;

    order=true;		//returns true if the indexes of the list is ordered.
    m_undirected=false;
    calculatedDP=false;
    calculatedDC=false;
    calculatedIC=false;
    calculatedCentralities=false;
    calculatedIRCC=false;
    calculatedPP=false;
    calculatedPRP=false;
    calculatedTriad=false;
    adjacencyMatrixCreated=false;
    reachabilityMatrixCreated=false;
    graphModified=false;
    symmetricAdjacencyMatrix=true;

    qDebug ()<< "Graph::clear()  -Do parser threads run ?";
    terminateParserThreads("Graph::initNet()");

    qDebug ()<< "Graph::clear()  -Do web crawler threads run ?";
    terminateCrawlerThreads("Graph::initNet");


    qDebug("Graph: m_graph cleared. Now reports size %i", m_graph.size());
}



/**
 * @brief Graph::isSymmetric
 * Returns TRUE if the adjacency matrix of the current relation is symmetric
 * @return bool
 */
bool Graph::isSymmetric(){
    qDebug() << "Graph::isSymmetric() ";
    if (!graphModified){
        return symmetricAdjacencyMatrix;
    }
    symmetricAdjacencyMatrix=true;
    int y=0, v2=0, v1=0;

    QHash<int,float> *enabledOutEdges = new QHash<int,float>;

    QHash<int,float>::const_iterator hit;
    QList<Vertex*>::const_iterator lit;


    for ( lit = m_graph.cbegin(); lit != m_graph.cend(); ++lit)
    {
        v1 = (*lit) -> name();

        if ( ! (*lit)->isEnabled() )
            continue;
        qDebug() << "Graph::isSymmetric() - Graph modified! " <<
                    " Iterate over all edges of " << v1 ;

        enabledOutEdges=(*lit)->returnEnabledOutEdges();

        hit=enabledOutEdges->cbegin();

        while ( hit!=enabledOutEdges->cend() ){
            v2 = hit.key();
            y=index[ v2 ];
            float weight = hit.value();
            if (  m_graph[y]->hasEdgeTo( v1) != weight) {
                symmetricAdjacencyMatrix=false;
//                qDebug() <<"Graph::isSymmetric() - "
//                         << " graph not symmetric because "
//                         << v1 << " -> " << v2 << " weight " << weight
//                         << " differs from " << v2 << " -> " << v1 ;

                break;
            }
            ++hit;
        }
    }
    delete enabledOutEdges;
    qDebug() << "Graph: isSymmetric() -"  << symmetricAdjacencyMatrix;
    return symmetricAdjacencyMatrix;
}



/**
*	Transform the digraph to undirected graph (all edges reciprocal)
*/
void Graph::symmetrize(){
    qDebug("Graph: symmetrize");
    QList<Vertex*>::const_iterator it;
    int y=0, v2=0, v1=0, weight;
    QHash<int,float> *enabledOutEdges = new QHash<int,float>;
    QHash<int,float>::const_iterator it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        v1 = (*it)->name();
        qDebug() << "Graph:symmetrize() - iterate over edges of v1 " << v1;
        enabledOutEdges=(*it)->returnEnabledOutEdges();
        it1=enabledOutEdges->cbegin();
        while ( it1!=enabledOutEdges->cend() ){
            v2 = it1.key();
            weight = it1.value();
            y=index[ v2 ];
            qDebug() << "Graph:symmetrize() - "
                     << " v1 " << v1
                     << " outLinked to " << v2 << " weight " << weight;
            if (  m_graph[y]->hasEdgeTo( v1 ) == 0 ) {
                qDebug() << "Graph:symmetrize(): s = " << v1
                         << " is NOT inLinked from y = " <<  v2  ;
                createEdge( v2, v1, weight, initEdgeColor, false, true, false);
            }
            else
                qDebug() << "Graph: symmetrize(): v1 = " << v1
                         << " is already inLinked from v2 = " << v2 ;
            ++it1;
        }
    }
    delete enabledOutEdges;
    graphModified=true;
    symmetricAdjacencyMatrix=true;
    emit graphChanged();
}


//Returns TRUE if (v1, v2) is symmetric.
bool Graph::symmetricEdge(int v1, int v2){
    qDebug("***Graph: symmetricEdge()");
    if ( (this->hasArc ( v1, v2 ) ) > 0  &&  (this->hasArc ( v2, v1 ) ) > 0   ) {
        qDebug("Graph: symmetricEdge: YES");
        return true;
    }
    else {
        qDebug("Graph: symmetricEdge: NO");
        return false;
    }

}




/**
*  Returns the distance between nodes numbered (i-1) and (j-1)
*/
int Graph::distance(const int i, const int j,
                    const bool considerWeights,
                    const bool inverseWeights){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false, considerWeights, inverseWeights, false);
    }
    return DM.item(index[i],index[j]);
}



/**
*  Returns the diameter of the graph, aka the largest geodesic distance between any two vertices
*/
int Graph::diameter(const bool considerWeights,
                    const bool inverseWeights){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false, considerWeights, inverseWeights, false);
    }
    return graphDiameter;
}



/**
*  Returns the average distance of the graph
*/
float Graph::averageGraphDistance(const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool dropIsolates){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false, considerWeights, inverseWeights,dropIsolates);
    }
    return averGraphDistance;
}


/**
 * @brief Graph::connectedness()
 * @return int:

 * 2: strongly connected digraph (exists path from i to j and vice versa for every i,j)
 * 1: connected undirected graph
 * 0: not connected undirected graph no isolates
 * -1: not connected undirected graph with isolates
 * -2: unilaterally connected digraph (exists path only from i to j or from j to i, not both)
 * -3  disconnected digraph (with isolates).
 * -4  disconnected digraph (there are pairs not connected at all).
 */
int Graph::connectedness() {
    qDebug() << "Graph::connectedness() ";
    if (!reachabilityMatrixCreated || graphModified) {
        reachabilityMatrix(false,false,false);
    }
    isolatedVertices=verticesIsolated().count();
    if ( isSymmetric() ) {
        qDebug() << "Graph::connectedness() IS SYMMETRIC";
        if ( disconnectedVertices.size() != 0 ) {
            if (isolatedVertices!=0 ) {
                qDebug() << "undirected graph is disconnected  (has isolates)" ;
                return -1;

            }
            else
            {
                qDebug() << " undirected graph is disconnected (no isolates)";
                return 0;
            }
        }
        qDebug() << " undirected graph is connected ";
        return 1;
    }
    else {
        qDebug() << "Graph::connectedness() NOT SYMMETRIC";
        if ( disconnectedVertices.size() != 0 ) {
            if ( unilaterallyConnectedVertices.size() == 0 ) {
                if (isolatedVertices!=0) {
                    qDebug() << " directed graph is disconnected (has isolates)";
                    return -3; // - can be connected directed if we remove isolate nodes
                }
            }
            qDebug () << " directed graph is disconnected (no isolates)";
            return -4;
        }
        else {
            if ( unilaterallyConnectedVertices.size() != 0 ) {
                qDebug () << " directed graph is unilaterally connected";
                return -2; // (exists path only from i to j or from j to i, not both)
            }
            else{
                qDebug () << " directed graph is connected ";
                return 2;
            }
        }

    }
    return -666; // for sanity check :P
}


/**
*  Writes the matrix of distances to a file
*/
void Graph::writeDistanceMatrix (QString fn, const char* netName,
                                 const bool considerWeights,
                                 const bool inverseWeights,
                                 const bool dropIsolates) {
    qDebug ("Graph::writeDistanceMatrix()");

    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false, considerWeights, inverseWeights, dropIsolates);
    }

    qDebug ("Graph::writeDistanceMatrix() writing to file");

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream outText(&file);
    outText.setCodec("UTF-8");
    outText.setRealNumberPrecision(m_precision);
    outText << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    outText << "Distance matrix of "<< netName<<": \n";

    outText << DM ;

    file.close();
}


/**
*  Saves the number of geodesic distances matrix TM to a file
*
*/
void Graph::writeNumberOfGeodesicsMatrix(const QString fn,
                                         const char* netName,
                                         const bool considerWeights,
                                         const bool inverseWeights) {
    qDebug ("Graph::writeDistanceMatrix()");
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false, considerWeights, inverseWeights, false);
    }

    qDebug ("Graph::writeDistanceMatrix() writing to file");

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream outText(&file);
    outText.setCodec("UTF-8");
    outText << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    outText << "Number of geodesics matrix of  "<< netName<<": \n";

    outText << TM ;

    file.close();

}



void Graph::writeEccentricity(
        const QString fileName, const bool considerWeights=false,
        const bool inverseWeights=false, const bool dropIsolates=false)
{

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );
    outText.setCodec("UTF-8");
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true, considerWeights,
                             inverseWeights, dropIsolates);
    }
    emit statusMessage ( QString(tr("Writing eccentricity to file:")).arg(fileName) );

    outText << tr("ECCENTRICITY (e)") << endl << endl;
    outText << tr("The eccentricity e of a node is the maximum geodesic distance "
                  " from that node to all other nodes in the network.") ;
    outText << endl  ;
    outText << tr("Therefore, e reflects farness: how far, at most, is each "
                  " node from every other node.") ;
    outText << endl  ;
    outText << tr("A node has maximum e when it has distance 1 "
          "to all other nodes (star node))\n");

    outText << endl << endl ;

    outText << tr("Range: 0 < e < ") << vertices()-1 <<" (g-1, "
             << tr("where g is the number of nodes |V|)");

    outText << endl << endl ;
    outText << "Node"<<"\te\t%e\n";
    QList<Vertex*>::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){
        outText << (*it)->name()<<"\t"<<(*it)->eccentricity() << "\t" <<
                   (100* ((*it)->eccentricity()) / sumEccentricity)<<endl;
    }
    if ( minEccentricity ==  maxEccentricity)
        outText << tr("\nAll nodes have the same e value.\n");
    else {
        outText << "\n";
        outText << tr("Max e = ") << maxEccentricity
                <<" (node "<< maxNodeEccentricity  <<  ")  \n";
        outText << tr("Min e = ") << minEccentricity
                <<" (node "<< minNodeEccentricity <<  ")  \n";
        outText << tr("e classes = ") << classesEccentricity<<" \n";
    }

    outText << "\n\n";
    outText << tr("Eccentricity report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



/**
 * @brief Graph::createDistanceMatrix
  Creates a matrix DM which stores geodesic distances between all vertices
    INPUT:
        boolean computeCentralities
        boolean considerWeights
        bool inverseWeights
    OUTPUT:
        DM(i,j)=geodesic distance between vertex i and vertex j
        TM(i,j)=number of shortest paths from vertex i to vertex j, called sigma(i,j).
        graphDiameter is set to the length of the longest shortest path between every (i,j)
        Eccentricity(i) is set to the length of the longest shortest path from i to every j
        Also, if computeCentralities==true, it calculates the centralities for every u in V:
        - Betweenness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
        - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
        - Eccentricity: EC(u) =  1/maxDistance(u,t)  for some t in V
        - Closeness: CC(u) =  1 / Sum( DM(u,t) )  for every  t in V
        - Power:
 * @param computeCentralities
 */
void Graph::createDistanceMatrix(const bool centralities,
                                 const bool considerWeights,
                                 const bool inverseWeights,
                                 const bool dropIsolates) {
    qDebug ("Graph::createDistanceMatrix()");
    if ( !graphModified && distanceMatrixCreated && !centralities)  {
        qDebug("Graph: distanceMatrix not mofified. Escaping.");
        return;
    }
    //Create a NxN DistanceMatrix. Initialise values to zero.
    m_totalVertices = vertices(false,true);
    qDebug() << "Graph::createDistanceMatrix() Resizing Matrices to hold "
             << m_totalVertices << " vertices";
    DM.resize(m_totalVertices);
    TM.resize(m_totalVertices);
    XRM.zeroMatrix(m_totalVertices);

    int aEdges = totalEdges();
    //drop isolated vertices from calculations (i.e. std C and group C).
    int aVertices=vertices(dropIsolates);

    symmetricAdjacencyMatrix = isSymmetric();

    if ( aEdges == 0 )
        DM.fillMatrix(RAND_MAX);
    else {
        qDebug() << "	for all vertices set their distances to -1 (infinum)";
        DM.fillMatrix(RAND_MAX);
        qDebug () << "	for all vertices set their sigmas as 0";
        TM.fillMatrix(0);

        QList<Vertex*>::const_iterator it, it1;
        QList<int>::iterator it2;
        int w=0, u=0,s=0, i=0;
        float d_sw=0, d_su=0;
        int progressCounter=0;

        graphDiameter=0;
        distanceMatrixCreated = false;
        averGraphDistance=0;
        nonZeroDistancesCounter=0;

        qDebug() << "	graphDiameter "<< graphDiameter << " averGraphDistance "
                 <<averGraphDistance;
        qDebug() << "	reciprocalEdgesVert "<< reciprocalEdgesVert
                 << " inboundEdgesVert " << inboundEdgesVert
                 << " outboundEdgesVert "<<  outboundEdgesVert;
        qDebug() << "	aEdges " << aEdges <<  " aVertices " << aVertices;

        qDebug() << "Graph: createDistanceMatrix() - "
                    " initialising variables for maximum centrality indeces";
        if (symmetricAdjacencyMatrix) {
            maxIndexBC=( aVertices-1.0) *  (aVertices-2.0)  / 2.0;
            maxIndexSC=( aVertices-1.0) *  (aVertices-2.0) / 2.0;
            maxIndexCC=aVertices-1.0;
            maxIndexPC=aVertices-1.0;
            qDebug("############# symmetricAdjacencyMatrix - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }
        else {
            maxIndexBC=( aVertices-1.0) *  (aVertices-2.0) ;
            maxIndexSC=( aVertices-1.0) *  (aVertices-2.0);
            maxIndexPC=aVertices-1.0;
            maxIndexCC=aVertices-1.0;
            qDebug("############# NOT SymmetricAdjacencyMatrix - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }

        qDebug("Graph: createDistanceMatrix() - initialising variables for centrality index");
        maxCC=0; minCC=RAND_MAX; nomCC=0; denomCC=0; groupCC=0; maxNodeCC=0;
        minNodeCC=0; sumCC=0;
        discreteCCs.clear(); classesCC=0;
        maxBC=0; minBC=RAND_MAX; nomBC=0; denomBC=0; groupBC=0; maxNodeBC=0;
        minNodeBC=0; sumBC=0;
        discreteBCs.clear(); classesBC=0;
        maxSC=0; minSC=RAND_MAX; nomSC=0; denomSC=0; groupSC=0; maxNodeSC=0;
        minNodeSC=0; sumSC=0;
        discreteSCs.clear(); classesSC=0;
        maxPC=0; minPC=RAND_MAX; nomPC=0; denomPC=0; groupPC=0; maxNodePC=0;
        minNodePC=0; sumPC=0;t_sumPC=0;
        discretePCs.clear(); classesPC=0;
        maxEccentricity=0; minEccentricity=RAND_MAX; maxNodeEccentricity=0;
        minNodeEccentricity=0; sumEccentricity=0; discreteEccentricities.clear();
        classesEccentricity=0;
        maxPC=0; minPC=RAND_MAX; maxNodePC=0; minNodePC=0; sumPC=0;
        float CC=0, BC=0, SC= 0, eccentricity=0, EC=0, PC=0;
        float SCC=0, SBC=0, SSC=0, SEC=0, SPC=0;
        float tempVarianceBC=0, tempVarianceSC=0,tempVarianceEC=0;
        float tempVarianceCC=0, tempVariancePC=0;
        float t_sumSC=0;

        maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0;
        minNodeEC=0; sumEC=0;
        discreteECs.clear(); classesEC=0;

        //Zero closeness indeces of each vertex
        if (centralities)
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                qDebug() << " Graph:createDistanceMatrix() - ZEROing all indices";
                (*it)->setBC( 0.0 );
                (*it)->setSC( 0.0 );
                (*it)->setEccentricity( 0.0 );
                (*it)->setEC( 0.0 );
                (*it)->setCC( 0.0 );
                (*it)->setPC( 0.0 );
            }
        qDebug("MAIN LOOP: for every s in V solve the Single Source Shortest Path problem...");
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
            progressCounter++;
            emit updateProgressDialog( progressCounter );
            //isolates are dropped by default in the beginning
            //
//            if ( ! (*it)->isEnabled() )
//                continue;
            s=index[(*it)->name()];
            qDebug() << "Source vertex s = " << (*it)->name()
                     << " of BFS algorithm has index " << s

                     << ". Clearing Stack ...";
            if (centralities){
                qDebug()<< "Empty stack Stack which will return vertices in "
                           "order of their (non increasing) distance from S ...";
                //- Complexity linear O(n)
                while ( !Stack.empty() )
                    Stack.pop();
                i=0;
                qDebug()<< "...and for each vertex: empty list Ps of predecessors";
                for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1) {
                    (*it1)->clearPs();
                    //initialize all sizeOfNthOrderNeighborhood to zero
                    sizeOfNthOrderNeighborhood.insert(i, 0);
                    i++;
                }
            }

            qDebug() << "PHASE 1 (SSSP): Call BFS or dijkstra for source vertex "
                     << (*it)->name() << " index " << s
                     << " to determine distances and geodesics from s to every vertex t" ;
            if (!considerWeights)
                BFS(s,centralities, dropIsolates );
            else
                dijkstra(s, centralities, inverseWeights, dropIsolates);


            qDebug("***** FINISHED PHASE 1 (SSSP) BFS ALGORITHM. Continuing to calculate centralities");

            if (centralities){
                qDebug() << "Set CC for source vertex " << (*it)->name()
                         << "  with index s = " << s ;
                if ( (*it)->CC() != 0 ) //Closeness centrality must be inverted
                    CC=1.0/(*it)->CC();
                else
                    CC=0;
                (*it)->setCC( CC );

                //Check eccentricity (max geodesic distance)
                eccentricity = (*it)->eccentricity();
                if ( eccentricity != 0 ) {
                    //Eccentricity Centrality is the inverted Eccentricity
                    EC=1.0 / eccentricity;
                }
                else {
                    EC=0;
                    eccentricity=0;
                }
                (*it)->setEC( EC ); //Set Eccentricity Centrality
                (*it)->setSEC( EC ); //Set std EC = EC
                sumEC+=EC;  //set sum EC

                //Find min/max Eccentricity
                minmax( eccentricity, (*it), maxEccentricity, minEccentricity,
                        maxNodeEccentricity, minNodeEccentricity) ;
                resolveClasses(eccentricity, discreteEccentricities,
                               classesEccentricity ,(*it)->name() );
                sumEccentricity+=eccentricity;

                qDebug()<< "PHASE 2 (ACCUMULATION): Start back propagation of dependencies." <<
                       "Set dependency delta[u]=0 on each vertex";

                i=1; //used in calculating power centrality
                sizeOfComponent = 1;
                PC=0;
                for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){

                    (*it1)->setDelta(0.0);
                    //Calculate Power Centrality: In = [ 1/(N-1) ] * ( Nd1 + Nd2 * 1/2 + ... + Ndi * 1/i )
                    // where Ndi (sizeOfNthOrderNeighborhood) is the number of nodes at distance i from this node.
                    //FIXME do we need to check for disabled nodes somewhere?
                    qDebug() << " sizeOfNthOrderNeighborhood.value("<< i<<")"
                                << sizeOfNthOrderNeighborhood.value(i);
                    PC += ( 1.0 / (float) i ) * sizeOfNthOrderNeighborhood.value(i);
                    // where N is the sum Nd0 + Nd1 + Nd2 + ... + Ndi, that is the amount of nodes in the same component as the current node
                    sizeOfComponent += sizeOfNthOrderNeighborhood.value(i);
                    i++;
                }

                (*it)->setPC( PC );	//Power Centrality is stdized already
                t_sumPC += PC;   //add to temp sumPC
                if ( sizeOfComponent != 1 )
                    SPC = ( 1.0/(sizeOfComponent-1.0) ) * PC;
                else
                    SPC = 0;

                (*it)->setSPC( SPC );	//Set std PC

                sumPC += SPC;   //add to sumPC -- used later to compute mean and variance

                qDebug() << "Visit all vertices in reverse order of their discovery (from s = " << s
                         << " ) to sum dependencies. Initial Stack size has " << Stack.size();

                while ( !Stack.empty() ) {
                    w=Stack.top();
                    qDebug("Stack top is vertex w=%i. This is the furthest vertex from s. Popping it.", w);
                    Stack.pop();
                    QList<int> lst=m_graph[w]->Ps();
                    qDebug("preLOOP: Checking size of predecessors list Ps[w]...  = %i ",lst.size());
                    qDebug("LOOP: for every other vertex u in the list of predecessors Ps[w] of w....");
                    if (lst.size() > 0) // just in case...do a sanity check
                        for ( it2=lst.begin(); it2 != lst.end(); it2++ ){
                            u=(*it2);
                            qDebug("Selecting Ps[w] element u=%i with delta_u=%f. sigma(u)=TM(s,u)=%f, sigma(w)=TM(s,w)=%f, delta_w=%f ",
                                   u, m_graph[u]->delta(),TM.item(s,u), TM.item(s,w), m_graph[w]->delta());
                            if ( TM.item(s,w) > 0) {
                                //delta[u]=delta[u]+(1+delta[w])*(sigma[u]/sigma[w]) ;
                                d_su=m_graph[u]->delta()+(1.0+m_graph[w]->delta() ) * ( (float)TM.item(s,u)/(float)TM.item(s,w) );
                            }
                            else {
                                d_su=m_graph[u]->delta();
                                qDebug("TM (s,w) zero, i.e. zero shortest path counts from s to w - using SAME DELTA for vertex u");
                            }
                            qDebug("Assigning new delta d_su = %f to u = %i", d_su, u);
                            m_graph[u]->setDelta( d_su);
                        }
                    qDebug()<<" Adding delta_w to BC of w";
                    if  (w!=s) {
                        qDebug("w!=s. For this furthest vertex we need to add its new delta %f to old BC index: %f",
                               m_graph[w]->delta(), m_graph[w]->BC());
                        d_sw = m_graph[w]->BC() + m_graph[w]->delta();
                        qDebug("New BC = d_sw = %f", d_sw);
                        m_graph[w]->setBC (d_sw);
                    }
                }
            }
        }

        if (averGraphDistance!=0) {
             averGraphDistance = averGraphDistance / ( aVertices * ( aVertices-1.0 ) );
        }


        if (centralities) {
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ){
                    qDebug() << "vertex " << (*it)->name()
                             << " isolated, continue. ";
                    continue;
                }
                // Compute classes and min/maxEC
                SEC=(*it)->SEC();
                resolveClasses(SEC, discreteECs, classesEC,(*it)->name() );
                minmax( SEC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;

                // Compute classes and min/maxPC
                SPC = (*it)->SPC();  //same as PC
                resolveClasses(SPC, discretePCs, classesPC,(*it)->name() );
                minmax( SPC, (*it), maxPC, minPC, maxNodePC, minNodePC) ;

                // Compute std BC, classes and min/maxBC
                if (symmetricAdjacencyMatrix) {
                    qDebug()<< "Betweenness centrality must be divided by"
                            <<" two if the graph is undirected";
                    (*it)->setBC ( (*it)->BC()/2.0);
                }
                BC=(*it)->BC();
                SBC = BC/maxIndexBC;
                (*it)->setSBC( SBC );
                resolveClasses(SBC, discreteBCs, classesBC);
                sumBC+=SBC;
                minmax( SBC, (*it), maxBC, minBC, maxNodeBC, minNodeBC) ;

                // Compute std CC, classes and min/maxCC
                CC = (*it)->CC();
                SCC = maxIndexCC * CC;
                (*it)->setSCC (  SCC );
                resolveClasses(SCC, discreteCCs, classesCC,(*it)->name() );
                sumCC+=SCC;
                minmax( SCC, (*it), maxCC, minCC, maxNodeCC, minNodeCC) ;

                //prepare to compute stdSC
                SC=(*it)->SC();
                if (symmetricAdjacencyMatrix){
                    (*it)->setSC(SC/2.0);
                    SC=(*it)->SC();
                    qDebug() << "SC of " <<(*it)->name()
                             << "  divided by 2 (because the graph is symmetric) "
                             << (*it)->SC();
                }
                t_sumSC+=SC;

                qDebug() << "vertex " << (*it)->name() << " - "
                         << " EC: "<< (*it)->EC()
                         << " CC: "<< (*it)->CC()
                         << " BC: "<< (*it)->BC()
                         << " SC: "<< (*it)->SC()
                         << " PC: "<< (*it)->PC();
            }

            // calculate mean values and prepare to compute variances
            meanBC = sumBC /(float) aVertices ;
            varianceBC=0;
            tempVarianceBC=0;

            meanCC = sumCC /(float) aVertices ;
            varianceCC=0;
            tempVarianceCC=0;

            meanPC = sumPC /(float) aVertices ;
            variancePC=0;
            tempVariancePC=0;

            meanEC = sumEC /(float) aVertices ;
            varianceEC=0;
            tempVarianceEC=0;

            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ) {
                    continue;
                }
                // Compute std SC, classes and min/maxSC
                SC=(*it)->SC();
                SSC=SC/t_sumSC;
                (*it)->setSSC(SSC);
                resolveClasses(SSC, discreteSCs, classesSC);
                sumSC+=SSC;
                minmax( SSC, (*it), maxSC, minSC, maxNodeSC, minNodeSC) ;

                //Compute numerator of groupBC
                SBC=(*it)->SBC();
                nomBC +=(maxBC - SBC );

                //calculate BC variance
                tempVarianceBC = (  SBC  -  meanBC  ) ;
                tempVarianceBC *=tempVarianceBC;
                varianceBC  += tempVarianceBC;

                //Compute numerator of groupCC
                nomCC += maxCC- (*it)->SCC();

                //calculate CC variance
                tempVarianceCC = (  (*it)->SCC()  -  meanCC  ) ;
                tempVarianceCC *=tempVarianceCC;
                varianceCC  += tempVarianceCC;

                //Compute numerator of groupPC
                SPC=(*it)->SPC();
                nomPC +=(maxPC - SPC );

                //calculate PC variance
                tempVariancePC = (  (*it)->SPC()  -  meanPC  ) ;
                tempVariancePC *=tempVariancePC;
                variancePC  += tempVariancePC;

                //calculate EC variance
                tempVarianceEC = (  (*it)->EC()  -  meanEC  ) ;
                tempVarianceEC *=tempVarianceEC;
                varianceEC  += tempVarianceEC;
            }

            //compute final variances
            varianceBC  /=  (float) aVertices;
            varianceCC  /=  (float) aVertices;
            variancePC  /=  (float) aVertices;
            varianceEC  /=  (float) aVertices;

            // calculate SC mean value and prepare to compute variance
            meanSC = sumSC /(float) aVertices ;
            varianceSC=0;
            tempVarianceSC=0;
            for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
                if ( dropIsolates && (*it)->isIsolated() ){
                    continue;
                }
                tempVarianceSC = (  (*it)->SSC()  -  meanSC  ) ;
                tempVarianceSC *=tempVarianceSC;
                varianceSC  += tempVarianceSC;
            }
            //calculate final SC variance
            varianceSC  /=  (float) aVertices;

            denomPC = (  (aVertices-2.0) ) / (2.0 );   //only for connected nets
            if (aVertices < 3 )
                 denomPC = aVertices-1.0;
            //what if the net is disconnected (isolates exist) ?
            groupPC = nomPC/denomPC;

            denomCC = ( ( aVertices-1.0) * (aVertices-2.0) ) / (2.0 * aVertices -3.0);
            if (aVertices < 3 )
                 denomCC = aVertices-1.0;

            groupCC = nomCC/denomCC;	//Calculate group Closeness centrality

            //nomBC*=2.0;
//            denomBC =   (aVertices-1.0) *  (aVertices-1.0) * (aVertices-2.0);
            denomBC =   (aVertices-1.0) ;  // Wasserman&Faust - formula 5.14
            groupBC=nomBC/denomBC;		//Calculate group Betweenness centrality

            calculatedCentralities=true;
        }
    }

    distanceMatrixCreated=true;

}





/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

    INPUT:
        a 'source' vertex with index s and a boolean computeCentralities.
        (Implicitly, BFS uses the m_graph structure)

    OUTPUT:
        For every vertex t: DM(s, t) is set to the distance of each t from s
        For every vertex t: TM(s, t) is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then BFS does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from Q is pushed to a stack Stack

*/ 
void Graph::BFS(int s, const bool computeCentralities=false,
                const bool dropIsolates=false){
    Q_UNUSED(dropIsolates);
    int u,w, dist_u=0, temp=0, dist_w=0;
    int relation=0, target=0;
    //int  weight=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1;
    //set distance of s from s equal to 0
    DM.setItem(s,s,0);
    //set sigma of s from s equal to 1
    TM.setItem(s,s,1);

//    qDebug("BFS: Construct a queue Q of integers and push source vertex s=%i to Q as initial vertex", s);
    queue<int> Q;
//    qDebug()<<"BFS: Q size "<< Q.size();

    Q.push(s);

    qDebug("BFS: LOOP: While Q not empty ");
    while ( !Q.empty() ) {
        qDebug("BFS: Dequeue: first element of Q is u=%i", Q.front());
        u=Q.front(); Q.pop();

        if ( ! m_graph [ u ]->isEnabled() ) continue ;

        if (computeCentralities){
//            qDebug("BFS: If we are to calculate centralities, we must push u=%i to global stack Stack ", u);
            Stack.push(u);
        }
        qDebug() << "BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u";
        it1=m_graph [ u ] ->m_outEdges.cbegin();
        while ( it1!=m_graph [ u ] -> m_outEdges.cend() ){
            relation = it1.value().first;
            if ( relation != currentRelation() )  {
                ++it1;
                continue;
            }
            edgeStatus=it1.value().second.second;
            if ( edgeStatus != true)   {
                ++it1;
                continue;
            }
            target = it1.key();
          //  weight = it1.value().second.first;
            w=index[ target ];
            qDebug("BFS: u=%i is connected with node %i of index w=%i. ", u, target, w);
//            qDebug("BFS: Start path discovery");
            if (	DM.item(s, w) == RAND_MAX ) { //if distance (s,w) is infinite, w found for the first time.
                qDebug("BFS: first time visiting w=%i. Enqueuing w to the end of Q", w);
                Q.push(w);
                qDebug()<<"BFS: First check if distance(s,u) = -1 (aka infinite :)) and set it to zero";
                dist_u=DM.item(s,u);
                dist_w = dist_u + 1;
                qDebug("BFS: Setting distance of w=%i from s=%i equal to distance(s,u) plus 1. New distance = %i",w,s, dist_w );
                DM.setItem(s, w, dist_w);
                averGraphDistance += dist_w;
                nonZeroDistancesCounter++;


                qDebug()<< "Graph::BFS()  - d("
                        << s <<"," << w
                        <<")=" << DM.item(s,w)
                       << " - inserting " << w
                       << " to inflRange J of " << s
                       << " - and " << s
                       << " to inflDomain I of "<< w;
                XRM.setItem(s,w,1);
                influenceRanges.insert(s,w);
                influenceDomains.insert(w,s);
//                disconnectedVertices

                if (computeCentralities){
                    qDebug()<<"BFS: Calculate PC: store the number of nodes at distance " << dist_w << "from s";
                    sizeOfNthOrderNeighborhood.insert(
                                dist_w,
                                sizeOfNthOrderNeighborhood.value(dist_w)+1
                                );
                    qDebug()<<"BFS: Calculate CC: the sum of distances (will invert it l8r)";
                    m_graph [s]->setCC (m_graph [s]->CC() + dist_w);
                    qDebug()<<"BFS: Calculate Eccentricity: the maximum distance ";
                    if (m_graph [s]->eccentricity() < dist_w )
                        m_graph [s]->setEccentricity(dist_w);

                }
//                qDebug("BFS: Checking graphDiameter");
                if ( dist_w > graphDiameter){
                    graphDiameter=dist_w;
//                    qDebug() << "BFS: new graphDiameter = " <<  graphDiameter ;
                }
            }

            qDebug("BFS: Start path counting"); 	//Is edge (u,w) on a shortest path from s to w via u?
            if ( DM.item(s,w)==DM.item(s,u)+1) {
                temp= TM.item(s,w)+TM.item(s,u);
                qDebug("BFS: Found a NEW SHORTEST PATH from s=%i to w=%i via u=%i. Setting Sigma(%i, %i) = %i",s, w, u, s, w,temp);
                if (s!=w)
                    TM.setItem(s,w, temp);
                if (computeCentralities){
                    qDebug("BFS/SC: If we are to calculate centralities, we must calculate SC as well");
                    if ( s!=w && s != u && u!=w ) {
                        qDebug() << "BFS: setSC of u="<<u<<" to "<<m_graph[u]->SC()+1;
                        m_graph[u]->setSC(m_graph[u]->SC()+1);
                    }
                    else {
//                        qDebug() << "BFS/SC: skipping setSC of u, because s="
//                                 <<s<<" w="<< w << " u="<< u;
                    }
//                    qDebug() << "BFS/SC: SC is " << m_graph[u]->SC();
                    qDebug() << "BFS: appending u="<< u << " to list Ps[w=" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[w]->appendToPs(u);
                }
            }
            ++it1;
        }

    }
}




/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

    INPUT:
        a 'source' vertex with index s and a boolean computeCentralities.
        (Implicitly, BFS uses the m_graph structure)

    OUTPUT:
        For every vertex t: DM(s, t) is set to the distance of each t from s
        For every vertex t: TM(s, t) is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then BFS does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from Q is pushed to a stack Stack

*/
void Graph::dijkstra(int s, const bool computeCentralities=false,
                     const bool inverseWeights=false,
                     const bool dropIsolates=false){
    Q_UNUSED(dropIsolates);
    int u,w,v, temp=0;
    int relation=0, target=0;
    float  weight=0, dist_u=0,  dist_w=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1;

    qDebug() << "dijkstra: Construct a priority queue Q of all vertices-distances";
    priority_queue<Distance, vector<Distance>, CompareDistances> Q;

    //set distance of s from s equal to 0
    DM.setItem(s,s,0);
    //set sigma of s from s equal to 1
    TM.setItem(s,s,1);

    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
        v=index[ (*it)->name() ];
        if (v != s ){
            // DM initialization to RAND_MAX already done in createDistanceMatrix
            //DM.setItem(s,v,RAND_MAX);
            qDebug() << " push " << v << " to Q with infinite distance from s";
            Q.push(Distance(v,RAND_MAX));
            //TODO // Previous node in optimal path from source
            //    previous[v]  := undefined
        }
    }
    qDebug() << " finally push source " << s << " to Q with 0 distance from s";
    //crucial: without it the priority Q would pop arbitrary node at first loop
    Q.push(Distance(s,0));
    qDebug()<<"dijkstra: Q size "<< Q.size();

    qDebug() << "\n\n ### dijkstra: LOOP: While Q not empty ";
    while ( !Q.empty() ) {
        u=Q.top().target;
        qDebug()<< "\n\n *** dijkstra: take u = "<< u
                   << " from Q which has minimum distance from s = " << s;
         Q.pop();

        if ( ! m_graph [ u ]->isEnabled() )
            continue ;

        if (computeCentralities){
            qDebug()<< "dijkstra: We will calculate centralities, push u="<< u
                    << " to global stack Stack ";
            Stack.push(u);
        }
        qDebug() << "*** dijkstra: LOOP over every edge ("<< u <<",w) e E, "
                 <<  "that is for each neighbor w of u";
        it1=m_graph [ u ] ->m_outEdges.cbegin();
        while ( it1!=m_graph [ u ] -> m_outEdges.cend() ){
            relation = it1.value().first;
            if ( relation != currentRelation() )  {
                ++it1;
                continue;
            }
            edgeStatus=it1.value().second.second;
            if ( edgeStatus != true)   {
                ++it1;
                continue;
            }
            target = it1.key();
            weight = it1.value().second.first;
            w=index[ target ];
            qDebug()<<"\ndijkstra: u="<< u << " --> w="<< w << " (node "<< target
                   << ") of weight "<<  weight;
            if (inverseWeights) { //only invert if user asked to do so
                weight = 1.0 / weight;
                qDebug () << " inverting weight to " << weight;
            }

            qDebug("dijkstra: Start path discovery");

            dist_u=DM.item(s,u);
            if (dist_u == RAND_MAX || dist_u < 0) {
                dist_w = RAND_MAX;
                qDebug() << "dijkstra: dist_w = RAND_MAX " << RAND_MAX;

            }
            else {
                dist_w = dist_u + weight;
                qDebug() << "dijkstra: dist_w = dist_u + weight = "
                         << dist_u << " + " << weight <<  " = " <<dist_w ;
            }
            qDebug() << "dijkstra: RELAXATION : check if dist_w=" << dist_w
                     <<  " is shorter than current DM(s,w)";
            if  (dist_w == DM.item(s, w)  && dist_w < RAND_MAX) {
                qDebug() << "dijkstra: dist_w : " << dist_w
                         <<  " ==  DM(s,w) : " << DM.item(s, w);
                temp= TM.item(s,w)+TM.item(s,u);
                qDebug()<<"dijkstra: Found another SP from s=" << s
                       << " to w=" << w << " via u="<< u
                       << " - Setting Sigma(s, w) = "<< temp;
                if (s!=w)
                    TM.setItem(s,w, temp);
                if (computeCentralities){
                    qDebug()<< "dijkstra/SC:";
                    if ( s!=w && s != u && u!=w ) {
                        qDebug() << "dijkstra: Calculate SC: setSC of u="<<u
                                 <<" to "<<m_graph[u]->SC()+1;
                        m_graph[u]->setSC(m_graph[u]->SC()+1);
                    }
                    else {
                        qDebug() << "dijkstra/SC: skipping setSC of u, because s="
                                 <<s<<" w="<< w << " u="<< u;
                    }
                    qDebug() << "dijkstra/SC: SC is " << m_graph[u]->SC();

                    qDebug() << "dijkstra: appending u="<< u << " to list Ps[w=" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[w]->appendToPs(u);
                }
            }

            else if (dist_w > 0 && dist_w < DM.item(s, w)  ) {
                qDebug() << "dijkstra: Yeap. Set DM (s,w) = DM(" << s
                         << ","<< w
                         << ") = "<< dist_w ;
                DM.setItem(s, w, dist_w);
                averGraphDistance += dist_w;
                nonZeroDistancesCounter++;


                qDebug()<< "Graph::dijkstra()  - d("
                        << s <<"," << w
                        <<")=" << DM.item(s,w)
                       << " - inserting " << w
                       << " to inflRange J of " << s
                       << " - and " << s
                       << " to inflDomain I of "<< w;
                XRM.setItem(s,w,1);
                influenceRanges.insert(s,w);
                influenceDomains.insert(w,s);
//                disconnectedVertices


                if (s!=w) {
                    qDebug()<<"dijkstra: Found NEW SP from s=" << s
                           << " to w=" << w << " via u="<< u
                           << " - Setting Sigma(s, w) = 1 ";
                    TM.setItem(s,w, 1);
                }

                if (computeCentralities){
                    sizeOfNthOrderNeighborhood.insert(
                                dist_w,
                                sizeOfNthOrderNeighborhood.value(dist_w)+1
                                );
                    qDebug()<<"dijkstra/PC: number of nodes at distance "
                           << dist_w << "from s is "
                           <<  sizeOfNthOrderNeighborhood.value(dist_w);

                    m_graph [s]->setCC (m_graph [s]->CC() + dist_w);
                    qDebug()<<"dijkstra/CC:: sum of distances = "
                           <<  m_graph [s]->CC() << " (will invert it l8r)";

                    if (m_graph [s]->eccentricity() < dist_w )
                        m_graph [s]->setEccentricity(dist_w);
                    qDebug()<<"dijkstra/Eccentricity: max distance  = "
                              <<  m_graph [s]->eccentricity();
                }

                qDebug("dijkstra/graphDiameter");
                if ( dist_w > graphDiameter){
                    graphDiameter=dist_w;
                    qDebug() << "dijkstra: new graphDiameter = " << graphDiameter ;
                }

            }
            else
                qDebug() << "dijkstra: NO";


//            qDebug()<< "### dijkstra: Start path counting";
//            // Is (u,w) on a shortest path from s to w via u?
//            if ( DM.item(s,w)==DM.item(s,u)+weight) {
//                temp= TM.item(s,w)+TM.item(s,u);

//            }
            ++it1;
        }

    }
}







/**
    minmax() facilitates the calculations of minimum and maximum centralities during createDistanceMatrix()
*/
void Graph::minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) {
    qDebug() << "MINMAX C = " <<  C << "  max = " << max << "  min = " << min << " name = " <<  v->name();
    if (C > max ) {
        max=C;
        maxNode=v->name();
    }
    if (C < min ) {
        min=C;
        minNode=v->name();
    }
}




/** 	This method calculates the number of discrete centrality classes of all vertices
    It stores that number in a QHash<QString,int> type where the centrality value is the key.
    Called from createDistanceMatrix()
*/
void Graph::resolveClasses(float C, H_StrToInt &discreteClasses, int &classes){
    H_StrToInt::iterator it2;
    it2 = discreteClasses.find(QString::number(C));    //Amort. O(1) complexity
    if (it2 == discreteClasses.end() )	{
        classes++;
        qDebug("######This is a new centrality class. Amount of classes = %i", classes);
        discreteClasses.insert(QString::number(C), classes);
    }
}


/*
 * Overloaded method. It only adds displaying current vertex for debugging purposes.
 */
void Graph::resolveClasses(float C, H_StrToInt &discreteClasses, int &classes, int vertex){
    H_StrToInt::iterator it2;
    it2 = discreteClasses.find(QString::number(C));    //Amort. O(1) complexity
    if (it2 == discreteClasses.end() )	{
        classes++;
        qDebug("######Vertex %i  belongs to a new centrality class. Amount of classes = %i", vertex, classes);
        discreteClasses.insert(QString::number(C), classes);
    }
}





/*  Calculates the Information centrality of each vertex - diagonal included
 *
 *  Note that there is no known generalization of Stephenson&Zelen's theory
 *  for information centrality to directional data
*/

void Graph::centralityInformation(const bool considerWeights,
                                  const bool inverseWeights){
    qDebug()<< "Graph:: centralityInformation()";
    if (calculatedIC && !graphModified) {
        return;
    }
    discreteICs.clear();
    sumIC=0;
    maxIC=0;
    t_sumIC=0;
    minIC=RAND_MAX;
    classesIC=0;
    varianceIC=0;

    TM.resize(m_totalVertices);
    isolatedVertices=verticesIsolated().count();
    int i=0, j=0, n=vertices();
    float m_weight=0, weightSum=1, diagonalEntriesSum=0, rowSum=0;
    float IC=0, SIC=0;
    /* Note: isolated nodes must be dropped from the AM
        Otherwise, the TM might be singular, therefore non-invertible. */
    bool dropIsolates=true;
    bool symmetrize=true;
    createAdjacencyMatrix(dropIsolates, considerWeights, inverseWeights, symmetrize);
    n-=isolatedVertices;  //isolatedVertices updated in createAdjacencyMatrix
    qDebug() << "Graph:: centralityInformation() - computing node ICs for total n = " << n;

    for (i=0; i<n; i++){
        weightSum = 1;
        for (j=0; j<n; j++){
            if ( i == j )
                continue;
            m_weight = AM.item(i,j);
            weightSum += m_weight; //sum of weights for all edges incident to i
            qDebug() << "Graph:: centralityInformation() -A("<< i <<  ","<< j <<") = 1-Xij = " << 1-m_weight;
            TM.setItem(i,j,1-m_weight);
        }
        TM.setItem(i,i,weightSum);
        qDebug() << "Graph:: centralityInformation() - A("<< i << ","<< i <<") = 1+sum of all tie values = " << weightSum;
    }

    invM.inverseByGaussJordanElimination(TM);

    diagonalEntriesSum = 0;
    rowSum = 0;
    for (j=0; j<n; j++){
        rowSum += invM.item(0,j);
    }
    for (i=0; i<n; i++){
        diagonalEntriesSum  += invM.item(i,i);  // calculate the matrix trace
    }
    qDebug() << "Graph:: centralityInformation() - R= " << rowSum << " D= "<<diagonalEntriesSum;


    QList<Vertex*>::const_iterator it;
    i=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( (*it)->isIsolated() ) {
            (*it) -> setIC ( 0 );
            qDebug()<< "Graph:: centralityInformation() vertex: " <<  (*it)->name() << " isolated";
            continue;
        }
        IC= 1.0 / ( invM.item(i,i) + (diagonalEntriesSum - 2.0 * rowSum) / n );

        (*it) -> setIC ( IC );
        t_sumIC += IC;
        qDebug()<< "Graph:: centralityInformation() vertex: " <<  (*it)->name()
                << " IC  " << IC ;
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

    float x=0;
    meanIC = sumIC /(float) n ;

    qDebug() << "sumSIC = " << sumIC << "  n = " << n << "  meanIC = " << meanIC;
    varianceIC=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        x = (  (*it)->SIC()  -  meanIC  ) ;
        x *=x;
        qDebug() << "SIC " <<  (*it)->SIC() << "  x "
                 <<   (*it)->SIC() - meanIC  << " x*x" << x ;
        varianceIC  += x;
    }
    qDebug() << "varianceIC   " << varianceIC   << " n " << n ;
    varianceIC  /=  (float) n;
    qDebug() << "varianceIC   " << varianceIC   ;

    calculatedIC = true;
}



//Writes the information centralities to a file
void Graph::writeCentralityInformation(const QString fileName,
                                       const bool considerWeights,
                                       const bool inverseWeights){
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );
    outText.setCodec("UTF-8");
    if (graphModified || !calculatedIC ) {
            emit statusMessage ( (tr("Calculating IC scores...")) );
            centralityInformation(considerWeights, inverseWeights);
    }

    emit statusMessage ( QString(tr("Writing information centralities to file: "))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("INFORMATION CENTRALITY (IC)")<<"\n";
    outText << tr("The IC index measures the information flow through "
                  "all paths between actors weighted by strength of tie and distance\n");
    outText << tr("IC' is the standardized IC (IC divided by the sumIC).") <<"\n"
               << tr ("Warning: To compute this index, SocNetV drops all isolated "
                      "nodes and symmetrizes (if needed) the adjacency matrix. "
                      "Read the Manual for more.") << "\n\n";

    outText << tr("IC  range:  0 < IC < inf (this index has no max value)") << "\n";
    outText << tr("IC' range:  0 < IC'< 1 (" )<<"\n\n";
    outText << "Node"<<"\tIC\t\tIC'\t\t%IC'\n";
    QList<Vertex*>::const_iterator it;
    float IC=0, SIC=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        IC = (*it)->IC();
        SIC = (*it)->SIC();
        outText << (*it)->name()<<"\t"
                << IC << "\t\t"
                << SIC  << "\t\t"
                <<  ( 100* SIC )<<endl;
        qDebug()<< "Graph::writeCentralityInformation() vertex: "
                <<  (*it)->name() << " SIC  " << SIC;
    }
    qDebug ("min %f, max %f", minIC, maxIC);
    if ( minIC == maxIC )
        outText << tr("\nAll nodes have the same IC value.\n");
    else  {
        outText << "\n";
        outText << tr("Max IC' = ") << maxIC <<" (node "<< maxNodeIC  <<  ")  \n";
        outText << tr("Min IC' = ") << minIC <<" (node "<< minNodeIC <<  ")  \n";
        outText << tr("IC classes = ") << classesIC<<" \n";
    }
    outText << "\n";
    outText << tr("IC' sum = ") << sumIC <<  " \n";
    outText << tr("IC' Mean = ") << meanIC <<  " \n";
    outText << tr("IC' Variance = ") << varianceIC <<  " \n\n";

    outText << tr("Since there is no way to compute Group Information Centralization, "
                  "you can use variance as a general centralization index.")
            <<" \n\n";

    outText << tr("Variance = 0, when all nodes have the same IC value, i.e. a "
                  "complete or a circle graph).\n");
    outText << tr("Larger values of variance suggest larger variability between the "
                  "IC' values.\n");
    outText <<"(Wasserman & Faust, formula 5.20, p. 197)\n\n";


    outText << tr("Information Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}





//Calculates the outDegree centrality of each vertex - diagonal included
void Graph::centralityDegree(const bool weights, const bool dropIsolates){
    qDebug("Graph::centralityDegree()");
    if (!graphModified && calculatedDC ) {
        qDebug() << "Graph::centralityDegree() - graph not changed - returning";
        return;
    }
    float DC=0, nom=0, denom=0,  SDC=0;
    float weight;
    classesDC=0;
    discreteDCs.clear();
    sumDC=0;
    t_sumDC=0;
    maxDC=0;
    minDC=RAND_MAX;
    varianceDC=0;
    meanDC=0;
    int vert=vertices(dropIsolates);

    QList<Vertex*>::const_iterator it, it1;
    H_StrToInt::iterator it2;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        DC=0;
        if (!(*it)->isIsolated()) {
            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
                if ( (weight=this->hasArc ( (*it)->name(), (*it1)->name() ) ) !=0  )   {
//                    qDebug() << "Graph::centralityDegree() - vertex "
//                             <<  (*it)->name()
//                             << " hasEdgeTo = " <<  (*it1)->name();
                    if (weights)
                        DC+=weight;
                    else
                        DC++;

                    //check here if the matrix is symmetric - we need this below
                    if ( ( this->hasArc ( (*it1)->name(), (*it)->name() ) ) !=
                         ( this->hasArc ( (*it)->name(), (*it1)->name() ) )   )
                        symmetricAdjacencyMatrix = false;
                }
            }
        }

        (*it) -> setDC ( DC ) ;	//Set OutDegree
        t_sumDC += DC;          // store temp sumDC (for std calc below)
        qDebug() << "Graph:centralityDegree() - vertex "
                 <<  (*it)->name() << " has DC = " << DC ;
    }

    // Calculate std Out-Degree, min, max, classes and sumSDC
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        DC= (*it)->DC();
        if (!weights) {
            SDC = ( DC / (vert-1.0) );
        }
        else {
            SDC= ( DC / (t_sumDC) );
        }
        (*it) -> setSDC( SDC );		//Set Standard DC

//        qDebug() << "Graph::centralityDegree() - vertex "
//                 <<  (*it)->name() << " SDC " << (*it)->SDC ();

        sumDC+=SDC;

        it2 = discreteDCs.find(QString::number(SDC));
        if (it2 == discreteDCs.end() )	{
            classesDC++;
           // qDebug("This is a new DC class");
            discreteDCs.insert ( QString::number(DC), classesDC );
        }
        //qDebug() << "DC classes =  " << classesDC;

        if (maxDC < SDC ) {
            maxDC = SDC ;
            maxNodeDC=(*it)->name();
        }
        if (minDC > SDC ) {
            minDC = SDC ;
            minNodeDC=(*it)->name();
        }
    }

    if (minDC == maxDC)
        maxNodeDC=-1;

    meanDC = sumDC / (float) vert;
    //    qDebug() << "Graph::centralityDegree() - sumDC  " << sumDC
    //             << " vertices " << vert << " meanDC = sumDC / vert = " << meanDC;

    // Calculate Variance and the Degree Centralisation of the whole graph.
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        SDC= (*it)->SDC();
        nom+= (maxDC-SDC);
        varianceDC += (SDC-meanDC) * (SDC-meanDC) ;

//        if ( dropIsolates  ) {
//            if   ( ! (*it)->isIsolated() ) {
//                varianceDC += (SDC-meanDC) * (SDC-meanDC) ;
//                nom+= (maxDC-SDC);
//            }
//        }
//        else {
//        }

    }
    varianceDC=varianceDC/(float) vert;

//    qDebug() << "Graph::centralityDegree() - variance = " << varianceDC;
    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);

    if (vert < 3 )
         denom = vert-1.0;
    //    qDebug () << "*** vert is " << vert << " nom " << nom << " denom is " << denom;
    if (!weights) {
        groupDC=nom/denom;
    }

    calculatedDC=true;

}




void Graph::writeCentralityDegree ( const QString fileName,
                                    const bool considerWeights,
                                    const bool dropIsolates)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );
    outText.setCodec("UTF-8");
    qDebug()<< "Graph:: writeCentralityDegree() considerWeights "
            << considerWeights
               << " dropIsolates " <<dropIsolates;
    centralityDegree(considerWeights, dropIsolates);

    float maximumIndexValue=vertices(dropIsolates)-1.0;

    outText.setRealNumberPrecision(m_precision);
    outText << tr("DEGREE CENTRALITY (DC)\n");
    outText << tr("In undirected graphs, the DC index is the sum of edges "
                  "attached to a node u.\n");
    outText << tr("In digraphs, the index is the sum of outbound arcs from "
                  "node u to all adjacent nodes.\n");
    outText << tr("If the network is weighted, the DC score is the sum of weights of outbound "
                  "edges from node u to all adjacent nodes.\n");
    outText << tr("DC' is the standardized DC\n\n");

    if (considerWeights){
        outText << tr("DC  range: 0 < C < undefined (valued graph)")<<"\n";
    }
    else
        outText << tr("DC  range: 0 < C < ")<<QString::number(maximumIndexValue)<<"\n";


    outText << "DC' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tDC\tDC'\t%DC'\n";
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText << (*it)->name()<<"\t"
                    <<(*it)->DC() << "\t"<< (*it)->SDC() << "\t"
                   <<  ( 100* ((*it)->SDC() ) )<< "\n";
        }
    }
    if ( minDC == maxDC ) {
        outText << "\n" << tr("All nodes have the same DC score.") << "\n";
    }
    else  {
        outText << "\n";
        outText << tr("Max DC' = ") << maxDC <<" (node "<< maxNodeDC <<  ")  \n";
        outText << tr("Min DC' = ") << minDC <<" (node "<< minNodeDC <<  ")  \n";
        outText << tr("DC classes = ") << classesDC<<" \n";
    }

    outText << "\n";
    outText << tr("DC sum = ") << t_sumDC<<"\n" ;
    outText << tr("DC' sum = ") << sumDC<<"\n" ;
    outText << tr("DC' Mean = ") << meanDC<<"\n" ;
    outText << tr("DC' Variance = ") << varianceDC<<"\n";

    if (!considerWeights) {
        outText << "\nGROUP DEGREE CENTRALISATION (GDC)\n\n";
        outText << "GDC = " << qSetRealNumberPrecision(m_precision) << groupDC<<"\n\n";

        outText << "GDC range: 0 < GDC < 1\n";
        outText << "GDC = 0, when all out-degrees are equal (i.e. regular lattice).\n";
        outText << "GDC = 1, when one node completely dominates or overshadows the other nodes.\n";
        outText << "(Wasserman & Faust, formula 5.5, p. 177)\n\n";
        outText << "(Wasserman & Faust, p. 101)\n";
    }
    else {
        outText << "This graph is weighted. No GDC value can be computed. \n"
                << "You can use DC mean or variance as a group-level DC measure";
    }

    outText << "\n\n";
    outText << tr("Degree Centrality (Out-Degree) Report, \n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}

/**
 * @brief Graph::centralityClosenessImproved
 * Improved node-level centrality closeness index which focuses on the
 * influence range of each node (the set of nodes that are reachable from it)
 * For each node v, this index calculates the fraction of nodes in its influence
 * range and divides it by the average distance of those nodes from v,
 * ignoring nodes that are not reachable from it.
 */
void Graph::centralityClosenessInfluenceRange(const bool considerWeights,
                                              const bool inverseWeights,
                                              const bool dropIsolates){
    qDebug()<< "Graph::centralityClosenessImproved()";
    if (!graphModified && calculatedIRCC ) {
        qDebug() << "Graph::centralityClosenessImproved() - "
                    " graph not changed - returning";
        return;
    }
     if (!reachabilityMatrixCreated || graphModified) {
         qDebug()<< "Graph::centralityClosenessImproved() - "
                    "call reachabilityMatrix()";
        reachabilityMatrix(considerWeights, inverseWeights, dropIsolates);
     }
    // calculate centralities
    QList<Vertex*>::const_iterator it;
    float IRCC=0,SIRCC=0;
    float Ji=0;
    classesIRCC=0;
    discreteIRCCs.clear();
    sumIRCC=0;
    maxIRCC=0;
    minIRCC=vertices(dropIsolates)-1;
    float V=vertices(dropIsolates);
    varianceIRCC=0;
    meanIRCC=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        IRCC=0;
        if (!(*it)->isIsolated()) {
            // find connected nodes
            QList<int> influencedVertices = influenceRanges.values((*it)->name()-1);
            Ji=influencedVertices.size();
            for (int i = 0; i < Ji; ++i) {
                qDebug() << "Graph:: centralityClosenessImproved - vertex " <<  (*it)->name()
                         << " is outbound connected to  = " << influencedVertices.at(i) + 1
                         << " at distance " << DM.item ((*it)->name()-1, influencedVertices.at(i) );
                IRCC += DM.item ((*it)->name()-1, influencedVertices.at(i) ) ;
            }
            qDebug()<< "Graph:: centralityClosenessImproved -  size of influenceRange Ji = " << Ji
                    << " IRCC=" << IRCC << " divided by Ji=" << Ji << " yields final IRCC =" << IRCC / Ji;
            // sanity check for IRCC=0 (=> node is disconnected)
            if (IRCC != 0)  {
                IRCC /= Ji;
                IRCC =  ( Ji / (V-1) ) / IRCC;
            }
        }
        sumIRCC += IRCC;
        (*it) -> setIRCC ( IRCC ) ;
        (*it) -> setSIRCC ( IRCC ) ;  // IRCC is a ratio, already std
        resolveClasses(IRCC, discreteIRCCs, classesIRCC);
        minmax( IRCC, (*it), maxIRCC, minIRCC, maxNodeIRCC, minNodeIRCC) ;
        qDebug() << "Graph::centralityClosenessImproved - vertex " <<  (*it)->name()
                 << " has IRCC = "
                 << Ji / (V-1) << " / " << IRCC << " = " << (*it)->IRCC();
    }

    meanIRCC = sumIRCC / (float) V;
    qDebug("Graph::centralityClosenessImproved - sumIRCC = %f, meanIRCC = %f",
           sumIRCC, meanIRCC);

    if (minIRCC == maxIRCC)
        maxNodeIRCC=-1;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! dropIsolates || ! (*it)->isIsolated()  ) {
            SIRCC= (*it) -> SIRCC();
            varianceIRCC += (SIRCC-meanIRCC) * (SIRCC-meanIRCC) ;
        }
    }
    varianceIRCC=varianceIRCC/(float) V;

    calculatedIRCC=true;
}

//Writes the closeness centralities to a file
void Graph::writeCentralityCloseness(
        const QString fileName,
        const bool considerWeights,
        const bool inverseWeights,
        const bool dropIsolates) {
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    if (graphModified || !calculatedCentralities ) {
            emit statusMessage ( (tr("Calculating shortest paths")) );
            createDistanceMatrix(true, considerWeights,
                                 inverseWeights, dropIsolates);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing closeness indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("CLOSENESS CENTRALITY (CC)")<<"\n";
    outText << tr("The CC index is the inverted sum of geodesic distances "
                  " from each node u to all other nodes.")<<"\n";
    outText << tr("CC' is the standardized CC (multiplied by N-1 minus isolates).")
            <<"\n";
    outText << tr("Note: The CC index considers outbound arcs only and "
                  "isolate nodes are dropped by default. Read the Manual for more.")
            << "\n\n";

    outText << tr("CC  range:  0 < C < ")<<QString::number(1.0/maxIndexCC)<<"\n";
    outText << tr("CC' range:  0 < C'< 1")<<"\n\n";
    outText << "Node"<<"\tCC\t\tCC'\t\t%CC'\n";
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText << (*it)->name()<<"\t"<<(*it)->CC() << "\t\t"
                    << (*it)->SCC() << "\t\t"
                    <<  (100* ((*it)->SCC()) )<<endl;
        }
    }
    qDebug ("min %f, max %f", minCC, maxCC);
    if ( minCC == maxCC ) {
        outText << "\n" << tr("All nodes have the same CC score.") << "\n";
    }
    else  {
        outText << "\n";
        outText << tr("Max CC' = ") << maxCC <<" (node "<< maxNodeCC  <<  ")  \n";
        outText << tr("Min CC' = ") << minCC <<" (node "<< minNodeCC <<  ")  \n";
        outText << tr("CC classes = ") << classesCC<<" \n\n";
    }
    outText << tr("CC' sum = ") << sumCC<<" \n";
    outText << tr("CC' Mean = ") << meanCC<<" \n";
    outText << tr("CC' Variance = ") << varianceCC<<" \n";

    if (!considerWeights) {
        outText << tr("\nGROUP CLOSENESS CENTRALISATION (GCC)\n\n");
        outText << tr("GCC = ") << groupCC<<"\n\n";

        outText << tr("GCC range: 0 < GCC < 1\n");
        outText << tr("GCC = 0, when the lengths of the geodesics are all equal "
                      "(i.e. a complete or a circle graph).\n");
        outText << tr("GCC = 1, when one node has geodesics of length 1 to all the "
                      "other nodes, and the other nodes have geodesics of length 2 "
                      "to the remaining (N-2) nodes. "
                      "This is exactly the situation realised by a star graph.\n");
        outText <<"(Wasserman & Faust, formula 5.9, p. 186-187)\n\n";
    }
    else
        outText << tr("Because this graphs is weighted, we cannot compute Group Centralization\n")
                << tr("Use variance instead.");
    outText << "\n\n";
    outText << tr("Closeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




//Writes the "improved" closeness centrality indices to a file
void Graph::writeCentralityClosenessInfluenceRange(const QString fileName,
                                                   const bool considerWeights,
                                                   const bool inverseWeights,
                                                   const bool dropIsolates)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( (tr("calculating IRCC indices")) );

    centralityClosenessInfluenceRange(considerWeights,inverseWeights, dropIsolates);

    emit statusMessage ( QString(tr("Writing IR closeness indices to file:")
                         .arg(fileName) ));
    outText.setRealNumberPrecision(m_precision);
    outText << tr("INFLUENCE RANGE CLOSENESS CENTRALITY (IRCC)")<<"\n";
    outText << tr("The IRCC index is the ratio of the fraction of nodes "
                   "reachable by each node u to the average distance of these nodes from u.\n"
                   "This index is optimized for graphs and directed graphs which "
                   "are not strongly connected. Read the Manual for more. ");
    outText <<"(Wasserman & Faust, formula 5.22, p. 201)\n\n";

    outText << tr("IRCC  range:  0 < IRCC < 1  (IRCC is a ratio)") << "\n\n";

    outText << "Node"<<"\tIRCC=IRCC'\t\t%IRCC'\n";
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText << (*it)->name()<<"\t"<<(*it)->IRCC() << "\t\t"
                << (100* ((*it)->SIRCC())  )<<endl;
        }
    }
    qDebug ("min %f, max %f", minIRCC, maxIRCC);
    if ( minIRCC == maxIRCC ){
        outText << "\n" << tr("All nodes have the same IRCC score.") << "\n";
    }
    else  {
        outText << "\n";
        outText << tr("Max IRCC = ") << maxIRCC <<" (node "<< maxNodeIRCC  <<  ")  \n";
        outText << tr("Min IRCC = ") << minIRCC <<" (node "<< minNodeIRCC <<  ")  \n";
        outText << tr("IRCC classes = ") << classesIRCC<<" \n\n";
    }
    outText << tr("IRCC sum = ") << sumIRCC<<"\n";
    outText << tr("IRCC Mean = ") << meanIRCC<<"\n";
    outText << tr("IRCC Variance = ") << varianceIRCC<<"\n";

    outText << "\n\n";
    outText << tr("InfluenceRange Closeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().
               toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}


//Writes the betweenness centralities to a file
void Graph::writeCentralityBetweenness(const QString fileName,
                                        const bool considerWeights,
                                        const bool inverseWeights,
                                       const bool dropIsolates) {
     QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");


    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true, considerWeights, inverseWeights, dropIsolates);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing betweenness indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("BETWEENESS CENTRALITY (BC)")<<"\n";
    outText << tr("The BC index of a node u is the sum of delta (s,t,u) for all s,t in V")<<"\n";
    outText << tr("where delta (s,t,u) is the ratio of all geodesics between "
                  "s and t which run through u. Read the Manual for more.")<<"\n";
    outText << tr("BC' is the standardized BC.")<<"\n\n";
    outText << tr("BC  range: 0 < BC < ")<<QString::number( maxIndexBC)
            << tr(" (Number of pairs of nodes excluding u)")<<"\n";
    outText << tr("BC' range: 0 < BC'< 1  (C' is 1 when the node falls on all geodesics)\n\n");
    outText << "Node"<<"\tBC\t\tBC'\t\t%BC'\n";
    QList<Vertex*>::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText <<(*it)->name()<<"\t"<<(*it)->BC()
               << "\t\t"<< (*it)->SBC() << "\t\t"
               <<  (100* ((*it)->SBC()))<<endl;
        }
    }
    if ( minBC ==  maxBC)
        outText << "\n" << tr("All nodes have the same BC score.") << "\n";
    else {
        outText << "\n";
        outText << tr("Max BC' = ") << maxBC <<" (node "<< maxNodeBC  <<  ")  \n";
        outText << tr("Min BC' = ") << minBC <<" (node "<< minNodeBC <<  ")  \n";
        outText << tr("BC classes = ") << classesBC<<" \n\n";
    }
    outText << tr("BC' sum = ") << sumBC<<" \n";
    outText << tr("BC' Mean = ") << meanBC<<" \n";
    outText << tr("BC' Variance = ") << varianceBC<<" \n";

    if (!considerWeights) {
        outText << tr("\nGROUP BETWEENESS CENTRALISATION (GBC)\n\n");
        outText << tr("GBC = ") <<  groupBC <<"\n\n";

        outText << tr("GBC range: 0 < GBC < 1\n");
        outText << tr("GBC = 0, when all the nodes have exactly the same betweenness index.\n");
        outText << tr("GBC = 1, when one node falls on all other geodesics between "
                      "all the remaining (N-1) nodes. "
                      "This is exactly the situation realised by a star graph.\n");
        outText << "(Wasserman & Faust, formula 5.13, p. 192)\n\n";
    }
    else
        outText << tr("Because this graph is weighted, we cannot compute Group Centralization\n")
                << tr("Use variance instead.");

    outText << "\n\n";
    outText << tr("Betweenness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



//Writes the Stress centralities to a file
void Graph::writeCentralityStress( const QString fileName,
                                   const bool considerWeights,
                                   const bool inverseWeights,
                                   const bool dropIsolates) {
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true, considerWeights, inverseWeights,dropIsolates);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing stress indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("STRESS CENTRALITY (SC)")<<"\n";
    outText << tr("The SC index of each node u is the sum of sigma(s,t,u): "
                  "the number of geodesics from s to t through u.")<<"\n";

    outText << tr("SC  range: 0 < SC < ")<<QString::number(maxIndexSC)<<"\n";
    outText << tr("SC' range: 0 < SC'< 1  (SC'=1 when the node falls on all "
                  "geodesics)\n\n");
    outText  << "Node"<<"\tSC\t\tSC'\t\t%SC'\n";
    QList<Vertex*>::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText <<(*it)->name()<<"\t"<<(*it)->SC() << "\t\t"
               << (*it)->SSC() << "\t\t"
               <<  (100* ((*it)->SSC()) )<<endl;
        }
    }

    if ( minSC ==  maxSC)
        outText  << tr("\nAll nodes have the same SC value.\n");
    else {
        outText << "\n";
        outText << tr("Max SC = ") << maxSC <<" (node "<< maxNodeSC  <<  ")  \n";
        outText << tr("Min SC = ") << minSC <<" (node "<< minNodeSC <<  ")  \n";
        outText << tr("SC classes = ") << classesSC<<" \n\n";
    }
    outText << tr("SC sum = ") << sumSC<<" \n";
    outText << tr("SC Mean = ") << meanSC<<" \n";
    outText << tr("SC Variance = ") << varianceSC<<" \n";

//    if (!considerWeights) {
//        outText << endl<< tr("GROUP STRESS CENTRALISATION (GSC)")<<"\n";
//        outText << tr("GSC = ") <<  groupSC<<"\n\n";

//        outText << tr("GSC range: 0 < GSC < 1\n");
//        outText << tr("GSC = 0, when all the nodes have exactly the same stress index.\n");
//        outText << tr("GSC = 1, when one node falls on all other geodesics between "
//                      "all the remaining (N-1) nodes. "
//                      "This is exactly the situation realised by a star graph.\n");
//    }
//    else
//        outText << tr("Because this graph is weighted, we cannot compute Group Centralization\n")
//                << tr("Use variance instead.");

    outText << "\n\n";
    outText << tr("Stress Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



void Graph::writeCentralityEccentricity(const QString fileName,
                                         const bool considerWeights,
                                         const bool inverseWeights,
                                        const bool dropIsolates) {
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true, considerWeights, inverseWeights,dropIsolates);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }
    emit statusMessage ( QString(tr("Writing eccentricity indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("ECCENTRICITY CENTRALITY (EC)") << "\n";
    outText << tr("The EC index of a node u is the inverse maximum geodesic distance "
                  " from u to all other nodes in the network.") << "\n";

    outText << tr("EC range: 0 < EC < 1 (GC=1 => max distance to all other nodes is 1)") << "\n\n";
    outText << "Node"<<"\tEC=EC'\t\t%EC\n";
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else  {
            outText << (*it)->name()<<"\t"<<(*it)->EC() << "\t\t"
                <<  (100* ((*it)->SEC()) )<<endl;
        }
    }
    if ( minEC ==  maxEC)
        outText << tr("\nAll nodes have the same EC value.\n");
    else {
        outText << "\n";
        outText << tr("Max EC = ") << maxEC <<" (node "<< maxNodeEC  <<  ")  \n";
        outText << tr("Min EC = ") << minEC <<" (node "<< minNodeEC <<  ")  \n";
        outText << tr("EC classes = ") << classesEC<<" \n\n";
    }
    outText << tr("EC sum = ") << sumEC<<" \n";
    outText << tr("EC Mean = ") << meanEC<<" \n";
    outText << tr("EC Variance = ") << varianceEC<<" \n";

    outText << "\n\n";
    outText << tr("Eccentricity Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




void Graph::writeCentralityPower(const QString fileName,
                                  const bool considerWeights,
                                  const bool inverseWeights,
                                 const bool dropIsolates) {
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true, considerWeights, inverseWeights, dropIsolates);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }
    emit statusMessage ( QString(tr("Writing Power indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);

    outText << tr("POWER CENTRALITY (PC)") << "\n";
    outText << tr("The PC index of a node u is the sum of the sizes of all Nth-order "
                  "neighbourhoods with weight 1/n.") << "\n";

    outText << tr("PC' is the standardized index: The PC score divided by the total number "
                  "of nodes in the same component minus 1") << "\n";
    outText << tr("PC  range: 0 < PC < ") << QString::number(maxIndexPC)
            << tr(" (star node)")<<"\n";
    outText << tr("PC' range: 0 < PC'< 1 \n\n");
    outText << "Node"<<"\tPC\t\tPC'\t\t%PC'\n";
    QList<Vertex*>::const_iterator it;
    for (it= m_graph.cbegin(); it!= m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText << (*it)->name()<<"\t"<<(*it)->PC() << "\t\t"
                << (*it)->SPC() << "\t\t"
                <<  (100* ((*it)->SPC()))<<endl;
        }

    }
    if ( minPC ==  maxPC)
        outText << tr("\nAll nodes have the same PC value.\n");
    else {
        outText << "\n";
        outText << tr("Max PC' = ") << maxPC <<" (node "<< maxNodePC  <<  ")  \n";
        outText << tr("Min PC' = ") << minPC <<" (node "<< minNodePC <<  ")  \n";
        outText << tr("PC classes = ") << classesPC<<" \n\n";
    }
    outText << tr("PC sum = ") << t_sumPC<<" \n";
    outText << tr("PC' sum = ") << sumPC<<" \n";
    outText << tr("PC' Mean = ") << meanPC<<" \n";
    outText << tr("PC' Variance = ") << variancePC<<" \n";

    if (!considerWeights) {
            outText << endl<<"GROUP POWER CENTRALIZATION (GPC)\n\n";
            outText << "GPC = " << groupPC<<"\n\n";

            outText << "GPC range: 0 < GPC < 1\n";
            outText << "GPC = 0, when all in-degrees are equal (i.e. regular lattice).\n";
            outText << "GPC = 1, when one node is linked to all other nodes (i.e. star).\n";
    }
    else {
        outText << tr("\nBecause the network is weighted, we cannot compute Group Centralization"
                   "You can use mean or variance instead.\n");
    }

    outText << "\n\n";
    outText << tr("Power Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}







/**
*	Calculates Degree Prestige (in-degree) of each vertex - diagonal included
*	Also the mean value and the variance of the in-degrees.
*/
void Graph::prestigeDegree(bool weights, bool dropIsolates=false){
    qDebug()<< "Graph::prestigeDegree()";
    if (!graphModified && calculatedDP ) {
        qDebug() << "Graph::prestigeDegree() - "
                    " graph not changed - returning";
        return;
    }
    float DP=0, SDP=0, nom=0, denom=0;
    float weight;
    classesDP=0;
    sumDP=0;
    t_sumDP=0;
    maxDP=0;
    minDP=vertices(dropIsolates)-1;
    discreteDPs.clear();
    varianceDP=0;
    meanDP=0;
    symmetricAdjacencyMatrix = true;
    QList<Vertex*>::const_iterator it, it1;
    H_StrToInt::iterator it2;
    int vert=vertices(dropIsolates);
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        DP=0;
        if (!(*it)->isIsolated()) {
            for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
                if ( (weight=this->hasArc ( (*it1)->name(), (*it)->name() ) ) !=0  )   {
                    if (weights)
                        DP+=weight;
                    else
                        DP++;
                }
                //check if the matrix is symmetric - we need this below
                if ( ( this->hasArc ( (*it1)->name(), (*it)->name() ) )
                     != ( this->hasArc ( (*it)->name(), (*it1)->name() ) )   )
                    symmetricAdjacencyMatrix = false;
            }
        }
        (*it) -> setDP ( DP ) ;		//Set DP
        t_sumDP += DP;
        qDebug() << "Graph: prestigeDegree() vertex " <<  (*it)->name()
                 << " DP "  << DP;
    }

    // Calculate std DP, min,max, mean
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        DP= (*it)->DP();
        if (!weights) {
            SDP=( DP / (vert-1.0) );		//Set Standard InDegree
        }
        else {
            SDP =( DP / (t_sumDP) );
        }
        (*it) -> setSDP( SDP );
        sumDP += SDP;
        qDebug() << "Graph::prestigeDegree - vertex " <<  (*it)->name() << " DP  "
                 << DP << " SDP " << (*it)->SDP ();
        it2 = discreteDPs.find(QString::number(SDP));
        if (it2 == discreteDPs.end() )	{
            classesDP++;
            qDebug("This is a new DP class");
            discreteDPs.insert ( QString::number(SDP), classesDP );
        }
        qDebug("DP classes = %i ", classesDP);
        if (maxDP < SDP ) {
            maxDP = SDP ;
            maxNodeDP=(*it)->name();
        }
        if (minDP > SDP ) {
            minDP = SDP ;
            minNodeDP=(*it)->name();
        }

    }

    if (minDP == maxDP)
        maxNodeDP=-1;

    meanDP = sumDP / (float) vert;
    qDebug("Graph: sumDP = %f, meanDP = %f", sumDP, meanDP);

    // Calculate Variance and the Degree Prestigation of the whole graph. :)
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        SDP= (*it)->SDP();
        nom+= maxDP-SDP;
        varianceDP += (SDP-meanDP) * (SDP-meanDP) ;
    }
    varianceDP=varianceDP/(float) vert;

    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);
    if (vert < 3 )
         denom = vert-1.0;

    //qDebug () << "*** vert is " << vert << " nom " << nom << " denom is " << denom;
    if (!weights) {
        groupDP=nom/denom;
        qDebug("Graph: varianceDP = %f, groupDP = %f", varianceDP, groupDP);
    }

    calculatedDP=true;
}




void Graph::writePrestigeDegree (const QString fileName,
                                 const bool considerWeights,
                                 const bool dropIsolates)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    prestigeDegree(considerWeights, dropIsolates);

    float maximumIndexValue=vertices()-1.0;
    outText.setRealNumberPrecision(m_precision);
    outText << tr("DEGREE PRESTIGE (DP)\n");
    outText << tr("The DP index of a node u is the sum of inbound edges to "
                  "that node from all adjacent nodes.\n");
    outText << tr("If the network is weighted, DP is the sum of inbound arc "
                  "weights (inDegree) to node u from all adjacent nodes.\n");
    outText << tr("The DP index is also known as InDegree Centrality.") << "\n";
    outText << tr("DP' is the standardized DP (divided by N-1)\n\n");
    if (considerWeights){
        maximumIndexValue=(vertices()-1.0)*maxDP;
        outText << tr("DP  range: 0 < C < undefined (valued graph)")<<"\n";
    }
    else
        outText << tr("DP  range: 0 < C < ")<<maximumIndexValue<<"\n";
    outText << "DP' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tDP\tDP'\t%DP\n";

    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText <<(*it)->name()<<"\t"<<(*it)->DP() << "\t"<< (*it)->SDP()
               << "\t" <<  (100* ((*it)->SDP()) )<<endl;
        }
    }

    if ( minDP == maxDP )
        outText << "\n"<< tr("All nodes have the same DP value.") << "\n";
    else  {
        outText << "\n"<< tr("Max DP' = ") << maxDP <<" (node "<< maxNodeDP <<  ")  \n";
        outText << tr("Min DP' = ") << minDP <<" (node "<< minNodeDP <<  ")  \n";
        outText << tr("DP classes = ") << classesDP<<" \n";
    }

    outText << "\n";
    outText << tr("DP sum = ") << t_sumDP<<"\n" ;
    outText << tr("DP' sum = ") << sumDP<<"\n" ;
    outText << tr("DP' Mean = ") << meanDP<<"\n" ;
    outText << tr("DP' Variance = ") << varianceDP<<"\n";

    if (!considerWeights) {
            outText << endl<<"GROUP DEGREE PRESTIGE (GDP)\n\n";
            outText << "GDP = " << groupDP<<"\n\n";

            outText << "GDP range: 0 < GDP < 1\n";
            outText << "GDP = 0, when all in-degrees are equal (i.e. regular lattice).\n";
            outText << "GDP = 1, when one node is chosen by all other nodes (i.e. star).\n";
            outText << "(Wasserman & Faust, p. 203)\n";
    }
    else {
        outText << tr("\nBecause the network is weighted, we cannot compute Group Centralization"
                   "You can use mean or variance instead.\n");
    }

    outText << "\n\n";
    outText << tr("Degree Prestige Report, \n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




/**
 * @brief Graph::prestigeProximity
 * Calculates Proximity Prestige of each vertex
 * Also the mean value and the variance of it..
 */
void Graph::prestigeProximity( const bool considerWeights,
                               const bool inverseWeights,
                               const bool dropIsolates){
    qDebug()<< "Graph::prestigeProximity()";
    if (!graphModified && calculatedPP ) {
        qDebug() << "Graph::prestigeProximity() - "
                    " graph not changed - returning";
        return;
    }
    if (!reachabilityMatrixCreated || graphModified) {
        qDebug()<< "Graph::prestigeProximity() - "
                   "call reachabilityMatrix()";
        reachabilityMatrix(considerWeights, inverseWeights, dropIsolates);
    }
    // calculate centralities
    QList<Vertex*>::const_iterator it;
    float PP=0;
    float Ii=0;
    int i=0;
    classesPP=0;
    discretePPs.clear();
    sumPP=0;
    maxPP=0;
    minPP=vertices(dropIsolates)-1;
    float V=vertices(dropIsolates);
    variancePP=0;
    meanPP=0;
    H_StrToInt::iterator it2;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        PP=0; i=0;
        if (!(*it)->isIsolated()){
            // find connected nodes
            QList<int> influencerVertices = influenceDomains.values((*it)->name()-1);
            Ii=influencerVertices.size();
            qDebug()<< "Graph::PP - vertex " <<  (*it)->name()
                    << " Ii size: " << Ii;
            for ( i = 0; i < Ii; i++) {
                qDebug() << "Graph::PP - vertex " <<  (*it)->name()
                         << " is inbound connected from " << influencerVertices.at(i) + 1
                         << " at distance " << DM.item (  influencerVertices.at(i), (*it)->name()-1);
                PP += DM.item (  influencerVertices.at(i), (*it)->name()-1);
            }
            qDebug()<< "Graph::PP - "
                       "size of influenceDomain Ii = " << Ii
                    << " PP=" << PP << " divided by Ii=" << Ii
                    << " yields graph-dependant PP index =" << PP / Ii;

            qDebug() << "Graph::PP - vertex " <<  (*it)->name()
                     << " has PP = "
                     << Ii / (V-1) << " / " << PP / Ii << " = " << ( Ii / (V-1) ) / (PP/Ii);

            // sanity check for PP=0 (=> node is disconnected)
            if (PP != 0)  {
                PP /= Ii;
                PP =  ( Ii / (V-1) ) / PP;
            }
            sumPP += PP;
        }
        (*it) -> setPP ( PP ) ;
        (*it) -> setSPP ( PP ) ; // PP is already stdized

        it2 = discretePPs.find(QString::number(PP));
        if (it2 == discretePPs.end() )	{
            classesPP++;
            qDebug() << "PP = " << (*it) -> PP() <<  " - this is a new PP class" ;
            discretePPs.insert ( QString::number(PP), classesPP );
        }
        //qDebug("PP classes = %i ", classesPP);
        if (maxPP < PP ) {
            maxPP = PP ;
            maxNodePP=(*it)->name();
        }
        if (minPP > PP ) {
            minPP = PP ;
            minNodePP=(*it)->name();
        }

    }

    if (minPP == maxPP)
        maxNodePP=-1;

    meanPP = sumPP / (float) V;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated() ) {
            continue;
        }
        PP= (*it) -> PP();
        variancePP += (PP-meanPP) * (PP-meanPP) ;
    }

    variancePP=variancePP/(float) V;
    qDebug() << "Graph::prestigeProximity - sumPP = " << sumPP
                << " meanPP = " << meanPP
                << " variancePP " << variancePP;

    calculatedPP=true;

}



//Writes the proximity prestige indeces to a file
void Graph::writePrestigeProximity( const QString fileName,
                                    const bool considerWeights,
                                    const bool inverseWeights,
                                    const bool dropIsolates)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( (tr("Calculating prestige proximity indices")) );
    prestigeProximity(considerWeights, inverseWeights, dropIsolates);
    emit statusMessage ( QString(tr("Writing proximity prestige indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("PROXIMITY PRESTIGE (PP)\n"
                  "The PP index of a node u is the ratio of the proportion of "
                  "nodes who can reach u to the average distance these nodes are "
                  "from u. Read the Manual for more.") <<"\n\n";

    outText << tr("PP range:  0 < PP < 1 "
            " (PP is a ratio)")<<"\n\n";
    outText << "Node"<<"\tPP=PP'\t\t%PP\n";
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            outText << (*it)->name()<<"\t -- \t\t --" <<endl;
        }
        else {
            outText << (*it)->name()<<"\t"
                <<(*it)->PP() << "\t\t"
               <<  (100* (*it)->SPP() ) <<endl;
        }
    }
    qDebug ("min %f, max %f", minPP, maxPP);
    if ( minPP == maxPP )
        outText << "\n"<<tr("All nodes have the same PP value.\n");
    else  {
        outText << "\n";
        outText << tr("Max PP = ") << maxPP <<" (node "<< maxNodePP  <<  ")  \n";
        outText << tr("Min PP = ") << minPP <<" (node "<< minNodePP <<  ")  \n";
        outText << tr("PP classes = ") << classesPP<<" \n\n";
    }
    outText << tr("PP Sum= ") << sumPP<<"\n";
    outText << tr("PP Mean = ") << meanPP<<"\n";
    outText << tr("PP Variance = ") << variancePP<<"\n";

    outText << "\n\n";
    outText << tr("Proximity Prestige report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




//Calculates the PageRank Prestige of each vertex
void Graph::prestigePageRank(const bool dropIsolates){
    qDebug()<< "Graph::prestigePageRank()";
    if (! graphModified && calculatedPRP ) {
        qDebug() << " graph not changed - return ";
        return;
    }
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

    float PRP=0, oldPRP = 0;
    float SPRP=0;
    int iterations = 1; // a counter
    int referrer;
    float delta = 0.00001; // The delta where we will stop the iterative calculation
    float maxDelta = RAND_MAX;
    float sumInLinksPR = 0;  // temp var for inlinks sum PR
    float transferedPRP = 0;
    float inLinks = 0;       // temp var
    float outLinks = 0;       // temp var
    float t_variance=0;
    float aVert =  vertices(dropIsolates) ;
    QList<Vertex*>::const_iterator it;
    int relation=0;
    bool edgeStatus=false;
    H_edges::const_iterator jt;


    qDebug()<< "Graph::prestigePageRank() "
            << "active vertices: " << aVert
            << " total vertices: " << vertices();

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        // At first, PR scores have probability distribution
        // from 0 to 1, so each one is set to 1/N
        (*it)->setPRP( 1.0 / aVert );
        // compute inEdges() to warm up inEdgesConst for everyone
        inLinks = (*it)->inEdges();
        outLinks = (*it)->outEdges();
        qDebug() << "Graph::prestigePageRank() - node "
                 << (*it)->name() << " PR = " << (*it)->PRP()
                 << " inLinks (set const): " << inLinks
                 << " outLinks (set const): " << outLinks;
    }

    if ( totalEdges() == 0 ) {
        qDebug()<< "Graph::prestigePageRank() "
                <<" - all vertices are isolated and of equal PR. Stop";
        return;
    }

    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta) {

        qDebug()<< "Graph::prestigePageRank() - ITERATION : " << iterations;

        t_sumPRP=0;
        maxDelta = 0;

        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
        {
            sumInLinksPR = 0;
            oldPRP = (*it)->PRP();

            qDebug() << "Graph::prestigePageRank() - computing PR for node: "
                     << (*it)->name()  << " current PR " << oldPRP;

            if ( (*it)->isIsolated() ) {
                // isolates have constant PR = 1/N
                qDebug() << "Graph::prestigePageRank() - isolated - CONTINUE ";
                continue;
            }

            jt=(*it)->m_inEdges.cbegin();

            qDebug() << "Graph::prestigePageRank() - Iterate over inEdges of "
                     << (*it)->name() ;

            while ( jt != (*it) -> m_inEdges.cend() )
            {
                relation = jt.value().first;
                if ( relation != currentRelation() ){
                    ++jt;
                    continue;
                }
                edgeStatus=jt.value().second.second;
                if ( edgeStatus != true){
                    ++jt;
                    continue;
                }

                referrer = jt.key();

                qDebug() << "Graph::prestigePageRank() - Node " << (*it)->name()
                         << " inLinked from neighbor " << referrer  << " index "
                         << index[referrer];

                if ( this->hasArc( referrer , (*it)->name() ) )
                {
                    inLinks = m_graph[ index[referrer] ] ->inEdgesConst();
                    outLinks = m_graph[ index[referrer] ]-> outEdgesConst();

                    PRP =  m_graph[ index[referrer] ]->PRP();

                    transferedPRP = (outLinks != 0 ) ? ( PRP / outLinks ) : PRP;

                    qDebug()<< "Graph::prestigePageRank() - neighbor " << referrer
                            << " has PR = " << PRP
                            << " and outLinks = " << outLinks
                               << "  will transfer " << transferedPRP ;

                    sumInLinksPR +=  transferedPRP;

                }
                ++jt;
            }

            PRP = (1-d_factor) / aVert + d_factor * sumInLinksPR;

           (*it) -> setPRP ( PRP );

            t_sumPRP+=PRP;

            qDebug() << "Graph::prestigePageRank() - Node "
                     << (*it)->name()
                      << " new PR = " << PRP
                    << " old PR was = " << oldPRP
                    << " diff = " << fabs(PRP - oldPRP);

            // calculate diff from last PageRank value for this vertex
            // and set it to minDelta if the latter is bigger.

            if ( maxDelta < fabs(PRP - oldPRP) ) {
                maxDelta = fabs(PRP - oldPRP);
                qDebug()<< "Graph::prestigePageRank() - Setting new maxDelta = "
                        <<  maxDelta;
            }

        }
        // normalize in every iteration
        qDebug() << "Graph::prestigePageRank() - sumPRP for this iteration " <<
                    t_sumPRP;
        for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it) {
            PRP = (*it)->PRP();
            SPRP = PRP / t_sumPRP ;
//            (*it)->setPRP( SPRP ); // ???

            qDebug() << "Graph::prestigePageRank() - Node "
                     << (*it)->name()
                        << " normalized SPRP =  " << SPRP;
        }
        iterations++;
    }

    // calculate std and min/max PRPs
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
    {
        if (dropIsolates && (*it)->isIsolated()) {
            continue;
        }
        PRP = (*it)->PRP();
        SPRP = PRP / t_sumPRP ;
        (*it)->setSPRP( SPRP );
        sumPRP +=  SPRP;
        qDebug()<< "Graph::prestigePageRank() vertex: " <<  (*it)->name()
                << " PR = " << PRP << " standard PR = " << SPRP
                   << " t_sumPRP " << t_sumPRP;
    }

    if (aVert != 0 )
        meanPRP = sumPRP / aVert ;
    else
        meanPRP = SPRP;

    qDebug() << "sumPRP = " << sumPRP << "  aVert = " << aVert
             << "  meanPRP = " << meanPRP;

    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if (dropIsolates && (*it)->isIsolated()) {
            continue;
        }
        SPRP=(*it)->SPRP();
        resolveClasses(SPRP,discretePRPs,classesPRP);
        if ( SPRP > maxPRP ) {
            maxPRP = SPRP;
            maxNodePRP=(*it)->name();
        }
        if ( SPRP < minPRP ) {
            minPRP = SPRP;
            minNodePRP=(*it)->name();
        }

        t_variance = ( SPRP  - meanPRP  ) ;
        t_variance *=t_variance;
        qDebug() << "SPRP " <<  (*it)->SPRP() << "  t_variance "
                 << SPRP - meanPRP  << " t_variance^2" << t_variance ;
        variancePRP  += t_variance;
    }
    qDebug() << "PRP' Variance   " << variancePRP   << " aVert " << aVert ;
    variancePRP  = variancePRP  /  (aVert);
    qDebug() << "PRP' Variance: " << variancePRP   ;

    calculatedPRP= true;

    return;

}


//Writes the PageRank indices to a file
void Graph::writePrestigePageRank(const QString fileName, const bool dropIsolates){
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( (tr("Calculating PageRank indices. Please wait...")) );

    prestigePageRank(dropIsolates);

    emit statusMessage ( QString(tr("Writing PageRank indices to file: %1"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("PAGERANK PRESTIGE (PRP)")<<"\n";
    outText << tr("")<<"\n";
    outText << tr("PRP  range: (1-d)/N = ") << ( 1- d_factor ) / vertices()
            << " < PRP" << endl;
    outText << " d =" << d_factor   << endl;
    outText << tr("PRP' is the standardized PR (PR divided by sumPR)")<<"\n";
    outText << tr("PRP' range:  ") << " (1-d)/N < C'< 1" <<"\n\n";
    outText << "Node"<<"\tPRP\t\tPRP'\t\t%PRP'\n";
    QList<Vertex*>::const_iterator it;
    float PRP=0, SPRP=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        PRP = (*it)->PRP();
        SPRP = (*it)->SPRP();
        outText << (*it)->name()<<"\t"<< PRP << "\t\t"<< SPRP  << "\t\t"
                <<  ( 100* SPRP )<<endl;
    }
    qDebug ("min %f, max %f", minPRP, maxPRP);
    if ( minPRP == maxPRP )
        outText << tr("\nAll nodes have the same PRP value.\n");
    else  {
        outText << "\n";
        outText << tr("Max PRP = ") << maxPRP <<" (node "<< maxNodePRP  <<  ")  \n";
        outText << tr("Min PRP = ") << minPRP <<" (node "<< minNodePRP <<  ")  \n";
        outText << tr("PRP classes = ") << classesPRP<<" \n";
    }
    outText << "\n";

    outText << tr("PRP sum = ") << t_sumPRP << endl;
    outText << tr("PRP' sum = ") << sumPRP << endl;
    outText << tr("PRP' Mean = ") << meanPRP << endl;
    outText << tr("PRP' Variance = ") << variancePRP << endl<< endl;

    outText << tr("PageRank Prestige report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}





//Writes the clustering coefficients to a file
void Graph::writeClusteringCoefficient(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( (tr("Calculating local and network clustering...")) );

    averageCLC= clusteringCoefficient();

    emit statusMessage ( QString(tr("Writing clustering coefficients to file: "))
                         + fileName );

    outText.setRealNumberPrecision(m_precision);

    outText << tr("CLUSTERING COEFFICIENT (CLC) REPORT") << endl << endl;

    outText << tr("Local CLC  range: 0 < C < 1") << endl<<endl;
    outText << "Node"<<"\tLocal CLC\n";


    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        outText << (*it)->name()<<"\t"<<(*it)->CLC() <<endl;
    }

    outText << "\nAverage local Clustering Coefficient = "<<  averageCLC<<"\n" ;

    if (  minCLC ==  maxCLC )
        outText << "\nAll nodes have the same clustering coefficient value.\n";
    else  {
        outText << "\nNode "<<  maxNodeCLC
                << " has the maximum Clustering Coefficient: " <<  maxCLC <<"\n";
        outText << "\nNode "<<  minNodeCLC
                << " has the minimum Clustering Coefficient: " <<  minCLC <<"\n";
    }

    outText << endl;
    outText << tr("NETWORK AVERAGE CLUSTERING COEFFICIENT (GCLC)") << endl <<endl;
    outText << "GCLC = " <<  averageCLC <<"\n\n";
    outText << tr("Range: 0 < GCLC < 1\n");
    outText << tr("GCLC = 0, when there are no cliques (i.e. acyclic tree).\n");
    outText << tr(
      "GCLC = 1, when every node and its neighborhood are complete cliques.\n");

    outText <<"\n\n" ;
    outText << tr("Clustering Coefficient Report,\n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

    file.close();
}




//Writes the triad census to a file
void Graph::writeTriadCensus(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }

    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( (tr("Conducting triad census. Please wait....")) );
    if (graphModified || !calculatedTriad) {
        if (!triadCensus()){
            qDebug() << "Error in triadCensus(). Exiting...";
            file.close();
            return;
        }
    }

    emit statusMessage ( QString(tr("Writing triad census to file: ")) +
                         fileName );

    outText << tr("TRIAD CENSUS (TRC)\n\n");

    outText << "Type\t\tCensus\t\tExpected Value" << "\n";
    outText << "003" << "\t\t" << triadTypeFreqs[0] << "\n";
    outText << "012" << "\t\t" <<triadTypeFreqs[1] <<"\n";
    outText << "102" << "\t\t" <<triadTypeFreqs[2] <<"\n";
    outText << "021D"<< "\t\t" <<triadTypeFreqs[3] <<"\n";
    outText << "021U"<< "\t\t" <<triadTypeFreqs[4] <<"\n";
    outText << "021C"<< "\t\t" <<triadTypeFreqs[5] <<"\n";
    outText << "111D"<< "\t\t" <<triadTypeFreqs[6] <<"\n";
    outText << "111U"<< "\t\t" <<triadTypeFreqs[7] <<"\n";
    outText << "030T"<< "\t\t" <<triadTypeFreqs[8] <<"\n";
    outText << "030C"<< "\t\t" <<triadTypeFreqs[9] <<"\n";
    outText << "201" << "\t\t" <<triadTypeFreqs[10] <<"\n";
    outText << "120D"<< "\t\t" <<triadTypeFreqs[11] <<"\n";
    outText << "120U"<< "\t\t" <<triadTypeFreqs[12] <<"\n";
    outText << "120C"<< "\t\t" <<triadTypeFreqs[13] <<"\n";
    outText << "210" << "\t\t" <<triadTypeFreqs[14] <<"\n";
    outText << "300" << "\t\t" <<triadTypeFreqs[15] <<"\n";

    outText << "\n\n";
    outText << tr("Triad Census report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



/** 
* Repositions all nodes on the periphery of different circles with radius
* analogous to their prominence index
*/
void Graph::layoutCircularByProminenceIndex(double x0, double y0,
                                            double maxRadius,
                                            int prominenceIndex,
                                            const bool considerWeights,
                                            const bool inverseWeights,
                                            const bool dropIsolates){
    qDebug() << "Graph::layoutCircularByProminenceIndex - "
                << "prominenceIndex index = " << prominenceIndex;
    //first calculate centrality indices if needed

    if ( prominenceIndex == 1) {
            if (graphModified || !calculatedDC )
                centralityDegree(true, dropIsolates);
        }
        else if ( prominenceIndex == 3 ){
            if (graphModified || !calculatedIRCC )
                centralityClosenessInfluenceRange();
        }
        else if ( prominenceIndex == 8 ) {
            if (graphModified || !calculatedIC )
                centralityInformation();
        }
        else if ( prominenceIndex == 9){
            if (graphModified || !calculatedDP )
                prestigeDegree(true, dropIsolates);
        }
        else if ( prominenceIndex == 10 ) {
            if (graphModified || !calculatedPRP )
                prestigePageRank();
        }
        else if ( prominenceIndex == 11 ){
            if (graphModified || !calculatedPP )
                prestigeProximity(considerWeights, inverseWeights);
        }
        else{
            if (graphModified || !calculatedCentralities )
                createDistanceMatrix(true, considerWeights,
                                       inverseWeights, dropIsolates);
        }

    double rad=0;
    double i=0, std=0;
    //offset controls how far from the centre the central nodes be positioned
    float C=0, maxC=0, offset=0.06;
    double new_radius=0, new_x=0, new_y=0;

    int vert=vertices();
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        switch (prominenceIndex) {
            case 1 : {
                qDebug("Layout according to DC");
                C=(*it)->SDC();
                std= (*it)->SDC();
                maxC=maxDC;
                break;
            }
            case 2 : {
                qDebug("Layout according to CC");
                C=(*it)->CC();
                std= (*it)->SCC();
                maxC=maxCC;
                break;
            }
            case 3 : {
                qDebug("Layout according to IRCC");
                qDebug() << "Layout according to IRCC C = " << (*it)->IRCC();
                qDebug() << "Layout according to IRCC std = " << (*it)->SIRCC();
                qDebug() << "Layout according to IRCC maxC= " << maxIRCC;
                C=(*it)->IRCC();
                std= (*it)->SIRCC();
                maxC=maxIRCC;

                break;
            }
            case 4 : {
                qDebug("Layout according to BC");
                C=(*it)->BC();
                std= (*it)->SBC();
                maxC=maxBC;
                break;
            }
            case 5 : {
                qDebug("Layout according to SC");
                C=(*it)->SC();
                std= (*it)->SSC();
                maxC=maxSC;
                break;
            }
            case 6 : {
                qDebug("Layout according to EC");
                C=(*it)->EC();
                std= (*it)->SEC();
                maxC=maxEC;
                break;
            }
            case 7 : {
                qDebug("Layout according to PC");
                C=(*it)->PC();
                std= (*it)->SPC();
                maxC=maxPC;
                break;
            }
            case 8 : {
                qDebug("Layout according to IC");
                C=(*it)->IC();
                std= (*it)->SIC();
                maxC=maxIC;
                break;
            }
            case 9 : {
                qDebug("Layout according to DP");
                C=(*it)->SDP();
                std= (*it)->SDP();
                maxC=maxDP;
                break;
            }
            case 10 : {
                qDebug("Layout according to PRP");
                C=(*it)->PRP();
                std= (*it)->SPRP();
                maxC=maxPRP;
                break;
            }
            case 11 : {
                qDebug("Layout according to PP");
                C=(*it)->PP();
                std= (*it)->SPP();
                maxC=maxPP;
                break;
            }
        };
        qDebug () << "Vertice " << (*it)->name() << " at x=" << (*it)->x()
                  << ", y= "<< (*it)->y() << ": C=" << C << ", stdC=" << std
                  << ", maxradius " <<  maxRadius
                  << ", maxC " << maxC << ", C/maxC " << (C/maxC)
                  << ", newradius " << (std/maxC - offset)*maxRadius;
        switch (static_cast<int> (ceil(maxC)) ){
        case 0: {
            qDebug("maxC=0.   Using maxHeight");
            new_radius=maxRadius;
            break;
        }
        default: {
            new_radius=(maxRadius- (std/maxC - offset)*maxRadius);
            break;
        }
        };

        qDebug ("new radius %f", new_radius);

        //Calculate new position
        rad= (2.0* Pi/ vert );
        new_x=x0 + new_radius * cos(i * rad);
        new_y=y0 + new_radius * sin(i * rad);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug("Finished Calculation. Vertice will move to x=%f and y=%f ",
               new_x, new_y);
        //Move node to new position
        emit moveNode((*it)->name(),  new_x,  new_y);
        i++;
        emit addGuideCircle (
                    static_cast<int> (x0),
                    static_cast<int> (y0),
                    static_cast<int> (new_radius)
                    );
    }
    graphModified=true;
}




/** 
*	 Repositions all nodes on different random positions
* 	Emits moveNode(i, x,y) to tell GW that the node item should be moved. 
*/
void Graph::layoutRandom(double maxWidth, double maxHeight){	
    qDebug("Graph: layoutRandom...");
    double new_x=0, new_y=0;
    for (Vertices::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        new_x= rand() % ( static_cast<int> (maxWidth) );
        new_y= rand() % ( static_cast<int> (maxHeight) );
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug()<< "Graph: Emitting moveNode to move Vertice " << (*it)->name()
                   //<< "indexed " << index((*it)->name())
                << " to new position " << new_x << " , "<< new_y;
        emit moveNode((*it)->name(),  new_x,  new_y);
    }
    graphModified=true;
}



/**
 * @brief Graph::layoutCircularRandom
 * Repositions all nodes on the periphery of different circles with random radius
 * @param x0
 * @param y0
 * @param maxRadius
 */
void Graph::layoutCircularRandom(double x0, double y0, double maxRadius){
    qDebug() << "Graph::layoutCircularRandom - ";
    double rad=0, new_radius=0, new_x=0, new_y=0;
    double i=0;
    //offset controls how far from the centre the central nodes be positioned
    float offset=0.06, randomDecimal=0;
    int vert=vertices();
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        randomDecimal = (float ) ( rand()%100 ) / 100.0;
        new_radius=(maxRadius- (randomDecimal - offset)*maxRadius);
        qDebug () << "Vertice " << (*it)->name()
                  << " at x=" << (*it)->x()
                  << ", y= "<< (*it)->y()
                  << ", maxradius " <<  maxRadius
                  << " randomDecimal " << randomDecimal
                  << " new radius " << new_radius;

        //Calculate new position
        rad= (2.0* Pi/ vert );
        new_x=x0 + new_radius * cos(i * rad);
        new_y=y0 + new_radius * sin(i * rad);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug("Vertice will move to x=%f and y=%f ", new_x, new_y);
        //Move node to new position
        emit moveNode((*it)->name(),  new_x,  new_y);
        i++;
        emit addGuideCircle (
                    static_cast<int> (x0),
                    static_cast<int> (y0),
                    static_cast<int> (new_radius)
                    );
    }
    graphModified=true;
}




/** 
*	 Repositions all nodes on different top-down levels according to their centrality
* 	Emits moveNode(i, x,y) to tell GW that the node item should be moved. 
*/
void Graph::layoutLevelByProminenceIndex(double maxWidth, double maxHeight,
                                         int prominenceIndex,
                                         const bool considerWeights,
                                         const bool inverseWeights,
                                         const bool dropIsolates){
    qDebug("Graph: layoutLevelCentrality...");

    if ( prominenceIndex == 1) {
            if (graphModified || !calculatedDC )
                centralityDegree(true, dropIsolates);
        }
        else if ( prominenceIndex == 3 ){
            if (graphModified || !calculatedIRCC )
                centralityClosenessInfluenceRange();
        }
        else if ( prominenceIndex == 8 ) {
            if (graphModified || !calculatedIC )
                centralityInformation();
        }
        else if ( prominenceIndex == 9){
            if (graphModified || !calculatedDP )
                prestigeDegree(true, dropIsolates);
        }
        else if ( prominenceIndex == 10 ) {
            if (graphModified || !calculatedPRP )
                prestigePageRank();
        }
        else if ( prominenceIndex == 11 ){
            if (graphModified || !calculatedPP )
                prestigeProximity(considerWeights, inverseWeights);
        }
        else{
            if (graphModified || !calculatedCentralities )
                createDistanceMatrix(true, considerWeights,
                                       inverseWeights, dropIsolates);
        }

    double i=0, std=0;
    //offset controls how far from the top the central nodes will be
    float C=0, maxC=0, offset=50;
    double new_x=0, new_y=0;
    //	int vert=vertices();
    maxHeight-=offset;
    maxWidth-=offset;
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        switch (prominenceIndex) {
            case 1 : {
                qDebug("Layout according to DC");
                C=(*it)->SDC();
                std= (*it)->SDC();
                maxC=maxDC;
                break;
            }
            case 2 : {
                qDebug("Layout according to CC");
                C=(*it)->CC();
                std= (*it)->SCC();
                maxC=maxCC;
                break;
            }
            case 3 : {
                qDebug("Layout according to IRCC");
                C=(*it)->IRCC();
                std= (*it)->SIRCC();
                maxC=maxIRCC;
                break;
            }
            case 4 : {
                qDebug("Layout according to BC");
                C=(*it)->BC();
                std= (*it)->SBC();
                maxC=maxBC;
                break;
            }
            case 5 : {
                qDebug("Layout according to SC");
                C=(*it)->SC();
                std= (*it)->SSC();
                maxC=maxSC;
                break;
            }
            case 6 : {
                qDebug("Layout according to EC");
                C=(*it)->EC();
                std= (*it)->SEC();
                maxC=maxEC;
                break;
            }
            case 7 : {
                qDebug("Layout according to PC");
                C=(*it)->PC();
                std= (*it)->SPC();
                maxC=maxPC;
                break;
            }
            case 8 : {
                qDebug("Layout according to IC");
                C=(*it)->IC();
                std= (*it)->SIC();
                maxC=maxIC;
                break;
            }
            case 9 : {
                qDebug("Layout according to DP");
                C=(*it)->SDP();
                std= (*it)->SDP();
                maxC=maxDP;
                break;
            }
            case 10 : {
                qDebug("Layout according to PRP");
                C=(*it)->PRP();
                std= (*it)->SPRP();
                maxC=maxPRP;
                break;
            }
            case 11 : {
                qDebug("Layout according to PP");
                C=(*it)->PP();
                std= (*it)->SPP();
                maxC=maxPP;
                break;
            }
        };
        qDebug()<< "Vertice " << (*it)->name()
                << " at x="<< (*it)->x() << ", y="<<  (*it)->y()
                << ": C=" << C << ", stdC=" << std
                << ", maxC "<<	maxC << ", maxWidth " << maxWidth
                <<" , maxHeight "<<maxHeight;
        //Calculate new position
        qDebug ("std %f, maxHeight-(std)*maxHeight = %f "
                , std, maxHeight-(std)*maxHeight );
        switch ( static_cast<int> (ceil(maxC)) ){
        case 0: {
            qDebug("maxC=0.   Using maxHeight");
            new_y=maxHeight;
            break;
        }
        default: {
            new_y=offset/2.0+maxHeight-(std/maxC)*maxHeight;
            break;
        }
        };
        new_x=offset/2.0 + rand() % ( static_cast<int> (maxWidth) );
        qDebug ("new_x %f, new_y %f", new_x, new_y);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug() << "Finished Calculation. "
                    "Vertice will move to x="<< new_x << " and y= " << new_y;
        //Move node to new position
        emit moveNode((*it)->name(),  new_x,  new_y);
        i++;
        emit addGuideHLine(static_cast<int> ( new_y ) );
    }
//    graphModified=true;
//    emit graphChanged();
}




/**
 * @brief Graph::layoutVerticesSizeByProminenceIndex
 * changes the node size to be proportinal to given prominence index
 * @param prominenceIndex
 */
void Graph::layoutVerticesSizeByProminenceIndex (int prominenceIndex,
                                                 const bool considerWeights,
                                                 const bool inverseWeights,
                                                 const bool dropIsolates){
    qDebug() << "Graph::layoutVerticesSizeByProminenceIndex - "
                << "prominenceIndex index = " << prominenceIndex;
    double std=0;
    float C=0, maxC=0;
    int new_size=0;

    //first calculate centrality indices if needed
    if ( prominenceIndex == 0) {
        // do nothing
    }
    else if ( prominenceIndex == 1) {
        if (graphModified || !calculatedDC )
            centralityDegree(true, dropIsolates);
    }
    else if ( prominenceIndex == 3 ){
        if (graphModified || !calculatedIRCC )
            centralityClosenessInfluenceRange();
    }
    else if ( prominenceIndex == 8 ) {
        if (graphModified || !calculatedIC )
            centralityInformation();
    }
    else if ( prominenceIndex == 9){
        if (graphModified || !calculatedDP )
            prestigeDegree(true, dropIsolates);
    }
    else if ( prominenceIndex == 10 ) {
        if (graphModified || !calculatedPRP )
            prestigePageRank();
    }
    else if ( prominenceIndex == 11 ){
        if (graphModified || !calculatedPP )
            prestigeProximity(considerWeights, inverseWeights);
    }
    else{
        if (graphModified || !calculatedCentralities )
            createDistanceMatrix(true, considerWeights,
                                   inverseWeights, dropIsolates);
    }
    QList<Vertex*>::const_iterator it;
    for  (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        switch (prominenceIndex) {
        case 0: {
            C=0;maxC=0;
            break;
        }
            case 1 : {
                qDebug("VerticesSize according to DC");
                C=(*it)->SDC();
                std= (*it)->SDC();
                maxC=maxDC;
                break;
            }
            case 2 : {
                qDebug("VerticesSize according to CC");
                C=(*it)->CC();
                std= (*it)->SCC();
                maxC=maxCC;
                break;
            }
            case 3 : {
                qDebug("VerticesSize according to IRCC");
                C=(*it)->IRCC();
                std= (*it)->SIRCC();
                maxC=maxIRCC;
                break;
            }
            case 4 : {
                qDebug("VerticesSize according to BC");
                C=(*it)->BC();
                std= (*it)->SBC();
                maxC=maxBC;
                break;
            }
            case 5 : {
                qDebug("VerticesSize according to SC");
                C=(*it)->SC();
                std= (*it)->SSC();
                maxC=maxSC;
                break;
            }
            case 6 : {
                qDebug("VerticesSize according to EC");
                C=(*it)->EC();
                std= (*it)->SEC();
                maxC=maxEC;
                break;
            }
            case 7 : {
                qDebug("VerticesSize according to PC");
                C=(*it)->PC();
                std= (*it)->SPC();
                maxC=maxPC;
                break;
            }
            case 8 : {
                qDebug("VerticesSize according to IC");
                C=(*it)->IC();
                std= (*it)->SIC();
                maxC=maxIC;
                break;
            }
            case 9 : {
                qDebug("VerticesSize according to DP");
                C=(*it)->SDP();
                std= (*it)->SDP();
                maxC=maxDP;
                break;
            }
            case 10 : {
                qDebug("VerticesSize according to PRP");
                C=(*it)->PRP();
                std= (*it)->SPRP();
                maxC=maxPRP;
                break;
            }
            case 11 : {
                qDebug("VerticesSize according to PP");
                C=(*it)->PP();
                std= (*it)->SPP();
                maxC=maxPP;
                break;
            }
        };
        qDebug () << "Vertex " << (*it)->name()
                  << ": C=" << C << ", stdC=" << std
                  << ", maxC " << maxC
                  << "initVertexSize " << initVertexSize
                  << " stdC/maxC " << std/maxC
                  << ", (std/maxC) * initVertexSize " << (std/maxC *initVertexSize);

        switch (static_cast<int> (ceil(maxC) )){
        case 0: {
            qDebug()<<"maxC=0.   Using initVertexSize";
            new_size=initVertexSize;
            //emit signal to change node size
            emit setNodeSize((*it)->name(),  new_size);
            break;
        }
        default: {
            //Calculate new size
            new_size=ceil ( initVertexSize/2.0 + (float) initVertexSize * (std/maxC));
            qDebug ()<< "new vertex size "<< new_size << " call setSize()";
            (*it)->setSize(new_size);
            //emit signal to change node size
            emit setNodeSize((*it)->name(),  new_size);
            break;
        }
        };
    }
//    graphModified=true;
//    emit graphChanged();
}




/**
    Adds a little universal randomness :)
*/
void Graph::makeThingsLookRandom()   {
    time_t now;				/* define 'now'. time_t is probably a typedef	*/
    now = time((time_t *)NULL);		/* Get the system time and put it
                     * into 'now' as 'calender time' the number of seconds since  1/1/1970   	*/

    srand( (unsigned int ) now);
}



/** layman's attempt to create a random network
*/
void Graph::createRandomNetErdos(  const int &vert,
                                   const QString &model,
                                   const int &edges,
                                   const float &eprob,
                                   const QString &mode,
                                   const bool &diag)
{
    qDebug() << "Graph::createRandomNetErdos() - vertices " << vert
                << " model " << model
                << " edges " << edges
                << " edge probability " << eprob
                << " graph mode " << mode
                << " diag " << diag;

    index.reserve(vert);

    makeThingsLookRandom();

    int progressCounter=0;
    int edgeCount = 0;

    qDebug() << "Graph::createRandomNetErdos() - Creating nodes...";

    for (register int i=0; i< vert ; i++)
    {
        int x=10+rand() %640;
        int y=10+rand() %480;
        qDebug("Graph: createRandomNetErdos, new node i=%i, at x=%i, y=%i", i+1, x,y);
        createVertex (
                    i+1, initVertexSize, initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape, false
                    );
        progressCounter++;
        emit updateProgressDialog( progressCounter );
    }

    qDebug() << "Graph::createRandomNetErdos() - Creating edges...";
    if ( model == "G(n,p)")
    {
        qDebug() << "Graph::createRandomNetErdos() - G(n,p) model...";
        for (register int i=0;i<vert; i++) {
            for (register int j=0; j<vert; j++) {
                qDebug() << "Graph::createRandomNetErdos() - Bernoulli trial "
                       << "for edge " <<  i+1 << " -> " << j+1;
                if (!diag && i==j) {
                    qDebug()<< " Graph::createRandomNetErdos() - skip because "
                            << i+1 << " = " << j+1
                            << " and diag " << diag;
                    continue;
                }
                if ( ( rand() % 100 + 1 ) / 100.0 < eprob )    {
                    edgeCount ++ ;

                    if (mode == "graph") {
                        qDebug() << "Graph::createRandomNetErdos() - "
                                    <<" create undirected Edge no "
                                    << edgeCount;
                        createEdge(i+1, j+1, 1, "black", 2, true, false);
                    }
                    else {
                        qDebug() << "Graph::createRandomNetErdos() - "
                                    <<" create directed Edge no "
                                    << edgeCount;

                        createEdge(i+1, j+1, 1, "black", 0, true, false);
                    }
                }
                else
                    qDebug() << "Graph::createRandomNetErdos() - do not create Edge";
            }
            progressCounter++;
            emit updateProgressDialog(progressCounter );
            qDebug("Emitting UPDATE PROGRESS %i", progressCounter);
        }

    }
    else
    {
        qDebug() << "Graph::createRandomNetErdos() - G(n,M) model...";
        int source = 0, target = 0 ;
        do {
            source =  rand() % vert + 1;
            target =  rand() % vert + 1;
            qDebug() << "Graph::createRandomNetErdos() - random pair "
                        << " " << source
                           << " , " << target ;
            if (!diag && source == target ) {
                qDebug() << "Graph::createRandomNetErdos() - skip self loop pair ";
                continue;
            }
            if ( hasArc(source, target) ) {
                qDebug() << "Graph::createRandomNetErdos() - skip pair - exists";
                continue;
            }
            edgeCount ++;
            if (mode == "graph") {
                qDebug() << "Graph::createRandomNetErdos() - create "
                            << " undirected Edge no " << edgeCount;
                createEdge(source, target, 1, "black", 2, true, false);
            }
            else {
                qDebug() << "Graph::createRandomNetErdos() - create "
                            << " directed Edge no " << edgeCount;
                createEdge(source, target, 1, "black", 0, true, false);
            }

        } while ( edgeCount != edges );

    }


    addRelationFromGraph(tr("erdos-renyi")); //FIXME

    emit graphChanged();
}




/** layman's attempt to create a random ring lattice network.
*/

void Graph::createRandomNetRingLattice( 
        int vert, int degree,
        double x0, double y0, double radius)
{
    qDebug("Graph: createRingLatticeNetwork");
    int x=0;
    int y=0;
    int progressCounter=0;

    double rad= (2.0* Pi/ vert );

    makeThingsLookRandom();

    index.reserve(vert);

    for (register int i=0; i< vert ; i++) {
        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);
        createVertex(	i+1,initVertexSize,initVertexColor,
                        initVertexNumberColor, initVertexNumberSize,
                        QString::number (i+1), initVertexLabelColor,  initVertexLabelSize,
                        QPoint(x, y), initVertexShape, false);
        qDebug("Graph: createPhysicistLatticeNetwork, new node i=%i, at x=%i, y=%i", i+1, x,y);
        progressCounter++;
        emit updateProgressDialog( progressCounter );
    }
    int target = 0;
    for (register int i=0;i<vert; i++){
        qDebug("Creating links for node %i = ", i+1);
        for (register int j=0; j< degree/2 ; j++) {
            target = i + j+1 ;
            if ( target > (vert-1))
                target = target-vert;
            qDebug("Creating Link between %i  and %i", i+1, target+1);
            createEdge(i+1, target+1, 1, "black", true, true, false);
        }
        progressCounter++;
        emit updateProgressDialog(progressCounter );
        qDebug("Emitting UPDATE PROGRESS %i", progressCounter);
    }
    addRelationFromGraph(tr("ring lattice"));

    emit graphChanged();
}



void Graph::createRandomNetScaleFree (const int &n,
                                       const int &power,
                                       const int &m0,
                                       const int &m,
                                       const float &alpha,
                                       const QString &mode,
                                       const double &x0,
                                       const double &y0,
                                       const double &radius)
{
    qDebug() << "Graph::createRandomNetScaleFree() - "
                << "Create initial connected net of m0 nodes";

    int progressCounter=0;

    makeThingsLookRandom();

    int x=0;
    int y=0;
    int newEdges = 0;
    double sumDegrees=0;
    double k_j;
    double rad= (2.0* Pi/ n );
    double  prob_j = 0, prob=0;

    index.reserve( n );

    for (register int i=0; i< m0 ; ++i) {
        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);

        qDebug() << "Graph::createRandomNetScaleFree() - "
                    << " initial node i " << i+1 << " pos " << x << "," << y;
        createVertex(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape,false
                    );
        progressCounter++;
        emit updateProgressDialog( progressCounter );
    }

    for (register int i=0; i < m0; ++i){
        qDebug() << "Graph::createRandomNetScaleFree() - "
                   << " Creating all edges for initial node i " << i+1;
        for (register int j=i+1; j< m0  ; ++j) {
            qDebug() << " --- Creating initial edge " << i+1 << " <-> " << j+1;
            createEdge (i+1, j+1, 1, "black", 2, true, false);
        }
        progressCounter++;
        emit updateProgressDialog(progressCounter );
    }

    qDebug()<< endl << "Graph::createRandomNetScaleFree() - "
               << " start network growth to " << n
               << " nodes with preferential attachment" << endl;

    for (register int i= m0 ; i < n ; ++i) {

        x=x0 + radius * cos(i * rad);
        y=y0 + radius * sin(i * rad);

        qDebug() << "Graph::createRandomNetScaleFree() - "
                    << " adding new node i " << i+1
                    << " pos " << x << "," << y << endl;

        createVertex(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape,false
                    );
        progressCounter++;
        emit updateProgressDialog( progressCounter );

        // no need to multiply by 2, since totalEdges already reports
        // twice the current number of edges in the network
        sumDegrees =  totalEdges();

        newEdges = 0;

        for (;;)
        {	//do until we create m new edges

            for (register int j=0; j < i  ; ++j) {
                qDebug() << "Graph::createRandomNetScaleFree() - "
                           << " preferential attachment test of new node i "
                           << i+1
                           << " with node j " << j+1
                            << " - newEdges " << newEdges ;

                if (newEdges == m)
                    break;

                k_j = inDegree(j+1);
                k_j = pow ( k_j , power );
                if (sumDegrees < 1 )
                    prob_j = 1; // always create edge if no other edge exist
                else
                    prob_j = ( alpha + k_j  ) / sumDegrees ;

                prob  = ( rand() % 100 + 1 ) / 100.0;

                qDebug() << "Graph::createRandomNetScaleFree() - "
                            << " Edge probability with old node "
                            << j+1 << " is: alpha + k_j ^ power " << alpha + k_j
                             << " / sumDegrees " << sumDegrees
                             << " = prob_j " << prob_j
                                << " prob " << prob ;

                if ( prob  <=  prob_j )  {
                    if ( mode == "graph") {
                        qDebug() << " --- Creating pref.att. reciprocal edge "
                                 <<  i+1 << " <-> " << j+1;
                        createEdge (i+1, j+1, 1, "black", 2, true, false);
                        newEdges ++;

                    }
                    else {
                        qDebug() << " --- Creating pref.att. directed edge "
                                 <<  i+1 << " <-> " << j+1;
                        createEdge (i+1, j+1, 1, "black", 1, true, false);
                        newEdges ++;

                    }
                }
            }
            if ( newEdges == m )
                break;
        }
    }

    addRelationFromGraph(tr("scale-free"));
    emit signalNodeSizesByInDegree(true);
    emit graphChanged();

}



void Graph::createRandomNetSmallWorld (
        int vert, int degree, double beta,
        double x0, double y0, double radius)
{
    qDebug("Graph: createRandomNetSmallWorld. First creating a ring lattice");

    createRandomNetRingLattice(vert, degree, x0, y0, radius);

    qDebug("******** Graph: REWIRING starts...");

    int candidate;

    for (register int i=1;i<vert; i++) {
        for (register int j=i+1;j<vert; j++) {
            qDebug()<<">>>>> REWIRING: Check if  "<< i << " is linked to " << j;
            if ( this-> hasArc(i, j) ) {
                qDebug()<<">>>>> REWIRING: They're linked. Do a random REWIRING "
                          "Experiment between "<< i<< " and " << j
                       << " Beta parameter is " << beta;
                if (rand() % 100 < (beta * 100))  {
                    qDebug(">>>>> REWIRING: We'l break this edge!");
                    removeEdge(i, j);
                    removeEdge(j, i);
                    qDebug()<<">>>>> REWIRING: OK. Let's create a new edge!";
                    for (;;) {	//do until we create a new edge
                        candidate=rand() % (vert+1) ;		//pick another vertex.
                        if (candidate == 0 || candidate == i) continue;
                        qDebug()<<">>>>> REWIRING: Candidate: "<< candidate;
                        //Only if differs from i and hasnot edge with it
                        if (! this->hasArc(i, candidate) )
                            qDebug("<----> Random New Edge Experiment between %i and %i:", i, candidate);
                        if (rand() % 100 > 0.5) {
                            qDebug("Creating new link!");
                            createEdge(i, candidate, 1, "black", true, true, false);
                            break;
                        }
                    }
                }
                else  qDebug("Will not break link!");
            }
        }
    }

    emit signalNodeSizesByInDegree(true);
}





/** layman's attempt to create a random network where nodes have the same degree.
*/

void Graph::createSameDegreeRandomNetwork(int vert, int degree){
    qDebug("Graph: createSameDegreeRandomNetwork");

    int progressCounter=0;

    makeThingsLookRandom();
    index.reserve(vert);

    for (register int i=0; i< vert ; i++) {
        int x=10+rand() %640;
        int y=10+rand() %480;
        qDebug("Graph: createUniformRandomNetwork, new node i=%i, at x=%i, y=%i", i+1, x,y);
        createVertex(
                    i+1, initVertexSize,initVertexColor,
                    initVertexNumberColor, initVertexNumberSize,
                    QString::number (i+1), initVertexLabelColor, initVertexLabelSize,
                    QPoint(x, y), initVertexShape,false
                    );
        progressCounter++;
        emit updateProgressDialog( progressCounter );

    }
    int target = 0;
    for (register int i=0;i<vert; i++){
        qDebug("Creating links for node %i = ", i+1);
        for (register int j=0; j< degree/2 ; j++) {
            target = i + j+1 ;
            if ( target > (vert-1))
                target = target-vert;
            qDebug("Creating Link between %i  and %i", i+1, target+1);
            createEdge(i+1, target+1, 1, "black", true, true, false);
        }
        progressCounter++;
        emit updateProgressDialog(progressCounter );
        qDebug("Emitting UPDATE PROGRESS %i", progressCounter);
    }
    addRelationFromGraph(tr("random"));
    emit graphChanged();
}



/**
    Calculates and returns the number of walks of a given length between v1 and v2
*/
int Graph::numberOfWalks(int v1, int v2, int length) {
    createNumberOfWalksMatrix(length);
    return XM.item(v1-1,v2-1);
}


/**
    Calculates two matrices:
    Matrix XM=AM^l where the elements denote the number of walks of length l
    between all pairs of vertices
    Matrix XSM=Sum{AM^n} where the elements denote the total number of walks of
    any length between pairs of vertices

    NOTE: This function is VERY SLOW on large networks (n>50), since it will
    calculate all powers of the sociomatrix up to n-1 in order to find out all
    possible walks. If you need to make a simple reachability test, we advise to
    use the reachabilityMatrix() function instead.
*/
void Graph::createNumberOfWalksMatrix(int length) {
    qDebug()<<"Graph::numberOfWalks() - first create the Adjacency Matrix AM";

    bool dropIsolates=false;
    bool considerWeights=true;
    bool inverseWeights=false;
    bool symmetrize=false;
    createAdjacencyMatrix(dropIsolates, considerWeights, inverseWeights, symmetrize);

    int size = vertices();
    int maxPower = length;

    XM = AM;   // XM will be the product matrix
    XSM = AM;  // XSM is the sum of product matrices
    Matrix PM; // temp matrix
    PM.zeroMatrix(size);

    qDebug()<< "Graph::writeNumberOfWalksMatrix() XM is  " ;
    for (register int i=0; i < size ; i++) {
        for (register int j=0; j < size ; j++) {
            qDebug() << XM.item(i,j) <<  " ";
        }
        qDebug()<< endl;
    }
    qDebug()<< "Graph::writeNumberOfWalksMatrix() calculating sociomatrix powers up to " << maxPower;
    for (register int i=2; i <= maxPower ; i++) {
        PM.product(XM,AM, false);
        XM=PM;
        XSM = XSM+XM;
    }

}


void Graph::writeTotalNumberOfWalksMatrix(QString fn, QString netName, int length){
    qDebug("Graph::writeTotalNumberOfWalksMatrix() ");

    QFile file (fn);

    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream outText(&file);
    outText.setCodec("UTF-8");
    outText << "-Social Network Visualizer- \n";
    outText << "Network name "<< netName<<": \n";
    outText << "Total number of walks of any length less than or equal to "<< length
        <<" between each pair of nodes \n\n";
    outText << "Warning: Walk counts consider unordered pairs of nodes\n\n";

    createNumberOfWalksMatrix(length);

    outText << XSM ;

    file.close();

}

void Graph::writeNumberOfWalksMatrix(QString fn, QString netName, int length){
    qDebug("Graph::writeNumberOfWalksMatrix() ");

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream outText(&file);
    outText.setCodec("UTF-8");
    outText << "-Social Network Visualizer- \n";
    outText << "Network name "<< netName<<": \n";
    outText << "Number of walks of length "<< length <<" between each pair of nodes \n\n";

    createNumberOfWalksMatrix(length);

    outText << XM ;

    file.close();
}




/**
    Calculates and returns non-zero if vertices v1 and v2 are reachable.
    If v1, v2 are reachable it returns the geodesic distance.
    This method is actually a reachability test (if it returns non-zero)

*/
int Graph::reachable(int v1, int v2) {
    qDebug()<< "Graph::reachable()";
    if (!distanceMatrixCreated || graphModified )
        createDistanceMatrix(false);
    return DM.item(v1-1,v2-1);
}




/**
 *  Returns the influence range of vertex v1, namely the set of nodes who are
 *  reachable by v1 (See Wasserman and Faust, pp.200-201, based on Lin, 1976).
 *  This function is for digraphs only
 */
QList<int> Graph::influenceRange(int v1){
    qDebug() << "Graph::influenceRange() ";
    if (!reachabilityMatrixCreated || graphModified) {
        // call reachabilityMatrix to construct a list of influence ranges
        // for each node
        reachabilityMatrix();
    }
    return influenceRanges.values(v1);
}



/**
 *  Returns the influence domain of vertex v1, namely the set of nodes who can
 *  reach v1
 *  This function is for digraphs only
 */
QList<int> Graph::influenceDomain(int v1){
    qDebug() << "Graph::influenceDomain() ";
    if (!reachabilityMatrixCreated || graphModified) {
        // call reachabilityMatrix to construct a list of influence domains
        // for each node
        reachabilityMatrix();
    }
    return influenceDomains.values(v1);
}



/**
    Calculates the reachability matrix X^R of the graph
    where the {i,j} element is 1 if the vertices i and j are reachable
    Actually, this just checks the corresponding element of Distance Matrix,
    If d(i,j) is non-zero, then the two nodes are reachable

    In the process, this function creates the InfluenceRange and InfluenceDomain
    of each node.
*/
void Graph::reachabilityMatrix( const bool considerWeights,
                                const bool inverseWeights,
                                const bool dropIsolates) {
    qDebug()<< "Graph::reachabilityMatrix()";

    if (reachabilityMatrixCreated && !graphModified) {
        qDebug()<< "Graph::reachabilityMatrix() - "
                   "XRM calculated and graph unmodified. Returning..." ;
        return;
    }
    else {

        createDistanceMatrix(false, considerWeights,inverseWeights,dropIsolates);
        int size = vertices(false,false), i=0, j=0;
        qDebug()<< "Graph::reachabilityMatrix() - calculating XRM..." ;
        influenceRanges.clear();
        influenceDomains.clear();
        disconnectedVertices.clear();
        bool isolateVertex;
        isolateVertex=true;
        for (i=0; i < size ; i++) {
            for (j=i+1; j < size ; j++) {
                if ( XRM.item(i,j) ==1 ) {
                    qDebug()<< "Graph::reachabilityMatrix() - d("<<i+1<<","
                            <<j+1<<")=" << DM.item(i,j)
                           << " - inserting " << j+1
                           << " to inflRange J of " << i+1
                           << " - and " << i+1
                           << " to inflDomain I of "<< j+1 ;
                    influenceRanges.insertMulti(i,j);
                    influenceDomains.insertMulti(j,i);
                    isolateVertex=false;
                    if ( XRM.item(j,i) ==1 ) {
                        qDebug()<< "Graph::reachabilityMatrix() - inverse path d("
                                <<j+1<<","<<i+1<<")="
                               << DM.item(j,i)
                               << " - inserting " << j+1 << " to influenceDomain I of " << i+1
                               << " - and " << i+1 << " to influenceRange J of " << j+1 ;
                        influenceDomains.insertMulti(i,j);
                        influenceRanges.insertMulti(j,i);
                    }
                    else {
                        qDebug()<< "Graph::reachabilityMatrix() - ("
                                <<i+1<<","<<j+1<<") unilaterallyConnectedVertices";
                        unilaterallyConnectedVertices.insertMulti(i,j);
                    }
                }
                else {
                    if ( XRM.item(j,i) == 0 ) {
                        qDebug()<< "Graph::reachabilityMatrix() - ("
                                <<j+1<<","<<i+1<<") disConnectedVertices";
                        disconnectedVertices.insertMulti(i,j);
                    }
                    else {
                        qDebug()<< "Graph::reachabilityMatrix() - ("
                                <<j+1<<","<<i+1<<") unilaterallyConnectedVertices";
                        unilaterallyConnectedVertices.insertMulti(j,i);
                    }
                }
            }
        }

        reachabilityMatrixCreated=true;
    }
}


/**
    Writes the reachability matrix X^R of the graph to a file
*/
void Graph::writeReachabilityMatrix(QString fn, QString netName,
                                   const bool dropIsolates) {
    qDebug("Graph::writeReachabilityMatrix() ");

    QFile file (fn);

    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream outText(&file);

    outText << "-Social Network Visualizer- \n";
    outText << "Network name: "<< netName<<" \n";
    outText << "Reachability Matrix (XR) \n";
    outText << "Two nodes are reachable if there is a walk between them (their geodesic distance is non-zero). \n";
    outText << "If nodes i and j are reachable then XR(i,j)=1 otherwise XR(i,j)=0.\n\n";

    if (!reachabilityMatrixCreated || graphModified) {
        reachabilityMatrix(false, false, dropIsolates);
    }

    outText << XRM ;

    file.close();
}




/**
*	Writes the number of cliques (triangles) of each vertex into a given file.
*/
void Graph::writeCliqueCensus(
        const QString fileName, const bool considerWeights)
{
    qDebug()<< "Graph::writeCliqueCensus() ";
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }

    long int N = vertices();

    cliques_2_Vertex.clear();
    cliques_3_Vertex.clear();
    cliques_4_Vertex.clear();
    QList<Vertex*>::const_iterator it;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
    {
        (*it)->clearCliques();
    }

    QTextStream outText ( &file ); outText.setCodec("UTF-8");

    emit statusMessage ( QString(tr("Writing clique census to file: ")).arg(fileName) );

    outText << tr("CLIQUE CENSUS (CLQs)") << endl<<endl;

    outText << tr("CLIQUE COUNTS BY VERTEX") << endl;
    outText << tr("Node")<<"\t"<< tr("2-Vertex") << "\t" << tr("3-Vertex")
            << "\t" << tr("4-Vertex") << endl;


    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        countCliquesWith ((*it)->name());
        outText << (*it)->name()<<"\t"<< (*it)->cliques(2)
                << "\t" <<  (*it)->cliques(3)
                << "\t"<<  (*it)->cliques(4)  <<endl;

    }

    outText << endl<< endl << tr("AGGREGATE COUNTS OF CLIQUES")<< endl;

    outText << "2-Vertex cliques" <<"\t " << cliques_2_Vertex.count()
            << "\t (max: " << ( N * (N-1)  ) /2  << ")\n";
    outText << "3-Vertex cliques" <<"\t " << cliques_3_Vertex.count()
            << "\t (max: " << ( N * (N-1) * (N-2)  ) /6  << ")\n";
    outText << "4-Vertex cliques" <<"\t " << cliques_4_Vertex.count()
            << "\t (max: " << ( N * (N-1) * (N-2) * (N-3)  ) /24  << ")\n";


    outText << endl<< endl << tr("CLIQUE ENUMERATION BY THEIR SIZE")<< endl;
    QHash<QString, bool>::const_iterator i;

    outText << endl << tr("2-Vertex cliques") << endl ;

    for (i = cliques_2_Vertex.constBegin(); i != cliques_2_Vertex.constEnd(); ++i) {
        outText << i.key() << endl;
    }


    outText << endl << tr("3-Vertex cliques") << endl ;

    for (i = cliques_3_Vertex.constBegin(); i != cliques_3_Vertex.constEnd(); ++i) {
        outText << i.key() << endl;
    }

    outText << endl << tr("4-Vertex cliques") << endl ;


    for (i = cliques_4_Vertex.constBegin(); i != cliques_4_Vertex.constEnd(); ++i) {
        outText << i.key() << endl;

    }

    outText <<"\n\n" ;
    outText << tr("Clique Census Report,\n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime().
               toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

    file.close();
}


bool Graph:: addClique(const QList<int> &list){
    qDebug() << "*** Graph::addClique()" <<
                list.count();
    for (int i = 0; i < list.size(); ++i) {
            qDebug() << "*** Graph::addClique() -  Found vertex " << list.at(i)
                 << " at position " << i << endl;
    }

    QString dyad, dyad_alt;
    QString triad, triad_alt1, triad_alt2, triad_alt3,triad_alt4,triad_alt5;
    QString quart, quart_alt1, quart_alt2, quart_alt3,quart_alt4,quart_alt5;
    bool knownClique=false;

    if (list.size() == 2 )
    {
        dyad = QString::number(list.at(0)) + ", " + QString::number(list.at(1));
        dyad_alt = QString::number(list.at(1)) + ", " + QString::number(list.at(0));


        if ( ! cliques_2_Vertex.contains(dyad) &&
             ! cliques_2_Vertex.contains(dyad_alt) )
        {
            cliques_2_Vertex.insert(dyad, true);
            qDebug() << "*** Graph::addClique() - new 2-vertex clique "
                        << " adding it to global list ";

        }

        if ( m_graph[ index[list.at(0)] ]->addClique(dyad,list.size()) ) {
            qDebug() << "*** Graph::addClique() - new 2-vertex clique: "
                        << list.at(0) << "," << list.at(1) ;
            return true;
        }
        return false;

    }
    else if (list.size() == 3 )
    {
        triad = QString::number(list.at(0)) + ", " + QString::number(list.at(1))
                + ", " + QString::number(list.at(2));
        triad_alt1 = QString::number(list.at(0)) + ", " + QString::number(list.at(2))
                + ", " + QString::number(list.at(1));
        triad_alt2 = QString::number(list.at(1)) + ", " + QString::number(list.at(2))
                + ", " + QString::number(list.at(0));
        triad_alt3 = QString::number(list.at(1)) + ", " + QString::number(list.at(0))
                + ", " + QString::number(list.at(2));
        triad_alt4 = QString::number(list.at(2)) + ", " + QString::number(list.at(0))
                + ", " + QString::number(list.at(1));
        triad_alt5 = QString::number(list.at(2)) + ", " + QString::number(list.at(1))
                + ", " + QString::number(list.at(0));
        if ( ! cliques_3_Vertex.contains(triad) &&
             ! cliques_3_Vertex.contains(triad_alt1) &&
             ! cliques_3_Vertex.contains(triad_alt2) &&
             ! cliques_3_Vertex.contains(triad_alt3) &&
             ! cliques_3_Vertex.contains(triad_alt4) &&
             ! cliques_3_Vertex.contains(triad_alt5)  )
        {
            cliques_3_Vertex.insert(triad, true);
            qDebug() << "*** Graph::addClique() - new 3-vertex clique "
                        << " adding it to global list ";
        }
        if ( m_graph[ index[list.at(0)] ]->addClique(triad,list.size()) ) {
            qDebug() << "*** Graph::addClique() - new 3-vertex clique: "
                     << list.at(0) << "," << list.at(1) << "," << list.at(2) ;
            return true;
        }
        return false;


    }
    else if (list.size() == 4 )
     {
        for (int i = 0; i < list.size(); ++i)
        {
            quart = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+1) % 4 ) )
                    + ", " + QString::number(list.at( (i+2) % 4 ) )
                    + ", " + QString::number(list.at( (i+3) % 4 ) );
            quart_alt1 = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+1) % 4 ) )
                    + ", " + QString::number(list.at( (i+3) % 4 ) )
                    + ", " + QString::number(list.at( (i+2) % 4 ) );
            quart_alt2 = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+2) % 4 ) )
                    + ", " + QString::number(list.at( (i+3) % 4 ) )
                    + ", " + QString::number(list.at( (i+1) % 4 ) );
            quart_alt3 = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+2) % 4 ) )
                    + ", " + QString::number(list.at( (i+1) % 4 ) )
                    + ", " + QString::number(list.at( (i+3) % 4 ) );
            quart_alt4 = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+3) % 4 ) )
                    + ", " + QString::number(list.at( (i+1) % 4 ) )
                    + ", " + QString::number(list.at( (i+2) % 4 ) );
            quart_alt5 = QString::number(list.at( (i)%4 ))
                    + ", " + QString::number(list.at( (i+3) % 4 ) )
                    + ", " + QString::number(list.at( (i+2) % 4 ) )
                    + ", " + QString::number(list.at( (i+1) % 4 ) );
            qDebug() << " checking other possible combinations: ";
            qDebug() << quart;
            qDebug() << quart_alt1;
            qDebug() << quart_alt2;
            qDebug() << quart_alt3;
            qDebug() << quart_alt4;
            qDebug() << quart_alt5;
            if (  cliques_4_Vertex.contains(quart) ||
                  cliques_4_Vertex.contains(quart_alt1) ||
                  cliques_4_Vertex.contains(quart_alt2) ||
                  cliques_4_Vertex.contains(quart_alt3) ||
                  cliques_4_Vertex.contains(quart_alt4) ||
                  cliques_4_Vertex.contains(quart_alt5)  )
            {
                knownClique = true;
            }
        }
        quart = QString::number(list.at( (0) ))
                + ", " + QString::number(list.at( (1)  ) )
                + ", " + QString::number(list.at( (2)  ) )
                + ", " + QString::number(list.at( (3)  ) );
        if (! knownClique) {
            cliques_4_Vertex.insert(quart, true);
            qDebug() << "*** Graph::addClique() - new 4-vertex clique "
                        << quart
                    << " adding it to global list ";
        }

        if ( m_graph[ index[list.at(0)] ]->addClique(quart,list.size()) ) {
            qDebug() << "*** Graph::addClique() - new 4-vertex clique: "
                     << list.at(0) << "," << list.at(1)
                     << "," << list.at(2) << "," << list.at(3) ;
            return true;
        }
        return false;

    }
    return false;
}

/**
    Calculates and returns the number of cliques which include given vertex 'source'
    A clique is a complete subgraph of N vertices.
    All N vertices must be mutually adjacenct.
    Due to computational complexity, SocNetV computes 2-vertex,
    3-vertex and 4-vertex cliques only.
*/	
float Graph:: countCliquesWith(int source, int size){
    qDebug() << "*** Graph::countCliquesWith(" <<  source << ")";

    int  vert1=0, vert2=0, vert3=0;
    int relation=0;

    bool edgeStatus=false;
    H_edges::const_iterator it1, it2, it3;

    QList<int> dyad, triad, quad;

    qDebug() << "Graph::countCliquesWith() Source vertex " << source
             << "[" << index[source] << "] has inEdges " << inboundEdges(source)
             << " and outEdges "<< outboundEdges(source);


    qDebug () << "Graph::countCliquesWith() - Checking inEdges to " << source;

    it1=m_graph [ index[source] ] ->m_inEdges.cbegin();

    while ( it1!=m_graph [ index[source] ] -> m_inEdges.cend() ){
        relation = it1.value().first;
        if ( relation != currentRelation() ) {
            ++it1;
            continue;
        }
        edgeStatus=it1.value().second.second;
        if ( edgeStatus != true) {
            ++it1;
            continue;
        }
        vert1 = it1.key();
        //            weight = it1.value().second.first;
        qDebug() << "Graph::countCliquesWith() - inLink from 1st neighbor "
                 << vert1
                 << "[" << index[vert1] << "] ";

        if (source == vert1) {
            qDebug() << "Graph::countCliquesWith() -     It's the source - CONTINUE";
            ++it1;
            continue;
        }


        if (  this->hasArc( source, vert1 )  == 0 )  {
            qDebug() << "Graph::countCliquesWith() - incomplete 2v-subgraph - CONTINUE";
            ++it1;
            continue;

        }
        qDebug() << "Graph::countCliquesWith() - complete 2v-subgraph ";

        dyad.clear();
        dyad << source << vert1;
        if ( addClique( dyad ) ) {
            qDebug() << "Graph::countCliquesWith() - 2v cliques "
                     << cliques_2_Vertex.count();
        }

        qDebug() << "Graph::countCliquesWith() - "
                 << " Iterate over all inEdges of " << vert1;

        it2=m_graph [ index[vert1] ] ->m_inEdges.cbegin();
        while ( it2!=m_graph [ index[vert1] ] -> m_inEdges.cend() ){

            relation = it2.value().first;
            if ( relation != currentRelation() ){
                ++it2;
                continue;
            }
            edgeStatus=it2.value().second.second;
            if ( edgeStatus != true){
                ++it2;
                continue;
            }
            vert2 = it2.key();
            qDebug() << "Graph::countCliquesWith() -     Possible other neighbor (for 3v clique)"
                     << vert2 << "[" << index[vert2] << "]";
            if (source == vert2) {
                qDebug() << "Graph::countCliquesWith() -     It's the source - CONTINUE";
                ++it2;
                continue;
            }
            if (vert1 == vert2) {
                qDebug() << "Graph::countCliquesWith() -     It's the vert1 - CONTINUE";
                ++it2;
                continue;
            }

            if (  this->hasArc( vert1, vert2 ) == 0 )  {
                qDebug() << "Graph::countCliquesWith() -     "
                            <<  vert1 << "  not outLinked to  " << vert2
                               << " - incomplete 3vertex-subgraph - CONTINUE";
                ++it2;
                continue;
            }
            else {
                qDebug() << "Graph::countCliquesWith() -     complete 3vertex-subgraph ? "
                         << vert2 << " <-> " << vert1
                            << ". Checking if "
                            << vert2 << " <-> " << source << " ... ";

                if ( this->hasEdge( source, vert2 ) ) {

                    qDebug() << "Graph::countCliquesWith() -     complete 3v-subgraph "
                             << source << " <-> " << vert2
                             << " possible (new?) 3-vertex clique: ";

                    triad.clear();
                    triad << source << vert1 << vert2;
                    if ( addClique( triad ) ) {
                        qDebug() << "Graph::countCliquesWith() -     3-vertex cliques "
                                 << cliques_3_Vertex.count();
                    }

                    qDebug() << "Graph::countCliquesWith() -         "
                                << " Iterate over all inEdges of " << vert2;
                    it3=m_graph [ index[vert2] ] ->m_inEdges.cbegin();
                    while ( it3!=m_graph [ index[vert2] ] -> m_inEdges.cend() ){
                        relation = it3.value().first;
                        if ( relation != currentRelation() ){
                            ++it3;
                            continue;
                        }
                        edgeStatus=it3.value().second.second;
                        if ( edgeStatus != true){
                            ++it3;
                            continue;
                        }
                        vert3 = it3.key();
                        qDebug() << "Graph::countCliquesWith() -     Possible other neighbor (for 4v clique)"
                                 << vert3 << "[" << index[vert3] << "]";
                        if (source == vert3 || vert1 == vert3  || vert2 == vert3 ) {
                            qDebug() << "Graph::countCliquesWith() -     same as source, vert1 or vert2- CONTINUE";
                            ++it3;
                            continue;
                        }
                        if (  this->hasEdge( source, vert3  ) == 0  ||
                              this->hasEdge( vert1,  vert3  ) == 0  ||
                              this->hasArc ( vert2,  vert3  ) == 0 )  {
                            qDebug() << "Graph::countCliquesWith() -     incomplete 4v-subgraph - CONTINUE";
                            ++it3;
                            continue;
                        }
                        quad.clear();
                        quad << source << vert1 << vert2<< vert3;
                        qDebug() << "Graph::countCliquesWith() -     complete 4v-subgraph "
                                 << source << "," << vert1 << "," << vert2 << "," << vert3
                                 << " possible (new?) 3-vertex clique: ";
                        if ( addClique( quad ) ) {
                            qDebug() << "Graph::countCliquesWith() -     4-vertex cliques "
                                     << cliques_4_Vertex.count();
                        }
                        ++it3;
                    }



                }
                else {
                    qDebug() << "Graph::countCliquesWith() -     Not mutual - CONTINUE";
                }
            }
            ++it2;
        }
        ++it1;
    } // end 1st while

    switch (size) {
    case 2: {
        return m_graph [ index[source] ] -> cliques(2);
        break;
    }
    case 3: {
        return m_graph [ index[source] ] -> cliques(3);
        break;
    }
    case 4: {
        return m_graph [ index[source] ] -> cliques(4);
        break;
    }
    default:
        return 0;
        break;
    };

    return 0;
}


/**
    Calculates and returns the total number of cliques in the graph.
    Calls countCliquesWith(v1) to calculate the number of cliques of each vertex v1,
    sums the total number, then divides it by 3 because each vertex has been counted three times.
*/	
float Graph::countCliquesOfSize(int size){
    qDebug("Graph::countCliquesOfSize()");
    float cliques=0;

    QList<Vertex*>::const_iterator v1;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
    {
        cliques += countCliquesWith( (*v1) -> name(), size );
    }
    cliques = cliques / size;

    //actually we can just return cliques_*_Vertex.count();

    qDebug() << "Graph::countCliquesOfSize - Dividing by size we get "<< cliques ;

    return cliques ;
}



/**
    Returns the number of triples of vertex v1
    A triple Υ at a vertex v is a path of length two for which v is the center vertex.
*/
float Graph::numberOfTriples(int v1){
    float totalDegree=0;
    if (isSymmetric()){
        totalDegree=outboundEdges(v1);
        return totalDegree * (totalDegree -1.0) / 2.0;
    }
    totalDegree=outboundEdges(v1) + inboundEdges(v1);  //FIXEM
    return	totalDegree * (totalDegree -1.0);
}



/**
    Returns the local clustering coefficient (CLUCOF) of a vertex v1
    CLUCOF in a graph quantifies how close the vertex and its neighbors are
    to being a clique, a connected subgraph.
    This is used to determine whether a graph is a small-world network.
*/
float Graph:: localClusteringCoefficient(const long int &v1){
    if ( !graphModified && (m_graph[ index [v1] ] -> hasCLC() ) )  {
        float clucof=m_graph[ index [v1] ] ->CLC();
        qDebug() << "Graph::localClusteringCoefficient("<< v1 << ") - "
                 << " Not modified. Returning previous clucof = " << clucof;
        return clucof;
    }

    qDebug() << "Graph::localClusteringCoefficient("<< v1 << ") - "
            << " Graph changed or clucof not calculated.";

    bool graphSymmetric = false;

    if ( isSymmetric() ) {
        graphSymmetric = true;
    }
    else {
        graphSymmetric = false;
    }

    float clucof=0, denom = 0 , nom = 0;
    int u1 = 0 , u2 = 0, k = 0;

    H_StrToBool neighborhoodEdges;
    neighborhoodEdges.clear();

    qDebug() << "Graph::localClusteringCoefficient() - vertex " << v1
             << "[" << index[v1] << "] ";


    qDebug () << "Graph::localClusteringCoefficient() - "
              << " Checking edges adjacent to " << v1;

    QHash<int,float> *reciprocalEdges = new QHash<int,float>;
    reciprocalEdges = m_graph [ index[v1] ] -> returnReciprocalEdges();

    QHash<int,float>::const_iterator it1;
    QHash<int,float>::const_iterator it2;

    it1=reciprocalEdges->cbegin();

    while ( it1 != reciprocalEdges->cend() )
    {
        u1 = it1.key();

        qDebug() << "Graph::localClusteringCoefficient() - "
                 << " edge with neighbor "
                 << u1
                 << " [" << index[u1] << "] "
                 << " weight " << it1.value();

        if ( v1 == u1 ) {
            qDebug() << "Graph::localClusteringCoefficient() - "
                     << " v1 == u1 - CONTINUE";
            ++it1;
            continue;
        }

        it2=reciprocalEdges->cbegin();

        while ( it2 != reciprocalEdges->cend() ){

            u2 = it2.key();

            qDebug() << "Graph::localClusteringCoefficient() - "
                     << " cross-checking edge with neighbor "
                     << u2
                     << " [" << index[u2] << "] "
                     << " weight " << it2.value();

            if ( u1 == u2 ) {
                qDebug() << "Graph::localClusteringCoefficient() - "
                         << " u1 == u2 - CONTINUE";
                ++it2;
                continue;
            }

            if ( hasArc( u1, u2 ) != 0 )
            {
                qDebug() << "Graph::localClusteringCoefficient() - "
                         << " connected neighbors: "
                         << u1 << " -> " << u2;

                QString edge = QString::number(u1) + "->" + QString::number(u2);
                QString revedge = QString::number(u2) + "->" + QString::number(u1);

                if ( ! neighborhoodEdges.contains(edge) &&
                     ( graphSymmetric && ! neighborhoodEdges.contains(revedge) )
                     )
                {
                    neighborhoodEdges.insert(edge, true);
                    qDebug() << "Graph::localClusteringCoefficient() - "
                             << " adding edge to neighborhoodEdges ";

                }
                else {
                    qDebug() << "Graph::localClusteringCoefficient() - "
                             << " edge discovered previously... ";
                }
            }
            if ( ! graphSymmetric )
            {
                if (  hasArc( u2, u1 ) != 0   )
                {
                    qDebug() << "Graph::localClusteringCoefficient() - "
                             << " graph not symmetric  "
                             << " connected neighbors: "
                             << u2 << " -> " << u1;

                    QString edge = QString::number(u2) + "->" + QString::number(u1);

                    if ( ! neighborhoodEdges.contains(edge) )
                    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::localClusteringCoefficient() - "
                                 << " adding edge to neighborhoodEdges ";

                    }
                    else {
                        qDebug() << "Graph::localClusteringCoefficient() - "
                                 << " edge discovered previously... ";
                    }
                }

            }
            ++it2;
        }
        ++it1;
    }

    nom=neighborhoodEdges.count();

    qDebug() << "Graph::localClusteringCoefficient("<< v1 << ") - "
            << " actual edges in neighborhood " <<  nom;

    if ( nom == 0)
        return 0;	//stop if we're at a leaf.

    if ( graphSymmetric ){
        k=reciprocalEdges->count();
        denom =	k * (k -1.0) / 2.0;

        qDebug() << "Graph::localClusteringCoefficient("<< v1 << ") - "
                    << " symmetric graph. "
                    << " max edges in neighborhood" << denom ;

    }
    else {
        // fixme : normally we should have a special method
        // to compute the number of vertices k_i = |N_i|, in the neighborhood N_i
        k=reciprocalEdges->count();
        denom = k * (k -1.0);

        qDebug() << "Graph::localClusteringCoefficient("<< v1 << ") - "
                    << " not symmetric graph. "
                    << " max edges in neighborhood" << denom ;
    }

    clucof = nom / denom;

    qDebug() << "=== Graph::localClusteringCoefficient("<< v1 << ") - "
             << " CLUCOF = "<< clucof;

    m_graph[ index [v1] ] -> setCLC(clucof);

    neighborhoodEdges.clear();
    return clucof;
}


/**
    Calculates local clustering coefficients
    and returns the network average Clustering Coefficient
*/
float Graph::clusteringCoefficient (){
    qDebug("=== Graph::clusteringCoefficient()  ");
    averageCLC=0;
    maxCLC=0; minCLC=1;
    float temp=0;
    QList<Vertex*>::const_iterator vertex;
    for ( vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex)
    {
        temp = localClusteringCoefficient( (*vertex)->name() );
        if (temp > maxCLC)  {
            maxCLC = temp;
            maxNodeCLC = (*vertex)->name();
        }
        if ( temp < minCLC ) {
            minNodeCLC = (*vertex)->name();
            minCLC= temp;
        }
        averageCLC += temp;
    }

    averageCLC = averageCLC / vertices();
    qDebug() << "Graph::clusteringCoefficient() network average " << averageCLC;

    return averageCLC;
}



/*
 *  Conducts a triad census and updates QList::triadTypeFreqs,
 * 		which is the list carrying all triad type frequencies
 *  Complexity:O(n!)
 */
bool Graph::triadCensus(){
    int mut=0, asy=0, nul =0;
    int temp_mut=0, temp_asy=0, temp_nul =0, counter_021=0;
    int ver1, ver2, ver3;
    int progressCounter = 0;

    qDebug() << "Graph::triadCensus()";
    /*
     * QList::triadTypeFreqs stores triad type frequencies with the following order:
     * 0	1	2	3		4	5	6	7	8		9	10	11	12		13	14	15
     * 003 012 102	021D 021U 021C 111D	111U 030T 030C 201 	120D 120U 120C 210 300
    */

    for (int i = 0; i <= 15; ++i) {
        triadTypeFreqs.append(0);
        qDebug() << " initializing triadTypeFreqs[" << i << "] = "<< triadTypeFreqs[i];
    }
    QList<Vertex*>::const_iterator v1;
    QList<Vertex*>::const_iterator v2;
    QList<Vertex*>::const_iterator v3;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); v1++) {

        for (v2=(v1+1); v2!=m_graph.cend(); v2++) {

            ver1=(*v1)->name();
            ver2=(*v2)->name();

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

                ver3=(*v3)->name();

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
                examine_MAN_label(mut, asy, nul, (*v1), (*v2),  (*v3) ) ;
                progressCounter++ ;
                emit updateProgressDialog( progressCounter );
                if ( mut==3 && asy==0 && nul==0 ){
                    counter_021++;
                }
            } // end 3rd for
        }// end 2rd for
    }// end 1rd for
    qDebug() << " ****** 003 COUNTER: "<< counter_021;

    calculatedTriad=true;
    return true;
}




/** 
    Examines the triad type (in Mutual-Asymmetric-Null label format)
    and increases by one the proper frequency element
    inside QList::triadTypeFreqs
*/
void Graph::examine_MAN_label(int mut, int asy, int nul,
                               Vertex* vert1,
                               Vertex* vert2,
                               Vertex* vert3
                               ) 	{
    QList<Vertex*> m_triad;
    bool isDown=false, isUp=false, isCycle=false, isTrans=false;
    bool isOutLinked=false, isInLinked=false;

    qDebug () << "Graph::examine_MAN_label() "
        << " adding ("<< vert1->name() << ","<< vert2->name()
        << ","<< vert3->name() << ") to m_triad ";

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
            //	qDebug() << "triad vertices: ( "<< vert1->name() << ", "<< vert2->name()<< ", "<< vert3->name()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (Vertex *source, m_triad)  {
                //qDebug() << "  Vertex " << source->name() ;
                isOutLinked=false; isInLinked=false;

                foreach (Vertex *target, m_triad)  	{
                    if ( source->name() == target->name() )
                        continue;

                    if ( source->hasEdgeTo(target->name()) ){
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
                    else if( target->hasEdgeTo(source->name()) ){
                        //	qDebug() << "    Vertex " << source->name()  << " is IN linked from " <<target->name();
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
            qDebug() << "triad vertices: ( "<< vert1->name() << ", "<< vert2->name()<< ", "<< vert3->name()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            isTrans=false;
            foreach (Vertex *source, m_triad)  {
                qDebug() << "  Vertex " << source->name() ;

                isOutLinked=false;

                foreach (Vertex *target, m_triad)  	{
                    if ( source->name() == target->name() )
                        continue;

                    if ( source->hasEdgeTo(target->name()) ){

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
            isDown=false; isUp=false;
            //qDebug() << "triad vertices: ( "<< vert1->name() << ", "<< vert2->name()<< ", "<< vert3->name()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (Vertex *source, m_triad)  {
                //	qDebug() << "  Vertex " << source->name() ;

                isInLinked=false;

                foreach (Vertex *target, m_triad)  	{
                    if ( source->name() == target->name() )
                        continue;

                    if ( target->hasEdgeTo(source->name()) ){

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
            qDebug() << "triad vertices: ( "<< vert1->name() << ", "
                     << vert2->name()<< ", "<< vert3->name()<< " ) = ("
                     <<mut<<","<< asy<<","<<nul<<")";

            foreach (Vertex *source, m_triad)  {
                //qDebug() << "  Vertex " << source->name() ;
                isOutLinked=false; isInLinked=false;

                foreach (Vertex *target, m_triad)  	{
                    if ( source->name() == target->name() )
                        continue;

                    if ( source->hasEdgeTo(target->name()) ){
                        if (target->hasEdgeTo(source->name() ) ){
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
                    else if( target->hasEdgeTo(source->name()) ){
                        //	qDebug() << "    Vertex " << source->name()  << " is IN linked from " <<target->name();
                        if (source->hasEdgeTo(target->name())){
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
    Our almost universal network loader. :)
    Actually it calls the load() method of parser/qthread class.
*/
bool Graph::loadGraph (	const QString m_fileName,
                        const QString m_codecName,
                        const bool m_showLabels,
                        const int maxWidth, const int maxHeight,
                        const int fileFormat, const int two_sm_mode){
    initShowLabels = m_showLabels;
    qDebug() << "Graph::loadGraph() : "<< m_fileName
                << " calling parser.load() from thread " << this->thread();

    Parser *file_parser = new Parser(
                m_fileName,
                m_codecName,
                initVertexSize, initVertexColor,
                initVertexShape,
                initVertexNumberColor, initVertexNumberSize,
                initVertexLabelColor, initVertexLabelSize,
                initEdgeColor,
                maxWidth, maxHeight,
                fileFormat,
                two_sm_mode
                );

    qDebug () << "Graph::loadGraph() file_parser thread  " << file_parser->thread()
                 << " moving it to new thread ";

    file_parser->moveToThread(&file_parserThread);

    qDebug () << "Graph::loadGraph() file_parser thread now " << file_parser->thread();

    qDebug () << "Graph::loadGraph()  connecting file_parser signals ";

    connect(&file_parserThread, &QThread::finished,
            file_parser, &QObject::deleteLater);

    connect (
                file_parser, SIGNAL( addRelation (QString) ),
                this, SLOT(addRelationFromParser(QString) )
                ) ;

    connect (
                file_parser, SIGNAL( changeRelation (int) ),
                this, SLOT( changeRelation (int) )
                ) ;


    connect (
                file_parser, SIGNAL( createNode (int,int,QString, QString, int, QString, QString, int, QPointF, QString, bool) ),
                this, SLOT(createVertex(int,int,QString, QString, int, QString, QString, int, QPointF, QString, bool) )
                ) ;

    connect (
                file_parser, SIGNAL(createEdge (int, int, float, QString, int, bool, bool)),
                this, SLOT(createEdge (int, int, float, QString, int, bool, bool) )
                );

    connect (
                file_parser, SIGNAL(fileType(int, QString, int, int, bool)),
                this, SLOT(setFileType(int, QString, int, int, bool))
                );

    connect (
                file_parser, SIGNAL(removeDummyNode(int)),
                this, SLOT (removeDummyNode(int))
                );

    connect (
                file_parser, &Parser::finished,
                this, &Graph::terminateParserThreads
                );

    qDebug() << "Graph::loadGraph()  Starting file_parserThread ";

    file_parserThread.start();

    bool loadGraphStatus = file_parser->run();
    qDebug() << "Graph::loadGraph() : loadGraphStatus "<< loadGraphStatus;
    return loadGraphStatus;
}


void Graph::terminateParserThreads(QString reason) {
    qDebug() << "Graph::terminateParserThreads() - reason " << reason
                    <<" is file_parserThread running? ";
    if (file_parserThread.isRunning() ) {
         qDebug() << "Graph::terminateParserThreads()  file_parserThread quit";
        file_parserThread.quit();
        qDebug() << "Graph::terminateCrawlerThreads() - deleting file_parser pointer";
        delete file_parser;
        file_parser = 0;  // see why here: https://goo.gl/tQxpGA

    }

}

/**
    Our almost universal graph saver. :)
    Actually it just checks the requested file type and
    calls the right saveGraphTo...() method
*/
bool Graph::saveGraph ( 
        QString fileName, int fileType,
        QString networkName, int maxWidth, int maxHeight )
{
    qDebug() << "Graph::saveGraph to ...";
    switch (fileType) {
    case 1 : {			//Pajek
        qDebug() << " 	... Pajek formatted file";
        return saveGraphToPajekFormat(fileName, networkName, maxWidth, maxHeight);
        break;
    }
    case 2: {			// Adjacency
        qDebug() << " 	... Adjacency formatted file";
        return saveGraphToAdjacencyFormat(fileName);
        break;
    }
    case 3: {			// Dot
        qDebug() << " 	... Dot formatted file";
        return saveGraphToDotFormat(fileName, networkName, maxWidth, maxHeight);
        break;
    }
    case 4: {			// GraphML
        qDebug() << " 	... GraphML formatted file";
        return saveGraphToGraphMLFormat(fileName, networkName, maxWidth, maxHeight);
        break;
    }
    default: {
        qDebug() << " 	... Error! What format number is this anyway?";
        break;
    }
    };
    graphModified=false;
    return true;
}




/**
    Saves the active graph to a Pajek-formatted file
    Preserves node properties (positions, colours, etc)
*/
bool Graph::saveGraphToPajekFormat (
        QString fileName, QString networkName, int maxWidth, int maxHeight)
{
    qDebug () << " Graph::saveGraphToPajekFormat to file: " << fileName.toUtf8();

    int weight=0;

    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly ) )  {
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName));
        return false;
    }
    QTextStream t( &f );
    t.setCodec("UTF-8");
    t<<"*Network "<<networkName<<"\n";

    t<<"*Vertices "<< vertices() <<"\n";
    QList<Vertex*>::const_iterator it;
    QList<Vertex*>::const_iterator jt;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        qDebug()<<" Name x "<<  (*it)->name()  ;
        t<<(*it)->name()  <<" "<<"\""<<(*it)->label()<<"\"" ;
        t << " ic ";
        t<<  (*it)->colorToPajek();
        qDebug()<<" Coordinates x " << (*it)->x()<< " "<<maxWidth<<" y " << (*it)->y()<< " "<<maxHeight;
        t << "\t\t" <<(*it)->x()/(maxWidth)<<" \t"<<(*it)->y()/(maxHeight);
        t << "\t"<<(*it)->shape();
        t<<"\n";
    }

    t<<"*Arcs \n";
    qDebug()<< "Graph::saveGraphToPajekFormat: Arcs";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveGraphToPajekFormat:  it=" << (*it)->name() << ", jt=" << (*jt)->name() ;
            if  ( (weight=this->hasArc( (*it)->name(), (*jt)->name())) !=0
                  &&   ( this->hasArc((*jt)->name(), (*it)->name())) == 0
                  )
            {
                qDebug()<<"Graph::saveGraphToPajekFormat  weight "<< weight << " color "<<  (*it)->outLinkColor( (*jt)->name() ) ;
                t << (*it)->name() <<" "<<(*jt)->name()<< " "<<weight;
                //FIXME bug in outLinkColor() when we remove then add many nodes from the end
                t<< " c "<< (*it)->outLinkColor( (*jt)->name() );
                t <<"\n";
            }

        }
    }

    t<<"*Edges \n";
    qDebug() << "Graph::saveGraphToPajekFormat: Edges";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveGraphToPajekFormat:  it=" <<  (*it)->name() << ", jt=" <<(*jt)->name() ;
            if  ( (weight=this->hasArc((*it)->name(), (*jt)->name()))!=0   &&
                  (this->hasArc((*jt)->name(), (*it)->name()))!=0
                  )  {
                if ( (*it)->name() > (*jt)->name() )
                    continue;
                t << (*it)->name() <<" "<<(*jt)->name()<< " "<<weight;
                t << " c "<< (*it)->outLinkColor( (*jt)->name() );
                t <<"\n";
            }
        }
    }
    f.close();
    QString fileNameNoPath=fileName.split("/").last();

    emit statusMessage (QString(tr( "File %1 saved" ) ).arg( fileNameNoPath ));
    return true;



}


/**
 * @brief Graph::saveGraphToAdjacencyFormat
 * @param fileName
 * @param maxWidth
 * @param maxHeight
 * @return
 */
bool Graph::saveGraphToAdjacencyFormat (QString fileName){
    QFile file( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage(QString(tr("Could not write to %1")).arg(fileName));
        return false;
    }
    QTextStream outText( &file );
    outText.setCodec("UTF-8");
    qDebug("Graph: saveGraphToAdjacencyFormat() for %i vertices", vertices());

    writeAdjacencyMatrixTo(outText);

    file.close();
    QString fileNameNoPath=fileName.split("/").last();
    emit statusMessage (QString( tr("Adjacency matrix-formatted network saved into file %1") ).arg( fileNameNoPath ));
    return true;
}




/**
 * @brief Graph::writeDataSetToFile
 * Writes a known dataset to the given file
 * @param fileName
 */
void Graph::writeDataSetToFile (const QString dir, const QString fileName) {
    qDebug() << "Graph::writeDataSetToFile() to " << dir+fileName;
    QFile file( dir+fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText( &file );
    outText.setCodec("UTF-8");
    QString datasetDescription=QString::null;
    qDebug()<< "		... writing dataset ";
    if ( fileName == "Krackhardt_High-tech_managers_Advice_relation.sm" ) {
        qDebug()<< "		... to  " << fileName;
        outText <<
                   "0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0 0 1" << endl <<
                   "0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                   "1 1 0 1 0 1 1 1 1 1 1 1 0 1 0 0 1 1 0 1 1" << endl <<
                   "1 1 0 0 0 1 0 1 0 1 1 1 0 0 0 1 1 1 0 1 1" << endl <<
                   "1 1 0 0 0 1 1 1 0 1 1 0 1 1 0 1 1 1 1 1 1" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                   "0 1 0 0 0 1 0 0 0 0 1 1 0 1 0 0 1 1 0 0 1" << endl <<
                   "0 1 0 1 0 1 1 0 0 1 1 0 0 0 0 0 0 1 0 0 1" << endl <<
                   "1 1 0 0 0 1 1 1 0 1 1 1 0 1 0 1 1 1 0 0 1" << endl <<
                   "1 1 1 1 1 0 0 1 0 0 1 0 1 0 1 1 1 1 1 1 0" << endl <<
                   "1 1 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                   "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                   "1 1 0 0 1 0 0 0 1 0 0 0 0 1 0 0 0 1 0 0 0" << endl <<
                   "0 1 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 0 0 1" << endl <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1" << endl <<
                   "1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0" << endl <<
                   "1 1 0 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                   "1 1 1 1 1 0 1 1 1 1 1 0 1 1 1 1 0 0 1 1 1" << endl <<
                   "1 1 1 0 1 0 1 0 0 1 1 0 0 1 1 0 0 1 0 1 0" << endl <<
                   "1 1 0 0 0 1 0 1 0 0 1 1 0 1 1 1 1 1 0 0 1" << endl <<
                   "0 1 1 1 0 1 1 1 0 0 0 1 0 1 0 0 1 1 0 1 0";

    }
    else if (fileName == "Krackhardt_High-tech_managers_Friendship_relation.sm"){
        outText<< "0 1 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 0 0" << endl <<
                  "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0" << endl <<
                  "1 1 0 0 0 0 0 1 0 0 0 1 0 0 0 1 1 0 0 0 0" << endl <<
                  "0 1 0 0 0 0 0 0 1 0 1 0 0 1 0 0 1 0 1 0 1" << endl <<
                  "0 1 0 0 0 0 1 0 1 0 0 1 0 0 0 0 1 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 1 0 1 0 0 1 1 0 0 1 0 0 0 1 0 0 0 1 0" << endl <<
                  "1 1 1 1 1 0 0 1 1 0 0 1 1 0 1 0 1 1 1 0 0" << endl <<
                  "1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1" << endl <<
                  "0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0" << endl <<
                  "1 0 1 0 1 1 0 0 1 0 1 0 0 1 0 0 0 0 1 0 0" << endl <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 0 0 1 1 1" << endl <<
                  "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 1 0 1 0 0 0 0 0 1 1 0 1 1 0 0 0 0 1 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 1 0 0 0" << endl <<
                  "0 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 1 0 0 0" ;
    }
    else if (fileName == "Krackhardt_High-tech_managers_ReportsTo_relation.sm"){
        outText<< "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
    }
    else if (fileName == "Padgett_Florentine_Families_Marital_relation.net"){
        outText<< "*Network Padgett's Florentine Families Marital Relation" << endl <<
                  "*Vertices      16" << endl <<
                    "1 \"Acciaiuoli\"         0.2024    0.1006" << endl <<
                    "2 \"Albizzi\"            0.3882    0.4754" << endl <<
                    "3 \"Barbadori\"          0.1633    0.7413" << endl <<
                    "4 \"Bischeri\"           0.6521    0.5605" << endl <<
                    "5 \"Castellani\"         0.6178    0.9114" << endl <<
                    "6 \"Ginori\"             0.3018    0.5976" << endl <<
                    "7 \"Guadagni\"           0.5219    0.5006" << endl <<
                    "8 \"Lamberteschi\"       0.4533    0.6299" << endl <<
                    "9 \"Medici\"             0.2876    0.3521" << endl <<
                   "10 \"Pazzi\"              0.0793    0.2587" << endl <<
                   "11 \"Peruzzi\"            0.6509    0.7365" << endl <<
                   "12 \"Pucci\"              0.4083    0.1186" << endl <<
                   "13 \"Ridolfi\"            0.6308    0.2060" << endl <<
                   "14 \"Salviati\"           0.0734    0.4455" << endl <<
                   "15 \"Strozzi\"            0.8639    0.5832" << endl <<
                   "16 \"Tornabuoni\"         0.5633    0.3713" << endl <<
                  "*Arcs \"Marital\""<< endl <<
                    "1  9 1" << endl <<
                    "2  6 1" << endl <<
                    "2  7 1" << endl <<
                    "2  9 1" << endl <<
                    "3  5 1" << endl <<
                    "3  9 1" << endl <<
                    "4  7 1" << endl <<
                    "4 11 1" << endl <<
                    "4 15 1" << endl <<
                    "5  3 1" << endl <<
                    "5 11 1" << endl <<
                    "5 15 1" << endl <<
                    "6  2 1" << endl <<
                    "7  2 1" << endl <<
                    "7  4 1" << endl <<
                    "7  8 1" << endl <<
                    "7 16 1" << endl <<
                    "8  7 1" << endl <<
                    "9  1 1" << endl <<
                    "9  2 1" << endl <<
                    "9  3 1" << endl <<
                    "9 13 1" << endl <<
                    "9 14 1" << endl <<
                    "9 16 1" << endl <<
                   "10 14 1" << endl <<
                   "11  4 1" << endl <<
                   "11  5 1" << endl <<
                   "11 15 1" << endl <<
                   "13  9 1" << endl <<
                   "13 15 1" << endl <<
                   "13 16 1" << endl <<
                   "14  9 1" << endl <<
                   "14 10 1" << endl <<
                   "15  4 1" << endl <<
                   "15  5 1" << endl <<
                   "15 11 1" << endl <<
                   "15 13 1" << endl <<
                   "16  7 1" << endl <<
                   "16  9 1" << endl <<
                  "16 13 1" ;
    }
    else if (fileName == "Padgett_Florentine_Families_Business_relation.paj"){
        outText<< "*Network Padgett's Florentine Families Business Relation" << endl <<
                  "*Vertices      16" << endl <<
                    "1 \"Acciaiuoli\"         0.2024    0.1006" << endl <<
                    "2 \"Albizzi\"            0.3882    0.4754" << endl <<
                    "3 \"Barbadori\"          0.1633    0.7413" << endl <<
                    "4 \"Bischeri\"           0.6521    0.5605" << endl <<
                    "5 \"Castellani\"         0.6178    0.9114" << endl <<
                    "6 \"Ginori\"             0.3018    0.5976" << endl <<
                    "7 \"Guadagni\"           0.5219    0.5006" << endl <<
                    "8 \"Lamberteschi\"       0.4533    0.6299" << endl <<
                    "9 \"Medici\"             0.2876    0.3521" << endl <<
                   "10 \"Pazzi\"              0.0793    0.2587" << endl <<
                   "11 \"Peruzzi\"            0.6509    0.7365" << endl <<
                   "12 \"Pucci\"              0.4083    0.1186" << endl <<
                   "13 \"Ridolfi\"            0.6308    0.2060" << endl <<
                   "14 \"Salviati\"           0.0734    0.4455" << endl <<
                   "15 \"Strozzi\"            0.8639    0.5832" << endl <<
                   "16 \"Tornabuoni\"         0.5633    0.3713" << endl <<
                  "*Arcs \"Business\""<< endl <<
                    "3  5 1" << endl <<
                    "3  6 1" << endl <<
                    "3  9 1" << endl <<
                    "3 11 1" << endl <<
                    "4  7 1" << endl <<
                    "4  8 1" << endl <<
                    "4 11 1" << endl <<
                    "5  3 1" << endl <<
                    "5  8 1" << endl <<
                    "5 11 1" << endl <<
                    "6  3 1" << endl <<
                    "6  9 1" << endl <<
                    "7  4 1" << endl <<
                    "7  8 1" << endl <<
                    "8  4 1" << endl <<
                    "8  5 1" << endl <<
                    "8  7 1" << endl <<
                    "8 11 1" << endl <<
                    "9  3 1" << endl <<
                    "9  6 1" << endl <<
                    "9 10 1" << endl <<
                    "9 14 1" << endl <<
                    "9 16 1" << endl <<
                   "10  9 1" << endl <<
                   "11  3 1" << endl <<
                   "11  4 1" << endl <<
                   "11  5 1" << endl <<
                   "11  8 1" << endl <<
                   "14  9 1" << endl <<
                   "16  9 1";
    }
    else if (fileName == "Zachary_Karate_Club_Simple_Ties.sm"){
        outText<< "0 1 1 1 1 1 1 1 1 0 1 1 1 1 0 0 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 1 0 0" << endl <<
                  "1 0 1 1 0 0 0 1 0 0 0 0 0 1 0 0 0 1 0 1 0 1 0 0 0 0 0 0 0 0 1 0 0 0" << endl <<
                  "1 1 0 1 0 0 0 1 1 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 0" << endl <<
                  "1 1 1 0 0 0 0 1 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 0 0 0 1 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 1" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "1 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 0 0 1 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 0 0 1 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 1 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 1 0 0 0 0 0 1 1" << endl <<
                  "0 1 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1" << endl <<
                  "1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 1 1" << endl <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 1 1 0 0 1 0 1 0 1 1 0 0 0 0 0 1 1 1 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 1 1 0 0 0 1 1 1 0 0 1 1 1 0 1 1 0 0 1 1 1 1 1 1 1 0" ;
    }
    else if (fileName == "Zachary_Karate_Club_Weighted_Ties.sm"){
        outText<< "0 4 5 3 3 3 3 2 2 0 2 3 1 3 0 0 0 2 0 2 0 2 0 0 0 0 0 0 0 0 0 2 0 0" << endl <<
                  "4 0 6 3 0 0 0 4 0 0 0 0 0 5 0 0 0 1 0 2 0 2 0 0 0 0 0 0 0 0 2 0 0 0" << endl <<
                  "5 6 0 3 0 0 0 4 5 1 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 2 2 0 0 0 2 0" << endl <<
                  "3 3 3 0 0 0 0 3 0 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "3 0 0 0 0 0 2 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "3 0 0 0 0 0 5 0 0 0 3 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "3 0 0 0 2 5 0 0 0 0 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "2 4 4 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "2 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 0 3 4" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2" << endl <<
                  "2 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "3 5 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 2" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 4" << endl <<
                  "0 0 0 0 0 3 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 2" << endl <<
                  "2 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 1" << endl <<
                  "2 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 3" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 0 4 0 3 0 0 5 4" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 3 0 0 0 2 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 2 0 0 0 0 0 0 7 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4 0 0 0 2" << endl <<
                  "0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4 3 0 0 0 0 0 0 0 0 4" << endl <<
                  "0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 2" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 0 0 4 0 0 0 0 0 4 2" << endl <<
                  "0 2 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 3" << endl <<
                  "2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 7 0 0 2 0 0 0 4 4" << endl <<
                  "0 0 2 0 0 0 0 0 3 0 0 0 0 0 3 3 0 0 1 0 3 0 2 5 0 0 0 0 0 4 3 4 0 5" << endl <<
                  "0 0 0 0 0 0 0 0 4 2 0 0 0 3 2 4 0 0 2 1 1 0 3 4 0 0 2 4 2 2 3 4 5 0";
    }
    else if (fileName == "Galaskiewicz_CEOs_and_clubs_affiliation_network_data.2sm"){
        outText<< "0 0 1 1 0 0 0 0 1 0 0 0 0 0 0" << endl <<
                  "0 0 1 0 1 0 1 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 1 0 0 0" << endl <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 0 0 1 1 0" << endl <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 0 1 0" << endl <<
                  "0 0 1 1 0 0 0 0 0 1 1 0 0 0 0" << endl <<
                  "0 0 0 1 0 0 1 0 0 1 0 0 0 0 0" << endl <<
                  "1 0 0 1 0 0 0 1 0 1 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 0" << endl <<
                  "0 1 1 0 0 0 0 0 1 0 0 0 0 0 0" << endl <<
                  "0 0 0 1 0 0 1 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 1 1 1 0 0 0 1 0 0 0 0 0 0" << endl <<
                  "0 1 1 1 0 0 0 0 0 0 1 1 1 0 1" << endl <<
                  "0 1 1 0 0 1 0 0 0 0 0 0 1 0 1" << endl <<
                  "0 1 1 0 0 1 0 1 0 0 0 0 0 1 0" << endl <<
                  "0 1 1 0 1 0 0 0 0 0 1 1 0 0 1" << endl <<
                  "0 0 0 1 0 0 0 0 1 0 0 1 1 0 1" << endl <<
                  "1 0 1 1 0 0 1 0 1 0 0 0 0 0 0" << endl <<
                  "0 1 1 1 0 0 0 0 0 0 1 0 0 0 1" << endl <<
                  "0 0 1 1 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 0 0 1 0 0 0 0 0 0 1" << endl <<
                  "0 1 1 0 0 1 0 0 0 0 0 0 0 0 1" << endl <<
                  "1 0 1 1 0 1 0 0 0 0 0 0 0 0 1" << endl <<
                  "0 1 1 0 0 0 0 0 0 0 0 0 1 0 0" << endl <<
                  "0 1 1 0 0 0 0 0 0 0 0 1 0 0 0";
    }
    else if (fileName == "Bernard_Killworth_Fraternity.dl"){
        datasetDescription =
                tr("Bernard & Killworth recorded the interactions among students living in a fraternity at "
                   "a West Virginia college. Subjects had been residents in the fraternity from 3 months to 3 years. "
                   "This network dataset contains two relations: \n"
                   "The BKFRAB relation is symmetric and valued. It counts the number of times a pair of subjects were "
                   "seen in conversation by an unobtrusive observer (observation time: 21 hours a day, for five days). \n"
                   "The BKFRAC relation is non-symmetric and valued. Contains rankings made by the subjects themselves of "
                   "how frequently they interacted with other subjects in the observation week.");
        outText << "DL"<<endl<<
                   "N=58 NM=2"<<endl<<
                   "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<endl<<
                   "LEVEL LABELS:"<<endl<<
                   "BKFRAB"<<endl<<
                   "BKFRAC"<<endl<<
                   "DATA:"<<endl<<
                     "0  0  2  1  0  0  2  0  0  0  1  1  2  0  0  0  1  0  1  0  0  1  0  0  0  0"<<endl<<
                     "0  0  2  1  1  1  0  2  1  2  0  0  0  0  1  0  0  0  0  0  0  0  1  0  0  0"<<endl<<
                     "0  1  1  4  1  1"<<endl<<
                     "0  0 10  0  0  2  1  0  2  0  0  0  6  2  0  1  0  0  0  1  0 10  2  0  4  0"<<endl<<
                     "3  0  1  1  0  0  0  0  5  1  0  4  0  0  0  0  0  1  1  0  0  5  3  0  0  0"<<endl<<
                     "0  1  0  1  4  0"<<endl<<
                     "2 10  0  6 11 14 15  4 12  0  5  4  3  8 10  8 11  0  2 19  2 15  1  2  6  1"<<endl<<
                     "5  0 12  5  4  0  1  4 15  3  1  3  6  0  2  3  0  9  8  2  1  3  6  2  0  2"<<endl<<
                     "2 16  4  5 19  1"<<endl<<
                     "1  0  6  0  2  3  9  1  8  0  0  5  0  0  2  4  3  2  2  6  0  1  1  3  1  0"<<endl<<
                     "5  1  1  3  0  1  1  4  1  0  1  3  2  0  1  0  0  1  1  1  1  2  1  3  0  0"<<endl<<
                     "2  1  2  2  3  5"<<endl<<
                     "0  0 11  2  0  2  8  1  1  1  0  0  2  0  1  1  0  0  0  3  0  0  0  0  0  0"<<endl<<
                     "8  0  1  5  0  0  1  0  0  0  0  0  9  2  1  0  1  8 25  0  0  0  0  0  0  0"<<endl<<
                     "1  2  0  0  4  0"<<endl<<
                     "0  2 14  3  2  0 30  2  8  0  4  4  1  6  2 14  9  0  1 51  0  3  2  1  0  1"<<endl<<
                     "6  0  3 11  2  0 15  5  3  1  0  2  2  1  3  1  0  3  2  2  6  1  3  4  0  2"<<endl<<
                     "8  9  3  2 18  2"<<endl<<
                     "2  1 15  9  8 30  0 10  4  2  7  3  0 12  9 10  9  2  3 40  2  2  5  2  0  1"<<endl<<
                    "19  1 10 14  5  3 14  7  7  5  3  4  5  7  8  5  0  2  4  7  3  7  7  2  0  0"<<endl<<
                     "6  5 14 16 20  4"<<endl<<
                     "0  0  4  1  1  2 10  0  3  0  2  0  1  3  3  3  5  0  0  6  1  0  2  3  0  1"<<endl<<
                     "6  0  2  0  9  1  0  1  2  4  2  5  1  0  3  5  0  0  5  0  1  3  1  1  0  1"<<endl<<
                     "2  5  0  2  4  2"<<endl<<
                     "0  2 12  8  1  8  4  3  0  0  5  5  2  2  4  5  6  1  0  5  0  5  0  3  3  3"<<endl<<
                     "3  1  2  3  1  0  2  4  4  3  5  1  2  0  1  1  1  2  0  0  4  0  1  4  0  6"<<endl<<
                     "1  4  3  2  7  1"<<endl<<
                     "0  0  0  0  1  0  2  0  0  0  0  0  0  0  1  2  0  0  0  0  0  0  0  0  0  0"<<endl<<
                     "6  0  1  0  1  0  0  0  0  0  0  1  2  2  0  0  0  0  0  1  0  0  0  0  0  0"<<endl<<
                     "0  1  0  0  0  0"<<endl<<
                     "1  0  5  0  0  4  7  2  5  0  0  0  0  1  3  3  5  3  0  7  4  1  0  3  0  0"<<endl<<
                     "4  0  5  1  3  0  0  2  2  3  5  3  2  0  0  1  0  2  1  4  5  2  1  0  0  0"<<endl<<
                     "0  4  6  6 12  0"<<endl<<
                     "1  0  4  5  0  4  3  0  5  0  0  0  0  0  0  0  0  0  0  3  0  1  0  1  1  0"<<endl<<
                     "0  0  2  0  2  0  1  2  3  2  2  1  0  0  0  1  0  1  1  1  0  0  1  2  0  0"<<endl<<
                     "1  2  0  7  3  3"<<endl<<
                     "2  6  3  0  2  1  0  1  2  0  0  0  0  2  1  3  3  0  1  0  0  6  2  0  0  0"<<endl<<
                     "3  0  1  0  0  0  1  1  1  0  0  1  1  1  1  1  1  0  2  1  0  0  2  0  0  0"<<endl<<
                     "2  4  1  0  0  0"<<endl<<
                     "0  2  8  0  0  6 12  3  2  0  1  0  2  0  3  8 11  1  4  8  0  1  0  0  1  1"<<endl<<
                     "4  0  8  4  6  0  3  1  5  1  1  0  0  0  1  3  0  2  2  1  1  1  0  0  0  0"<<endl<<
                     "1  0  2  1  5  1"<<endl<<
                     "0  0 10  2  1  2  9  3  4  1  3  0  1  3  0  9 14  0  6  9  0  2  1  2  1  0"<<endl<<
                     "4  0  3  0  2  1  1  4  2  3  0  6  1  0  7  1  0  7  1  1  0  0  1  1  0  0"<<endl<<
                     "7  6  4  9  4  0"<<endl<<
                     "0  1  8  4  1 14 10  3  5  2  3  0  3  8  9  0 26  3  1 12  0  2  0  0  1  0"<<endl<<
                     "7  0  5  6  5  4  2  2  2  2  0  4  4  0  2  5  1  3  2  1  1  4  0  2  0  0"<<endl<<
                     "8  4  2  0 11  3"<<endl<<
                     "1  0 11  3  0  9  9  5  6  0  5  0  3 11 14 26  0  3  0  9  0  1  0  0  1  0"<<endl<<
                     "5  0  5  2  2  4  2  1  4  2  0  1  1  1  2  3  0  3  1  0  0  3  1  2  0  0"<<endl<<
                     "7  7  4  0 11  0"<<endl<<
                     "0  0  0  2  0  0  2  0  1  0  3  0  0  1  0  3  3  0  0  0  3  0  0  0  0  0"<<endl<<
                     "0  0  1  0  0  3  0  1  1  1  1  0  1  0  0  0  0  1  0  2  0  2  0  0  0  0"<<endl<<
                     "0  0  2  1  0  1"<<endl<<
                     "1  0  2  2  0  1  3  0  0  0  0  0  1  4  6  1  0  0  0  5  0  0  2  1  3  0"<<endl<<
                     "0  0  0  1  1  0  0  1  1  1  1  2  0  1 14  1  0  1  0  0  1  0  3  0  0  0"<<endl<<
                     "1  0  0  3  1  2"<<endl<<
                     "0  1 19  6  3 51 40  6  5  0  7  3  0  8  9 12  9  0  5  0  3  2  3  2  1  1"<<endl<<
                     "7  1 10  6  6  1 13 12  9  2  1  6  2  1 10  4  0  2  2  1  2  1  6  1  0  0"<<endl<<
                    "12 17 11  9 23  5"<<endl<<
                     "0  0  2  0  0  0  2  1  0  0  4  0  0  0  0  0  0  3  0  3  0  0  1  0  0  0"<<endl<<
                     "0  0  2  0  2  0  0  1  1  1  0  1  0  0  1  1  0  0  0  5  0  1  1  0  0  0"<<endl<<
                     "0  1  2  4  2  1"<<endl<<
                     "1 10 15  1  0  3  2  0  5  0  1  1  6  1  2  2  1  0  0  2  0  0  1  1  7  2"<<endl<<
                     "1  0  3  1  0  0  0  0  1  1  1  0  2  0  0  0  0  1  0  3  0  0  2  1  0  0"<<endl<<
                     "0  2  1  1  3  0"<<endl<<
                     "0  2  1  1  0  2  5  2  0  0  0  0  2  0  1  0  0  0  2  3  1  1  0  0  1  0"<<endl<<
                     "1  0  2  0  2  0  3  1  2  1  2  2  2  1  7  1  0  1  2  0  2  0 11  1  1  0"<<endl<<
                     "1  4  1  2  3  1"<<endl<<
                     "0  0  2  3  0  1  2  3  3  0  3  1  0  0  2  0  0  0  1  2  0  1  0  0  0  1"<<endl<<
                     "0  0  1  1  1  0  0  2  1  1  0  2  0  0  0  0  0  1  0  1  0  1  0  0  0  0"<<endl<<
                     "0  0  0  2  1  1"<<endl<<
                     "0  4  6  1  0  0  0  0  3  0  0  1  0  1  1  1  1  0  3  1  0  7  1  0  0  0"<<endl<<
                     "0  0  3  1  0  0  0  0  3  0  1  1  0  0  4  0  0  1  0  0  0  0  0  0  0  0"<<endl<<
                     "2  1  1  1  5  0"<<endl<<
                     "0  0  1  0  0  1  1  1  3  0  0  0  0  1  0  0  0  0  0  1  0  2  0  1  0  0"<<endl<<
                     "1  0  0  1  0  0  0  0  1  0  0  1  3  0  0  0  0  0  1  0  0  1  2  0  0  2"<<endl<<
                     "0  1  1  1  2  0"<<endl<<
                     "0  3  5  5  8  6 19  6  3  6  4  0  3  4  4  7  5  0  0  7  0  1  1  0  0  1"<<endl<<
                     "0  0  6  6  2  1  1  4  0  1  0  2  4  0  3  2  1  1  4  1  0  5  2  0  0  0"<<endl<<
                     "1  2  2  4  6  2"<<endl<<
                     "0  0  0  1  0  0  1  0  1  0  0  0  0  0  0  0  0  0  0  1  0  0  0  0  0  0"<<endl<<
                     "0  0  0  0  0  0  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0"<<endl<<
                     "1  0  0  0  0  0"<<endl<<
                     "2  1 12  1  1  3 10  2  2  1  5  2  1  8  3  5  5  1  0 10  2  3  2  1  3  0"<<endl<<
                     "6  0  0  3  1  0  0  0 20  2  2  3  3  2  1  2  0  3  3  0  1  1  1  1  0  0"<<endl<<
                     "0  7  1  2 10  1"<<endl<<
                     "1  1  5  3  5 11 14  0  3  0  1  0  0  4  0  6  2  0  1  6  0  1  0  1  1  1"<<endl<<
                     "6  0  3  0  3  0  1  0  6  1  1  1  3  1  4  1  2  0  1  0  5  1  3  1  0  0"<<endl<<
                     "3  2  1  6 10  2"<<endl<<
                     "1  0  4  0  0  2  5  9  1  1  3  2  0  6  2  5  2  0  1  6  2  0  2  1  0  0"<<endl<<
                     "2  0  1  3  0  4  0  3  1  3  0  1  0  1  3  3  0  0  1  3  0  2  1  0  0  0"<<endl<<
                     "1  4  1  1  3  2"<<endl<<
                     "1  0  0  1  0  0  3  1  0  0  0  0  0  0  1  4  4  3  0  1  0  0  0  0  0  0"<<endl<<
                     "1  0  0  0  4  0  0  2  0  0  0  0  0  0  1  0  0  0  0  3  0  6  0  0  0  0"<<endl<<
                     "0  0  0  0  0  1"<<endl<<
                     "0  0  1  1  1 15 14  0  2  0  0  1  1  3  1  2  2  0  0 13  0  0  3  0  0  0"<<endl<<
                     "1  0  0  1  0  0  0  1  1  1  0  0  0  3  1  0  0  0  0  0  0  0  1  0  0  2"<<endl<<
                     "8  1  0  1  3  0"<<endl<<
                     "2  0  4  4  0  5  7  1  4  0  2  2  1  1  4  2  1  1  1 12  1  0  1  2  0  0"<<endl<<
                     "4  1  0  0  3  2  1  0  3  1  0  0  1  1  2  1  0  0  0  3  2  2  1  3  0  0"<<endl<<
                     "2  4  3  4  3  6"<<endl<<
                     "1  5 15  1  0  3  7  2  4  0  2  3  1  5  2  2  4  1  1  9  1  1  2  1  3  1"<<endl<<
                     "0  0 20  6  1  0  1  3  0  2  1  3  2  2  3  4  2  2  0  0  1  0  6  1  0  0"<<endl<<
                     "1 12  2  3  6  2"<<endl<<
                     "2  1  3  0  0  1  5  4  3  0  3  2  0  1  3  2  2  1  1  2  1  1  1  1  0  0"<<endl<<
                     "1  0  2  1  3  0  1  1  2  0  0  0  1  0  1  2  0  1  0  3  0  0  3  0  0  0"<<endl<<
                     "1  0  2 10  1  1"<<endl<<
                     "0  0  1  1  0  0  3  2  5  0  5  2  0  1  0  0  0  1  1  1  0  1  2  0  1  0"<<endl<<
                     "0  0  2  1  0  0  0  0  1  0  0  0  3  0  1  0  0  0  1  0  4  0  2  0  1  0"<<endl<<
                     "2  1  0  1  3  0"<<endl<<
                     "0  4  3  3  0  2  4  5  1  1  3  1  1  0  6  4  1  0  2  6  1  0  2  2  1  1"<<endl<<
                     "2  0  3  1  1  0  0  0  3  0  0  0  0  1  2  1  0  0  1  0  2  0  0  1  0  0"<<endl<<
                     "1  6  1  1  4  2"<<endl<<
                     "0  0  6  2  9  2  5  1  2  2  2  0  1  0  1  4  1  1  0  2  0  2  2  0  0  3"<<endl<<
                     "4  0  3  3  0  0  0  1  2  1  3  0  0  1  0  0  0  4  9  2  1  2  5  4  3  0"<<endl<<
                     "0  2  2  1  2  0"<<endl<<
                     "0  0  0  0  2  1  7  0  0  2  0  0  1  0  0  0  1  0  1  1  0  0  1  0  0  0"<<endl<<
                     "0  0  2  1  1  0  3  1  2  0  0  1  1  0  0  0  0  0  0  0  1  0  0  0  0  0"<<endl<<
                     "1  2  0  0  2  0"<<endl<<
                     "1  0  2  1  1  3  8  3  1  0  0  0  1  1  7  2  2  0 14 10  1  0  7  0  4  0"<<endl<<
                     "3  0  1  4  3  1  1  2  3  1  1  2  0  0  0  1  1  1  1  0  0  0  9  0  0  0"<<endl<<
                     "4  1  1  5  1  2"<<endl<<
                     "0  0  3  0  0  1  5  5  1  0  1  1  1  3  1  5  3  0  1  4  1  0  1  0  0  0"<<endl<<
                     "2  0  2  1  3  0  0  1  4  2  0  1  0  0  1  0  1  1  1  1  1  1  1  0  0  0"<<endl<<
                     "2  1  1  0  3  1"<<endl<<
                     "0  0  0  0  1  0  0  0  1  0  0  0  1  0  0  1  0  0  0  0  0  0  0  0  0  0"<<endl<<
                     "1  0  0  2  0  0  0  0  2  0  0  0  0  0  1  1  0  0  0  0  0  0  0  0  0  0"<<endl<<
                     "0  0  1  0  0  0"<<endl<<
                     "0  1  9  1  8  3  2  0  2  0  2  1  0  2  7  3  3  1  1  2  0  1  1  1  1  0"<<endl<<
                     "1  0  3  0  0  0  0  0  2  1  0  0  4  0  1  1  0  0  2  0  1  0  2  1  0  0"<<endl<<
                     "0  2  1  2  3  0"<<endl<<
                     "0  1  8  1 25  2  4  5  0  0  1  1  2  2  1  2  1  0  0  2  0  0  2  0  0  1"<<endl<<
                     "4  0  3  1  1  0  0  0  0  0  1  1  9  0  1  1  0  2  0  0  1  2  4  1  1  0"<<endl<<
                     "0  4  0  0  1  0"<<endl<<
                     "0  0  2  1  0  2  7  0  0  1  4  1  1  1  1  1  0  2  0  1  5  3  0  1  0  0"<<endl<<
                     "1  0  0  0  3  3  0  3  0  3  0  0  2  0  0  1  0  0  0  0  0  5  1  0  0  0"<<endl<<
                     "0  0  1  2  4  1"<<endl<<
                     "0  0  1  1  0  6  3  1  4  0  5  0  0  1  0  1  0  0  1  2  0  0  2  0  0  0"<<endl<<
                     "0  0  1  5  0  0  0  2  1  0  4  2  1  1  0  1  0  1  1  0  0  1  2  0  2  0"<<endl<<
                     "3  0  0  2  6  1"<<endl<<
                     "0  5  3  2  0  1  7  3  0  0  2  0  0  1  0  4  3  2  0  1  1  0  0  1  0  1"<<endl<<
                     "5  0  1  1  2  6  0  2  0  0  0  0  2  0  0  1  0  0  2  5  1  0  3  2  0  0"<<endl<<
                     "0  2  1  0  2  0"<<endl<<
                     "1  3  6  1  0  3  7  1  1  0  1  1  2  0  1  0  1  0  3  6  1  2 11  0  0  2"<<endl<<
                     "2  0  1  3  1  0  1  1  6  3  2  0  5  0  9  1  0  2  4  1  2  3  0  4  0  1"<<endl<<
                     "4  4  2  2  3  1"<<endl<<
                     "0  0  2  3  0  4  2  1  4  0  0  2  0  0  1  2  2  0  0  1  0  1  1  0  0  0"<<endl<<
                     "0  0  1  1  0  0  0  3  1  0  0  1  4  0  0  0  0  1  1  0  0  2  4  0  1  0"<<endl<<
                     "0  1  1  1  0  3"<<endl<<
                     "0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0  0  0"<<endl<<
                     "0  0  0  0  0  0  0  0  0  0  1  0  3  0  0  0  0  0  1  0  2  0  0  1  0  0"<<endl<<
                     "0  0  0  0  0  0"<<endl<<
                     "0  0  2  0  0  2  0  1  6  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  2"<<endl<<
                     "0  0  0  0  0  0  2  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0  0  0"<<endl<<
                     "0  0  0  0  0  0"<<endl<<
                     "0  0  2  2  1  8  6  2  1  0  0  1  2  1  7  8  7  0  1 12  0  0  1  0  2  0"<<endl<<
                     "1  1  0  3  1  0  8  2  1  1  2  1  0  1  4  2  0  0  0  0  3  0  4  0  0  0"<<endl<<
                     "0  5  1  2  4  3"<<endl<<
                     "1  1 16  1  2  9  5  5  4  1  4  2  4  0  6  4  7  0  0 17  1  2  4  0  1  1"<<endl<<
                     "2  0  7  2  4  0  1  4 12  0  1  6  2  2  1  1  0  2  4  0  0  2  4  1  0  0"<<endl<<
                     "5  0  5  3 10  0"<<endl<<
                     "1  0  4  2  0  3 14  0  3  0  6  0  1  2  4  2  4  2  0 11  2  1  1  0  1  1"<<endl<<
                     "2  0  1  1  1  0  0  3  2  2  0  1  2  0  1  1  1  1  0  1  0  1  2  1  0  0"<<endl<<
                     "1  5  0 12  7  1"<<endl<<
                     "4  1  5  2  0  2 16  2  2  0  6  7  0  1  9  0  0  1  3  9  4  1  2  2  1  1"<<endl<<
                     "4  0  2  6  1  0  1  4  3 10  1  1  1  0  5  0  0  2  0  2  2  0  2  1  0  0"<<endl<<
                     "2  3 12  0 12  0"<<endl<<
                     "1  4 19  3  4 18 20  4  7  0 12  3  0  5  4 11 11  0  1 23  2  3  3  1  5  2"<<endl<<
                     "6  0 10 10  3  0  3  3  6  1  3  4  2  2  1  3  0  3  1  4  6  2  3  0  0  0"<<endl<<
                     "4 10  7 12  0  1"<<endl<<
                     "1  0  1  5  0  2  4  2  1  0  0  3  0  1  0  3  0  1  2  5  1  0  1  1  0  0"<<endl<<
                     "2  0  1  2  2  1  0  6  2  1  0  2  0  0  2  1  0  0  0  1  1  0  1  3  0  0"<<endl<<
                     "3  0  1  0  1  0"<<endl<<
                    "0 4 4 5 4 4 5 5 4 5 5 4 4 5 5 4 4 5 5 5 5 4 5 4 4 5 5 4 4 5 5 5 4 4 4 5 4 5 4 4"<<endl<<
                    "5 4 5 5 4 5 5 5 4 5 4 5 4 4 5 5 5 5"<<endl<<
                    "3 0 2 3 4 2 2 2 3 2 3 3 5 4 3 3 3 2 3 3 2 5 4 2 5 3 2 2 3 4 4 2 4 2 5 3 3 5 3 2"<<endl<<
                    "3 3 3 2 4 2 4 3 3 3 2 3 4 4 2 3 2 2"<<endl<<
                    "2 2 0 4 5 4 4 1 4 3 3 3 3 3 4 4 4 2 2 5 2 5 3 1 4 3 3 2 4 5 4 2 3 3 5 1 2 3 3 1"<<endl<<
                    "2 4 4 3 4 2 2 2 2 2 1 3 2 5 3 2 5 2"<<endl<<
                    "4 4 5 0 5 5 4 3 5 4 4 5 3 4 4 4 4 3 5 5 3 4 5 5 4 4 5 4 4 4 5 4 5 5 4 3 3 4 4 3"<<endl<<
                    "5 4 4 4 4 3 4 4 5 4 2 4 5 5 4 4 5 5"<<endl<<
                    "2 3 5 5 0 3 2 3 3 4 4 1 4 2 4 2 2 1 2 2 1 3 4 1 2 5 4 1 3 1 3 1 1 1 1 1 2 2 5 4"<<endl<<
                    "2 2 2 5 5 1 3 2 5 2 3 2 1 2 2 2 4 1"<<endl<<
                    "3 2 5 5 2 0 5 3 4 3 4 3 3 5 4 5 5 2 2 5 3 2 3 1 1 1 3 1 4 4 4 1 4 3 3 2 1 2 2 2"<<endl<<
                    "2 3 1 2 2 3 5 3 3 3 1 2 4 4 3 3 5 4"<<endl<<
                    "2 1 3 4 2 5 0 2 2 2 2 2 1 5 5 5 4 3 3 5 3 2 2 2 2 2 3 1 4 3 3 2 5 4 3 4 2 2 2 2"<<endl<<
                    "3 2 2 3 2 2 2 2 3 3 1 2 3 3 3 4 5 3"<<endl<<
                    "5 3 3 3 2 2 3 0 3 3 4 3 3 3 3 3 2 3 3 3 2 2 3 5 2 2 3 2 2 3 5 4 3 5 3 4 3 3 3 2"<<endl<<
                    "3 2 2 3 2 3 2 3 4 3 2 2 3 2 2 3 3 4"<<endl<<
                    "2 2 5 4 2 3 3 2 0 1 3 5 2 3 3 3 3 1 3 4 2 4 3 3 4 5 2 1 3 3 2 2 3 3 4 3 3 2 2 2"<<endl<<
                    "2 2 2 2 2 2 4 2 3 2 1 5 2 4 2 3 4 4"<<endl<<
                    "3 3 5 4 4 4 3 3 2 0 3 4 2 4 5 4 4 1 2 4 2 4 2 2 3 2 5 1 5 2 3 1 2 3 4 1 1 2 4 5"<<endl<<
                    "3 3 2 1 3 4 1 3 2 4 1 2 2 5 4 1 3 3"<<endl<<
                    "3 3 3 3 3 3 3 3 3 3 0 3 3 3 3 3 3 3 3 3 5 3 3 3 3 3 3 3 3 3 5 5 3 3 3 3 3 3 3 3"<<endl<<
                    "3 3 3 3 3 5 3 5 3 3 2 3 3 3 3 3 3 3"<<endl<<
                    "2 3 4 4 2 3 2 2 5 3 3 0 2 3 2 3 3 2 3 4 2 4 2 2 3 3 3 1 2 2 3 1 2 4 3 4 2 2 4 2"<<endl<<
                    "2 2 3 2 1 3 1 2 3 2 2 2 2 4 2 4 3 4"<<endl<<
                    "2 5 4 2 4 3 2 2 2 2 2 2 0 3 2 3 2 2 2 3 1 5 3 2 5 2 3 2 4 3 3 2 3 3 3 3 2 4 4 5"<<endl<<
                    "2 4 3 4 4 2 3 2 3 3 3 2 3 4 3 3 3 2"<<endl<<
                    "3 2 5 4 3 5 5 2 4 3 3 3 3 0 4 5 4 3 4 4 3 3 2 2 3 3 4 2 5 3 4 3 4 3 4 3 2 3 4 3"<<endl<<
                    "4 4 3 4 4 3 3 3 3 3 2 2 4 4 4 4 5 3"<<endl<<
                    "2 1 3 3 4 4 5 2 3 2 2 1 1 3 0 4 5 1 5 2 2 2 3 2 1 1 4 1 3 2 3 3 3 4 4 3 1 4 2 1"<<endl<<
                    "5 3 1 5 3 2 2 1 4 1 1 1 3 3 3 5 5 5"<<endl<<
                    "3 3 4 4 3 5 5 2 3 2 3 3 4 4 5 0 5 3 5 5 2 3 2 2 2 1 2 2 4 4 3 2 3 3 3 3 2 3 4 3"<<endl<<
                    "4 5 3 3 4 4 3 4 3 3 2 1 5 4 2 4 3 3"<<endl<<
                    "2 2 5 3 3 4 4 2 3 3 3 3 2 4 5 5 0 2 4 5 2 3 2 2 3 1 2 3 4 3 2 2 4 2 5 2 1 4 3 2"<<endl<<
                    "3 5 3 5 3 2 2 3 3 3 1 1 4 5 3 4 5 3"<<endl<<
                    "5 3 2 5 3 3 5 5 3 2 5 5 4 4 3 5 4 0 2 1 5 3 3 3 3 2 2 1 2 5 5 5 3 5 4 4 2 2 3 3"<<endl<<
                    "3 3 3 5 5 5 2 5 3 5 2 1 2 3 4 5 5 5"<<endl<<
                    "3 3 4 4 2 2 3 2 4 3 3 3 3 4 5 5 5 2 0 4 2 4 5 4 5 2 3 3 4 4 3 3 4 4 2 4 4 3 1 2"<<endl<<
                    "5 4 3 3 3 2 4 2 5 3 3 2 3 3 3 4 4 3"<<endl<<
                    "3 2 4 4 2 5 5 2 3 3 3 3 2 4 3 4 3 2 3 0 2 3 3 1 1 1 2 2 3 3 3 2 5 3 3 3 2 2 2 2"<<endl<<
                    "3 2 2 3 2 2 1 2 3 3 1 1 5 5 3 2 5 3"<<endl<<
                    "3 1 2 2 1 2 3 1 2 2 5 2 1 2 2 1 1 4 1 3 0 1 2 2 2 1 1 1 1 2 5 4 1 2 2 2 1 1 1 1"<<endl<<
                    "2 1 1 3 1 5 2 5 3 4 1 1 1 2 2 2 3 2"<<endl<<
                    "2 5 5 3 3 3 2 2 4 2 2 3 4 3 3 2 3 2 3 4 3 0 3 3 5 3 3 1 4 3 3 3 3 3 4 3 2 3 3 3"<<endl<<
                    "4 3 3 3 3 4 3 3 4 3 1 3 2 4 3 3 4 3"<<endl<<
                    "2 3 4 4 5 3 4 3 4 2 2 3 2 4 4 2 2 2 5 3 1 2 0 2 4 3 2 1 3 1 2 1 4 5 2 1 4 4 5 2"<<endl<<
                    "5 2 1 5 5 1 5 1 5 1 4 2 5 2 3 2 4 5"<<endl<<
                    "4 1 2 5 2 2 3 5 4 3 4 3 2 2 3 4 2 2 4 2 1 3 2 0 2 4 4 1 1 2 5 2 2 5 2 4 1 1 1 1"<<endl<<
                    "4 1 2 3 1 4 4 2 2 5 1 3 1 1 3 4 2 5"<<endl<<
                    "2 4 5 3 3 2 2 2 3 1 3 4 4 2 2 1 3 1 4 2 2 5 4 2 0 3 2 1 3 4 2 1 2 3 3 2 2 2 3 3"<<endl<<
                    "2 3 4 2 2 1 1 2 3 2 1 2 2 2 1 3 4 2"<<endl<<
                    "3 3 4 4 4 2 2 2 5 2 3 3 2 2 2 2 2 2 3 1 2 4 3 3 4 0 2 2 2 2 2 2 2 3 4 2 4 2 2 2"<<endl<<
                    "2 4 4 3 4 2 3 4 3 3 3 5 2 3 2 2 3 2"<<endl<<
                    "4 2 5 5 5 3 5 3 1 5 5 3 1 4 4 2 1 2 1 5 2 4 3 4 1 1 0 1 5 4 5 3 4 5 4 3 1 2 1 2"<<endl<<
                    "1 4 1 2 3 1 1 4 2 4 1 1 4 1 2 5 5 3"<<endl<<
                    "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 0 3 3 3 3 3 3 3 3 3 3 3 3"<<endl<<
                    "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3"<<endl<<
                    "2 3 5 3 3 3 4 2 3 4 3 2 3 5 4 4 4 1 3 3 1 4 3 1 4 1 5 2 0 2 3 2 3 2 5 3 2 4 4 3"<<endl<<
                    "2 4 4 3 2 1 2 1 2 1 1 2 3 5 3 3 5 2"<<endl<<
                    "5 2 5 5 1 3 2 3 3 1 5 1 2 2 3 3 1 1 2 5 2 2 1 3 3 1 3 1 3 0 2 2 2 3 4 2 4 3 1 1"<<endl<<
                    "3 4 3 3 1 1 5 2 1 2 1 2 3 3 3 3 5 3"<<endl<<
                    "4 2 4 4 2 4 5 5 3 3 5 3 2 3 4 3 3 3 3 4 4 3 4 5 3 2 4 2 4 2 0 5 3 4 4 4 3 3 2 2"<<endl<<
                    "4 4 2 3 2 4 3 4 3 4 2 4 3 4 3 4 4 3"<<endl<<
                    "5 3 4 4 3 4 4 4 3 3 5 3 2 4 5 4 4 5 4 5 5 4 3 4 3 2 4 2 2 5 5 0 5 5 4 3 3 3 2 2"<<endl<<
                    "3 3 2 4 2 5 3 5 5 5 2 2 4 4 3 4 5 5"<<endl<<
                    "3 3 3 5 2 5 5 2 3 2 3 4 3 4 4 4 3 3 4 5 2 2 4 2 3 3 3 4 3 3 2 3 0 4 4 3 2 3 2 3"<<endl<<
                    "3 3 4 2 2 2 3 2 4 3 2 3 5 4 4 4 4 4"<<endl<<
                    "4 3 4 5 3 4 4 4 4 3 3 3 2 2 3 3 3 4 5 5 3 2 5 5 3 4 4 2 3 4 4 4 5 0 3 4 4 3 4 3"<<endl<<
                    "5 3 3 3 2 4 3 3 5 5 2 4 5 3 4 5 4 5"<<endl<<
                    "2 4 5 3 3 4 4 2 3 4 3 4 3 3 4 2 5 2 4 4 2 4 3 2 5 3 3 2 5 4 3 2 4 2 0 2 2 4 4 3"<<endl<<
                    "4 4 5 3 2 1 4 1 3 3 2 2 2 5 2 3 4 2"<<endl<<
                    "5 1 2 5 1 4 5 1 3 3 5 5 2 5 3 4 2 3 5 5 5 2 1 1 1 1 3 3 3 2 2 2 5 4 4 0 1 1 1 1"<<endl<<
                    "5 3 4 4 4 5 4 5 1 2 1 2 1 3 1 5 5 4"<<endl<<
                    "2 3 3 4 2 4 3 3 4 3 3 4 3 3 3 3 2 2 5 3 2 3 5 2 4 4 3 2 2 3 3 2 3 4 4 2 0 4 5 2"<<endl<<
                    "4 3 3 4 2 2 5 2 5 3 5 3 3 3 3 2 3 3"<<endl<<
                    "2 5 3 3 2 3 2 2 3 3 3 3 3 3 4 4 4 2 4 4 2 2 3 2 3 2 2 3 4 3 3 3 4 2 5 3 3 0 3 2"<<endl<<
                    "3 4 4 3 2 3 4 3 3 3 2 2 4 4 2 3 4 3"<<endl<<
                    "2 2 3 4 5 3 3 2 3 4 4 5 4 4 3 5 3 2 2 4 2 5 5 2 5 4 4 2 5 2 2 2 2 3 5 2 5 2 0 3"<<endl<<
                    "2 5 5 3 5 2 5 2 5 3 5 2 3 4 3 3 4 3"<<endl<<
                    "2 3 3 1 4 3 4 2 1 2 1 2 5 3 3 3 2 2 4 4 1 2 1 2 3 2 2 1 4 1 1 1 2 3 3 3 2 2 3 0"<<endl<<
                    "2 3 2 3 3 1 1 1 1 2 1 5 2 4 2 2 4 2"<<endl<<
                    "3 2 2 5 3 4 4 2 2 4 2 2 2 4 4 3 3 2 5 5 2 4 5 2 3 2 2 2 3 4 3 2 4 3 4 4 3 3 3 2"<<endl<<
                    "0 2 4 3 4 2 3 2 5 3 4 3 5 2 2 4 4 5"<<endl<<
                    "2 1 4 1 2 3 3 2 1 1 3 1 4 4 3 5 5 1 3 3 1 2 1 1 2 2 2 1 4 3 3 1 2 2 5 1 1 4 2 1"<<endl<<
                    "2 0 5 2 1 1 2 1 2 1 1 1 4 4 2 3 3 1"<<endl<<
                    "2 3 4 3 2 2 2 1 3 2 2 2 2 3 2 3 2 1 4 2 1 2 2 2 5 4 2 2 3 3 2 1 2 1 5 3 2 3 3 2"<<endl<<
                    "3 4 0 1 2 1 3 1 2 1 1 2 3 4 3 3 2 1"<<endl<<
                    "3 2 4 4 4 3 3 2 2 1 4 2 3 4 5 3 5 5 3 3 5 4 4 2 2 2 2 4 3 2 3 2 3 2 2 3 4 3 3 2"<<endl<<
                    "1 3 3 0 3 3 5 3 4 2 1 1 3 2 2 3 4 2"<<endl<<
                    "2 3 4 4 5 2 1 1 1 3 4 3 2 4 4 4 2 1 1 1 1 2 4 1 2 3 3 1 1 1 3 1 1 1 1 1 1 1 5 3"<<endl<<
                    "2 1 1 3 0 1 1 1 4 1 1 1 1 1 1 1 4 1"<<endl<<
                    "4 2 3 1 1 2 3 3 3 4 4 3 2 3 3 3 2 4 3 4 4 3 3 2 1 1 3 1 4 3 4 5 1 2 3 4 1 2 3 2"<<endl<<
                    "1 1 1 4 1 0 1 5 3 3 1 2 1 3 3 4 4 1"<<endl<<
                    "3 3 3 3 4 4 2 2 5 1 5 3 2 2 3 3 3 1 5 3 1 3 5 2 3 3 1 2 3 5 3 2 2 2 4 3 4 3 5 2"<<endl<<
                    "3 2 3 5 2 1 0 1 4 2 5 2 3 1 4 3 5 1"<<endl<<
                    "4 4 3 3 3 3 3 3 3 4 5 3 3 2 3 4 3 4 1 4 5 3 3 1 3 4 4 2 3 3 4 5 2 2 2 3 2 2 3 2"<<endl<<
                    "2 3 2 3 2 5 2 0 3 4 1 2 2 3 3 3 3 2"<<endl<<
                    "4 4 2 5 4 4 3 2 4 3 3 4 1 4 4 4 4 1 5 4 1 3 5 3 3 3 3 2 3 2 4 3 4 5 4 3 4 3 4 3"<<endl<<
                    "5 2 2 5 5 2 4 2 0 5 2 3 4 2 2 3 4 5"<<endl<<
                    "5 4 2 5 3 4 4 4 4 5 5 3 3 3 4 3 2 2 2 5 4 3 3 4 3 3 4 3 2 3 3 2 3 5 3 2 3 3 3 3"<<endl<<
                    "2 3 3 4 3 5 3 5 4 0 1 3 3 1 3 5 5 5"<<endl<<
                    "2 2 2 3 4 2 2 2 2 5 2 2 3 3 2 2 2 2 3 1 1 1 5 1 1 4 2 1 2 2 2 2 2 1 3 1 5 2 5 2"<<endl<<
                    "3 1 2 2 2 1 5 1 3 2 0 1 2 2 2 1 2 1"<<endl<<
                    "2 2 3 3 2 2 4 2 4 2 3 4 2 3 3 2 2 3 3 2 4 4 3 3 3 4 3 2 2 2 2 2 3 3 3 2 3 2 2 4"<<endl<<
                    "2 2 3 3 3 3 3 3 3 3 2 0 3 3 2 3 4 3"<<endl<<
                    "2 2 3 5 2 5 5 1 2 1 3 2 2 4 3 4 4 1 4 5 1 1 4 1 3 1 1 5 3 5 2 2 5 4 3 1 1 3 2 1"<<endl<<
                    "4 4 3 1 1 1 1 1 4 2 1 1 0 3 1 2 3 4"<<endl<<
                    "2 3 5 3 3 5 3 2 4 3 2 4 4 2 2 3 3 1 2 5 2 5 2 1 5 2 4 3 5 3 2 1 4 2 5 3 2 4 3 4"<<endl<<
                    "2 4 4 3 3 2 2 2 3 4 2 2 4 0 3 4 5 3"<<endl<<
                    "2 2 3 2 2 3 4 2 2 3 4 2 3 3 3 2 2 4 1 5 4 3 2 2 5 1 3 1 4 2 4 1 3 3 3 2 1 2 1 1"<<endl<<
                    "1 2 2 2 2 3 4 2 2 4 1 1 2 5 0 2 2 2"<<endl<<
                    "3 1 3 4 2 5 5 3 5 3 4 5 2 4 5 5 5 1 5 3 3 3 3 3 4 2 4 1 4 4 3 3 4 4 5 5 3 1 2 4"<<endl<<
                    "4 3 4 4 2 5 3 5 3 5 1 4 3 5 4 0 5 5"<<endl<<
                    "2 1 5 5 5 3 4 1 4 1 5 2 2 3 3 4 3 1 2 5 3 2 2 1 1 1 4 1 5 5 1 1 3 2 5 5 1 2 3 2"<<endl<<
                    "1 3 2 3 4 2 5 1 3 2 1 2 3 3 1 5 0 5"<<endl<<
                    "3 2 2 5 2 4 2 3 3 3 3 4 1 3 3 3 2 3 3 2 2 2 4 4 2 2 3 1 2 3 3 2 2 5 3 2 2 2 2 2"<<endl<<
                    "4 2 2 2 2 2 2 2 5 4 1 3 3 3 3 3 3 0";

    }
    else if ( fileName == "Freeman_EIES_networks_32actors.dl" ) {
        qDebug()<< "		... to  " << fileName;
        datasetDescription ="Freeman's_EIES includes the following three 32x32 relations: \n"
        "TIME_1 non-symmetric, valued\n"
        "TIME_2 non-symmetric, valued\n"
        "NUMBER_OF_MESSAGES non-symmetric, valued\n"
        "This data comes from an early experiment on computer mediated communication. \n"
        "Fifty academics were allowed to contact each other via an "
        "Electronic Information Exchange System (EIES). "
        "The data collected consisted of all messages sent plus acquaintance "
        "relationships at two time periods.\n "
        "The data includes the 32 actors who completed the study. \n"
        "TIME_1 and TIME_2 give the acquaintance information at the beginning "
        "and end of the study. This is coded as follows: \n"
        "4 = close personal fiend, 3= friend, 2= person I've met, 1 = person I've heard of but not met, and 0 = person unknown to me (or no reply). \n"
        "NUMBER_OF MESSAGES is the total number of messages person i sent to j over the entire period of the study. ";
        outText <<"DL"<<endl<<
                  "N=32 NM=3"<<endl<<
                  "FORMAT = FULLMATRIX DIAGONAL PRESENT"<<endl<<
                  "ROW LABELS:"<<endl<<
                   "\"1\""<<endl<<
                   "\"2\""<<endl<<
                   "\"3\""<<endl<<
                   "\"6\""<<endl<<
                   "\"8\""<<endl<<
                   "\"10\""<<endl<<
                   "\"11\""<<endl<<
                   "\"13\""<<endl<<
                   "\"14\""<<endl<<
                   "\"18\""<<endl<<
                   "\"19\""<<endl<<
                   "\"20\""<<endl<<
                   "\"21\""<<endl<<
                   "\"22\""<<endl<<
                   "\"23\""<<endl<<
                   "\"24\""<<endl<<
                   "\"25\""<<endl<<
                   "\"26\""<<endl<<
                   "\"27\""<<endl<<
                   "\"32\""<<endl<<
                   "\"33\""<<endl<<
                   "\"35\""<<endl<<
                   "\"36\""<<endl<<
                   "\"37\""<<endl<<
                   "\"38\""<<endl<<
                   "\"39\""<<endl<<
                   "\"40\""<<endl<<
                   "\"41\""<<endl<<
                   "\"42\""<<endl<<
                   "\"43\""<<endl<<
                   "\"44\""<<endl<<
                   "\"45\""<<endl<<
                  "COLUMN LABELS:"<<endl<<
                   "\"1\""<<endl<<
                   "\"2\""<<endl<<
                   "\"3\""<<endl<<
                   "\"6\""<<endl<<
                   "\"8\""<<endl<<
                   "\"10\""<<endl<<
                   "\"11\""<<endl<<
                   "\"13\""<<endl<<
                   "\"14\""<<endl<<
                   "\"18\""<<endl<<
                   "\"19\""<<endl<<
                   "\"20\""<<endl<<
                   "\"21\""<<endl<<
                   "\"22\""<<endl<<
                   "\"23\""<<endl<<
                   "\"24\""<<endl<<
                   "\"25\""<<endl<<
                   "\"26\""<<endl<<
                   "\"27\""<<endl<<
                   "\"32\""<<endl<<
                   "\"33\""<<endl<<
                   "\"35\""<<endl<<
                   "\"36\""<<endl<<
                   "\"37\""<<endl<<
                   "\"38\""<<endl<<
                   "\"39\""<<endl<<
                   "\"40\""<<endl<<
                   "\"41\""<<endl<<
                   "\"42\""<<endl<<
                   "\"43\""<<endl<<
                   "\"44\""<<endl<<
                   "\"45\""<<endl<<
                  "LEVEL LABELS:"<<endl<<
                   "\"TIME_1\""<<endl<<
                   "\"TIME_2\""<<endl<<
                   "\"NUMBER_OF_MESSAGES\""<<endl<<
                  "DATA:"<<endl<<
                   "0 4 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 2 2 2 2 2 2 2 3 2 2 2 2 4 2"<<endl<<
                   "4 0 2 0 1 0 3 3 4 1 3 0 2 2 2 3 2 0 1 2 3 2 0 2 0 0 2 1 2 3 4 4"<<endl<<
                   "3 1 0 4 1 0 0 2 0 2 4 4 0 4 1 2 2 2 1 2 2 2 4 2 0 2 0 1 1 1 0 0"<<endl<<
                   "2 0 2 0 2 0 0 2 2 2 2 2 2 2 2 1 0 0 4 2 2 2 2 2 2 0 2 2 2 0 2 0"<<endl<<
                   "3 0 0 2 0 0 0 2 3 2 2 1 0 2 1 2 2 0 1 2 2 2 0 2 1 0 1 2 2 0 2 2"<<endl<<
                   "3 0 0 0 0 0 0 2 0 0 0 0 0 2 0 1 0 0 2 0 1 0 0 0 0 0 2 0 2 0 2 0"<<endl<<
                   "3 2 1 0 0 0 0 2 2 0 1 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0"<<endl<<
                   "2 2 2 2 2 0 0 0 1 0 2 0 2 2 2 2 2 0 1 2 2 1 1 2 2 0 2 0 2 2 0 0"<<endl<<
                   "3 4 0 0 2 0 0 2 0 0 1 0 2 1 0 0 0 0 0 0 1 3 0 0 0 0 3 0 0 0 0 4"<<endl<<
                   "2 1 3 3 2 0 1 2 2 0 2 3 0 1 2 2 2 0 2 3 2 2 4 2 2 0 0 2 2 2 0 0"<<endl<<
                   "1 3 2 1 1 0 0 3 1 1 0 0 0 2 1 2 2 0 1 2 2 2 1 2 2 0 2 1 1 0 1 0"<<endl<<
                   "1 0 1 2 0 0 0 1 0 3 0 0 0 2 0 1 0 0 2 2 2 2 0 0 2 0 0 0 2 2 0 0"<<endl<<
                   "3 3 1 2 1 0 3 3 2 1 1 0 0 1 1 1 0 0 2 1 1 1 1 0 0 2 4 2 2 2 3 3"<<endl<<
                   "3 2 4 2 3 0 0 3 2 1 2 3 1 0 3 4 3 2 3 3 3 4 3 3 3 2 1 2 4 3 2 0"<<endl<<
                   "3 2 2 3 1 0 1 2 2 2 2 1 0 3 0 2 2 0 2 1 2 1 2 2 2 0 0 0 3 0 2 0"<<endl<<
                   "2 2 2 1 3 0 0 3 1 0 2 0 0 3 2 0 3 0 1 2 4 3 0 3 2 0 0 0 2 0 0 0"<<endl<<
                   "3 2 3 0 2 0 0 3 2 1 2 0 0 3 2 2 0 0 1 3 3 3 0 2 0 0 0 1 1 0 2 0"<<endl<<
                   "4 1 2 0 0 0 0 0 0 0 2 0 0 2 1 0 0 0 1 0 0 0 0 1 0 2 2 1 2 2 4 0"<<endl<<
                   "2 0 2 4 1 0 0 2 0 2 0 2 0 2 2 1 0 0 0 1 2 3 2 2 2 2 0 2 2 1 2 0"<<endl<<
                   "2 2 2 2 2 0 0 2 0 3 2 2 0 3 1 2 2 0 2 0 3 4 2 3 3 0 0 2 3 1 0 0"<<endl<<
                   "3 3 2 2 2 0 0 3 1 2 3 2 0 2 3 4 3 0 2 2 0 3 2 2 3 0 1 2 2 1 0 1"<<endl<<
                   "2 2 2 3 0 0 0 2 3 2 2 0 0 3 0 3 2 0 3 3 3 0 0 4 2 0 0 2 4 0 0 0"<<endl<<
                   "2 0 4 3 0 0 0 0 0 4 0 1 0 2 1 1 0 0 2 2 2 1 0 1 2 0 0 1 2 0 0 0"<<endl<<
                   "2 2 2 2 2 0 0 3 2 2 2 2 0 3 2 3 2 0 3 3 3 4 2 0 3 0 2 2 4 0 0 0"<<endl<<
                   "2 2 2 2 1 0 0 2 0 3 2 2 0 3 2 3 0 0 2 4 3 3 3 4 0 0 0 1 0 0 0 0"<<endl<<
                   "4 1 2 1 1 0 1 1 0 1 1 1 2 2 1 1 0 3 2 1 1 2 1 2 1 0 0 2 2 0 3 0"<<endl<<
                   "2 2 1 2 1 0 0 2 2 1 1 0 4 1 1 1 1 0 1 1 1 0 0 0 0 0 0 0 0 2 0 0"<<endl<<
                   "3 2 0 3 0 0 0 0 0 1 1 0 1 2 2 2 0 0 3 2 2 3 0 2 1 2 1 0 2 0 2 0"<<endl<<
                   "2 2 2 2 2 0 0 2 0 2 0 2 0 3 2 2 0 2 2 2 2 4 2 3 0 2 2 2 0 2 2 0"<<endl<<
                   "3 4 1 0 0 0 0 4 0 2 0 0 2 2 0 2 2 2 2 2 2 2 0 0 0 0 2 1 2 0 0 2"<<endl<<
                   "4 4 2 2 2 2 1 2 2 0 2 0 2 2 2 1 2 3 2 0 1 2 2 2 0 2 2 2 2 2 0 0"<<endl<<
                   "3 3 0 1 2 0 0 3 4 0 1 0 2 1 0 1 0 0 1 1 1 0 0 0 0 0 2 0 0 3 3 0"<<endl<<
                   "0 4 2 2 2 2 2 3 3 2 3 2 3 2 2 2 2 3 2 2 2 2 2 2 2 3 2 2 3 2 4 3"<<endl<<
                   "4 0 2 2 1 2 2 3 4 2 3 0 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 4 4"<<endl<<
                   "3 1 0 4 1 0 0 2 0 2 4 4 0 4 1 2 2 2 1 2 2 2 4 2 0 2 0 1 1 1 0 0"<<endl<<
                   "2 2 2 0 2 2 0 2 2 3 2 2 1 2 2 2 0 2 4 2 2 2 2 2 2 2 2 2 2 2 2 0"<<endl<<
                   "3 0 0 2 0 0 0 2 3 2 2 1 0 2 1 2 2 0 1 2 2 2 0 2 1 0 1 2 2 0 2 2"<<endl<<
                   "4 2 0 0 0 0 0 3 0 2 2 0 0 2 2 2 0 0 2 0 2 0 0 3 0 2 2 2 3 0 4 2"<<endl<<
                   "3 2 1 0 0 0 0 2 2 0 1 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0"<<endl<<
                   "3 2 2 2 2 2 1 0 1 2 4 1 2 2 2 2 2 2 2 2 2 2 2 2 2 0 2 1 2 2 2 4"<<endl<<
                   "3 4 0 0 2 0 0 2 0 0 2 0 2 1 0 1 2 0 0 0 2 2 0 0 0 0 3 0 1 0 2 4"<<endl<<
                   "3 0 2 3 2 0 1 2 1 0 2 3 2 1 2 2 2 2 2 4 2 2 4 2 2 0 2 2 3 2 2 1"<<endl<<
                   "3 2 2 2 2 2 0 4 2 2 0 0 2 2 2 2 2 2 2 2 2 2 1 2 2 0 2 2 2 0 3 3"<<endl<<
                   "2 0 1 2 0 0 1 1 0 3 0 0 0 2 1 1 0 0 2 3 2 1 1 1 2 0 0 1 0 1 2 2"<<endl<<
                   "3 3 1 2 1 0 3 3 2 1 2 0 0 1 1 2 0 2 2 1 2 2 1 0 0 2 4 2 2 2 3 3"<<endl<<
                   "3 2 4 3 3 0 0 3 0 2 2 3 2 0 3 4 4 2 3 3 3 4 3 3 3 2 2 3 4 3 3 2"<<endl<<
                   "3 2 2 3 1 0 0 2 2 2 2 2 0 3 0 2 2 0 2 1 2 2 2 2 2 0 1 0 3 0 3 1"<<endl<<
                   "2 2 2 2 3 2 0 3 1 2 2 0 0 3 2 0 2 0 2 2 4 3 0 2 2 2 1 1 2 0 2 2"<<endl<<
                   "3 2 3 1 2 0 0 3 2 1 3 1 1 3 2 3 0 1 1 3 3 3 0 2 0 1 2 1 2 2 2 2"<<endl<<
                   "4 2 2 0 0 0 1 2 0 0 2 0 1 2 0 0 0 0 0 0 0 0 0 0 0 2 2 0 2 2 4 1"<<endl<<
                   "2 0 2 4 1 0 0 2 0 2 0 2 0 2 2 1 0 0 0 2 2 3 2 2 2 2 0 2 2 1 2 0"<<endl<<
                   "2 2 2 2 2 0 0 2 0 3 2 2 0 3 1 2 2 0 2 0 3 4 2 3 3 0 0 2 3 1 0 0"<<endl<<
                   "3 3 2 2 2 0 0 3 1 2 3 2 0 2 3 4 3 0 2 2 0 3 2 2 3 0 1 2 2 1 0 1"<<endl<<
                   "2 2 2 3 0 0 0 2 3 2 2 0 0 3 2 3 2 0 3 3 3 0 0 4 2 0 0 2 4 0 0 2"<<endl<<
                   "3 2 4 3 0 0 0 2 0 4 0 1 0 3 1 1 0 0 3 2 1 1 0 2 2 0 0 2 3 2 2 0"<<endl<<
                   "3 2 2 2 3 2 0 3 2 2 3 2 2 3 2 3 2 2 2 3 3 4 2 0 3 0 2 3 3 2 2 2"<<endl<<
                   "2 2 2 3 1 0 0 3 0 3 2 2 0 3 2 3 0 0 2 3 3 3 3 3 0 0 0 1 2 0 0 0"<<endl<<
                   "4 1 2 1 1 0 1 1 0 1 1 1 2 2 1 1 0 3 2 1 1 2 1 2 1 0 0 2 2 0 3 0"<<endl<<
                   "3 2 2 2 2 2 0 3 3 2 2 0 4 1 2 2 2 2 2 1 2 2 1 2 0 0 0 0 2 2 2 2"<<endl<<
                   "3 2 0 3 0 0 0 2 0 1 1 0 2 2 2 2 0 0 3 2 2 3 0 2 1 2 1 0 2 0 2 2"<<endl<<
                   "3 2 2 3 2 2 0 3 0 3 2 3 2 4 3 2 2 2 2 3 2 4 2 4 0 2 2 2 0 2 3 2"<<endl<<
                   "3 3 1 2 0 2 0 3 0 2 2 0 2 2 0 2 2 2 2 2 2 2 0 2 2 0 3 2 3 0 3 3"<<endl<<
                   "4 4 2 2 2 3 2 2 2 2 3 2 3 2 3 2 2 3 2 2 2 2 2 2 2 2 2 2 3 2 0 4"<<endl<<
                   "4 4 0 2 2 2 0 4 4 2 3 0 2 1 0 3 2 0 0 1 2 3 1 1 0 2 2 1 3 2 4 0"<<endl<<
                    "24 488  28  65  20  65  45 346  82  52 177  28  24  49  81  77  77  73  33  31  22  46  31 128  38  89  95  25 388  71 212 185"<<endl<<
                   "364   6  17  17  15   0  30  20  35  20  22  15  15  15  15  50  25   8   0  15  15  15  15   0  15  15  10  24  89  23 163  39"<<endl<<
                     "4   5   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0"<<endl<<
                    "52  30   0   4   0   2   0  32  21  34   9   0   0   0   0   5   4   2  35   0   0   0   0  12   0   0  12   5  20   4  19  33"<<endl<<
                    "26   4   4   4   0   4   8   4   4   4   4   4   4   4   4   4   4   4   4   0   4   8   4  14   4   0   4   0   4   7   4   4"<<endl<<
                    "72  23   0   2   0  34   0  16   0   7  15   0   0   0   8   7   6   0   0   0   0   0   0  14   0   0   7   3  34   3  22   0"<<endl<<
                    "14   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   6   0"<<endl<<
                   "239  82   5  37   3  34   5  10  12  18 164  18   0   0   0  30  53  27  20   4   0   5   4  55   0   9  34   0 146 216  88 288"<<endl<<
                    "24  25   0   2   0   0   0   8  16   0  15   0  10   0   0   0   5   0   0   0   0   0   0   0   0   0  15   0  10   0  30  44"<<endl<<
                    "43  15   0  32   0  12   0  14   0   5  25   2   0   0   0  10  10   0  20  15   0   5  20  29   0   4  10   0  47   6  22  19"<<endl<<
                   "178  36   0  11   0  19  10 172  39  28  29   0   4   0   0  23  15  24   0   0   8   0   0  29  10  11  22   0  46   0 119  34"<<endl<<
                     "0   5   0   0   0   0   0   5   0   0   0   3   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0  53   0   5   9"<<endl<<
                     "5   0   0   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   5   0"<<endl<<
                    "12   0   9   0   0   0   0   0   0   0   0   0   0   2   0  12   0   0   5   0   0   0   0   0   0   0   0   0  35   0   8   0"<<endl<<
                   "120   0   0   0   0   4   0   0   0   0   0   5   0   0  78   0   0   0   0   0   0   0   0   0   0   0   8   0  58   0  32   0"<<endl<<
                    "58  25   0  10   0   0   0  20   0   5  10   0   0   5   0  15  10   0   0   0   5   0   0   5   0   0   0   0  35   0  10   0"<<endl<<
                    "63  18   9   7   0   6   0  36   0   5   9   5   0   5   0   5   0   0   0   5   2   0   0   0   0   0  15   0  10   9  15   9"<<endl<<
                    "58   8   5   4   0   0   0   4   0   5  18   0   0   0   0   0   0   4   0   0   0   0   0   0   0   0  20   0   8  10  48   0"<<endl<<
                     "5   5   0  25   0   0   0  10   0   0   0   0   0   5   0   0   0   0   5   0   0   0   5   0   0   0   0   0   0   0  10   0"<<endl<<
                     "0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   4   0   0   0   0   0   0   0   4   0   0   0"<<endl<<
                     "9   0   0   0   0   0   0   0   0   0   3   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   5   0   0   0"<<endl<<
                    "10   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0  40   0   0   0   0  15   0   0   5"<<endl<<
                     "5   5   5   0   0   0   0   0   0  19   0   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0  14   0   5   0"<<endl<<
                    "89  17   4  14  14  18   8  41   4  19  31   4   4   9   4  14   4   9   4   4   4  58   4   5  18  14   9   4 156   4  56  10"<<endl<<
                    "32   5   0   0   0   0   0   0   0   0   0   0   0  15   0   0   0   0   0   0   0  10   0  23  10   0   0   0   0   9  15   0"<<endl<<
                    "35   5   0   0   0   0   0   0   0   5   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0  10   0  13   0"<<endl<<
                    "50  28   0  13   0   0   0  19  29   5   8   0  33   0   4   0  10  15   0   0   0   0   0  10   0   0   0   3  32   0  13  33"<<endl<<
                     "9   6   0   0   0   3   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   3   0   0   0   6"<<endl<<
                   "559 132   5  24  21  29   0 155  15  98  69  89  37  76  80  63  15   4   9  18  43 108  29 218   0  15  66   0   6  14  91 126"<<endl<<
                    "39  21   0   6   3   3   0 140   0   7   0   2   0   0   0   0   9   5   0   0   0   0   0   0   0   0   2   0  18   2  20   8"<<endl<<
                    "82 125  10  22  10  15  18  70  35  23 114  20  16  15  24  30  28  49  30   5   5  15   8  53  25   8  21   8  65  28   0  67"<<endl<<
                   "239  99   0  27   3   0   0 268 101  18  35   4   0   0   0   0   7   0   0   0   0  14   0   5   0   0  50   6  71   7 107 219";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time-1.dl" ) {
        qDebug()<< "		... to  " << fileName;
        outText <<
                   "dl"<<endl<<
                   "N=48"<<endl<<
                   "format=edgelist1"<<endl<<
                   "data:"<<endl<<
                   "1 2 4"<<endl<<
                   "1 3 2"<<endl<<
                   "1 6 2"<<endl<<
                   "1 8 2"<<endl<<
                   "1 10 2"<<endl<<
                   "1 11 2"<<endl<<
                   "1 13 2"<<endl<<
                   "1 14 2"<<endl<<
                   "1 18 2"<<endl<<
                   "1 19 2"<<endl<<
                   "1 20 2"<<endl<<
                   "1 21 2"<<endl<<
                   "1 22 2"<<endl<<
                   "1 23 2"<<endl<<
                   "1 24 2"<<endl<<
                   "1 25 2"<<endl<<
                   "1 26 3"<<endl<<
                   "1 27 2"<<endl<<
                   "1 31 2"<<endl<<
                   "1 32 2"<<endl<<
                   "1 33 2"<<endl<<
                   "1 35 2"<<endl<<
                   "1 36 2"<<endl<<
                   "1 37 2"<<endl<<
                   "1 38 2"<<endl<<
                   "1 39 3"<<endl<<
                   "1 40 2"<<endl<<
                   "1 41 2"<<endl<<
                   "1 42 2"<<endl<<
                   "1 43 2"<<endl<<
                   "1 44 4"<<endl<<
                   "1 45 2"<<endl<<
                   "1 46 2"<<endl<<
                   "2 1 4"<<endl<<
                   "2 3 2"<<endl<<
                   "2 8 1"<<endl<<
                   "2 11 3"<<endl<<
                   "2 13 3"<<endl<<
                   "2 14 4"<<endl<<
                   "2 18 1"<<endl<<
                   "2 19 3"<<endl<<
                   "2 21 2"<<endl<<
                   "2 22 2"<<endl<<
                   "2 23 2"<<endl<<
                   "2 24 3"<<endl<<
                   "2 25 2"<<endl<<
                   "2 27 1"<<endl<<
                   "2 32 2"<<endl<<
                   "2 33 3"<<endl<<
                   "2 35 2"<<endl<<
                   "2 37 2"<<endl<<
                   "2 40 2"<<endl<<
                   "2 41 1"<<endl<<
                   "2 42 2"<<endl<<
                   "2 43 3"<<endl<<
                   "2 44 4"<<endl<<
                   "2 45 4"<<endl<<
                   "2 46 2"<<endl<<
                   "3 1 3"<<endl<<
                   "3 2 1"<<endl<<
                   "3 6 4"<<endl<<
                   "3 8 1"<<endl<<
                   "3 13 2"<<endl<<
                   "3 18 2"<<endl<<
                   "3 19 4"<<endl<<
                   "3 20 4"<<endl<<
                   "3 22 4"<<endl<<
                   "3 23 1"<<endl<<
                   "3 24 2"<<endl<<
                   "3 25 2"<<endl<<
                   "3 26 2"<<endl<<
                   "3 27 1"<<endl<<
                   "3 31 1"<<endl<<
                   "3 32 2"<<endl<<
                   "3 33 2"<<endl<<
                   "3 35 2"<<endl<<
                   "3 36 4"<<endl<<
                   "3 37 2"<<endl<<
                   "3 39 2"<<endl<<
                   "3 41 1"<<endl<<
                   "3 42 1"<<endl<<
                   "3 43 1"<<endl<<
                   "6 1 2"<<endl<<
                   "6 3 2"<<endl<<
                   "6 8 2"<<endl<<
                   "6 13 2"<<endl<<
                   "6 14 2"<<endl<<
                   "6 18 2"<<endl<<
                   "6 19 2"<<endl<<
                   "6 20 2"<<endl<<
                   "6 21 2"<<endl<<
                   "6 22 2"<<endl<<
                   "6 23 2"<<endl<<
                   "6 24 1"<<endl<<
                   "6 27 4"<<endl<<
                   "6 31 1"<<endl<<
                   "6 32 2"<<endl<<
                   "6 33 2"<<endl<<
                   "6 35 2"<<endl<<
                   "6 36 2"<<endl<<
                   "6 37 2"<<endl<<
                   "6 38 2"<<endl<<
                   "6 40 2"<<endl<<
                   "6 41 2"<<endl<<
                   "6 42 2"<<endl<<
                   "6 44 2"<<endl<<
                   "8 1 3"<<endl<<
                   "8 6 2"<<endl<<
                   "8 13 2"<<endl<<
                   "8 14 3"<<endl<<
                   "8 18 2"<<endl<<
                   "8 19 2"<<endl<<
                   "8 20 1"<<endl<<
                   "8 22 2"<<endl<<
                   "8 23 1"<<endl<<
                   "8 24 2"<<endl<<
                   "8 25 2"<<endl<<
                   "8 27 1"<<endl<<
                   "8 32 2"<<endl<<
                   "8 33 2"<<endl<<
                   "8 35 2"<<endl<<
                   "8 37 2"<<endl<<
                   "8 38 1"<<endl<<
                   "8 40 1"<<endl<<
                   "8 41 2"<<endl<<
                   "8 42 2"<<endl<<
                   "8 44 2"<<endl<<
                   "8 45 2"<<endl<<
                   "10 1 3"<<endl<<
                   "10 13 2"<<endl<<
                   "10 22 2"<<endl<<
                   "10 24 1"<<endl<<
                   "10 27 2"<<endl<<
                   "10 33 1"<<endl<<
                   "10 40 2"<<endl<<
                   "10 42 2"<<endl<<
                   "10 44 2"<<endl<<
                   "11 1 3"<<endl<<
                   "11 2 2"<<endl<<
                   "11 3 1"<<endl<<
                   "11 13 2"<<endl<<
                   "11 14 2"<<endl<<
                   "11 19 1"<<endl<<
                   "11 21 3"<<endl<<
                   "11 41 2"<<endl<<
                   "13 1 2"<<endl<<
                   "13 2 2"<<endl<<
                   "13 3 2"<<endl<<
                   "13 6 2"<<endl<<
                   "13 8 2"<<endl<<
                   "13 14 1"<<endl<<
                   "13 19 2"<<endl<<
                   "13 21 2"<<endl<<
                   "13 22 2"<<endl<<
                   "13 23 2"<<endl<<
                   "13 24 2"<<endl<<
                   "13 25 2"<<endl<<
                   "13 27 1"<<endl<<
                   "13 32 2"<<endl<<
                   "13 33 2"<<endl<<
                   "13 35 1"<<endl<<
                   "13 36 1"<<endl<<
                   "13 37 2"<<endl<<
                   "13 38 2"<<endl<<
                   "13 40 2"<<endl<<
                   "13 42 2"<<endl<<
                   "13 43 2"<<endl<<
                   "14 1 3"<<endl<<
                   "14 2 4"<<endl<<
                   "14 8 2"<<endl<<
                   "14 13 2"<<endl<<
                   "14 19 1"<<endl<<
                   "14 21 2"<<endl<<
                   "14 22 1"<<endl<<
                   "14 33 1"<<endl<<
                   "14 35 3"<<endl<<
                   "14 40 3"<<endl<<
                   "14 45 4"<<endl<<
                   "18 1 2"<<endl<<
                   "18 2 1"<<endl<<
                   "18 3 3"<<endl<<
                   "18 6 3"<<endl<<
                   "18 8 2"<<endl<<
                   "18 11 1"<<endl<<
                   "18 13 2"<<endl<<
                   "18 14 2"<<endl<<
                   "18 19 2"<<endl<<
                   "18 20 3"<<endl<<
                   "18 22 1"<<endl<<
                   "18 23 2"<<endl<<
                   "18 24 2"<<endl<<
                   "18 25 2"<<endl<<
                   "18 27 2"<<endl<<
                   "18 31 2"<<endl<<
                   "18 32 3"<<endl<<
                   "18 33 2"<<endl<<
                   "18 35 2"<<endl<<
                   "18 36 4"<<endl<<
                   "18 37 2"<<endl<<
                   "18 38 2"<<endl<<
                   "18 41 2"<<endl<<
                   "18 42 2"<<endl<<
                   "18 43 2"<<endl<<
                   "19 1 1"<<endl<<
                   "19 2 3"<<endl<<
                   "19 3 2"<<endl<<
                   "19 6 1"<<endl<<
                   "19 8 1"<<endl<<
                   "19 13 3"<<endl<<
                   "19 14 1"<<endl<<
                   "19 18 1"<<endl<<
                   "19 22 2"<<endl<<
                   "19 23 1"<<endl<<
                   "19 24 2"<<endl<<
                   "19 25 2"<<endl<<
                   "19 27 1"<<endl<<
                   "19 31 2"<<endl<<
                   "19 32 2"<<endl<<
                   "19 33 2"<<endl<<
                   "19 35 2"<<endl<<
                   "19 36 1"<<endl<<
                   "19 37 2"<<endl<<
                   "19 38 2"<<endl<<
                   "19 40 2"<<endl<<
                   "19 41 1"<<endl<<
                   "19 42 1"<<endl<<
                   "19 44 1"<<endl<<
                   "20 1 1"<<endl<<
                   "20 3 1"<<endl<<
                   "20 6 2"<<endl<<
                   "20 13 1"<<endl<<
                   "20 18 3"<<endl<<
                   "20 22 2"<<endl<<
                   "20 24 1"<<endl<<
                   "20 27 2"<<endl<<
                   "20 32 2"<<endl<<
                   "20 33 2"<<endl<<
                   "20 35 2"<<endl<<
                   "20 38 2"<<endl<<
                   "20 42 2"<<endl<<
                   "20 43 2"<<endl<<
                   "21 1 3"<<endl<<
                   "21 2 3"<<endl<<
                   "21 3 1"<<endl<<
                   "21 6 2"<<endl<<
                   "21 8 1"<<endl<<
                   "21 11 3"<<endl<<
                   "21 13 3"<<endl<<
                   "21 14 2"<<endl<<
                   "21 18 1"<<endl<<
                   "21 19 1"<<endl<<
                   "21 22 1"<<endl<<
                   "21 23 1"<<endl<<
                   "21 24 1"<<endl<<
                   "21 27 2"<<endl<<
                   "21 31 1"<<endl<<
                   "21 32 1"<<endl<<
                   "21 33 1"<<endl<<
                   "21 35 1"<<endl<<
                   "21 36 1"<<endl<<
                   "21 39 2"<<endl<<
                   "21 40 4"<<endl<<
                   "21 41 2"<<endl<<
                   "21 42 2"<<endl<<
                   "21 43 2"<<endl<<
                   "21 44 3"<<endl<<
                   "21 45 3"<<endl<<
                   "22 1 3"<<endl<<
                   "22 2 2"<<endl<<
                   "22 3 4"<<endl<<
                   "22 6 2"<<endl<<
                   "22 8 3"<<endl<<
                   "22 13 3"<<endl<<
                   "22 14 2"<<endl<<
                   "22 18 1"<<endl<<
                   "22 19 2"<<endl<<
                   "22 20 3"<<endl<<
                   "22 21 1"<<endl<<
                   "22 23 3"<<endl<<
                   "22 24 4"<<endl<<
                   "22 25 3"<<endl<<
                   "22 26 2"<<endl<<
                   "22 27 3"<<endl<<
                   "22 31 2"<<endl<<
                   "22 32 3"<<endl<<
                   "22 33 3"<<endl<<
                   "22 35 4"<<endl<<
                   "22 36 3"<<endl<<
                   "22 37 3"<<endl<<
                   "22 38 3"<<endl<<
                   "22 39 2"<<endl<<
                   "22 40 1"<<endl<<
                   "22 41 2"<<endl<<
                   "22 42 4"<<endl<<
                   "22 43 3"<<endl<<
                   "22 44 2"<<endl<<
                   "22 46 1"<<endl<<
                   "23 1 3"<<endl<<
                   "23 2 2"<<endl<<
                   "23 3 2"<<endl<<
                   "23 6 3"<<endl<<
                   "23 8 1"<<endl<<
                   "23 11 1"<<endl<<
                   "23 13 2"<<endl<<
                   "23 14 2"<<endl<<
                   "23 18 2"<<endl<<
                   "23 19 2"<<endl<<
                   "23 20 1"<<endl<<
                   "23 22 3"<<endl<<
                   "23 24 2"<<endl<<
                   "23 25 2"<<endl<<
                   "23 27 2"<<endl<<
                   "23 31 4"<<endl<<
                   "23 32 1"<<endl<<
                   "23 33 2"<<endl<<
                   "23 35 1"<<endl<<
                   "23 36 2"<<endl<<
                   "23 37 2"<<endl<<
                   "23 38 2"<<endl<<
                   "23 42 3"<<endl<<
                   "23 44 2"<<endl<<
                   "23 46 1"<<endl<<
                   "24 1 2"<<endl<<
                   "24 2 2"<<endl<<
                   "24 3 2"<<endl<<
                   "24 6 1"<<endl<<
                   "24 8 3"<<endl<<
                   "24 13 3"<<endl<<
                   "24 14 1"<<endl<<
                   "24 19 2"<<endl<<
                   "24 22 3"<<endl<<
                   "24 23 2"<<endl<<
                   "24 25 3"<<endl<<
                   "24 27 1"<<endl<<
                   "24 31 2"<<endl<<
                   "24 32 2"<<endl<<
                   "24 33 4"<<endl<<
                   "24 35 3"<<endl<<
                   "24 37 3"<<endl<<
                   "24 38 2"<<endl<<
                   "24 42 2"<<endl<<
                   "25 1 3"<<endl<<
                   "25 2 2"<<endl<<
                   "25 3 3"<<endl<<
                   "25 8 2"<<endl<<
                   "25 13 3"<<endl<<
                   "25 14 2"<<endl<<
                   "25 18 1"<<endl<<
                   "25 19 2"<<endl<<
                   "25 22 3"<<endl<<
                   "25 23 2"<<endl<<
                   "25 24 2"<<endl<<
                   "25 27 1"<<endl<<
                   "25 32 3"<<endl<<
                   "25 33 3"<<endl<<
                   "25 35 3"<<endl<<
                   "25 37 2"<<endl<<
                   "25 41 1"<<endl<<
                   "25 42 1"<<endl<<
                   "25 44 2"<<endl<<
                   "25 46 1"<<endl<<
                   "26 1 4"<<endl<<
                   "26 2 1"<<endl<<
                   "26 3 2"<<endl<<
                   "26 19 2"<<endl<<
                   "26 22 2"<<endl<<
                   "26 23 1"<<endl<<
                   "26 27 1"<<endl<<
                   "26 37 1"<<endl<<
                   "26 39 2"<<endl<<
                   "26 40 2"<<endl<<
                   "26 41 1"<<endl<<
                   "26 42 2"<<endl<<
                   "26 43 2"<<endl<<
                   "26 44 4"<<endl<<
                   "26 46 2"<<endl<<
                   "27 1 2"<<endl<<
                   "27 3 2"<<endl<<
                   "27 6 4"<<endl<<
                   "27 8 1"<<endl<<
                   "27 13 2"<<endl<<
                   "27 18 2"<<endl<<
                   "27 20 2"<<endl<<
                   "27 22 2"<<endl<<
                   "27 23 2"<<endl<<
                   "27 24 1"<<endl<<
                   "27 32 1"<<endl<<
                   "27 33 2"<<endl<<
                   "27 35 3"<<endl<<
                   "27 36 2"<<endl<<
                   "27 37 2"<<endl<<
                   "27 38 2"<<endl<<
                   "27 39 2"<<endl<<
                   "27 41 2"<<endl<<
                   "27 42 2"<<endl<<
                   "27 43 1"<<endl<<
                   "27 44 2"<<endl<<
                   "27 46 2"<<endl<<
                   "31 1 1"<<endl<<
                   "31 3 2"<<endl<<
                   "31 6 1"<<endl<<
                   "31 8 1"<<endl<<
                   "31 18 2"<<endl<<
                   "31 19 2"<<endl<<
                   "31 20 2"<<endl<<
                   "31 22 2"<<endl<<
                   "31 23 2"<<endl<<
                   "31 24 2"<<endl<<
                   "31 32 1"<<endl<<
                   "31 35 3"<<endl<<
                   "31 36 1"<<endl<<
                   "31 37 3"<<endl<<
                   "31 38 2"<<endl<<
                   "31 42 1"<<endl<<
                   "32 1 2"<<endl<<
                   "32 2 2"<<endl<<
                   "32 3 2"<<endl<<
                   "32 6 2"<<endl<<
                   "32 8 2"<<endl<<
                   "32 13 2"<<endl<<
                   "32 18 3"<<endl<<
                   "32 19 2"<<endl<<
                   "32 20 2"<<endl<<
                   "32 22 3"<<endl<<
                   "32 23 1"<<endl<<
                   "32 24 2"<<endl<<
                   "32 25 2"<<endl<<
                   "32 27 2"<<endl<<
                   "32 31 1"<<endl<<
                   "32 33 3"<<endl<<
                   "32 35 4"<<endl<<
                   "32 36 2"<<endl<<
                   "32 37 3"<<endl<<
                   "32 38 3"<<endl<<
                   "32 41 2"<<endl<<
                   "32 42 3"<<endl<<
                   "32 43 1"<<endl<<
                   "33 1 3"<<endl<<
                   "33 2 3"<<endl<<
                   "33 3 2"<<endl<<
                   "33 6 2"<<endl<<
                   "33 8 2"<<endl<<
                   "33 13 3"<<endl<<
                   "33 14 1"<<endl<<
                   "33 18 2"<<endl<<
                   "33 19 3"<<endl<<
                   "33 20 2"<<endl<<
                   "33 22 2"<<endl<<
                   "33 23 3"<<endl<<
                   "33 24 4"<<endl<<
                   "33 25 3"<<endl<<
                   "33 27 2"<<endl<<
                   "33 31 2"<<endl<<
                   "33 32 2"<<endl<<
                   "33 35 3"<<endl<<
                   "33 36 2"<<endl<<
                   "33 37 2"<<endl<<
                   "33 38 3"<<endl<<
                   "33 40 1"<<endl<<
                   "33 41 2"<<endl<<
                   "33 42 2"<<endl<<
                   "33 43 1"<<endl<<
                   "33 45 1"<<endl<<
                   "35 1 2"<<endl<<
                   "35 2 2"<<endl<<
                   "35 3 2"<<endl<<
                   "35 6 3"<<endl<<
                   "35 13 2"<<endl<<
                   "35 14 3"<<endl<<
                   "35 18 2"<<endl<<
                   "35 19 2"<<endl<<
                   "35 22 3"<<endl<<
                   "35 24 3"<<endl<<
                   "35 25 2"<<endl<<
                   "35 27 3"<<endl<<
                   "35 32 3"<<endl<<
                   "35 33 3"<<endl<<
                   "35 37 4"<<endl<<
                   "35 38 2"<<endl<<
                   "35 41 2"<<endl<<
                   "35 42 4"<<endl<<
                   "36 1 2"<<endl<<
                   "36 3 4"<<endl<<
                   "36 6 3"<<endl<<
                   "36 18 4"<<endl<<
                   "36 20 1"<<endl<<
                   "36 22 2"<<endl<<
                   "36 23 1"<<endl<<
                   "36 24 1"<<endl<<
                   "36 27 2"<<endl<<
                   "36 31 1"<<endl<<
                   "36 32 2"<<endl<<
                   "36 33 2"<<endl<<
                   "36 35 1"<<endl<<
                   "36 37 1"<<endl<<
                   "36 38 2"<<endl<<
                   "36 41 1"<<endl<<
                   "36 42 2"<<endl<<
                   "37 1 2"<<endl<<
                   "37 2 2"<<endl<<
                   "37 3 2"<<endl<<
                   "37 6 2"<<endl<<
                   "37 8 2"<<endl<<
                   "37 13 3"<<endl<<
                   "37 14 2"<<endl<<
                   "37 18 2"<<endl<<
                   "37 19 2"<<endl<<
                   "37 20 2"<<endl<<
                   "37 22 3"<<endl<<
                   "37 23 2"<<endl<<
                   "37 24 3"<<endl<<
                   "37 25 2"<<endl<<
                   "37 27 3"<<endl<<
                   "37 31 4"<<endl<<
                   "37 32 3"<<endl<<
                   "37 33 3"<<endl<<
                   "37 35 4"<<endl<<
                   "37 36 2"<<endl<<
                   "37 38 3"<<endl<<
                   "37 40 2"<<endl<<
                   "37 41 2"<<endl<<
                   "37 42 4"<<endl<<
                   "38 1 2"<<endl<<
                   "38 2 2"<<endl<<
                   "38 3 2"<<endl<<
                   "38 6 2"<<endl<<
                   "38 8 1"<<endl<<
                   "38 13 2"<<endl<<
                   "38 18 3"<<endl<<
                   "38 19 2"<<endl<<
                   "38 20 2"<<endl<<
                   "38 22 3"<<endl<<
                   "38 23 2"<<endl<<
                   "38 24 3"<<endl<<
                   "38 27 2"<<endl<<
                   "38 31 2"<<endl<<
                   "38 32 4"<<endl<<
                   "38 33 3"<<endl<<
                   "38 35 3"<<endl<<
                   "38 36 3"<<endl<<
                   "38 37 4"<<endl<<
                   "38 41 1"<<endl<<
                   "39 1 4"<<endl<<
                   "39 2 1"<<endl<<
                   "39 3 2"<<endl<<
                   "39 6 1"<<endl<<
                   "39 8 1"<<endl<<
                   "39 11 1"<<endl<<
                   "39 13 1"<<endl<<
                   "39 18 1"<<endl<<
                   "39 19 1"<<endl<<
                   "39 20 1"<<endl<<
                   "39 21 2"<<endl<<
                   "39 22 2"<<endl<<
                   "39 23 1"<<endl<<
                   "39 24 1"<<endl<<
                   "39 26 3"<<endl<<
                   "39 27 2"<<endl<<
                   "39 32 1"<<endl<<
                   "39 33 1"<<endl<<
                   "39 35 2"<<endl<<
                   "39 36 1"<<endl<<
                   "39 37 2"<<endl<<
                   "39 38 1"<<endl<<
                   "39 41 2"<<endl<<
                   "39 42 2"<<endl<<
                   "39 44 3"<<endl<<
                   "39 46 1"<<endl<<
                   "40 1 2"<<endl<<
                   "40 2 2"<<endl<<
                   "40 3 1"<<endl<<
                   "40 6 2"<<endl<<
                   "40 8 1"<<endl<<
                   "40 13 2"<<endl<<
                   "40 14 2"<<endl<<
                   "40 18 1"<<endl<<
                   "40 19 1"<<endl<<
                   "40 21 4"<<endl<<
                   "40 22 1"<<endl<<
                   "40 23 1"<<endl<<
                   "40 24 1"<<endl<<
                   "40 25 1"<<endl<<
                   "40 27 1"<<endl<<
                   "40 32 1"<<endl<<
                   "40 33 1"<<endl<<
                   "40 43 2"<<endl<<
                   "41 1 3"<<endl<<
                   "41 2 2"<<endl<<
                   "41 6 3"<<endl<<
                   "41 18 1"<<endl<<
                   "41 19 1"<<endl<<
                   "41 21 1"<<endl<<
                   "41 22 2"<<endl<<
                   "41 23 2"<<endl<<
                   "41 24 2"<<endl<<
                   "41 27 3"<<endl<<
                   "41 32 2"<<endl<<
                   "41 33 2"<<endl<<
                   "41 35 3"<<endl<<
                   "41 37 2"<<endl<<
                   "41 38 1"<<endl<<
                   "41 39 2"<<endl<<
                   "41 40 1"<<endl<<
                   "41 42 2"<<endl<<
                   "41 44 2"<<endl<<
                   "42 1 2"<<endl<<
                   "42 2 2"<<endl<<
                   "42 3 2"<<endl<<
                   "42 6 2"<<endl<<
                   "42 8 2"<<endl<<
                   "42 13 2"<<endl<<
                   "42 18 2"<<endl<<
                   "42 20 2"<<endl<<
                   "42 22 3"<<endl<<
                   "42 23 2"<<endl<<
                   "42 24 2"<<endl<<
                   "42 26 2"<<endl<<
                   "42 27 2"<<endl<<
                   "42 32 2"<<endl<<
                   "42 33 2"<<endl<<
                   "42 35 4"<<endl<<
                   "42 36 2"<<endl<<
                   "42 37 3"<<endl<<
                   "42 39 2"<<endl<<
                   "42 40 2"<<endl<<
                   "42 41 2"<<endl<<
                   "42 43 2"<<endl<<
                   "42 44 2"<<endl<<
                   "42 46 3"<<endl<<
                   "43 1 3"<<endl<<
                   "43 2 4"<<endl<<
                   "43 3 1"<<endl<<
                   "43 13 4"<<endl<<
                   "43 18 2"<<endl<<
                   "43 21 2"<<endl<<
                   "43 22 2"<<endl<<
                   "43 24 2"<<endl<<
                   "43 25 2"<<endl<<
                   "43 26 2"<<endl<<
                   "43 27 2"<<endl<<
                   "43 32 2"<<endl<<
                   "43 33 2"<<endl<<
                   "43 35 2"<<endl<<
                   "43 40 2"<<endl<<
                   "43 41 1"<<endl<<
                   "43 42 2"<<endl<<
                   "43 45 2"<<endl<<
                   "43 46 1"<<endl<<
                   "44 1 4"<<endl<<
                   "44 2 4"<<endl<<
                   "44 3 2"<<endl<<
                   "44 6 2"<<endl<<
                   "44 8 2"<<endl<<
                   "44 10 2"<<endl<<
                   "44 11 1"<<endl<<
                   "44 13 2"<<endl<<
                   "44 14 2"<<endl<<
                   "44 19 2"<<endl<<
                   "44 21 2"<<endl<<
                   "44 22 2"<<endl<<
                   "44 23 2"<<endl<<
                   "44 24 1"<<endl<<
                   "44 25 2"<<endl<<
                   "44 26 3"<<endl<<
                   "44 27 2"<<endl<<
                   "44 33 1"<<endl<<
                   "44 35 2"<<endl<<
                   "44 36 2"<<endl<<
                   "44 37 2"<<endl<<
                   "44 39 2"<<endl<<
                   "44 40 2"<<endl<<
                   "44 41 2"<<endl<<
                   "44 42 2"<<endl<<
                   "44 43 2"<<endl<<
                   "44 46 1"<<endl<<
                   "45 1 3"<<endl<<
                   "45 2 3"<<endl<<
                   "45 6 1"<<endl<<
                   "45 8 2"<<endl<<
                   "45 13 3"<<endl<<
                   "45 14 4"<<endl<<
                   "45 19 1"<<endl<<
                   "45 21 2"<<endl<<
                   "45 22 1"<<endl<<
                   "45 24 1"<<endl<<
                   "45 27 1"<<endl<<
                   "45 32 1"<<endl<<
                   "45 33 1"<<endl<<
                   "45 40 2"<<endl<<
                   "45 43 3"<<endl<<
                   "45 44 3"<<endl<<
                   "45 46 1"<<endl<<
                   "46 1 2"<<endl<<
                   "46 2 2"<<endl<<
                   "46 42 3";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Acquaintanceship_at_time-2.dl" ) {
        qDebug()<< "		... to  " << fileName;
        outText <<
                   "dl"<<endl<<
                   "N=48"<<endl<<
                   "format=edgelist1"<<endl<<
                   "data:"<<endl<<
                   "1 2 4"<<endl<<
                   "1 3 2"<<endl<<
                   "1 6 2"<<endl<<
                   "1 8 2"<<endl<<
                   "1 10 2"<<endl<<
                   "1 11 2"<<endl<<
                   "1 13 3"<<endl<<
                   "1 14 3"<<endl<<
                   "1 18 2"<<endl<<
                   "1 19 3"<<endl<<
                   "1 20 2"<<endl<<
                   "1 21 3"<<endl<<
                   "1 22 2"<<endl<<
                   "1 23 2"<<endl<<
                   "1 24 2"<<endl<<
                   "1 25 2"<<endl<<
                   "1 26 3"<<endl<<
                   "1 27 2"<<endl<<
                   "1 31 2"<<endl<<
                   "1 32 2"<<endl<<
                   "1 33 2"<<endl<<
                   "1 35 2"<<endl<<
                   "1 36 2"<<endl<<
                   "1 37 2"<<endl<<
                   "1 38 2"<<endl<<
                   "1 39 3"<<endl<<
                   "1 40 2"<<endl<<
                   "1 41 2"<<endl<<
                   "1 42 3"<<endl<<
                   "1 43 2"<<endl<<
                   "1 44 4"<<endl<<
                   "1 45 3"<<endl<<
                   "1 46 3"<<endl<<
                   "2 1 4"<<endl<<
                   "2 3 2"<<endl<<
                   "2 6 2"<<endl<<
                   "2 8 1"<<endl<<
                   "2 10 2"<<endl<<
                   "2 11 2"<<endl<<
                   "2 13 3"<<endl<<
                   "2 14 4"<<endl<<
                   "2 18 2"<<endl<<
                   "2 19 3"<<endl<<
                   "2 21 2"<<endl<<
                   "2 22 2"<<endl<<
                   "2 23 2"<<endl<<
                   "2 24 2"<<endl<<
                   "2 25 2"<<endl<<
                   "2 26 2"<<endl<<
                   "2 27 2"<<endl<<
                   "2 32 2"<<endl<<
                   "2 33 2"<<endl<<
                   "2 35 2"<<endl<<
                   "2 36 2"<<endl<<
                   "2 37 2"<<endl<<
                   "2 38 2"<<endl<<
                   "2 39 2"<<endl<<
                   "2 40 2"<<endl<<
                   "2 41 2"<<endl<<
                   "2 42 2"<<endl<<
                   "2 43 3"<<endl<<
                   "2 44 4"<<endl<<
                   "2 45 4"<<endl<<
                   "2 46 2"<<endl<<
                   "3 1 3"<<endl<<
                   "3 2 1"<<endl<<
                   "3 6 4"<<endl<<
                   "3 8 1"<<endl<<
                   "3 13 2"<<endl<<
                   "3 18 2"<<endl<<
                   "3 19 4"<<endl<<
                   "3 20 4"<<endl<<
                   "3 22 4"<<endl<<
                   "3 23 1"<<endl<<
                   "3 24 2"<<endl<<
                   "3 25 2"<<endl<<
                   "3 26 2"<<endl<<
                   "3 27 1"<<endl<<
                   "3 31 1"<<endl<<
                   "3 32 2"<<endl<<
                   "3 33 2"<<endl<<
                   "3 35 2"<<endl<<
                   "3 36 4"<<endl<<
                   "3 37 2"<<endl<<
                   "3 39 2"<<endl<<
                   "3 41 1"<<endl<<
                   "3 42 1"<<endl<<
                   "3 43 1"<<endl<<
                   "6 1 2"<<endl<<
                   "6 2 2"<<endl<<
                   "6 3 2"<<endl<<
                   "6 8 2"<<endl<<
                   "6 10 2"<<endl<<
                   "6 13 2"<<endl<<
                   "6 14 2"<<endl<<
                   "6 18 3"<<endl<<
                   "6 19 2"<<endl<<
                   "6 20 2"<<endl<<
                   "6 21 1"<<endl<<
                   "6 22 2"<<endl<<
                   "6 23 2"<<endl<<
                   "6 24 2"<<endl<<
                   "6 26 2"<<endl<<
                   "6 27 4"<<endl<<
                   "6 31 1"<<endl<<
                   "6 32 2"<<endl<<
                   "6 33 2"<<endl<<
                   "6 35 2"<<endl<<
                   "6 36 2"<<endl<<
                   "6 37 2"<<endl<<
                   "6 38 2"<<endl<<
                   "6 39 2"<<endl<<
                   "6 40 2"<<endl<<
                   "6 41 2"<<endl<<
                   "6 42 2"<<endl<<
                   "6 43 2"<<endl<<
                   "6 44 2"<<endl<<
                   "6 46 2"<<endl<<
                   "8 1 3"<<endl<<
                   "8 6 2"<<endl<<
                   "8 13 2"<<endl<<
                   "8 14 3"<<endl<<
                   "8 18 2"<<endl<<
                   "8 19 2"<<endl<<
                   "8 20 1"<<endl<<
                   "8 22 2"<<endl<<
                   "8 23 1"<<endl<<
                   "8 24 2"<<endl<<
                   "8 25 2"<<endl<<
                   "8 27 1"<<endl<<
                   "8 32 2"<<endl<<
                   "8 33 2"<<endl<<
                   "8 35 2"<<endl<<
                   "8 37 2"<<endl<<
                   "8 38 1"<<endl<<
                   "8 40 1"<<endl<<
                   "8 41 2"<<endl<<
                   "8 42 2"<<endl<<
                   "8 44 2"<<endl<<
                   "8 45 2"<<endl<<
                   "10 1 4"<<endl<<
                   "10 2 2"<<endl<<
                   "10 13 3"<<endl<<
                   "10 18 2"<<endl<<
                   "10 19 2"<<endl<<
                   "10 22 2"<<endl<<
                   "10 23 2"<<endl<<
                   "10 24 2"<<endl<<
                   "10 27 2"<<endl<<
                   "10 31 2"<<endl<<
                   "10 33 2"<<endl<<
                   "10 37 3"<<endl<<
                   "10 39 2"<<endl<<
                   "10 40 2"<<endl<<
                   "10 41 2"<<endl<<
                   "10 42 3"<<endl<<
                   "10 44 4"<<endl<<
                   "10 45 2"<<endl<<
                   "10 46 3"<<endl<<
                   "11 1 3"<<endl<<
                   "11 2 2"<<endl<<
                   "11 3 1"<<endl<<
                   "11 13 2"<<endl<<
                   "11 14 2"<<endl<<
                   "11 19 1"<<endl<<
                   "11 21 3"<<endl<<
                   "11 41 2"<<endl<<
                   "13 1 3"<<endl<<
                   "13 2 2"<<endl<<
                   "13 3 2"<<endl<<
                   "13 6 2"<<endl<<
                   "13 8 2"<<endl<<
                   "13 10 2"<<endl<<
                   "13 11 1"<<endl<<
                   "13 14 1"<<endl<<
                   "13 18 2"<<endl<<
                   "13 19 4"<<endl<<
                   "13 20 1"<<endl<<
                   "13 21 2"<<endl<<
                   "13 22 2"<<endl<<
                   "13 23 2"<<endl<<
                   "13 24 2"<<endl<<
                   "13 25 2"<<endl<<
                   "13 26 2"<<endl<<
                   "13 27 2"<<endl<<
                   "13 32 2"<<endl<<
                   "13 33 2"<<endl<<
                   "13 35 2"<<endl<<
                   "13 36 2"<<endl<<
                   "13 37 2"<<endl<<
                   "13 38 2"<<endl<<
                   "13 40 2"<<endl<<
                   "13 41 1"<<endl<<
                   "13 42 2"<<endl<<
                   "13 43 2"<<endl<<
                   "13 44 2"<<endl<<
                   "13 45 4"<<endl<<
                   "13 46 2"<<endl<<
                   "14 1 3"<<endl<<
                   "14 2 4"<<endl<<
                   "14 8 2"<<endl<<
                   "14 13 2"<<endl<<
                   "14 19 2"<<endl<<
                   "14 21 2"<<endl<<
                   "14 22 1"<<endl<<
                   "14 24 1"<<endl<<
                   "14 25 2"<<endl<<
                   "14 33 2"<<endl<<
                   "14 35 2"<<endl<<
                   "14 40 3"<<endl<<
                   "14 42 1"<<endl<<
                   "14 44 2"<<endl<<
                   "14 45 4"<<endl<<
                   "14 46 2"<<endl<<
                   "18 1 3"<<endl<<
                   "18 3 2"<<endl<<
                   "18 6 3"<<endl<<
                   "18 8 2"<<endl<<
                   "18 11 1"<<endl<<
                   "18 13 2"<<endl<<
                   "18 14 1"<<endl<<
                   "18 19 2"<<endl<<
                   "18 20 3"<<endl<<
                   "18 21 2"<<endl<<
                   "18 22 1"<<endl<<
                   "18 23 2"<<endl<<
                   "18 24 2"<<endl<<
                   "18 25 2"<<endl<<
                   "18 26 2"<<endl<<
                   "18 27 2"<<endl<<
                   "18 31 2"<<endl<<
                   "18 32 4"<<endl<<
                   "18 33 2"<<endl<<
                   "18 35 2"<<endl<<
                   "18 36 4"<<endl<<
                   "18 37 2"<<endl<<
                   "18 38 2"<<endl<<
                   "18 40 2"<<endl<<
                   "18 41 2"<<endl<<
                   "18 42 3"<<endl<<
                   "18 43 2"<<endl<<
                   "18 44 2"<<endl<<
                   "18 45 1"<<endl<<
                   "19 1 3"<<endl<<
                   "19 2 2"<<endl<<
                   "19 3 2"<<endl<<
                   "19 6 2"<<endl<<
                   "19 8 2"<<endl<<
                   "19 10 2"<<endl<<
                   "19 13 4"<<endl<<
                   "19 14 2"<<endl<<
                   "19 18 2"<<endl<<
                   "19 21 2"<<endl<<
                   "19 22 2"<<endl<<
                   "19 23 2"<<endl<<
                   "19 24 2"<<endl<<
                   "19 25 2"<<endl<<
                   "19 26 2"<<endl<<
                   "19 27 2"<<endl<<
                   "19 31 2"<<endl<<
                   "19 32 2"<<endl<<
                   "19 33 2"<<endl<<
                   "19 35 2"<<endl<<
                   "19 36 1"<<endl<<
                   "19 37 2"<<endl<<
                   "19 38 2"<<endl<<
                   "19 40 2"<<endl<<
                   "19 41 2"<<endl<<
                   "19 42 2"<<endl<<
                   "19 44 3"<<endl<<
                   "19 45 3"<<endl<<
                   "19 46 2"<<endl<<
                   "20 1 2"<<endl<<
                   "20 3 1"<<endl<<
                   "20 6 2"<<endl<<
                   "20 11 1"<<endl<<
                   "20 13 1"<<endl<<
                   "20 18 3"<<endl<<
                   "20 22 2"<<endl<<
                   "20 23 1"<<endl<<
                   "20 24 1"<<endl<<
                   "20 27 2"<<endl<<
                   "20 31 2"<<endl<<
                   "20 32 3"<<endl<<
                   "20 33 2"<<endl<<
                   "20 35 1"<<endl<<
                   "20 36 1"<<endl<<
                   "20 37 1"<<endl<<
                   "20 38 2"<<endl<<
                   "20 41 1"<<endl<<
                   "20 43 1"<<endl<<
                   "20 44 2"<<endl<<
                   "20 45 2"<<endl<<
                   "20 46 2"<<endl<<
                   "21 1 3"<<endl<<
                   "21 2 3"<<endl<<
                   "21 3 1"<<endl<<
                   "21 6 2"<<endl<<
                   "21 8 1"<<endl<<
                   "21 11 3"<<endl<<
                   "21 13 3"<<endl<<
                   "21 14 2"<<endl<<
                   "21 18 1"<<endl<<
                   "21 19 2"<<endl<<
                   "21 22 1"<<endl<<
                   "21 23 1"<<endl<<
                   "21 24 2"<<endl<<
                   "21 26 2"<<endl<<
                   "21 27 2"<<endl<<
                   "21 31 1"<<endl<<
                   "21 32 1"<<endl<<
                   "21 33 2"<<endl<<
                   "21 35 2"<<endl<<
                   "21 36 1"<<endl<<
                   "21 39 2"<<endl<<
                   "21 40 4"<<endl<<
                   "21 41 2"<<endl<<
                   "21 42 2"<<endl<<
                   "21 43 2"<<endl<<
                   "21 44 3"<<endl<<
                   "21 45 3"<<endl<<
                   "22 1 3"<<endl<<
                   "22 2 2"<<endl<<
                   "22 3 4"<<endl<<
                   "22 6 3"<<endl<<
                   "22 8 3"<<endl<<
                   "22 13 3"<<endl<<
                   "22 18 2"<<endl<<
                   "22 19 2"<<endl<<
                   "22 20 3"<<endl<<
                   "22 21 2"<<endl<<
                   "22 23 3"<<endl<<
                   "22 24 4"<<endl<<
                   "22 25 4"<<endl<<
                   "22 26 2"<<endl<<
                   "22 27 3"<<endl<<
                   "22 31 3"<<endl<<
                   "22 32 3"<<endl<<
                   "22 33 3"<<endl<<
                   "22 35 4"<<endl<<
                   "22 36 3"<<endl<<
                   "22 37 3"<<endl<<
                   "22 38 3"<<endl<<
                   "22 39 2"<<endl<<
                   "22 40 2"<<endl<<
                   "22 41 3"<<endl<<
                   "22 42 4"<<endl<<
                   "22 43 3"<<endl<<
                   "22 44 3"<<endl<<
                   "22 45 2"<<endl<<
                   "22 46 2"<<endl<<
                   "23 1 3"<<endl<<
                   "23 2 2"<<endl<<
                   "23 3 2"<<endl<<
                   "23 6 3"<<endl<<
                   "23 8 1"<<endl<<
                   "23 13 2"<<endl<<
                   "23 14 2"<<endl<<
                   "23 18 2"<<endl<<
                   "23 19 2"<<endl<<
                   "23 20 2"<<endl<<
                   "23 22 3"<<endl<<
                   "23 24 2"<<endl<<
                   "23 25 2"<<endl<<
                   "23 27 2"<<endl<<
                   "23 31 4"<<endl<<
                   "23 32 1"<<endl<<
                   "23 33 2"<<endl<<
                   "23 35 2"<<endl<<
                   "23 36 2"<<endl<<
                   "23 37 2"<<endl<<
                   "23 38 2"<<endl<<
                   "23 40 1"<<endl<<
                   "23 42 3"<<endl<<
                   "23 44 3"<<endl<<
                   "23 45 1"<<endl<<
                   "24 1 2"<<endl<<
                   "24 2 2"<<endl<<
                   "24 3 2"<<endl<<
                   "24 6 2"<<endl<<
                   "24 8 3"<<endl<<
                   "24 10 2"<<endl<<
                   "24 13 3"<<endl<<
                   "24 14 1"<<endl<<
                   "24 18 2"<<endl<<
                   "24 19 2"<<endl<<
                   "24 22 3"<<endl<<
                   "24 23 2"<<endl<<
                   "24 25 2"<<endl<<
                   "24 27 2"<<endl<<
                   "24 31 2"<<endl<<
                   "24 32 2"<<endl<<
                   "24 33 4"<<endl<<
                   "24 35 3"<<endl<<
                   "24 37 2"<<endl<<
                   "24 38 2"<<endl<<
                   "24 39 2"<<endl<<
                   "24 40 1"<<endl<<
                   "24 41 1"<<endl<<
                   "24 42 2"<<endl<<
                   "24 44 2"<<endl<<
                   "24 45 2"<<endl<<
                   "24 46 2"<<endl<<
                   "25 1 3"<<endl<<
                   "25 2 2"<<endl<<
                   "25 3 3"<<endl<<
                   "25 6 1"<<endl<<
                   "25 8 2"<<endl<<
                   "25 13 3"<<endl<<
                   "25 14 2"<<endl<<
                   "25 18 1"<<endl<<
                   "25 19 3"<<endl<<
                   "25 20 1"<<endl<<
                   "25 21 1"<<endl<<
                   "25 22 3"<<endl<<
                   "25 23 2"<<endl<<
                   "25 24 3"<<endl<<
                   "25 26 1"<<endl<<
                   "25 27 1"<<endl<<
                   "25 32 3"<<endl<<
                   "25 33 3"<<endl<<
                   "25 35 3"<<endl<<
                   "25 37 2"<<endl<<
                   "25 39 1"<<endl<<
                   "25 40 2"<<endl<<
                   "25 41 1"<<endl<<
                   "25 42 2"<<endl<<
                   "25 43 2"<<endl<<
                   "25 44 2"<<endl<<
                   "25 45 2"<<endl<<
                   "25 46 1"<<endl<<
                   "26 1 4"<<endl<<
                   "26 2 2"<<endl<<
                   "26 3 2"<<endl<<
                   "26 11 1"<<endl<<
                   "26 13 2"<<endl<<
                   "26 19 2"<<endl<<
                   "26 21 1"<<endl<<
                   "26 22 2"<<endl<<
                   "26 39 2"<<endl<<
                   "26 40 2"<<endl<<
                   "26 42 2"<<endl<<
                   "26 43 2"<<endl<<
                   "26 44 4"<<endl<<
                   "26 45 1"<<endl<<
                   "26 46 2"<<endl<<
                   "27 1 2"<<endl<<
                   "27 3 2"<<endl<<
                   "27 6 4"<<endl<<
                   "27 8 1"<<endl<<
                   "27 13 2"<<endl<<
                   "27 18 2"<<endl<<
                   "27 20 2"<<endl<<
                   "27 22 2"<<endl<<
                   "27 23 2"<<endl<<
                   "27 24 1"<<endl<<
                   "27 32 2"<<endl<<
                   "27 33 2"<<endl<<
                   "27 35 3"<<endl<<
                   "27 36 2"<<endl<<
                   "27 37 2"<<endl<<
                   "27 38 2"<<endl<<
                   "27 39 2"<<endl<<
                   "27 41 2"<<endl<<
                   "27 42 2"<<endl<<
                   "27 43 1"<<endl<<
                   "27 44 2"<<endl<<
                   "27 46 2"<<endl<<
                   "31 1 1"<<endl<<
                   "31 3 2"<<endl<<
                   "31 6 1"<<endl<<
                   "31 8 1"<<endl<<
                   "31 18 2"<<endl<<
                   "31 19 2"<<endl<<
                   "31 20 2"<<endl<<
                   "31 22 2"<<endl<<
                   "31 23 2"<<endl<<
                   "31 24 2"<<endl<<
                   "31 32 1"<<endl<<
                   "31 35 3"<<endl<<
                   "31 36 1"<<endl<<
                   "31 37 3"<<endl<<
                   "31 38 2"<<endl<<
                   "31 42 1"<<endl<<
                   "32 1 2"<<endl<<
                   "32 2 2"<<endl<<
                   "32 3 2"<<endl<<
                   "32 6 2"<<endl<<
                   "32 8 2"<<endl<<
                   "32 13 2"<<endl<<
                   "32 18 3"<<endl<<
                   "32 19 2"<<endl<<
                   "32 20 2"<<endl<<
                   "32 22 3"<<endl<<
                   "32 23 1"<<endl<<
                   "32 24 2"<<endl<<
                   "32 25 2"<<endl<<
                   "32 27 2"<<endl<<
                   "32 31 1"<<endl<<
                   "32 33 3"<<endl<<
                   "32 35 4"<<endl<<
                   "32 36 2"<<endl<<
                   "32 37 3"<<endl<<
                   "32 38 3"<<endl<<
                   "32 41 2"<<endl<<
                   "32 42 3"<<endl<<
                   "32 43 1"<<endl<<
                   "33 1 3"<<endl<<
                   "33 2 3"<<endl<<
                   "33 3 2"<<endl<<
                   "33 6 2"<<endl<<
                   "33 8 2"<<endl<<
                   "33 13 3"<<endl<<
                   "33 14 1"<<endl<<
                   "33 18 2"<<endl<<
                   "33 19 3"<<endl<<
                   "33 20 2"<<endl<<
                   "33 22 2"<<endl<<
                   "33 23 3"<<endl<<
                   "33 24 4"<<endl<<
                   "33 25 3"<<endl<<
                   "33 27 2"<<endl<<
                   "33 31 2"<<endl<<
                   "33 32 2"<<endl<<
                   "33 35 3"<<endl<<
                   "33 36 2"<<endl<<
                   "33 37 2"<<endl<<
                   "33 38 3"<<endl<<
                   "33 40 1"<<endl<<
                   "33 41 2"<<endl<<
                   "33 42 2"<<endl<<
                   "33 43 1"<<endl<<
                   "33 45 1"<<endl<<
                   "35 1 2"<<endl<<
                   "35 2 2"<<endl<<
                   "35 3 2"<<endl<<
                   "35 6 3"<<endl<<
                   "35 13 2"<<endl<<
                   "35 14 3"<<endl<<
                   "35 18 2"<<endl<<
                   "35 19 2"<<endl<<
                   "35 22 3"<<endl<<
                   "35 23 2"<<endl<<
                   "35 24 3"<<endl<<
                   "35 25 2"<<endl<<
                   "35 27 3"<<endl<<
                   "35 32 3"<<endl<<
                   "35 33 3"<<endl<<
                   "35 37 4"<<endl<<
                   "35 38 2"<<endl<<
                   "35 41 2"<<endl<<
                   "35 42 4"<<endl<<
                   "35 45 2"<<endl<<
                   "36 1 3"<<endl<<
                   "36 2 2"<<endl<<
                   "36 3 4"<<endl<<
                   "36 6 3"<<endl<<
                   "36 13 2"<<endl<<
                   "36 18 4"<<endl<<
                   "36 20 1"<<endl<<
                   "36 22 3"<<endl<<
                   "36 23 1"<<endl<<
                   "36 24 1"<<endl<<
                   "36 27 3"<<endl<<
                   "36 31 1"<<endl<<
                   "36 32 2"<<endl<<
                   "36 33 1"<<endl<<
                   "36 35 1"<<endl<<
                   "36 37 2"<<endl<<
                   "36 38 2"<<endl<<
                   "36 41 2"<<endl<<
                   "36 42 3"<<endl<<
                   "36 43 2"<<endl<<
                   "36 44 2"<<endl<<
                   "36 46 2"<<endl<<
                   "37 1 3"<<endl<<
                   "37 2 2"<<endl<<
                   "37 3 2"<<endl<<
                   "37 6 2"<<endl<<
                   "37 8 3"<<endl<<
                   "37 10 2"<<endl<<
                   "37 13 3"<<endl<<
                   "37 14 2"<<endl<<
                   "37 18 2"<<endl<<
                   "37 19 3"<<endl<<
                   "37 20 2"<<endl<<
                   "37 21 2"<<endl<<
                   "37 22 3"<<endl<<
                   "37 23 2"<<endl<<
                   "37 24 3"<<endl<<
                   "37 25 2"<<endl<<
                   "37 26 2"<<endl<<
                   "37 27 2"<<endl<<
                   "37 31 4"<<endl<<
                   "37 32 3"<<endl<<
                   "37 33 3"<<endl<<
                   "37 35 4"<<endl<<
                   "37 36 2"<<endl<<
                   "37 38 3"<<endl<<
                   "37 40 2"<<endl<<
                   "37 41 3"<<endl<<
                   "37 42 3"<<endl<<
                   "37 43 2"<<endl<<
                   "37 44 2"<<endl<<
                   "37 45 2"<<endl<<
                   "37 46 2"<<endl<<
                   "38 1 2"<<endl<<
                   "38 2 2"<<endl<<
                   "38 3 2"<<endl<<
                   "38 6 3"<<endl<<
                   "38 8 1"<<endl<<
                   "38 13 3"<<endl<<
                   "38 18 3"<<endl<<
                   "38 19 2"<<endl<<
                   "38 20 2"<<endl<<
                   "38 22 3"<<endl<<
                   "38 23 2"<<endl<<
                   "38 24 3"<<endl<<
                   "38 27 2"<<endl<<
                   "38 31 3"<<endl<<
                   "38 32 3"<<endl<<
                   "38 33 3"<<endl<<
                   "38 35 3"<<endl<<
                   "38 36 3"<<endl<<
                   "38 37 3"<<endl<<
                   "38 41 1"<<endl<<
                   "38 42 2"<<endl<<
                   "39 1 4"<<endl<<
                   "39 2 1"<<endl<<
                   "39 3 2"<<endl<<
                   "39 6 1"<<endl<<
                   "39 8 1"<<endl<<
                   "39 11 1"<<endl<<
                   "39 13 1"<<endl<<
                   "39 18 1"<<endl<<
                   "39 19 1"<<endl<<
                   "39 20 1"<<endl<<
                   "39 21 2"<<endl<<
                   "39 22 2"<<endl<<
                   "39 23 1"<<endl<<
                   "39 24 1"<<endl<<
                   "39 26 3"<<endl<<
                   "39 27 2"<<endl<<
                   "39 32 1"<<endl<<
                   "39 33 1"<<endl<<
                   "39 35 2"<<endl<<
                   "39 36 1"<<endl<<
                   "39 37 2"<<endl<<
                   "39 38 1"<<endl<<
                   "39 41 2"<<endl<<
                   "39 42 2"<<endl<<
                   "39 44 3"<<endl<<
                   "39 46 1"<<endl<<
                   "40 1 3"<<endl<<
                   "40 2 2"<<endl<<
                   "40 3 2"<<endl<<
                   "40 6 2"<<endl<<
                   "40 8 2"<<endl<<
                   "40 10 2"<<endl<<
                   "40 13 3"<<endl<<
                   "40 14 3"<<endl<<
                   "40 18 2"<<endl<<
                   "40 19 2"<<endl<<
                   "40 21 4"<<endl<<
                   "40 22 1"<<endl<<
                   "40 23 2"<<endl<<
                   "40 24 2"<<endl<<
                   "40 25 2"<<endl<<
                   "40 26 2"<<endl<<
                   "40 27 2"<<endl<<
                   "40 32 1"<<endl<<
                   "40 33 2"<<endl<<
                   "40 35 2"<<endl<<
                   "40 36 1"<<endl<<
                   "40 37 2"<<endl<<
                   "40 42 2"<<endl<<
                   "40 43 2"<<endl<<
                   "40 44 2"<<endl<<
                   "40 45 2"<<endl<<
                   "40 46 2"<<endl<<
                   "41 1 3"<<endl<<
                   "41 2 2"<<endl<<
                   "41 6 3"<<endl<<
                   "41 13 2"<<endl<<
                   "41 18 1"<<endl<<
                   "41 19 1"<<endl<<
                   "41 21 2"<<endl<<
                   "41 22 2"<<endl<<
                   "41 23 2"<<endl<<
                   "41 24 2"<<endl<<
                   "41 27 3"<<endl<<
                   "41 32 2"<<endl<<
                   "41 33 2"<<endl<<
                   "41 35 3"<<endl<<
                   "41 37 2"<<endl<<
                   "41 38 1"<<endl<<
                   "41 39 2"<<endl<<
                   "41 40 1"<<endl<<
                   "41 42 2"<<endl<<
                   "41 44 2"<<endl<<
                   "41 45 2"<<endl<<
                   "41 46 2"<<endl<<
                   "42 1 3"<<endl<<
                   "42 2 2"<<endl<<
                   "42 3 2"<<endl<<
                   "42 6 3"<<endl<<
                   "42 8 2"<<endl<<
                   "42 10 2"<<endl<<
                   "42 13 3"<<endl<<
                   "42 18 3"<<endl<<
                   "42 19 2"<<endl<<
                   "42 20 3"<<endl<<
                   "42 21 2"<<endl<<
                   "42 22 4"<<endl<<
                   "42 23 3"<<endl<<
                   "42 24 2"<<endl<<
                   "42 25 2"<<endl<<
                   "42 26 2"<<endl<<
                   "42 27 2"<<endl<<
                   "42 32 3"<<endl<<
                   "42 33 2"<<endl<<
                   "42 35 4"<<endl<<
                   "42 36 2"<<endl<<
                   "42 37 4"<<endl<<
                   "42 39 2"<<endl<<
                   "42 40 2"<<endl<<
                   "42 41 2"<<endl<<
                   "42 43 2"<<endl<<
                   "42 44 3"<<endl<<
                   "42 45 2"<<endl<<
                   "42 46 4"<<endl<<
                   "43 1 3"<<endl<<
                   "43 2 3"<<endl<<
                   "43 3 1"<<endl<<
                   "43 6 2"<<endl<<
                   "43 10 2"<<endl<<
                   "43 13 3"<<endl<<
                   "43 18 2"<<endl<<
                   "43 19 2"<<endl<<
                   "43 21 2"<<endl<<
                   "43 22 2"<<endl<<
                   "43 24 2"<<endl<<
                   "43 25 2"<<endl<<
                   "43 26 2"<<endl<<
                   "43 27 2"<<endl<<
                   "43 32 2"<<endl<<
                   "43 33 2"<<endl<<
                   "43 35 2"<<endl<<
                   "43 37 2"<<endl<<
                   "43 38 2"<<endl<<
                   "43 40 3"<<endl<<
                   "43 41 2"<<endl<<
                   "43 42 3"<<endl<<
                   "43 44 3"<<endl<<
                   "43 45 3"<<endl<<
                   "43 46 2"<<endl<<
                   "44 1 4"<<endl<<
                   "44 2 4"<<endl<<
                   "44 3 2"<<endl<<
                   "44 6 2"<<endl<<
                   "44 8 2"<<endl<<
                   "44 10 3"<<endl<<
                   "44 11 2"<<endl<<
                   "44 13 2"<<endl<<
                   "44 14 2"<<endl<<
                   "44 18 2"<<endl<<
                   "44 19 3"<<endl<<
                   "44 20 2"<<endl<<
                   "44 21 3"<<endl<<
                   "44 22 2"<<endl<<
                   "44 23 3"<<endl<<
                   "44 24 2"<<endl<<
                   "44 25 2"<<endl<<
                   "44 26 3"<<endl<<
                   "44 27 2"<<endl<<
                   "44 31 2"<<endl<<
                   "44 32 2"<<endl<<
                   "44 33 2"<<endl<<
                   "44 35 2"<<endl<<
                   "44 36 2"<<endl<<
                   "44 37 2"<<endl<<
                   "44 38 2"<<endl<<
                   "44 39 2"<<endl<<
                   "44 40 2"<<endl<<
                   "44 41 2"<<endl<<
                   "44 42 3"<<endl<<
                   "44 43 2"<<endl<<
                   "44 45 4"<<endl<<
                   "44 46 3"<<endl<<
                   "45 1 4"<<endl<<
                   "45 2 4"<<endl<<
                   "45 6 2"<<endl<<
                   "45 8 2"<<endl<<
                   "45 10 2"<<endl<<
                   "45 13 4"<<endl<<
                   "45 14 4"<<endl<<
                   "45 18 2"<<endl<<
                   "45 19 3"<<endl<<
                   "45 21 2"<<endl<<
                   "45 22 1"<<endl<<
                   "45 24 3"<<endl<<
                   "45 25 2"<<endl<<
                   "45 32 1"<<endl<<
                   "45 33 2"<<endl<<
                   "45 35 3"<<endl<<
                   "45 36 1"<<endl<<
                   "45 37 1"<<endl<<
                   "45 39 2"<<endl<<
                   "45 40 2"<<endl<<
                   "45 41 1"<<endl<<
                   "45 42 3"<<endl<<
                   "45 43 2"<<endl<<
                   "45 44 4"<<endl<<
                   "45 46 3"<<endl<<
                   "46 1 3"<<endl<<
                   "46 2 2"<<endl<<
                   "46 6 1"<<endl<<
                   "46 8 1"<<endl<<
                   "46 10 2"<<endl<<
                   "46 13 3"<<endl<<
                   "46 14 2"<<endl<<
                   "46 18 2"<<endl<<
                   "46 19 2"<<endl<<
                   "46 24 2"<<endl<<
                   "46 25 2"<<endl<<
                   "46 27 1"<<endl<<
                   "46 37 2"<<endl<<
                   "46 42 4"<<endl<<
                   "46 44 3"<<endl<<
                   "46 45 3";

    }
    else if ( fileName == "Freeman_EIES_network_48actors_Messages.dl" ) {
        qDebug()<< "		... to  " << fileName;
        outText <<
              "dl"<<endl<<
              "N=32"<<endl<<
              "format=edgelist1"<<endl<<
              "data:"<<endl<<
              "1 1 24"<<endl<<
              "1 2 488"<<endl<<
              "1 3 28"<<endl<<
              "1 4 65"<<endl<<
              "1 5 20"<<endl<<
              "1 6 65"<<endl<<
              "1 7 45"<<endl<<
              "1 8 346"<<endl<<
              "1 9 82"<<endl<<
              "1 10 52"<<endl<<
              "1 11 177"<<endl<<
              "1 12 28"<<endl<<
              "1 13 24"<<endl<<
              "1 14 49"<<endl<<
              "1 15 81"<<endl<<
              "1 16 77"<<endl<<
              "1 17 77"<<endl<<
              "1 18 73"<<endl<<
              "1 19 33"<<endl<<
              "1 20 31"<<endl<<
              "1 21 22"<<endl<<
              "1 22 46"<<endl<<
              "1 23 31"<<endl<<
              "1 24 128"<<endl<<
              "1 25 38"<<endl<<
              "1 26 89"<<endl<<
              "1 27 95"<<endl<<
              "1 28 25"<<endl<<
              "1 29 388"<<endl<<
              "1 30 71"<<endl<<
              "1 31 212"<<endl<<
              "1 32 185"<<endl<<
              "2 1 364"<<endl<<
              "2 2 6"<<endl<<
              "2 3 17"<<endl<<
              "2 4 17"<<endl<<
              "2 5 15"<<endl<<
              "2 7 30"<<endl<<
              "2 8 20"<<endl<<
              "2 9 35"<<endl<<
              "2 10 20"<<endl<<
              "2 11 22"<<endl<<
              "2 12 15"<<endl<<
              "2 13 15"<<endl<<
              "2 14 15"<<endl<<
              "2 15 15"<<endl<<
              "2 16 50"<<endl<<
              "2 17 25"<<endl<<
              "2 18 8"<<endl<<
              "2 20 15"<<endl<<
              "2 21 15"<<endl<<
              "2 22 15"<<endl<<
              "2 23 15"<<endl<<
              "2 25 15"<<endl<<
              "2 26 15"<<endl<<
              "2 27 10"<<endl<<
              "2 28 24"<<endl<<
              "2 29 89"<<endl<<
              "2 30 23"<<endl<<
              "2 31 163"<<endl<<
              "2 32 39"<<endl<<
              "3 1 4"<<endl<<
              "3 2 5"<<endl<<
              "3 8 5"<<endl<<
              "4 1 52"<<endl<<
              "4 2 30"<<endl<<
              "4 4 4"<<endl<<
              "4 6 2"<<endl<<
              "4 8 32"<<endl<<
              "4 9 21"<<endl<<
              "4 10 34"<<endl<<
              "4 11 9"<<endl<<
              "4 16 5"<<endl<<
              "4 17 4"<<endl<<
              "4 18 2"<<endl<<
              "4 19 35"<<endl<<
              "4 24 12"<<endl<<
              "4 27 12"<<endl<<
              "4 28 5"<<endl<<
              "4 29 20"<<endl<<
              "4 30 4"<<endl<<
              "4 31 19"<<endl<<
              "4 32 33"<<endl<<
              "5 1 26"<<endl<<
              "5 2 4"<<endl<<
              "5 3 4"<<endl<<
              "5 4 4"<<endl<<
              "5 6 4"<<endl<<
              "5 7 8"<<endl<<
              "5 8 4"<<endl<<
              "5 9 4"<<endl<<
              "5 10 4"<<endl<<
              "5 11 4"<<endl<<
              "5 12 4"<<endl<<
              "5 13 4"<<endl<<
              "5 14 4"<<endl<<
              "5 15 4"<<endl<<
              "5 16 4"<<endl<<
              "5 17 4"<<endl<<
              "5 18 4"<<endl<<
              "5 19 4"<<endl<<
              "5 21 4"<<endl<<
              "5 22 8"<<endl<<
              "5 23 4"<<endl<<
              "5 24 14"<<endl<<
              "5 25 4"<<endl<<
              "5 27 4"<<endl<<
              "5 29 4"<<endl<<
              "5 30 7"<<endl<<
              "5 31 4"<<endl<<
              "5 32 4"<<endl<<
              "6 1 72"<<endl<<
              "6 2 23"<<endl<<
              "6 4 2"<<endl<<
              "6 6 34"<<endl<<
              "6 8 16"<<endl<<
              "6 10 7"<<endl<<
              "6 11 15"<<endl<<
              "6 15 8"<<endl<<
              "6 16 7"<<endl<<
              "6 17 6"<<endl<<
              "6 24 14"<<endl<<
              "6 27 7"<<endl<<
              "6 28 3"<<endl<<
              "6 29 34"<<endl<<
              "6 30 3"<<endl<<
              "6 31 22"<<endl<<
              "7 1 14"<<endl<<
              "7 31 6"<<endl<<
              "8 1 239"<<endl<<
              "8 2 82"<<endl<<
              "8 3 5"<<endl<<
              "8 4 37"<<endl<<
              "8 5 3"<<endl<<
              "8 6 34"<<endl<<
              "8 7 5"<<endl<<
              "8 8 10"<<endl<<
              "8 9 12"<<endl<<
              "8 10 18"<<endl<<
              "8 11 164"<<endl<<
              "8 12 18"<<endl<<
              "8 16 30"<<endl<<
              "8 17 53"<<endl<<
              "8 18 27"<<endl<<
              "8 19 20"<<endl<<
              "8 20 4"<<endl<<
              "8 22 5"<<endl<<
              "8 23 4"<<endl<<
              "8 24 55"<<endl<<
              "8 26 9"<<endl<<
              "8 27 34"<<endl<<
              "8 29 146"<<endl<<
              "8 30 216"<<endl<<
              "8 31 88"<<endl<<
              "8 32 288"<<endl<<
              "9 1 24"<<endl<<
              "9 2 25"<<endl<<
              "9 4 2"<<endl<<
              "9 8 8"<<endl<<
              "9 9 16"<<endl<<
              "9 11 15"<<endl<<
              "9 13 10"<<endl<<
              "9 17 5"<<endl<<
              "9 27 15"<<endl<<
              "9 29 10"<<endl<<
              "9 31 30"<<endl<<
              "9 32 44"<<endl<<
              "10 1 43"<<endl<<
              "10 2 15"<<endl<<
              "10 4 32"<<endl<<
              "10 6 12"<<endl<<
              "10 8 14"<<endl<<
              "10 10 5"<<endl<<
              "10 11 25"<<endl<<
              "10 12 2"<<endl<<
              "10 16 10"<<endl<<
              "10 17 10"<<endl<<
              "10 19 20"<<endl<<
              "10 20 15"<<endl<<
              "10 22 5"<<endl<<
              "10 23 20"<<endl<<
              "10 24 29"<<endl<<
              "10 26 4"<<endl<<
              "10 27 10"<<endl<<
              "10 29 47"<<endl<<
              "10 30 6"<<endl<<
              "10 31 22"<<endl<<
              "10 32 19"<<endl<<
              "11 1 178"<<endl<<
              "11 2 36"<<endl<<
              "11 4 11"<<endl<<
              "11 6 19"<<endl<<
              "11 7 10"<<endl<<
              "11 8 172"<<endl<<
              "11 9 39"<<endl<<
              "11 10 28"<<endl<<
              "11 11 29"<<endl<<
              "11 13 4"<<endl<<
              "11 16 23"<<endl<<
              "11 17 15"<<endl<<
              "11 18 24"<<endl<<
              "11 21 8"<<endl<<
              "11 24 29"<<endl<<
              "11 25 10"<<endl<<
              "11 26 11"<<endl<<
              "11 27 22"<<endl<<
              "11 29 46"<<endl<<
              "11 31 119"<<endl<<
              "11 32 34"<<endl<<
              "12 2 5"<<endl<<
              "12 8 5"<<endl<<
              "12 12 3"<<endl<<
              "12 19 5"<<endl<<
              "12 29 53"<<endl<<
              "12 31 5"<<endl<<
              "12 32 9"<<endl<<
              "13 1 5"<<endl<<
              "13 11 5"<<endl<<
              "13 31 5"<<endl<<
              "14 1 12"<<endl<<
              "14 3 9"<<endl<<
              "14 14 2"<<endl<<
              "14 16 12"<<endl<<
              "14 19 5"<<endl<<
              "14 29 35"<<endl<<
              "14 31 8"<<endl<<
              "15 1 120"<<endl<<
              "15 6 4"<<endl<<
              "15 12 5"<<endl<<
              "15 15 78"<<endl<<
              "15 27 8"<<endl<<
              "15 29 58"<<endl<<
              "15 31 32"<<endl<<
              "16 1 58"<<endl<<
              "16 2 25"<<endl<<
              "16 4 10"<<endl<<
              "16 8 20"<<endl<<
              "16 10 5"<<endl<<
              "16 11 10"<<endl<<
              "16 14 5"<<endl<<
              "16 16 15"<<endl<<
              "16 17 10"<<endl<<
              "16 21 5"<<endl<<
              "16 24 5"<<endl<<
              "16 29 35"<<endl<<
              "16 31 10"<<endl<<
              "17 1 63"<<endl<<
              "17 2 18"<<endl<<
              "17 3 9"<<endl<<
              "17 4 7"<<endl<<
              "17 6 6"<<endl<<
              "17 8 36"<<endl<<
              "17 10 5"<<endl<<
              "17 11 9"<<endl<<
              "17 12 5"<<endl<<
              "17 14 5"<<endl<<
              "17 16 5"<<endl<<
              "17 20 5"<<endl<<
              "17 21 2"<<endl<<
              "17 27 15"<<endl<<
              "17 29 10"<<endl<<
              "17 30 9"<<endl<<
              "17 31 15"<<endl<<
              "17 32 9"<<endl<<
              "18 1 58"<<endl<<
              "18 2 8"<<endl<<
              "18 3 5"<<endl<<
              "18 4 4"<<endl<<
              "18 8 4"<<endl<<
              "18 10 5"<<endl<<
              "18 11 18"<<endl<<
              "18 18 4"<<endl<<
              "18 27 20"<<endl<<
              "18 29 8"<<endl<<
              "18 30 10"<<endl<<
              "18 31 48"<<endl<<
              "19 1 5"<<endl<<
              "19 2 5"<<endl<<
              "19 4 25"<<endl<<
              "19 8 10"<<endl<<
              "19 14 5"<<endl<<
              "19 19 5"<<endl<<
              "19 23 5"<<endl<<
              "19 31 10"<<endl<<
              "20 21 4"<<endl<<
              "20 29 4"<<endl<<
              "21 1 9"<<endl<<
              "21 11 3"<<endl<<
              "21 16 5"<<endl<<
              "21 29 5"<<endl<<
              "22 1 10"<<endl<<
              "22 24 40"<<endl<<
              "22 29 15"<<endl<<
              "22 32 5"<<endl<<
              "23 1 5"<<endl<<
              "23 2 5"<<endl<<
              "23 3 5"<<endl<<
              "23 10 19"<<endl<<
              "23 19 5"<<endl<<
              "23 29 14"<<endl<<
              "23 31 5"<<endl<<
              "24 1 89"<<endl<<
              "24 2 17"<<endl<<
              "24 3 4"<<endl<<
              "24 4 14"<<endl<<
              "24 5 14"<<endl<<
              "24 6 18"<<endl<<
              "24 7 8"<<endl<<
              "24 8 41"<<endl<<
              "24 9 4"<<endl<<
              "24 10 19"<<endl<<
              "24 11 31"<<endl<<
              "24 12 4"<<endl<<
              "24 13 4"<<endl<<
              "24 14 9"<<endl<<
              "24 15 4"<<endl<<
              "24 16 14"<<endl<<
              "24 17 4"<<endl<<
              "24 18 9"<<endl<<
              "24 19 4"<<endl<<
              "24 20 4"<<endl<<
              "24 21 4"<<endl<<
              "24 22 58"<<endl<<
              "24 23 4"<<endl<<
              "24 24 5"<<endl<<
              "24 25 18"<<endl<<
              "24 26 14"<<endl<<
              "24 27 9"<<endl<<
              "24 28 4"<<endl<<
              "24 29 156"<<endl<<
              "24 30 4"<<endl<<
              "24 31 56"<<endl<<
              "24 32 10"<<endl<<
              "25 1 32"<<endl<<
              "25 2 5"<<endl<<
              "25 14 15"<<endl<<
              "25 22 10"<<endl<<
              "25 24 23"<<endl<<
              "25 25 10"<<endl<<
              "25 30 9"<<endl<<
              "25 31 15"<<endl<<
              "26 1 35"<<endl<<
              "26 2 5"<<endl<<
              "26 10 5"<<endl<<
              "26 29 10"<<endl<<
              "26 31 13"<<endl<<
              "27 1 50"<<endl<<
              "27 2 28"<<endl<<
              "27 4 13"<<endl<<
              "27 8 19"<<endl<<
              "27 9 29"<<endl<<
              "27 10 5"<<endl<<
              "27 11 8"<<endl<<
              "27 13 33"<<endl<<
              "27 15 4"<<endl<<
              "27 17 10"<<endl<<
              "27 18 15"<<endl<<
              "27 24 10"<<endl<<
              "27 28 3"<<endl<<
              "27 29 32"<<endl<<
              "27 31 13"<<endl<<
              "27 32 33"<<endl<<
              "28 1 9"<<endl<<
              "28 2 6"<<endl<<
              "28 6 3"<<endl<<
              "28 28 3"<<endl<<
              "28 32 6"<<endl<<
              "29 1 559"<<endl<<
              "29 2 132"<<endl<<
              "29 3 5"<<endl<<
              "29 4 24"<<endl<<
              "29 5 21"<<endl<<
              "29 6 29"<<endl<<
              "29 8 155"<<endl<<
              "29 9 15"<<endl<<
              "29 10 98"<<endl<<
              "29 11 69"<<endl<<
              "29 12 89"<<endl<<
              "29 13 37"<<endl<<
              "29 14 76"<<endl<<
              "29 15 80"<<endl<<
              "29 16 63"<<endl<<
              "29 17 15"<<endl<<
              "29 18 4"<<endl<<
              "29 19 9"<<endl<<
              "29 20 18"<<endl<<
              "29 21 43"<<endl<<
              "29 22 108"<<endl<<
              "29 23 29"<<endl<<
              "29 24 218"<<endl<<
              "29 26 15"<<endl<<
              "29 27 66"<<endl<<
              "29 29 6"<<endl<<
              "29 30 14"<<endl<<
              "29 31 91"<<endl<<
              "29 32 126"<<endl<<
              "30 1 39"<<endl<<
              "30 2 21"<<endl<<
              "30 4 6"<<endl<<
              "30 5 3"<<endl<<
              "30 6 3"<<endl<<
              "30 8 140"<<endl<<
              "30 10 7"<<endl<<
              "30 12 2"<<endl<<
              "30 17 9"<<endl<<
              "30 18 5"<<endl<<
              "30 27 2"<<endl<<
              "30 29 18"<<endl<<
              "30 30 2"<<endl<<
              "30 31 20"<<endl<<
              "30 32 8"<<endl<<
              "31 1 82"<<endl<<
              "31 2 125"<<endl<<
              "31 3 10"<<endl<<
              "31 4 22"<<endl<<
              "31 5 10"<<endl<<
              "31 6 15"<<endl<<
              "31 7 18"<<endl<<
              "31 8 70"<<endl<<
              "31 9 35"<<endl<<
              "31 10 23"<<endl<<
              "31 11 114"<<endl<<
              "31 12 20"<<endl<<
              "31 13 16"<<endl<<
              "31 14 15"<<endl<<
              "31 15 24"<<endl<<
              "31 16 30"<<endl<<
              "31 17 28"<<endl<<
              "31 18 49"<<endl<<
              "31 19 30"<<endl<<
              "31 20 5"<<endl<<
              "31 21 5"<<endl<<
              "31 22 15"<<endl<<
              "31 23 8"<<endl<<
              "31 24 53"<<endl<<
              "31 25 25"<<endl<<
              "31 26 8"<<endl<<
              "31 27 21"<<endl<<
              "31 28 8"<<endl<<
              "31 29 65"<<endl<<
              "31 30 28"<<endl<<
              "31 32 67"<<endl<<
              "32 1 239"<<endl<<
              "32 2 99"<<endl<<
              "32 4 27"<<endl<<
              "32 5 3"<<endl<<
              "32 8 268"<<endl<<
              "32 9 101"<<endl<<
              "32 10 18"<<endl<<
              "32 11 35"<<endl<<
              "32 12 4"<<endl<<
              "32 17 7"<<endl<<
              "32 22 14"<<endl<<
              "32 24 5"<<endl<<
              "32 27 50"<<endl<<
              "32 28 6"<<endl<<
              "32 29 71"<<endl<<
              "32 30 7"<<endl<<
              "32 31 107"<<endl<<
              "32 32 219";

    }
    else if (fileName=="Freeman_34_possible_graphs_with_N_5_multirelational.paj") {
        datasetDescription=tr("This data comes from Freeman's (1979) seminal paper "
                "\"Centrality in social networks\".\n"
                "It illustrates all 34 possible graphs of five nodes. \n"
                "Freeman used them to calculate and compare the three measures "
                 "of Centrality: Degree, Betweenness and Closeness. \n"
               "Use Relation buttons on the toolbar to move between the graphs.");
        outText<< "*Network \"34 possible graphs of N=5\"" << endl <<
                  "*Vertices 5" << endl <<
                  "1 \"1\" ic red		0.221583 	0.644042	circle" << endl <<
                  "2 \"2\" ic red		0.233094 	0.351433	circle" << endl <<
                  "3 \"3\" ic red		0.696403 	0.328808	circle" << endl <<
                  "4 \"4\" ic red		0.471942 	0.197587	circle" << endl <<
                  "5 \"5\" ic red		0.726619 	0.644042	circle" << endl <<
                  "*Matrix :1" << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :2" << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :3" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :4" << endl <<
                  "0 0 0 0 1 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 0 0 " << endl <<
                  "*Matrix :5" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 1 0 0 0" << endl <<
                  "*Matrix :6" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 0 1 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :7" << endl <<
                  "0 1 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "1 1 0 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :8" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :9 \"star\"" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 1 1 1 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "*Matrix :10 \"fork\"" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 1 0 0 0 " << endl <<
                  "*Matrix :11 \"chain\"" << endl <<
                  "0 1 0 0 1 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 0 0 " << endl <<
                  "*Matrix :12" << endl <<
                  "0 1 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "1 1 1 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :13" << endl <<
                  "0 1 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 0 1 " << endl <<
                  "1 1 0 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :14" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :15" << endl <<
                  "0 1 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "0 0 0 1 0 " << endl <<
                  "*Matrix :16" << endl <<
                  "0 1 0 0 0 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "0 1 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :17" << endl <<
                  "0 1 0 0 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "0 1 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 0 0 " << endl <<
                  "*Matrix :18" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :19" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "1 1 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :20" << endl <<
                  "0 1 0 0 1 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 1 0 0 " << endl <<
                  "*Matrix :21" << endl <<
                  "0 1 0 1 0 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 1 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "0 0 1 1 0 " << endl <<
                  "*Matrix :22" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 1 1 0 0" << endl <<
                  "*Matrix :23" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "1 1 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :24" << endl <<
                  "0 1 0 0 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "0 1 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 1 0 0" << endl <<
                  "*Matrix :25" << endl <<
                  "0 1 1 0 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "1 1 0 1 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 0 0 0 " << endl <<
                  "*Matrix :26 " << endl <<
                  "0 1 1 1 0 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "1 1 0 1 0 " << endl <<
                  "1 1 1 0 0 " << endl <<
                  "0 0 0 0 0 " << endl <<
                  "*Matrix :27" << endl <<
                  "0 1 0 1 1 " << endl <<
                  "1 0 0 1 0 " << endl <<
                  "0 0 0 1 1 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "*Matrix :28" << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 0 1 1 1 " << endl <<
                  "1 1 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "*Matrix :29" << endl <<
                  "0 1 1 0 1 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 1 1 0 0 " << endl <<
                  "*Matrix :30" << endl <<
                  "0 1 1 1 0 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "1 1 0 1 1 " << endl <<
                  "1 1 1 0 0 " << endl <<
                  "0 0 1 0 0 " << endl <<
                  "*Matrix :31" << endl <<
                  "0 1 0 1 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "0 1 0 1 1 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "1 0 1 1 0 " << endl <<
                  "*Matrix :32" << endl <<
                  "0 1 1 0 1 " << endl <<
                  "1 0 1 1 1 " << endl <<
                  "1 1 0 1 1 " << endl <<
                  "0 1 1 0 0 " << endl <<
                  "1 1 1 0 0 " << endl <<
                  "*Matrix :33" << endl <<
                  "0 1 1 1 1 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "1 0 0 1 1 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "1 1 1 1 0 " << endl <<
                  "*Matrix :34" << endl <<
                  "0 1 1 1 1 " << endl <<
                  "1 0 1 1 1 " << endl <<
                  "1 1 0 1 1 " << endl <<
                  "1 1 1 0 1 " << endl <<
                  "1 1 1 1 0 ";

    }
    else if (fileName == "Mexican_Power_Network_1940s.lst"){
        outText<< "18 8 10 23 21" << endl <<
                  "19 11 21" << endl <<
                  "29 5 9 10" << endl <<
                  "23 8 9 18 11" << endl <<
                  "4 7 6 8 20 5 21" << endl <<
                  "5 4 29 20 7 6 8 9 26 21" << endl <<
                  "6 5 7 4 20 21 8" << endl <<
                  "7 4 6 5 8 20 21" << endl <<
                  "9 5 8 23 29 20 21 11 10" << endl <<
                  "8 18 23 4 5 6 7 21 24 26 25 9 10 37 20" << endl <<
                  "10 18 29 8 11 9 20 25 26" << endl <<
                  "11 19 23 9 10 25 21 36" << endl <<
                  "20 4 5 6 7 8 9 10" << endl <<
                  "24 8 26" << endl <<
                  "26 5 8 24 10" << endl <<
                  "21 19 4 5 6 7 8 9 11 18" << endl <<
                  "36 37 11" << endl <<
                  "37 8 36" << endl <<
                  "25 10 11 8";
    }
    else if (fileName == "Knocke_Bureacracies_Information_Exchange_Network.pajek"){
        qDebug()<< "		Knocke_Bureacracies_Information_Exchange_Network.pajek written... ";
        outText<< "*Network KNOKI " << endl <<
                  "*Vertices 10" << endl <<
                   "1 \"COUN\" 0.1000    0.5000    0.5000" << endl <<
                   "2 \"COMM\" 0.1764    0.2649    0.5000" << endl <<
                   "3 \"EDUC\" 0.3764    0.1196    0.5000" << endl <<
                   "4 \"INDU\" 0.6236    0.1196    0.5000" << endl <<
                   "5 \"MAYR\" 0.8236    0.2649    0.5000" << endl <<
                   "6 \"WRO \" 0.9000    0.5000    0.5000" << endl <<
                   "7 \"NEWS\" 0.8236    0.7351    0.5000" << endl <<
                   "8 \"UWAY\" 0.6236    0.8804    0.5000" << endl <<
                   "9 \"WELF\" 0.3764    0.8804    0.5000" << endl <<
                   "10 \"WEST\" 0.1764    0.7351    0.5000" << endl <<
                  "*Arcs" << endl <<
                  " 1 2  1" << endl <<
                  " 1 5  1" << endl <<
                  " 1 7  1" << endl <<
                  " 1 9  1" << endl <<
                  " 2 1  1" << endl <<
                  " 2 3  1" << endl <<
                  " 2 4  1" << endl <<
                  " 2 5  1" << endl <<
                  " 2 7  1" << endl <<
                  " 2 8  1" << endl <<
                  " 2 9  1" << endl <<
                  " 3 2  1" << endl <<
                  " 3 4  1" << endl <<
                  " 3 5  1" << endl <<
                  " 3 6  1" << endl <<
                  " 3 7  1" << endl <<
                  " 3 10  1" << endl <<
                  " 4 1  1" << endl <<
                  " 4 2  1" << endl <<
                  " 4 5  1" << endl <<
                  " 4 7  1" << endl <<
                  " 5 1  1" << endl <<
                  " 5 2  1" << endl <<
                  " 5 3  1" << endl <<
                  " 5 4  1" << endl <<
                  " 5 7  1" << endl <<
                  " 5 8  1" << endl <<
                  " 5 9  1" << endl <<
                  " 5 10  1" << endl <<
                  " 6 3  1" << endl <<
                  " 6 7  1" << endl <<
                  " 6 9  1" << endl <<
                  " 7 2  1" << endl <<
                  " 7 4  1" << endl <<
                  " 7 5  1" << endl <<
                  " 8 1  1" << endl <<
                  " 8 2  1" << endl <<
                  " 8 4  1" << endl <<
                  " 8 5  1" << endl <<
                  " 8 7  1" << endl <<
                  " 8 9  1" << endl <<
                  " 9 2  1" << endl <<
                  " 9 5  1" << endl <<
                  " 9 7  1" << endl <<
                  " 10 1  1" << endl <<
                  " 10 2  1" << endl <<
                  " 10 3  1" << endl <<
                  " 10 5  1" << endl <<
                  " 10 7  1";
                    qDebug()<< "		Knocke_Bureacracies_Information_Exchange_Network.pajek written... ";
    }
    else if (fileName=="Stephenson&Zelen_40_AIDS_patiens_sex_contact.paj"){
        qDebug()<<"Stephenson&Zelen_40_AIDS_patiens";
        outText << "*Network Stephenson&Zelen_40_AIDS_patiens"<<endl<<
                   "*Vertices 40"<<endl<<
                   "1 \"1\" ic red		0.15899 	0.150442	circle"<<endl<<
                   "2 \"2\" ic red		0.178306 	0.210914	circle"<<endl<<
                   "3 \"3\" ic red		0.242199 	0.181416	circle"<<endl<<
                   "4 \"4\" ic red		0.31055 	0.182891	circle"<<endl<<
                   "5 \"5\" ic red		0.20951 	0.253687	circle"<<endl<<
                   "6 \"6\" ic red		0.132244 	0.29351	circle"<<endl<<
                   "7 \"7\" ic red		0.0846954 	0.327434	circle"<<endl<<
                   "8 \"8\" ic red		0.200594 	0.351032	circle"<<endl<<
                   "9 \"9\" ic red		0.170877 	0.412979	circle"<<endl<<
                   "10 \"10\" ic red		0.120357 	0.458702	circle"<<endl<<
                   "11 \"11\" ic red		0.283804 	0.292035	circle"<<endl<<
                   "12 \"12\" ic red		0.329866 	0.244838	circle"<<endl<<
                   "13 \"13\" ic red		0.389302 	0.210914	circle"<<endl<<
                   "14 \"14\" ic red		0.459138 	0.238938	circle"<<endl<<
                   "15 \"15\" ic red		0.497771 	0.294985	circle"<<endl<<
                   "16 \"16\" ic red		0.401189 	0.351032	circle"<<endl<<
                   "17 \"17\" ic red		0.280832 	0.349558	circle"<<endl<<
                   "18 \"18\" ic red		0.251114 	0.482301	circle"<<endl<<
                   "19 \"19\" ic red		0.344725 	0.547198	circle"<<endl<<
                   "20 \"20\" ic red		0.317979 	0.463127	circle"<<endl<<
                   "21 \"21\" ic red		0.401189 	0.449852	circle"<<endl<<
                   "22 \"22\" ic red		0.536404 	0.418879	circle"<<endl<<
                   "23 \"23\" ic red		0.63893 	0.355457	circle"<<endl<<
                   "24 \"24\" ic red		0.658247 	0.268437	circle"<<endl<<
                   "25 \"25\" ic red		0.676077 	0.443953	circle"<<endl<<
                   "26 \"26\" ic red		0.576523 	0.516224	circle"<<endl<<
                   "27 \"27\" ic red		0.468053 	0.511799	circle"<<endl<<
                   "28 \"28\" ic red		0.482912 	0.600295	circle"<<endl<<
                   "29 \"29\" ic red		0.482912 	0.675516	circle"<<endl<<
                   "30 \"30\" ic red		0.423477 	0.728614	circle"<<endl<<
                   "31 \"31\" ic red		0.592868 	0.646018	circle"<<endl<<
                   "32 \"32\" ic red		0.59584 	0.728614	circle"<<endl<<
                   "33 \"33\" ic red		0.594354 	0.792035	circle"<<endl<<
                   "34 \"34\" ic red		0.69688 	0.839233	circle"<<endl<<
                   "35 \"35\" ic red		0.805349 	0.889381	circle"<<endl<<
                   "36 \"36\" ic red		0.710253 	0.669617	circle"<<endl<<
                   "37 \"37\" ic red		0.787519 	0.70944	circle"<<endl<<
                   "38 \"38\" ic red		0.698366 	0.539823	circle"<<endl<<
                   "39 \"39\" ic red		0.808321 	0.466077	circle"<<endl<<
                   "40 \"40\" ic red		0.817236 	0.564897	circle"<<endl<<
                   "*Edges "<<endl<<
                   "1 2 1 c black"<<endl<<
                   "2 5 1 c black"<<endl<<
                   "3 5 1 c black"<<endl<<
                   "4 5 1 c black"<<endl<<
                   "5 6 1 c black"<<endl<<
                   "5 11 1 c black"<<endl<<
                   "7 8 1 c black"<<endl<<
                   "8 9 1 c black"<<endl<<
                   "8 11 1 c black"<<endl<<
                   "9 10 1 c black"<<endl<<
                   "11 16 1 c black"<<endl<<
                   "12 16 1 c black"<<endl<<
                   "13 14 1 c black"<<endl<<
                   "14 16 1 c black"<<endl<<
                   "15 16 1 c black"<<endl<<
                   "16 17 1 c black"<<endl<<
                   "16 20 1 c black"<<endl<<
                   "16 21 1 c black"<<endl<<
                   "16 22 1 c black"<<endl<<
                   "18 20 1 c black"<<endl<<
                   "19 20 1 c black"<<endl<<
                   "19 28 1 c black"<<endl<<
                   "22 23 1 c black"<<endl<<
                   "22 25 1 c black"<<endl<<
                   "22 26 1 c black"<<endl<<
                   "23 24 1 c black"<<endl<<
                   "26 27 1 c black"<<endl<<
                   "26 28 1 c black"<<endl<<
                   "26 31 1 c black"<<endl<<
                   "26 38 1 c black"<<endl<<
                   "28 29 1 c black"<<endl<<
                   "29 30 1 c black"<<endl<<
                   "31 32 1 c black"<<endl<<
                   "31 36 1 c black"<<endl<<
                   "32 33 1 c black"<<endl<<
                   "32 34 1 c black"<<endl<<
                   "33 34 1 c black"<<endl<<
                   "34 35 1 c black"<<endl<<
                   "36 37 1 c black"<<endl<<
                   "38 39 1 c black"<<endl<<
                   "38 40 1 c black";

    }
    else if (fileName == "Stephenson&Zelen_5actors_6edges_IC_test_dataset.paj"){
        qDebug() << "Stephenson&Zelen_5actors_6edges_IC_test_dataset.paj";
        outText<<"*Network Stephenson&Zelen_5_actors_6edges"<<endl<<
                 "*Vertices 5"<<endl<<
                 "1 \"1\" ic red		0.226804 	0.365782	circle"<<endl<<
                 "2 \"2\" ic red		0.745214 	0.365782	circle"<<endl<<
                 "3 \"3\" ic red		0.758468 	0.724189	circle"<<endl<<
                 "4 \"4\" ic red		0.226804 	0.724189	circle"<<endl<<
                 "5 \"5\" ic red		0.480118 	0.10472	circle"<<endl<<
                 "*Matrix :1 non-weighted"<<endl<<
                 "0 1 0 1 1 "<<endl<<
                 "1 0 1 0 1 "<<endl<<
                 "0 1 0 1 0 "<<endl<<
                 "1 0 1 0 0 "<<endl<<
                 "1 1 0 0 0"<<endl<<
                 "*Matrix :2 weighted"<<endl<<
                 "0 2 0 1 5 "<<endl<<
                 "2 0 1 0 5 "<<endl<<
                 "0 1 0 10 0 "<<endl<<
                 "1 0 10 0 0 "<<endl<<
                 "5 5 0 0 0 ";

    }
    else if (fileName=="Wasserman_Faust_7actors_star_circle_line_graphs.paj") {
        qDebug () << "Wasserman_Faust_7actors_star_circle_line_graphs.paj";
        outText<< "*Network 7actors-wasserman-test-net-all"<<endl<<
                  "*Vertices 7"<<endl<<
                  "1 \"1\" ic red         0.441826        0.426254        circle"<<endl<<
                  "2 \"2\" ic red         0.584683        0.19469 circle"<<endl<<
                  "3 \"3\" ic red         0.71134         0.417404        circle"<<endl<<
                  "4 \"4\" ic red         0.664212        0.687316        circle"<<endl<<
                  "5 \"5\" ic red         0.310751        0.70944 circle"<<endl<<
                  "6 \"6\" ic red         0.157585        0.427729        circle"<<endl<<
                  "7 \"7\" ic red         0.248895        0.193215        circle"<<endl<<
                  "*Matrix :1 star"<<endl<<
                  "0 1 1 1 1 1 1 "<<endl<<
                  "1 0 0 0 0 0 0 "<<endl<<
                  "1 0 0 0 0 0 0 "<<endl<<
                  "1 0 0 0 0 0 0 "<<endl<<
                  "1 0 0 0 0 0 0 "<<endl<<
                  "1 0 0 0 0 0 0 "<<endl<<
                  "1 0 0 0 0 0 0"<<endl<<
                  "*Matrix :2 circle"<<endl<<
                  "0 1 0 0 0 0 1 "<<endl<<
                  "1 0 1 0 0 0 0 "<<endl<<
                  "0 1 0 1 0 0 0 "<<endl<<
                  "0 0 1 0 1 0 0 "<<endl<<
                  "0 0 0 1 0 1 0 "<<endl<<
                  "0 0 0 0 1 0 1 "<<endl<<
                  "1 0 0 0 0 1 0 "<<endl<<
                  "*Matrix :3 line"<<endl<<
                  "0 1 1 0 0 0 0 "<<endl<<
                  "1 0 0 1 0 0 0 "<<endl<<
                  "1 0 0 0 1 0 0 "<<endl<<
                  "0 1 0 0 0 1 0 "<<endl<<
                  "0 0 1 0 0 0 1 "<<endl<<
                  "0 0 0 1 0 0 0 "<<endl<<
                  "0 0 0 0 1 0 0";
    }
    else if (fileName == "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek"){
        qDebug()<< "		Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek written... ";
        outText<< "*Network Countries_Trade_Basic_Manufactured_Goods" << endl <<
                  "*Vertices      24" << endl <<
                    "1 \"ALG\"     0.5408 0.0347" << endl <<
                    "2 \"ARG\"     0.9195 0.1080" << endl <<
                    "3 \"BRA\"     0.7626 0.4348" << endl <<
                    "4 \"CHI\"     0.5190 0.2900" << endl <<
                    "5 \"CZE\"     0.4734 0.5176" << endl <<
                    "6 \"ECU\"     0.9669 0.3401" << endl <<
                    "7 \"EGY\"     0.1749 0.9478" << endl <<
                    "8 \"ETH\"     0.4757 0.9701" << endl <<
                    "9 \"FIN\"     0.6789 0.5941" << endl <<
                   "10 \"HON\"     0.9499 0.6624" << endl <<
                   "11 \"IND\"     0.0638 0.2404" << endl <<
                   "12 \"ISR\"     0.6606 0.1142" << endl <<
                   "13 \"JAP\"     0.4718 0.4038" << endl <<
                   "14 \"LIB\"     0.9210 0.9313" << endl <<
                   "15 \"MAD\"     0.7077 0.9150" << endl <<
                   "16 \"NZ\"      0.0501 0.6893" << endl <<
                   "17 \"PAK\"     0.3653 0.3211" << endl <<
                   "18 \"SPA\"     0.6454 0.3687" << endl <<
                   "19 \"SWI\"     0.5480 0.7162" << endl <<
                   "20 \"SYR\"     0.2465 0.0501" << endl <<
                   "21 \"TAI\"     0.3805 0.6520" << endl <<
                   "22 \"UK\"      0.5921 0.4555" << endl <<
                   "23 \"US\"      0.5464 0.5983" << endl <<
                  "24 \"YUG\"     0.3576 0.4845" << endl <<
                  "*Matrix :3 \"ws6 - Basic manufactured goods\"" << endl <<
                   "0 0 0 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1" << endl <<
                   "1 0 1 1 0 1 0 0 1 0 1 1 1 0 0 0 1 1 1 0 1 0 1 0" << endl <<
                   "1 1 0 1 1 1 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1" << endl <<
                   "1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1" << endl <<
                   "1 1 1 1 0 1 1 1 1 1 1 0 1 1 0 1 1 1 1 1 1 1 1 1" << endl <<
                   "0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0" << endl <<
                   "0 0 0 0 1 0 0 1 1 0 0 0 1 0 0 0 0 1 1 0 0 1 1 1" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 0" << endl <<
                   "1 1 1 1 1 1 1 1 0 1 1 1 1 0 0 1 1 1 1 1 1 1 1 1" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0" << endl <<
                   "1 0 0 1 1 0 1 0 1 0 0 0 1 0 0 1 1 1 1 0 1 1 1 1" << endl <<
                   "0 1 0 0 0 0 0 1 1 0 0 0 1 0 0 1 0 1 1 0 1 1 1 1" << endl <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0" << endl <<
                   "1 0 0 1 0 0 1 0 0 0 1 0 1 0 0 0 1 1 0 0 1 1 1 1" << endl <<
                   "0 0 0 1 1 0 0 0 1 0 1 0 1 1 0 1 0 1 1 1 1 1 1 0" << endl <<
                   "1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1" << endl <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1" << endl <<
                   "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                   "0 0 1 1 0 0 0 0 1 0 1 1 1 0 0 1 1 1 1 1 0 1 1 1" << endl <<
                   "1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1" << endl <<
                   "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1" << endl <<
                   "1 1 0 1 1 0 1 1 1 0 1 1 1 0 0 1 1 1 1 1 1 1 1 0";
                    qDebug()<< "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek written... ";
    }
    file.close();
    if ( !datasetDescription.isEmpty() ) {
        emit describeDataset(datasetDescription);
    }

}




/** 
    Exports the adjacency matrix to a given textstream
*/
void Graph::writeAdjacencyMatrixTo(QTextStream& os){
    qDebug("Graph: adjacencyMatrix(), writing matrix with %i vertices", vertices());
    QList<Vertex*>::const_iterator it, it1;
    float weight=-1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight = this->hasArc ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
                os << static_cast<int> (weight) << " ";
            }
            else
                os << "0 ";
        }
        os << endl;
    }

}



/**  	Outputs adjacency matrix to a text stream
*	Used in slotExportSM() of MainWindow class.
*/
//QTextStream& operator <<  (QTextStream& os, Graph& m){
//    QList<Vertex*>::const_iterator it, it1;
//    float weight=-1;
//    for (it=m.m_graph.begin(); it!=m.m_graph.end(); it++){
//        for (it1=m.m_graph.begin(); it1!=m.m_graph.end(); it1++){
//            if ( (weight = m.hasArc ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
//                os << static_cast<int> (weight) << " ";
//            }
//            else
//                os << "0 ";
//        }
//        os << endl;
//    }
//    return os;
//}



/** 
    Writes the adjacency matrix of G to a specified file fn
    This is called by MainWindow::slotViewAdjacencyMatrix()
    The resulting matrix HAS NO spaces between elements.
*/
void Graph::writeAdjacencyMatrix (const QString fn, const char* netName) {
    qDebug()<<"Graph::writeAdjacencyMatrix() to : " << fn;
    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream outText( &file );
    outText.setCodec("UTF-8");
    int sum=0;
    float weight=0;
    outText << "-Social Network Visualizer- \n";
    outText << "Adjacency matrix of "<< netName<<": \n\n";
    QList<Vertex*>::const_iterator it, it1;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight =  hasArc ( (*it)->name(), (*it1)->name() )  )!=0 ) {
                sum++;
                outText <<  (weight) << " "; // TODO make the matrix look symmetric
            }
            else
                outText << "0 ";
        }
        outText << endl;
    }

    qDebug("Graph: Found a total of %i edge",sum);
    if ( sum != totalEdges() ) qDebug ("Error in edge count found!!!");
    else qDebug("Edge count OK!");

    file.close();
}


/*
 *  Creates an adjacency matrix AM
 *  where AM(i,j)=1 if i is connected to j
 *  and AM(i,j)=0 if i not connected to j
 *  Used in Graph::centralityInformation() and Graph::invertAdjacencyMatrix()
 */
void Graph::createAdjacencyMatrix(const bool dropIsolates,
                                  const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool symmetrize ){
    qDebug() << "Graph::createAdjacencyMatrix()";
    float m_weight=-1;
    int i=0, j=0;
    if (dropIsolates){
        qDebug() << "Graph::createAdjacencyMatrix() - Find and dropp possible isolates";
        isolatedVertices = verticesIsolated().count();
        qDebug() << "Graph::createAdjacencyMatrix() - found " << isolatedVertices << " isolates to drop. "
                 << " Will resize AM to " << m_totalVertices-isolatedVertices;
        AM.resize(m_totalVertices-isolatedVertices);
    }
    else
        AM.resize(m_totalVertices);
    QList<Vertex*>::const_iterator it, it1;
    qDebug() << "Graph::createAdjacencyMatrix() - creating new adjacency matrix ";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() || ( (*it)->isIsolated() && dropIsolates) ) {
            qDebug()<<"Graph::createAdjacencyMatrix() - vertex "
                   << (*it)->name()
                   << " is isolated. Continue";
            continue;
        }
        j=i;
        for (it1=it; it1!=m_graph.end(); it1++){
            if ( ! (*it1)->isEnabled() || ( (*it1)->isIsolated() && dropIsolates) ) {
                qDebug()<<"Graph::createAdjacencyMatrix() - vertex "
                       << (*it1)->name()
                       << " is isolated. Continue";
                continue;
            }
            if ( (m_weight = this->hasArc ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
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
            qDebug()<<" AM("<< i+1 << ","<< j+1 << ") = " <<  AM.item(i,j);
            if (i != j ) {
                if ( (m_weight = this->hasArc ( (*it1)->name(), (*it)->name() )  ) !=0 ) {
                    if (!considerWeights)
                        AM.setItem(j,i, 1 );
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
                qDebug()<<" AM("<< j+1 << ","<< i+1 << ") = " <<  AM.item(j,i);
            }
            j++;
        }
        i++;
    }
    qDebug() << "Graph::createAdjacencyMatrix() - Done.";

    adjacencyMatrixCreated=true;
}


void Graph::invertAdjacencyMatrix(){
    qDebug() << "Graph::invertAdjacencyMatrix()";
    qDebug()<<"Graph::invertAdjacencyMatrix() - first create the Adjacency Matrix AM";
    bool dropIsolates=true;
    bool considerWeights=false;
    createAdjacencyMatrix(dropIsolates, considerWeights);
    invAM.resize(m_totalVertices-isolatedVertices);
    qDebug()<<"Graph::invertAdjacencyMatrix() - invert the Adjacency Matrix AM and store it to invAM";
    invAM.inverseByGaussJordanElimination(AM);


}



void Graph::writeInvertAdjacencyMatrix(QString fn, const char* netName){
    qDebug("Graph::writeInvertAdjacencyMatrix() ");
    int i=0, j=0;
    QList<Vertex*>::const_iterator it, it1;
    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream outText( &file );
    outText.setCodec("UTF-8");
    outText << "-Social Network Visualizer- \n";
    outText << "Invert Matrix of "<< netName<<": \n\n";
    invertAdjacencyMatrix();
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled() )
            continue;
        j=0;
        for (it1=m_graph.cbegin(); it1!=m_graph.cend(); ++it1){
            if ( ! (*it1)->isEnabled() )
                continue;
            outText << invAM.item(i,j)<< " ";
            qDebug() << invAM.item(i,j)<< " ";
            j++;
        }
        i++;
        outText << endl;
        qDebug() << endl;
    }

    file.close();
}


bool Graph::saveGraphToDotFormat (
        QString fileName, QString networkName, int maxWidth, int maxHeight)
{
    Q_UNUSED(fileName);
    Q_UNUSED(networkName);
    Q_UNUSED(maxWidth);
    Q_UNUSED(maxHeight);

    return true;
}



bool Graph::saveGraphToGraphMLFormat (
        QString fileName, QString networkName, int maxWidth, int maxHeight)
{
    qDebug () << " Graph::saveGraphToGraphMLFormat to file: " << fileName.toUtf8();

    int weight=0, source=0, target=0, edgeCount=0, m_size=1, m_labelSize;
    QString m_color, m_labelColor, m_label;
    bool openToken;
    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly ) )  {

        emit statusMessage( QString(tr("Could not write to %1")).arg(fileName) );
        return false;
    }
    QTextStream outText( &f );
    outText.setCodec("UTF-8");
    qDebug () << " codec used for saving stream: " << outText.codec()->name();

    qDebug()<< "		... writing xml version";
    outText << "<?xml version=\"1.0\" encoding=\"" << outText.codec()->name() << "\"?> \n";
    outText << " <!-- Created by SocNetV v."<<  VERSION << " --> \n" ;
    outText << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" "
               "      xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance \" "
               "      xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
               "      http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
               "\n";

    qDebug()<< "		... writing keys ";

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

    qDebug()<< "		... writing graph tag";
    if (networkName == "")
        networkName = "G";
    if (m_undirected)
        outText << "  <graph id=\""<< networkName << "\" edgedefault=\"undirected\"> \n";
    else
        outText << "  <graph id=\""<< networkName << "\" edgedefault=\"directed\"> \n";

    QList<Vertex*>::const_iterator it;
    QList<Vertex*>::const_iterator jt;

    qDebug()<< "		    writing nodes data";
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it){
        if ( ! (*it)->isEnabled () )
            continue;
        qDebug() << " 	Node id: "<<  (*it)->name()  ;
        outText << "    <node id=\"" << (*it)->name() << "\"> \n";
        m_color = (*it)->color();
        m_size = (*it)->size() ;
        m_labelSize=(*it)->labelSize() ;
        m_labelColor=(*it)->labelColor() ;
        m_label=(*it)->label();

        if (m_label.contains('&') ){
            m_label=m_label.replace('&',"&amp;");
        }
        if (m_label.contains('<') ){
            m_label=m_label.replace('<',"&lt;");
        }
        if (m_label.contains('>') ){
            m_label=m_label.replace('>',"&gt;");
        }
        if (m_label.contains('\"') ){
            m_label=m_label.replace('\"',"&quot;");
        }
        if (m_label.contains('\'') ){
            m_label=m_label.replace('\'',"&apos;");
        }


        outText << "      <data key=\"d0\">" << m_label <<"</data>\n";

        qDebug()<<" 		... Coordinates x " << (*it)->x()<< " "<<maxWidth
               <<" y " << (*it)->y()<< " "<<maxHeight;

        outText << "      <data key=\"d1\">" << (*it)->x()/(maxWidth) <<"</data>\n";
        outText << "      <data key=\"d2\">" << (*it)->y()/(maxHeight) <<"</data>\n";

        if (  initVertexSize != m_size ) {
            outText << "      <data key=\"d3\">" << m_size  <<"</data>\n";
        }

        if (  QString::compare ( initVertexColor, m_color,  Qt::CaseInsensitive) != 0) {
            outText << "      <data key=\"d4\">" << m_color <<"</data>\n";
        }

        outText << "      <data key=\"d5\">" << (*it)->shape() <<"</data>\n";


        if (  QString::compare ( initVertexLabelColor, m_labelColor,  Qt::CaseInsensitive) != 0) {
            outText << "      <data key=\"d6\">" << m_labelColor <<"</data>\n";
        }

        if (  initVertexLabelSize != m_labelSize ) {
            outText << "      <data key=\"d7\">" << m_labelSize <<"</data>\n";
        }

        outText << "    </node>\n";

    }

    qDebug() << "		... writing edges data";
    edgeCount=0;
    for (it=m_graph.cbegin(); it!=m_graph.cend(); ++it)
    {
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++)
        {
            source=(*it)->name();
            target=(*jt)->name();

            if  ( 	(weight= this->hasArc( source,target ) ) !=0 )
            {
                ++edgeCount;
                m_color = (*it)->outLinkColor( target );
                qDebug()<< "				edge no "<< edgeCount
                        << " from n1=" << source << " to n2=" << target
                        << " with weight " << weight
                        << " and color " << m_color.toUtf8() ;
                outText << "    <edge id=\""<< "e"+QString::number(edgeCount)
                        << "\" directed=\"" << "true" << "\" source=\"" << source
                        << "\" target=\"" << target << "\"";

                openToken = true;
                if (weight > 1) {
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
                if (openToken)
                    outText << "/> \n";
                else
                    outText << "    </edge>\n";

            }

        }
    }
    outText << "  </graph>\n";
    outText << "</graphml>\n";

    f.close();
    QString fileNameNoPath=fileName.split("/").last();
    emit statusMessage( QString(tr( "File %1 saved" ) ).arg( fileNameNoPath ) );

    return true;
}





void Graph::setShowLabels(bool toggle){
    initShowLabels=toggle;

}

void Graph::setShowNumbersInsideNodes(bool toggle){
    initNumbersInsideNodes=toggle;

}





/** 
    This slot is activated when the user clicks on the relevant MainWindow checkbox
    (SpringEmbedder, Fruchterman)
    to start or stop the movement of nodes, according to the requested model.
    PARAMETERS:
    state: movement on/off toggle
    type:  controls the type of layout model requested. Available options
            1: Spring Embedder
            2: FruchtermanReingold
    cW, cH: control the current canvasWidth and canvasHeight
*/
void Graph::nodeMovement(bool state, int type, int cW, int cH){
    qDebug()<< "Graph: startNodeMovement() - state " << state;
    canvasWidth = cW;
    canvasHeight = cH;
    //factor controls speed. Decrease it to increase speed...
    // the smaller the factor is, the less responsive is the application
    // when there are many nodes.
    int factor=50;
    if (state == true){
        qDebug()<< "Graph: startNodeMovement() - STARTING dynamicMovement" ;
        dynamicMovement = true;
        layoutType=type;
        if (!timerId) {
            qDebug("Graph: startTimer()");
            timerId = startTimer(factor);
        }
    }
    else {
        qDebug()<< "Graph: startNodeMovement() - STOPPING dynamicMovement" ;
        dynamicMovement = false;
        killTimer(timerId);
        timerId = 0;
    }
}




/**	
    This method is automatically invoked when a QTimerEvent occurs
    It checks layoutType to call the appropriate method with the Force Directed Placement algorithm.
*/
void Graph::timerEvent(QTimerEvent *event) {	
    qDebug("Graph: timerEvent()");
    Q_UNUSED(event);
    switch (layoutType){
    case 1: {
        layoutForceDirectedSpringEmbedder(dynamicMovement);
        break;
    }
    case 2: {
        layoutForceDirectedFruchtermanReingold(dynamicMovement);
        break;
    }
    }
    if (!graphModified) {
        qDebug("Timer will be KILLED since no vertex is movin any more...");
        killTimer(timerId);
        timerId = 0;
    }
}




/** 
    The Spring Embedder model (Eades, 1984), part of the Force Directed Placement (FDP) family,
    assigns forces to all vertices and edges, as if nodes were electrically charged particles (Coulomb's law)
    and all edges were springs (i.e. Hooke's law).

    These forces are applied to the nodes iteratively, pulling them closer together or pushing them further apart,
    until the system comes to an equilibrium state (node positions do not change anymore).

    Note that, following Eades, we do not need to have a faithful simulation;
    we can -and we do- apply unrealistic forces in an unrealistic manner.
    For instance, instead of the forces described by Hooke's law,
    we will assume weaker logarithmic forces between far apart vertices...
*/

void Graph::layoutForceDirectedSpringEmbedder(bool &dynamicMovement){
    qreal dist = 0;
    qreal f_rep=0, f_att=0;
    QPointF DV;
    qreal c4=0.1; //normalization factor for final displacement

    QList<Vertex*>::const_iterator v1;
    QList<Vertex*>::const_iterator v2;

    /**
     * compute max spring length as function of canvas area divided by the
     * total vertices area
    */
    qreal V = (qreal) vertices() ;
    qreal naturalLength= computeOptimalDistance(V);

    qDebug() << "\n\n layoutForceDirectedSpringEmbedder() "
             << " vertices " << V
             << " naturalLength " << naturalLength;

    if (dynamicMovement){

        //setup init disp

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            (*v1) -> disp().rx() = 0;
            (*v1) -> disp().ry() = 0;
            qDebug() << " 0000 s " << (*v1)->name() << " zeroing rx/ry";
        }

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            qDebug() << "*********  Calculate forces for source s  "
                     << (*v1) -> name()
                     <<" pos "<< (*v1) -> x()<< ", "<< (*v1) -> y();

            if ( ! (*v1)->isEnabled() ) {
                qDebug() << "  vertex s disabled. Continue";
                continue;
            }

            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2)
            {
                if ( ! (*v2)->isEnabled() ) {
                    qDebug() << "   t " << (*v1)->name() << " disabled. Continue";
                    continue;
                }

                if (v2 == v1) {
                    qDebug() << "   s==t, continuing";
                    continue;
                }

                DV.setX( (*v2) -> x() - (*v1)->x());
                DV.setY( (*v2) -> y() - (*v1)->y());

                dist = euclideian_distance(DV);

                /**
                  *  calculate electric (repulsive) forces between
                  *  all vertices.
                  */
                f_rep = layoutForceDirected_F_rep (dist, naturalLength) ;
                (*v1)->disp().rx() += sign( DV.x() ) * f_rep ;
                (*v1)->disp().ry() += sign( DV.y() ) * f_rep  ;
                qDebug() <<"  s = "<< (*v1)->name()
                         <<" pushed away from t = " << (*v2) -> name()
                           << " dist " <<dist
                        << " f_rep=" << f_rep
                        << " sign * f_repx " << sign( DV.x() ) * f_rep
                        << " sign * f_repy " << sign( DV.y() ) * f_rep ;

                /**
                * calculate spring forces between adjacent nodes
                * that pull them together (if d > naturalLength)
                * or push them apart (if d < naturalLength)
                */
                if ( this->hasArc ( (*v1) ->name(), (*v2) -> name()) ) {

                    f_att = layoutForceDirected_F_att (dist, naturalLength) ;

                    (*v1)->disp().rx() += sign( DV.x() ) * f_att ;
                    (*v1)->disp().ry() += sign( DV.y() ) * f_att ;
                    (*v2)->disp().rx() -= sign( DV.x() ) * f_att ;
                    (*v2)->disp().ry() -= sign( DV.y() ) * f_att ;

                    qDebug() << "  s= "<<(*v1)->name()
                             << " attracted by t= "<< (*v2)->name()
                                << " dist " <<dist
                             << " f_att="<< f_att
                             << " sdx * f_att " <<sign( DV.x() ) * f_att
                             << " sdy * f_att " <<sign( DV.y() ) * f_att
                             << " disp_s.x="<< (*v2)->disp().rx()
                             << " disp_s.y="<< (*v2)->disp().ry()
                             << " disp_t.x="<< (*v2)->disp().rx()
                             << " disp_t.y="<< (*v2)->disp().ry();

                }  // end if hasArc

            } //end for v2
            //recompute naturalLength (in case the user resized the window)
            naturalLength= computeOptimalDistance(V);
            qDebug() << "  >>> final s = "<< (*v1)->name()
                     << " disp_s.x="<< (*v1)->disp().rx()
                      << " disp_s.y="<< (*v1)->disp().ry();

        } // end for v1


        layoutForceDirected_Eades_moveNodes(c4) ;

    } //end dynamicMovement
}


/**
    Fruchterman and Reingold (1991) refined the Spring Embedder model by replacing the forces.
    In this model, "the vertices behave as atomic particles or celestial bodies,
    exerting attractive and repulsive forces on one another." (ibid).
    Again, only vertices that are neighbours attract each other but, unlike Spring Embedder,
    all vertices repel each other.
    These forces induce movement. The algorithm might resemble molecular or planetary simulations,
    sometimes called n-body problems.
*/
void Graph::layoutForceDirectedFruchtermanReingold(bool dynamicMovement){
    qreal dist = 0;
    qreal f_att, f_rep;
    QPointF DV;   //difference vector
    //qreal temperature=canvasWidth / 10.0; //limits the displacement of the vertex
    qreal temperature=2.0; //limits the displacement of the vertex
    qreal V = (qreal) vertices() ;
    qreal C=0.9; //this is found experimentally
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    qreal optimalDistance= C * computeOptimalDistance(V);

    QList<Vertex*>::const_iterator v1;
    QList<Vertex*>::const_iterator v2;

    if (dynamicMovement){
        qDebug() << "Graph: layoutForceDirectedFruchtermanReingold() ";
        qDebug () << "Graph: Setting optimalDistance = "<<  optimalDistance
                  << "...following Fruchterman-Reingold (1991) formula ";

        //setup init disp
        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
            qDebug() << " 0000 s " << (*v1)->name() << " zeroing rx/ry";
        }

        for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
        {
            qDebug() << "*****  Calculate forces for s " << (*v1)->name()
                     << " index " <<  index[(*v1)->name()]
                     << " pos "<< (*v1)->x() << ", "<< (*v1)->y();

            if ( ! (*v1)->isEnabled() ) {
                qDebug() << "  vertex s " << (*v1)->name() << " disabled. Continue";
                continue;
            }

            for (v2=m_graph.cbegin(); v2!=m_graph.cend(); ++v2)
            {
                qDebug () << "  t = "<< (*v2)->name()
                          << "  pos (" <<  (*v2)->x() << "," << (*v2)->y() << ")";

                if ( ! (*v2)->isEnabled() ) {
                    qDebug()<< " t "<< (*v2)->name()<< " disabled. Continue";
                    continue;
                }

                if (v2 == v1) {
                    qDebug() << "  s==t, continuing";
                    continue;
                }

                DV.setX( (*v2)->x() - (*v1)->x() );
                DV.setY( (*v2)->y() - (*v1)->y() );

                dist = euclideian_distance( DV );

                //calculate repulsive force from _near_ vertices
                f_rep = layoutForceDirected_F_rep(dist, optimalDistance);
                (*v1)->disp().rx() += sign( DV.x() ) * f_rep;
                (*v1)->disp().ry() += sign( DV.y() ) * f_rep ;

                qDebug()<< " dist( " << (*v1)->name() <<  "," <<  (*v2)->name() <<  " = "
                        << dist
                        << " f_rep " << f_rep
                        << " disp_s.x="<< (*v1)->disp().rx()
                        << " disp_s.y="<< (*v1)->disp().ry();

                if ( this->hasArc ((*v1)->name(), (*v2)->name()) ) {
                    //calculate attracting force
                    f_att = layoutForceDirected_F_att (dist, optimalDistance);
                    (*v1)->disp().rx() += sign( DV.x() ) * f_att;
                    (*v1)->disp().ry() += sign( DV.y() ) * f_att;
                    (*v2)->disp().rx() -= sign( DV.x() ) * f_att ;
                    (*v2)->disp().ry() -= sign( DV.y() ) * f_att ;

                    qDebug() << "  s= "<<(*v1)->name()
                             << " attracted by t= "<< (*v2)->name()
                             <<"  optimalDistance =" << optimalDistance
                             << " f_att " << f_att
                             << " disp_s.x="<< (*v1)->disp().rx()
                             << " disp_s.y="<< (*v1)->disp().ry()
                             << " disp_t.x="<< (*v2)->disp().rx()
                             << " disp_t.y="<< (*v2)->disp().ry();
                } //endif

            }//end for v2
            //recompute optimalDistance (in case the user resized the window)
            optimalDistance= C * computeOptimalDistance(V);
        } //end for v1

        layoutForceDirected_FR_moveNodes(temperature);

        // reduce the temperature as the layout approaches a better configuration
        //cool(temperature);
    }

}



/**
 * @brief Graph::computeOptimalDistance
 * @return qreal optimalDistance
 */
qreal Graph::computeOptimalDistance(const int &Vertices){
    qreal vertexWidth = (qreal)  2.0 * initVertexSize ;
    qreal screenArea = canvasHeight*canvasWidth;
    qreal vertexArea =  qCeil ( qSqrt( screenArea / Vertices ) ) ;
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    return (vertexWidth + vertexArea);
}






qreal Graph::layoutForceDirected_F_att( const qreal &dist, const qreal &optimalDistance) {
    qreal f_att;
    if (layoutType == 1) {  //layoutType -> Eades
        qreal c_spring=2;
        f_att = c_spring * log10 ( dist / optimalDistance );
    }
    else {   // layoutType -> FR
        f_att= ( dist * dist ) / optimalDistance;
    }

    return f_att;
}


qreal Graph::layoutForceDirected_F_rep(const qreal &dist, const qreal &optimalDistance) {
    qreal f_rep;
    if (layoutType == 1) { //layoutType -> Eades
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
    else {  // layoutType -> FR
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
 * @brief Graph::euclideian_distance
 * @param a
 * @param b
 * @return  the euclideian distance of QPointF a and b
 */
qreal Graph::euclideian_distance (const QPointF & a, const QPointF & b){
    return qSqrt (
                 ( b.x() - a.x() ) * (b.x() - a.x() ) +
                 ( b.y() - a.y())  * (b.y() - a.y() )
                );
}


/**
 * @brief Graph::euclideian_distance
 * @param a
 * @return  the euclideian distance of QPointF a (where a is difference vector)
 */
qreal Graph::euclideian_distance (const QPointF & a){
    return qSqrt (
                  a.x()  * a.x()  +
                  a.y() * a.y()
                );
}



void Graph::layoutForceDirected_Eades_moveNodes(const qreal &c4) {
    qDebug() << "\n *****  layoutForceDirected_Eades_moveNodes() " ;
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    QList<Vertex*>::const_iterator v1;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
    {
        // calculate new overall velocity vector
        xvel =  c4 * (*v1)->disp().rx();
        yvel =  c4 * (*v1)->disp().ry();
        qDebug() << " ##### source vertex  " <<  (*v1)->name()
                 << " xvel,yvel = ("<< xvel << ", "<< yvel << ")";

         //fix Qt error a positive QPoint to the floor
        // when we ask for moveNode to happen.
         xvel < 1 && xvel > 0 ? xvel = 1 : xvel = xvel;
         yvel < 1 && yvel > 0 ? yvel = 1 : yvel = yvel;

        //Move source node to new position according to overall velocity
        newPos = QPointF( (qreal) (*v1)->x() + xvel, (qreal) (*v1)->y() + yvel);

        qDebug() << " source vertex v1 " << (*v1)->name()
                    << " current pos: (" <<  (*v1)->x()
                    << " , " << (*v1)->y()
                    << " Possible new pos (" <<  newPos.x()
                    << " , " <<  newPos.y();

        // check if new pos is out of screen and adjust
        newPos.rx() = qMin (
                    canvasWidth - 42.0 , qMax (42.0 , newPos.x() )
                    );
        newPos.ry() = qMin (
                    canvasHeight -42.0 , qMax (42.0 , newPos.y() )
                    );


        qDebug() << "  Final new pos (" <<  newPos.x() << ","
                 << newPos.y()<< ")";
        emit moveNode((*v1)->name(),  newPos.x(),  newPos.y());

    }

}

/**
 * @brief Graph::layoutForceDirected_FR_moveNodes
 * @param temperature
 */
void Graph::layoutForceDirected_FR_moveNodes(const qreal &temperature) {

    qDebug() << "\n\n *****  layoutForceDirected_FR_moveNodes() \n\n " ;
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    QList<Vertex*>::const_iterator v1;

    for (v1=m_graph.cbegin(); v1!=m_graph.cend(); ++v1)
    {
        // compute the new position
        // limit the maximum displacement to a maximum temperature
        xvel = sign((*v1)->disp().rx()) * qMin( qAbs((*v1)->disp().rx()), temperature) ;
        yvel = sign((*v1)->disp().ry()) * qMin( qAbs((*v1)->disp().ry()), temperature) ;
        newPos = QPointF((*v1)->x()+ xvel, (*v1)->y()+yvel);
        qDebug()<< " source vertex v1 " << (*v1)->name()
                << " current pos: (" << (*v1)->x() << "," << (*v1)->y() << ")"
                << "Possible new pos (" <<  newPos.x() << ","
                << newPos.y()<< ")";

        newPos.rx() = qMin (
                    canvasWidth - 42.0 , qMax (42.0 , newPos.x() )
                    );
        newPos.ry() = qMin (
                    canvasHeight -42.0 , qMax (42.0 , newPos.y() )
                    );
        //Move node to new position
        if ( newPos.x() < 5.0  ||newPos.y() < 5.0   ||
                newPos.x() >= (canvasWidth -5)||
                newPos.y() >= (canvasHeight-5)||
                ((*v1)->x() == newPos.x() && (*v1)->y() == newPos.y() )
                )
            continue;
        qDebug()<< " final new pos "
                <<  newPos.x() << ","
                << newPos.y()<< ")";
        emit moveNode((*v1)->name(),  newPos.x(),  newPos.y());
    }
}


Graph::~Graph() {
    clear();
}

