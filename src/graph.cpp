/******************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.31
 Written in Qt
 
                         graph.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2014 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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


#include <cstdlib>		//allows the use of RAND_MAX macro 
#include <QPointF>
#include <QDebug>		//used for qDebug messages
#include <QDateTime> 	// used in exporting centrality files
#include <QHash> 
#include <queue>		//for BFS queue Q

#include "graph.h"


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
    calculatedCentralities=false;
    calculatedIRCC=false;
    calculatedPP=false;
    m_precision = 3;
    m_curRelation=0;
    dynamicMovement=false;
    timerId=0;
    layoutType=0;

    parser.setParent(this);

    connect (
                &parser, SIGNAL( addRelation (QString) ),
                this, SLOT(addRelationFromParser(QString) )
                ) ;

    connect (
                &parser, SIGNAL( changeRelation (int) ),
                this, SLOT( changeRelation (int) )
                ) ;


    connect (
                &parser, SIGNAL( createNode (int,int,QString, QString, int, QString, QString, int, QPointF, QString, bool) ),
                this, SLOT(createVertex(int,int,QString, QString, int, QString, QString, int, QPointF, QString, bool) )
                ) ;

    connect (
                &parser, SIGNAL(createEdge (int, int, float, QString, int, bool, bool)),
                this, SLOT(createEdge (int, int, float, QString, int, bool, bool) )
                );

    connect (
                &parser, SIGNAL(fileType(int, QString, int, int, bool)),
                this, SLOT(setFileType(int, QString, int, int, bool))
                );

    connect (
                &parser, SIGNAL(removeDummyNode(int)),
                this, SLOT (removeDummyNode(int))
                );



    connect (
                &crawler, SIGNAL( createNode (QString, int) ),
                this, SLOT(createVertex(QString, int ) )
                ) ;

    connect (
                &crawler, SIGNAL(createEdge (int, int)),
                this, SLOT(createEdge (int, int) )
                );


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
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() )
            continue;
       (*it)->changeRelation(relation);
    }
    m_curRelation = relation;
    emit relationChanged(m_curRelation);
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

void Graph::createVertex(QString label, int i) {
    if ( i < 0 )  i = lastVertexNumber() +1;
    qDebug() << "Graph::createVertex() " << i << " rand coords with label";
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
    qDebug()<<" Graph::createEdge() " << v1 << " -> " << v2
           << " weight " << weight << " relation " << m_curRelation;
    if ( reciprocal == 2) {
        qDebug()<<"  Creating edge as RECIPROCAL - emitting drawEdge signal to GW";
        addEdge ( v1, v2, weight, color, reciprocal);
        emit drawEdge(v1, v2, weight, reciprocal, drawArrows, color, bezier);
    }
    else if (this->hasEdge( v2, v1) )  {
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
void Graph::createEdge (int source, int target){
    qDebug()<< " Graph::createEdge() - from " << source << " to " << target ;
    if (this->hasEdge(source, target) ) {
        qDebug()<< "  Already exists - returning...";
        return;
    }
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
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        if  ( (*it)->isLinkedTo(Doomed) != 0) {
            qDebug()<< "Graph: Vertex " << (*it)->name()
                    << " is linked to doomed "<< Doomed << " and has "
                    << (*it)->outLinks() << " and " <<  (*it)->outDegree() ;
            if ( (*it)->outLinks() == 1 && (*it)->isLinkedFrom(Doomed) != 0 )	{
                qDebug() << "Graph: decreasing reciprocalEdgesVert";
                (*it)->setReciprocalLinked(false);
            }
            (*it)->removeLinkTo(Doomed) ;
        }
        if (  (*it)->isLinkedFrom(Doomed) != 0 ) {
            (*it)->removeLinkFrom(Doomed);
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

    m_graph [ source ]->setOutLinked(true) ;
    m_graph [ source ]->addLinkTo(v2, weight );
    m_graph [ target ]->setInLinked(true) ;
    m_graph [ target ]->addLinkFrom(v1, weight);
    m_totalEdges++;

    if (reciprocal == 1){
        m_graph [ source ]->setReciprocalLinked(true);
        m_graph [ target ]->setReciprocalLinked(true);
    }
    else if (reciprocal == 2){
        m_graph [ source ]->setReciprocalLinked(true);
        m_graph [ target ]->setReciprocalLinked(true);
        m_graph [ target ]->addLinkTo(v1, weight );
        m_graph [ source ]->addLinkFrom(target, weight);
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
void Graph::setEdgeWeight (int v1, int v2, float weight) {
    qDebug() << "Graph: setEdgeWeight between " << v1 << "[" << index[v1] << "] and " << v2 << "[" << index[v2] << "]" << " = " << weight;
    m_graph [ index[v1] ]->changeLinkWeightTo(v2, weight);
    graphModified=true;
    emit graphChanged();
}


/** 	Removes the edge (arc) between v1 and v2
*/
void Graph::removeEdge (int v1, int v2) {	
    qDebug ()<< "Graph: removeEdge edge " << v1 << " to " << v2 << " to be removed from graph";
    qDebug() << "Graph: Vertex named " << m_graph[ index[v1] ]->name() << " has index = " << index[v1];
    m_graph [ index[v1] ]->removeLinkTo(v2);
    m_graph [ index[v2] ]->removeLinkFrom(v1);
    qDebug()<< "Graph: removeEdge between " << v1 << " i " << index[v1] << " and " << v2 << " i "<< index[v2]
               << "  NOW vertex v1 reports edge weight " << m_graph [ index[v1] ]->isLinkedTo(v2) ;
    if ( this->hasEdge(v2,v1) !=0) symmetricAdjacencyMatrix=false;
    m_totalEdges--;
    if (m_totalEdges<0) m_totalEdges=0;
    graphModified=true;
    qDebug("Graph: removeEdge(): emitting eraseEdge to GW");
    emit eraseEdge(v1,v2);
    emit graphChanged();
}





//Called by MW to start a web crawler...
void Graph::webCrawl( QString seed, int maxNodes, int maxRecursion,  bool goOut){
    qDebug() << "Graph:: webCrawl - Calling thread for " << seed ;
    crawler.load(seed, maxNodes, maxRecursion, goOut);
    qDebug("Graph:: See the thread? :)");
}






/**
    Called from filterOrphanNodes via MainWindow  to filter nodes with no links
    For each orphan Vertex in the Graph, emits the filterVertex()
*/
void Graph::filterIsolateVertices(bool filterFlag){
    if (filterFlag)
        qDebug() << "Graph: filterIsolateVertices() enabling all orphan nodes";
    else
        qDebug() << "Graph: filterIsolateVertices() disabling all orphan nodes";

    QList<Vertex*>::iterator it;
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( (*it)->isOutLinked() ||  (*it)->isInLinked() ){
            continue;
        }
        else {
            qDebug() << "Graph:filterOrphanNodes() Vertex " << (*it)->name()
                     << " not linked. Toggling it and emitting setVertexVisibility signal to GW...";
            (*it)->setEnabled (filterFlag) ;
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

    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( (*it)->isOutLinked() ){
            (*it)->filterEdgesByWeight ( m_threshold, overThreshold );
        }
        else
            qDebug() << "Graph:filterEdgesByWeight() Vertex " << (*it)->name()
                     << " not linked. Proceeding...";
    }
}



/**
 * @brief Graph::filterEdgesByRelation
 * Not called by Called from MW to filter out all edges of a given relation
 * calls the homonymous method of Vertex class.
 * @param relation
  */
void Graph::filterEdgesByRelation(int relation, bool status){
    qDebug() << "Graph::filterEdgesByRelation() " ;
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
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
    QList<Vertex*>::iterator it;
    int i=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( (*it) ->label() == label)  {
            qDebug()<< "Graph: hasVertex() at pos %i" << i;
            return i;
        }
        i++;
    }
    qDebug("Graph: hasVertex() NO - returning -1");
    return -1;
}




void Graph::setInitVertexSize (long int size) {
    initVertexSize=size;
}


//Changes the size.of vertex v 
void Graph::setVertexSize(long int v, int size) {
    m_graph[ index[v] ]->setSize(size);
    graphModified=true;
    emit graphChanged();
}


void Graph::setInitVertexShape(QString shape) {
    initVertexShape=shape;
}

//Changes the shape.of vertex v 
void Graph::setVertexShape(int v1, QString shape){
    m_graph[ index[v1] ]->setShape(shape);
    graphModified=true;
    emit graphChanged();
}


//returns the shape of this vertex
QString Graph::shape(int v1){
    return m_graph[ index[v1] ]->shape();

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


QString Graph::label(int v1){
    return m_graph[ index[v1] ]->label ();
}


/**
    Changes the color of vertice v1
*/
void Graph::setVertexColor(long int v1, QString color){
    qDebug()<< "Graph: setVertexColor for "<< v1 << ", index " << index[v1]<< " with color "<< color;
    m_graph[ index[v1] ]->setColor ( color );
    graphModified=true;
    emit graphChanged();
}


void Graph::setInitVertexColor(QString color){
    initVertexColor=color;
}




void Graph::setInitEdgeColor(QString color){
    initEdgeColor=color;
}


/**
    Changes the color of edge (s,t).
*/
void Graph::setEdgeColor(long int s, long int t, QString color){
    qDebug()<< "Graph: setEdgeColor for edge ("<< s << ","<< t<<")"<<" with index ("<< index[s]<< ","<<index[t]<<")"<<" with color "<< color;
    m_graph[ index[s] ]->setOutLinkColor(t, color);
    if (isSymmetric()) {
        m_graph[ index[t] ]->setOutLinkColor(s, color);
    }
    graphModified=true;
    emit graphChanged();
}	



//Returns the edgeColor
QString Graph::edgeColor (long int s, long int t){
    return m_graph[ index[s] ]->outLinkColor(t);
}



/**	Checks if there is an edge from v1 to v2
    Complexity:  O(logN) for index retrieval + O(1) for QList index retrieval + O(logN) for checking edge(v2)
*/
float Graph::hasEdge (int v1, int v2) {		
    qDebug() << "Graph: hasEdge() " << v1 << " -> " << v2 << " ? " ;
    float weight=0;
    if ( ! m_graph[ index[v1] ] -> isEnabled() || ! m_graph[ index[v2] ] -> isEnabled())
        return 0;
    if ( (weight=  m_graph[ index[v1] ] -> isLinkedTo(v2) ) != 0 ) {
        //qDebug() << "Graph: hasEdge() between " <<  v1 << " " << index[v1] <<  " and " <<  v2 << " " << index[v2] << " = " << weight;
        return weight;
    }
    else {
        return 0;
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
    return m_graph[ index[v1] ]->outLinks();
}


/**
 * @brief Graph::inboundEdges
 * Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::inboundEdges (int v1) {
    qDebug("Graph: inboundEdges()");
    return m_graph[ index[v1] ]->inLinks();
}




/**
 * @brief Graph::outDegree
 * Returns the outDegree (sum of outLinks weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::outDegree (int v1) {
    qDebug("Graph: outDegree()");
    return m_graph[ index[v1] ]->outDegree();
}


/**
    Returns the inDegree (sum of inLinks weights) of vertex v1
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
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        tEdges+=(*it)->outLinks();
    }
    qDebug() << "Graph: m_totalEdges = " << m_totalEdges << ", tEdges=" <<  tEdges;
    return tEdges;
}


/**	
    Returns |V| of graph
*/
int Graph::vertices () { 
    qDebug("Graph: vertices()");
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
    int i=0, j=0;
    QList<Vertex*>::iterator it, it1;
    m_isolatedVerticesList.clear();
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        (*it)->setIsolated(true);
        if ( ! (*it)->isEnabled() )
            continue;
        j=i;
        for (it1=it; it1!=m_graph.end(); it1++){
            (*it1)->setIsolated(true);
            if ( ! (*it1)->isEnabled() )
                continue;
            if (i != j ) {
                if ( (this->hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
                    (*it)->setIsolated(false);
                    (*it1)->setIsolated(false);
                    // this check is needed for Graph::createDistanceMatrix()
                    // because we only run verticesIsolated() in the begining
                    // and we do not know if the matrix is symmetric.
                    // Symmetry test is needed for maxIndex*
                    if ( this->hasEdge ( (*it1)->name(), (*it)->name() ) == 0 )
                        symmetricAdjacencyMatrix=false;
                    // FIXME DO WE REALLY NEED TO CHECK TWICE!!! ???
                }
            }
            j++;
        }
        if ((*it)->isIsolated()) {
            m_isolatedVerticesList << i;
            qDebug()<< "Graph::verticesIsolated() - node " << i+1 << " is isolated. Marking it." ;
        }
        i++;
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
    QList<Vertex*>::iterator it, it1;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
       for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( ( this->hasEdge ( (*it1)->name(), (*it)->name() ) )  > 1  )   {
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
    discretePCs.clear(); discreteICs.clear();  discretePRCs.clear();
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
    notStronglyConnectedVertices.clear();
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
    calculatedCentralities=false;
    calculatedIRCC=false;
    calculatedPP=false;
    adjacencyMatrixCreated=false;
    reachabilityMatrixCreated=false;
    graphModified=false;
    symmetricAdjacencyMatrix=true;

    if (parser.isRunning() )		//tell the other thread that we must quit!
        parser.quit();

    if (crawler.isRunning() ){
        //tell the other thread that we must quit!
        crawler.terminateReaderQuit();
        crawler.quit();
    }
    qDebug("Graph: m_graph cleared. Now reports size %i", m_graph.size());
}



/**
 * @brief Graph::isSymmetric
 * Returns TRUE if the adjacency matrix of the current relation is symmetric
 * @return bool
 */
bool Graph::isSymmetric(){
    qDebug("Graph: isSymmetric ");
    if (!graphModified){
        return symmetricAdjacencyMatrix;
    }
    symmetricAdjacencyMatrix=true;
    int y=0, target=0, source=0;
    QHash<int,float> *enabledOutLinks = new QHash<int,float>;
    QHash<int,float>::const_iterator it1;
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        source = (*it)->name();
        if ( ! (*it)->isEnabled() )
            continue;
        qDebug() << "Graph::isSymmetric(): GRAPH Modified! " <<
                    " Iterate over all edges of " << source ;
        enabledOutLinks=(*it)->returnEnabledOutLinks();
        it1=enabledOutLinks->cbegin();
        while ( it1!=enabledOutLinks->cend() ){
            target = it1.key();
            y=index[ target ];
            if ( ! m_graph[y]->isLinkedTo( source)) {
                qDebug() << "Graph: isSymmetric: u = " << source
                         << " IS NOT inLinked from y = " <<  target  ;
                symmetricAdjacencyMatrix=false;
                qDebug("Graph: isSymmetric()  NO");
                break;
            }
            else {
                //	qDebug("Graph: isSymmetric():  u = %i IS inLinked from y = %i",it1->first, (*it)->name()  );
            }
            ++it1;
        }
    }
    delete enabledOutLinks;
    qDebug() << "Graph: isSymmetric()" << symmetricAdjacencyMatrix;
    return symmetricAdjacencyMatrix;
}



/**
*	Transform the digraph to undirected graph (all edges reciprocal)
*/
void Graph::symmetrize(){
    qDebug("Graph: symmetrize");
    QList<Vertex*>::iterator it;
    int y=0, target=0, source=0, weight;
    QHash<int,float> *enabledOutLinks = new QHash<int,float>;
    QHash<int,float>::const_iterator it1;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        source = (*it)->name();
        qDebug() << "Graph:symmetrize() - iterate over edges of source " << source;
        enabledOutLinks=(*it)->returnEnabledOutLinks();
        it1=enabledOutLinks->cbegin();
        while ( it1!=enabledOutLinks->cend() ){
            target = it1.key();
            weight = it1.value();
            y=index[ target ];
            qDebug() << "Graph:symmetrize() - "
                     << " source " << source
                     << " outLinked to " << target << " weight " << weight;
            if ( ! m_graph[y]->isLinkedTo( source )) {
                qDebug() << "Graph:symmetrize(): s = " << source
                         << " is NOT inLinked from y = " <<  target  ;
                createEdge( target, source, weight, initEdgeColor, false, true, false);
            }
            else
                qDebug() << "Graph: symmetrize(): source = " << source
                         << " is already inLinked from target = " << target ;
            ++it1;
        }
    }
    delete enabledOutLinks;
    graphModified=true;
    symmetricAdjacencyMatrix=true;
    emit graphChanged();
}


//Returns TRUE if (v1, v2) is symmetric.
bool Graph::symmetricEdge(int v1, int v2){
    qDebug("***Graph: symmetricEdge()");
    if ( (this->hasEdge ( v1, v2 ) ) > 0  &&  (this->hasEdge ( v2, v1 ) ) > 0   ) {
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
int Graph::distance(int i, int j){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false);
    }
    return DM.item(index[i],index[j]);
}



/**
*  Returns the diameter of the graph, aka the largest geodesic distance between any two vertices
*/
int Graph::diameter(){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false);
    }
    return graphDiameter;
}



