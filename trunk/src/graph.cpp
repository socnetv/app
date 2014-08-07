/******************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.2
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


#include <fstream>		//for writing the adjacency matrix to a file
#include <cmath>		//allows the use of pow(float/double, float/double) function
#include <cstdlib>		//allows the use of RAND_MAX macro 

#include <QPointF>
#include <QDebug>		//used for qDebug messages
#include <QDateTime> 	// used in exporting centrality files
#include <QHash> 
#include <list>			//for list iterators
#include <queue>		//for BFS queue Q

#include "graph.h"




Graph::Graph() {
    m_totalVertices=0;
    outEdgesVert=0;
    inEdgesVert=0;
    reciprocalEdgesVert=0;
    order=true;		//returns true if the indexes of the list is ordered.
    graphModified=false;
    m_undirected=false;
    symmetricAdjacencyMatrix=true;
    adjacencyMatrixCreated=false;
    distanceMatrixCreated=false;
    calculatedIDC=false;
    calculatedODC=false;
    calculatedCentralities=false;

    dynamicMovement=false;
    timerId=0;
    layoutType=0;

    parser.setParent(this);

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
    main node creation slot, associated with homonymous signal from Parser.
    Adds a Vertex to the Graph and calls addNode of GraphicsWidget
    p holds the desired position of the new node.
    The new Vertex is named i and stores its color, label, label color, shape and position p.
*/
void Graph::createVertex(int i, int size, QString nodeColor, QString numColor, int numSize, QString label, QString lColor, int lSize, QPointF p, QString nodeShape, bool signalMW){
    int value = 1;
    addVertex(i, value, size,  nodeColor, numColor, numSize, label, lColor, lSize, p, nodeShape);
    emit drawNode( i, size,  nodeColor, numColor, numSize, label, lColor, lSize, p, nodeShape, initShowLabels, initNumbersInsideNodes, true);
    if (signalMW)
        emit graphChanged();
    initVertexColor=nodeColor; //draw new user-clicked nodes with the same color with that of the file loaded
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
    qDebug("Graph::createVertex(). Using vertex number %i with FIXED coords...", i);
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
    qDebug("Graph::createVertex(). Using vertex number %i with RANDOM node coords...", i);
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
    qDebug("Graph::createVertex(). Using vertex number %i with RANDOM node coords but with LABEL...", i);
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
    Called from homonymous signal of Parser class.
    Adds an Edge to the Graph, then emits drawEdge() which calls GraphicsWidget::addEdge() to draw the new edge.
    Also called from MW when user clicks on the "add link" button
    Alse called from GW (via createEdge() below) when user middle-clicks.
*/
void Graph::createEdge(int v1, int v2, float weight, QString color, int reciprocal=0, bool drawArrows=true, bool bezier=false){
    if ( reciprocal == 2) {
        qDebug()<<"*** Graph: createEdge() from "<<v1<<" to "<<v2<<" of weight "<<weight << " as RECIPROCAL and emitting drawEdge signal to GW";
        addEdge ( v1, v2, weight, color, reciprocal);
        emit drawEdge(v1, v2, weight, reciprocal, drawArrows, color, bezier);
    }
    else if (this->hasEdge( v2, v1) )  {
        qDebug()<<"*** Graph: createEdge() from "<<v1<<" to "<<v2<<" of weight "<<weight << ". Opposite link exists. Emitting drawEdgeReciprocal to GW to make the original reciprocal";
        reciprocal = 1;
        addEdge ( v1, v2, weight, color, reciprocal);
        emit drawEdgeReciprocal(v2, v1);
    }
    else {
        qDebug()<<"*** Graph: createEdge() from "<<v1<<" to "<<v2<<" of weight "<<weight << ". Opposite link does not exist. Emitting drawEdge to GW...";
        reciprocal = 0;
        addEdge ( v1, v2, weight, color, reciprocal);
        emit drawEdge(v1, v2, weight, reciprocal, drawArrows, color, bezier);
    }
    initEdgeColor=color; //draw new edges the same color with those of the file loaded, on user clicks on the canvas
    emit graphChanged();
}


/**
    Called (via MW::addLink()) from GW when user middle-clicks on two nodes.
    Calls the above createEdge() method with initEdgeColor to set the default edge color.
*/
void Graph::createEdge(int v1, int v2, float weight, int reciprocal=0, bool drawArrows=true, bool bezier=false){
    createEdge(v1, v2, (float) weight, initEdgeColor, reciprocal, drawArrows, bezier);
}


/**
    Called from WebCrawler when it finds an new link
    Calls the above createEdge() method with initEdgeColor
*/
void Graph::createEdge (int source, int target){
    if (this->hasEdge(source, target) ) {
        qDebug()<< " Graph::createEdge - Edge from " << source << " to " << target << " already exists - returning...";
        return;
    }
    qDebug()<< " Graph::createEdge - New edge from " << source << " to " << target ;
    float weight = 1.0;
    bool reciprocal=false;
    bool drawArrows=true;
    bool bezier=false;

    createEdge(source, target, weight, initEdgeColor, reciprocal, drawArrows, bezier);
}

/**
*	This is called from loadPajek method of Parser in order to delete any redundant (dummy) nodes.
*/
void Graph::removeDummyNode(int i){
    qDebug("**Graph: RemoveDummyNode %i", i);
    removeVertex(i);
    //	emit selectedVertex(i);

}



/**	Adds a Vertex 
    named v1, valued val, sized nszm colored nc, labeled nl, labelColored lc, shaped nsp, at point p
    This method is called by createVertex() method
*/
void Graph::addVertex (
        int v1, int val, int size, QString color,
        QString numColor, int numSize,
        QString label, QString labelColor, int labelSize,
        QPointF p, QString shape
        ){

    if (order)
        index[v1]=m_totalVertices;
    else
        index[v1]=m_graph.size();

    m_graph.append( new Vertex(this, v1, val, size, color, numColor, numSize, label, labelColor, labelSize, p, shape) );
    m_totalVertices++;

    qDebug() << "Graph: addVertex(): Vertex named " << m_graph.back()->name() << " appended with index= "<<index[v1]
                << " Now, m_graph size " << m_graph.size() << ". New vertex position: " << p.x() << "," << p.y();
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
    qDebug() << "Graph: removeVertex " << m_graph[ index[Doomed] ]->name() << "  with index= " << index[Doomed] ;
    long int indexOfDoomed=index[Doomed];

    //Remove links to Doomed from each other vertex
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        if  ( (*it)->isLinkedTo(Doomed) != 0) {
            qDebug()<< "Graph: Vertex " << (*it)->name() << " is linked to selected and has " << (*it)->outEdges() << " and " <<  (*it)->outDegree() ;
            if ( (*it)->outEdges() == 1 && (*it)->isLinkedFrom(Doomed) != 0 )	{
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
    long int tempIndex=0;
    //Find the position of the Vertex inside m_graph
    map<long int,long int>::iterator pos=index.find(Doomed);
    qDebug() << "Graph: vertex " << (pos)->first << " index "<< (pos)->second << " to be erased. ";

    //find next active vertex inside index
    (pos)->second = -1;
    while ( (pos)->second == -1 )
        ++pos;
    qDebug() << "Graph: posNext " << (pos)->first << " index "<< (pos)->second ;


    //Update the index of all subsequent vertices
    map<long int,long int>::iterator it1;
    for (it1=pos; it1!=index.end(); it1++)	{
        qDebug() << "Graph: vertex " << (it1)->first << " with index "<< (it1)->second << " will take index" << prevIndex;
        tempIndex=(it1)->second;
        (it1)->second=prevIndex;
        prevIndex=tempIndex;
        qDebug() << "Graph: now vertex " << (it1)->first << " has index "<< (it1)->second ;
    }

    //Now remove vertex Doomed from m_graph
    qDebug() << "Graph: graph vertices=size="<< vertices() << "=" << m_graph.size() <<  " removing vertex at index " << indexOfDoomed ;
    m_graph.removeAt( indexOfDoomed ) ;
    m_totalVertices--;
    qDebug() << "Graph: Now graph vertices=size="<< vertices() << "=" << m_graph.size() <<  " total edges now  " << totalEdges();

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

    qDebug()<< "Graph: addEdge() from vertex "<< v1 << "["<< source<< "] to vertex "<< v2 << "["<< target << "] of weight "<<weight;

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


    qDebug()<<"Graph: addEdge() now a("<< v1 << ","<< v2<< ") = " << m_graph [ source ]->isLinkedTo(v2) << " with color "<<  color<<" . Storing edge color..." << ". Total Links " <<m_totalEdges;
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
void Graph::filterOrphanVertices(bool filterFlag){
    if (filterFlag)
        qDebug() << "Graph: filterOrphanVertices() enabling all orphan nodes";
    else
        qDebug() << "Graph: filterOrphanVertices() disabling all orphan nodes";

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
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( (*it)->isOutLinked() ){
            (*it)->filterEdgesByWeight ( m_threshold, overThreshold );
        }
        else
            qDebug() << "Graph:filterEdgesByWeight() Vertex " << (*it)->name()
                     << " not linked. Proceeding...";
    }
}


void Graph::slotSetEdgeVisibility ( int source, int target, bool visible) {
    qDebug() << "Graph: slotSetEdgeVisibility  - emitting signal to GW";
    emit setEdgeVisibility ( source, target, visible);
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



/**	Returns the number of edges (arcs) from vertex v1
*/
int Graph::outEdges(int v1) {
    qDebug("Graph: outEdges()");
    return m_graph[ index[v1] ]->outEdges();
}


/**	
    Returns the number of edges (arcs) to vertex v1
*/
int Graph::inEdges (int v1) {
    qDebug("Graph: inEdges()");
    return m_graph[ index[v1] ]->inEdges();
}




/**	Returns the outDegree (sum of outLinks weights) of vertex v1
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
        tEdges+=(*it)->outEdges();
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
                if ( (this->hasEdge ( (*it1)->name(), (*it)->name() )  ) !=0 ) {
                    (*it)->setIsolated(false);
                    (*it1)->setIsolated(false);
                }
            }
            j++;
        }
        if ((*it)->isIsolated()) {
            m_isolatedVerticesList << i;
            qDebug()<< "Graph::createAdjacencyMatrix() - node " << i+1 << " is isolated. Marking it." ;
        }
        i++;
    }
    return m_isolatedVerticesList ;
}

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
    Returns the sum of vertices having outEdges
*/
int Graph::verticesWithOutEdges(){
    return outEdgesVert;
}

/**
    Returns the sum of vertices having inEdges
*/
int Graph::verticesWithInEdges(){
    return inEdgesVert;
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
    //qDebug("Graph: m_graph reports size %i", m_graph.size());
    m_graph.clear();
    index.clear();
    discreteIDCs.clear();
    discreteODCs.clear();
    m_isolatedVerticesList.clear();
    m_totalVertices=0;
    m_totalEdges=0;
    outEdgesVert=0;
    inEdgesVert=0;
    reciprocalEdgesVert=0;

    order=true;		//returns true if the indexes of the list is ordered.
    m_undirected=false;
    calculatedIDC=false;
    calculatedODC=false;
    calculatedCentralities=false;
    adjacencyMatrixCreated=false;
    graphModified=false;
    symmetricAdjacencyMatrix=true;

    if (parser.isRunning() )		//tell the other thread that we must quit!
        parser.quit();

    if (crawler.isRunning() ){
        //tell the other thread that we must quit!
        crawler.terminateReaderQuit();
        crawler.quit();
    }
    //qDebug("Graph: m_graph cleared. Now reports size %i", m_graph.size());
}



/**
    Returns true if the adjacency matrix is symmetric
*/
bool Graph::isSymmetric(){
    qDebug("Graph: isSymmetric ");
    if (graphModified){
        symmetricAdjacencyMatrix=true;
        imap_f::iterator it1;
        int y=0;
        QList<Vertex*>::iterator it;
        for (it=m_graph.begin(); it!=m_graph.end(); it++){ 	//for all edges of u, (u,y)
            //qDebug("Graph: isSymmetric(): GRAPH CHANGED! Iterate over all edges of u...");
            for( it1 = (*it)->m_outEdges.begin(); it1 != (*it)->m_outEdges.end(); it1++ ) {
                y=index[it1->first];
                if ( ! m_graph[y]->isLinkedTo( (*it)->name() )) {
                    //qDebug("Graph: isSymmetric():  u = %i IS NOT inLinked from y = %i", (*it)->name(), it1->first  );
                    symmetricAdjacencyMatrix=false;
                    qDebug("Graph: isSymmetric()  NO");
                    return symmetricAdjacencyMatrix;
                }
                else {
                    //	qDebug("Graph: isSymmetric():  u = %i IS inLinked from y = %i",it1->first, (*it)->name()  );
                }
            }
        }
        graphModified=false;
    }
    qDebug("Graph: isSymmetric() YES");
    return symmetricAdjacencyMatrix;
}



/**
*	Transform the directed network to symmetric (all edges reciprocal) 
*/
void Graph::symmetrize(){
    qDebug("Graph: symmetrize");
    QList<Vertex*>::iterator it;
    imap_f::iterator it1;
    int y;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        //for all edges (u,y) of u, do
        qDebug("Graph: making all edges reciprocal. First iterate over all edges of u...");
        for( it1 = (*it)->m_outEdges.begin(); it1 != (*it)->m_outEdges.end(); it1++ ) {
            y=index[it1->first];
            if ( ! m_graph[y]->isLinkedTo( (*it)->name() )) {
                qDebug() << "Graph: symmetrize: u = " << (*it)->name() << " IS NOT inLinked from y = " <<  it1->first  ;
                createEdge(it1->first, (*it)->name(), it1->second, initEdgeColor, false, true, false);
            }
            else
                qDebug() << "Graph: symmetrize: u = " << it1->first  << " IS inLinked from y = " <<   (*it)->name() ;
        }
    }
    graphModified=true;
    symmetricAdjacencyMatrix=true;
    emit graphChanged();
}



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
    if (graphModified ||  !distanceMatrixCreated ){
        createDistanceMatrix(false);
        graphModified=false;
    }
    return DM.item(index[i],index[j]);
}



/**
*  Returns the diameter of the graph, aka the largest geodesic distance between any two vertices
*/
int Graph::diameter(){
    if (graphModified){
        createDistanceMatrix(false);
        graphModified=false;
    }
    return graphDiameter;
}



/**
*  Returns the average distance of the graph
*/
float Graph::averageGraphDistance(){
    if (graphModified){
        createDistanceMatrix(false);
        graphModified=false;
    }
    return averGraphDistance;
}



/**
*  Writes the matrix of distances to a file
*/
void Graph::writeDistanceMatrix (const char* fn, const char* netName) {
    qDebug ("Graph::writeDistanceMatrix()");
    createDistanceMatrix(false);
    qDebug ("Graph::writeDistanceMatrix() writing to file");
    ofstream file (fn);
    int dist=-1;

    char one_space[]     = " ";
    char two_spaces[]    = "  ";
    char three_spaces[]  = "   ";
    char four_spaces[]   = "    ";
    char five_spaces[]   = "     ";
    char six_spaces[]    = "      "; Q_UNUSED (six_spaces);
    char seven_spaces[]  = "       ";
    char eight_spaces[]  = "        "; Q_UNUSED (eight_spaces);
    char nine_spaces[]   = "         "; Q_UNUSED (nine_spaces);
    char ten_spaces[]    = "          ";

    file << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    file << "Distance matrix of "<< netName<<": \n";
    file << "The first column is the node number "<<": \n\n";
    //write out matrix of geodesic distances
    QList<Vertex*>::iterator it, it1;
    int i=0, j=0;

    file << ten_spaces <<endl;

    i=0;
    // print rows of distances.
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        // print node number first
        file << ++i ;
        if (i>9999)
            file << two_spaces;
        if (i>999)
            file << three_spaces;
        else if (i>99)
            file << four_spaces ;
        else if(i>9)
            file << five_spaces;
        else
            file << seven_spaces;

        file << "|  ";

        j=0;

        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            ++j;
            if ( (dist= DM.item( index[(*it)->name()],  index[(*it1)->name()] ) )!=-1 ) {
                file << dist;
                if (dist>9999)
                    file << one_space;
                else if (dist>999)
                    file << one_space;
                else if (dist>99)
                    file << two_spaces;
                else if(dist>9)
                    file << three_spaces;
                else
                    file << five_spaces ;
            }
            else
                file << "0"<< five_spaces;
        }
        file << endl;
    }
    file.close();
}


