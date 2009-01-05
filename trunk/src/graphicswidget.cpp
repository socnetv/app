/***************************************************************************
 SocNetV: Social Networks Visualiser
 version: 0.49
 Written in Qt 4.4

                        graphicswidget.cpp description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
 ***************************************************************************/

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

#include "graphicswidget.h"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QWheelEvent>
#include <math.h>
#include "mainwindow.h"
#include "node.h"
#include "edge.h"
#include "nodenumber.h"
#include "nodelabel.h"
#include "backgrcircle.h"
#include "edgeweight.h"

/** 
	Constructor method. Called when a GraphicsWidget object is created in MW 
*/
GraphicsWidget::GraphicsWidget( QGraphicsScene *sc, MainWindow* par)  : QGraphicsView ( sc,par) {
	setScene(sc);
	secondDoubleClick=FALSE;
	dynamicMovement=FALSE;
	moving=0;
	timerId=0;
	layoutType=0;
	m_numberColor="black";
	m_nodeLabel="";
	
	zoomIndex=3;
	
	m_currentScaleFactor = 1;
	m_currentRotationAngle = 0;
}



/**
	http://thesmithfam.org/blog/2007/02/03/qt-improving-qgraphicsview-performance/#comment-7215
*/
void GraphicsWidget::paintEvent ( QPaintEvent * event ){
	QPaintEvent *newEvent=new QPaintEvent(event->region().boundingRect());
	QGraphicsView::paintEvent(newEvent);
	delete newEvent;
}



/** 
	Clears the scene 
*/
void GraphicsWidget::clear() {
	qDebug("GW: clear()");
	int i=0;
	nodeVector.clear();
	QList<QGraphicsItem *> allItems=scene()->items();
	foreach (QGraphicsItem *item, allItems ) {
		(item)->hide();
		scene()->removeItem (item);
		i++;
	}
	qDebug("GW: Removed %i items from scene. Scene items now: %i ",i, scene()->items().size());
	qDebug("GW: items now: %i ", this->items().size());
}





/**	
	Adds a new node onto the scene 
	Calls MW to get lastAvailableNodeNumber from graph and 
	and signals MW to update graph.

	This method is called from: 
	- mouseDoubleClickEvent, when the user double clicks somewhere.
	- Graph::createVertex method (on loading files or pressing "Add Node" button.

*/
void GraphicsWidget::drawNode(int num, int size, QString nodeColor, QString nodeLabel, QString labelColor, QPointF p, QString ns, bool showLabels, bool showNumbers) {
	qDebug()<< "GW: drawNode(): drawing new node at: " << p.x() << ", "<< p.y();

	Node *jim= new Node (this, num, size, nodeColor, nodeLabel, labelColor, ns, m_labelDistance, m_numberDistance, p);

	qDebug()<< "GW: drawNode(): drawing node label...";
	NodeLabel *labelJim = new  NodeLabel (jim, nodeLabel, scene() );
	labelJim -> setPos(p.x()+m_labelDistance, p.y()-m_labelDistance);
	labelJim -> setDefaultTextColor (labelColor);

	if (showLabels) qDebug("GW: drawNode: will display label "+ nodeLabel.toAscii() + " for %i", num);
	else qDebug("GW: drawNode: NOT display labels for %i", num);

	if (!showLabels){
		labelJim->hide();
	}

	qDebug()<< "GW: drawNode(): drawing node number...";
	NodeNumber *numberJim = new  NodeNumber ( jim, size+2, QString::number(num), scene() );
	numberJim -> setPos( p.x()+m_numberDistance, p.y()+m_numberDistance );
	numberJim -> setDefaultTextColor (m_numberColor);

	if (!showNumbers){
		numberJim->hide();
        }

	//add the new node to a nodeVector to ease finding which node has a specific nodeNumber 
	//The nodeVector is used in drawEdge() method
	nodeVector.push_back(jim);
}







