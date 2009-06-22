/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.70
 Written in Qt 4.4

                        node.cpp  -  description
                        -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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
#include <QDebug>
 
#include "graphicswidget.h"
#include "node.h"
#include "edge.h"
#include "nodelabel.h"
#include "nodenumber.h"

#include <math.h> //sqrt

Node::Node( GraphicsWidget* gw, int num, int size, 
			QString col, QString shape, bool numIn, 
			int ldist, int ndist, QPointF p 
			) : graphicsWidget (gw) 
{
	graphicsWidget->scene()->addItem(this); //Without this nodes don't appear on the screen...
	setFlags(ItemIsSelectable | ItemIsMovable); //Without this, the node cannot move nor be selected ...
	setAcceptsHoverEvents(true);
	m_num=num;
	m_size=size;
	m_hasLabel=false;
	m_hasNumber=false;
	m_isNumberInside = numIn;
	m_shape=shape;
	m_col_str=col;
	m_col=QColor(col);
	m_nd=ndist;
	m_ld=ldist;
	m_poly_t=new QPolygon(3);
	m_poly_d=new QPolygon(4);
	qDebug()<< "Node: constructor: initial position at: "<< this->x()<<", "<<this->y()<< " Moving now at: "<< p.x()<<", "<<p.y();;



/*	connect (this, SIGNAL(nodeClicked(Node*)),graphicsWidget , SLOT(nodeClicked(Node*)));
	connect (this, SIGNAL(startNodeMovement(int)), graphicsWidget, SLOT(startNodeMovement(int)));
	connect (this, SIGNAL(openNodeContextMenu()), graphicsWidget, SLOT(openNodeContextMenu()));
	connect (this, SIGNAL(startEdge(Node*)),graphicsWidget, SLOT(startEdge(Node*)));*/
	//qDebug("Node() - End of constructor");
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
	qreal c=15.0;
	qreal weight_coefficient=10;		//affects speed and line length. Try 10...
	if (dynamicMovement){
		// Sum up all pushing forces (i.e. imagine nodes are electrons)
		foreach (QGraphicsItem *item, scene()->items()) {
			Node *node = qgraphicsitem_cast<Node *>(item);
			if (!node)
				continue;
			QLineF line(mapFromItem(node, 0, 0) , QPointF(0, 0) );
			qreal dx = line.dx();
			qreal dy = line.dy();
			dist = (dx * dx + dy * dy);
			if (dist > 0) {
				xvel += (dx * c) / dist;
				yvel += (dy * c) / dist;
			}
		}

	    // Now subtract all pulling forces (i.e. springs)
		double weight = (inEdgeList.size() + 1) * weight_coefficient;
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
	Used by MW::slotChangeNodeColor
*/
void Node::setColor(QString str) {
	prepareGeometryChange();
	m_col=QColor(str);
}

/** 
	Used by MW::slotFindNode() 
*/
void Node::setColor(QColor color){
	m_col=color;
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



/**  Used by MainWindow::findNode() and Edge::Edge()  */
int Node::size(){
	qDebug("size()");
	return m_size;  
}


/**  Called every time the user needs to change the shape of an node. */
void Node::setShape(QString shape) {
	qDebug("Node: setShape()");
	prepareGeometryChange();
	m_shape=shape;
	qDebug ("Node: setShape(): node is at x=%f and y=%f", x(), y());
}



/* 
*	Returns the shape of the node as a path (an accurate outline of the item's shape)
*	Used by the collision algorithm in collidesWithItem() 
*/
QPainterPath Node::shape() const {
 	//qDebug ("Node: shape()");
	return (*m_path);
}


/*
 *  Returns the bounding rectangle of the node
 *  That is the rectangle where all painting will take place.
 */
QRectF Node::boundingRect() const {
	qreal adjust = 6;
	return QRectF(-m_size -adjust , -m_size-adjust , 2*m_size+adjust , 2*m_size +adjust);
	

}


/** 
	Does the actual painting. 
	Called by GraphicsView in every update() 
*/
void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) {
//	painter->setClipRect( option->exposedRect );

	//if the node is being dragged aroung, darken it!
	if (option->state & QStyle::State_Selected) {
		//qDebug()<< " node : selected ";
		painter->setBrush(m_col.dark(150));
	}
	else if (option->state & QStyle::State_MouseOver) {
		//qDebug()<< " node : mouse over";	
		painter->setBrush(m_col.dark(150));
		setZValue(255);		
	}
	//else if (option->state & QStyle::State_Sunken) {
		//qDebug()<< " node : sunken ";
		//setZValue(255);
		//painter->setBrush(m_col_dark.dark(160));
	//}
 	else { //no, just paint it with the usual color.
 		//qDebug()<< " node : nothing";
		setZValue(254);		
		painter->setBrush(m_col);
	}
	painter->setPen(QPen(Qt::black, 0));

	m_path = new QPainterPath;
	if ( m_shape == "circle") {
		m_path->addEllipse (-m_size, -m_size, 2*m_size, 2*m_size);
	}
	else if ( m_shape == "ellipse") {
		m_path->addEllipse(-m_size, -m_size, 2*m_size, 1.5* m_size);
	}
	else if ( m_shape == "box" || m_shape == "rectangle"  ) {  //rectangle: for GraphML compliance
		m_path->addRect (-m_size , -m_size , 1.8*m_size , 1.8*m_size );
	}
	else if (m_shape == "roundrectangle"  ) {  //roundrectangle: GraphML only
		m_path->addRoundedRect (-m_size , -m_size , 1.8*m_size , 1.8*m_size, 60.0, 60.0, Qt::RelativeSize );
	}
	else if ( m_shape == "triangle") {
		m_poly_t -> setPoints (3,  0,-m_size,  -m_size,m_size, m_size,+m_size);
		m_path->addPolygon(*m_poly_t);
		m_path->closeSubpath();
	}	
	else if ( m_shape == "diamond"){
		m_poly_d -> setPoints (4, 0,-m_size,  -m_size,0,       0,+m_size,     +m_size,0);
		m_path->addPolygon(*m_poly_d);
		m_path->closeSubpath();
	}

	painter->drawPath (*m_path);
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
	this->deleteNumber();
	this->deleteLabel();
	inEdgeList.clear();
	outEdgeList.clear();
	this->hide();
	graphicsWidget->removeItem(this);

}


void Node::setLabel ( QString label) {
	prepareGeometryChange();
	m_label->setPlainText(label);
	m_hasLabel=true;
}


QString Node::label ( ) { 
	if (m_hasLabel) {
		return m_label->toPlainText();				
	}	
	else return "";
}



void Node::setNumberInside (bool numIn){


	m_isNumberInside = numIn;
	
	//Move its graphic number
	QPointF myPos = this->pos();
	if ( m_hasNumber ) 
			{
				if (!m_isNumberInside) 	{ //move it outside
						m_number -> setZValue(254);
						m_number -> setPos( myPos.x()+m_nd, myPos.y() );
					}	 
					else { 				//move it inside node
						this->setSize(m_size+2); //increase size to display nicely the number  
						m_number -> setZValue(255);
						m_number->setPos( myPos.x() - m_size-2, myPos.y() - m_size-2 );
					}	 
			}
}




/** Propagates the changes to connected elements, i.e. edges, numbers, etc. 
 *  Checks if the node is inside the scene.
 */
QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
// 	qDebug("Node: itemChange()");
	QPointF newPos = value.toPointF();
	switch (change) {
		case ItemPositionHasChanged:{  //ItemPositionChange
// 			emit adjustOutEdge();
// 			emit adjustInEdge();
			foreach (Edge *edge, inEdgeList) 		//Move each inEdge of this node
				edge->adjust();
			foreach (Edge *edge, outEdgeList)		//Move each outEdge of this node
				edge->adjust();
			
			//Move its graphic number
			if ( m_hasNumber ) 
			{
				qDebug()<< "Node: itemChange() moving number to " << newPos.x() << " " << newPos.y();
				if (!m_isNumberInside) 	{ //move it outside
						m_number -> setZValue(254);
						m_number -> setPos( newPos.x()+m_nd, newPos.y());
					}	 
					else { 				//move it inside node
						m_number -> setZValue(255);
						m_number->setPos( newPos.x() - m_size-2, newPos.y() - m_size-2 );
					}	 
			}
			
			if (m_hasLabel) {
				m_label->setPos( newPos.x()-2, newPos.y()+m_ld);
			}
				
			if ( newPos.x() !=0 && newPos.y() != 0 ){
				qDebug()<<  "Node: ItemChange(): Emitting nodeMoved() for "<< nodeNumber()<< " at: "<<  newPos.x()<< ", "<<  newPos.y();
				graphicsWidget->nodeMoved(nodeNumber(), (int) newPos.x(), (int) newPos.y());	
			}
			else qDebug()<<  "Node: ItemChange(): Not emitting nodeMoved. Node "<< nodeNumber()<<" is at 0,0";

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
	qDebug("Node: >> pressEvent() emitting nodeClicked");
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
		qDebug("Node: Middle-Click on a node. Calling GraphicsWidget startEdge()");
//		emit startEdge(this);
		graphicsWidget->startEdge(this);
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
	m_hasLabel=false;
	m_label->hide();
	graphicsWidget->removeItem(m_label);		
}


void Node::clearLabel(){
	m_hasLabel=false;
}


void Node::deleteNumber( ){
	qDebug ("Node: deleteNumber ");
	m_number->hide();
	graphicsWidget->removeItem(m_number);	
}




void Node::addLabel (NodeLabel* gfxLabel  )  { 
	qDebug("NODE: add label");
	m_label=gfxLabel ;
	m_hasLabel=true;
}

void Node::addNumber (NodeNumber *gfxNum ) {
	 m_number=gfxNum ;
	 m_hasNumber=true;
}

// Node::~Node(){
// /*	qDebug("~Node() calling hide()");
// 	this->hide();
// 	qDebug("hide() ok!");*/
// 	
// }
// 
