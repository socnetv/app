/******************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.49
 Written in Qt 4.4
 
                         graph.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
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
#include <cmath>		//for pow(float/double, float/double) function
#include <QPointF>
#include <QtDebug>		//used for qDebug messages
#include <list>			//for list iterators
#include <queue>		//for BFS queue Q

#include "graph.h"
#include "mainwindow.h" //for parent



Graph::Graph() {
	m_totalVertices=0;
	outEdgesVert=0;
	inEdgesVert=0;
	reciprocalEdgesVert=0;
	order=TRUE;		//returns true if the indexes of the list is ordered.
	graphModified=FALSE;
	symmetricAdjacencyMatrix=TRUE;
	adjacencyMatrixCreated=FALSE;
	distanceMatrixCreated=FALSE;
	calculatedIDC=FALSE;
	calculatedODC=FALSE;
	calculatedCentralities=FALSE;

	dynamicMovement=FALSE;
	timerId=0;
	layoutType=0;

	parser.setParent(this);
	connect (&parser, SIGNAL( createNode (int,int,QString, QString, QString, QPointF, QString, bool) ), this, SLOT(createVertex(int,int,QString, QString, QString, QPointF, QString) ) ) ;

	connect (&parser, SIGNAL(createEdge (int, int, float, QString, bool, bool, bool)), this, SLOT(createEdge (int, int, float, QString, bool, bool, bool) ) );

	connect (&parser, SIGNAL(fileType(int, QString, int, int)), this, SLOT(fileType(int, QString, int, int)) );
	connect (&parser, SIGNAL(removeDummyNode(int)), this, SLOT (removeDummyNode(int)) );
}




/**
	main node creation slot, associated with homonymous signal from Parser. 
	Adds a Vertex to the Graph and calls addNode of GraphicsWidget 
	p holds the desired position of the new node.
	The new Vertex is named i and stores its color, label, label color, shape and position p.
*/
void Graph::createVertex(int i, int size, QString nodeColor, QString label, QString lColor, QPointF p, QString nodeShape){
	qDebug()<<"*** Graph:: createVertex(): Calling AddVertex for node: "<<i<< " Attributes: "<<size<<" "<<nodeColor<<" "<<label<<" "<<lColor<<" "<<p.x()<<" " <<p.y()<<" "<<nodeShape;
	//add the vertex to the Graph.
	addVertex(i, 1, size,  nodeColor, label, lColor, p, nodeShape);
	//emit a signal for MW to create the new node onto the canvas.
	emit drawNode( i, size ,  nodeColor, label, lColor, p, nodeShape, initShowLabels, true);
	emit graphChanged(); 
	initVertexColor=nodeColor; //just to draw new nodes of the same color with that of the file loaded, when user clicks on the canvas
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
	createVertex(i, initVertexSize,  initVertexColor, QString::number(i), initVertexLabelColor, p, initVertexShape);
}


/**
	second auxilliary node creation slot. 
	Called from MW only with parameter i.
	Calculates a random position p from canvasWidth and Height.
	Then calls the main creation slot with init node values.
*/
void Graph::createVertex(int i, int cWidth, int cHeight){
	if ( i < 0 )  i = lastVertexNumber() +1;
	qDebug("Graph:: createVertex(). Using vertex number %i with RANDOM node coords...", i);
	QPointF p;
	p.setX(rand()%cWidth);
       	p.setY(rand()%cHeight);
	createVertex(i, initVertexSize,  initVertexColor, QString::number(i), initVertexLabelColor, p, initVertexShape);
}


/**
	slot associated with homonymous signal from Parser. 
	Adds an Edge to the activeGraph and calls addEdge of GraphicsWidget to update the Graphics View. 
	Also called from MW when user clicks on the "add link" button.
*/
void Graph::createEdge(int v1, int v2, float weight, QString color, bool reciprocal=false, bool drawArrows=true, bool bezier=false){
	qDebug()<<"*** Graph: createEdge():"<<v1<<" "<<v2<<" "<<weight;

	if ( reciprocal ) {
		qDebug (" Graph:: createEdge() RECIPROCAL new link -- Adding new edge to Graph and calling GW::drawEdge(). ");
		addEdge ( v1, v2, weight, color, reciprocal);
		
		emit drawEdge(v1, v2, reciprocal, drawArrows, color, bezier, false);
	}
	else if (this->hasEdge( v2, v1) )  {  
		qDebug (" Graph:: createEdge() opposite link EXISTS - Adding new edge to Graph and emitting drawEdgeReciprocal() to make the original RECIPROCAL. ");
		reciprocal = true;
		addEdge ( v1, v2, weight, color, reciprocal);
		emit drawEdgeReciprocal(v2, v1);

	}
	else {
		qDebug (" Graph:: createEdge() NOT RECIPROCAL new link -- Adding new edge to Graph and calling GW::drawEdge(). ");
		reciprocal = false;
		addEdge ( v1, v2, weight, color, reciprocal);
		emit drawEdge(v1, v2, reciprocal, drawArrows, color, bezier, false);

	}
	initEdgeColor=color; //just to draw new edges of the same color with those of the file loaded, when user clicks on the canvas
	emit graphChanged(); 
}


/**
	Called from GraphicsWidget when user middle clicks
	Calls main createEdge() method with initEdgeColor.
*/
void Graph::createEdge(int v1, int v2, int weight, bool reciprocal=false, bool drawArrows=true, bool bezier=false){
	createEdge(v1, v2, (float) weight, initEdgeColor, reciprocal, drawArrows, bezier);
}


/**
*	This is called from loadPajek method of Parser in order to delete any redundant (dummy) nodes.
*/
void Graph::removeDummyNode(int i){
	qDebug("**Graph: RemoveDummyNode %i", i);
//	removeVertex(i);
	( (MainWindow*)parent() )->clickedJimNumber=i;
	( (MainWindow*)parent() )->slotRemoveNode();

}

void Graph::setParent( QMainWindow *mp){
	m_parent=mp;
}

QMainWindow* Graph::parent(){
	return m_parent;
}

/**	Adds a Vertex 
	named v1, valued val, sized nszm colored nc, labeled nl, labelColored lc, shaped nsp, at point p
	This method is called by createVertex() method
*/
void Graph::addVertex (int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp){ 
	qDebug ("Graph: addVertex(): Appending vertex %i to graph", v1);
	if (order)
		index[v1]=m_totalVertices; 
	else 
		index[v1]=m_graph.size();

	m_graph.append( new Vertex(v1, val, nsz, nc, nl, lc, p, nsp) );
	m_totalVertices++;		

	qDebug("Graph: addVertex(): Vertex named %i appended with index=%i. Now, m_graph size %i. New vertex position: %f, %f",m_graph.back()->name(), index[v1], m_graph.size(), p.x(), p.y() );

	graphModified=true;
}




