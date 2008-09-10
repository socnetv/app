/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.46
 Written in Qt 4.4 with KDevelop 

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


Node::Node( GraphicsWidget* gw, int num, int val, int size, QString col, QString label, QString lcol, QString shape, int ldist, int ndist) : graphicsWidget (gw) {
	qDebug("Node() - constructor");
	graphicsWidget->scene()->addItem(this); //Without this nodes dont appear on the screen...
	setFlag(ItemIsMovable); //Without this, nodes do not move...
	m_num=num;
	m_val=val;
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



void Node::calculateForces(bool dynamicMovement){
	if (!scene() || scene()->mouseGrabberItem() == this) {
		newPos = pos();
		return;
	}
	qreal xvel = 0;
	qreal yvel = 0;

	if (dynamicMovement){
		// Sum up all forces pushing this item away (spring)
		foreach (QGraphicsItem *item, scene()->items()) {
			Node *node = qgraphicsitem_cast<Node *>(item);
			if (!node)
				continue;
			QLineF line(mapFromItem(node, 0, 0), QPointF(0, 0));
			qreal dx = line.dx();
			qreal dy = line.dy();
			double l = 4.0 * (dx * dx + dy * dy);
			if (l > 0) {
				xvel += (dx * 150.0) / l;
				yvel += (dy * 150.0) / l;
			}
		}

	    // Now subtract all forces pulling items together (gravitational)
		double weight = (inEdgeList.size() + 1) * 20;
//		double weight1 = (outEdgeList.size() + 1) * 20;
		qDebug("Node: edge weight %f", weight);
		foreach (Edge *edge, inEdgeList) {
			QPointF pos;
			if (edge->sourceNode() == this)
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
	m_shape=shape;
	qDebug ("Node: setShape(): node is at x=%f and y=%f", x(), y());
	prepareGeometryChange();
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

void Node::remove () {
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
		graphicsWidget->startNodeMovement(0);
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
