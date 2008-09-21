/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.48
 Written in Qt 4.4

                        node.cpp  -  description
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


#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOption>
#include <QPainter>
 
#include "graphicswidget.h"
#include "node.h"
#include "edge.h"
#include "nodelabel.h"
#include "nodenumber.h"

#include <math.h> //sqrt

Node::Node( GraphicsWidget* gw, int num, int size, QString col, QString label, QString lcol, QString shape, int ldist, int ndist) : graphicsWidget (gw) {
	qDebug("Node() - constructor");
	graphicsWidget->scene()->addItem(this); //Without this nodes dont appear on the screen...
	setFlag(ItemIsMovable); //Without this, nodes do not move...
	m_num=num;
	m_size=size;
	Q_UNUSED (label);
//	m_label->setPlainText(label);
	hasLabel=false;
	m_shape=shape;
	m_col_str=col;
	m_col=QColor(col);
	m_col_dark=m_col.dark(150);
	m_lcol=lcol;
	m_nd=ndist;
	m_ld=ldist;
	m_poly_t=new QPolygon(3);
	m_poly_d=new QPolygon(4);
/*	connect (this, SIGNAL(nodeClicked(Node*)),graphicsWidget , SLOT(nodeClicked(Node*)));
	connect (this, SIGNAL(startNodeMovement(int)), graphicsWidget, SLOT(startNodeMovement(int)));
	connect (this, SIGNAL(openNodeContextMenu()), graphicsWidget, SLOT(openNodeContextMenu()));
	connect (this, SIGNAL(startEdge(Node*)),graphicsWidget, SLOT(edgeNodes(Node*)));*/
	qDebug("Node() - End of constructor");
} 

/** 
	Assigns forces to the edges and nodes, as if the edges were springs (i.e. Hooke's law) 
	and the nodes were electrically charged particles (Coulomb's law).
	These forces are applied to the nodes iteratively, pulling them closer together or pushing them further apart, 
	until the system comes to an equilibrium state (node positions do not change anymore).
*/

void Node::calculateForcesSpringEmbedder(bool dynamicMovement){
	if (!scene() || scene()->mouseGrabberItem() == this) {
		newPos = pos();
		return;
	}
	qreal xvel = 0;
	qreal yvel = 0;
	double dist =0;
	qreal l=15.0;
	if (dynamicMovement){
		// Sum up all forces pushing this item away (i.e. electron)
		foreach (QGraphicsItem *item, scene()->items()) {
			Node *node = qgraphicsitem_cast<Node *>(item);
			if (!node)
				continue;
			QLineF line(mapFromItem(node, 0, 0) , QPointF(0, 0));
			qreal dx = line.dx();
			qreal dy = line.dy();
			dist = (dx * dx + dy * dy);
			if (dist > 0) {
				xvel += (dx * l) / dist;
				yvel += (dy * l) / dist;
			}
		}

	    // Now subtract all forces pulling items together (spring)
		double weight = (inEdgeList.size() + 1) * 20;
//		double weight1 = (outEdgeList.size() + 1) * 20;
		qDebug("Node: edge weight %f", weight);
		foreach (Edge *edge, inEdgeList) {
			QPointF pos;
			if (edge->sourceNode() == this)	//get other node's coordinates
				pos = mapFromItem(edge->targetNode(), 0, 0);
			else
	            		pos = mapFromItem(edge->sourceNode(), 0, 0);
        		xvel += pos.x() / weight;
			yvel += pos.y() / weight;
		}
	

	 	foreach (Edge *edge, outEdgeList) {
			QPointF pos;
			if (edge->sourceNode() == this)
				pos = mapFromItem(edge->targetNode(), 0, 0);
			else
	            		pos = mapFromItem(edge->sourceNode(), 0, 0);
        		xvel += pos.x() / weight;
			yvel += pos.y() / weight;
		}
		if (qAbs(xvel) < 0.1 && qAbs(yvel) < 0.1)
	        	xvel = yvel = 0;
	}
	QRectF sceneRect = scene()->sceneRect();
	newPos = pos() + QPointF(xvel, yvel);
	newPos.setX(qMin(qMax(newPos.x(), sceneRect.left() + 10), sceneRect.right() - 10));
	newPos.setY(qMin(qMax(newPos.y(), sceneRect.top() + 10), sceneRect.bottom() - 10));
}