/**
*  Returns the average distance of the graph
*/
float Graph::averageGraphDistance(){
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false);
    }
    return averGraphDistance;
}


/**
 * @brief Graph::connectedness()
 * @return int:
 * 1: connected graph or strongly connected digraph
*  0 => weakly connected digraph
*  -1 -> disconnected graph/digraph
 */
int Graph::connectedness(){
    qDebug() << "Graph::connectedness() ";
    if (!reachabilityMatrixCreated || graphModified) {
        reachabilityMatrix();
    }
    if ( (notStronglyConnectedVertices.size() != 0 ) && isolatedVertices == 0 )
     return 0;
    else if ( (notStronglyConnectedVertices.size() != 0 ) && isolatedVertices !=0 )
        return -1;
    return 1;
}


/**
*  Writes the matrix of distances to a file
*/
void Graph::writeDistanceMatrix (QString fn, const char* netName) {
    qDebug ("Graph::writeDistanceMatrix()");

    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false);
    }

    qDebug ("Graph::writeDistanceMatrix() writing to file");

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    out << "Distance matrix of "<< netName<<": \n";

    out << DM ;

    file.close();
}


/**
*  Saves the number of geodesic distances matrix TM to a file
*
*/
void Graph::writeNumberOfGeodesicsMatrix(const QString fn, const char* netName) {
    qDebug ("Graph::writeDistanceMatrix()");
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(false);
    }

    qDebug ("Graph::writeDistanceMatrix() writing to file");

    QFile file (fn);
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    out << "Number of geodesics matrix of  "<< netName<<": \n";

    out << TM ;

    file.close();

}



void Graph::writeEccentricity(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );
    if ( !distanceMatrixCreated || graphModified ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true);
    }
    emit statusMessage ( QString(tr("Writing eccentricity to file:")).arg(fileName) );

    outText << tr("ECCENTRICITY (e)") <<"\n";
    outText << tr("The eccentricity e of a node is the maximum geodesic distance "
                  " from that node to all other nodes in the network.") << "\n";
    outText << tr("Therefore, e reflects farness: how far, at most, is each "
                  " node from every other node.") << "\n";

    outText << tr("Range: 0 < e < ") << vertices()-1 <<" (g-1, "
             << tr("where g is the number of nodes |V|)\n")
             << tr("A node has maximum e when it has distance 1 "
                   "to all other nodes (star node))\n");

    outText << "Node"<<"\te\t\t%e\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->eccentricity() << "\t\t" <<
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
        boolean doCalculcateCentralities
    OUTPUT:
        DM(i,j)=geodesic distance between vertex i and vertex j
        TM(i,j)=number of shortest paths from vertex i to vertex j, called sigma(i,j).
        graphDiameter is set to the length of the longest shortest path between every (i,j)
        Eccentricity(i) is set to the length of the longest shortest path from i to every j
        Also, if doCalculcateCentralities==true, it calculates the centralities for every u in V:
        - Betweeness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
        - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
        - Eccentricity: EC(u) =  1/maxDistance(u,t)  for some t in V
        - Closeness: CC(u) =  1 / Sum( DM(u,t) )  for every  t in V
 * @param doCalculcateCentralities
 */
void Graph::createDistanceMatrix(bool doCalculcateCentralities) {
    qDebug ("Graph::createDistanceMatrix()");
    if ( !graphModified && distanceMatrixCreated && !doCalculcateCentralities)  {
        qDebug("Graph: distanceMatrix not mofified. Escaping.");
        return;
    }
    //Create a NxN DistanceMatrix. Initialise values to zero.
    qDebug() << "Graph::createDistanceMatrix() Resizing Matrices to hold "
             << m_totalVertices << " vertices";
    DM.resize(m_totalVertices);
    TM.resize(m_totalVertices);

    int aEdges = totalEdges();
    isolatedVertices = verticesIsolated().count();
    //drop isolated vertices from calculations (i.e. std Centrality and group Centrality).
    int aVertices=vertices() - isolatedVertices;

    if ( aEdges == 0 )
        DM.fillMatrix(0);
    else {
        qDebug() << "	for all vertices set their distances to -1 (infinum)";
        DM.fillMatrix(-1);
        qDebug () << "	for all vertices set their sigmas as 0";
        TM.fillMatrix(0);

        QList<Vertex*>::iterator it, it1;
        QList<int>::iterator it2;
        int w=0, u=0,s=0, i=0;
        float d_sw=0, d_su=0;
        float CC=0, BC=0, SC= 0, eccentricity=0, EC=0, PC=0;
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


        maxIndexBC=0;
        maxIndexSC=0;

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
        maxEccentricity=0; minEccentricity=RAND_MAX; maxNodeEccentricity=0;
        minNodeEccentricity=0; sumEccentricity=0; discreteEccentricities.clear();
        classesEccentricity=0;
        maxPC=0; minPC=RAND_MAX; maxNodePC=0; minNodePC=0; sumPC=0;
        discretePCs.clear(); classesPC=0;
        maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0;
        minNodeEC=0; sumEC=0;
        discreteECs.clear(); classesEC=0;

        //Zero closeness indeces of each vertex
        if (doCalculcateCentralities)
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                qDebug() << " Graph:createDistanceMatrix() - ZEROing all indices";
                (*it)->setBC( 0.0 );
                (*it)->setSC( 0.0 );
                (*it)->setEccentricity( 0.0 );
                (*it)->setEC( 0.0 );
                (*it)->setCC( 0.0 );
                (*it)->setPC( 0.0 );
            }
        qDebug("MAIN LOOP: for every s in V do (solve the single source shortest path problem...");
        for (it=m_graph.begin(); it!=m_graph.end(); it++){
            progressCounter++;
            emit updateProgressDialog( progressCounter );
            if ( ! (*it)->isEnabled() )
                continue;
            s=index[(*it)->name()];
            qDebug() << "Source vertex s = " << (*it)->name() << " of BFS algorithm has index " << s << ". Clearing Stack ...";
            if (doCalculcateCentralities){
                qDebug("Empty stack Stack which will return vertices in order of their (non increasing) distance from S ...");
                //- Complexity linear O(n)
                while ( !Stack.empty() )
                    Stack.pop();
                i=0;
                qDebug("...and for each vertex: empty list Ps of predecessors");
                for (it1=m_graph.begin(); it1!=m_graph.end(); it1++) {
                    (*it1)->clearPs();
                    //initialize all sizeOfNthOrderNeighborhood to zero
                    sizeOfNthOrderNeighborhood.insert(i, 0);
                    i++;
                }
            }

            qDebug() << "PHASE 1 (SSSP): Call BFS for source vertex " << (*it)->name()
                     << " to determine distances and shortest path counts from s to every vertex t" ;
            BFS(s,doCalculcateCentralities );
            qDebug("***** FINISHED PHASE 1 (SSSP) BFS ALGORITHM. Continuing to calculate centralities");

            if (doCalculcateCentralities){
                qDebug() << "Set centrality for current source vertex " << (*it)->name() << "  with index s = " << s ;
                if ( (*it)->CC() != 0 ) //Closeness centrality must be inverted
                    CC=1.0/(*it)->CC();
                else CC=0;
                //Resolve classes Closeness centrality
                qDebug("=========Resolving CC classes...");
                resolveClasses(CC, discreteCCs, classesCC,(*it)->name() );
                sumCC+=CC;
                (*it)->setCC( CC );

                //Check eccentricity (max geodesic distance)
                eccentricity = (*it)->eccentricity();
                if ( eccentricity != 0 ) {
                    //Eccentricity Centrality is the inverted Eccentricity
                    EC=1.0 / eccentricity;
                }
                else { EC=0;eccentricity=0;}
                (*it)->setEC( EC ); //Set Eccentricity Centrality = 0

                //Find min/max Eccentricity
                minmax( eccentricity, (*it), maxEccentricity, minEccentricity,
                        maxNodeEccentricity, minNodeEccentricity) ;
                resolveClasses(eccentricity, discreteEccentricities,
                               classesEccentricity ,(*it)->name() );
                sumEccentricity+=eccentricity;

                //Find min/max Eccentricity centrality
                minmax( EC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;
                sumEC+=EC;
                resolveClasses(EC, discreteECs, classesEC,(*it)->name() );

                i=1; //used in calculating power centrality
                sizeOfComponent = 1;
                PC=0;
                qDebug("PHASE 2 (ACCUMULATION): Start back propagation of dependencies."
                       "Set dependency delta[u]=0 on each vertex");
                for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
                    (*it1)->setDelta(0.0);
                    //Calculate Power Centrality: In = [ 1/(N-1) ] * ( Nd1 + Nd2 * 1/2 + ... + Ndi * 1/i )
                    // where Ndi (sizeOfNthOrderNeighborhood) is the number of nodes at distance i from this node.
                    PC += ( 1.0 / (float) i ) * sizeOfNthOrderNeighborhood.value(i);
                    // where N is the sum Nd0 + Nd1 + Nd2 + ... + Ndi, that is the amount of nodes in the same component as the current node
                    sizeOfComponent += sizeOfNthOrderNeighborhood.value(i);
                    i++;
                }
                (*it)->setPC( PC );		//Set Power Centrality
                sumPC += PC;
                //Find min & max PC - not using stdSC
                minmax( PC, (*it), maxPC, minPC, maxNodePC, minNodePC) ;
                resolveClasses(PC, discretePCs, classesPC,(*it)->name() );
                if ( sizeOfComponent != 1 )
                    PC = ( 1.0/(sizeOfComponent-1.0) ) * PC;
                else
                    PC = 0;
                (*it)->setSPC( PC );		//Set Standardized Power Centrality

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
        if (averGraphDistance!=0)
            averGraphDistance = averGraphDistance / (nonZeroDistancesCounter);

        if (doCalculcateCentralities) {
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                if (symmetricAdjacencyMatrix) {
                    qDebug("Betweeness centrality must be divided by two if the graph is undirected");
                    (*it)->setBC ( (*it)->BC()/2.0);
                }
                BC=(*it)->BC();
                qDebug("Resolving BC classes...");
                resolveClasses(BC, discreteBCs, classesBC);
                qDebug("******************* BC %f maxIndex: %f", BC, maxIndexBC);
                (*it)->setSBC( BC/maxIndexBC );
                //Find min & max BC - not using stdBC:  Wasserman & Faust, pp. 191-192
                sumBC+=BC;
                minmax( BC, (*it), maxBC, minBC, maxNodeBC, minNodeBC) ;

                qDebug()<< "Calculating Std Closeness centrality";
                CC = (*it)->CC();
                (*it)->setSCC ( maxIndexCC * CC  );
                minmax( (*it)->SCC(), (*it), maxCC, minCC, maxNodeCC, minNodeCC) ;

                qDebug("Resolving SC classes...");
                qDebug() << "SC OF " <<(*it)->name() << " is "<< (*it)->SC();
                SC=(*it)->SC();
                if (symmetricAdjacencyMatrix){
                    qDebug() << "SC OF " <<(*it)->name()
                             << " must be divided by 2 because the graph is symmetric";
                    (*it)->setSC(SC/2.0);
                    SC=(*it)->SC();
                    qDebug() << "SC OF " <<(*it)->name() << " now is "<< (*it)->SC();
                }
                resolveClasses(SC, discreteSCs, classesSC);
                sumSC+=SC;

            }
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                if ((*it)->isIsolated())
                    continue;
                BC=(*it)->BC();
                SC=(*it)->SC();

                qDebug()<< "Calculating Std Stress centrality";
                (*it)->setSSC( SC/sumSC );
                //Find min & max SC
                minmax( (*it)->SC(), (*it), maxSC, minSC, maxNodeSC, minNodeSC) ;

                //Calculate the numerator of groupBC according to Freeman's group Betweeness
                nomBC +=(maxBC - BC );

                //Find numerator of groupCC
                nomCC += maxCC- (*it)->SCC();

            }
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                if ((*it)->isIsolated())
                    continue;
                //Calculate numerator of groupSC
                nomSC +=( maxSC - (*it)->SC() );

            }

            denomCC = ( ( aVertices-1.0) * (aVertices-2.0) ) / (2.0 * aVertices -3.0);
            groupCC = nomCC/denomCC;	//Calculate group Closeness centrality

            nomBC*=2.0;
            denomBC =   (aVertices-1.0) *  (aVertices-1.0) * (aVertices-2.0);
            groupBC=nomBC/denomBC;		//Calculate group Betweeness centrality

            denomSC =   (aVertices-1.0); //TOFIX
            groupSC = nomSC/denomSC;	//Calculate group Stress centrality
            calculatedCentralities=true;
        }
    }

    distanceMatrixCreated=true;
}





/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

    INPUT:
        a 'source' vertex with index s and a boolean doCalculcateCentralities.
        (Implicitly, BFS uses the m_graph structure)

    OUTPUT:
        For every vertex t: DM(s, t) is set to the distance of each t from s
        For every vertex t: TM(s, t) is set to the number of shortest paths between s and t

        Also, if doCalculcateCentralities is true then BFS does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from Q is pushed to a stack Stack