/** 	Draws an edge from source to target Node. 
	This is used when we dont have references to nodes - only nodeNumbers:
	a) when we load a network file (check = FALSE)
	b) when the user clicks on the AddLink button on the MW.
*/
void GraphicsWidget::drawEdge(int i, int j, int weight, bool reciprocal, bool drawArrows, QString color, bool bezier, bool check){
	qDebug("GW: drawEdge (%i, %i) weight %i", i, j, weight);
	qDebug()<<"GW: nodeVector reports "<< nodeVector.size()<<" items";
	if (check) {
		vector<Node*>::iterator it;
		int index=1;
		for ( it=nodeVector.begin() ; it < nodeVector.end(); it++ ) {
			if ((*it)->nodeNumber() == i ) {		
				break;
			}
			index++;
		}
		i=index;
		index=1;
		for ( it=nodeVector.begin() ; it < nodeVector.end(); it++ ) {
			if ((*it)->nodeNumber() == j ) {		
				break;
			}
			index++;
		}
		j=index;

	}
	if (i == j ) bezier = true;
	
	Edge *edge=new Edge (this, nodeVector.at(i-1), nodeVector.at(j-1), weight, m_nodeSize, color, reciprocal, drawArrows, bezier);
	edge->setColor(color);
	
	QString edgeName = QString::number(i) + QString(">")+ QString::number(j);
	qDebug("GW: adding edge between %i and % to edgesMap. Name: "+edgeName.toAscii(), i, j);
	edgesMap [edgeName] =  edge;

	qDebug("Scene items now: %i ", scene()->items().size());
	qDebug("GW items now: %i ", items().size());

	qDebug()<< "GW: drawNode(): drawing edge weight...";
	double x = ( (nodeVector.at(i-1))->x() + (nodeVector.at(j-1))->x() ) / 2.0;
	double y = ( (nodeVector.at(i-1))->y() + (nodeVector.at(j-1))->y() ) / 2.0;
	EdgeWeight *edgeWeight = new  EdgeWeight (edge, 7, QString::number(weight), scene() );
	edgeWeight-> setPos(x,y);
	edgeWeight-> setDefaultTextColor (color);
	edgeWeight-> hide();
	qDebug("Scene items now: %i ", scene()->items().size());
	qDebug("GW items now: %i ", items().size());

}




/**
	Called from Graph to make an existing arc symmetric (reciprocal)
*/
void GraphicsWidget::drawEdgeReciprocal(int source, int target){
	qDebug("GW: drawEdgeReciprocal ()");
	QString edgeName = QString::number(source) + QString(">")+ QString::number(target);
	qDebug("GW: making existing edge between %i and %i reciprocal. Name: "+edgeName.toAscii(), source, target );
	edgesMap [edgeName]->makeReciprocal();
}



/**
	Called from Graph to unmake an existing symmetric (reciprocal) edge to one-directed only.
*/
void GraphicsWidget::unmakeEdgeReciprocal(int source, int target){
	qDebug("GW: unmakeEdgeReciprocal ()");
	QString edgeName = QString::number(source) + QString(">")+ QString::number(target);
	qDebug("GW: removing edge between %i and %i. Name: "+edgeName.toAscii(), source, target );
	edgesMap [edgeName]->unmakeReciprocal();
}



/** 	
	This method is called when the user middle clicks on two nodes consecutively. .
	It saves the source & target nodes that were clicked 
	On the second double/middle click event, it calls drawEdge method. 
*/
void GraphicsWidget::startEdge(Node *node){
	qDebug("GW: startEdge()");
	if (secondDoubleClick){
		qDebug("GW: startEdge(): this is the second double click. Emitting userMiddleClicked() to create edge");
		secondNode=node;
		emit userMiddleClicked(firstNode->nodeNumber(), secondNode->nodeNumber(), 1);
		( (MainWindow*)parent() )->setCursor(Qt::ArrowCursor);
		secondDoubleClick=FALSE;
		
	}
	else{
		qDebug("GW: startEdge(): this is the first double click.");
		firstNode=node;
		secondDoubleClick=TRUE;
		( (MainWindow*)parent() )->setCursor( Qt::PointingHandCursor); 
	}
}



/** 
	This is called from each node when the user clicks on it.
	It emits the selectedNode signal to MW which is used to
	- display node info on the status bar
	- notify context menus for the clicked node.
*/
void GraphicsWidget::nodeClicked(Node *node){
	qDebug ("GW: Emitting selectedNode()");
	emit selectedNode(node);
}