/**
The vertices behave as atomic particles or celestial bodies, exerting attractive and repulsive forces on one another; 
the forces induce movement. Our algorithm will resemble molecular or planetary simulations, some-
times called n -body problems. 

Following Eades, however, we know that we need
not have a faithful simulation; we can apply unrealistic forces in an unrealistic
manner. So, like Eades, we make only vertices that are neighbours attract each
other, but all vertices repel each other. This is consistent with the asymmetry of our
two guidelines above.
*/
void Node::calculateForcesFruchterman(bool dynamicMovement){
	if (!scene() || scene()->mouseGrabberItem() == this) {
		newPos = pos();
		return;
	}
	qreal xvel = 0;
	qreal yvel = 0;
	qreal c=1, ca=600;
	qreal dx = 0, dy=0;
	double l=0, dist=0;
	QLineF line;
	if (dynamicMovement){
		l=c*sqrt(ca/10);
		// Sum up all forces pushing this item away (i.e. electron)
		foreach (QGraphicsItem *item, scene()->items()) {
			Node *node = qgraphicsitem_cast<Node *>(item);
			if (!node)
				continue;
			QLineF line(mapFromItem(node, 0, 0) , QPointF(0, 0));
// 			line.setP1(mapFromItem(node, 0, 0));
// 			line.setP2(QPointF(0, 0));
			dx = line.dx();
			dy = line.dy();
			dist = sqrt(dx * dx + dy * dy); //Euclidean distance
			qDebug("NODE: FR().... Repelling forces xvel = %f,   yvel = %f", l*l / dist, l*l / dist);
			if (dist > 0) {
				xvel += ( dx * l ) / dist;
				yvel += ( dy * l ) / dist;
			}
		}
		qDebug("NODE: FR().... Repelling forces SUM xvel = %f,   yvel = %f", xvel, yvel);
	    // Now subtract all forces pulling items together 
		foreach (Edge *edge, inEdgeList) {
			line.setP1(mapFromItem(edge->sourceNode(), 0, 0));
			line.setP2(QPointF(0, 0));
			dx = line.dx();
			dy = line.dy();
			dist = (dx * dx + dy * dy); //Euclidean distance ^2
			qDebug("NODE: FR().... Attracting forces xvel = %f,   yvel = %f", dist /l, dist /l);
			if (dist > 0) {
				xvel += dist /l;
				yvel += dist / l;
			}

		}
		qDebug("NODE: FR().... Attracting forces SUM xvel = %f,   yvel = %f", xvel, yvel);

	    // Now subtract all forces pulling items together 
		foreach (Edge *edge, outEdgeList) {
			line.setP1(QPointF(0, 0));
			line.setP2(mapFromItem(edge->sourceNode(), 0, 0));
			dx = line.dx();
			dy = line.dy();
			dist = (dx * dx + dy * dy); //Euclidean distance ^2
			qDebug("NODE: FR().... Attracting forces xvel = %f,   yvel = %f", dist /l, dist /l);
			if (dist > 0) {
				xvel += dist /l;
				yvel += dist / l;
/*				xvel += dx+log (dist /10);
				yvel += dy+log (dist / 10);*/
			}
		}

		qDebug("NODE: FR().... Attracting forces SUM xvel = %f,   yvel = %f", xvel, yvel);
		if (qAbs(xvel) < 0.1 && qAbs(yvel) < 0.1)
	        	xvel = yvel = 0;
		qDebug("NODE: FR().... TOTAL SUM xvel = %f,   yvel = %f", xvel, yvel);
	}
	QRectF sceneRect = scene()->sceneRect();
	
	newPos = this->pos() + QPointF(xvel, yvel);

	newPos.setX(qMin(qMax(newPos.x(), sceneRect.left() + 10), sceneRect.right() - 10));
	newPos.setY(qMin(qMax(newPos.y(), sceneRect.top() + 10), sceneRect.bottom() - 10));

	qDebug("NODE: FR().... NEW NODE POSITION x = %f,   y = %f", newPos.x(), newPos.y());
}


/** 
	Called from GraphicsWidget->timerEvent() to move continuously the node 
*/
bool Node::advance(){
	if (newPos == pos())
		return false;
	qDebug("Node: at x = %f, y=%f will advance to x = %f, y=%f", x(), y(), newPos.x(), newPos.y() );
	setPos(newPos);
	return true;
}


/** 
	Used by MW::slotChangeNodeColor
*/
void Node::setColor(QString str) {
	prepareGeometryChange();
	m_col=QColor(str);
	m_col_dark=m_col.dark(160);
}

/** 
	Used by MW::slotFindNode() 
*/
void Node::setColor(QColor color){
	m_col=color;
	m_col_dark=m_col.dark(160);
}

QString Node::color () { 
	return m_col_str;
}