/**
*  Saves the number of geodesic distances matrix to a file
*/
void Graph::writeNumberOfGeodesicsMatrix(const char* fn, const char* netName) {
    qDebug ("Graph::writeDistanceMatrix()");
    if (!distanceMatrixCreated )
        createDistanceMatrix(false);
    qDebug ("Graph::writeDistanceMatrix() writing to file");
    ofstream file (fn);
    int sigma=-1;

    char one_space[]     = " ";
    char two_spaces[]    = "  ";
    char three_spaces[]  = "   ";
    char four_spaces[]   = "    ";
    char five_spaces[]   = "     ";
    char six_spaces[]    = "      "; Q_UNUSED (six_spaces);
    char seven_spaces[]  = "       ";
    char eight_spaces[]  = "        "; Q_UNUSED (eight_spaces);
    char nine_spaces[]   = "         "; Q_UNUSED (nine_spaces);
    char ten_spaces[]    = "          ";

    file << "-Social Network Visualizer- \n";
    if (!netName) netName="Unnamed network";
    file << "Number of geodesics of "<< netName<<": \n";
    file << "The first column is the node number"<<": \n\n";
    //write out a matrix of number of Geodesics
    QList<Vertex*>::iterator it, it1;
    int i=0, j=0;

    file << ten_spaces <<endl;

    i=0;

    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        // print node number first
        file << ++i ;
        if (i>9999)
            file << two_spaces;
        if (i>999)
            file << three_spaces;
        else if (i>99)
            file << four_spaces ;
        else if(i>9)
            file << five_spaces;
        else
            file << seven_spaces;

        file << "|  ";

        j=0;

        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            ++j;
            if ( (sigma= TM.item( index[(*it)->name()],  index[(*it1)->name()] ) )!=-1 ) {
                file << sigma;
                if (sigma>999)
                    file <<  one_space;
                else if (sigma>99)
                    file <<  one_space;
                else if(sigma>9)
                    file << two_spaces;
                else
                    file << five_spaces;
            }
            else
                file << "0"<< five_spaces;
        }
        file << endl;
    }
    file.close();

}



/**
    Creates a matrix DM which stores geodesic distances between all vertices
    INPUT:
        boolean doCalculcateCentralities
    OUTPUT:
        DM(i,j)=geodesic distance between vertex i and vertex j
        TM(i,j)=number of shortest paths from vertex i to vertex j, called sigma(i,j).
        graphDiameter is set to the length of the longest shortest path between every (i,j)
        Also, if doCalculcateCentralities==true, it calculates the centralities for every u in V:
        - Betweeness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
        - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
        - Graph: CC(u) =  1/maxDistance(u,t)  for some t in V
        - Closeness: CC(u) =  1 / Sum( DM(u,t) )  for every  t in V
*/

void Graph::createDistanceMatrix(bool doCalculcateCentralities) {
    qDebug ("Graph::createDistanceMatrix()");
    if ( !graphModified && distanceMatrixCreated && !doCalculcateCentralities)  {
        qDebug("Graph: distanceMatrix not mofified. Escaping.");
        return;
    }
    //Create a NxN DistanceMatrix. Initialise values to zero.
    qDebug() << "Graph::createDistanceMatrix() Resizing Matrices to hold " << m_totalVertices << " vertices";
    DM.resize(m_totalVertices);
    TM.resize(m_totalVertices);

    int aVertices=vertices();
    int aEdges = totalEdges();    //maybe we will use m_totalEdges here to save some time?...
    isolatedVertices = verticesIsolated().count();

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
        float CC=0, BC=0, SC= 0, GC=0, EC=0, PC=0, stdGC=0, stdEC=0;
        int progressCounter=0;

        graphDiameter=0;

        distanceMatrixCreated = false;
        averGraphDistance=0;
        nonZeroDistancesCounter=0;

        qDebug() << "	graphDiameter "<< graphDiameter << " averGraphDistance " <<averGraphDistance;
        qDebug() << "	reciprocalEdgesVert "<< reciprocalEdgesVert << " inEdgesVert " << inEdgesVert
                 << " outEdgesVert "<<  outEdgesVert;
        qDebug() << "	aEdges " << aEdges <<  " aVertices " << aVertices;


        maxIndexBC=0;
        maxIndexSC=0;
        maxIndexEC=0;

        qDebug("Graph: createDistanceMatrix() - initialising variables for maximum centrality indeces");
        if (symmetricAdjacencyMatrix) {
            maxIndexBC=( aVertices-1.0) *  (aVertices-2.0)  / 2.0;
            maxIndexSC=( aVertices-1.0) *  (aVertices-2.0) / 2.0;
            maxIndexCC=1.0/(aVertices-1.0);
            maxIndexEC=aVertices-1.0;
            maxIndexPC=aVertices-1.0;
            qDebug("############# symmetricAdjacencyMatrix - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }
        else {
            maxIndexBC=( aVertices-1.0) *  (aVertices-2.0) ;
            maxIndexSC=1;
            maxIndexEC=(aVertices-1.0);
            maxIndexPC=aVertices-1.0;
            maxIndexCC=1.0/(aVertices-1.0);  //FIXME This applies only on undirected graphs
            qDebug("############# NOT SymmetricAdjacencyMatrix - maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
        }
        //float maxIndexBC-directed= (n1-1) * (n2-1)-(ns-1) , n1  vert outgoing n2 ingoing vert ns self  // all this divided by two.
        qDebug("Graph: createDistanceMatrix() - initialising variables for centrality index");
        maxCC=0; minCC=RAND_MAX; nomCC=0; denomCC=0; groupCC=0; maxNodeCC=0; minNodeCC=0; sumCC=0;
        discreteCCs.clear(); classesCC=0;
        maxBC=0; minBC=RAND_MAX; nomBC=0; denomBC=0; groupBC=0; maxNodeBC=0; minNodeBC=0; sumBC=0;
        discreteBCs.clear(); classesBC=0;
        maxSC=0; minSC=RAND_MAX; nomSC=0; denomSC=0; groupSC=0; maxNodeSC=0; minNodeSC=0; sumSC=0;
        discreteSCs.clear(); classesSC=0;
        maxGC=0; minGC=RAND_MAX; nomGC=0; denomGC=0; groupGC=0; maxNodeGC=0; minNodeGC=0; sumGC=0;
        discreteGCs.clear(); classesGC=0;
        maxPC=0; minPC=RAND_MAX; maxNodePC=0; minNodePC=0; sumPC=0;
        discretePCs.clear(); classesPC=0;
        maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0; minNodeEC=0; sumEC=0;
        discreteECs.clear(); classesEC=0;

        //Zero closeness indeces of each vertex
        if (doCalculcateCentralities)
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                (*it)->setBC( 0.0 );
                (*it)->setSC( 0.0 );
                (*it)->setGC( 0.0 );
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
                    sizeOfNthOrderNeighborhood[ i ]=0;
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

                //And graph centrality must be inverted...
                if ( (*it)->GC() != 0 ) {
                    EC=(*it)->GC();		//Eccentricity Centrality is max geodesic
                    GC=1.0/EC;		//Graph Centrality is inverted Eccentricity
                }
                else { GC=0; EC=0;}
                (*it)->setGC( GC );		//Set Graph Centrality
                (*it)->setEC( EC ); 		//Set Eccentricity Centrality

                //Resolve classes Graph centrality
                resolveClasses(GC, discreteGCs, classesGC);
                sumGC+=GC;

                stdEC =EC/(aVertices-1.0);
                (*it)->setSEC(stdEC);
                sumEC+=EC;
                minmax( EC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;

                i=1; //used in calculating power centrality
                sizeOfComponent = 1;
                PC=0;
                qDebug("PHASE 2 (ACCUMULATION): Start back propagation of dependencies. Set dependency delta[u]=0 on each vertex");
                for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
                    (*it1)->setDelta(0.0);
                    //Calculate Power Centrality: In = [ 1/(N-1) ] * ( Nd1 + Nd2 * 1/2 + ... + Ndi * 1/i )
                    // where Ndi (sizeOfNthOrderNeighborhood) is the number of nodes at distance i from this node.
                    PC += ( 1.0 / (float) i ) * sizeOfNthOrderNeighborhood[i];
                    // where N is the sum Nd0 + Nd1 + Nd2 + ... + Ndi, that is the amount of nodes in the same component as the current node
                    sizeOfComponent += sizeOfNthOrderNeighborhood[i];
                    i++;
                }
                (*it)->setPC( PC );		//Set Power Centrality
                sumPC += PC;
                minmax( PC, (*it), maxPC, minPC, maxNodePC, minNodePC) ; //Find min & max PC - not using stdSC
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
                            qDebug("Selecting Ps[w] element u=%i with delta_u=%f. sigma(u)=TM(s,u)=%f, sigma(w)=TM(s,w)=%f, delta_w=%f ", u, m_graph[u]->delta(),TM.item(s,u), TM.item(s,w), m_graph[w]->delta());
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
                        qDebug("w!=s. For this furthest vertex we need to add its new delta %f to old BC index: %f",m_graph[w]->delta(), m_graph[w]->BC());
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
                (*it)->setSCC ( CC / sumCC  );
                minmax( (*it)->SCC(), (*it), maxCC, minCC, maxNodeCC, minNodeCC) ;

                qDebug()<< "Calculating Std Graph centrality";
                GC = (*it)->GC();
                (*it)->setSGC( GC / sumGC );
                minmax( (*it)->SGC(), (*it), maxGC, minGC, maxNodeGC, minNodeGC) ;

                qDebug("Resolving SC classes...");
                SC=(*it)->SC();
                resolveClasses(SC, discreteSCs, classesSC);
                sumSC+=SC;

            }
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                BC=(*it)->BC();
                SC=(*it)->SC();

                qDebug()<< "Calculating Std Stress centrality";
                (*it)->setSSC ( SC/sumSC );
                //Find min & max SC
                minmax( (*it)->SSC(), (*it), maxSC, minSC, maxNodeSC, minNodeSC) ;

                //Calculate the numerator of groupBC according to Freeman's group Betweeness
                nomBC +=(maxBC - BC );
                //Find numerator of groupGC
                nomGC += maxGC-(*it)->SGC();
                //Find numerator of groupCC
                nomCC += maxCC- (*it)->SCC();

            }
            for (it=m_graph.begin(); it!=m_graph.end(); it++) {
                //Calculate numerator of groupSC
                nomSC +=( maxSC - (*it)->SSC() );

            }
            denomCC = aVertices-1.0;
            groupCC = nomCC/denomCC;	//Calculate group Closeness centrality

            nomBC*=2.0;
            denomBC =   (aVertices-1.0) *  (aVertices-1.0) * (aVertices-2.0);
            //denomBC =  (aVertices-1.0);
            groupBC=nomBC/denomBC;		//Calculate group Betweeness centrality

            denomGC =  aVertices-1.0;
            groupGC= nomGC/denomGC;		//Calculate group Graph centrality

            denomSC =   (aVertices-1.0); //TOFIX
            groupSC = nomSC/denomSC;	//Calculate group Stress centrality
            calculatedCentralities=true;
        }
    }

    distanceMatrixCreated=true;
    graphModified=false;

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
                it calculates GC(s) as the maximum distance from all other vertices.
                it increases sizeOfNthOrderNeighborhood [ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s
            c) Each vertex u popped from Q is pushed to a stack Stack