/** 
	This is called from each edge when the user clicks on it.
	It emits the selectedEdge signal to MW which is used to
	- display edge info on the status bar
	- notify context menus for the clicked edge.
*/
void GraphicsWidget::edgeClicked(Edge *edge){
	qDebug ("GW: Emitting selectedEdge()");
	emit selectedEdge(edge);
}




/** 
	Called from each node when they move.
	Updates 
	- node coordinates in activeGraph (via updateNodeCoords() signal)

*/
void GraphicsWidget::nodeMoved(int number, int x, int y){
	qDebug ("GW: nodeMoved() for %i with %i, %i. Emitting updateNodeCoords() signal", number, x,y);
	emit updateNodeCoords(number, x, y);
}



/** 
	Called from activeGraph to update node coordinates 
	on the canvas  
*/
void GraphicsWidget::moveNode(int number, int x, int y){
	qDebug ("GW: updateNode() %i with %i, %i", number, x,y);
	nodeVector.at(number-1)->setPos(x,y);
	if (nodeVector.at(number-1)->nodeNumber()!=number) qDebug("ERRRRRRRRRRRRRRRRRRROOOOR");
}


/**
	Called from MainWindow
*/
void GraphicsWidget::removeNode(int doomedJim){
	QList<QGraphicsItem *> list=scene()->items();
	qDebug("GW: Scene items= %i - View items : %i",scene()->items().size(), items().size());
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
		if ( (*it)->type()==TypeNode) {
			Node *jim=(Node*) (*it);
			if ( jim->nodeNumber()==doomedJim)	{
				qDebug("GW: found doomedJim %i. Calling node->remove()", jim->nodeNumber());
				jim->die();
				delete *it;
				break;
			}
		}
	}
	qDebug("GW: Scene items now= %i - View items now= %i ", scene()->items().size(), items().size() );

}



/**
	Called from MainWindow when removing links by vertex numbers
*/
void GraphicsWidget::removeEdge(int sourceNode, int targetNode){
	qDebug("GW: Scene items= %i - View items : %i",scene()->items().size(), items().size());
	QList<QGraphicsItem *>  list=scene()->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++){
		if ( (*it)->type()==TypeEdge ) {
			Edge *edge=(Edge*) (*it);
			if ( edge->sourceNodeNumber()==sourceNode && edge->targetNodeNumber()==targetNode ) {
				removeItem(edge);
				break;
			}
		}
	}
	qDebug("GW: Scene items now= %i - View items now= %i ", scene()->items().size(), items().size() );
}


/** 
	Called from Node::die() to removeItem from nodeVector...
*/
void GraphicsWidget::removeItem( Node *node){
	vector<Node*>::iterator it;
	int i=0;
	for ( it=nodeVector.begin() ; it < nodeVector.end(); it++ ) {
		if ((*it)->nodeNumber() == node->nodeNumber()) {		
			break;
		}
		i++;
	}
	nodeVector.erase(nodeVector.begin()+i);
	node->deleteLater ();
	qDebug("GW items now: %i ", items().size());
}




/** Called from Node::die() to removeItem from nodeVector... */
void GraphicsWidget::removeItem( Edge * edge){
	edge->remove();
	delete (edge);
}


void GraphicsWidget::removeItem( NodeLabel *nodeLabel){
	qDebug("GW items now: %i ", items().size());
	delete (nodeLabel);
	qDebug("GW items now: %i ", items().size());
}



void GraphicsWidget::removeItem( NodeNumber *nodeNumber){
	delete (nodeNumber);
}






/** 
	Passes initial node color from MW.
	It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNodeColor(QString color){
	qDebug("GW setting initNodeColor");
	m_nodeColor=color;
}



/** 
	Passes initial node color from MW.
	It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitLinkColor(QString color){
	qDebug("GW setting initLinkColor");
	m_linkColor=color;
}



/** 
	Changes/Sets the color of an node.
	It is called from MW when the user changes the color of a node (right-clicking).
*/
bool GraphicsWidget::setNodeColor(int nodeNumber, QString color){
	QList<QGraphicsItem *> list=scene()->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++){
		if ( (*it)->type()==TypeNode) {
			Node *node=(Node*) (*it);
			if ( node->nodeNumber()==nodeNumber ) {
				node->setColor(color);
				return true;
			}
		}
	}
	return false;
}