*/ 
void Graph::BFS(int s, bool doCalculcateCentralities){
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

//    qDebug("BFS: LOOP: While Q not empty ");
    while ( !Q.empty() ) {
//        qDebug("BFS: Dequeue: first element of Q is u=%i", Q.front());
        u=Q.front(); Q.pop();

        if ( ! m_graph [ u ]->isEnabled() ) continue ;

        if (doCalculcateCentralities){
//            qDebug("BFS: If we are to calculate centralities, we must push u=%i to global stack Stack ", u);
            Stack.push(u);
        }
//        qDebug() << "BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u";
        it1=m_graph [ u ] ->m_outLinks.cbegin();
        while ( it1!=m_graph [ u ] -> m_outLinks.cend() ){
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
//            qDebug("BFS: u=%i is connected with node %i of index w=%i. ", u, target, w);
//            qDebug("BFS: Start path discovery");
            if (	DM.item(s, w) == -1 ) { //if distance (s,w) is infinite, w found for the first time.
//                qDebug("BFS: first time visiting w=%i. Enqueuing w to the end of Q", w);
                Q.push(w);
//                qDebug()<<"BFS: First check if distance(s,u) = -1 (aka infinite :)) and set it to zero";
                dist_u=DM.item(s,u);
                if (dist_u < 0 )
                    dist_w = 0;
                else
                    dist_w = dist_u + 1;
//                qDebug("BFS: Setting distance of w=%i from s=%i equal to distance(s,u) plus 1. New distance = %i",w,s, dist_w );
                DM.setItem(s, w, dist_w);
                averGraphDistance += dist_w;
                nonZeroDistancesCounter++;

                if (doCalculcateCentralities){
//                    qDebug()<<"BFS: Calculate PC: store the number of nodes at distance " << dist_w << "from s";
                    sizeOfNthOrderNeighborhood.insert(
                                dist_w,
                                sizeOfNthOrderNeighborhood.value(dist_w)+1
                                );
//                    qDebug()<<"BFS: Calculate CC: the sum of distances (will invert it l8r)";
                    m_graph [s]->setCC (m_graph [s]->CC() + dist_w);
//                    qDebug()<<"BFS: Calculate Eccentricity: the maximum distance ";
                    if (m_graph [s]->eccentricity() < dist_w )
                        m_graph [s]->setEccentricity(dist_w);

                }
//                qDebug("BFS: Checking graphDiameter");
                if ( dist_w > graphDiameter){
                    graphDiameter=dist_w;
//                    qDebug() << "BFS: new graphDiameter = " <<  graphDiameter ;
                }
            }

//            qDebug("BFS: Start path counting"); 	//Is edge (u,w) on a shortest path from s to w via u?
            if ( DM.item(s,w)==DM.item(s,u)+1) {
                temp= TM.item(s,w)+TM.item(s,u);
//                qDebug("BFS: Found a NEW SHORTEST PATH from s=%i to w=%i via u=%i. Setting Sigma(%i, %i) = %i",s, w, u, s, w,temp);
                if (s!=w)
                    TM.setItem(s,w, temp);
                if (doCalculcateCentralities){
//                    qDebug("BFS/SC: If we are to calculate centralities, we must calculate SC as well");
                    if ( s!=w && s != u && u!=w ) {
//                        qDebug() << "BFS: setSC of u="<<u<<" to "<<m_graph[u]->SC()+1;
                        m_graph[u]->setSC(m_graph[u]->SC()+1);
                    }
                    else {
//                        qDebug() << "BFS/SC: skipping setSC of u, because s="
//                                 <<s<<" w="<< w << " u="<< u;
                    }
//                    qDebug() << "BFS/SC: SC is " << m_graph[u]->SC();
//                    qDebug() << "BFS: appending u="<< u << " to list Ps[w=" << w
//                             << "] with the predecessors of w on all shortest paths from s ";
                    m_graph[w]->appendToPs(u);
                }
            }
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

void Graph::centralityInformation(){
    qDebug()<< "Graph:: centralityInformation()";
    discreteICs.clear();
    sumIC=0;
    maxIC=0;
    minIC=RAND_MAX;
    classesIC=0;
    groupIC=0;

    TM.resize(m_totalVertices);
    isolatedVertices=0;
    int i=0, j=0, n=vertices();
    float m_weight=0, weightSum=1, diagonalEntriesSum=0, rowSum=0;
    float IC=0, SIC=0, sumSIC=0;;
    /* Note: isolated nodes must be dropped from the AM
        Otherwise, the TM might be singular, therefore non-invertible. */
    bool dropIsolates=true;
    bool omitWeights=false;

    //TODO ASK THE USER TO SYMMETRIZE GRAPH?
    createAdjacencyMatrix(dropIsolates, omitWeights);
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


    QList<Vertex*>::iterator it;
    i=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( (*it)->isIsolated() ) {
            (*it) -> setIC ( 0 );
            qDebug()<< "Graph:: centralityInformation() vertex: " <<  (*it)->name() << " isolated";
            continue;
        }
        IC= 1.0 / ( invM.item(i,i) + (diagonalEntriesSum - 2.0 * rowSum) / n );
        if ( IC > maxIC ) {
            maxIC = IC;
            maxNodeIC=(*it)->name();
        }
        if ( IC < minIC ) {
            minIC = IC;
            minNodeIC=(*it)->name();
        }

        (*it) -> setIC ( IC );
        sumIC += IC;
        qDebug()<< "Graph:: centralityInformation() vertex: " <<  (*it)->name() << " IC  " << IC ;
        i++;
    }
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IC = (*it)->IC();
        SIC = IC / sumIC ;
        (*it)->setSIC( SIC );
    }

    float x=0;
    averageIC = sumSIC / n ;
    qDebug() << "sumSIC = " << sumSIC << "  n = " << n << "  averageIC = " << averageIC;
    groupIC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        x = (  (*it)->SIC()  -  averageIC  ) ;
        x *=x;
        qDebug() << "SIC " <<  (*it)->SIC() << "  x " <<   (*it)->SIC() - averageIC  << " x*x" << x ;
        groupIC  += x;
    }
    qDebug() << "groupIC   " << groupIC   << " n " << n ;
    groupIC  = groupIC  /  (n);
    qDebug() << "groupIC   " << groupIC   ;


}



//Writes the information centralities to a file
void Graph::writeCentralityInformation(const QString fileName){
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating information centralities. Please wait...")) );
    centralityInformation();
    emit statusMessage ( QString(tr("Writing information centralities to file: "))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("INFORMATION CENTRALITY (IC)")<<"\n";
    outText << tr("The IC index measures the information that is contained in "
                  "the paths passing through each actor.\n");
    outText << tr("IC' is the standardized IC")<<"\n";
    outText << tr("The standardized values IC' can be seen as the proportion "
                  "of total information flow that is controlled by each actor. "
                  "Note that standard IC' values sum to unity, unlike most "
                  "other centrality indices.\n");
    outText << "(Wasserman & Faust, p. 196)\n";

    outText << tr("IC  range:  0 < C < inf (this index has no max value)") << "\n";
    outText << tr("IC' range:  0 < C'< 1")<<"\n\n";
    outText << "Node"<<"\tIC\t\tIC'\t\t%IC\n";
    QList<Vertex*>::iterator it;
    float IC=0, SIC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IC = (*it)->SIC();
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

    outText << tr("\nGROUP INFORMATION CENTRALISATION (GIC)\n\n");
    outText << tr("GIC = ") << groupIC<<"\n\n";
    outText << tr("GIC range: 0 < GIC < inf \n");
    outText << tr("GIC is computed using a simple variance formula. \n");
    outText << tr("In fact, following the results of Wasserman & Faust, we are "
                  "using a bias-corrected sample variance.\n ");

    outText << tr("GIC = 0, when all nodes have the same IC value, i.e. a "
                  "complete or a circle graph).\n");
    outText << tr("Larger values of GIC mean larger variability between the "
                  "nodes' IC values.\n");
    outText <<"(Wasserman & Faust, formula 5.20, p. 197)\n\n";


    outText << tr("Information Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}





//Calculates the outDegree centrality of each vertex - diagonal included
void Graph::centralityDegree(bool weights){
    qDebug("Graph:: centralityDegree()");
    if (!graphModified && calculatedDC ) {
        qDebug() << "Graph::centralityDegree() - graph not changed - returning";
        return;
    }

    float DC=0, nom=0, denom=0;
    float weight;
    classesDC=0;
    discreteDCs.clear();
    sumDC=0;
    maxDC=0;
    minDC=vertices()-1;
    varianceDegree=0;
    meanDegree=0;
    int vert=vertices();
    QList<Vertex*>::iterator it, it1;
    H_StrToInt::iterator it2;

    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        DC=0;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( (weight=this->hasEdge ( (*it)->name(), (*it1)->name() ) ) !=0  )   {
                qDebug() << "Graph: vertex " <<  (*it)->name() << " isLinkedTo = " <<  (*it1)->name();
                if (weights)
                    DC+=weight;
                else
                    DC++;
                //check here if the matrix is symmetric - we need this below
                if ( ( this->hasEdge ( (*it1)->name(), (*it)->name() ) ) !=
                     ( this->hasEdge ( (*it)->name(), (*it1)->name() ) )   )
                    symmetricAdjacencyMatrix = false;
            }
        }
        (*it) -> setDC ( DC ) ;				//Set OutDegree
        qDebug() << "Graph: vertex " <<  (*it)->name() << " has DC = " << DC ;
        sumDC += DC;
        it2 = discreteDCs.find(QString::number(DC));
        if (it2 == discreteDCs.end() )	{
            classesDC++;
            qDebug("This is a new DC class");
            discreteDCs.insert ( QString::number(DC), classesDC );
        }
        qDebug("DC classes = %i ", classesDC);
        if (maxDC < DC ) {
            maxDC = DC ;
            maxNodeDC=(*it)->name();
        }
        if (minDC > DC ) {
            minDC = DC ;
            minNodeDC=(*it)->name();
        }
    }

    if (minDC == maxDC)
        maxNodeDC=-1;

    meanDegree = sumDC / (float) vert;
    qDebug("Graph: sumDC = %f, meanDegree = %f", sumDC, meanDegree);

    // Calculate std Out-Degree, Variance and the Degree Centralisation of the whole graph.
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        DC= (*it)->DC();
        if (!weights) {
            (*it) -> setSDC( DC / (vert-1.0) );		//Set Standard InDegree
        }
        else {
            (*it) -> setSDC( DC / (sumDC) );
        }
        nom+= (maxDC-DC);
        qDebug() << "Graph: vertex " <<  (*it)->name() << " SDC " << (*it)->SDC ();

        varianceDegree += (DC-meanDegree) * (DC-meanDegree) ;
    }
    varianceDegree=varianceDegree/(float) vert;

    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);

    if (!weights) {
        groupDC=nom/denom;
    }
    else {
        qDebug()<< "Graph::centralityDegree vertices isolated: " << verticesIsolated().count() << ". I will subtract groupDC by " << ((float)verticesIsolated().count()/(float)vert);
        groupDC=( ( nom * (vert-1.0))/( denom * maxDC) ) - ((float) verticesIsolated().count()/ (float) vert);
    }

    qDebug("Graph: varianceDegree = %f, groupDC = %f", varianceDegree, groupDC);

    if (!weights) {
        minDC/=(float)(vert-1); // standardize
        maxDC/=(float)(vert-1);
    }
    else {
        minDC/=(float)(sumDC); // standardize
        maxDC/=(float)(sumDC);
    }

    calculatedDC=true;
}




void Graph::writeCentralityDegree (
        const QString fileName, const bool considerWeights)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    centralityDegree(considerWeights);

    float maximumIndexValue=vertices()-1.0;
    outText.setRealNumberPrecision(m_precision);
    outText << tr("DEGREE CENTRALITY (DC)\n");
    outText << tr("In undirected graphs, the DC index is the sum of edges "
                  "attached to a node u.\n");
    outText << tr("In digraphs, the index is the sum of outbound links of "
                  "node u to all adjacent nodes.\n");
    outText << tr("If the network is weighted, the DC is the sum of outbound "
                  "link weights of node u to all adjacent nodes.\n");
    outText << tr("DC' is the standardized DC\n\n");

    if (considerWeights){
        maximumIndexValue=(vertices()-1.0)*maxDC;
        outText << tr("DC  range: 0 < C < undefined (since this is a weighted network)")<<"\n";
    }
    else
        outText << tr("DC  range: 0 < C < ")<<QString::number(maximumIndexValue)<<"\n";


    outText << "DC' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tDC\tDC'\t%DC\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"
                   <<(*it)->DC() << "\t"<< (*it)->SDC() << "\t"
                  <<  (100* ((*it)->DC()) / sumDC)<< "\n";
    }
    if (symmetricAdjacencyMatrix) {
        outText << "\n";
        outText << tr("Mean Node Degree = ") << meanDegree<<"\n" ;
        outText << tr("Degree Variance = ") << varianceDegree<<"\n";
    }
    else{
        outText << "\n" ;
        outText << tr("Mean Node OutDegree = ") << meanDegree<<"\n" ;
        outText << tr("OutDegree Variance = ") << varianceDegree<<"\n";
    }
    if ( minDC == maxDC )
        outText << tr("All nodes have the same DC value.") << "\n";
    else  {
        outText << tr("Max DC' = ") << maxDC <<" (node "<< maxNodeDC <<  ")  \n";
        outText << tr("Min DC' = ") << minDC <<" (node "<< minNodeDC <<  ")  \n";
        outText << tr("DC classes = ") << classesDC<<" \n";
    }

    outText << "\nGROUP OUT-DEGREE CENTRALISATION (GDC)\n\n";
    outText << "GDC = " << qSetRealNumberPrecision(m_precision) << groupDC<<"\n\n";

    outText << "GDC range: 0 < GDC < 1\n";
    outText << "GDC = 0, when all out-degrees are equal (i.e. regular lattice).\n";
    outText << "GDC = 1, when one node completely dominates or overshadows the other nodes.\n";
    outText << "(Wasserman & Faust, formula 5.5, p. 177)\n\n";
    outText << "(Wasserman & Faust, p. 101)\n";

    if (considerWeights) {
        outText << "\nNOTE: Because the network is weighted, we normalize Group "
                   "Centrality multiplying by (N-1)/maxDC, where N is the total "
                   "vertices, and subtracting the percentage of isolated vertices\n";
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
void Graph::centralityClosenessInfluenceRange(){
    qDebug()<< "Graph::centralityClosenessImproved()";
    if (!graphModified && calculatedIRCC ) {
        qDebug() << "Graph::centralityClosenessImproved() - "
                    " graph not changed - returning";
        return;
    }
     if (!reachabilityMatrixCreated || graphModified) {
         qDebug()<< "Graph::centralityClosenessImproved() - "
                    "call reachabilityMatrix()";
        reachabilityMatrix();
     }
    // calculate centralities
    QList<Vertex*>::iterator it;
    float IRCC=0;
    float Ji=0;
    classesIRCC=0;
    discreteIRCCs.clear();
    sumIRCC=0;
    maxIRCC=0;
    minIRCC=vertices()-1;
    float V=vertices();
    varianceIRCC=0;
    meanIRCC=0;
    H_StrToInt::iterator it2;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IRCC=0;
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
        sumIRCC += IRCC;
        (*it) -> setIRCC ( IRCC ) ;
        qDebug() << "Graph::centralityClosenessImproved - vertex " <<  (*it)->name()
                 << " has IRCC = "
                 << Ji / (V-1) << " / " << IRCC << " = " << (*it)->IRCC();

        it2 = discreteIRCCs.find(QString::number(IRCC));
        if (it2 == discreteIRCCs.end() )	{
            classesIRCC++;
            qDebug("This is a new IRCC class");
            discreteIRCCs.insert ( QString::number(IRCC), classesIRCC );
        }
        qDebug("IRCC classes = %i ", classesIRCC);
        if (maxIRCC < IRCC ) {
            maxIRCC = IRCC ;
            maxNodeIRCC=(*it)->name();
        }
        if (minIRCC > IRCC ) {
            minIRCC = IRCC ;
            minNodeIRCC=(*it)->name();
        }

    }

    if (minIRCC == maxIRCC)
        maxNodeIRCC=-1;

    meanIRCC = sumIRCC / (float) V;
    qDebug("Graph::centralityClosenessImproved - sumIRCC = %f, meanIRCC = %f",
           sumIRCC, meanIRCC);

    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IRCC= (*it) -> IRCC();
        varianceIRCC += (IRCC-meanIRCC) * (IRCC-meanIRCC) ;
        (*it) -> setSIRCC ( IRCC / sumIRCC) ;
        qDebug() << "Graph::centralityClosenessImproved - vertex " <<  (*it)->name()
                 << " has Std IRCC = "
                 << IRCC << " / " << sumIRCC << " = " << (*it)->SIRCC();

    }

    varianceIRCC=varianceIRCC/(float) V;
    calculatedIRCC=true;

}