*/ 
void Graph::BFS(int s, bool doCalculcateCentralities){
    int u,w, dist_u=0, temp=0, dist_w=0;

    //set distance of s from s equal to 0
    DM.setItem(s,s,0);
    //set sigma of s from s equal to 1
    TM.setItem(s,s,1);

    qDebug("BFS: Construct a queue Q of integers and push source vertex s=%i to Q as initial vertex", s);
    queue<int> Q;
    //	qDebug("BFS: Q size %i", Q.size());

    Q.push(s);

    qDebug("BFS: LOOP: While Q not empty ");
    while ( !Q.empty() ) {
        qDebug("BFS: Dequeue: first element of Q is u=%i", Q.front());
        u=Q.front(); Q.pop();

        if ( ! m_graph [ u ]->isEnabled() ) continue ;

        if (doCalculcateCentralities){
            qDebug("BFS: If we are to calculate centralities, we must push u=%i to global stack Stack ", u);
            Stack.push(u);
        }
        imap_f::iterator it;
        qDebug("BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u");
        for( it = m_graph [ u ]->m_outEdges.begin(); it != m_graph [ u ]->m_outEdges.end(); it++ ) {

            w=index[it->first];
            qDebug("BFS: u=%i is connected with w=%i of index %i. ", u, it->first, w);
            qDebug("BFS: Start path discovery");
            if (	DM.item(s, w) == -1 ) { //if distance (s,w) is infinite, w found for the first time.
                qDebug("BFS: first time visiting w=%i. Enqueuing w to the end of Q", w);
                Q.push(w);
                qDebug()<<"First check if distance(s,u) = -1 (aka infinite :)) and set it to zero";
                dist_u=DM.item(s,u);
                if (dist_u < 0 )
                    dist_w = 0;
                else
                    dist_w = dist_u + 1;
                qDebug("BFS: Setting distance of w=%i from s=%i equal to distance(s,u) plus 1. New distance = %i",w,s, dist_w );
                DM.setItem(s, w, dist_w);
                averGraphDistance += dist_w;
                nonZeroDistancesCounter++;
                if (doCalculcateCentralities){
                    qDebug()<<"Calculate PC: store the number of nodes at distance " << dist_w << "from s";
                    sizeOfNthOrderNeighborhood[dist_w]=sizeOfNthOrderNeighborhood[dist_w]+1;
                    qDebug()<<"Calculate CC: the sum of distances (will invert it l8r)";
                    m_graph [s]->setCC (m_graph [s]->CC() + dist_w);
                    qDebug()<<"Calculate GC: the maximum distance (will invert it l8r) - also for Eccentricity";
                    if (m_graph [s]->GC() < dist_w )
                        m_graph [s]->setGC(dist_w);

                }
                qDebug("BFS: Checking graphDiameter");
                if ( dist_w > graphDiameter){
                    graphDiameter=dist_w;
                    qDebug() << "BFS: new graphDiameter = " <<  graphDiameter ;
                }
            }

            qDebug("BFS: Start path counting"); 	//Is edge (u,w) on a shortest path from s to w via u?
            if ( DM.item(s,w)==DM.item(s,u)+1) {
                temp= TM.item(s,w)+TM.item(s,u);
                qDebug("BFS: Found a NEW SHORTEST PATH from s=%i to w=%i via u=%i. Setting Sigma(%i, %i) = %i",s, w, u, s, w,temp);
                if (s!=w)
                    TM.setItem(s,w, temp);
                if (doCalculcateCentralities){
                    qDebug("If we are to calculate centralities, we must calculate SC as well");
                    m_graph[u]->setSC(m_graph[u]->SC()+1);

                    qDebug("BFS: appending u=%i to list Ps[w=%i] with the predecessors of w on all shortest paths from s ", u, w);
                    m_graph[w]->appendToPs(u);
                }
            }
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
void Graph::resolveClasses(float C, hash_si &discreteClasses, int &classes){
    hash_si::iterator it2;
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
void Graph::resolveClasses(float C, hash_si &discreteClasses, int &classes, int vertex){
    hash_si::iterator it2;
    it2 = discreteClasses.find(QString::number(C));    //Amort. O(1) complexity
    if (it2 == discreteClasses.end() )	{
        classes++;
        qDebug("######Vertex %i  belongs to a new centrality class. Amount of classes = %i", vertex, classes);
        discreteClasses.insert(QString::number(C), classes);
    }
}





//Calculates the Information centrality of each vertex - diagonal included
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
    float IC=0, SIC=0;
    /* Note: isolated nodes must be dropped from the AM
        Otherwise, the TM might be singular, therefore non-invertible. */
    bool dropIsolates=true;
    createAdjacencyMatrix(dropIsolates);
    n-=isolatedVertices;
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
    graphModified=false;
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
    emit statusMessage ( QString(tr("Writing information centralities to file: ")).arg(fileName) );

    outText << tr("INFORMATION CENTRALITY (IC) OF EACH NODE")<<"\n";
    outText << tr("IC measures how much information is contained in the paths that originate or end at each node.")<<"\n";
    outText << tr("IC' is the standardized IC")<<"\n";

    outText << tr("IC  range:  0 < C < inf (this index has no max value)") << "\n";
    outText << tr("IC' range:  0 < C'< 1")<<"\n\n";
    outText << "Node"<<"\tIC\t\tIC'\t\t%IC\n";
    QList<Vertex*>::iterator it;
    float IC=0, SIC=0, sumSIC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IC = (*it)->SIC();
        SIC = (*it)->SIC();
        sumSIC +=  SIC;
        outText << (*it)->name()<<"\t"<< IC << "\t\t"<< SIC  << "\t\t" <<  ( 100* SIC )<<endl;
        qDebug()<< "Graph::writeCentralityInformation() vertex: " <<  (*it)->name() << " SIC  " << SIC;
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
    outText << tr("The IC index measures the information that is contained in the paths passing through each actor.\n");
    outText << tr("The standardized values IC' can be seen as the proportion of total information flow that is controlled by each actor. Note that standard IC' values sum to unity, unlike any other centrality index.\n");
    outText << "(Wasserman & Faust, p. 196)\n";
    outText << "\n";

    float x=0;
    float n = ( this->vertices() - isolatedVertices );

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
    groupIC  = groupIC  /  (n-1);
    qDebug() << "groupIC   " << groupIC   ;
    outText << tr("\nGROUP INFORMATION CENTRALISATION (GIC)\n\n");
    outText << tr("GIC = ") << groupIC<<"\n\n";
    outText << tr("GIC range: 0 < GIC < inf \n");
    outText << tr("GIC is computed using a simple variance formula. \n");
    outText << tr("In fact, following the results of Wasserman & Faust, we are using a bias-corrected sample variance.\n ");

    outText << tr("GIC = 0, when all nodes have the same IC value, i.e. a complete or a circle graph).\n");
    outText << tr("Larger values of GIC mean larger variability between the nodes' IC values.\n");
    outText <<"(Wasserman & Faust, formula 5.20, p. 197)\n\n";


    outText << tr("Information Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}




//Calculates the PageRank centrality of each vertex
int Graph::centralityPageRank(){
    qDebug()<< "Graph:: centralityPageRank()";
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
    imap_f::iterator jt;
    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta) {
        for (it=m_graph.begin(); it!=m_graph.end(); it++){
            qDebug() << "Graph:: centralityPageRank() - calculating PR for node: " << (*it)->name() ;
            // In the first iteration, we have no PageRanks
            // So we set them to (1-d)
            if ( i == 1 ) {
                (*it)->setPRC( 1 - dampingFactor );
                qDebug() << "Graph:: centralityPageRank() - first iteration - node: " << (*it)->name() << " PR = " << (*it)->PRC() ;
                if ( (*it)->isIsolated() ) {
                    isolatedVertices++;
                    qDebug()<< "Graph:: centralityPageRank() vertex: " << (*it)->name() << " is isolated. PR will be just 1-d. Continue... ";
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
                for( jt = (*it)->m_inEdges.begin(); jt != (*it)->m_inEdges.end(); jt++ ) {
                    qDebug() << "Graph:: centralityPageRank " << (*it)->name() << " is inLinked from " << jt->first  ;
                    referrer=jt->first;
                    if ( this->hasEdge( referrer , (*it)->name() ) )
                    {

                        outDegree = m_graph[ index[referrer] ] ->outDegree();
                        PRC =  m_graph[ index[referrer] ]->PRC();
                        qDebug()<< "Graph:: centralityPageRank() " <<  jt->first  << " has PRC = " << PRC  << " and outDegree = " << outDegree << " PRC / outDegree = " << PRC / outDegree ;
                        sumPageRanksOfLinkedNodes += PRC / outDegree;
                    }

                }
                // OK. Now calculate PageRank of current node
                PRC = (1-dampingFactor) + dampingFactor * sumPageRanksOfLinkedNodes;
                // store new PageRank
                (*it) -> setPRC ( PRC );
                // calculate diff from last PageRank value for this vertex and set it to minDelta if the latter is bigger.
                qDebug()<< "Graph:: centralityPageRank() vertex: " <<  (*it)->name() << " new PageRank = " << PRC
                        << " old PR was = " << oldPRC << " diff = " << fabs(PRC - oldPRC);
                if ( maxDelta < fabs(PRC - oldPRC) ) {
                    maxDelta = fabs(PRC - oldPRC);
                    qDebug()<< "Graph:: centralityPageRank() setting new maxDelta = " <<  maxDelta;
                }

            }
        }
        if (allNodesAreIsolated) {
            qDebug()<< "Graph:: centralityPageRank() all vertices are isolated. Break...";
            qDebug() << "isolatedVertices: " << isolatedVertices << " total vertices " << this->vertices();

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
        qDebug()<< "Graph:: centralityPageRank() vertex: " <<  (*it)->name() << " PageRank = " << PRC << " standard PR = " << SPRC;
    }
    graphModified=false;
    if (allNodesAreIsolated) {
        qDebug()<< "Graph:: centralityPageRank() all vertices are isolated. Equal PageRank for all....";
        return 1;
    }
    qDebug()<< "Graph:: centralityPageRank() vertex: " <<  maxNodePRC << " has max PageRank = " << maxPRC;
    return 0;

}


//Writes the PageRank centralities to a file
void Graph::writeCentralityPageRank(const QString fileName){
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    emit statusMessage ( (tr("Calculating PageRank centralities. Please wait...")) );
    this->centralityPageRank();

    emit statusMessage ( QString(tr("Writing PageRank centralities to file: %1")).arg(fileName) );

    outText << tr("PAGERANK CENTRALITY (PRC) OF EACH NODE")<<"\n";
    outText << tr("")<<"\n";
    outText << tr("PRC' is the standardized PRC")<<"\n";

    outText << tr("PRC  range:  1-d < C  where d=") << dampingFactor   << "\n";
    outText << tr("PRC' range:  ") << dampingFactor / sumPRC  << " < C'< 1" <<"\n\n";
    outText << "Node"<<"\tPRC\t\tPRC'\t\t%PRC\n";
    QList<Vertex*>::iterator it;
    float PRC=0, SPRC=0, sumSPRC=0;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        PRC = (*it)->PRC();
        SPRC = (*it)->SPRC();
        sumSPRC +=  SPRC;
        outText << (*it)->name()<<"\t"<< PRC << "\t\t"<< SPRC  << "\t\t" <<  ( 100* SPRC )<<endl;
        qDebug()<< "Graph::writeCentralityPageRank() vertex: " <<  (*it)->name() << " SPRC  " << SPRC;
    }
    qDebug ("min %f, max %f", minPRC, maxPRC);
    if ( minPRC == maxPRC )
        outText << tr("\nAll nodes have the same PRC value.\n");
    else  {
        outText << "\n";
        outText << tr("Max PRC' = ") << maxPRC <<" (node "<< maxNodePRC  <<  ")  \n";
        outText << tr("Min PRC' = ") << minPRC <<" (node "<< minNodePRC <<  ")  \n";
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
    outText << tr("\nGROUP PAGERANK CENTRALISATION (GPC)\n\n");
    outText << tr("GPC = ") << groupPRC<<"\n\n";
    outText << tr("GPC range: 0 < GPRC < inf \n");
    outText << tr("GPC is computed using a simple variance formula. \n");



    outText << tr("PageRank Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}


/**
*	Calculates In-Degree Centralities of each vertex - diagonal included
*	Also the mean value and the variance of the in-degrees.
*/
void Graph::centralityInDegree(bool weights){
    qDebug()<< "Graph:: centralityInDegree()";
    float IDC=0, nom=0, denom=0;
    float weight;
    classesIDC=0;
    sumIDC=0;
    maxIDC=0;
    minIDC=vertices()-1;
    discreteIDCs.clear();
    varianceDegree=0;
    meanDegree=0;
    symmetricAdjacencyMatrix = true;
    QList<Vertex*>::iterator it, it1;
    hash_si::iterator it2;
    int vert=vertices();
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IDC=0;
        qDebug() << "Graph: centralityInDegree() vertex " <<  (*it)->name()  ;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( (weight=this->hasEdge ( (*it1)->name(), (*it)->name() ) ) !=0  )   {
                if (weights)
                    IDC+=weight;
                else
                    IDC++;
            }
            //check here if the matrix is symmetric - we need this below
            if ( ( this->hasEdge ( (*it1)->name(), (*it)->name() ) ) != ( this->hasEdge ( (*it)->name(), (*it1)->name() ) )   )
                symmetricAdjacencyMatrix = false;
        }
        (*it) -> setIDC ( IDC ) ;				//Set InDegree
        qDebug() << "Graph: vertex = " <<  (*it)->name() << " has IDC = " << IDC ;
        sumIDC += IDC;
        it2 = discreteIDCs.find(QString::number(IDC));
        if (it2 == discreteIDCs.end() )	{
            classesIDC++;
            qDebug("This is a new IDC class");
            discreteIDCs.insert ( QString::number(IDC), classesIDC );
        }
        qDebug("IDC classes = %i ", classesIDC);
        if (maxIDC < IDC ) {
            maxIDC = IDC ;
            maxNodeIDC=(*it)->name();
        }
        if (minIDC > IDC ) {
            minIDC = IDC ;
            minNodeIDC=(*it)->name();
        }
    }

    if (minIDC == maxIDC)
        maxNodeIDC=-1;


    meanDegree = sumIDC / (float) vert;
    qDebug("Graph: sumIDC = %f, meanDegree = %f", sumIDC, meanDegree);

    // Calculate std In-Degree, Variance and the Degree Centralisation of the whole graph.
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        IDC= (*it)->IDC();
        if (!weights) {
            (*it) -> setSIDC( IDC / (vert-1.0) );		//Set Standard InDegree
        }
        else {
            (*it) -> setSIDC( IDC / (sumIDC) );
        }
        nom+= maxIDC-IDC;
        qDebug() << "Graph: vertex = " <<  (*it)->name() << " has IDC = " << IDC << " and SIDC " << (*it)->SIDC ();

        //qDebug("Graph: IDC = %f, meanDegree = %f", IDC, meanDegree);
        varianceDegree += (IDC-meanDegree) * (IDC-meanDegree) ;
    }

    varianceDegree=varianceDegree/(float) vert;

    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);

    if (!weights) {
        groupIDC=nom/denom;
    }
    else {
        qDebug()<< "Graph::centralityInDegree vertices isolated: " << verticesIsolated().count() << ". I will subtract groupIDC by " << ((float)verticesIsolated().count()/(float)vert);
        groupIDC=( ( nom * (vert-1.0))/( denom * maxIDC) ) - ((float) verticesIsolated().count()/ (float) vert);
    }

    qDebug("Graph: varianceDegree = %f, groupIDC = %f", varianceDegree, groupIDC);

    if (!weights) {
        minIDC/=(float)(vert-1); // standardize
        maxIDC/=(float)(vert-1);
    }
    else {
        minIDC/=(float)(sumIDC); // standardize
        maxIDC/=(float)(sumIDC);
    }
    calculatedIDC=true;
    graphModified=false;
}




void Graph::writeCentralityInDegree (const QString fileName, const bool considerWeights)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    centralityInDegree(considerWeights);
    float maximumIndexValue=vertices()-1.0;

    outText << tr("IN-DEGREE CENTRALITIES (IDC) OF EACH NODE\n");
    outText << tr("IDC is the sum of incoming links to node u from all adjacent nodes.\n");
    outText << tr("If the network is weighted, IDC is the sum of incoming link weights to node u from all adjacent nodes.\n");
    outText << tr("The in-degree of the node is a measure of the \'activity\' of the node it represents\n");
    outText << tr("IDC' is the standardized IDC\n\n");
    if (considerWeights){
        maximumIndexValue=(vertices()-1.0)*maxIDC;
        outText << tr("IDC  range: 0 < C < undefined (since this is a weighted network")<<"\n";
    }
    else
        outText << tr("IDC  range: 0 < C < ")<<maximumIndexValue<<"\n";
    outText << "IDC' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tIDC\tIDC'\t%IDC\n";

    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->IDC() << "\t"<< (*it)->SIDC() << "\t" <<  (100* ((*it)->IDC()) / sumIDC)<<endl;
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

    if ( minIDC == maxIDC )
        outText << tr("All nodes have the same IDC value.") << "\n";
    else  {
        outText << tr("Max IDC' = ") << maxIDC <<" (node "<< maxNodeIDC <<  ")  \n";
        outText << tr("Min IDC' = ") << minIDC <<" (node "<< minNodeIDC <<  ")  \n";
        outText << tr("IDC classes = ") << classesIDC<<" \n";
    }

    outText << "\nGROUP IN-DEGREE CENTRALISATION (GIDC)\n\n";
    outText << "GIDC = " << groupIDC<<"\n\n";

    outText << "GIDC range: 0 < GIDC < 1\n";
    outText << "GIDC = 0, when all in-degrees are equal (i.e. regular lattice).\n";
    outText << "GIDC = 1, when one node is linked from every other node.\n";

    outText << "(Wasserman & Faust, p. 101)\n";

    if (considerWeights) {
        outText << "\nNOTE: Because the network is weighted, we normalize Group Centrality multiplying by (N-1)/maxDC, where N is the total vertices, and subtracting the percentage of isolated vertices\n";
    }

    outText << "\n\n";
    outText << tr("In-Degree Centrality Report, \n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



//Calculates the outDegree centrality of each vertex - diagonal included
void Graph::centralityOutDegree(bool weights){
    qDebug("Graph:: centralityOutDegree()");
    float ODC=0, nom=0, denom=0;
    float weight;
    classesODC=0;
    discreteODCs.clear();
    sumODC=0;
    maxODC=0;
    minODC=vertices()-1;
    varianceDegree=0;
    meanDegree=0;
    int vert=vertices();
    QList<Vertex*>::iterator it, it1;
    hash_si::iterator it2;

    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        ODC=0;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( (weight=this->hasEdge ( (*it)->name(), (*it1)->name() ) ) !=0  )   {
                qDebug() << "Graph: vertex " <<  (*it)->name() << " isLinkedTo = " <<  (*it1)->name();
                if (weights)
                    ODC+=weight;
                else
                    ODC++;
                //check here if the matrix is symmetric - we need this below
                if ( ( this->hasEdge ( (*it1)->name(), (*it)->name() ) ) != ( this->hasEdge ( (*it)->name(), (*it1)->name() ) )   )
                    symmetricAdjacencyMatrix = false;
            }
        }
        (*it) -> setODC ( ODC ) ;				//Set OutDegree
        qDebug() << "Graph: vertex " <<  (*it)->name() << " has ODC = " << ODC ;
        sumODC += ODC;
        it2 = discreteODCs.find(QString::number(ODC));
        if (it2 == discreteODCs.end() )	{
            classesODC++;
            qDebug("This is a new ODC class");
            discreteODCs.insert ( QString::number(ODC), classesODC );
        }
        qDebug("ODC classes = %i ", classesODC);
        if (maxODC < ODC ) {
            maxODC = ODC ;
            maxNodeODC=(*it)->name();
        }
        if (minODC > ODC ) {
            minODC = ODC ;
            minNodeODC=(*it)->name();
        }
    }

    if (minODC == maxODC)
        maxNodeODC=-1;

    meanDegree = sumODC / (float) vert;
    qDebug("Graph: sumODC = %f, meanDegree = %f", sumODC, meanDegree);

    // Calculate std Out-Degree, Variance and the Degree Centralisation of the whole graph.
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        ODC= (*it)->ODC();
        if (!weights) {
            (*it) -> setSODC( ODC / (vert-1.0) );		//Set Standard InDegree
        }
        else {
            (*it) -> setSODC( ODC / (sumODC) );
        }
        nom+= (maxODC-ODC);
        qDebug() << "Graph: vertex " <<  (*it)->name() << " SODC " << (*it)->SODC ();

        varianceDegree += (ODC-meanDegree) * (ODC-meanDegree) ;
    }
    varianceDegree=varianceDegree/(float) vert;

    if (symmetricAdjacencyMatrix)
        denom=(vert-1.0)*(vert-2.0);
    else
        denom=(vert-1.0)*(vert-1.0);

    if (!weights) {
        groupODC=nom/denom;
    }
    else {
        qDebug()<< "Graph::centralityOutDegree vertices isolated: " << verticesIsolated().count() << ". I will subtract groupODC by " << ((float)verticesIsolated().count()/(float)vert);
        groupODC=( ( nom * (vert-1.0))/( denom * maxODC) ) - ((float) verticesIsolated().count()/ (float) vert);
    }

    qDebug("Graph: varianceDegree = %f, groupODC = %f", varianceDegree, groupODC);

    if (!weights) {
        minODC/=(float)(vert-1); // standardize
        maxODC/=(float)(vert-1);
    }
    else {
        minODC/=(float)(sumODC); // standardize
        maxODC/=(float)(sumODC);
    }

    calculatedODC=true;
    graphModified=false;
}




void Graph::writeCentralityOutDegree (
        const QString fileName, const bool considerWeights)
{
    QFile file ( fileName );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        qDebug()<< "Error opening file!";
        emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    centralityOutDegree(considerWeights);

    float maximumIndexValue=vertices()-1.0;

    outText << tr("OUT-DEGREE CENTRALITIES (ODC) FOR EACH NODE\n");
    outText << tr("ODC is the sum of outgoing links of node u to all adjacent nodes.\n");
    outText << tr("If the network is weighted, ODC is the sum of outgoing link weights of node u to all adjacent nodes.\n");
    outText << tr("ODC' is the standardized ODC\n\n");

    if (considerWeights){
        maximumIndexValue=(vertices()-1.0)*maxIDC;
        outText << tr("ODC  range: 0 < C < undefined (since this is a weighted network")<<"\n";
    }
    else
        outText << tr("ODC  range: 0 < C < ")<<QString::number(maximumIndexValue)<<"\n";


    outText << "ODC' range: 0 < C'< 1"<<"\n\n";

    outText << "Node"<<"\tODC\tODC'\t%ODC\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->ODC() << "\t"<< (*it)->SODC() << "\t" <<  (100* ((*it)->ODC()) / sumODC)<< "\n";
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
    if ( minODC == maxODC )
        outText << tr("All nodes have the same ODC value.") << "\n";
    else  {
        outText << tr("Max ODC' = ") << maxODC <<" (node "<< maxNodeODC <<  ")  \n";
        outText << tr("Min ODC' = ") << minODC <<" (node "<< minNodeODC <<  ")  \n";
        outText << tr("ODC classes = ") << classesODC<<" \n";
    }

    outText << "\nGROUP OUT-DEGREE CENTRALISATION (GODC)\n\n";
    outText << "GODC = " << groupODC<<"\n\n";

    outText << "GODC range: 0 < GODC < 1\n";
    outText << "GODC = 0, when all out-degrees are equal (i.e. regular lattice).\n";
    outText << "GODC = 1, when one node completely dominates or overshadows the other nodes.\n";
    outText << "(Wasserman & Faust, formula 5.5, p. 177)\n\n";
    outText << "(Wasserman & Faust, p. 101)\n";

    if (considerWeights) {
        outText << "\nNOTE: Because the network is weighted, we normalize Group Centrality multiplying by (N-1)/maxDC, where N is the total vertices, and subtracting the percentage of isolated vertices\n";
    }

    outText << "\n\n";
    outText << tr("Out-Degree Centrality Report, \n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

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

    emit statusMessage ( (tr("Calculating shortest paths")) );
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing closeness centralities to file:")).arg(fileName) );

    outText << tr("CLOSENESS CENTRALITY (CC) OF EACH NODE")<<"\n";
    outText << tr("CC is the invert sum of the geodesic distances of node u from all other nodes.")<<"\n";
    outText << tr("CC' is the standardized CC")<<"\n";

    outText << tr("CC  range:  0 < C < ")<<QString::number(maxIndexCC)<<"\n";
    outText << tr("CC' range:  0 < C'< 1")<<"\n\n";
    outText << "Node"<<"\tCC\t\tCC'\t\t%CC\n";
    QList<Vertex*>::iterator it;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->CC() << "\t\t"<< (*it)->SCC() << "\t\t" <<  (100* ((*it)->CC()) / sumCC)<<endl;
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
    outText << tr("GCC = 0, when the lengths of the geodesics are all equal (i.e. a complete or a circle graph).\n");
    outText << tr("GCC = 1, when one node has geodesics of length 1 to all the other nodes, and the other nodes have geodesics of length 2 to the remaining (N-2) nodes. This is exactly the situation realised by a star graph.\n");
    outText <<"(Wasserman & Faust, formula 5.9, p. 187)\n\n";
    outText << tr("This measure focuses on how close a node is to all\n");
    outText << tr("the other nodes in the set of nodes. The idea is that a node\n");
    outText << tr("is central if it can quickly interact with all others\n");
    outText << "(Wasserman & Faust, p. 181)\n";

    outText << "\n\n";
    outText << tr("Closeness Centrality report, \n");
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

    emit statusMessage ( (tr("Calculating shortest paths")) );
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing betweeness centralities to file:")).arg(fileName) );

    outText << tr("BETWEENESS CENTRALITY (BC) OF EACH NODE")<<"\n";
    outText << tr("BC of a node u is the sum of delta (s,t,u) for all s,t in V")<<"\n";
    outText << tr("where delta (s,t,u) is the ratio of all geodesics between s and t which run through u.")<<"\n";
    outText << tr("Therefore, BC(u) reflects how often the node u lies on the geodesics between the other nodes of the network")<<"\n";
    outText << tr("BC' is the standardized BC")<<"\n";
    outText << tr("BC  range: 0 < BC < ")<<QString::number( maxIndexBC)<< tr(" (Number of pairs of nodes excluding i")<<"\n";
    outText << tr("BC' range: 0 < BC'< 1  (C' is 1 when the node falls on all geodesics)\n\n");
    outText << "Node"<<"\tBC\t\tBC'\t\t%BC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->BC() << "\t\t"<< (*it)->SBC() << "\t\t" <<  (100* ((*it)->BC()) /  sumBC)<<endl;
    }
    if ( minBC ==  maxBC)
        outText << "\n"<< tr("All nodes have the same BC value.\n");
    else {
        outText << "\n";
        outText << tr("Max BC' = ") << maxBC <<" (node "<< maxNodeBC  <<  ")  \n";
        outText << tr("Min BC' = ") << minBC <<" (node "<< minNodeBC <<  ")  \n";
        outText << tr("BC classes = ") << classesBC<<" \n";
    }

    outText << tr("\nGROUP BETWEENESS CENTRALISATION (GBC)\n\n");
    outText << tr("GBC = ") <<  groupBC <<"\n\n";

    outText << tr("GBC range: 0 < GBC < 1\n");
    outText << tr("GBC = 0, when all the nodes have exactly the same betweeness index.\n");
    outText << tr("GBC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");
    outText << "(Wasserman & Faust, formula 5.13, p. 192)\n\n";

    outText << "\n\n";
    outText << tr("Betweeness Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}


//Writes the Graph centralities to a file
void Graph::writeCentralityGraph(
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
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing graph centralities to file:")).arg(fileName) );

    outText << tr("GRAPH CENTRALITY (GC) OF EACH NODE")<<"\n";
    outText << tr("The GC of a node is the invert of the maximum of all geodesic distances from that node to all other nodes in the network.") << "\n";
    outText << tr("Nodes with high GC have short distances to all other nodes in the graph.")<< "\n";

    outText << tr("GC  range: 0 < GC < ")<<maxIndexGC<< " (GC=1 => distance from other nodes is max 1)\n";
    outText << tr("GC' range: 0 < GC'< 1  (GC'=1 => directly linked with all nodes)")<<"\n\n";

    outText << "Node"<<"\tGC\t\tGC'\t\t%GC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->GC() << "\t\t"<< (*it)->SGC() << "\t\t" <<  (100* ((*it)->GC()) /  sumGC)<<endl;
    }

    if ( minGC ==  maxGC)
        outText << tr("\nAll nodes have the same GC value.\n");
    else {
        outText << "\n";
        outText << tr("Max GC' = ") << maxGC <<" (node "<< maxNodeGC  <<  ")  \n";
        outText << tr("Min GC' = ") << minGC <<" (node "<< minNodeGC <<  ")  \n";
        outText << tr("GC classes = ") << classesGC<<" \n";
    }

    outText << tr("\nGROUP GRAPH CENTRALISATION (GGC)\n\n");

    outText << tr("GGC = ") <<  groupGC<<"\n\n";

    outText << tr("GGC range: 0 < GGC < 1\n");
    outText << tr("GGC = 0, when all the nodes have exactly the same graph index.\n");
    outText << tr("GGC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");


    outText << "\n\n";
    outText << tr("Graph Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
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

    emit statusMessage ( (tr("Calculating shortest paths")) );
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing stress centralities to file:")).arg(fileName) );

    outText << tr("STRESS CENTRALITY (SC) OF EACH NODE")<<"\n";
    outText << tr("SC(u) is the sum of sigma(s,t,u): the number of geodesics from s to t through u.")<<"\n";
    outText << tr("SC(u) reflect the total number of geodesics between all other nodes which run through u")<<"\n";

    outText << tr("SC  range: 0 < SC < ")<<QString::number(maxIndexSC)<<"\n";
    outText << tr("SC' range: 0 < SC'< 1  (SC'=1 when the node falls on all geodesics)\n\n");
    outText  << "Node"<<"\tSC\t\tSC'\t\t%SC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText <<(*it)->name()<<"\t"<<(*it)->SC() << "\t\t"<< (*it)->SSC() << "\t\t" <<  (100* ((*it)->SC()) /  sumSC)<<endl;
    }

    if ( minSC ==  maxSC)
        outText  << tr("\nAll nodes have the same SC value.\n");
    else {
        outText << "\n";
        outText << tr("Max SC' = ") << maxSC <<" (node "<< maxNodeSC  <<  ")  \n";
        outText << tr("Min SC' = ") << minSC <<" (node "<< minNodeSC <<  ")  \n";
        outText << tr("SC classes = ") << classesSC<<" \n";
    }

    outText << tr("GROUP STRESS CENTRALISATION (GSC)")<<"\n";
    outText << tr("GSC = ") <<  groupSC<<"\n\n";

    outText << tr("GSC range: 0 < GSC < 1\n");
    outText << tr("GSC = 0, when all the nodes have exactly the same stress index.\n");
    outText << tr("GSC = 1, when one node falls on all other geodesics between all the remaining (N-1) nodes. This is exactly the situation realised by a star graph.\n");

    outText << "\n\n";
    outText << tr("Stress Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
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

    emit statusMessage ( (tr("Calculating shortest paths")) );
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing eccentricity centralities to file:")).arg(fileName) );

    outText << tr("ECCENTRICITY CENTRALITY (EC) OF EACH NODE") << "\n";
    outText << tr("EC of a node u is the largest geodesic distance (u,t) for t in V") << "\n";
    outText << tr("Therefore, EC(u) reflects how far, at most, is each node from every other node.") << "\n";
    outText << tr("EC' is the standardized EC") << "\n";
    outText << tr("EC  range: 0 < EC < ") << QString::number(maxIndexEC)<< tr(" (max geodesic distance)")<<"\n";
    outText << tr("EC' range: 0 < EC'< 1 \n\n");
    outText << "Node"<<"\tEC\t\tEC'\t\t%EC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->EC() << "\t\t"<< (*it)->SEC() << "\t\t" <<  (100* ((*it)->EC()) /  sumEC)<<endl;
    }
    if ( minEC ==  maxEC)
        outText << tr("\nAll nodes have the same EC value.\n");
    else {
        outText << "\n";
        outText << tr("Max EC' = ") << maxEC <<" (node "<< maxNodeEC  <<  ")  \n";
        outText << tr("Min EC' = ") << minEC <<" (node "<< minNodeEC <<  ")  \n";
        outText << tr("EC classes = ") << classesEC<<" \n";
    }

    outText << "\n\n";
    outText << tr("Eccentricity Centrality report, \n");
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
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

    emit statusMessage ( (tr("Calculating shortest paths")) );
    createDistanceMatrix(true);
    emit statusMessage ( QString(tr("Writing Power centralities to file:")).arg(fileName) );

    outText << tr("POWER CENTRALITY (PC) OF EACH NODE") << "\n";
    outText << tr("PC of each node k, is the sum of the sizes of all Nth-order neighbourhoods with weight 1/n.") << "\n";
    outText << tr("Therefore, PC(u) is a generalised degree centrality index.") << "\n";
    outText << tr("PC' is the standardized index; divided by the total numbers of nodes in the same component minus 1") << "\n";
    outText << tr("PC  range: 0 < PC < ") << QString::number(maxIndexEC)<< tr(" (star node)")<<"\n";
    outText << tr("PC' range: 0 < PC'< 1 \n\n");
    outText << "Node"<<"\tPC\t\tPC'\t\t%PC\n";
    QList<Vertex*>::iterator it;
    for (it= m_graph.begin(); it!= m_graph.end(); it++){
        outText << (*it)->name()<<"\t"<<(*it)->PC() << "\t\t"<< (*it)->SPC() << "\t\t" <<  (100* ((*it)->PC()) /  sumPC)<<endl;
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

    outText << tr("NUMBER OF CLIQUES (CLQs) OF EACH NODE") << "\n";
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
    emit statusMessage ( QString(tr("Writing clustering coefficients to file:")).arg(fileName) );

    outText << tr("CLUSTERING COEFFICIENT (CLC) OF EACH NODE\n");
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
        //		outText << "ODC Variance = "<<  varianceDegree<<"\n\n";
    }
    if (  minCLC ==  maxCLC )
        outText << "\nAll nodes have the same clustering coefficient value.\n";
    else  {
        outText << "\nNode "<<  maxNodeCLC << " has the maximum Clustering Coefficient: " <<  maxCLC <<"  \n";
        outText << "\nNode "<<  minNodeCLC << " has the minimum Clustering Coefficient: " <<  minCLC <<"  \n";
    }

    outText << "\nGRAPH CLUSTERING COEFFICIENT (GCLC)\n\n";
    outText << "GCLC = " <<  averageCLC<<"\n\n";
    outText << tr("Range: 0 < GCLC < 1\n");
    outText << tr("GCLC = 0, when there are no cliques (i.e. acyclic tree).\n");
    outText << tr("GCLC = 1, when every node and its neighborhood are complete cliques.\n");

    outText <<"\n\n" ;
    outText << tr("Clustering Coefficient Report,\n");
    outText << tr("created by SocNetV: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";

    file.close();
}





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


    emit statusMessage ( QString(tr("Writing clustering coefficients to file:")).arg(fileName) );

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
    outText << tr("created by SocNetV on: ")<< actualDateTime.currentDateTime().toString ( QString ("ddd, dd.MMM.yyyy hh:mm:ss")) << "\n\n";
    file.close();

}



/** 
* Repositions all nodes on the periphery of different circles with radius analogous to their centrality
*/
void Graph::layoutRadialCentrality(double x0, double y0, double maxRadius, int CentralityType){
    qDebug("Graph: layoutRadialCentrality...");
    //first calculate centralities
    if ((graphModified || !calculatedCentralities) && CentralityType > 2 && CentralityType < 9 ) {
        qDebug("Graph: Calling createDistanceMatrix() to calc centralities");
        this->createDistanceMatrix(true);
    }
    else if ((graphModified || !calculatedIDC) && CentralityType == 1)
        this->centralityInDegree(true);
    else if ((graphModified || !calculatedODC) && CentralityType == 2)
        this->centralityOutDegree(true);
    else if ( CentralityType == 9 ){
        this->centralityInformation();
    }
    else if ( CentralityType == 10 ) {
        this->centralityPageRank();
    }

    double rad=0;
    double i=0, std=0;
    float C=0, maxC=0, offset=0.06;  //offset controls how far from the centre the central nodes be positioned
    double new_radius=0, new_x=0, new_y=0;
    double Pi = 3.14159265;
    int vert=vertices();

    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        switch (CentralityType) {
        case 1 : {
            qDebug("Layout according to InDegree Centralities");
            C=(*it)->SIDC();
            std= (*it)->SIDC();
            maxC=maxIDC;
            break;
        }
        case 2 : {
            qDebug("Layout according to OutDegree Centralities");
            C=(*it)->SODC();
            std= (*it)->SODC();
            maxC=maxODC;
            break;
        }
        case 3 : {
            qDebug("Layout according to Closeness Centralities");
            C=(*it)->CC();
            std= (*it)->SCC();
            maxC=maxCC;
            break;
        }
        case 4 : {
            qDebug("Layout according to Betweeness Centralities");
            C=(*it)->BC();
            std= (*it)->SBC();
            maxC=maxBC;
            break;
        }
        case 5 : {
            qDebug("Layout according to Graph Centralities");
            C=(*it)->GC();
            std= (*it)->SGC();
            maxC=maxGC;
            break;
        }
        case 6 : {
            qDebug("Layout according to Stress Centralities");
            C=(*it)->SC();
            std= (*it)->SSC();
            maxC=maxSC;
            break;
        }
        case 7 : {
            qDebug("Layout according to Eccentricity Centralities");
            C=(*it)->EC();
            std= (*it)->SEC();
            maxC=maxEC;
            break;
        }
        case 8 : {
            qDebug("Layout according to Power Centralities");
            C=(*it)->PC();
            std= (*it)->SPC();
            maxC=maxPC;
            break;
        }
        case 9 : {
            qDebug("Layout according to Information Centralities");
            C=(*it)->IC();
            std= (*it)->SIC();
            maxC=maxIC;
            break;
        }
        case 10 : {
            qDebug("Layout according to PageRank Centralities");
            C=(*it)->PRC();
            std= (*it)->SPRC();
            maxC=maxPRC;
            break;
        }
        };
        qDebug () << "Vertice " << (*it)->name() << " at x=" << (*it)->x() << ", y= "<< (*it)->y()
                  << ": C=" << C << ", stdC=" << std << ", maxradius " <<  maxRadius
                  << ", maxC " << maxC << ", C/maxC " << (C/maxC) << ", *maxRadius " << (C/maxC - 0.06)*maxRadius;
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
        qDebug("Finished Calculation. Vertice will move to x=%f and y=%f ",new_x, new_y);
        //Move node to new position
        emit moveNode((*it)->name(),  new_x,  new_y);
        i++;
        emit addGuideCircle (
                    static_cast<int> (x0),
                    static_cast<int> (y0),
                    static_cast<int> (new_radius)
                    );
    }
    graphModified=false;
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
void Graph::layoutLayeredCentrality(double maxWidth, double maxHeight, int CentralityType){
    qDebug("Graph: layoutLevelCentrality...");
    //first calculate centralities
    if ((graphModified || !calculatedCentralities) && CentralityType > 2) {
        qDebug("Graph: Calling createDistanceMatrix() to calc centralities");
        createDistanceMatrix(true);
    }
    else if ((graphModified || !calculatedIDC) && CentralityType == 1)
        centralityInDegree(true);
    else if ((graphModified || !calculatedODC) && CentralityType == 2)
        centralityOutDegree(true);

    double i=0, std=0;
    float C=0, maxC=0, offset=50;  //offset controls how far from the top the central nodes will be positioned
    double new_x=0, new_y=0;
    //	int vert=vertices();
    maxHeight-=offset;
    maxWidth-=offset;
    for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
        switch (CentralityType) {
        case 1 : {
            qDebug("Layout according to InDegree Centralities");
            C=(*it)->SIDC();
            std= (*it)->SIDC();
            maxC=maxIDC;
            break;
        }
        case 2 : {
            qDebug("Layout according to OutDegree Centralities");
            C=(*it)->SODC();
            std= (*it)->SODC();
            maxC=maxODC;
            break;
        }
        case 3 : {
            qDebug("Layout according to Closeness Centralities");
            C=(*it)->CC();
            std= (*it)->SCC();
            maxC=maxCC;
            break;
        }
        case 4 : {
            qDebug("Layout according to Betweeness Centralities");
            C=(*it)->BC();
            std= (*it)->SBC();
            maxC=maxBC;
            break;
        }
        case 5 : {
            qDebug("Layout according to Graph Centralities");
            C=(*it)->GC();
            std= (*it)->SGC();
            maxC=maxGC;
            break;
        }
        case 6 : {
            qDebug("Layout according to Stress Centralities");
            C=(*it)->SC();
            std= (*it)->SSC();
            maxC=maxSC;
            break;
        }
        };
        qDebug()<< "Vertice " << (*it)->name() << " at x="<< (*it)->x() << ", y="<<  (*it)->y() << ": C=" << C << ", stdC=" << std
                << ", maxC "<<	maxC << ", maxWidth " << maxWidth <<" , maxHeight "<<maxHeight;
        //Calculate new position
        qDebug ("C/maxC %f, *maxHeight %f, +maxHeight %f ",C/maxC, (C/maxC)*maxHeight, maxHeight-(C/maxC)*maxHeight );
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
    graphModified=false;
}



/** layman's attempt to create a random network
*/
void Graph::createRandomNetErdos(int vert, double probability){
    qDebug("Graph: createRandomNetErdos");

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
    graphChanged();
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
                qDebug()<<">>>>> REWIRING: They're linked. Do a random REWIRING Experiment between "<< i<< " and " << j << " Beta parameter is " << beta;
                if (rand() % 100 < (beta * 100))  {
                    qDebug(">>>>> REWIRING: We'l break this edge!");
                    removeEdge(i, j);
                    removeEdge(j, i);
                    qDebug()<<">>>>> REWIRING: OK. Let's create a new edge!";
                    for (;;) {	//do until we create a new edge
                        candidate=rand() % (vert+1) ;		//pick another vertex.
                        if (candidate == 0 || candidate == i) continue;
                        qDebug()<<">>>>> REWIRING: Candidate: "<< candidate;
                        if (! this->hasEdge(i, candidate) )	//Only if differs from i and hasnot edge with it
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
    graphChanged();
}


/**
    Calculates and returns the number of walks of a given length between v1 and v2
*/
int Graph::numberOfWalks(int v1, int v2, int length) {
    return 1;
}


/**
    Calculates and returns the numbers of walks of length l between all pairs of vertices
*/
void Graph::numberOfWalks(int length) {
    qDebug()<<"Graph::numberOfWalks() - first create the Adjacency Matrix AM";
    bool dropIsolates=false;
    createAdjacencyMatrix(dropIsolates);

    XSM = AM;
    XSM.pow(length, false);
}




void Graph::writeNumberOfWalksMatrix(QString fn, QString netName, int length){
    qDebug("Graph::writeNumberOfWalksMatrix() ");

    QFile file (fn);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

    bool dropIsolates=false;
    createAdjacencyMatrix(dropIsolates);

    QTextStream out(&file);

    out << "-Social Network Visualizer- \n";
    out << "Network name "<< netName<<": \n";
    out << "Total number of walks of any length (sums all walks from length 2 to "<< length <<") \n\n";

    int size = vertices();
    int maxPower = size - 1;

    XM = AM;
    Matrix PM; // the product matrix
    PM.zeroMatrix(size);
    XSM.zeroMatrix(size); // the sum of product matrices
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

    out << endl;

    out << XSM ;

}



/**
    Calculates and returns the minimum length of a path between a pair of vertices
    This method is actually a reachability test (if it returns non-zero)
*/
int Graph::minimumPathLength(int v1, int v2) {

}

/**
    Calculates and writes the reachability matrix X^R of the graph
    where the {i,j} element is 1 if the vertices i and j are reachable
    Actually, this just checks the corresponding element of X^S matrix,
    which is the sum of all product matrices of A.

*/
void Graph::writeReachabilityMatrix() {

}



/**
    Calculates and returns the number of cliques which include vertex v1
    A clique (or triangle) is a complete subgraph of three nodes.
*/	
float Graph:: numberOfCliques(int v1){
    qDebug("*** Graph::numberOfCliques(%i) ", v1);
    float cliques=0;
    int  connectedVertex1=0, connectedVertex2=0;
    qDebug() << "Graph::numberOfCliques() Source vertex " << v1 << "[" << index[v1] << "] has inDegree " << inEdges(v1) << " and outDegree "<< outEdges(v1);
    imap_f::iterator it1, it2;
    bool symmetric=false;
    if ( ! (symmetric = isSymmetric()) ) {  //graph is not symmetric
        for( it1 =  m_graph[ index[v1] ] -> m_inEdges.begin(); it1 !=  m_graph[ index[v1] ] ->m_inEdges.end(); it1++ ) {
            connectedVertex1=it1->first;
            qDebug() << "Graph::numberOfCliques() In-connectedVertex1  " << connectedVertex1 << "[" << index[connectedVertex1] << "] ...Checking inLinks....";
            for( it2 =  m_graph[ index[v1] ] -> m_inEdges.begin(); it2 !=  m_graph[ index[v1] ] ->m_inEdges.end(); it2++ ) {
                connectedVertex2=it2->first;
                if (connectedVertex1 == connectedVertex2) continue;
                else {
                    qDebug() << "Graph::numberOfCliques() Out-connectedVertex2  " << connectedVertex2 << "[" << index[connectedVertex2] << "]";
                    if ( this->hasEdge( connectedVertex1, connectedVertex2 ) ) {
                        qDebug("Graph::numberOfCliques()  %i  is connected to %i. Therefore we found a clique!", connectedVertex1, connectedVertex2);
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
                }
            }
            qDebug("Graph::numberOfCliques()  .....Checking outLinks.... ");
            for( it2 =  m_graph[ index[v1] ] -> m_outEdges.begin(); it2 !=  m_graph[ index[v1] ] ->m_outEdges.end(); it2++ ) {
                connectedVertex2=it2->first;
                if (connectedVertex1 == connectedVertex2) continue;
                else {
                    qDebug() << "Graph::numberOfCliques() Out-connectedVertex2  " << connectedVertex2 << "[" << index[connectedVertex2] << "]";
                    if ( this->hasEdge( connectedVertex1, connectedVertex2 ) || this-> hasEdge( connectedVertex2, connectedVertex1 ) ) {
                        qDebug("Graph::numberOfCliques()  %i  is connected to %i. Therefore we found a clique!", connectedVertex1, connectedVertex2);
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
                }
            }
        }

    }

    for( it1 =  m_graph[ index[v1] ] -> m_outEdges.begin(); it1 !=  m_graph[ index[v1] ] ->m_outEdges.end(); it1++ ) {
        connectedVertex1=it1->first;
        qDebug() << "Graph::numberOfCliques() Out-connectedVertex1  " << connectedVertex1 << "[" << index[connectedVertex1] << "]";
        for( it2 =  m_graph[ index[v1] ] -> m_outEdges.begin(); it2 !=  m_graph[ index[v1] ] ->m_outEdges.end(); it2++ ) {
            connectedVertex2=it2->first;
            if (connectedVertex1 == connectedVertex2) continue;
            else if ( connectedVertex1 >= connectedVertex2 && symmetric) continue;
            else {
                qDebug() << "Graph::numberOfCliques() Out-connectedVertex2  " << connectedVertex2 << "[" << index[connectedVertex2] << "]";
                if ( this->hasEdge( connectedVertex1, connectedVertex2 ) ) {
                    qDebug("Graph::numberOfCliques()  %i  is out-connected to %i. Therefore we found a clique!", connectedVertex1, connectedVertex2);
                    cliques++;
                    qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                }
                if (!symmetric)
                    if ( this->hasEdge( connectedVertex2, connectedVertex1 ) ) {
                        qDebug("Graph::numberOfCliques()  %i  is also in-connected to %i. Therefore we found a clique!", connectedVertex2, connectedVertex1);
                        cliques++;
                        qDebug("Graph::numberOfCliques() cliques = %f" ,  cliques);
                    }
            }
        }
    }
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
        totalDegree=outEdges(v1);
        return totalDegree * (totalDegree -1.0) / 2.0;
    }
    totalDegree=outEdges(v1) + inEdges(v1);  //FIXEM
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
        totalDegree=outEdges(v1);
        denom =	totalDegree * (totalDegree -1.0) / 2.0;
        qDebug("Graph:: Symmetric. Number of triples is %f.  Dividing number of cliques with it", denom);

    }
    else {
        qDebug(" Graph::Calculating number of triples");
        totalDegree=outEdges(v1) + inEdges(v1);  //FIXME
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
bool Graph::loadGraph (	QString fileName,  bool iSL, int maxWidth, int maxHeight, int fileFormat, int two_sm_mode){
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
    if ( fileName == "Krackhardt_High-tech_managers_Advice_relation.sm" ) {
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
    else if (fileName == "Padgett_Florentine_Families_Marital_relation.sm"){
        outText<< "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 1 1 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 1 0 0 0 1 0" << endl <<
                  "0 0 1 0 0 0 0 0 0 0 1 0 0 0 1 0" << endl <<
                  "0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1" << endl <<
                  "0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0" << endl <<
                  "1 1 1 0 0 0 0 0 0 0 0 0 1 1 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0" << endl <<
                  "0 0 0 1 1 0 0 0 0 0 0 0 0 0 1 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 1 1" << endl <<
                  "0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0" << endl <<
                  "0 0 0 1 1 0 0 0 0 0 1 0 1 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 0 1 0 0 0 1 0 0 0" ;
    }
    else if (fileName == "Padgett_Florentine_Families_Business_relation.sm"){
        outText<< "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 1 1 0 0 1 0 1 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 0 0 1 0 0 1 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 1 1 0 1 0 0 0 1 0 0 0 0 0" << endl <<
                  "0 0 1 0 0 1 0 0 0 1 0 0 0 1 0 1" << endl <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 1 1 1 0 0 1 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
                  "0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0";
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
    else if (fileName == "Bernard_Killworth_Fraternity_Symmetric_Observer_Data.sm"){
        /*
          Bernard & Killworth  recorded the interactions among students living in a fraternity at a West Virginia college.
                  Subjects had been residents in the fraternity from 3 months to 3 years.
          This matrix counts the number of times a pair of subjects were seen in conversation
                  by an "unobtrusive" observer (observation time: 21 hours a day, for five days).
                  */
        outText<< "0 0 2 1 0 0 2 0 0 0 1 1 2 0 0 0 1 0 1 0 0 1 0 0 0 0 0 0 2 1 1 1 0 2 1 2 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 1 1 4 1 1 " << endl <<
                  "0 0 10 0 0 2 1 0 2 0 0 0 6 2 0 1 0 0 0 1 0 10 2 0 4 0 3 0 1 1 0 0 0 0 5 1 0 4 0 0 0 0 0 1 1 0 0 5 3 0 0 0 0 1 0 1 4 0" << endl <<
                  "2 10 0 6 11 14 15 4 12 0 5 4 3 8 10 8 11 0 2 19 2 15 1 2 6 1 5 0 12 5 4 0 1 4 15 3 1 3 6 0 2 3 0 9 8 2 1 3 6 2 0 2 2 16 4 5 19 1" << endl <<
                  "1 0 6 0 2 3 9 1 8 0 0 5 0 0 2 4 3 2 2 6 0 1 1 3 1 0 5 1 1 3 0 1 1 4 1 0 1 3 2 0 1 0 0 1 1 1 1 2 1 3 0 0 2 1 2 2 3 5 " << endl <<
                  "0 0 11 2 0 2 8 1 1 1 0 0 2 0 1 1 0 0 0 3 0 0 0 0 0 0 8 0 1 5 0 0 1 0 0 0 0 0 9 2 1 0 1 8 25 0 0 0 0 0 0 0 1 2 0 0 4 0 " << endl <<
                  "0 2 14 3 2 0 30 2 8 0 4 4 1 6 2 14 9 0 1 51 0 3 2 1 0 1 6 0 3 11 2 0 15 5 3 1 0 2 2 1 3 1 0 3 2 2 6 1 3 4 0 2 8 9 3 2 18 2" << endl <<
                  "2 1 15 9 8 30 0 10 4 2 7 3 0 12 9 10 9 2 3 40 2 2 5 2 0 1 19 1 10 14 5 3 14 7 7 5 3 4 5 7 8 5 0 2 4 7 3 7 7 2 0 0 6 5 14 16 20 4" << endl <<
                  "0 0 4 1 1 2 10 0 3 0 2 0 1 3 3 3 5 0 0 6 1 0 2 3 0 1 6 0 2 0 9 1 0 1 2 4 2 5 1 0 3 5 0 0 5 0 1 3 1 1 0 1 2 5 0 2 4 2 " << endl <<
                  "0 2 12 8 1 8 4 3 0 0 5 5 2 2 4 5 6 1 0 5 0 5 0 3 3 3 3 1 2 3 1 0 2 4 4 3 5 1 2 0 1 1 1 2 0 0 4 0 1 4 0 6 1 4 3 2 7 1 " << endl <<
                  "0 0 0 0 1 0 2 0 0 0 0 0 0 0 1 2 0 0 0 0 0 0 0 0 0 0 6 0 1 0 1 0 0 0 0 0 0 1 2 2 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 " << endl <<
                  "1 0 5 0 0 4 7 2 5 0 0 0 0 1 3 3 5 3 0 7 4 1 0 3 0 0 4 0 5 1 3 0 0 2 2 3 5 3 2 0 0 1 0 2 1 4 5 2 1 0 0 0 0 4 6 6 12 0 " << endl <<
                  "1 0 4 5 0 4 3 0 5 0 0 0 0 0 0 0 0 0 0 3 0 1 0 1 1 0 0 0 2 0 2 0 1 2 3 2 2 1 0 0 0 1 0 1 1 1 0 0 1 2 0 0 1 2 0 7 3 3 " << endl <<
                  "2 6 3 0 2 1 0 1 2 0 0 0 0 2 1 3 3 0 1 0 0 6 2 0 0 0 3 0 1 0 0 0 1 1 1 0 0 1 1 1 1 1 1 0 2 1 0 0 2 0 0 0 2 4 1 0 0 0 " << endl <<
                  "0 2 8 0 0 6 12 3 2 0 1 0 2 0 3 8 11 1 4 8 0 1 0 0 1 1 4 0 8 4 6 0 3 1 5 1 1 0 0 0 1 3 0 2 2 1 1 1 0 0 0 0 1 0 2 1 5 1 " << endl <<
                  "0 0 10 2 1 2 9 3 4 1 3 0 1 3 0 9 14 0 6 9 0 2 1 2 1 0 4 0 3 0 2 1 1 4 2 3 0 6 1 0 7 1 0 7 1 1 0 0 1 1 0 0 7 6 4 9 4 0 " << endl <<
                  "0 1 8 4 1 14 10 3 5 2 3 0 3 8 9 0 26 3 1 12 0 2 0 0 1 0 7 0 5 6 5 4 2 2 2 2 0 4 4 0 2 5 1 3 2 1 1 4 0 2 0 0 8 4 2 0 11 3 " << endl <<
                  "1 0 11 3 0 9 9 5 6 0 5 0 3 11 14 26 0 3 0 9 0 1 0 0 1 0 5 0 5 2 2 4 2 1 4 2 0 1 1 1 2 3 0 3 1 0 0 3 1 2 0 0 7 7 4 0 11 0 " << endl <<
                  "0 0 0 2 0 0 2 0 1 0 3 0 0 1 0 3 3 0 0 0 3 0 0 0 0 0 0 0 1 0 0 3 0 1 1 1 1 0 1 0 0 0 0 1 0 2 0 2 0 0 0 0 0 0 2 1 0 1 " << endl <<
                  "1 0 2 2 0 1 3 0 0 0 0 0 1 4 6 1 0 0 0 5 0 0 2 1 3 0 0 0 0 1 1 0 0 1 1 1 1 2 0 1 14 1 0 1 0 0 1 0 3 0 0 0 1 0 0 3 1 2 " << endl <<
                  "0 1 19 6 3 51 40 6 5 0 7 3 0 8 9 12 9 0 5 0 3 2 3 2 1 1 7 1 10 6 6 1 13 12 9 2 1 6 2 1 10 4 0 2 2 1 2 1 6 1 0 0 12 17 11 9 23 5 " << endl <<
                  "0 0 2 0 0 0 2 1 0 0 4 0 0 0 0 0 0 3 0 3 0 0 1 0 0 0 0 0 2 0 2 0 0 1 1 1 0 1 0 0 1 1 0 0 0 5 0 1 1 0 0 0 0 1 2 4 2 1 " << endl <<
                  "1 10 15 1 0 3 2 0 5 0 1 1 6 1 2 2 1 0 0 2 0 0 1 1 7 2 1 0 3 1 0 0 0 0 1 1 1 0 2 0 0 0 0 1 0 3 0 0 2 1 0 0 0 2 1 1 3 0 " << endl <<
                  "0 2 1 1 0 2 5 2 0 0 0 0 2 0 1 0 0 0 2 3 1 1 0 0 1 0 1 0 2 0 2 0 3 1 2 1 2 2 2 1 7 1 0 1 2 0 2 0 11 1 1 0 1 4 1 2 3 1 " << endl <<
                  "0 0 2 3 0 1 2 3 3 0 3 1 0 0 2 0 0 0 1 2 0 1 0 0 0 1 0 0 1 1 1 0 0 2 1 1 0 2 0 0 0 0 0 1 0 1 0 1 0 0 0 0 0 0 0 2 1 1 " << endl <<
                  "0 4 6 1 0 0 0 0 3 0 0 1 0 1 1 1 1 0 3 1 0 7 1 0 0 0 0 0 3 1 0 0 0 0 3 0 1 1 0 0 4 0 0 1 0 0 0 0 0 0 0 0 2 1 1 1 5 0 " << endl <<
                  "0 0 1 0 0 1 1 1 3 0 0 0 0 1 0 0 0 0 0 1 0 2 0 1 0 0 1 0 0 1 0 0 0 0 1 0 0 1 3 0 0 0 0 0 1 0 0 1 2 0 0 2 0 1 1 1 2 0 " << endl <<
                  "0 3 5 5 8 6 19 6 3 6 4 0 3 4 4 7 5 0 0 7 0 1 1 0 0 1 0 0 6 6 2 1 1 4 0 1 0 2 4 0 3 2 1 1 4 1 0 5 2 0 0 0 1 2 2 4 6 2 " << endl <<
                  "0 0 0 1 0 0 1 0 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 " << endl <<
                  "2 1 12 1 1 3 10 2 2 1 5 2 1 8 3 5 5 1 0 10 2 3 2 1 3 0 6 0 0 3 1 0 0 0 20 2 2 3 3 2 1 2 0 3 3 0 1 1 1 1 0 0 0 7 1 2 10 1 " << endl <<
                  "1 1 5 3 5 11 14 0 3 0 1 0 0 4 0 6 2 0 1 6 0 1 0 1 1 1 6 0 3 0 3 0 1 0 6 1 1 1 3 1 4 1 2 0 1 0 5 1 3 1 0 0 3 2 1 6 10 2 " << endl <<
                  "1 0 4 0 0 2 5 9 1 1 3 2 0 6 2 5 2 0 1 6 2 0 2 1 0 0 2 0 1 3 0 4 0 3 1 3 0 1 0 1 3 3 0 0 1 3 0 2 1 0 0 0 1 4 1 1 3 2 " << endl <<
                  "1 0 0 1 0 0 3 1 0 0 0 0 0 0 1 4 4 3 0 1 0 0 0 0 0 0 1 0 0 0 4 0 0 2 0 0 0 0 0 0 1 0 0 0 0 3 0 6 0 0 0 0 0 0 0 0 0 1 " << endl <<
                  "0 0 1 1 1 15 14 0 2 0 0 1 1 3 1 2 2 0 0 13 0 0 3 0 0 0 1 0 0 1 0 0 0 1 1 1 0 0 0 3 1 0 0 0 0 0 0 0 1 0 0 2 8 1 0 1 3 0 " << endl <<
                  "2 0 4 4 0 5 7 1 4 0 2 2 1 1 4 2 1 1 1 12 1 0 1 2 0 0 4 1 0 0 3 2 1 0 3 1 0 0 1 1 2 1 0 0 0 3 2 2 1 3 0 0 2 4 3 4 3 6 " << endl <<
                  "1 5 15 1 0 3 7 2 4 0 2 3 1 5 2 2 4 1 1 9 1 1 2 1 3 1 0 0 20 6 1 0 1 3 0 2 1 3 2 2 3 4 2 2 0 0 1 0 6 1 0 0 1 12 2 3 6 2 " << endl <<
                  "2 1 3 0 0 1 5 4 3 0 3 2 0 1 3 2 2 1 1 2 1 1 1 1 0 0 1 0 2 1 3 0 1 1 2 0 0 0 1 0 1 2 0 1 0 3 0 0 3 0 0 0 1 0 2 10 1 1 " << endl <<
                  "0 0 1 1 0 0 3 2 5 0 5 2 0 1 0 0 0 1 1 1 0 1 2 0 1 0 0 0 2 1 0 0 0 0 1 0 0 0 3 0 1 0 0 0 1 0 4 0 2 0 1 0 2 1 0 1 3 0 " << endl <<
                  "0 4 3 3 0 2 4 5 1 1 3 1 1 0 6 4 1 0 2 6 1 0 2 2 1 1 2 0 3 1 1 0 0 0 3 0 0 0 0 1 2 1 0 0 1 0 2 0 0 1 0 0 1 6 1 1 4 2 " << endl <<
                  "0 0 6 2 9 2 5 1 2 2 2 0 1 0 1 4 1 1 0 2 0 2 2 0 0 3 4 0 3 3 0 0 0 1 2 1 3 0 0 1 0 0 0 4 9 2 1 2 5 4 3 0 0 2 2 1 2 0 " << endl <<
                  "0 0 0 0 2 1 7 0 0 2 0 0 1 0 0 0 1 0 1 1 0 0 1 0 0 0 0 0 2 1 1 0 3 1 2 0 0 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 1 2 0 0 2 0 " << endl <<
                  "1 0 2 1 1 3 8 3 1 0 0 0 1 1 7 2 2 0 14 10 1 0 7 0 4 0 3 0 1 4 3 1 1 2 3 1 1 2 0 0 0 1 1 1 1 0 0 0 9 0 0 0 4 1 1 5 1 2 " << endl <<
                  "0 0 3 0 0 1 5 5 1 0 1 1 1 3 1 5 3 0 1 4 1 0 1 0 0 0 2 0 2 1 3 0 0 1 4 2 0 1 0 0 1 0 1 1 1 1 1 1 1 0 0 0 2 1 1 0 3 1 " << endl <<
                  "0 0 0 0 1 0 0 0 1 0 0 0 1 0 0 1 0 0 0 0 0 0 0 0 0 0 1 0 0 2 0 0 0 0 2 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 " << endl <<
                  "0 1 9 1 8 3 2 0 2 0 2 1 0 2 7 3 3 1 1 2 0 1 1 1 1 0 1 0 3 0 0 0 0 0 2 1 0 0 4 0 1 1 0 0 2 0 1 0 2 1 0 0 0 2 1 2 3 0 " << endl <<
                  "0 1 8 1 25 2 4 5 0 0 1 1 2 2 1 2 1 0 0 2 0 0 2 0 0 1 4 0 3 1 1 0 0 0 0 0 1 1 9 0 1 1 0 2 0 0 1 2 4 1 1 0 0 4 0 0 1 0 " << endl <<
                  "0 0 2 1 0 2 7 0 0 1 4 1 1 1 1 1 0 2 0 1 5 3 0 1 0 0 1 0 0 0 3 3 0 3 0 3 0 0 2 0 0 1 0 0 0 0 0 5 1 0 0 0 0 0 1 2 4 1 " << endl <<
                  "0 0 1 1 0 6 3 1 4 0 5 0 0 1 0 1 0 0 1 2 0 0 2 0 0 0 0 0 1 5 0 0 0 2 1 0 4 2 1 1 0 1 0 1 1 0 0 1 2 0 2 0 3 0 0 2 6 1 " << endl <<
                  "0 5 3 2 0 1 7 3 0 0 2 0 0 1 0 4 3 2 0 1 1 0 0 1 0 1 5 0 1 1 2 6 0 2 0 0 0 0 2 0 0 1 0 0 2 5 1 0 3 2 0 0 0 2 1 0 2 0 " << endl <<
                  "1 3 6 1 0 3 7 1 1 0 1 1 2 0 1 0 1 0 3 6 1 2 11 0 0 2 2 0 1 3 1 0 1 1 6 3 2 0 5 0 9 1 0 2 4 1 2 3 0 4 0 1 4 4 2 2 3 1 " << endl <<
                  "0 0 2 3 0 4 2 1 4 0 0 2 0 0 1 2 2 0 0 1 0 1 1 0 0 0 0 0 1 1 0 0 0 3 1 0 0 1 4 0 0 0 0 1 1 0 0 2 4 0 1 0 0 1 1 1 0 3 " << endl <<
                  "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 3 0 0 0 0 0 1 0 2 0 0 1 0 0 0 0 0 0 0 0 " << endl <<
                  "0 0 2 0 0 2 0 1 6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0 0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 " << endl <<
                  "0 0 2 2 1 8 6 2 1 0 0 1 2 1 7 8 7 0 1 12 0 0 1 0 2 0 1 1 0 3 1 0 8 2 1 1 2 1 0 1 4 2 0 0 0 0 3 0 4 0 0 0 0 5 1 2 4 3 " << endl <<
                  "1 1 16 1 2 9 5 5 4 1 4 2 4 0 6 4 7 0 0 17 1 2 4 0 1 1 2 0 7 2 4 0 1 4 12 0 1 6 2 2 1 1 0 2 4 0 0 2 4 1 0 0 5 0 5 3 10 0 " << endl <<
                  "1 0 4 2 0 3 14 0 3 0 6 0 1 2 4 2 4 2 0 11 2 1 1 0 1 1 2 0 1 1 1 0 0 3 2 2 0 1 2 0 1 1 1 1 0 1 0 1 2 1 0 0 1 5 0 12 7 1 " << endl <<
                  "4 1 5 2 0 2 16 2 2 0 6 7 0 1 9 0 0 1 3 9 4 1 2 2 1 1 4 0 2 6 1 0 1 4 3 10 1 1 1 0 5 0 0 2 0 2 2 0 2 1 0 0 2 3 12 0 12 0 " << endl <<
                  "1 4 19 3 4 18 20 4 7 0 12 3 0 5 4 11 11 0 1 23 2 3 3 1 5 2 6 0 10 10 3 0 3 3 6 1 3 4 2 2 1 3 0 3 1 4 6 2 3 0 0 0 4 10 7 12 0 1 " << endl <<
                  "1 0 1 5 0 2 4 2 1 0 0 3 0 1 0 3 0 1 2 5 1 0 1 1 0 0 2 0 1 2 2 1 0 6 2 1 0 2 0 0 2 1 0 0 0 1 1 0 1 3 0 0 3 0 1 0 1 0 ";

    }
    else if (fileName == "Bernard_Killworth_Fraternity_Non_Symmetric_Cognitive_Data.sm"){
        /*
          Bernard & Killworth  recorded the interactions among students living in a fraternity at a West Virginia college.
          Subjects had been residents in the fraternity from 3 months to 3 years.
          This matrix depicts rankings made by the subjects themselves of how frequently they interacted with other subjects in the observation week.
        */
        outText<< "0 3 2 4 2 3 2 5 2 3 3 2 2 3 2 3 2 5 3 3 3 2 2 4 2 3 4 3 2 5 4 5 3 4 2 5 2 2 2 2 3 2 2 3 2 4 3 4 4 5 2 2 2 2 2 3 2 3" << endl <<
                  "3 0 2 4 3 2 1 3 2 3 3 3 5 2 1 3 2 3 3 2 1 5 3 1 4 3 2 3 3 2 2 3 3 3 4 1 3 5 2 3 2 1 3 2 3 2 3 4 4 4 2 2 2 3 2 1 1 2" << endl <<
                  "2 2 0 5 5 5 3 3 5 5 3 4 4 5 3 4 5 2 4 4 2 5 4 2 5 4 5 3 5 5 4 4 3 4 5 2 3 3 3 3 2 4 4 4 4 3 3 3 2 2 2 3 3 5 3 3 5 2" << endl <<
                  "4 4 5 0 5 5 4 3 4 4 3 4 2 4 3 4 3 5 4 4 2 3 4 5 3 4 5 3 3 5 4 4 5 5 3 5 4 3 4 1 5 1 3 4 4 1 3 3 5 5 3 3 5 3 2 4 5 5" << endl <<
                  "2 3 5 5 0 2 2 2 2 4 3 2 4 3 4 3 3 3 2 2 1 3 5 2 3 4 5 3 3 1 2 3 2 3 3 1 2 2 5 4 3 2 2 4 5 1 4 3 4 3 4 2 2 3 2 2 5 2" << endl <<
                  "3 2 5 5 2 0 5 2 3 4 3 3 3 5 4 5 4 3 2 5 2 3 3 2 2 2 3 3 3 3 4 4 5 4 4 4 4 3 3 3 4 3 2 3 2 2 4 3 4 4 2 2 5 5 3 5 3 4" << endl <<
                  "2 1 3 4 2 5 0 3 3 3 3 2 2 5 5 5 4 5 3 5 3 2 4 3 2 2 5 3 4 2 5 4 5 4 4 5 3 2 3 4 4 3 2 3 1 3 2 3 3 4 2 4 5 3 4 5 4 2" << endl <<
                  "5 3 3 3 2 2 3 0 2 3 3 2 2 2 2 2 2 5 2 2 1 2 3 5 2 2 3 3 2 3 5 4 2 4 2 1 3 2 2 2 2 2 1 2 1 3 2 3 2 4 2 2 1 2 2 3 1 3" << endl <<
                  "2 2 5 4 2 3 3 2 0 2 3 5 2 4 3 3 3 3 4 3 2 4 4 4 3 5 1 3 3 3 3 3 3 4 3 3 4 3 3 1 2 1 3 2 1 3 5 3 4 4 2 4 2 4 2 5 4 3" << endl <<
                  "3 3 5 4 4 4 3 3 2 0 3 3 2 3 2 2 3 2 3 3 2 2 2 3 1 2 5 3 4 1 3 3 2 3 4 3 3 3 4 2 4 1 2 1 3 4 1 4 3 5 5 2 1 3 3 3 1 3" << endl <<
                  "3 3 3 3 3 3 3 3 3 3 0 3 2 3 2 3 3 5 3 3 5 2 2 4 3 3 5 3 3 5 5 5 3 3 3 5 3 3 4 1 2 3 2 4 4 4 5 5 3 5 2 3 3 2 4 4 5 3" << endl <<
                  "2 3 4 4 2 3 2 2 5 3 3 0 2 3 1 3 3 5 3 3 2 3 3 3 4 3 3 3 2 1 3 3 4 3 4 5 4 3 5 2 2 1 2 2 3 3 3 3 4 3 2 4 2 4 2 5 2 4" << endl <<
                  "2 5 4 2 4 3 2 2 2 2 2 2 0 3 1 4 2 4 3 2 1 4 2 2 4 2 1 3 3 2 2 2 3 2 3 2 3 3 4 5 2 4 2 3 2 2 2 3 1 3 3 2 2 4 3 2 2 1" << endl <<
                  "3 2 5 4 3 5 5 2 4 3 3 3 3 0 3 4 4 4 4 4 2 3 4 2 2 2 4 3 5 2 3 4 4 2 3 5 3 3 4 3 4 4 3 4 4 3 2 2 4 3 3 3 4 2 3 4 3 3" << endl <<
                  "2 1 3 3 4 4 5 2 3 2 2 1 1 3 0 5 5 3 5 3 2 3 4 3 2 2 4 3 4 3 4 5 4 3 4 3 3 4 3 3 4 3 2 5 4 3 3 3 4 4 2 3 3 2 3 5 3 3" << endl <<
                  "3 3 4 4 3 5 5 2 3 2 3 3 4 4 5 0 5 5 5 4 1 2 2 4 1 2 2 3 4 3 3 4 4 3 2 4 3 4 5 3 3 5 3 3 4 3 3 4 4 3 2 2 4 3 2 5 4 3" << endl <<
                  "2 2 5 3 3 4 4 2 3 3 3 3 2 4 5 5 0 4 5 3 1 3 2 2 3 2 1 3 4 1 3 4 3 3 5 2 2 4 3 2 3 5 2 5 2 2 3 3 4 2 2 2 4 3 2 5 3 2" << endl <<
                  "5 3 2 5 3 3 5 5 3 2 5 5 4 4 3 5 4 0 2 2 4 2 2 2 1 2 2 3 1 1 3 5 3 4 2 3 2 2 2 2 2 1 1 5 1 4 1 4 1 2 2 3 1 1 4 1 1 3" << endl <<
                  "3 3 4 4 2 2 3 2 4 3 3 3 3 4 5 5 5 2 0 3 1 3 5 4 4 3 1 3 3 2 3 4 4 5 4 5 5 4 2 4 5 3 4 3 1 3 5 1 5 2 3 3 4 2 1 5 2 3" << endl <<
                  "3 2 4 4 2 5 5 2 3 3 3 3 2 4 3 4 3 2 3 0 3 4 3 2 2 1 5 3 3 5 4 5 5 5 4 5 3 4 4 4 5 3 2 3 1 4 3 4 4 5 1 2 5 5 5 3 5 2" << endl <<
                  "3 1 2 2 1 2 3 1 2 2 5 2 1 2 2 1 1 4 1 3 0 3 1 1 2 2 2 3 1 2 4 5 2 3 2 5 2 2 2 1 2 1 1 5 1 4 1 5 1 4 1 4 1 2 4 3 3 2" << endl <<
                  "2 5 5 3 3 3 2 2 4 2 2 3 4 3 3 2 3 2 3 4 3 0 2 3 5 4 4 3 4 2 3 4 2 2 4 2 3 2 5 2 4 2 2 4 2 3 3 3 3 3 1 4 1 5 3 3 2 2" << endl <<
                  "2 3 4 4 5 3 4 3 4 2 2 3 2 4 4 2 2 2 5 3 1 2 0 2 4 3 3 3 3 1 4 3 4 5 3 1 5 3 5 1 5 1 2 4 4 3 5 3 5 3 5 3 4 2 2 3 2 4" << endl <<
                  "4 1 2 5 2 2 3 5 4 3 4 3 2 2 3 4 2 2 4 2 1 3 2 0 2 3 4 3 1 3 5 4 2 5 2 1 2 2 2 2 2 1 2 2 1 2 2 1 3 4 1 3 1 1 2 3 1 4" << endl <<
                  "2 4 5 3 3 2 2 2 3 1 3 4 4 2 2 1 3 1 4 2 2 5 4 2 0 4 1 3 4 3 3 3 3 3 5 1 4 3 5 3 3 2 5 2 2 1 3 3 3 3 1 3 3 5 5 4 1 2" << endl <<
                  "3 3 4 4 4 2 2 2 5 2 3 3 2 2 2 2 2 2 3 1 2 4 3 3 4 0 1 3 1 1 2 2 3 4 3 1 4 2 4 2 2 2 4 2 3 1 3 4 3 3 4 4 1 2 1 2 1 2" << endl <<
                  "4 2 5 5 5 3 5 3 1 5 5 3 1 4 4 2 1 2 1 5 2 4 3 4 1 1 0 3 5 3 4 4 3 4 3 3 3 2 4 2 2 2 2 2 3 3 1 4 3 4 2 3 1 4 3 4 4 3" << endl <<
                  "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 0 2 1 2 2 4 2 2 3 2 3 2 1 2 1 2 4 1 1 2 2 2 3 1 2 5 3 1 1 1 1" << endl <<
                  "2 3 5 3 3 3 4 2 3 4 3 2 3 5 4 4 4 1 3 3 1 4 3 1 4 1 5 2 0 3 4 2 3 3 5 3 2 4 5 4 3 4 3 3 1 4 3 3 3 2 2 2 3 5 4 4 5 2" << endl <<
                  "5 2 5 5 1 3 2 3 3 1 5 1 2 2 3 3 1 1 2 5 2 2 1 3 3 1 3 1 3 0 2 5 3 4 4 2 3 3 2 1 4 3 3 2 1 3 5 3 2 3 2 2 5 3 2 4 5 3" << endl <<
                  "4 2 4 4 2 4 5 5 3 3 5 3 2 3 4 3 3 3 3 4 4 3 4 5 3 2 4 2 4 2 0 5 2 4 3 2 3 3 2 1 3 3 2 3 3 4 3 4 4 3 2 2 2 2 4 3 1 3" << endl <<
                  "5 3 4 4 3 4 4 4 3 3 5 3 2 4 5 4 4 5 4 5 5 4 3 4 3 2 4 2 2 5 5 0 3 4 2 2 2 3 2 1 2 1 1 2 1 5 2 5 3 2 2 2 2 1 1 3 1 2" << endl <<
                  "3 3 3 5 2 5 5 2 3 2 3 4 3 4 4 4 3 3 4 5 2 2 4 2 3 3 3 4 3 3 2 3 0 5 4 5 3 4 2 2 4 2 2 3 1 1 2 2 4 3 2 3 5 4 3 4 3 2" << endl <<
                  "4 3 4 5 3 4 4 4 4 3 3 3 2 2 3 3 3 4 5 5 3 2 5 5 3 4 4 2 3 4 4 4 5 0 2 4 4 2 3 3 3 2 1 2 1 2 2 2 5 5 1 3 4 2 3 4 2 5" << endl <<
                  "2 4 5 3 3 4 4 2 3 4 3 4 3 3 4 2 5 2 4 4 2 4 3 2 5 3 3 2 5 4 3 2 4 2 0 4 4 5 5 3 4 5 5 2 1 3 4 2 4 3 3 3 3 5 3 5 5 3" << endl <<
                  "5 1 2 5 1 4 5 1 3 3 5 5 2 5 3 4 2 3 5 5 5 2 1 1 1 1 3 3 3 2 2 2 5 4 4 0 2 3 2 3 4 1 3 3 1 4 3 3 3 2 1 2 1 3 2 5 5 2" << endl <<
                  "2 3 3 4 2 4 3 3 4 3 3 4 3 3 3 3 2 2 5 3 2 3 5 2 4 4 3 2 2 3 3 2 3 4 4 2 0 3 5 2 3 1 2 4 1 1 4 2 4 3 5 3 1 2 1 3 1 2" << endl <<
                  "2 5 3 3 2 3 2 2 3 3 3 3 3 3 4 4 4 2 4 4 2 2 3 2 3 2 2 3 4 3 3 3 4 2 5 3 3 0 2 2 3 4 3 3 1 2 3 2 3 3 2 2 3 4 2 1 2 2" << endl <<
                  "2 2 3 4 5 3 3 2 3 4 4 5 4 4 3 5 3 2 2 4 2 5 5 2 5 4 4 2 5 2 2 2 2 3 5 2 5 2 0 3 3 2 3 3 5 3 5 3 4 3 5 2 2 3 1 2 3 2" << endl <<
                  "2 3 3 1 4 3 4 2 1 2 1 2 5 3 3 3 2 2 4 4 1 2 1 2 3 2 2 1 4 1 1 1 2 3 3 3 2 2 3 0 2 1 2 2 3 2 2 2 3 3 2 4 1 4 1 4 2 2" << endl <<
                  "3 2 2 5 3 4 4 2 2 4 2 2 2 4 4 3 3 2 5 5 2 4 5 2 3 2 2 2 3 4 3 2 4 3 4 4 3 3 3 2 0 2 3 1 2 1 3 2 5 2 3 2 4 2 1 4 1 4" << endl <<
                  "2 1 4 1 2 3 3 2 1 1 3 1 4 4 3 5 5 1 3 3 1 2 1 1 2 2 2 1 4 3 3 1 2 2 5 1 1 4 2 1 2 0 4 3 1 1 2 3 2 3 1 2 4 4 2 3 3 2" << endl <<
                  "2 3 4 3 2 2 2 1 3 2 2 2 2 3 2 3 2 1 4 2 1 2 2 2 5 4 2 2 3 3 2 1 2 1 5 3 2 3 3 2 3 4 0 3 1 1 3 2 2 3 2 3 3 4 2 4 2 2" << endl <<
                  "3 2 4 4 4 3 3 2 2 1 4 2 3 4 5 3 5 5 3 3 5 4 4 2 2 2 2 4 3 2 3 2 3 2 2 3 4 3 3 2 1 3 3 0 3 4 5 3 5 4 2 3 1 3 2 4 3 2" << endl <<
                  "2 3 4 4 5 2 1 1 1 3 4 3 2 4 4 4 2 1 1 1 1 2 4 1 2 3 3 1 1 1 3 1 1 1 1 1 1 1 5 3 2 1 1 3 0 1 2 2 5 3 2 3 1 3 2 2 4 2" << endl <<
                  "4 2 3 1 1 2 3 3 3 4 4 3 2 3 3 3 2 4 3 4 4 3 3 2 1 1 3 1 4 3 4 5 1 2 3 4 1 2 3 2 1 1 1 4 1 0 1 5 2 5 1 3 1 2 3 5 2 2" << endl <<
                  "3 3 3 3 4 4 2 2 5 1 5 3 2 2 3 3 3 1 5 3 1 3 5 2 3 3 1 2 3 5 3 2 2 2 4 3 4 3 5 2 3 2 3 5 2 1 0 2 4 3 5 3 1 2 4 3 5 2" << endl <<
                  "4 4 3 3 3 3 3 3 3 4 5 3 3 2 3 4 3 4 1 4 5 3 3 1 3 4 4 2 3 3 4 5 2 2 2 3 2 2 3 2 2 3 2 3 2 5 2 0 2 5 1 3 1 2 2 5 1 2" << endl <<
                  "4 4 2 5 4 4 3 2 4 3 3 4 1 4 4 4 4 1 5 4 1 3 5 3 3 3 3 2 3 2 4 3 4 5 4 3 4 3 4 3 5 2 2 5 5 2 4 2 0 4 3 3 4 3 2 3 3 5" << endl <<
                  "5 4 2 5 3 4 4 4 4 5 5 3 3 3 4 3 2 2 2 5 4 3 3 4 3 3 4 3 2 3 3 2 3 5 3 2 3 3 3 3 2 3 3 4 3 5 3 5 4 0 2 3 2 4 4 5 2 4" << endl <<
                  "2 2 2 3 4 2 2 2 2 5 2 2 3 3 2 2 2 2 3 1 1 1 5 1 1 4 2 1 2 2 2 2 2 1 3 1 5 2 5 2 3 1 2 2 2 1 5 1 3 2 0 2 1 2 1 1 1 1" << endl <<
                  "2 2 3 3 2 2 4 2 4 2 3 4 2 3 3 2 2 3 3 2 4 4 3 3 3 4 3 2 2 2 2 2 3 3 3 2 3 2 2 4 2 2 3 3 3 3 3 3 3 3 2 0 1 2 1 4 2 3" << endl <<
                  "2 2 3 5 2 5 5 1 2 1 3 2 2 4 3 4 4 1 4 5 1 1 4 1 3 1 1 5 3 5 2 2 5 4 3 1 1 3 2 1 4 4 3 1 1 1 1 1 4 2 1 1 0 4 2 3 3 3" << endl <<
                  "2 3 5 3 3 5 3 2 4 3 2 4 4 2 2 3 3 1 2 5 2 5 2 1 5 2 4 3 5 3 2 1 4 2 5 3 2 4 3 4 2 4 4 3 3 2 2 2 3 4 2 2 4 0 5 5 3 3" << endl <<
                  "2 2 3 2 2 3 4 2 2 3 4 2 3 3 3 2 2 4 1 5 4 3 2 2 5 1 3 1 4 2 4 1 3 3 3 2 1 2 1 1 1 2 2 2 2 3 4 2 2 4 1 1 2 5 0 4 1 3" << endl <<
                  "3 1 3 4 2 5 5 3 5 3 4 5 2 4 5 5 5 1 5 3 3 3 3 3 4 2 4 1 4 4 3 3 4 4 5 5 3 1 2 4 4 3 4 4 2 5 3 5 3 5 1 4 3 5 4 0 5 3" << endl <<
                  "2 1 5 5 5 3 4 1 4 1 5 2 2 3 3 4 3 1 2 5 3 2 2 1 1 1 4 1 5 5 1 1 3 2 5 5 1 2 3 2 1 3 2 3 4 2 5 1 3 2 1 2 3 3 1 5 0 3" << endl <<
                  "3 2 2 5 2 4 2 3 3 3 3 4 1 3 3 3 2 3 3 2 2 2 4 4 2 2 3 1 2 3 3 2 2 5 3 2 2 2 2 2 4 2 2 2 2 2 2 2 5 4 1 3 3 3 3 3 3 0";

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

    f.close();
}




/** 
    Returns the adjacency matrix of G
    This is called from saveGraphToAdjacency() using << operator of Matrix class
    The resulting matrix HAS spaces between elements.
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
    graphModified=false;
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
    Writes the adjacency matrix of G to a specified file
    This is called by MainWindow::slotViewAdjacencyMatrix()
    The resulting matrix HAS NO spaces between elements.
*/
void Graph::writeAdjacencyMatrix (const char* fn, const char* netName) {
    qDebug()<<"Graph::writeAdjacencyMatrix() ";
    ofstream file (fn);
    int sum=0;
    float weight=0;
    file << "-Social Network Visualizer- \n";
    file << "Adjacency matrix of "<< netName<<": \n\n";
    QList<Vertex*>::iterator it, it1;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() ) continue;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( ! (*it1)->isEnabled() ) continue;
            if ( (weight =  this->hasEdge ( (*it)->name(), (*it1)->name() )  )!=0 ) {
                sum++;
                if (weight >= 1)
                    file << static_cast<int> (weight) << " "; // TODO make the matric look symmetrical
            }
            else
                file << "0 ";
        }
        file << endl;
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
 *  Used in Graph::centralityInformation()
 */
void Graph::createAdjacencyMatrix(bool dropIsolates){
    qDebug() << "Graph::createAdjacencyMatrix()";
    float m_weight=-1;
    int i=0, j=0;
    isolatedVertices = 0;
    AM.resize(m_totalVertices);

    QList<Vertex*>::iterator it, it1;
    QList<int> isolatesList;
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        (*it)->setIsolated(true);
        if ( ! (*it)->isEnabled() )
            continue;
        j=i;
        for (it1=it; it1!=m_graph.end(); it1++){
            (*it1)->setIsolated(true);
            if ( ! (*it1)->isEnabled() )
                continue;
            if ( (m_weight = this->hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
                AM.setItem(i,j, m_weight );
                (*it)->setIsolated(false);
                (*it1)->setIsolated(false);
            }
            else{
                AM.setItem(i,j, 0);
            }
            qDebug()<<" AM("<< i+1 << ","<< j+1 << ") = " <<  AM.item(i,j);
            if (i != j ) {
                if ( (m_weight = this->hasEdge ( (*it1)->name(), (*it)->name() )  ) !=0 ) {
                    AM.setItem(j,i, m_weight );
                    (*it)->setIsolated(false);
                    (*it1)->setIsolated(false);
                }
                else {
                    AM.setItem(j,i, 0);
                }
                qDebug()<<" AM("<< j+1 << ","<< i+1 << ") = " <<  AM.item(j,i);
            }
            j++;
        }
        if ((*it)->isIsolated()) {
            isolatesList << i;
            qDebug()<< "Graph::createAdjacencyMatrix() - node " << i+1 << " is isolated. Marking it." ;
        }
        i++;
    }
    if (dropIsolates){
        qDebug()<< "Graph::createAdjacencyMatrix() - Dropping all isolated nodes.";
        for (int k = 0; k < isolatesList.size(); ++k) {
            AM.deleteRowColumn( isolatesList.at(k) );
        }
        isolatedVertices=isolatesList.size();
        qDebug() << "Graph::createAdjacencyMatrix() - Total isolates found: " << isolatedVertices;
    }
    qDebug() << "Graph::createAdjacencyMatrix() - Done.";
}


void Graph::invertAdjacencyMatrix(){
    qDebug() << "Graph::invertAdjacencyMatrix()";

    invAM.resize(m_totalVertices);

    qDebug()<<"Graph::invertAdjacencyMatrix() - first create the Adjacency Matrix AM";
    bool dropIsolates=false;
    createAdjacencyMatrix(dropIsolates);

    qDebug()<<"Graph::invertAdjacencyMatrix() - invert the Adjacency Matrix AM and store it to invAM";
    invAM.inverseByGaussJordanElimination(AM);


}



void Graph::writeInvertAdjacencyMatrix(const char* fn, const char* netName){
    qDebug("Graph::writeInvertAdjacencyMatrix() ");
    int i=0, j=0;
    QList<Vertex*>::iterator it, it1;
    ofstream file (fn);
    file << "-Social Network Visualizer- \n";
    file << "Invert Matrix of "<< netName<<": \n\n";
    invertAdjacencyMatrix();
    for (it=m_graph.begin(); it!=m_graph.end(); it++){
        if ( ! (*it)->isEnabled() )
            continue;
        j=0;
        for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
            if ( ! (*it1)->isEnabled() )
                continue;
            file << invAM.item(i,j)<< " ";
            qDebug() << invAM.item(i,j)<< " ";
            j++;
        }
        i++;
        file << endl;
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
                    xvel = ( dux / abs (dux) ) *  qMin( abs(dux), temperature) ;
                    yvel = ( duy / abs (duy) ) *  qMin( abs(duy), temperature) ;

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
                xvel += ( dux / abs (dux) ) *  qMin( abs(dux), temperature) ;
                yvel += ( duy / abs (duy) ) *  qMin( abs(duy), temperature) ;

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