/** 
	Changes/Sets the color of an edge.
	It is called from MW when the user changes the color of an edge (right-clicking).
*/
bool GraphicsWidget::setEdgeColor(int source, int target, QString color){
	QList<QGraphicsItem *> list=scene()->items();
	for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++){
		if ( (*it)->type()==TypeEdge) {
			Edge *edge=(Edge*) (*it);
			if ( edge->sourceNodeNumber()==source && edge->targetNodeNumber()==target ) {
				edge->setColor(color);
				return true;
			}
		}
	}
	return false;
}




/** 
	Passes initial node size from MW.
	It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNodeSize(int size){
	qDebug("GW setting initNodeSize");
	m_nodeSize=size;
}



/** 
	Passes initial number distance from node 
	It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNumberDistance(int numberDistance){
	qDebug("GW setting initNumberDistance");
	m_numberDistance=numberDistance;
}


/** 
	Passes initial label distance from node 
	It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitLabelDistance(int labelDistance){
	qDebug("GW setting initLabelDistance");
	m_labelDistance=labelDistance;
}


/**
*	Changes the visibility of an GraphicsView item (number, label, edge, etc)
*/
void GraphicsWidget::setAllItemsVisibility(int type, bool visible){
	QList<QGraphicsItem *> list = scene()->items();
	for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
		if ( (*item)->type() == type){
			if (visible)	(*item)->show();
			else	(*item)->hide();
		}
	}
}



void GraphicsWidget::addBackgrCircle( int x0, int y0, int radius){
	BackgrCircle *circ=new BackgrCircle (this, x0, y0, radius);
	circ->show();

}


void GraphicsWidget::addBackgrHLine( int y0){
	BackgrCircle *circ=new BackgrCircle (this, y0, 	this->width());
	circ->show();
}

void GraphicsWidget::clearBackgrCircles(){
	QList<QGraphicsItem *> allItems=scene()->items();
	foreach (QGraphicsItem *item, allItems ) {
		if ( (item)->type()==TypeBackgrCircle) {
			qDebug("GW: Deleting a background Circle now...");
			(item)->hide();
			delete (item);
		}
	}
}





/** 	
	Creates a new node when the user double-clicks somewhere
*/
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {
	qDebug("GW: mouseDoubleClickEvent() double click detected!");
	if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
		if (Node *node = qgraphicsitem_cast<Node *>(item)) {
			Q_UNUSED(node);
			qDebug("double click on a node... Cant create new one!");
			return;
		}
	}

	QPointF p = mapToScene(e->pos());
	qDebug("GW: mouseDoubleClickEvent(): Emit a signal to MW to create a new node in graph. e->pos() (%i, %i) at %f, %f", e->pos().x(),e->pos().y(), p.x(),p.y());
	
	emit userDoubleClicked(-1, p);
	qDebug("GW: mouseDoubleClickEvent(): Scene and GW items now: %i and %i. Emitting Changed() signal... ", scene()->items().size(), items().size());
}



/** 	
	On the event of a right-click on a node, the node calls this function
	to emit a signal to MW to open a context menu at the mouse position.
	Node is already passed with selectedNode(Node *) signal
	The position of the menu is determined by QMouse:pos()...
*/
void GraphicsWidget::openNodeContextMenu(){
	qDebug("GW: emitting openNodeMenu()");
	emit openNodeMenu();
}


/** 	
	On the event of a right-click on an edge, the edge calls this function
	to emit a signal to MW to open a context menu at the mouse position.
	Edge is already passed with selectedNode(Node *) signal
	The position of the menu is determined by QMouse:pos()...
*/
void GraphicsWidget::openEdgeContextMenu(){
	qDebug("GW: emitting openEdgeMenu()");
	emit openEdgeMenu();
}




/** 
	This slot is activated when the user clicks on the relevant MainWindow checkbox (SpringEmbedder, Fruchterman) to start or stop 
	the movement of nodes, according to the requested model. 
	state: toggle 
	type:  controls the type of model requested.
*/
void GraphicsWidget::nodeMovement(int state, int type){
	qDebug("GW: startNodeMovement()");	
	if (state == Qt::Checked){
		dynamicMovement = TRUE;
		layoutType=type;
		if (!timerId) {
			timerId = startTimer(100);	
			qDebug("GW: startTimer()");
		}
	}
	else
		dynamicMovement = FALSE;
}