/**
	updates MW  with the file type (0=nofile, 1=Pajek, 2=Adjacency etc)
*/
void Graph::fileType(int type, QString networkName, int aNodes, int totalLinks){
	qDebug("Graph: fileType %i", type);
	( (MainWindow*)parent() )->fileType(type, networkName, aNodes, totalLinks);
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
	It removes edges to Doomed from other vertices
	Then it changes the index of all subsequent vertices inside m_graph
	Finally, it removes the vertex.
*/
void Graph::removeVertex(int Doomed){
	qDebug("Graph: removeVertex %i. Graph has %i=%i total Vertices and %i total Edges", Doomed, vertices(), m_graph.size(), totalEdges());
	int indexOfDoomed=index[Doomed];
	int outEdgesOfDoomed= m_graph[indexOfDoomed]->outLinks(); 
	int inEdgesOfDoomed = m_graph[indexOfDoomed]->inLinks(); 
	qDebug("Graph: Vertex %i with index=%i has %i OutEdges and %i InEdges.",m_graph[ index[Doomed] ]->name(), index[Doomed], outEdgesOfDoomed, inEdgesOfDoomed);
	
	//Decrease the variable which count vertices with in- and out-edges
	if (!isSymmetric()) {
		if (outEdgesOfDoomed>0) {
			outEdgesVert--;
			m_graph[indexOfDoomed]->setOutLinked(FALSE);
		}
		if (inEdgesOfDoomed>0) {
			inEdgesVert--;
			m_graph[indexOfDoomed]->setInLinked(FALSE);
		}

	}
	else {//FIXME
		if (m_graph[indexOfDoomed]->isReciprocalLinked())
			reciprocalEdgesVert--;
			m_graph[indexOfDoomed]->setReciprocalLinked(FALSE);
	}

	//Remove links to Doomed from each other vertex	
	for (QList<Vertex*>::iterator it=m_graph.begin(); it!=m_graph.end(); it++){
		if  ( (*it)->isLinkedTo(Doomed) != 0) {
			qDebug("Graph: Vertex %i  is linked to selected and has %i outEdges.",(*it)->name(), (*it)->outLinks());
			//Decrease the variables which count vertices with out and reciprocal edges 
			if ( (*it)->outLinks()==1 && !isSymmetric()) {
				qDebug("Graph: decreasing outEdgesVert");
				outEdgesVert--;
				(*it)->setOutLinked(FALSE);
			}
			else if ( (*it)->outLinks()==1 && isSymmetric() )	{
				qDebug("Graph: decreasing reciprocalEdgesVert");
				reciprocalEdgesVert--;
				outEdgesVert--;
				(*it)->setReciprocalLinked(FALSE);
				(*it)->setOutLinked(FALSE);
			}
			(*it)->removeLinkTo(Doomed) ;
		}
		else if (m_graph [indexOfDoomed]->isLinkedTo ((*it)->name()) !=0 ) {
			if ( (*it)->inLinks()==1 ) {
				inEdgesVert--;
				(*it)->setInLinked(FALSE);
				(*it)->removeLinkFrom(Doomed);
			}
		}
		qDebug("Graph: Now inEdgesVert = %i, outEdgesVert = %i  and reciprocal = %i.",inEdgesVert, outEdgesVert, reciprocalEdgesVert);
	}
	//Update the index mapping vertices inside m_graph
	qDebug("Graph: Finished with vertices. Updating index");
	int prevIndex=indexOfDoomed;
	int tempIndex=-1;
	//Find the position of the Vertex inside m_graph
	map<int,int>::iterator pos=index.find(Doomed);
	qDebug("Graph: vertex %i to be erased with index %i", (pos)->first, (pos)->second );

	//Disable the value/position of Doomed inside index. Then find next vertex inside index
	(pos)->second = -1;
	while ((pos)->second ==-1) ++pos;
	qDebug("Graph: posNext %i index %i", (pos)->first, (pos)->second );

	//Update the index of all subsequent vertices
	for (map<int,int>::iterator it1=pos; it1!=index.end(); it1++)	{
		qDebug("Graph: Vertex %i with index %i will take prevIndex %i. TempIndex is %i", (it1)->first, (it1)->second,prevIndex, tempIndex);
		tempIndex=(it1)->second;
		(it1)->second=prevIndex;
		prevIndex=tempIndex;
		qDebug("Graph: now vertex %i has index %i", (it1)->first, (it1)->second);
	}
	
	//Now remove vertex Doomed from m_graph
	qDebug("Graph: graph vertices=size=%i=%i removing at %i",  vertices(), m_graph.size(), indexOfDoomed );
	m_graph.removeAt( indexOfDoomed ) ;
	m_totalVertices--;

	qDebug("Graph: Now graph vertices=size=%i=%i. TotalEdges now %i ", vertices(), m_graph.size(), totalEdges());

	order=false;
	graphModified=true;
	emit graphChanged(); 
}



/**	Creates an edge between v1 and v2
*/
void Graph::addEdge (int v1, int v2, float weight, QString color, bool reciprocal) {
	int source=index[v1];
	int target=index[v2];

	qDebug("Graph: addEdge FROM %i with %i TO  %i with %i, weight %f", v1, source,v2,target, weight);

	if ( !m_graph [ source ]->isOutLinked() ) {
		qDebug("Graph: addEdge() SOURCE %i reports no outlinks -- setting outLinked TRUE for it.", v1);
		m_graph [ source ]->setOutLinked(TRUE) ;
		outEdgesVert++;
	}
	if ( ! m_graph [ source ]->isReciprocalLinked() &&  m_graph [ target ]->isLinkedTo (v1) ) {
		reciprocalEdgesVert++;
		m_graph [ source ]->setReciprocalLinked(TRUE);
		if ( ! m_graph [ target ]->isReciprocalLinked() ) {
			m_graph [ target ]->setReciprocalLinked(TRUE);
			reciprocalEdgesVert++;
		}
		qDebug("Graph: addEdge() SOURCE IS INLINKED BY TARGET. INCREASING RECIPROCAL %i", reciprocalEdgesVert);
	}
	
	if ( !m_graph [ target ]->isInLinked() ) {
		qDebug("Graph: addEdge() TARGET %i reports no inLinks -- setting inLinked TRUE for it", v2);
		inEdgesVert++;
		m_graph [ target ]->setInLinked(TRUE) ;
	}


	m_graph [ source ]->addLinkTo(v2, weight );
	m_graph [ target ]->addLinkFrom(v1);
	m_totalEdges++;

	if (reciprocal) {
		if (! m_graph [ target ]->isOutLinked() ) {
			outEdgesVert++;
			m_graph [ target ]->setOutLinked(TRUE);
		}
		if ( !m_graph [ source ]->isInLinked() ) {
			inEdgesVert++;
			m_graph [ source]->setInLinked(TRUE);
		}

		m_graph [ target ]->addLinkTo(v1, weight );
		m_graph [ source ]->addLinkFrom(target);
		m_totalEdges++;
	}
	qDebug()<<"Graph: addEdge() vertex "<< v1 << " reports that it has an edge to vertex "<< v2<< " with weight " << m_graph [ source ]->isLinkedTo(v2) << " and color "<<  color<<" -- Storing edge color...";
	m_graph[ source]->setOutLinkColor(v2, color);

	qDebug( "Graph: addEdge():: Now vertex %i has %i edges. Total Edges %i....", v1,  edgesFrom(v1), m_totalEdges);
	graphModified=true;
}




/**	
	Change edge (arc) weight between v1 and v2
*/
void Graph::setEdgeWeight (int v1, int v2, float weight) {
	qDebug("Graph: setEdgeWeight between %i (%i) and %i (%i), weight %f", v1, index[v1],v2,index[v2], weight);
	m_graph [ index[v1] ]->changeLinkWeightTo(v2, weight);
	qDebug("Graph: setEdgeWeight between %i (%i) and %i (%i), NOW weight %f", v1, index[v1],v2,index[v2], this->hasEdge(v1, v2) );
	qDebug("Graph: setEdgeWeight between %i (%i) and %i (%i), NOW vertex weight %f", v1, index[v1],v2,index[v2], m_graph [ index[v1] ]->isLinkedTo(v2) );
	graphModified=true;
	emit graphChanged(); 
}

	
/** 	Removes the edge (arc) between v1 and v2
*/
void Graph::removeEdge (int v1, int v2) {	
	qDebug ("Graph: edge (%i, %i) to be removed from graph", v1, v2);
	qDebug("Graph: Vertex named %i has index=%i",m_graph[ index[v1] ]->name(), index[v1]);
	m_graph [ index[v1] ]->removeLinkTo(v2);
	m_graph [ index[v2] ]->removeLinkFrom(v1);
	qDebug("Graph: removeEdge between %i (%i) and %i (%i), NOW vertex v1 reports edge weight %f", v1, index[v1],v2,index[v2], m_graph [ index[v1] ]->isLinkedTo(v2) );
	if ( hasEdge(v2,v1) !=0) symmetricAdjacencyMatrix=false;
	m_totalEdges--;
	outEdgesVert--;
	inEdgesVert--;
	graphModified=true;
	emit graphChanged(); 
}



/**	Checks if there is a specific vertex in the graph
	Returns the index or -1
	Complexity:  O(logN) for index retrieval 
*/
int Graph::hasVertex(int num){			
	qDebug ("Graph: hasVertex() v: %i with index %i named %i", num, index[num], m_graph[ index[num]] ->name());
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
	qDebug ("Graph: hasVertex( "+ label.toAscii()+ " ) ?" );
	QList<Vertex*>::iterator it;
	int i=0;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		if ( (*it) ->label() == label)  {
			qDebug("Graph: hasVertex() at pos %i", i);
			return i;
		}
		i++;
	}	
	qDebug("Graph: hasVertex() NO - returning -1");
	return -1;
}