//Writes the closeness centralities to a file
void Graph::writeCentralityCloseness(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);

    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );


    if (graphModified || !calculatedCentralities ) {
            emit statusMessage ( (tr("Calculating shortest paths")) );
            createDistanceMatrix(true);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing closeness indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("CLOSENESS CENTRALITY (CC)")<<"\n";
    outText << tr("The CC index is the inverted sum of geodesic distances "
                  " from node u to all the other nodes.")<<"\n";
    outText << tr("This measure focuses on how close a node is to all "
                  "the other nodes in the set of nodes. The idea is that a node "
                  "is central if it can quickly interact with all others\n");

    outText << tr("CC' is the standardized CC (multiplied by N-1 minus isolates).")<<"\n";
    outText << tr("Note: In not strongly connected graphs or digraphs, "
                  "the ordinary CC is undefined. In that case, use "
                  "the Influence Range Closeness Centrality index.\n");

    outText << tr("CC  range:  0 < C < ")<<QString::number(maxIndexCC)<<"\n";
    outText << tr("CC' range:  0 < C'< 1")<<"\n\n";
    outText << "Node"<<"\tCC\t\tCC'\t\t%CC\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->CC() << "\t\t"
                   << (*it)->SCC() << "\t\t"
                   <<  (100* ((*it)->CC()) / sumCC)<<endl;
    }
    qDebug ("min %f, max %f", minCC, maxCC);
    if ( minCC == maxCC )
        outText << tr("\nAll nodes have the same CC value.\n");
    else  {
        outText << "\n";
        outText << tr("Max CC' = ") << maxCC <<" (node "<< maxNodeCC  <<  ")  \n";
        outText << tr("Min CC' = ") << minCC <<" (node "<< minNodeCC <<  ")  \n";
        outText << tr("CC classes = ") << classesCC<<" \n";
    }

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

    outText << "\n\n";
    outText << tr("Closeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




//Writes the "improved" closeness centrality indices to a file
void Graph::writeCentralityClosenessInfluenceRange(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("calculating IRCC indices")) );

    centralityClosenessInfluenceRange();

    emit statusMessage ( QString(tr("Writing IR closeness indices to file:")
                         .arg(fileName) ));
    outText.setRealNumberPrecision(m_precision);
    outText << tr("INFLUENCE RANGE CLOSENESS CENTRALITY (IRCC)")<<"\n";
    outText << tr(
                   "The IRCC index is the ratio of the fraction of nodes "
                   "reachable by u to the average distance of these nodes from u.\n"
                   "This improved Closeness Centrality index is optimized for "
                   "graphs and directed graphs which are not strongly connected.\n"
                   "Unlike the ordinary CC, which is the inverted sum of "
                   "distances from node u to all others (thus undefined if a "
                   "node is isolated or the digraph is not strongly connected), "
                   "the IRCC index considers only distances from node u to nodes "
                   "in its influence range J (nodes reachable from u).\n ");
    outText <<"(Wasserman & Faust, formula 5.22, p. 201)\n\n";

    outText << tr("IRCC  range:  0 < IRCC < 1 ")
            <<" (IRCC is a ratio)\n";
    outText << tr("IRCC' is the standardized IRCC (divided by sumIRCC). \n\n");
    outText << "Node"<<"\tIRCC\t\tIRCC'\t\t%IRCC\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->IRCC() << "\t\t"<< (*it)->SIRCC() << "\t\t" <<  (100* ((*it)->SIRCC()) )<<endl;
    }
    qDebug ("min %f, max %f", minIRCC, maxIRCC);
    if ( minIRCC == maxIRCC )
        outText << tr("\nAll nodes have the same IRCC value.\n");
    else  {
        outText << "\n";
        outText << tr("Max IRCC = ") << maxIRCC <<" (node "<< maxNodeIRCC  <<  ")  \n";
        outText << tr("Min IRCC = ") << minIRCC <<" (node "<< minNodeIRCC <<  ")  \n";
        outText << tr("IRCC classes = ") << classesIRCC<<" \n";
    }
    outText << tr("Mean IRCC = ") << meanIRCC<<"\n";
    outText << tr("Sum IRCC= ") << sumIRCC<<"\n";
    outText << tr("Variance IRCC = ") << varianceIRCC<<"\n\n";


    outText << "\n\n";
    outText << tr("InfluenceRange Closeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}


//Writes the betweeness centralities to a file
void Graph::writeCentralityBetweeness(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );


    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing betweeness indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("BETWEENESS CENTRALITY (BC)")<<"\n";
    outText << tr("The BC index of a node u is the sum of delta (s,t,u) for all s,t in V")<<"\n";
    outText << tr("where delta (s,t,u) is the ratio of all geodesics between "
                  "s and t which run through u.")<<"\n";
    outText << tr("Therefore, the BC value reflects how often the node u lies "
                  "on the geodesics between the other nodes of the network")<<"\n";
    outText << tr("BC' is the standardized BC")<<"\n";
    outText << tr("BC  range: 0 < BC < ")<<QString::number( maxIndexBC)
            << tr(" (Number of pairs of nodes excluding u)")<<"\n";
    outText << tr("BC' range: 0 < BC'< 1  (C' is 1 when the node falls on all geodesics)\n\n");
    outText << "Node"<<"\tBC\t\tBC'\t\t%BC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->BC()
               << "\t\t"<< (*it)->SBC() << "\t\t"
               <<  (100* ((*it)->BC()) /  sumBC)<<endl;
    }
    if ( minBC ==  maxBC)
        outText << "\n"<< tr("All nodes have the same BC value.\n");
    else {
        outText << "\n";
        outText << tr("Max BC = ") << maxBC <<" (node "<< maxNodeBC  <<  ")  \n";
        outText << tr("Min BC = ") << minBC <<" (node "<< minNodeBC <<  ")  \n";
        outText << tr("BC classes = ") << classesBC<<" \n";
    }

    outText << tr("\nGROUP BETWEENESS CENTRALISATION (GBC)\n\n");
    outText << tr("GBC = ") <<  groupBC <<"\n\n";

    outText << tr("GBC range: 0 < GBC < 1\n");
    outText << tr("GBC = 0, when all the nodes have exactly the same betweeness index.\n");
    outText << tr("GBC = 1, when one node falls on all other geodesics between "
                  "all the remaining (N-1) nodes. "
                  "This is exactly the situation realised by a star graph.\n");
    outText << "(Wasserman & Faust, formula 5.13, p. 192)\n\n";

    outText << "\n\n";
    outText << tr("Betweeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



//Writes the Stress centralities to a file
void Graph::writeCentralityStress(
        const QString fileName, const bool considerWeights)
{	
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }

    emit statusMessage ( QString(tr("Writing stress indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("STRESS CENTRALITY (SC)")<<"\n";
    outText << tr("SC(u) is the sum of sigma(s,t,u): the number of geodesics "
                  "from s to t through u.")<<"\n";
    outText << tr("The SC index reflects the total number of geodesics between all "
                  "other nodes which run through u")<<"\n";

    outText << tr("SC  range: 0 < SC < ")<<QString::number(maxIndexSC)<<"\n";
    outText << tr("SC' range: 0 < SC'< 1  (SC'=1 when the node falls on all "
                  "geodesics)\n\n");
    outText  << "Node"<<"\tSC\t\tSC'\t\t%SC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->SC() << "\t\t"
               << (*it)->SSC() << "\t\t"
               <<  (100* ((*it)->SC()) /  sumSC)<<endl;
    }

    if ( minSC ==  maxSC)
        outText  << tr("\nAll nodes have the same SC value.\n");
    else {
        outText << "\n";
        outText << tr("Max SC = ") << maxSC <<" (node "<< maxNodeSC  <<  ")  \n";
        outText << tr("Min SC = ") << minSC <<" (node "<< minNodeSC <<  ")  \n";
        outText << tr("SC classes = ") << classesSC<<" \n";
    }

    outText << tr("GROUP STRESS CENTRALISATION (GSC)")<<"\n";
    outText << tr("GSC = ") <<  groupSC<<"\n\n";

    outText << tr("GSC range: 0 < GSC < 1\n");
    outText << tr("GSC = 0, when all the nodes have exactly the same stress index.\n");
    outText << tr("GSC = 1, when one node falls on all other geodesics between "
                  "all the remaining (N-1) nodes. "
                  "This is exactly the situation realised by a star graph.\n");

    outText << "\n\n";
    outText << tr("Stress Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



void Graph::writeCentralityEccentricity(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true);
    }
    else {
        qDebug() << " graph not modified, and centralities calculated. Returning";
    }
    emit statusMessage ( QString(tr("Writing eccentricity indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("ECCENTRICITY CENTRALITY (EC)") << "\n";
    outText << tr("The EC indx of a node is the inverse maximum geodesic distance "
                  " from that node to all other nodes in the network.") << "\n";
    outText << tr("Therefore, the EC value reflects farness: how far, at most, is each "
                  " node from every other node.") << "\n";
    outText << tr("Nodes with very high EC index have short distances to all other "
                  "nodes in the graph.")<< "\n";
    outText << tr("Nodes with very low EC index have longer distances to some other "
                   "nodes in the graph.")<< "\n";
    outText << tr("GC  range: 0 < EC < 1 (GC=1 => max distance to all other nodes is 1)\n");

    outText << "Node"<<"\tEC\t\t%EC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->EC() << "\t\t"
                <<  (100* ((*it)->EC()) /  sumEC)<<endl;
    }
    if ( minEC ==  maxEC)
        outText << tr("\nAll nodes have the same EC value.\n");
    else {
        outText << "\n";
        outText << tr("Max EC = ") << maxEC <<" (node "<< maxNodeEC  <<  ")  \n";
        outText << tr("Min EC = ") << minEC <<" (node "<< minNodeEC <<  ")  \n";
        outText << tr("EC classes = ") << classesEC<<" \n";
    }

    outText << "\n\n";
    outText << tr("Eccentricity Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




void Graph::writeCentralityPower(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    if (graphModified || !calculatedCentralities ) {
        emit statusMessage ( (tr("Calculating shortest paths")) );
        createDistanceMatrix(true);
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
    outText << tr("Therefore, PC(u) is a generalised degree centrality index.")
            << "\n";
    outText << tr("PC' is the standardized index; divided by the total numbers "
                  "of nodes in the same component minus 1") << "\n";
    outText << tr("PC  range: 0 < PC < ") << QString::number(maxIndexPC)
            << tr(" (star node)")<<"\n";
    outText << tr("PC' range: 0 < PC'< 1 \n\n");
    outText << "Node"<<"\tPC\t\tPC'\t\t%PC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->PC() << "\t\t"
                << (*it)->SPC() << "\t\t"
                <<  (100* ((*it)->PC()) /  sumPC)<<endl;
    }
    if ( minPC ==  maxPC)
        outText << tr("\nAll nodes have the same PC value.\n");
    else {
        outText << "\n";
        outText << tr("Max PC' = ") << maxPC <<" (node "<< maxNodePC  <<  ")  \n";
        outText << tr("Min PC' = ") << minPC <<" (node "<< minNodePC <<  ")  \n";
        outText << tr("PC classes = ") << classesPC<<" \n";
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
void Graph::prestigeDegree(bool weights){
    qDebug()<< "Graph:: prestigeDegree()";
    if (!graphModified && calculatedDP ) {
        qDebug() << "Graph::prestigeDegree() - "
                    " graph not changed - returning";
        return;
    }
    float DP=0, nom=0, denom=0;
    float weight;
    classesDP=0;
    sumDP=0;
    maxDP=0;
    minDP=vertices()-1;
    discreteDPs.clear();
    varianceDegree=0;
    meanDegree=0;
    symmetricAdjacencyMatrix = true;
    QList<Vertex*>::iterator it, it1;
    H_StrToInt::iterator it2;
    int vert=vertices();
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        DP=0;
        qDebug() << "Graph: prestigeDegree() vertex " <<  (*it)->name()  ;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( (weight=this->hasEdge ( (*it1)->name(), (*it)->name() ) ) !=0  )   {
                if (weights)
                    DP+=weight;
                else
                    DP++;
            }
            //check here if the matrix is symmetric - we need this below
            if ( ( this->hasEdge ( (*it1)->name(), (*it)->name() ) ) != ( this->hasEdge ( (*it)->name(), (*it1)->name() ) )   )
                symmetricAdjacencyMatrix = false;
        }
        (*it) -> setDP ( DP ) ;				//Set InDegree
        qDebug() << "Graph: vertex = " <<  (*it)->name() << " has DP = " << DP ;
        sumDP += DP;
        it2 = discreteDPs.find(QString::number(DP));
        if (it2 == discreteDPs.end() )	{
            classesDP++;
            qDebug("This is a new DP class");
            discreteDPs.insert ( QString::number(DP), classesDP );
        }
        qDebug("DP classes = %i ", classesDP);
        if (maxDP < DP ) {
            maxDP = DP ;
            maxNodeDP=(*it)->name();
        }
        if (minDP > DP ) {
            minDP = DP ;
            minNodeDP=(*it)->name();
        }
    }

    if (minDP == maxDP)
        maxNodeDP=-1;


    meanDegree = sumDP / (float) vert;
    qDebug("Graph: sumDP = %f, meanDegree = %f", sumDP, meanDegree);

    // Calculate std In-Degree, Variance and the Degree Centralisation of the whole graph.
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        DP= (*it)->DP();
        if (!weights) {
            (*it) -> setSDP( DP / (vert-1.0) );		//Set Standard InDegree
        }
        else {
            (*it) -> setSDP( DP / (sumDP) );
        }
        nom+= maxDP-DP;
        qDebug() << "Graph: vertex = " <<  (*it)->name() << " has DP = " << DP << " and SDP " << (*it)->SDP ();

        //qDebug("Graph: DP = %f, meanDegree = %f", DP, meanDegree);
        varianceDegree += (DP-meanDegree) * (DP-meanDegree) ;
    }

    varianceDegree=varianceDegree/(float) vert;

    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);

    if (!weights) {
        groupDP=nom/denom;
    }
    else {
        qDebug()<< "Graph::prestigeDegree vertices isolated: " << verticesIsolated().count() << ". I will subtract groupDP by " << ((float)verticesIsolated().count()/(float)vert);
        groupDP=( ( nom * (vert-1.0))/( denom * maxDP) ) - ((float) verticesIsolated().count()/ (float) vert);
    }

    qDebug("Graph: varianceDegree = %f, groupDP = %f", varianceDegree, groupDP);

    if (!weights) {
        minDP/=(float)(vert-1); // standardize
        maxDP/=(float)(vert-1);
    }
    else {
        minDP/=(float)(sumDP); // standardize
        maxDP/=(float)(sumDP);
    }
    calculatedDP=true;
}




void Graph::writePrestigeDegree (const QString fileName, const bool considerWeights)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    prestigeDegree(considerWeights);

    float maximumIndexValue=vertices()-1.0;
    outText.setRealNumberPrecision(m_precision);
    outText << tr("DEGREE PRESTIGE (DP)\n");
    outText << tr("The DP index of a node u is the sum of incoming links to "
                  "that node from all adjacent nodes.\n");
    outText << tr("If the network is weighted, DP is the sum of incoming link "
                  "weights (inDegree) to node u from all adjacent nodes.\n");
    outText << tr("The DP of a node is a measure of how prestigious it is.\n");
    outText << tr("DP' is the standardized DP\n\n");
    if (considerWeights){
        maximumIndexValue=(vertices()-1.0)*maxDP;
        outText << tr("DP  range: 0 < C < undefined (since this is a weighted network")<<"\n";
    }
    else
        outText << tr("DP  range: 0 < C < ")<<maximumIndexValue<<"\n";
    outText << "DP' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tDP\tDP'\t%DP\n";

    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->DP() << "\t"<< (*it)->SDP() << "\t" <<  (100* ((*it)->DP()) / sumDP)<<endl;
    }
    if (symmetricAdjacencyMatrix) {
        outText << "\n";
        outText << tr("Mean Nodal Degree = ") << meanDegree<<"\n" ;
        outText << tr("Degree Variance = ") << varianceDegree<<"\n";
    }
    else{
        outText << "\n";
        outText << tr("Mean Nodal InDegree = ") << meanDegree<<"\n" ;
        outText << tr("InDegree Variance = ") << varianceDegree<<"\n";
    }

    if ( minDP == maxDP )
        outText << tr("All nodes have the same DP value.") << "\n";
    else  {
        outText << tr("Max DP' = ") << maxDP <<" (node "<< maxNodeDP <<  ")  \n";
        outText << tr("Min DP' = ") << minDP <<" (node "<< minNodeDP <<  ")  \n";
        outText << tr("DP classes = ") << classesDP<<" \n";
    }

    outText << "\nGROUP DEGREE PRESTIGE (GDP)\n\n";
    outText << "GDP = " << groupDP<<"\n\n";

    outText << "GDP range: 0 < GDP < 1\n";
    outText << "GDP = 0, when all in-degrees are equal (i.e. regular lattice).\n";
    outText << "GDP = 1, when one node is chosen by all other nodes (i.e. star).\n";

    outText << "(Wasserman & Faust, p. 203)\n";

    if (considerWeights) {
        outText << "\nNOTE: Because the network is weighted, we normalize "
                   "Group Prestige multiplying by (N-1)/maxDP, where N is the "
                   "total vertices, and subtracting the percentage of isolated vertices\n";
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
void Graph::prestigeProximity(){
    qDebug()<< "Graph::prestigeProximity()";
    if (!graphModified && calculatedPP ) {
        qDebug() << "Graph::prestigeProximity() - "
                    " graph not changed - returning";
        return;
    }
    if (!reachabilityMatrixCreated || graphModified) {
        qDebug()<< "Graph::prestigeProximity() - "
                   "call reachabilityMatrix()";
        reachabilityMatrix();
    }
    // calculate centralities
    QList<Vertex*>::iterator it;
    float PP=0;
    float Ii=0;
    int i=0;
    classesPP=0;
    discretePPs.clear();
    sumPP=0;
    maxPP=0;
    minPP=vertices()-1;
    float V=vertices();
    variancePP=0;
    meanPP=0;
    H_StrToInt::iterator it2;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        PP=0; i=0;
        // find connected nodes
        QList<int> influencerVertices = influenceDomains.values((*it)->name()-1);
        Ii=influencerVertices.size();
        qDebug()<< "Graph::prestigeProximity - vertex " <<  (*it)->name()
                   << " Ii size: " << Ii;
        for ( i = 0; i < Ii; i++) {

            qDebug() << "Graph::prestigeProximity - vertex " <<  (*it)->name()
                     << " is inbound connected from  = " << influencerVertices.at(i) + 1
                     << " at distance " << DM.item (  influencerVertices.at(i), (*it)->name()-1);
            PP += DM.item (  influencerVertices.at(i), (*it)->name()-1);
        }
        qDebug()<< "Graph::prestigeProximity - "
                   "size of influenceDomain Ii = " << Ii
                << " PP=" << PP << " divided by Ii=" << Ii
                << " yields graph-dependant PP index =" << PP / Ii;

        qDebug() << "Graph::prestigeProximity - vertex " <<  (*it)->name()
                 << " has PP = "
                 << Ii / (V-1) << " / " << PP / Ii << " = " << ( Ii / (V-1) ) / (PP/Ii);

        // sanity check for PP=0 (=> node is disconnected)
        if (PP != 0)  {
            PP /= Ii;
            PP =  ( Ii / (V-1) ) / PP;
        }
        sumPP += PP;
        (*it) -> setPP ( PP ) ;

        it2 = discretePPs.find(QString::number(PP));
        if (it2 == discretePPs.end() )	{
            classesPP++;
            qDebug() << "PP = " << (*it) -> PP() <<  " - this is a new PP class" ;
            discretePPs.insert ( QString::number(PP), classesPP );
        }
        qDebug("PP classes = %i ", classesPP);
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
    qDebug("Graph::prestigeProximity - sumPP = %f, meanPP = %f",
           sumPP, meanPP);

    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        PP= (*it) -> PP();
        variancePP += (PP-meanPP) * (PP-meanPP) ;
        (*it) -> setSPP ( PP / sumPP) ;
        qDebug() << "Graph::prestigeProximity - vertex " <<  (*it)->name()
                 << " has std PP = "
                 << PP << " / " << sumPP << " = " << (*it)->SPP();

    }

    variancePP=variancePP/(float) V;
    calculatedPP=true;

}



//Writes the proximity prestige indeces to a file
void Graph::writePrestigeProximity(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating prestige proximity indices")) );
    prestigeProximity();
    emit statusMessage ( QString(tr("Writing proximity prestige indices to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("PROXIMITY PRESTIGE (PP)\n"
                  "The PP index of a node u is the ratio of the proportion of "
                  "nodes who can reach u to the average distance these nodes are "
                  "from u.\n"
                  "This index measures how proximate a node v is to the nodes "
                  "in its influence domain I (the influence domain I of a node "
                  "is the number of other nodes that can reach it).\n "
                  "The algorithm takes the average distance to node u of all "
                  "nodes in its influence domain, standardizes it by "
                  "multiplying with (N-1)/I and takes its reciprocal. "
                  );

    outText <<"(Wasserman & Faust, formula 5.25, p. 204)\n\n";

    outText << tr("PP range:  0 < PP < 1 "
            " (PP is a ratio)")<<"\n";
    outText << tr("PP' is the standardized PP (divided by sumPP). \n\n");
    outText << "Node"<<"\tPP\t\tPP'\t\t%PP'\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"
                <<(*it)->PP() << "\t\t"
               << (*it)->SPP() << "\t\t"
               <<  (100* (*it)->SPP()) <<endl;
    }
    qDebug ("min %f, max %f", minPP, maxPP);
    if ( minPP == maxPP )
        outText << tr("\nAll nodes have the same PP value.\n");
    else  {
        outText << "\n";
        outText << tr("Max PP' = ") << maxPP <<" (node "<< maxNodePP  <<  ")  \n";
        outText << tr("Min PP' = ") << minPP <<" (node "<< minNodePP <<  ")  \n";
        outText << tr("PP classes = ") << classesPP<<" \n";
    }

    outText << tr("PP Mean = ") << meanPP<<"\n";
    outText << tr("PP Sum= ") << sumPP<<"\n";
    outText << tr("PP Variance = ") << variancePP<<"\n\n";



    outText << "\n\n";
    outText << tr("Proximity Prestige report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime()
               .toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




//Calculates the PageRank Prestige of each vertex
int Graph::prestigePageRank(){
    qDebug()<< "Graph:: prestigePageRank()";
    discretePRCs.clear();
    sumPRC=0;
    maxPRC=0;
    minPRC=RAND_MAX;
    classesPRC=0;
    groupPRC=0;
    isolatedVertices = 0 ;
    dampingFactor = 0.85; // The parameter d is a damping factor which can be set between 0 and 1. Google creators set d to 0.85.

    float PRC=0, oldPRC = 0;
    float SPRC=0;
    int i = 1; // a counter
    int referrer;

    float delta = 0.01; // The delta where we will stop the iterative calculation
    float maxDelta = RAND_MAX;
    float sumPageRanksOfLinkedNodes = 0;  // temporary variable to calculate PR
    float outDegree = 0;
    bool allNodesAreIsolated = true;
    QList<Vertex*>::iterator it;
    int relation=0;
    bool edgeStatus=false;
    H_edges::const_iterator jt;

    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta) {
        for (it=m_graph.begin(); it!=m_graph.end(); it++){
            qDebug() << "Graph:: prestigePageRank() - calculating PR for node: "
                     << (*it)->name() ;
            // In the first iteration, we have no PageRanks
            // So we set them to (1-d)
            if ( i == 1 ) {
                (*it)->setPRC( 1 - dampingFactor );
                qDebug() << "Graph:: prestigePageRank() - 1st iteration - node: "
                         << (*it)->name() << " PR = " << (*it)->PRC() ;
                if ( (*it)->isIsolated() ) {
                    isolatedVertices++;
                    qDebug()<< "Graph:: prestigePageRank() vertex: "
                            << (*it)->name()
                            << " is isolated. PR will be just 1-d. Continue... ";
                }
                else
                    allNodesAreIsolated = false;
            }
            // In every other iteration we calculate PageRanks.
            else {
                sumPageRanksOfLinkedNodes = 0;
                maxDelta = 0;
                oldPRC = (*it)->PRC();
                // take every other node which links to the current node.
                jt=(*it)->m_inLinks.cbegin();
                while ( jt != (*it) -> m_inLinks.cend() ){
                    qDebug() << "Graph::numberOfCliques() "
                             << " iterate over all inLinks ";
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
                    qDebug() << "Graph:: prestigePageRank " << (*it)->name()
                             << " is inLinked from " << referrer  ;

                    if ( this->hasEdge( referrer , (*it)->name() ) ) {
                        outDegree = m_graph[ index[referrer] ] ->outDegree();
                        PRC =  m_graph[ index[referrer] ]->PRC();
                        qDebug()<< "Graph:: prestigePageRank() " <<  referrer
                                << " has PRC = " << PRC
                                << " and outDegree = " << outDegree
                                << " PRC / outDegree = " << PRC / outDegree ;
                        sumPageRanksOfLinkedNodes += PRC / outDegree;
                    }
                    ++jt;
                }
                // OK. Now calculate PageRank of current node
                PRC = (1-dampingFactor) + dampingFactor * sumPageRanksOfLinkedNodes;
                // store new PageRank
                (*it) -> setPRC ( PRC );
                // calculate diff from last PageRank value for this vertex
                // and set it to minDelta if the latter is bigger.
                qDebug()<< "Graph:: prestigePageRank() vertex: " <<  (*it)->name()
                        << " new PageRank = " << PRC
                        << " old PR was = " << oldPRC
                        << " diff = " << fabs(PRC - oldPRC);
                if ( maxDelta < fabs(PRC - oldPRC) ) {
                    maxDelta = fabs(PRC - oldPRC);
                    qDebug()<< "Graph:: prestigePageRank() setting new maxDelta = "
                            <<  maxDelta;
                }

            }
        }
        if (allNodesAreIsolated) {
            qDebug()<< "Graph:: prestigePageRank() all vertices are isolated. Break...";
            qDebug() << "isolatedVertices: " << isolatedVertices
                     << " total vertices " << this->vertices();

            break;
        }
        i++;
    }
    // calculate sumPRC
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        sumPRC +=  (*it)->PRC();
    }
    // calculate std and min/max PRCs
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        PRC = (*it)->PRC();
        resolveClasses(PRC,discretePRCs,classesPRC);
        if ( PRC > maxPRC ) {
            maxPRC = PRC;
            maxNodePRC=(*it)->name();
        }
        if ( PRC < minPRC ) {
            minPRC = PRC;
            minNodePRC=(*it)->name();
        }

        SPRC = PRC / sumPRC ;
        (*it)->setSPRC( SPRC );
        qDebug()<< "Graph:: prestigePageRank() vertex: " <<  (*it)->name()
                << " PageRank = " << PRC << " standard PR = " << SPRC;
    }
    if (allNodesAreIsolated) {
        qDebug()<< "Graph:: prestigePageRank() all vertices are isolated. Equal PageRank for all....";
        return 1;
    }
    qDebug()<< "Graph:: prestigePageRank() vertex: " <<  maxNodePRC
            << " has max PageRank = " << maxPRC;
    return 0;

}


//Writes the PageRank indices to a file
void Graph::writePrestigePageRank(const QString fileName){
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating PageRank indices. Please wait...")) );
    this->prestigePageRank();

    emit statusMessage ( QString(tr("Writing PageRank indices to file: %1"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("PAGERANK PRESTIGE (PR)")<<"\n";
    outText << tr("")<<"\n";
    outText << tr("PR  range:  1-d < C  where d=") << dampingFactor   << "\n";
    outText << tr("PR' is the standardized PR")<<"\n";
    outText << tr("PR' range:  ") << dampingFactor / sumPRC  << " < C'< 1" <<"\n\n";
    outText << "Node"<<"\tPRC\t\tPRC'\t\t%PRC\n";
    QList<Vertex*>::iterator it;
    float PRC=0, SPRC=0, sumSPRC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        PRC = (*it)->PRC();
        SPRC = (*it)->SPRC();
        sumSPRC +=  SPRC;
        outText << (*it)->name()<<"\t"<< PRC << "\t\t"<< SPRC  << "\t\t" <<  ( 100* SPRC )<<endl;
        qDebug()<< "Graph::writeprestigePageRank() vertex: " <<  (*it)->name() << " SPRC  " << SPRC;
    }
    qDebug ("min %f, max %f", minPRC, maxPRC);
    if ( minPRC == maxPRC )
        outText << tr("\nAll nodes have the same PRC value.\n");
    else  {
        outText << "\n";
        outText << tr("Max PRC = ") << maxPRC <<" (node "<< maxNodePRC  <<  ")  \n";
        outText << tr("Min PRC = ") << minPRC <<" (node "<< minNodePRC <<  ")  \n";
        outText << tr("PRC classes = ") << classesPRC<<" \n";
    }
    outText << "\n";

    float x=0;
    float n = ( this->vertices() - isolatedVertices );
    if (n != 0 )
        averagePRC = sumSPRC / n ;
    else
        averagePRC = SPRC;

    qDebug() << "sumPRC = " << sumSPRC << "  n = " << n << "  averagePRC = " << averagePRC;
    groupPRC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        x = ( 100 * (*it)->SPRC()  - 100 * averagePRC  ) ;
        x *=x;
        qDebug() << "SPRC " <<  (*it)->SPRC() << "  x " <<   (*it)->SPRC() - averagePRC  << " x*x" << x ;
        groupPRC  += x;
    }
    qDebug() << "groupPRC   " << groupPRC   << " n " << n ;
    groupPRC  = groupPRC  /  (n-1);
    qDebug() << "groupPRC   " << groupPRC   ;
    outText << tr("\nGROUP PAGERANK PRESTIGE (GPRP)\n\n");
    outText << tr("GPRP = ") << groupPRC<<"\n\n";
    outText << tr("GPRP range: 0 < GPRP < inf \n");
    outText << tr("GPRP is computed using a simple variance formula. \n");



    outText << tr("PageRank Prestige report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




/**
*	Writes the number of cliques (triangles) of each vertex into a given file.
*/
void Graph::writeNumberOfCliques(
        const QString fileName, const bool considerWeights)
{
    Q_UNUSED(considerWeights);
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    long int cliques=0, cliques_sum=0, N = vertices();

    QTextStream outText ( &file );

    emit statusMessage ( QString(tr("Writing number of triangles to file:")).arg(fileName) );

    outText << tr("NUMBER OF CLIQUES (CLQs)") << "\n";
    outText << tr("CLQs range: 0 < CLQs < ") <<"\n\n";
    outText << "Node"<<"\tCLQs\n";

    QList<Vertex*>::iterator it;

    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        cliques=this->numberOfCliques((*it)->name());
        outText << (*it)->name()<<"\t"<< cliques <<endl;
        cliques_sum += cliques;
    }

    outText << endl << tr("NUMBER OF CLIQUES (CLQSUM) OF GRAPH")<< endl;
    outText << "CLQSUM = " <<  cliques_sum / 3.0 <<"\n\n";
    if ( N > 3)
        outText << tr("CLQSUM Range: 0 < CLQSUM < ") << N * (N-1) * (N-2)/ 3 << endl;

    outText <<"\n\n" ;
    outText << tr("Number of Cliques Report,\n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

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
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating shortest paths")) );
    float clucof= clusteringCoefficient();
    Q_UNUSED(clucof);
    emit statusMessage ( QString(tr("Writing clustering coefficients to file:"))
                         .arg(fileName) );
    outText.setRealNumberPrecision(m_precision);
    outText << tr("CLUSTERING COEFFICIENT (CLC)\n");
    outText << tr("CLC  range: 0 < C < 1") <<"\n";
    outText << "Node"<<"\tCLC\n";


    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->CLC() <<endl;
    }
    if ( isSymmetric()) {
        outText << "\nAverage Clustering Coefficient = "<<  averageCLC<<"\n" ;
        //	outText << "DC Variance = "<<  varianceDegree<<"\n\n";
    }
    else{
        outText << "\nAverage Clustering Coefficient= "<<  averageCLC<<"\n" ;
        //		outText << "DC Variance = "<<  varianceDegree<<"\n\n";
    }
    if (  minCLC ==  maxCLC )
        outText << "\nAll nodes have the same clustering coefficient value.\n";
    else  {
        outText << "\nNode "<<  maxNodeCLC
                << " has the maximum Clustering Coefficient: " <<  maxCLC <<"\n";
        outText << "\nNode "<<  minNodeCLC
                << " has the minimum Clustering Coefficient: " <<  minCLC <<"\n";
    }

    outText << "\nGRAPH CLUSTERING COEFFICIENT (GCLC)\n\n";
    outText << "GCLC = " <<  averageCLC<<"\n\n";
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

    QTextStream outText ( &file );

    emit statusMessage ( (tr("Conducting triad census. Please wait....")) );

    if (!triadCensus()){
        qDebug() << "Error in triadCensus(). Exiting...";
        file.close();
        return;
    }


    emit statusMessage ( QString(tr("Writing clustering coefficients to file:"))
                         .arg(fileName) );

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
void Graph::layoutCircularByProminenceIndex(double x0, double y0, double maxRadius,
                                   int prominenceIndex){
    qDebug() << "Graph::layoutCircularByProminenceIndex - "
                << "prominenceIndex index = " << prominenceIndex;
    //first calculate centrality indices if needed

    if ( prominenceIndex == 1)
        this->centralityDegree(true);
    else if ( prominenceIndex == 3 )
        this->centralityClosenessInfluenceRange();
    else if ( prominenceIndex == 8 )
        this->centralityInformation();
    else if ( prominenceIndex == 9)
        this->prestigeDegree(true);
    else if ( prominenceIndex == 10 )
        this->prestigePageRank();
    else if ( prominenceIndex == 11 )
        this->prestigeProximity();
    else
        this->createDistanceMatrix(true);

    double rad=0;
    double i=0, std=0;
    //offset controls how far from the centre the central nodes be positioned
    float C=0, maxC=0, offset=0.06;
    double new_radius=0, new_x=0, new_y=0;
    double Pi = 3.14159265;
    int vert=vertices();

    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
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
                C=(*it)->PRC();
                std= (*it)->SPRC();
                maxC=maxPRC;
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
                  << ", *maxRadius " << (C/maxC - 0.06)*maxRadius;
        switch (static_cast<int> (ceil(maxC)) ){
        case 0: {
            qDebug("maxC=0.   Using maxHeight");
            new_radius=maxRadius;
            break;
        }
        default: {
            new_radius=(maxRadius- (C/maxC - offset)*maxRadius);
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
}


/** 
*	 Repositions all nodes on different top-down levels according to their centrality
* 	Emits moveNode(i, x,y) to tell GW that the node item should be moved. 
*/
void Graph::layoutLevelByProminenceIndex(double maxWidth, double maxHeight,
                                         int prominenceIndex){
    qDebug("Graph: layoutLevelCentrality...");

    if (prominenceIndex == 1)
        this->centralityDegree(true);
    else if ( prominenceIndex == 3 )
        this->centralityClosenessInfluenceRange();
    else if ( prominenceIndex == 8 )
        this->centralityInformation();
    else if ( prominenceIndex == 9)
        this->prestigeDegree(true);
    else if ( prominenceIndex == 10 )
        this->prestigePageRank();
    else if ( prominenceIndex == 11 )
        this->prestigeProximity();
    else
        this->createDistanceMatrix(true);

    double i=0, std=0;
    //offset controls how far from the top the central nodes will be
    float C=0, maxC=0, offset=50;
    double new_x=0, new_y=0;
    //	int vert=vertices();
    maxHeight-=offset;
    maxWidth-=offset;
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
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
                C=(*it)->PRC();
                std= (*it)->SPRC();
                maxC=maxPRC;
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
        qDebug ("C/maxC %f, *maxHeight %f, +maxHeight %f "
                , C/maxC, (C/maxC)*maxHeight, maxHeight-(C/maxC)*maxHeight );
        switch ( static_cast<int> (ceil(maxC)) ){
        case 0: {
            qDebug("maxC=0.   Using maxHeight");
            new_y=maxHeight;
            break;
        }
        default: {
            new_y=offset/2.0+maxHeight-(C/maxC)*maxHeight;
            break;
        }
        };
        new_x=offset/2.0 + rand() % ( static_cast<int> (maxWidth) );
        qDebug ("new_x %f, new_y %f", new_x, new_y);
        (*it)->setX( new_x );
        (*it)->setY( new_y );
        qDebug("Finished Calculation. Vertice will move to x=%f and y=%f ",new_x, new_y);
        //Move node to new position
        emit moveNode((*it)->name(),  new_x,  new_y);
        i++;
        emit addGuideHLine(static_cast<int> ( new_y ) );
    }
    graphModified=true;
}



/** layman's attempt to create a random network
*/
void Graph::createRandomNetErdos(int vert, double probability){
    qDebug("Graph: createRandomNetErdos");

    index.reserve(vert);

    int progressCounter=0;

    for (register int i=0; i< vert ; i++) {
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
    for (register int i=0;i<vert; i++) {
        for (register int j=0; j<vert; j++) {
            qDebug("Random Experiment for link creation between %i and %i:", i+1, j+1);
            if (rand() % 100 < probability)    {
                qDebug("Creating link!");
                createEdge(i+1, j+1, 1, "black", true, true, false);
            }
            else
                qDebug("Will not create link!");
        }
        progressCounter++;
        emit updateProgressDialog(progressCounter );
        qDebug("Emitting UPDATE PROGRESS %i", progressCounter);
    }
    addRelationFromGraph(tr("random")); //FIXME
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

    double Pi = 3.14159265;
    double rad= (2.0* Pi/ vert );

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
    addRelationFromGraph(tr("random"));
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
            if ( this-> hasEdge(i, j) ) {
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
                        if (! this->hasEdge(i, candidate) )
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
}





/** layman's attempt to create a random network where nodes have the same degree.
*/

void Graph::createSameDegreeRandomNetwork(int vert, int degree){
    qDebug("Graph: createSameDegreeRandomNetwork");

    int progressCounter=0;

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
    bool omitWeights=false;
    createAdjacencyMatrix(dropIsolates, omitWeights);

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

    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    out << "Network name "<< netName<<": \n";
    out << "Total number of walks of any length less than or equal to "<< length
        <<" between each pair of nodes \n\n";
    out << "Warning: Walk counts consider unordered pairs of nodes\n\n";

    createNumberOfWalksMatrix(length);

    out << XSM ;

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

    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    out << "Network name "<< netName<<": \n";
    out << "Number of walks of length "<< length <<" between each pair of nodes \n\n";

    createNumberOfWalksMatrix(length);

    out << XM ;

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
void Graph::reachabilityMatrix() {
    qDebug()<< "Graph::reachabilityMatrix()";

    if (reachabilityMatrixCreated && !graphModified) {
        qDebug()<< "Graph::reachabilityMatrix() - "
                   "XRM calculated and graph unmodified. Returning..." ;
        return;
    }
    else {
        int size = vertices(), i=0, j=0;
        createDistanceMatrix(false);
        XRM.zeroMatrix(size);
        qDebug()<< "Graph::reachabilityMatrix() - calculating XRM..." ;
        influenceRanges.clear();
        influenceDomains.clear();
        notStronglyConnectedVertices.clear();
        for (i=0; i < size ; i++) {
            for (j=i; j < size ; j++) {
                qDebug()<< "Graph::reachabilityMatrix()  total shortest paths between ("
                        << i+1 <<"," << j+1<< ")=" << TM.item(i,j) <<  " ";
                if ( DM.item(i,j) > 0 ) {
                    qDebug()<< "Graph::reachabilityMatrix()  - d("<<i+1<<","
                            <<j+1<<")=" << DM.item(i,j)
                            << " - inserting " << j+1
                            << " to inflRange J of " << i+1
                            << " - and " << i+1
                            << " to inflDomain I of "<< j+1 ;
                    XRM.setItem(i,j,1);
                    influenceRanges.insertMulti(i,j);
                    influenceDomains.insertMulti(j,i);
                }
                else if (i==j) {
                       XRM.setItem(i,j,1);
                }
                else {
                       XRM.setItem(i,j,0);
                       notStronglyConnectedVertices.insertMulti(i,j);
                }
                if ( DM.item(j,i) > 0 ) {
                    qDebug()<< "Graph::reachabilityMatrix()  - inverse path d("
                            <<j+1<<","<<i+1<<")="
                            << DM.item(j,i)
                            << " - inserting " << j+1 << " to influenceDomain I of " << i+1
                            << " - and " << i+1 << " to influenceRange J of " << j+1 ;
                    XRM.setItem(j,i,1);
                    influenceDomains.insertMulti(i,j);
                    influenceRanges.insertMulti(j,i);
                }
                else if (i==j) {
                       XRM.setItem(i,j,1);
                }
                else {
                       XRM.setItem(j,i,0);
                       notStronglyConnectedVertices.insertMulti(j,i);
                }
            }
            qDebug()<< endl;
        }
        reachabilityMatrixCreated=true;
    }

}


/**
    Writes the reachability matrix X^R of the graph to a file
*/
void Graph::writeReachabilityMatrix(QString fn, QString netName) {
    qDebug("Graph::writeReachabilityMatrix() ");

    QFile file (fn);

    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fn) );
        return;
    }

    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    out << "Network name: "<< netName<<" \n";
    out << "Reachability Matrix (XR) \n";
    out << "Two nodes are reachable if there is a walk between them (their geodesic distance is non-zero). \n";
    out << "If nodes i and j are reachable then XR(i,j)=1 otherwise XR(i,j)=0.\n\n";

    if (!reachabilityMatrixCreated || graphModified) {
        reachabilityMatrix();
    }

    out << XRM ;

    file.close();
}



/**
    Calculates and returns the number of cliques which include vertex v1
    A clique (or triangle) is a complete subgraph of three nodes.
*/	
float Graph:: numberOfCliques(int v1){
    qDebug("*** Graph::numberOfCliques(%i) ", v1);
    float cliques=0;
    int  connectedVertex1=0, connectedVertex2=0;
    int relation=0;
//    int weight=0;
    bool edgeStatus=false;
    bool symmetric=false;
    H_edges::const_iterator it1, it2;

    qDebug() << "Graph::numberOfCliques() Source vertex " << v1
             << "[" << index[v1] << "] has inDegree " << inboundEdges(v1)
             << " and outDegree "<< outboundEdges(v1);

    if ( ! (symmetric = isSymmetric()) ) {
        qDebug () << "Graph::numberOfCliques() - graph is not symmetric"
                  << " checking inLinks to " << v1;
        it1=m_graph [ index[v1] ] ->m_inLinks.cbegin();
        while ( it1!=m_graph [ index[v1] ] -> m_inLinks.cend() ){
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
            connectedVertex1 = it1.key();
//            weight = it1.value().second.first;
            qDebug() << "Graph::numberOfCliques() "
                        << " inLink from 1st neighbor " << connectedVertex1
                     << "[" << index[connectedVertex1] << "] "
                        << "...Cross-checking with it inLinks from other neighbors";
            it2=m_graph [ index[v1] ] ->m_inLinks.cbegin();
            while ( it2!=m_graph [ index[v1] ] -> m_inLinks.cend() ){
                qDebug() << "Graph::numberOfCliques() "
                         << " iterate over all inLinks ";
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
                connectedVertex2 = it2.key();
                qDebug() << "Graph::numberOfCliques() "
                         << " possible other neighbor" << connectedVertex2;
                if (connectedVertex1 == connectedVertex2) {
                    qDebug() << "Graph::numberOfCliques() "
                             << " it is the same 1st neighbor - CONTINUE";
                    ++it2;
                    continue;
                }
                else {
                    qDebug() << "Graph::numberOfCliques() "
                             << " inLink from other neighbor "
                             << connectedVertex2
                             << "[" << index[connectedVertex2] << "]";
                    if ( this->hasEdge( connectedVertex1, connectedVertex2 ) ) {
                        qDebug() << "Graph::numberOfCliques() "
                                 << " 1st neighbor " << connectedVertex1
                                 << " has OutLink to other neighbor "
                                  << connectedVertex2
                                  << " Therefore we found a clique!";
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
                }
                ++it2;
            }
            qDebug()<< "Graph::numberOfCliques()  .....Checking outLinks.... ";
            it2=m_graph [ index[v1] ] ->m_outLinks.cbegin();
            while ( it2!=m_graph [ index[v1]  ] -> m_outLinks.cend() ){
                relation = it2.value().first;
                if ( relation != currentRelation() ) {
                    ++it2;
                    continue;
                }
                edgeStatus=it2.value().second.second;
                if ( edgeStatus != true) {
                    ++it2;
                    continue;
                }
                connectedVertex2=it2.key();
                if (connectedVertex1 == connectedVertex2) {
                    ++it2;
                    continue;
                }
                else {
                    qDebug() << "Graph::numberOfCliques() "
                             << " outLink to other neighbor "
                             << connectedVertex2
                             << "["  << index[connectedVertex2] << "]";
                    if ( this->hasEdge( connectedVertex1, connectedVertex2 )
                         || this-> hasEdge( connectedVertex2, connectedVertex1 ) ) {
                        qDebug() << "Graph::numberOfCliques() "
                               << " other neighbor " << connectedVertex2
                                  << " is connected to neighbor "
                                  << connectedVertex1
                                  <<  "Therefore we found a clique!";
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
                }
                ++it2;
            } // end 2nd while loop
            ++it1;
        } // end 1st while

    } // end if not symmetric


    it1=m_graph [ index[v1] ] ->m_outLinks.cbegin();
    while ( it1!=m_graph [ index[v1]  ] -> m_outLinks.cend() ){
        relation = it1.value().first;
        if ( relation != currentRelation() ){
            ++it1;
            continue;
        }
        edgeStatus=it1.value().second.second;
        if ( edgeStatus != true) {
            ++it1;
            continue;
        }
        connectedVertex1=it1.key();
        qDebug() << "Graph::numberOfCliques() "
                    << " outLink to 1st neighbor " << connectedVertex1
                 << "[" << index[connectedVertex1] << "] "
                    << "...Cross-checking with it outLinks to other neighbors";

        it2=m_graph [ index[v1] ] ->m_outLinks.cbegin();
        while ( it2!=m_graph [ index[v1]  ] -> m_outLinks.cend() ){
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
            connectedVertex2=it2.key();
            if (connectedVertex1 == connectedVertex2){
                ++it2;
                continue;
            }
            else if ( (connectedVertex1 >= connectedVertex2) && symmetric){
                ++it2;
                continue;
            }
            else {
                qDebug() << "Graph::numberOfCliques() "
                         << " outLink to other neighbor "
                         << connectedVertex2
                         << "["  << index[connectedVertex2] << "]";
                if ( this->hasEdge( connectedVertex1, connectedVertex2 ) ) {
                    qDebug() << "Graph::numberOfCliques() "
                           << " 1st neighbor " << connectedVertex1
                              << " is connected to other neighbor "
                              << connectedVertex2
                              <<  "Therefore we found a clique!";
                    cliques++;
                    qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                }
                if (!symmetric)
                    if ( this->hasEdge( connectedVertex2, connectedVertex1 ) ) {
                        qDebug() << "Graph::numberOfCliques() "
                               << " other neighbor " << connectedVertex2
                                  << " has also inLink connected to 1st neighbor "
                                  << connectedVertex1
                                  <<  "Therefore we found a clique!";
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
            }
            ++it2;
        } // end 2nd while
        ++it1;
    } // end 1st while
    return cliques;
}


/**
    Calculates and returns the total number of cliques in the graph.
    Calls numberOfCliques(v1) to calculate the number of cliques of each vertex v1,
    sums the total number, then divides it by 3 because each vertex has been counted three times.
*/	
float Graph::numberOfCliques(){
    qDebug("Graph::	numberOfCliques()");
    float cliques=0;
    foreach (Vertex *v1, m_graph)  {
        cliques += numberOfCliques(v1->name());
        qDebug("Graph::	numberOfCliques now %f", cliques );
    }
    cliques = cliques / 3.0;
    qDebug("Graph::	numberOfCliques Dividing by three we get %f", cliques );
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
    Returns the clustering coefficient (CLUCOF) of a vertex v1
    CLUCOF in a graph quantifies how close the vertex and its neighbors are to being a clique
    This is used to determine whether a graph is a small-world network.
*/
float Graph:: clusteringCoefficient(int v1){
    float clucof=0;
    if ( !graphModified && (m_graph[ index [v1] ] -> hasCLC() ) )  {
        float clucof=m_graph[ index [v1] ] ->CLC();
        qDebug("Graph: clusteringCoefficient(%i) not modified. Returning previous clucof = %f", v1, clucof);
        return clucof;
    }

    qDebug("Graph::	clusteringCoefficient(v1) - Graph changed or clucof not calculated. Calling numberOfCliques() for vertex %i", v1);
    float totalCliques=numberOfCliques(v1);
    qDebug("Graph::	Number of Cliques for %i is %f.", v1, totalCliques);

    if (totalCliques==0) return 0;	//stop if we're at a leaf.

    float denom=0, totalDegree=0;

    if (isSymmetric()){
        totalCliques = totalCliques / 2.0;
        qDebug(" Graph::Calculating number of triples");
        totalDegree=outboundEdges(v1);
        denom =	totalDegree * (totalDegree -1.0) / 2.0;
        qDebug("Graph:: Symmetric. Number of triples is %f.  Dividing number of cliques with it", denom);

    }
    else {
        qDebug(" Graph::Calculating number of triples");
        totalDegree=outboundEdges(v1) + inboundEdges(v1);  //FIXME
        denom = totalDegree * (totalDegree -1.0);
        qDebug("Graph:: Symmetric. Number of triples is %f.  Dividing number of cliques with it", denom);
    }

    clucof = totalCliques / denom;
    qDebug() << "=== Graph::clusteringCoefficient() - vertex " <<  v1 << " ["<< index[v1] << "]" << " has CLUCOF = "<< clucof;
    m_graph[ index [v1] ] ->setCLC(clucof);
    return clucof;
}


/**
    Calculates and returns the Clustering Coefficient for the whole graph
*/
float Graph::clusteringCoefficient (){
    qDebug("=== Graph::graphClusteringCoefficient()  ");
    averageCLC=0;
    maxCLC=0; minCLC=1;
    float temp=0;
    foreach (Vertex *v1, m_graph)  {
        temp = clusteringCoefficient(v1->name());
        if (temp > maxCLC)  {
            maxCLC = temp;
            maxNodeCLC = v1->name();
        }
        if ( temp < minCLC ) {
            minNodeCLC = v1->name();
            minCLC= temp;
        }
        averageCLC += temp;
    }

    averageCLC = averageCLC / vertices();
    qDebug("=== Graph::graphClusteringCoefficient()  is equal to %f", averageCLC);

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
    QString last_char;

    int progressCounter = 0;
    /*
     * QList::triadTypeFreqs stores triad type frequencies with the following order:
     * 0	1	2	3		4	5	6	7	8		9	10	11	12		13	14	15
     * 003 012 102	021D 021U 021C 111D	111U 030T 030C 201 	120D 120U 120C 210 300
    */

    for (int i = 0; i < 15; ++i) {
        triadTypeFreqs.append(0);
    }
    QList<Vertex*>::iterator v1;
    QList<Vertex*>::iterator v2;
    QList<Vertex*>::iterator v3;

    for (v1=m_graph.begin(); v1!=m_graph.end(); v1++) {

        for (v2=(v1+1); v2!=m_graph.end(); v2++) {

            ver1=(*v1)->name();
            ver2=(*v2)->name();

            temp_mut=0, temp_asy=0, temp_nul =0;

            if ( (*v1)->isLinkedTo( ver2 ) ) {
                if ( (*v2)->isLinkedTo( ver1 ) )
                    temp_mut++;
                else
                    temp_asy++;
            }
            else if ( (*v2)->isLinkedTo( ver1 )  )
                temp_asy++;
            else
                temp_nul++;

            for (v3=(v2+1); v3!=m_graph.end(); v3++){

                mut = temp_mut ;
                asy = temp_asy ;
                nul = temp_nul ;

                ver3=(*v3)->name();

                if ( (*v1)->isLinkedTo( ver3 ) ) {
                    if ( (*v3)->isLinkedTo( ver1 ) )
                        mut++;
                    else
                        asy++;
                }
                else if ( (*v3)->isLinkedTo( ver1 )  )
                    asy++;
                else
                    nul++;

                if ( (*v2)->isLinkedTo( ver3 ) ) {
                    if ( (*v3)->isLinkedTo( ver2 ) )
                        mut++;
                    else
                        asy++;
                }
                else if ( (*v3)->isLinkedTo( ver2 )  )
                    asy++;
                else
                    nul++;

                //qDebug()<< "triad of ("<< ver1 << ","<< ver2 << ","<< ver3 << ") = ("	<<mut<<","<< asy<<","<<nul<<")";
                examine_MAN_label(mut, asy, nul, (*v1), (*v2),  (*v3) ) ;
                progressCounter++ ;
                emit updateProgressDialog( progressCounter );
                if ( mut==3 && asy==0 && nul==0 ){
                    counter_021++;
                }
            } // end 3rd foreach
        }// end 2rd foreach
    }// end 1rd foreach
    qDebug() << " ****** 003 COUNTER: "<< counter_021;
    return true;
}




/** 
    Examines the triad type (in Mutual-Asymmetric-Null label format)
    and increases by one the proper frequency element
    inside QList::triadTypeFreqs
*/
void Graph:: examine_MAN_label(int mut, int asy, int nul, 
                               Vertex* vert1,
                               Vertex* vert2,
                               Vertex* vert3
                               ) 	{
    QString last_char;
    QList<Vertex*> m_triad;
    bool isDown=false, isUp=false, isCycle=false, isTrans=false;
    bool isOutLinked=false, isInLinked=false;
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

                    if ( source->isLinkedTo(target->name()) ){
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
                    else if( target->isLinkedTo(source->name()) ){
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

                    if ( source->isLinkedTo(target->name()) ){

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

                    if ( target->isLinkedTo(source->name()) ){

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
            qDebug() << "triad vertices: ( "<< vert1->name() << ", "<< vert2->name()<< ", "<< vert3->name()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";

            foreach (Vertex *source, m_triad)  {
                //qDebug() << "  Vertex " << source->name() ;
                isOutLinked=false; isInLinked=false;

                foreach (Vertex *target, m_triad)  	{
                    if ( source->name() == target->name() )
                        continue;

                    if ( source->isLinkedTo(target->name()) ){
                        if (target->isLinkedTo(source->name() ) ){
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
                    else if( target->isLinkedTo(source->name()) ){
                        //	qDebug() << "    Vertex " << source->name()  << " is IN linked from " <<target->name();
                        if (source->isLinkedTo(target->name())){
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
bool Graph::loadGraph (	QString fileName,  bool iSL,
                        int maxWidth, int maxHeight,
                        int fileFormat, int two_sm_mode){
    initShowLabels = iSL;
    bool loadGraphStatus = parser.load(
                fileName,
                initVertexSize, initVertexColor,
                initVertexShape,
                initVertexNumberColor, initVertexNumberSize,
                initVertexLabelColor, initVertexLabelSize,
                initEdgeColor,
                maxWidth, maxHeight,
                fileFormat,
                two_sm_mode
                );
    return loadGraphStatus;
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
        return saveGraphToAdjacencyFormat(fileName, maxWidth,maxHeight);
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
    t<<"*Network "<<networkName<<"\n";

    t<<"*Vertices "<< vertices() <<"\n";
    QList<Vertex*>::iterator it;
    QList<Vertex*>::iterator jt;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        qDebug()<<" Name x "<<  (*it)->name()  ;
        t<<(*it)->name()  <<" "<<"\""<<(*it)->label()<<"\"" ;
        t << " ic ";
        t<<  (*it)->color() ;
        qDebug()<<" Coordinates x " << (*it)->x()<< " "<<maxWidth<<" y " << (*it)->y()<< " "<<maxHeight;
        t << "\t\t" <<(*it)->x()/(maxWidth)<<" \t"<<(*it)->y()/(maxHeight);
        t << "\t"<<(*it)->shape();
        t<<"\n";
    }

    t<<"*Arcs \n";
    qDebug()<< "Graph::saveGraphToPajekFormat: Arcs";
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveGraphToPajekFormat:  it=" << (*it)->name() << ", jt=" << (*jt)->name() ;
            if  ( (weight=this->hasEdge( (*it)->name(), (*jt)->name())) !=0
                  &&   ( this->hasEdge((*jt)->name(), (*it)->name())) == 0
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
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++){
            qDebug() << "Graph::saveGraphToPajekFormat:  it=" <<  (*it)->name() << ", jt=" <<(*jt)->name() ;
            if  ( (weight=this->hasEdge((*it)->name(), (*jt)->name()))!=0   &&
                  (this->hasEdge((*jt)->name(), (*it)->name()))!=0
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



bool Graph::saveGraphToAdjacencyFormat (
        QString fileName, int maxWidth, int maxHeight)
{
    Q_UNUSED(maxWidth);
    Q_UNUSED(maxHeight);
    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly ) )  {
        emit statusMessage(QString(tr("Could not write to %1")).arg(fileName));
        return false;
    }
    QTextStream t( &f );
    qDebug("Graph: saveGraphToAdjacencyFormat() for %i vertices", vertices());

    writeAdjacencyMatrixTo(t);

    f.close();
    QString fileNameNoPath=fileName.split("/").last();
    emit statusMessage (QString( tr("Adjacency matrix-formatted network saved into file %1") ).arg( fileNameNoPath ));
    return true;
}


// Writes a known dataset to a file
void Graph::writeDataSetToFile (QString fileName) {
    QFile f( fileName );
    if ( !f.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText( &f );
    qDebug()<< "		... writing";
    QStringList fileNameNoPath=fileName.split ("/" );
    QString name=fileNameNoPath.last();
    QString datasetDescription=QString::null;
    if ( name == "Krackhardt_High-tech_managers_Advice_relation.sm" ) {
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
    else if (name == "Krackhardt_High-tech_managers_Friendship_relation.sm"){
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
    else if (name == "Krackhardt_High-tech_managers_ReportsTo_relation.sm"){
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
    else if (name == "Padgett_Florentine_Families_Marital_relation.net"){
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
    else if (name == "Padgett_Florentine_Families_Business_relation.paj"){
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
    else if (name == "Zachary_Karate_Club_Simple_Ties.sm"){
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
    else if (name == "Zachary_Karate_Club_Weighted_Ties.sm"){
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
    else if (name == "Galaskiewicz_CEOs_and_clubs_affiliation_network_data.2sm"){
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
    else if (name == "Bernard_Killworth_Fraternity.dl"){
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
    else if (name == "Mexican_Power_Network_1940s.lst"){
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
    else if (name == "Knocke_Bureacracies_Information_Exchange_Network.pajek"){
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
    else if (name == "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek"){
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
    f.close();
    if ( !datasetDescription.isEmpty() ) {
        emit describeDataset(datasetDescription);
    }

}




/** 
    Exports the adjacency matrix to a given textstream
*/
void Graph::writeAdjacencyMatrixTo(QTextStream& os){
    qDebug("Graph: adjacencyMatrix(), writing matrix with %i vertices", vertices());
    QList<Vertex*>::iterator it, it1;
    float weight=-1;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight = this->hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
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
QTextStream& operator <<  (QTextStream& os, Graph& m){
    QList<Vertex*>::iterator it, it1;
    float weight=-1;
    for (it=m.m_graph.begin(); it!=m.m_graph.end(); it++){
        for (it1=m.m_graph.begin(); it1!=m.m_graph.end(); it1++){
            if ( (weight = m.hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
                os << static_cast<int> (weight) << " ";
            }
            else
                os << "0 ";
        }
        os << endl;
    }
    return os;
}



/** 
    Writes the adjacency matrix of G to a specified file fn
    This is called by MainWindow::slotViewAdjacencyMatrix()
    The resulting matrix HAS NO spaces between elements.
*/
void Graph::writeAdjacencyMatrix (const QString fn, const char* netName) {
    qDebug()<<"Graph::writeAdjacencyMatrix() ";
    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream outText( &file );
    int sum=0;
    float weight=0;
    outText << "-Social Network Visualizer- \n";
    outText << "Adjacency matrix of "<< netName<<": \n\n";
    QList<Vertex*>::iterator it, it1;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight =  this->hasEdge ( (*it)->name(), (*it1)->name() )  )!=0 ) {
                sum++;
                if (weight >= 1)
                    outText << static_cast<int> (weight) << " "; // TODO make the matric look symmetrical
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
void Graph::createAdjacencyMatrix(bool dropIsolates=false, bool omitWeights=false){
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
    QList<Vertex*>::iterator it, it1;
    qDebug() << "Graph::createAdjacencyMatrix() - creating new adjacency matrix ";
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
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
            if ( (m_weight = this->hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
                if (omitWeights)
                    AM.setItem(i,j, 1 );
                else
                    AM.setItem(i,j, m_weight );
            }
            else{
                AM.setItem(i,j, 0);
            }
            qDebug()<<" AM("<< i+1 << ","<< j+1 << ") = " <<  AM.item(i,j);
            if (i != j ) {
                if ( (m_weight = this->hasEdge ( (*it1)->name(), (*it)->name() )  ) !=0 ) {
                    if (omitWeights)
                        AM.setItem(j,i, 1 );
                    else
                        AM.setItem(j,i, m_weight );
                }
                else {
                    AM.setItem(j,i, 0);
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
    bool omitWeights=true;
    createAdjacencyMatrix(dropIsolates, omitWeights);
    invAM.resize(m_totalVertices-isolatedVertices);
    qDebug()<<"Graph::invertAdjacencyMatrix() - invert the Adjacency Matrix AM and store it to invAM";
    invAM.inverseByGaussJordanElimination(AM);


}



void Graph::writeInvertAdjacencyMatrix(QString fn, const char* netName){
    qDebug("Graph::writeInvertAdjacencyMatrix() ");
    int i=0, j=0;
    QList<Vertex*>::iterator it, it1;
    QFile file( fn );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        emit statusMessage( QString(tr("Could not write to %1")).arg(fn) );
        return;
    }
    QTextStream outText( &file );

    outText << "-Social Network Visualizer- \n";
    outText << "Invert Matrix of "<< netName<<": \n\n";
    invertAdjacencyMatrix();
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() )
            continue;
        j=0;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
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
    qDebug()<< "		... writing xml version";
    outText << "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n";
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

    QList<Vertex*>::iterator it;
    QList<Vertex*>::iterator jt;

    qDebug()<< "		    writing nodes data";
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
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
    for (it=m_graph.begin(); it!=m_graph.end(); it++)
    {
        for (jt=m_graph.begin(); jt!=m_graph.end(); jt++)
        {
            source=(*it)->name();
            target=(*jt)->name();

            if  ( 	(weight= this->hasEdge( source,target ) ) !=0 )
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
    This slot is activated when the user clicks on the relevant MainWindow checkbox (SpringEmbedder, Fruchterman)
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
    int factor=100;		//factor controls speed. Decrease it to increase speed...
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

void Graph::layoutForceDirectedSpringEmbedder(bool dynamicMovement){
    qreal xvel = 0, yvel = 0, dx=0, dy=0, ulv_x=0, ulv_y=0;
    qreal c_rep=3, c_spring=3, dux=0, duy=0, natural_length=70;
    double dist = 0;
    QPointF curPos, newPos, pos ;

    if (dynamicMovement){
        qDebug () << "max dx "<< canvasWidth << "max dy "<< canvasHeight;
        foreach (Vertex *v1, m_graph)  {
            xvel=0; yvel=0;
            qDebug() << "****************  Calculate forces for vertex " << v1->name()
                     << " with index " <<  index[v1->name()] << " and pos "<< v1->x() << ", "<< v1->y();
            foreach (Vertex *v2, m_graph)  {
                qDebug () << " v2 = "<< v2->name() << " with pos (" <<  v2->x() << "," << v2->y() << ")";
                if (v2 == v1) {
                    qDebug() << " v1==v2, continuing";
                    continue;
                }
                dx = v2->x() - v1->x();
                dy = v2->y() - v1->y();
                dist = sqrt (dx * dx + dy * dy); //the euclideian distance of the two vertices
                qDebug()<< "v1= " << v1->name() <<  " v2= " <<  v2->name() << " - euclideian distance = " << dist;

                if ( this->hasEdge (v1->name(), v2->name())  ) {  //calculate spring forces (pulling) force
                    ulv_x =  + dx / dist;
                    ulv_y =  + dy / dist;
                    dux = (ulv_x * c_spring) * log ( dist / natural_length );
                    duy = (ulv_y * c_spring) * log ( dist / natural_length );
                    xvel +=  dux;
                    yvel +=  duy;
                    qDebug() << " v1= "<<v1->name() <<  " connected to and pulled by v2= "<< v2->name()
                             <<"  c_spring=" << c_spring <<"  nat_length =" << natural_length
                            <<" ulv_x="<<ulv_x 	<<" ulv_y="<<ulv_y 	<<" dist= "<<dist
                           << " dux="<< dux << " duy="<< duy;
                    qDebug() << " ========== New Total Velocity for "<<  v1->name() << "xvel, yvel  "<< xvel << ", "<< yvel;
                    continue;
                }
                else {
                    //calculate electric (repulsive) forces between non-adjacent vertices.
                    ulv_x = - dx / dist;
                    ulv_y = - dy / dist;
                    dux = (ulv_x * c_rep) / (dist * dist);
                    duy = (ulv_y * c_rep) / ( dist * dist) ;
                    qDebug() << " v1 = "<<v1->name() <<  " NOT connected to and pushed away from  v2 = "<< v2->name()
                             <<"  c_rep=" << c_rep
                            <<" ulv_x="<<ulv_x 	<<" ulv_y="<<ulv_y 	<<" dist^2="<<dist * dist
                           << " dux=" << dux 	<< " duy=" << duy;
                    xvel +=  dux ;
                    yvel +=  duy;
                    qDebug() << " ========== New Total Velocity for "<<  v1->name() << "xvel, yvel  "<< xvel << ", "<< yvel;
                }
            }
            //Move node to new position
            newPos = QPointF(v1->x()+ xvel, v1->y()+yvel);
            qDebug("current x and y: %f, %f. Possible new pos is to new x new y = %f, %f", v1->x(), v1->y(),  newPos.x(), newPos.y());
            if (newPos.x() < 5.0  ||newPos.y() < 5.0   || newPos.x() >= (canvasWidth -5)||   newPos.y() >= (canvasHeight-5)|| (v1->x() == newPos.x() && v1->y() == newPos.y() )) continue;
            qDebug("current x and y: %f, %f. This node will move to new x new y = %f, %f", v1->x(), v1->y(),  newPos.x(), newPos.y());
            emit moveNode((*v1).name(),  newPos.x(),  newPos.y());
        }

    }

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
    qreal xvel = 0, yvel = 0, dx=0, dy=0, ulv_x=0, ulv_y=0;
    qreal c_rep=10, dux=0, duy=0, natural_length=100, temperature=2;
    double dist = 0;
    QPointF curPos, newPos, pos ;

    if (dynamicMovement){
        qDebug() << "Graph: layoutForceDirectedFruchtermanReingold() "
                 << "max dx "<< canvasWidth << "max dy "<< canvasHeight;
        natural_length= sqrt ( (canvasWidth - 10)* (canvasHeight-10) / vertices() );
        qDebug () << "Graph: Setting natural_length = "<<  natural_length
                  << "...following Fruchterman-Reingold (1991) formula ";
        foreach (Vertex *v1, m_graph)  {
            qDebug() << "*****  Calculate forces for vertex " << v1->name()
                     << " with index " <<  index[v1->name()] << " and pos "<< v1->x() << ", "<< v1->y();
            if ( ! v1->isEnabled() ) {
                qDebug() << "  vertex " << v1->name() << " not enabled. Continuing...";
                continue;
            }
            xvel=0; yvel=0;
            dux=0; duy=0;
            foreach (Vertex *v2, m_graph)  {
                if ( ! v2->isEnabled() ) continue;
                qDebug () << "  v2 = "<< v2->name() << " with pos (" <<  v2->x() << "," << v2->y() << ")";
                if (v2 == v1) {
                    qDebug() << "  v1==v2, continuing";
                    continue;
                }
                dx = v2->x() - v1->x();
                dy = v2->y() - v1->y();
                dist = (dx * dx + dy * dy);
                dist = sqrt ( dist );	//the euclideian distance of the two vertices
                qDebug()<< "  v1= " << v1->name() <<  " v2= " <<  v2->name() << " - euclideian distance = " << dist;

                if ( this->hasEdge (v1->name(), v2->name())  ) {  //calculate spring (pulling) force
                    ulv_x =  dx / dist;
                    ulv_y =  dy / dist;
                    dux = ( ulv_x ) * ( dist * dist ) / natural_length;
                    duy = ( ulv_y ) * ( dist * dist ) / natural_length;
                    //limit the maximum displacement to a maximum temperature
                    xvel = ( dux / qAbs (dux) ) *  qMin( qAbs(dux), temperature) ;
                    yvel = ( duy / qAbs (duy) ) *  qMin( qAbs(duy), temperature) ;

                    qDebug() << "  v1= "<<v1->name() <<  " connected to and pulled by v2= "<< v2->name()
                             <<"  nat_length =" << natural_length
                            <<" ulv_x="<<ulv_x 	<<" ulv_y="<<ulv_y 	<<" dist= "<<dist
                           << " dux="<< dux << " duy="<< duy
                           << " xvel="<< xvel <<" yvel="<< yvel;
                    continue;
                }
                //calculate electric (repulsive) force between all vertices
                ulv_x = - dx / dist;
                ulv_y = - dy / dist;

                dux = (ulv_x * natural_length * natural_length ) / ( dist );
                duy = (ulv_y * natural_length * natural_length ) / ( dist ) ;

                //limit the maximum displacement to a maximum temperature
                xvel += ( dux / qAbs (dux) ) *  qMin( qAbs(dux), temperature) ;
                yvel += ( duy / qAbs (duy) ) *  qMin( qAbs(duy), temperature) ;

                qDebug() << "  v1 = "<<v1->name() <<  " NOT connected to and pushed away from  v2 = "<< v2->name()
                         <<"  c_rep=" << c_rep
                        <<" ulv_x="<<ulv_x 	<<" ulv_y="<<ulv_y 	<<" dist^2="<<dist * dist
                       << " dux=" << dux 	<< " duy=" << duy
                       << " xvel="<< xvel <<" yvel="<< yvel;

            }
            //Move node to new position
            newPos = QPointF(v1->x()+ xvel, v1->y()+yvel);
            qDebug(">>>  current x and y: %f, %f. Possible new pos is to new x new y = %f, %f", v1->x(), v1->y(),  newPos.x(), newPos.y());
            if (newPos.x() < 5.0  ||newPos.y() < 5.0   || newPos.x() >= (canvasWidth -5)||   newPos.y() >= (canvasHeight-5)|| (v1->x() == newPos.x() && v1->y() == newPos.y() )) continue;
            qDebug(">>> current x and y: %f, %f. This node will move to new x new y = %f, %f", v1->x(), v1->y(),  newPos.x(), newPos.y());
            emit moveNode((*v1).name(),  newPos.x(),  newPos.y());
        }

    }

}



Graph::~Graph() {
    clear();
    index.clear();
}