/** Sets the size of the node */
void Node::setSize(int size){
	qDebug("Node: setSize()");
 	prepareGeometryChange();
	m_size=size;
	foreach (Edge *edge, inEdgeList) {
		qDebug("Node: updating edges in inEdgeList");
		edge->setEndOffset(size);
	}
	foreach (Edge *edge, outEdgeList) {
		qDebug("Node: updating edges in outEdgeList");
		edge->setStartOffset(size);
	}
	setShape(m_shape);
}



/**  Used by MainWindow::findActor()  */
int Node::width(){
	qDebug("width()");
	return m_size;  
}


/**  Called every time the user needs to change the shape of an node. */
void Node::setShape(QString shape) {
	qDebug("Node: setShape()");
	prepareGeometryChange();
	m_shape=shape;
	qDebug ("Node: setShape(): node is at x=%f and y=%f", x(), y());
}



/** 	Returns the shape of the node as a path (an accurate outline of the item's shape)
	Used by the collision algorithm in collidesWithItem() 
*/
QPainterPath Node::shape() const {
// 	qDebug ("Node: shape()");
	QPainterPath path;

	path.addEllipse(-m_size, -m_size, 2*m_size, 2*m_size);		
/*	if ( m_shape == "circle") {
		path.addEllipse(-m_size, -m_size, 2*m_size, 2*m_size);		
	}
	else if ( m_shape == "ellipse") {
		path.addEllipse(-m_size, -m_size, 2*m_size, 2* m_size);		
	}
	else if ( m_shape == "box") {
		path.addRect( QRectF(-m_size , -m_size , 1.8*m_size , 1.8*m_size ) );
	}
	else if ( m_shape == "triangle") {
		path.addPolygon( *m_poly_t);
	}
	else if ( m_shape == "diamond"){
		path.addPolygon(*m_poly_d);
	}*/
	return path;
}


/** Returns the bounding rectangle of the node */
QRectF Node::boundingRect() const {
//	qDebug ("Node: boundingRect()");
	qreal adjust = 10;
	return QRectF(-m_size -adjust , -m_size-adjust , 2*m_size+adjust , 2*m_size +adjust);

}


/** 
	Does the actual painting. 
	Called by GraphicsView in every update() 
*/
void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) {
	//if the node is being dragged aroung, darken it!
	if (option->state & QStyle::State_Sunken) {
		setZValue(255);
		painter->setBrush(m_col_dark.dark(160));
	} else { //no, just paint it with the usual color.
		setZValue(254);
		painter->setBrush(m_col);
	}
	painter->setPen(QPen(Qt::black, 0));

	if ( m_shape == "circle") {
		painter->drawEllipse(-m_size, -m_size, 2*m_size, 2*m_size);		
	}
	else if ( m_shape == "ellipse") {
		painter->drawEllipse(-m_size, -m_size, 2*m_size, 1.5* m_size);		
	}
	else if ( m_shape == "box") {
		painter->drawRect( QRectF(-m_size , -m_size , 1.8*m_size , 1.8*m_size ) );
	}
	else if ( m_shape == "triangle") {
		m_poly_t -> setPoints (3 ,   0,-m_size,  -m_size,m_size, m_size,+m_size);	
		painter->drawPolygon( *m_poly_t);
	}	
	else if ( m_shape == "diamond"){
		m_poly_d -> setPoints (4 ,   0,-m_size,  -m_size,0,       0,+m_size,     +m_size,0);
		painter->drawPolygon(*m_poly_d);
	}
}




/** Removes the node */

void Node::die() {
	qDebug ("Node: Remove() node= %i", nodeNumber());
/*	emit removeOutEdge();
	emit removeInEdge();*/
	foreach (Edge *edge, inEdgeList) {
		qDebug("Node: removing edges in inEdgeList");
		edge->remove();
		graphicsWidget->removeItem(edge);
	}
	foreach (Edge *edge, outEdgeList) {
		qDebug("Node: removing edges in outEdgeList");
		edge->remove();
		graphicsWidget->removeItem(edge);
	}
	foreach (NodeNumber *num, gfxNumberList) {
		qDebug("Node: removing number in gfxNumberList");
		num->hide();
		this->deleteNumber(num);
		graphicsWidget->removeItem(num);
	}
	this->deleteLabel();

	inEdgeList.clear();
	outEdgeList.clear();
	this->hide();
	graphicsWidget->removeItem(this);

}


void Node::setLabel ( QString label) {
	prepareGeometryChange();
	m_label->setPlainText(label);
	hasLabel=true;
}


QString Node::label ( ) { 
	if (hasLabel)
		return m_label->toPlainText();	
	else return "";
}





/** Propagates the changes to connected elements, i.e. edges, numbers, etc. 
 *  Checks if the node is inside the scene.
 */
QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
// 	qDebug("Node: itemChange()");
	switch (change) {
		case ItemPositionChange:{
// 			emit adjustOutEdge();
// 			emit adjustInEdge();
			foreach (Edge *edge, inEdgeList) 		//Move each inEdge of this node
				edge->adjust();
			foreach (Edge *edge, outEdgeList)		//Move each outEdge of this node
				edge->adjust();
			foreach (NodeNumber *num, gfxNumberList) 	//Move its graphic number
				num->setPos( x()+m_nd, y()+m_nd);
			if (hasLabel)
				m_label->setPos( x()+m_ld, y()-m_ld);
			//graphicsWidget->itemMoved();	
			break;
		}
		case ItemVisibleChange: {	
			return 0;
		}
		default: {
			break;
		}
	};
	return QGraphicsItem::itemChange(change, value);
}



/** handles the events of a click on a node */
void Node::mousePressEvent(QGraphicsSceneMouseEvent *event) {  
	qDebug("Node: pressEvent() emitting nodeClicked");
//	emit nodeClicked(this);
	graphicsWidget->nodeClicked(this);
	if ( event->button()==Qt::LeftButton ) {
		qDebug("Node: pressEvent() left click > startNodeMovement");
//		emit startNodeMovement(0);
//		graphicsWidget->startNodeMovement(0);
	}
	if ( event->button()==Qt::RightButton ) {
		qDebug("Node: Right-click on node, at %i, %i", event->screenPos().x(), event->screenPos().y()); 
//		emit openNodeContextMenu();
		graphicsWidget->openNodeContextMenu();
		/** Update commented out - caused segmentation fault when removing node */
//		update();
//		QGraphicsItem::mousePressEvent(event);
	}
	if ( event->button()==Qt::MidButton) {
		qDebug("Node: Middle-Click on a node. Calling GraphicsWidget edgeNodes");
//		emit startEdge(this);
		graphicsWidget->edgeNodes(this);
	}
}


void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}



void Node::addInLink( Edge *edge ) {
	qDebug("Node:  %i addInLink()", m_num);
/*	connect (this, SIGNAL(adjustInEdge()),edge, SLOT(adjust()));
	connect (this, SIGNAL(removeInEdge()),edge, SLOT(remove()));*/
	inEdgeList.push_back( edge); 
	//qDebug ("Node:  %i inEdgeList has now %i edges", m_num, inEdgeList.size());

}


void Node::deleteInLink( Edge *link ){
	qDebug ("Node:  %i inEdgeList deleting link", m_num);
	//qDebug ("Node: %i inEdgeList has %i edges", m_num, inEdgeList.size());
	inEdgeList.remove( link);
	//qDebug ("Node:  %i inEdgeList has now %i edges", m_num, inEdgeList.size());
}



void Node::addOutLink( Edge *edge ) {
	qDebug("Node: addOutLink()");

/*	connect (this, SIGNAL(adjustOutEdge()),edge, SLOT(adjust()));
	connect (this, SIGNAL(removeOutEdge()),edge, SLOT(remove()));*/
	outEdgeList.push_back( edge); 
//	qDebug ("Node: outEdgeList has now %i edges", outEdgeList.size());
}



void Node::deleteOutLink(Edge *link){
	qDebug ("Node: %i outEdgeList deleting link", m_num);
//	qDebug ("Node: %i outEdgeList has %i edges", m_num, outEdgeList.size());
	outEdgeList.remove( link);
//	qDebug ("Node: %i outEdgeList has now %i edges", m_num, outEdgeList.size());
}



void Node::deleteLabel(){
	qDebug ("Node: deleteLabel ");
	hasLabel=false;
	m_label->hide();
	graphicsWidget->removeItem(m_label);
	//delete m_label;

}


void Node::clearLabel(){
	hasLabel=false;
}


void Node::deleteNumber( NodeNumber *number ){
	qDebug ("Node: deleteNumber ");
//	qDebug ("Node: numberList has %i items", gfxNumberList.size());
	gfxNumberList.remove( number);
//	qDebug ("Node: numberList has now %i items", gfxNumberList.size());

}

void Node::clearNumber(){
	//gfxNumberList
}



void Node::addLabel (NodeLabel* gfxLabel  )  { 
	qDebug("NODE: add label");
	m_label=gfxLabel ;
	hasLabel=true;
}

void Node::addNumber (NodeNumber *gfxNum ) {
	 gfxNumberList.push_back(gfxNum) ;
}

// Node::~Node(){
// /*	qDebug("~Node() calling hide()");
// 	this->hide();
// 	qDebug("hide() ok!");*/
// 	
// }
// 
