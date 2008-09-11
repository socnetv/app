/***************************************************************************
 SocNetV: Social Networks Visualiser
 version: 0.46
 Written in Qt 4.4 with KDevelop

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

/** 
	Constructor method. Called when a GraphicsWidget object is created in MW 
*/
GraphicsWidget::GraphicsWidget( QGraphicsScene *sc, MainWindow* par)  : QGraphicsView ( sc,par) {
	setScene(sc);
	secondDoubleClick=FALSE;
	dynamicMovement=FALSE;
	moving=0;
	timerId=0;
	m_numberColor="black";
	m_nodeLabel="";
	init_Transform = transform();
	zoomIndex=3;
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
				emit changed();
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
/*				(*it)->hide();
				(*it)->show();*/
				emit changed();
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


/**	
	Adds a new node onto the scene 
	Calls MW to get lastAvailableNodeNumber from graph and 
	and signals MW to update graph.

	This method is called from mouseDoubleClickEvent, when the user double clicks somewhere.
	It is also called from MW, on loading files or pressing "Add Node" button.
*/
void GraphicsWidget::addNode(int num, int val, int size, QString nodeColor, QString nodeLabel, QString labelColor, QPointF p, QString ns, bool showLabels, bool showNumbers) {
	qDebug("GW: addNode()");
	Node *jim= new Node (this, num, val, size, nodeColor, nodeLabel, labelColor, ns, m_labelDistance, m_numberDistance);

	jim->setPos(p);
	qDebug("GW: new node position is now at %f, %f", jim->pos().x(), jim-> pos().y());
	
	NodeLabel *labelJim =new  NodeLabel (jim, nodeLabel, scene() );
	labelJim ->setPos(p.x()+m_labelDistance, p.y()-m_labelDistance);
	labelJim->setDefaultTextColor (labelColor);

	if (showLabels) qDebug("GW: addNode: will display label "+ nodeLabel.toAscii() + " for %i", num);
	else qDebug("GW: addNode: NOT display labels for %i", num);

	if (!showLabels){
		labelJim->hide();
	}
	//FIXME nodenumber should be an independant variable
	NodeNumber *numberJim =new  NodeNumber (jim, size+2, QString::number(num), scene());
	numberJim -> setPos(p.x()+m_numberDistance, p.y()+m_numberDistance);
	numberJim -> setDefaultTextColor (m_numberColor);
	if (!showNumbers){
		numberJim->hide();
        }
	//add new node to a nodeVector so its easier to find which node has a specific nodeNumber 
	//This is used during addEdge
	nodeVector.push_back(jim);

	//Notify MW that graph has changed so that the networkModified flag be raised.
	//Usefull on saving/exiting the program.
	qDebug("GW: emitting changed signal");
	emit changed(); 
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
	This method draws an edge from source to target node. 
	It is called by edgeNodes() only on middle clicks.
*/
void GraphicsWidget::addEdge(Node *source, Node *target, bool drawArrows, QString color, bool bezier){
	qDebug("GW: addEdge (node, node)");
	int i=source->nodeNumber();
	int j=target->nodeNumber();
	int weight=1;

	Edge *edge=new Edge (this, source, target, weight, m_nodeSize, color, false, drawArrows, bezier);

	edge->setColor(color);
	
	/**Notify MW that graph has changed. Usefull on saving/exiting the program.*/
	emit changed(); 
	/** Emit a signal to MW to create a new edge in the activeGraph. */
	qDebug("GW: emitting userMiddleClicked to MW, source=%i, target=%i", i, j); 
	emit userMiddleClicked(i, j, weight);
}



/** 	
	Draws an edge from source to target Node. 
	This is used only when we load a network file, 
	because in that case we dont have references to nodes - only nodeNumbers
*/
void GraphicsWidget::addEdge(int i, int j, bool undirected, bool drawArrows, QString color, bool bezier){
	qDebug("GW: addEdge (%i, %i)", i, j);
	int weight=1;
	//FIXME nodeVector indeces must change when add/removing items
	qDebug()<<"GW: nodeVector reports"<< nodeVector.size()<<"items";
	Edge *edge=new Edge (this, nodeVector.at(i-1), nodeVector.at(j-1), weight, m_nodeSize, color, undirected, drawArrows, bezier);
	edge->setColor(color);
	/**Notify MW that graph has been changed. Usefull on saving/exiting the program.*/
	emit changed(); 
}



/** 	
	This method is called when the user middle clicks on two nodes consecutively. .
	It saves the source & target nodes that were clicked 
	On the second double/middle click event, it calls addEdge method. 
*/
void GraphicsWidget::edgeNodes(Node *node){
	qDebug("GW: edgesNodes()");
	if (secondDoubleClick){
		qDebug("GW: this is the second double click. Creating edge");
		secondNode=node;
		addEdge (firstNode, secondNode, true, m_linkColor, false);
		( (MainWindow*)parent() )->setCursor(Qt::ArrowCursor);
		secondDoubleClick=FALSE;
		
	}
	else{
		qDebug("GW: this is the first double click.");
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
	Called from each node when it moves.
	Updates node coordinates in activeGraph of MainWindow
*/
void GraphicsWidget::nodeMoved(int number, int x, int y){
	qDebug ("GW: Emitting nodeMoved() for %i with %i, %i", number, x,y);
	/**Notify MW that graph has changed. Usefull on saving/exiting the program.*/
	emit changed(); 
	emit updateNodeCoords(number, x, y);
}



/** 
	Called from each node when it moves.
	Updates node coordinates in activeGraph of MainWindow
*/
void GraphicsWidget::updateNode(int number, int x, int y){
	qDebug ("GW: updateNode() %i with %i, %i", number, x,y);
	nodeVector.at(number-1)->setPos(x,y);
	if (nodeVector.at(number-1)->nodeNumber()!=number) qDebug("ERRRRRRRRRRRRRRRRRRROOOOR");
	nodeVector.at(number-1)->moveBy(1,1);nodeVector.at(number-1)->moveBy(-1,-1);
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


//Called from Node::die() to removeItem from nodeVector...
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


//Called from Node::die() to removeItem from nodeVector...
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
	Creates a new node when the user double-clicks somewhere
	Calls addNode
*/
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {
	qDebug("GW: double click detected!");
	if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
		if (Node *node = qgraphicsitem_cast<Node *>(item)) {
			Q_UNUSED(node);
			qDebug("double click on a node... Cant create new one!");
			return;
		}
	}
	QPointF p = matrix().inverted().map(e->pos());
	qDebug("GW: e->pos() (%i, %i)", e->pos().x(),e->pos().y());
	qDebug("GW: at %f, %f", p.x(),p.y());

	//Why use p (Qpointf) when e->pos() is Qpoint?  Because QGraphicsItem::Node and setPos works with QPointF
	int m_nodeNumber = ( (MainWindow*)parent() )->lastAvailableNodeNumber() +1;
	bool labels=( (MainWindow*)parent() )->showLabels();
	m_nodeLabel=QString::number(m_nodeNumber);
	bool numbers=( (MainWindow*)parent() )->showNumbers();
	qDebug("GW: lastAvailableNodeNumber is %i", m_nodeNumber);
	m_nodeColor= ( (MainWindow*)parent() )->initNodeColor;	
	addNode(m_nodeNumber, 1, m_nodeSize, m_nodeColor, m_nodeLabel, m_labelColor,  p,  "circle", labels, numbers);
	//Emit a signal to MW to create a new node in graph.
	qDebug("GW: emitting userDoubleClicked to MW"); 
	emit userDoubleClicked(m_nodeNumber, (int) p.x(), (int) p.y());

	qDebug("Scene items now: %i ", scene()->items().size());
	qDebug("GW items now: %i ", items().size());
	emit changed();
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
	This method starts the timer used in node movement
	Called by startNodeMovement()
	and by each node changing position.
*/
void GraphicsWidget::itemMoved(){
//	qDebug("GW: itemMoved()");
	if (!timerId) {
		//timerId = startTimer(1000 / 100);	//increase denom to increase
		timerId = startTimer(100);	
		qDebug("GW: startTimer()");
	}
}



/** 
	This slot is activated when the user clicks on the checkbox to start or stop 
	the movement of nodes.
*/
void GraphicsWidget::startNodeMovement(int state){
	qDebug("GW: startNodeMovement()");	
	if (state == Qt::Checked){
		dynamicMovement = TRUE;
		itemMoved();
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
// 	foreach (QGraphicsItem *item, scene()->items()) {
// 		if (Node *node = qgraphicsitem_cast<Node *>(item))
// 			nodes << node;
// 	}

 	foreach (Node *node, nodeVector)
  		node->calculateForces(dynamicMovement);

	bool itemsMoved = false;
	foreach (Node *node, nodeVector) { 
		if (node->advance()){
			qDebug("GW: timerEvent() a node is moving!");
			itemsMoved = true;
		}
	}

	if (!itemsMoved) {
		killTimer(timerId);
		timerId = 0;
//		qDebug("GW: timerEvent: KillTimer()");
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
	switch (value) {
		case 0:	setTransform (init_Transform); 
			scale(0.25, 0.25);
			break;
		case 1:	setTransform (init_Transform); 
			scale(0.5, 0.5);
			break;
		case 2:	setTransform (init_Transform);
			scale(0.75, 0.75);
			break;
		case 3: setTransform (init_Transform);
			scale(1, 1);
			break;
		case 4: setTransform (init_Transform);
			scale(1.25, 1.25);
			break;
		case 5: setTransform (init_Transform);
			scale(1.5, 1.5);
			break;
		case 6: setTransform (init_Transform);
			scale(1.75, 1.75);
			break;
	}
}



void GraphicsWidget::rot(int angle){
	qDebug("rotating");
	setTransform (init_Transform);	
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