void Graph::setInitVertexSize (int size) {
	initVertexSize=size;
}


//Changes the size.of vertex v 
void Graph::setVertexSize(int v, int size) { 
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




/**Changes the label.of vertex v  */
void Graph::setVertexLabel(int v1, QString label){
	qDebug()<< "Graph: setVertexLabel for "<< v1 << ", index " << index[v1]<< " with label"<< label;
	m_graph[ index[v1] ]->setLabel ( label);
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
void Graph::setVertexColor(int v1, QString color){
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
void Graph::setEdgeColor(int s, int t, QString color){
	qDebug()<< "Graph: setEdgeColor for edge ("<< s << ","<< t<<")"<<" with index ("<< index[s]<< ","<<index[t]<<")"<<" with color "<< color;
	m_graph[ index[s] ]->setOutLinkColor(t, color);
	if (isSymmetric()) {
		m_graph[ index[t] ]->setOutLinkColor(s, color);
	}
	emit graphChanged(); 
}	




/**	Checks if there is an edge from v1 to v2
	Complexity:  O(logN) for index retrieval + O(1) for QList index rerieval + O(logN) for checking edge(v2) 
*/
float Graph::hasEdge (int v1, int v2) {		
	float weight=0;
	if ( (weight=  m_graph[ index[v1] ]->isLinkedTo(v2) ) != 0 ) {
		qDebug("Graph: hasEdge() between %i (%i) and %i (%i) = %f", v1, index[v1], v2, index[v2], weight);
		return weight;
	}
	else {	
		qDebug("Graph: hasEdge() between %i (%i) and %i (%i) = NO", v1, index[v1], v2, index[v2]);
		return 0;
	}
}


/**
	Called from MainWindow
*/
void Graph::updateVertCoords(int v1, int  x, int y){
	qDebug("Graph: updateVertCoords() for %i with index %i with %i, %i", v1, index[v1], x,y);
	m_graph[ index[v1] ]->setX( x );
	m_graph[ index[v1] ]->setY( y );
	graphModified=true;
}



/**	Returns the number of edges (arcs) from vertex v1
*/
int Graph::edgesFrom (int v1) {  
	qDebug("Graph: edgesFrom()");
	return m_graph[ index[v1] ]->outLinks();
}


/**	
	Returns the number of edges (arcs) to vertex v1
*/
int Graph::edgesTo (int v1) {  
	qDebug("Graph: edgesTo()");
	QList<Vertex*>::iterator it;
	int m_edgesTo=0;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		if  ( (*it)->isLinkedTo(v1) != 0) m_edgesTo++;
	}
	return m_edgesTo;
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
	qDebug("Graph: m_totalEdges = %i, tEdges=%i", m_totalEdges, tEdges);
 	return tEdges;
}


/**	
	Returns |V| of graph
*/
int Graph::vertices () { 
	qDebug("Graph: vertices()");
	return m_totalVertices;
}


float Graph::density() {
	qDebug("Graph: density()");
	int vert=vertices();
	if (vert!=0 && vert!=1)
		return  (float) totalEdges() / (float)(vert*(vert-1.0));
	else return 0;
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
	qDebug("Graph: m_graph reports size %i", m_graph.size());
	m_graph.clear();
	index.clear();
	discreteIDCs.clear();
	discreteODCs.clear();
	m_totalVertices=0;
	m_totalEdges=0;
	outEdgesVert=0;
	inEdgesVert=0;
	reciprocalEdgesVert=0;
	
	order=true;		//returns true if the indexes of the list is ordered.

	calculatedIDC=FALSE;
	calculatedODC=FALSE;
	calculatedCentralities=FALSE;
	adjacencyMatrixCreated=FALSE;
	graphModified=FALSE;
	symmetricAdjacencyMatrix=TRUE;
	qDebug("Graph: m_graph cleared. Now reports size %i", m_graph.size());
}



/**Returns TRUE if the adjacency matrix is symmetric */
bool Graph::isSymmetric(){
	qDebug("=========================Graph: isSymmetric ");
	if (graphModified){
		symmetricAdjacencyMatrix=TRUE;
		imap_f::iterator it1;
		int y;
		QList<Vertex*>::iterator it;
		for (it=m_graph.begin(); it!=m_graph.end(); it++){
			//for all edges of u, (u,y)
			qDebug("Graph: isSymmetric(): GRAPH CHANGED! Iterate over all edges of u...");
			for( it1 = (*it)->m_edges.begin(); it1 != (*it)->m_edges.end(); it1++ ) {
				y=index[it1->first];	
				if ( ! m_graph[y]->isLinkedTo( (*it)->name() )) {
					qDebug("Graph: isSymmetric():  u = %i IS NOT inLinked from y = %i", (*it)->name(), it1->first  );
					symmetricAdjacencyMatrix=FALSE;
					return symmetricAdjacencyMatrix;
				}
				else 
					qDebug("Graph: isSymmetric():  u = %i IS inLinked from y = %i",it1->first, (*it)->name()  );
			}

		}
		graphModified=false;
	}
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
		for( it1 = (*it)->m_edges.begin(); it1 != (*it)->m_edges.end(); it1++ ) {
			y=index[it1->first];	
			if ( ! m_graph[y]->isLinkedTo( (*it)->name() )) {
				qDebug("Graph: symmetrize: u = %i IS NOT inLinked from y = %i", (*it)->name(), it1->first  );
				createEdge(it1->first, (*it)->name(), it1->second, initEdgeColor, false, true, false);
			}
			else 
				qDebug("Graph: symmetrize:  u = %i IS inLinked from y = %i",it1->first, (*it)->name()  );
		}
	}
	graphModified=TRUE;
	symmetricAdjacencyMatrix=TRUE;
	emit graphChanged(); 
}



bool Graph::symmetricEdge(int v1, int v2){
	qDebug("Graph: symmetricEdge");
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
	Returns the adjacency matrix of G
	This is called from MainWindow::slotExportSM() using << operator of Matrix class
	The resulting matrix HAS spaces between elements.
*/
void Graph::writeAdjacencyMatrixTo(QTextStream& os){
	qDebug("Graph: adjacencyMatrix(), writing matrix with %i vertices", vertices());
	QList<Vertex*>::iterator it, it1;	
	float weight=-1;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){	
			if ( (weight = hasEdge ( (*it)->name(), (*it1)->name() )  ) !=0 ) {
				
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
	qDebug("writeAdjacencyMatrix() ");
	ofstream file (fn);
	int sum=0;
	float weight=0;
	file << "-Social Network Visualiser- \n";
	file << "Adjacency matrix of "<< netName<<": \n\n";
	QList<Vertex*>::iterator it, it1;	
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){	
			if ( (weight =  this->hasEdge ( (*it)->name(), (*it1)->name() )  )!=0 ) {
				sum++;
				if (weight >= 1)
					file << static_cast<int> (weight);
				else 
					file << "1";
			}
			else
				file << "0";
		}
 		file << endl;
	}

	qDebug("Graph: Found a total of %i edge",sum);
	if ( sum != totalEdges() ) qDebug ("Error in edge count found!!!");
	else qDebug("Edge count OK!");
	file.close();
}





/**
*  Returns the distance between nodes numbered (i-1) and (j-1)
*/
int Graph::distance(int i, int j){
	if (graphModified){
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
*  Writes the matrix of distances to a file
*/
void Graph::writeDistanceMatrix (const char* fn, const char* fn1, const char* netName) {
	qDebug ("Graph::writeDistanceMatrix()");
	createDistanceMatrix(false);
	qDebug ("Graph::writeDistanceMatrix() writing to file");
	ofstream file (fn);
	ofstream file1 (fn1);
	int dist=-1, sigma=-1;
	char aspace[] = "       ";
	char bspace[] = "     ";
	char cspace[] = "   ";
	char dspace[] = "   ";

	file << "-Social Network Visualiser- \n";
	file << "Distance matrix of "<< netName<<": \n\n";
	//write out matrix of geodesic distances
	QList<Vertex*>::iterator it, it1;	
	int i=0, j=0;
	file << "         ";
	
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		file << i++<<aspace;

	}
	file<<endl;
	i=0;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		file << ++i ;
		if (i>999)
			file << " "; 
		else if (i>99)
			file << cspace; 
		else if(i>9) 
			file << bspace; 
		else 
 			file << aspace; 
 		j=0;
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){	
			++j;
			if ( (dist= DM.item( index[(*it)->name()],  index[(*it1)->name()] ) )!=-1 ) {
				file << dist;
				if (dist>999)
					file << " "; 
				else if (dist>99)
					file << cspace; 
				else if(dist>9) 
					file << bspace; 
				else 
					file << aspace; 
			}
			else
				file << "0"<<aspace;
			if (j>999)
				file << cspace; 
			else if (j>99)
				file << cspace; 
			else if(j>9) 
				file << dspace; 

		}
 		file << endl;
	}
	file.close();
	//write out matrix of sigmas
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){	
			if ( (sigma= TM.item( index[(*it)->name()],  index[(*it1)->name()] ) )!=-1 ) {
				file1 << sigma<<" ";
			}
			else
				file1 << "0 ";
		}
 		file1 << endl;
	}
	file1.close();

}