/**	This method is called automatically when a QTimerEvent occurs
	It makes all nodes calculate the forces applied to them 
*/
void GraphicsWidget::timerEvent(QTimerEvent *event) {	
	qDebug("GW: timerEvent()");
	Q_UNUSED(event);
	QList<Node *> nodes;

 	foreach (Node *node, nodeVector) {
		switch (layoutType){
			case 1: 
				node->calculateForcesSpringEmbedder(dynamicMovement);
				break;
			case 2: 
				node->calculateForcesFruchterman(dynamicMovement);
				break;
		
		}
	}

	bool itemsMoved = false;
// 	foreach (Node *node, nodeVector) { 
// 		if (node->advance()){
// 			qDebug("GW: timerEvent() a node is moving!");
// 			itemsMoved = true;
// 		}
// 	}

	if (!itemsMoved) {
		killTimer(timerId);
		timerId = 0;
	}
}




/** 
	Calls the scaleView() when the user spins the mouse wheel
	It passes delta as new m_scale
*/
void GraphicsWidget::wheelEvent(QWheelEvent *e) {
	qDebug("GW: Mouse wheel event");
	qDebug("GW: delta = %i", e->delta());	
  	float m_scale = e->delta() / qreal(600);
	qDebug("GW: m_scale = %f", m_scale);	
	if ( m_scale > 0)
		zoomIn();
	else  if ( m_scale < 0) 
		zoomOut();
	else m_scale=1;
}


/** 
	Called from MW magnifier widgets
*/
void GraphicsWidget::zoomOut (){
	if (zoomIndex > 0) {
		zoomIndex--;
		changeZoom(zoomIndex);
	}
	qDebug("GW: ZoomOut() index %i", zoomIndex);
    	emit zoomChanged(zoomIndex);
}	


/** 
	Called from MW magnifier widgets
*/
void GraphicsWidget::zoomIn(){
	qDebug("GW: ZoomIn()");
	if (zoomIndex < 6) {
		zoomIndex++;
		changeZoom(zoomIndex);
	}
	qDebug("GW: ZoomIn() index %i", zoomIndex);
    	emit zoomChanged(zoomIndex);
}


/**
      Initiated from MW zoomCombo widget to zoom in or out.
*/
void GraphicsWidget::changeZoom(int value) {
	double scaleFactor = 0.25;
	scaleFactor *= (value + 1);
	m_currentScaleFactor = scaleFactor;
	QMatrix oldMatrix = matrix();
	resetMatrix();
//	translate(oldMatrix.dx(), oldMatrix.dy());
	this->scale(scaleFactor, scaleFactor);
	rotate(m_currentRotationAngle);
}



void GraphicsWidget::rot(int angle){
	qDebug("rotating");
	m_currentRotationAngle = angle;
	resetMatrix();
	scale(m_currentScaleFactor, m_currentScaleFactor);
	rotate(angle);

}

/** Resizing the view causes a repositioning of the nodes maintaining the same pattern*/
void GraphicsWidget::resizeEvent( QResizeEvent *e ) {
	Q_UNUSED(e);
// 	qDebug ("GraphicsWidget: resizeEvent");
// 	int w=e->size().width();
// 	int h=e->size().height();
// 	int w0=e->oldSize().width();
// 	int h0=e->oldSize().height();
// 	qreal fX=  (double)(w)/(double)(w0);
// 	qreal fY= (double)(h)/(double)(h0);
// 	foreach (QGraphicsItem *item, scene()->items()) {
// 		qDebug ("item will move by %f, %f", fX, fY);
// 		if (Node *node = qgraphicsitem_cast<Node *>(item) ) {
// 			qDebug("Node original position %f, %f", item->x(), item->y());
// 			qDebug("Node will move to %f, %f",item->x()*fX, item->y()*fY);
// 			node->setPos(mapToScene(item->x()*fX, item->y()*fY));
// 		}	
// 		else 	item->setPos(mapToScene(item->x()*fX, item->y()*fY));
// 	}
// 	emit windowResized(w, h);

}



/** 
	Destructor.
*/
GraphicsWidget::~GraphicsWidget(){
}