/**
	minmax() facilitates the calculations of minimum and maximum centralities during createDistanceMatrix()
*/
void Graph::minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) {
	qDebug("MINMAX C=%f, max=%f, min=%f, name= %i", C, max, min, v->name()); 
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
	It stores that number in a map<float,int> where the centrality value is the key.
	Called from createDistanceMatrix()
*/
void Graph::resolveClasses(float C, fmap_i &discreteClasses, int &classes){
Q_UNUSED(C);
Q_UNUSED(discreteClasses);
Q_UNUSED(classes);
// 	fmap_i::iterator it2;
// 	it2 = discreteClasses.find(C);    //O(logN) complexity
// 	if (it2 == discreteClasses.end() )	{
// 		classes++; 
// 		qDebug("######This is a new centrality class. Amount of classes = %i", classes);
// 		discreteClasses[C]=classes;
// 	}
}


void Graph::resolveClasses(float C, fmap_i &discreteClasses, int &classes, int vertex){
Q_UNUSED(C);
Q_UNUSED(discreteClasses);
Q_UNUSED(classes);
Q_UNUSED(vertex);
// 	fmap_i::iterator it2;
// 	it2 = discreteClasses.find(C);    //O(logN) complexity
// 	if (it2 == discreteClasses.end() )	{
// 		classes++; 
// 		qDebug("######Vertex %i  belongs to a new centrality class. Amount of classes = %i", vertex, classes);
// 		discreteClasses[C]=classes;
// 	}
}



/**
	Creates a matrix DM which stores geodesic distances between all vertices
	INPUT: 
		boolean calc_centralities
	OUTPUT:
		DM(i,j)=geodesic distance between vertex i and vertex j
		TM(i,j)=number of shortest paths from vertex i to vertex j, called sigma(i,j).
		graphDiameter is set to the length of the longest shortest path between every (i,j)
		Also, if calc_centralities==TRUE, it calculates the centralities for every u in V:
		- Betweeness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
		- Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
		- Graph: CC(u) =  1/maxDistance(u,t)  for some t in V
		- Closeness: CC(u) =  1 / Sum( DM(u,t) )  for every  t in V
*/

void Graph::createDistanceMatrix(bool calc_centralities) {
	qDebug ("Graph::createDistanceMatrix()");
	if ( !graphModified && distanceMatrixCreated && !calc_centralities)  { 
		qDebug("Graph: distanceMatrix not mofified. Escaping.");
		return;
	}
	//Create a NxN DistanceMatrix. Initialise values to zero.
	qDebug ("Graph::resizing Matrices %i", m_totalVertices);
	DM.resize(m_totalVertices);
	TM.resize(m_totalVertices);

	graphDiameter=0;

	int vert=vertices();


	if (totalEdges() == 0 ) //can user m_totalEdges here to save some time...
		DM.fillMatrix(0);	
	else{
		//for all vertices set their distances to infinum, aka -1
		qDebug("for all vertices set their distances to -1 (infinum)");
		DM.fillMatrix(-1);
		//for all vertices set their sigmas as 0
		TM.fillMatrix(0);

		QList<Vertex*>::iterator it, it1;	
		QList<int>::iterator it2;
		int w=0, u=0,s=0;
		float d_sw=0, d_su=0;	
		reciprocalEdgesVert=0;
		outEdgesVert=0;
		inEdgesVert=0;
		maxIndexBC=0;
		maxIndexSC=0;
		maxIndexEC=0;
		//The following are for CC
		fmap_i::iterator it3; 

		float CC=0, BC=0, SC=0, GC=0, EC=0, stdGC=0, stdEC=0;
		qDebug("Graph: createDistanceMatrix() - initialising variables for maximum centrality indeces");
		if (symmetricAdjacencyMatrix) {
			maxIndexBC=( vert-1.0) *  (vert-2.0)  / 2.0;
			maxIndexSC=( vert-1.0) *  (vert-2.0) / 2.0;
			maxIndexCC=1.0/(vert-1.0);
			maxIndexEC=vert-1.0;
			qDebug("############# maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
		}
		else {	
			maxIndexBC= ( ( outEdgesVert-1.0) *  (inEdgesVert-2.0) - (reciprocalEdgesVert-1.0))/ 2.0;
			maxIndexSC=1;
			maxIndexEC=(vert-1.0);
			maxIndexCC=1.0/(vert-1.0);  //FIXME This applies only on undirected graphs
			qDebug("############# maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
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
		maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0; minNodeEC=0; sumEC=0;
		discreteECs.clear(); classesEC=0;
		//Zero closeness indeces of each vertex
		if (calc_centralities) 
			for (it=m_graph.begin(); it!=m_graph.end(); it++) {
				(*it)->setBC( 0.0 );
				(*it)->setSC( 0.0 );
				(*it)->setGC( 0.0 );
				(*it)->setCC( 0.0 );
		}
		qDebug("MAIN LOOP: for every s in V do (solve the single source shortest path problem...");
		for (it=m_graph.begin(); it!=m_graph.end(); it++){
			s=index[(*it)->name()];
			qDebug("Source vertex s=%i of BFS algorithm has index %i. Clearing Stack ...", (*it)->name(), s);
			if (calc_centralities){
				qDebug("Empty stack Stack which will return vertices in order of their (non increasing) distance from S ...");
				//- Complexity linear O(n) 
				while ( !Stack.empty() )  
					Stack.pop();
				qDebug("...and for each vertex: empty list Ps of predecessors");
				//Complexity linear O(n)
 				for (it1=m_graph.begin(); it1!=m_graph.end(); it1++) 
 					(*it1)->clearPs();
			}

			qDebug("PHASE 1 (SSSP): Call BFS for source vertex %i to determine distances and shortest path counts from s to every vertex t", (*it)->name());
			BFS(s,calc_centralities );
			qDebug("Finished BFS. Continuing to calculate centralities");
			if (calc_centralities){
				qDebug("Set centrality for current source vertex %i  with index s=%i", (*it)->name(), s);
				if ( (*it)->CC() != 0 ) //Closeness centrality must be inverted 	
					CC=1.0/(*it)->CC();
				else CC=0;
				(*it)->setSCC ( CC * ( vert-1.0)  );
				(*it)->setCC( CC );
				//Resolve classes Closeness centrality
				qDebug("=========Resolving CC classes...");
				resolveClasses(CC, discreteCCs, classesCC,(*it)->name() );
				sumCC+=CC;
				minmax( CC, (*it), maxCC, minCC, maxNodeCC, minNodeCC) ;
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
				stdGC =(vert-1.0)*GC ;
				(*it)->setSGC(stdGC);
				sumGC+=GC;
				minmax( GC, (*it), maxGC, minGC, maxNodeGC, minNodeGC) ;

				stdEC =EC/(vert-1.0);
				(*it)->setSEC(stdEC);
				sumEC+=EC;
				minmax( EC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;
				
				
				qDebug("PHASE 2 (ACCUMULATION): Start back propagation of dependencies. Set dependency delta[u]=0 on each vertex");
				for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
					(*it1)->setDelta(0.0);
					qDebug("vertex %i with index %i has delta = %F", (*it1)->name(),index[(*it1)->name()], (*it1)->delta());
				}

				qDebug("Visit all vertices in reverse order of their discovery (from s = %i) to sum dependencies. Initial Stack size has %i", s, Stack.size());

				while ( !Stack.empty() ) {
					w=Stack.top(); 
					qDebug("Stack top is vertex w=%i. This is the furthest vertex from s. Popping it.", w);
					Stack.pop();
					qDebug("preLOOP: Checking size of predecessors list Ps[w]...  = %i ",m_graph[w]->Ps().size());
					qDebug("LOOP: for every other vertex u in the list of predecessors Ps[w] of w....");
					if (m_graph[w]->Ps().size() > 0) // just in case...do a sanity check
					for ( it2=m_graph[w]->Ps().begin(); it2 != m_graph[w]->Ps().end(); it2++ ){
						u=(*it2);
						qDebug("Selecting Ps[w] element u=%i with delta_u=%f. TM(s,u)=%i, TM(s,w)=%i, delta_w=%f ", u, m_graph[u]->delta(),TM.item(s,u), TM.item(s,w), m_graph[w]->delta());
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
					if  (w!=s) { 
						qDebug("w!=s. For this furthest vertex we need to add its new delta %f to old BC index: %f",m_graph[w]->delta(), m_graph[w]->BC());
						d_sw = m_graph[w]->BC() + m_graph[w]->delta();
						qDebug("New BC = d_sw = %f", d_sw);
						m_graph[w]->setBC (d_sw);
					}
				}
			}
		}
		
		if (calc_centralities) {
			for (it=m_graph.begin(); it!=m_graph.end(); it++) {

				if (symmetricAdjacencyMatrix) {
					qDebug("Betweeness centrality must be divided by two if the graph is undirected");
					(*it)->setBC ( (*it)->BC()/2.0);
				}

				BC=(*it)->BC();
				//Resolve classes Betweeness centrality
				qDebug("Resolving BC classes...");
				resolveClasses(BC, discreteBCs, classesBC);
				//Store standard Betweeness 
				qDebug("******************* BC %f maxIndex: %f", BC, maxIndexBC);
				(*it)->setSBC( BC/maxIndexBC );   
				//Find min & max BC - not using stdBC:  Wasserman & Faust, pp. 191-192
				sumBC+=BC;
				minmax( BC, (*it), maxBC, minBC, maxNodeBC, minNodeBC) ;
				//Find denominal of groupBC
				nomBC +=(maxBC - BC );

				//Resolve classes Stress centrality
				SC=(*it)->SC();
				qDebug("Resolving SC classes...");
				resolveClasses(SC, discreteSCs, classesSC);
				//Store standard Stress centrality
				(*it)->setSSC ( SC/maxIndexSC );
				//Find min & max SC - not using stdSC:  Wasserman & Faust, pp. 191-192
				sumSC+=SC;
				minmax( SC, (*it), maxSC, minSC, maxNodeSC, minNodeSC) ;
				//Find denominal of groupSC
				nomSC +=(maxSC - SC );
				
				//Find denominal of groupGC
				nomGC += maxGC-(*it)->SGC();
				//Find denominal of groupCC
				nomCC += maxCC- (*it)->SCC();
			}
			maxCC = (vert-1.0)*maxCC;	//standardize minimum and maximum Closeness centrality
			minCC = (vert-1.0)*minCC; 
			denomCC =  (( vert-2.0) *  (vert-1.0))/ (2.0*vert-3.0);
			groupCC = nomCC/denomCC;	//Calculate group Closeness centrality
	
			nomBC*=2.0;
			denomBC =   (vert-1.0) *  (vert-1.0) * (vert-2.0);
			groupBC=nomBC/denomBC;		//Calculate group Betweeness centrality
	
			denomGC =  ( ( vert-2.0) *  (vert-1.0) )/ (2.0*vert-3.0);
			groupGC= nomGC/denomGC;		//Calculate group Graph centrality
	
			nomSC*=2.0;
			denomSC =   (vert-1.0) *  (vert-1.0) * (vert-2.0);
			groupSC = nomSC/denomSC;	//Calculate group Stress centrality
			calculatedCentralities=TRUE;
		}
	}
	distanceMatrixCreated=TRUE;
	graphModified=FALSE;
	
}




/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

	INPUT: 
		a 'source' vertex with index s and a boolean calc_centralities.
		(Implicitly, BFS uses the m_graph structure)
	
	OUTPUT: 
		For every vertex t: DM(s, t) is set to the distance of each t from s
		For every vertex t: TM(s, t) is set to the number of shortest paths between s and t
		For every vertex u: it increases SC(u) by one, when it finds a new shor. path from s to t through u.
		For source vertex s: it calculates CC(s) as the sum of its distances from every other vertex. 
		For every source s: it calculates GC(u) as the maximum distance from all other vertices.

		Also, if calc_centralities is TRUE then BFS does extra operations:
			a) each vertex u popped from Q is pushed to a stack Stack 
			b) Append each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s 
	
*/ 
void Graph::BFS(int s, bool calc_centralities){
	int u,w, dist_u, temp=0;

	//set distance of s from s equal to 0	
	DM.setItem(s,s,0);
	//set sigma of s from s equal to 1
	TM.setItem(s,s,1);

	//
	qDebug("BFS: Construct a queue Q of integers and push source vertex s=%i to Q as initial vertex", s);
	queue<int> Q;
//	qDebug("BFS: Q size %i", Q.size());

	Q.push(s);

	qDebug("BFS: LOOP: While Q not empty ");
	while ( !Q.empty() ) {
		qDebug("BFS: Dequeue: first element of Q is u=%i", Q.front());
		u=Q.front(); Q.pop();
		if (calc_centralities){
			qDebug("BFS: If we are to calculate centralities, we must push u=%i to global stack Stack ", u);
			Stack.push(u);
		}
		imap_f::iterator it;
		qDebug("BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u");
		for( it = m_graph [ u ]->m_edges.begin(); it != m_graph [ u ]->m_edges.end(); it++ ) {
			
			w=index[it->first];	
			qDebug("BFS: u=%i is connected with w=%i of index %i. ", u, it->first, w);
			qDebug("BFS: Start path discovery");
			if (	DM.item(s, w) == -1 ) { //if distance (s,w) is infinite, w found for the first time.
				//append w to Q
				qDebug("BFS: first time visiting w=%i. Pushing w to the end of Q", w);
				Q.push(w);
				//First check if distance(s,u) = -1 (aka infinite :)) and set it to zero
				dist_u=DM.item(s,u);
// 				if (dist_u <0) dist_u=0;
				qDebug("BFS: Setting distance of w=%i from s=%i equal to distance(s,u) plus 1. New distance = %i",w,s, dist_u+1);
				DM.setItem(s, w, dist_u+1);
				
				if (calc_centralities){
					//Calculate CC: the sum of distances (will invert it l8r)
					m_graph [s]->setCC (m_graph [s]->CC() + dist_u+1);
					//Calculate GC: the maximum distance (will invert it l8r) - also for Eccentricity
					if (m_graph [s]->GC() < dist_u+1 ) m_graph [s]->setGC(dist_u+1);

				}
				qDebug("BFS: Checking graphDiameter");
				if ( dist_u+1 > graphDiameter){
					graphDiameter=dist_u+1;
					qDebug("BFS: new graphDiameter = %i",graphDiameter );
				}
			}		

			qDebug("BFS: Start path counting"); 	//Is edge (u,w) on a shortest path from s to w via u?
			if ( DM.item(s,w)==DM.item(s,u)+1) {
				temp= TM.item(s,w)+TM.item(s,u);
				qDebug("BFS: Found a NEW SHORTEST PATH from s=%i to w=%i via u=%i. Setting Sigma(%i, %i) = %i",s, w, u, s, w,temp);
				if (s!=w)
					TM.setItem(s,w, temp);
				if (calc_centralities){
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
*	Calculates In-Degree Centralities of each vertex - diagonal included
*	Also the mean value and the variance of the in-degrees.
*/
void Graph::centralityInDegree(bool weights){
	qDebug("Graph:: centralityInDegree()");
	float IDC=0, nom=0, denom=0;
	float weight;
	classesIDC=0;
	sumIDC=0; 
	maxIDC=0;
	minIDC=vertices()-1;
	discreteIDCs.clear();
	varianceDegree=0;
	meanDegree=0;
	QList<Vertex*>::iterator it, it1;
	map<float, int>::iterator it2;
	int vert=vertices();
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		IDC=0;
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
			if ( (weight=this->hasEdge ( (*it1)->name(), (*it)->name() ) ) !=0  )   {	
				if (weights)
					IDC+=weight;
				else
					IDC++;
			}		
		}
		(*it) -> setIDC ( IDC ) ;				//Set InDegree
		(*it) -> setSIDC( IDC / (vert-1.0) );		//Set Standard InDegree
		qDebug("Graph: vertex %i has IDC = %f and SIDC %f", (*it)->name(), IDC, (*it)->SIDC ());
		sumIDC += IDC;
		it2 = discreteIDCs.find(IDC);
		if (it2 == discreteIDCs.end() )	{
			classesIDC++; 
			qDebug("This is a new IDC class");
			discreteIDCs[IDC]=classesIDC;
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
	
	meanDegree = sumIDC / (float) vert;  /** BUG? WEIGHTS???? */
	qDebug("Graph: sumIDC = %f, meanDegree = %f", sumIDC, meanDegree);
	// Calculate Variance and the Degree Centralisation of the whole graph.
	for (it=m_graph.begin(); it!=m_graph.end(); it++){ 
		IDC= (*it)->IDC();
		//qDebug("Graph: IDC = %f, meanDegree = %f", IDC, meanDegree);
		varianceDegree+=pow ( (IDC-meanDegree), 2 );	//BUG OCCURED IN SLACKWARE...
		nom+= maxIDC-IDC;
	}
	if (symmetricAdjacencyMatrix)
		denom=(vert-1.0)*(vert-2.0);
	else
		denom=(vert-1.0)*(vert-1.0);
	varianceDegree=varianceDegree/(float) vert;
	groupIDC=nom/denom;
	qDebug("Graph: varianceDegree = %f, groupIDC = %f", varianceDegree, groupIDC);

	minIDC/=(float)(vert-1); // standardize
	maxIDC/=(float)(vert-1);
	calculatedIDC=TRUE;
	graphModified=false;
}


void Graph::writeCentralityInDegree(){

	ofstream file ("centrality-in-degree.dat");
	centralityInDegree(true);
	float maximumIndexValue=vertices()-1.0;
	
	file <<"-SocNetV- \n\n";
	file<< "IN-DEGREE CENTRALITIES (IDC) OF EACH NODE\n";
	file<< "IDC  range: 0 < C < "<<maximumIndexValue<<"\n";
	file<< "IDC' range: 0 < C'< 1"<<"\n\n";

	file << "Node"<<"\tIDC\tIDC'\t%IDC\n";
	QList<Vertex*>::iterator it;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){ 
		file<<(*it)->name()<<"\t"<<(*it)->IDC() << "\t"<< (*it)->SIDC() << "\t" <<  (100* ((*it)->IDC()) / sumIDC)<<endl;
	}
	if (symmetricAdjacencyMatrix) {
		file << "Mean Nodal Degree = "<< meanDegree<<"\n" ;
		file << "Degree Variance = "<< varianceDegree<<"\n\n";
	}
	else{
		file << "Mean Nodal InDegree = "<< meanDegree<<"\n" ;
		file << "InDegree Variance = "<< varianceDegree<<"\n\n";
	}
	if ( minIDC == maxIDC )
		file << "\nAll nodes have the same IDC value.\n";
	else  {
		file << "\nNode "<< maxNodeIDC << " has the maximum IDC value (std): " << maxIDC <<"  \n";
		file << "\nNode "<< minNodeIDC << " has the minimum IDC value (std): " << minIDC <<"  \n";
	}
	if (classesIDC!=1)
		file<< "\nThere are "<<classesIDC<<" different IDC classes.\n";	
	else 
		file<< "\nThere is only "<<classesIDC<<" IDC class.\n";	
	file<<"\nGROUP IN-DEGREE CENTRALISATION (GIDC)\n\n";
	file<<"GIDC = " << groupIDC<<"\n\n";
	file<<"GIDC range: 0 < GIDC < 1\n";
	file<<"GIDC = 0, when all in-degrees are equal (i.e. regular lattice).\n";
	file<<"GIDC = 1, when one node is linked from every other node.\n";
	file<<"The in-degree of the node is a measure of the \'activity\' of the node it represents\n";
	file<<"(Wasserman & Faust, p. 101)\n";
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
	fmap_i::iterator it2;
	
	for (it=m_graph.begin(); it!=m_graph.end(); it++){
		ODC=0;
		for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){	
			if ( (weight=this->hasEdge ( (*it)->name(), (*it1)->name() ) ) !=0  )   {	
				qDebug("Graph: vertex %i isLinkedTo= %i", (*it)->name(), (*it1)->name());
				if (weights)
					ODC+=weight;
				else 
					ODC++;
			}		
		}
		(*it) -> setODC ( ODC ) ;				//Set OutDegree
		(*it) -> setSODC( ODC / (vert-1.0) );		//Set Standard OutDegree
		qDebug("Graph: vertex %i has ODC = %f and SODC %f", (*it)->name(), ODC, (*it)->SODC ());
		sumODC += ODC;
		it2 = discreteODCs.find(ODC);
		if (it2 == discreteODCs.end() )	{
			classesODC++; 
			qDebug("This is a new ODC class");
			discreteODCs[ODC]=classesODC;
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
	
	meanDegree = sumODC / (float) vert;  /** BUG? WEIGHTS???? */
	qDebug("Graph: sumODC = %f, meanDegree = %f", sumODC, meanDegree);
	// Calculate Variance and the Degree Centralisation of the whole graph.
	for (it=m_graph.begin(); it!=m_graph.end(); it++){ 
		ODC= (*it)->ODC();
		//qDebug("Graph: ODC = %f, meanDegree = %f", ODC, meanDegree);
		varianceDegree+=pow ( (ODC-meanDegree), 2 );	//BUG OCCURED IN SLACKWARE...
		nom+= maxODC-ODC;
	}
	if (symmetricAdjacencyMatrix)
		denom=(vert-1.0)*(vert-2.0);
	else
		denom=(vert-1.0)*(vert-1.0);
	varianceDegree=varianceDegree/(float) vert;
	groupODC=nom/denom;
	qDebug("Graph: varianceDegree = %f, groupODC = %f", varianceDegree, groupODC);

	minODC/=(float)(vert-1); // standardize
	maxODC/=(float)(vert-1);
	calculatedODC=TRUE;
	graphModified=false;
}

void Graph::writeCentralityOutDegree(){

	ofstream file ("centrality-out-degree.dat");
	centralityOutDegree(true);
	float maximumIndexValue=vertices()-1.0;
	
	file <<"-SocNetV- \n\n";
	file<< "OUT-DEGREE CENTRALITIES (ODC) OF EACH NODE\n";
	file<< "ODC  range: 0 < C < "<<maximumIndexValue<<"\n";
	file<< "ODC' range: 0 < C'< 1"<<"\n\n";

	file << "Node"<<"\tODC\tODC'\t%ODC\n";
	QList<Vertex*>::iterator it;
	for (it=m_graph.begin(); it!=m_graph.end(); it++){ 
		file<<(*it)->name()<<"\t"<<(*it)->ODC() << "\t"<< (*it)->SODC() << "\t" <<  (100* ((*it)->ODC()) / sumODC)<<endl;
	}
	if (symmetricAdjacencyMatrix) {
		file << "Mean Node Degree = "<< meanDegree<<"\n" ;
		file << "Degree Variance = "<< varianceDegree<<"\n\n";
	}
	else{
		file << "Mean Vertex OutDegree = "<< meanDegree<<"\n" ;
		file << "OutDegree Variance = "<< varianceDegree<<"\n\n";
	}
	if ( minODC == maxODC )
		file << "\nAll nodes have the same ODC value.\n";
	else  {
		file << "\nNode "<< maxNodeODC << " has the maximum ODC value (std): " << maxODC <<"  \n";
		file << "\nNode "<< minNodeODC << " has the minimum ODC value (std): " << minODC <<"  \n";
	}
	if (classesODC!=1)
		file<< "\nThere are "<<classesODC<<" different out-degree centrality classes.\n";		
	else 
		file<< "\nThere is only "<<classesODC<<" out-degree centrality class.\n";	
	
	file<<"\nGROUP OUT-DEGREE CENTRALISATION (GODC)\n\n";
	file<<"GODC = " << groupODC<<"\n\n";
	file<<"GODC range: 0 < GODC < 1\n";
	file<<"GODC = 0, when all out-degrees are equal (i.e. regular lattice).\n";
	file<<"GODC = 1, when one node completely dominates or overshadows the other nodes.\n";
	file<<"(Wasserman & Faust, formula 5.5, p. 177)\n\n";
	file<<"The degree of the node is a measure of the \'activity\' of the node it represents\n";
	file<<"(Wasserman & Faust, p. 101)\n";

	file.close();

}





/** 
* Repositions all nodes on the periphery of different circles with radius analogous to their centrality
*/
void Graph::layoutCircleCentrality(double x0, double y0, double maxRadius, int CentralityType){
	qDebug("Graph: layoutCircleCentrality...");
	//first calculate centralities
	if ((graphModified || !calculatedCentralities) && CentralityType > 2) {
		qDebug("Graph: Calling createDistanceMatrix() to calc centralities");
		createDistanceMatrix(true);
	}
	else if ((graphModified || !calculatedIDC) && CentralityType == 1)
		centralityInDegree(true);
	else if ((graphModified || !calculatedODC) && CentralityType == 2)
		centralityOutDegree(true);

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
		};
		qDebug ("Vertice %i at x=%f, y=%f: C=%f, stdC=%f, maxradius %f",(*it)->name(), (*it)->x(), (*it)->y(), C, std, maxRadius);
		
		qDebug ("C %f, maxC %f, C/maxC %f, *maxRadius %f",C , maxC, (C/maxC), (C/maxC - 0.06)*maxRadius);
		switch ((int) ceil(maxC)){
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
		emit addBackgrCircle((int)x0, (int)y0, (int)new_radius);
	}
	graphModified=false;
}




/** 
* Repositions all nodes on different top-down levels according to their centrality
*/
void Graph::layoutLevelCentrality(double maxWidth, double maxHeight, int CentralityType){
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
		qDebug ("Vertice %i at x=%f, y=%f: C=%f, stdC=%f, maxC %f, maxWidth %f, maxHeight %f",(*it)->name(), (*it)->x(), (*it)->y(), C, std, maxC, maxWidth, maxHeight);
		//Calculate new position
		qDebug ("C/maxC %f, *maxHeight %f, +maxHeight %f ",C/maxC, (C/maxC)*maxHeight, maxHeight-(C/maxC)*maxHeight );
		switch ((int) ceil(maxC)){
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
		new_x=offset/2.0+rand()%(int)maxWidth;
		qDebug ("new_x %f, new_y %f", new_x, new_y);
		(*it)->setX( new_x );
		(*it)->setY( new_y );
		qDebug("Finished Calculation. Vertice will move to x=%f and y=%f ",new_x, new_y);
		//Move node to new position
		emit moveNode((*it)->name(),  new_x,  new_y);
		i++;
		emit addBackgrHLine((int)new_y);
	}
	graphModified=false;
}



/** layman's attempt to create a random network
*/
void Graph::createRandomNetErdos(int vert, double probability){
	qDebug("Graph: createRandomNetErdos");
	bool showLabels = false;
	int progressCounter=0;
	showLabels = ( (MainWindow*)parent() )->showLabels();

	for (register int i=0; i< vert ; i++) {
		int x=10+rand() %640;
		int y=10+rand() %480;
		qDebug("Graph: createRandomNetErdos, new node i=%i, at x=%i, y=%i", i+1, x,y);
		createVertex(i+1,initVertexSize,initVertexColor, QString::number (i+1), initVertexLabelColor, QPoint(x, y), initVertexShape);
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
}




/** layman's attempt to create a random Lattice network.
*/

void Graph::createRandomNetPhysLattice(int vert, int degree,double x0, double y0, double radius){
	qDebug("Graph: createPhysicistLatticeNetwork");
	int x=0;
	int y=0;
	int progressCounter=0;
	bool showLabels = false;
	showLabels = ( (MainWindow*)parent() )->showLabels();
	double Pi = 3.14159265;
	double rad= (2.0* Pi/ vert );
	for (register int i=0; i< vert ; i++) {
		x=x0 + radius * cos(i * rad);
		y=y0 + radius * sin(i * rad);
		createVertex(i+1,initVertexSize,initVertexColor, QString::number (i+1), initVertexLabelColor, QPoint(x, y), initVertexShape);
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
}


/** layman's attempt to create a random network where nodes have the same degree.
*/

void Graph::createSameDegreeRandomNetwork(int vert, int degree){
	qDebug("Graph: createSameDegreeRandomNetwork");
	bool showLabels = false;
	int progressCounter=0;
	showLabels = ( (MainWindow*)parent() )->showLabels();
	for (register int i=0; i< vert ; i++) {
		int x=10+rand() %640;
		int y=10+rand() %480;
		qDebug("Graph: createUniformRandomNetwork, new node i=%i, at x=%i, y=%i", i+1, x,y);
		createVertex(i+1,initVertexSize,initVertexColor, QString::number (i+1), initVertexLabelColor, QPoint(x, y), initVertexShape);
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

}



/** 
	Calculates x! factorial...
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
int Graph::loadFile(QString fileName, int iNS, QString iNC, QString iNL, QString iNSh, bool iSL, int maxWidth, int maxHeight){
	qDebug("Calling thread");
	parser.load(fileName, iNS, iNC, iNL, iNSh, iSL, maxWidth, maxHeight);
	qDebug("See the thread?");

	return 1;

}




void Graph::setShowLabels(bool toggle){
	initShowLabels=toggle;

}





/** 
	This slot is activated when the user clicks on the relevant MainWindow checkbox (SpringEmbedder, Fruchterman) to start or stop the movement of nodes, according to the requested model. 
	state: toggle 
	type:  controls the type of layout model requested.
	cW, cH: control the current canvasWidth and canvasHeight
*/
void Graph::nodeMovement(int state, int type, int cW, int cH){
	qDebug("Graph: startNodeMovement()");
	canvasWidth = cW;
	canvasHeight = cH;
	int factor=100;		//factor controls speed. Decrease it to increase speed...
	if (state == Qt::Checked){
		dynamicMovement = TRUE;
		layoutType=type;
		if (!timerId) {
			qDebug("Graph: startTimer()");
			timerId = startTimer(factor);		
		}
	}
	else
		dynamicMovement = FALSE;
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

The Spring Embedder model (Eades, 1984) assigns forces to all vertices and edges, as if nodes were electrically charged particles (Coulomb's law) and all edges were springs (i.e. Hooke's law).

These forces are applied to the nodes iteratively, pulling them closer together or pushing them further apart,  until the system comes to an equilibrium state (node positions do not change anymore).

Note that, following Eades, we dont need to have a faithful simulation; we can apply unrealistic forces in an unrealistic manner.
*/

void Graph::layoutForceDirectedSpringEmbedder(bool dynamicMovement){
	qreal xvel = 0, yvel = 0, dx=0, dy=0;
	double dist =0;
	qreal l=440, c1=canvasHeight/10.0;
	qreal dux=0, duy=0;
	QPointF curPos, newPos, pos ;
	int targetVertex=0;
	qreal weight_coefficient=1;		//affects speed and line length. Try 10...
	imap_f::iterator it1; //delete me.
	if (dynamicMovement){
		foreach (Vertex *v1, m_graph)  {
			// Sum up all repelling forces (i.e. imagine nodes are electrons)
			xvel=0; yvel=0;
			qDebug("<----------->  Calculate total repelling Force for vertex %i with index %i and pos %f, %f ", v1->name(), index[v1->name()], v1->x(), v1->y());
			foreach (Vertex *v2, m_graph)  {
				if ( ! hasEdge(v1->name(), v2->name()) )   //Spring embedder counts repelling forces only between non-adjacent vertices. Compare with F-R.
				{
					if ( v2 == v1 ) continue;
					QLineF line(v1->x(), v1->y(),  v2->x(), v2->y() );	//imaginary line v1 --> v2
					dx = line.dx();
					dy = line.dy();
					dist = (dx * dx + dy * dy);
					qDebug("c1, dx, dy, dist: %f, %f, %f, %f", c1, dx, dy, dist);
					
					if (dist > 0) { //only if dist is positive.
						dux = (dx * c1) / dist; 	
						duy = (dy * c1) / dist;
						qDebug("%i is pushed away from %i of index %i  and  pos (%f, %f) with.... ", v1->name(), v2->name(), index[v2->name()], v2->pos().x(), v2->pos().y());
						if ( dx < 0 ) {
							xvel +=  -dux ;	
							qDebug("add to xvel  += %f", dux);
						}
						else {
							xvel -=  dux ;	
							qDebug("sub from xvel -= %f", -dux);
						}
						if ( dy < 0 ) {
							yvel +=  -duy;
							qDebug("add to  yvel += %f", duy);
						}
						else { 
							yvel -=  duy;
							qDebug("sub from yvel -= %f", -duy);
						}

					}
					qDebug(" ========== After push, Total Velocity for %i xvel, yvel  %f, %f", v1->name(), xvel, yvel);

				}
				else {
				}
			}
			// Now subtract all pulling forces (i.e. springs)
			qDebug(">-------------<  Calculate pulling force for %i", v1->name());
			double weight = (v1->m_edges.size() + 1) * weight_coefficient;
			qDebug("weight %f", weight);
			for( it1 = (*v1).m_edges.begin(); it1 != (*v1).m_edges.end(); it1++ ) {
//				foreach (int targetVertex, (*v1).m_edges	 ) {
					//get other node's coordinates
				targetVertex=index[it1->first];	
				pos = m_graph[targetVertex]->pos();
				QLineF line( v1->x(), v1->y(),  m_graph[targetVertex]->x(), m_graph[targetVertex]->y());
				dx = line.dx();
				dy = line.dy();
				dist = sqrt(dx * dx + dy * dy);

				dux = log ( abs(dx)*  dist/l ); 
				duy = log ( abs(dy)*  dist/l );
				qDebug("%i with index %i is linked with %i of index %i  and  pos (%f, %f) ", v1->name(), index[v1->name()], it1->first, targetVertex,pos.x(), pos.y());
				qDebug("dx, dy, dist: %f, %f, %f, log x is %f and log y is %f", dx, dy, dist, dux, duy) ;
				if ( dx > 0 ) {
					xvel +=  dux ;	
					qDebug("add to xvel  += %f", dux);
				}
				else {
					xvel -=  dux ;	
					qDebug("sub from xvel -= %f", -dux);
				}
				if ( dy > 0 ) {
					yvel +=  duy;
						qDebug("add to  yvel += %f", duy);
				}
				else { 
					yvel -=  duy;
					qDebug("sub from yvel -= %f", -duy);
				}

			}
			qDebug(" ========== After PULL, Total Velocity for %i xvel, yvel  %f, %f", v1->name(), xvel, yvel);
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

In FR model, the vertices behave as atomic particles or celestial bodies, exerting attractive and repulsive forces on one another. Again, only vertices that are neighbours attract each other but, unlike Spring Embedder, all vertices repel each other. 

These forces induce movement. The algorithm might resemble molecular or planetary simulations, some-
times called n -body problems. 

*/
void Graph::layoutForceDirectedFruchtermanReingold(bool dynamicMovement){
	qreal xvel = 0, yvel = 0, dx=0, dy=0;
	double dist =0;
	qreal c=0.1, area= (canvasHeight* canvasWidth);
	qreal k = c* sqrt (area/m_totalVertices);
	QPointF curPos, newPos, pos ;
	int targetVertex=0;
	qreal weight_coefficient=10;		//affects speed and line length. Try 10...
	imap_f::iterator it1; 
	if (dynamicMovement){
		qDebug("layoutForceDirectedFruchtermanReingold");
		foreach (Vertex *v1, m_graph)  {
			// Sum up all repelling forces (i.e. imagine nodes are electrons)
			xvel=0; yvel=0;
			curPos = QPointF ( v1->x(), v1->y());  //convert to real....
			qDebug("<----------->  Calculate total repelling force for vertex %i with index %i and pos %f, %f ", v1->name(), index[v1->name()], curPos.x(), curPos.y());
			foreach (Vertex *v2, m_graph)  {
				if ( v2 == v1 ) continue; // F-R counts repelling forces between all vertices. Compare with S-E.
				QLineF line(  v2->x(), v2->y(), curPos.x(), curPos.y()); //imaginary line v2 --> v1
				dx = line.dx();
				dy = line.dy();
				dist = sqrt(dx * dx + dy * dy);
				if (dist > 0) { //only if dist is positive.
					xvel += (dx / dist )  *  (k * k) / dist;
					yvel += (dy / dist ) * (k * k) / dist;
				}
				qDebug("v1 %i is pushed away of %i.  area %f, k %f, dx %f, dy %f, dist %f, (addx, addy) = (%f, %f)", v1->name(), v2->name(), area, k, dx, dy, dist,  (dx / dist )  *  (k * k) / dist, (dy / dist )  *  (k * k) / dist);
				qDebug("xvel, yvel = %f, %f ", xvel, yvel);
			}
			// Now calculate and subtract all attractive forces (i.e. imagine nodes springs)
			qDebug(">-------------<  Calculate total attractive force for %i", v1->name());
			double weight = (v1->m_edges.size() + 1) * weight_coefficient;
			qDebug("weight %f", weight);
			for ( it1 = (*v1).m_edges.begin(); it1 != (*v1).m_edges.end(); it1++ ) {
				targetVertex=index[it1->first];	
				pos = m_graph[targetVertex]->pos();
				QLineF line( curPos.x(), curPos.y(),  m_graph[targetVertex]->x(),  m_graph[targetVertex]->y() );
				dx = line.dx();
				dy = line.dy();
				dist = (dx * dx + dy * dy);

// 				xvel += dx / weight;
// 				yvel += dy / weight;
				xvel += (dx / dist )  * dist / k ;
				yvel += (dy / dist )  * dist / k ;
 				qDebug("%i (%i) linked with %i (%i) of pos (%f, %f), dx %f, dy %f, dist %f, ADD TO VEL %f ", v1->name(), index[v1->name()], it1->first, targetVertex, pos.x(), pos.y(), dx, dy, dist, dx / weight);
 				qDebug("VELOCITY %f, %f",  xvel, yvel);
			}

			//Move node to new position
			newPos = QPointF(curPos.x()+ xvel, curPos.y()+yvel);
			qDebug("current x and y: %f, %f. Possible new pos is to new x new y = %f, %f", curPos.x(), curPos.y(),  newPos.x(), newPos.y());
			if (newPos.x() < 15.0  ||newPos.y() < 15.0   || newPos.x() >= (canvasWidth -15)||   newPos.y() >= (canvasHeight-15) ) continue;
			if (( curPos.x() == newPos.x() ) && (curPos.y() == newPos.y()) )  continue;
			qDebug(" Graph: Emitting signal for node %i to move from %f, %f to new x new y = %f, %f",(*v1).name(), curPos.x(), curPos.y(),  newPos.x(), newPos.y());
			emit moveNode((*v1).name(),  newPos.x(),  newPos.y());
			(*v1).setX( newPos.x());
			(*v1).setY( newPos.y());
		}

	
	}

}

Graph::~Graph() {
 	clear();
	index.clear();
}





