/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.50
 Written in Qt 4.4

                        edge.cpp  -  description
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
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QtDebug>		//used for qDebug messages
#include <math.h>

#include "graphicswidget.h"
#include "edge.h"
#include "node.h"
#include "edgeweight.h"


static const double Pi = 3.14159265;
static double TwoPi = 2.0 * Pi;


Edge::Edge(  GraphicsWidget *gw, Node *from, Node *to, int weight, int nodeSize, QString color, bool reciprocal, bool drawArrows, bool bez): graphicsWidget(gw){
	qDebug("Edge: Edge()");
	Q_UNUSED(nodeSize);
	graphicsWidget->scene()->addItem(this);  //Without this, edges dont appear on the screen...

 	from->addOutLink( this );	//adds this Edge to sourceNode
	to->addInLink( this );		//adds this Edge to targetNode

	source=from;			//saves the sourceNode
	target=to;			//Saves the targetNode
	m_color=color;
	m_drawArrows=drawArrows;
	m_reciprocal=reciprocal;
	m_startOffset=source->width();  //used to offset edge from the centre of node
	m_endOffset=target->width();  //used to offset edge from the centre of node	
	qDebug("Edge() m_startOffset %i",(int) m_startOffset);
	qDebug("Edge() m_endOffset %i",(int) m_endOffset);

	m_arrowSize=5;		//controls the width of the edge arrow

	eFrom = source->nodeNumber() ;
	eTo = target->nodeNumber() ;
	m_weight = weight ;
	m_Bezier = bez; 
	this-> setZValue(253);		//Edges have lower z than nodes. Nodes always appear above edges.
	this->setBoundingRegionGranularity(0.05);				//slows down the universe...
	//this->setCacheMode (QGraphicsItem::DeviceCoordinateCache);  //slows down
	adjust();
}



void Edge::showArrows(bool drawArrows){
	m_drawArrows=drawArrows;
}



void Edge::removeRefs(){
	//FIXME Need to disconnect signals from node to this "erased" edge
	qDebug("Edge: removeRefs()");
	source->deleteOutLink(this);
	target->deleteInLink(this);
}

void Edge::remove(){
	qDebug("Edge: remove(), calling removeRefs()");
	removeRefs();
	this->hide();
	
}

void Edge::setColor( QString str) {
	m_color=str;
 	prepareGeometryChange();
}


QString Edge::color() { 
	return m_color; 
}

void Edge::setWeight( int w) {
	m_weight = w;
}

int Edge::weight() { 
	return m_weight; 
}


void Edge::setStartOffset(int offset){
	m_startOffset=offset;
}

void Edge::setEndOffset(int offset){
	m_endOffset=offset;
}


Node *Edge::sourceNode() const {
    return source;
}

void Edge::setSourceNode(Node *node) {
	source = node;
	adjust();
}


Node *Edge::targetNode() const {
	return target;
}


void Edge::setTargetNode(Node *node){
	target = node;
	adjust();
}


int Edge::sourceNodeNumber () { 
	return eFrom;
}

int Edge::targetNodeNumber() { 
	return eTo;
}


void Edge::addWeight (EdgeWeight* canvasWeight  )  {
	weightList.push_back( canvasWeight) ; 
}


void Edge::clearWeightList(){
	weightList.clear();
}


//leaves space 
void Edge::adjust(){
	qDebug("Edge: adjust()");
	if (!source || !target)
		return;
	QLineF line(mapFromItem(source, 0, 0), mapFromItem(target, 0, 0));
	QPointF edgeOffset;
	if (source!=target) {
	 	qreal length = line.length();
		edgeOffset = QPointF((line.dx() * m_endOffset) / length, (line.dy() *m_endOffset) / length);		
	}
	else edgeOffset = QPointF(0, 0);  
	
 	prepareGeometryChange();
	sourcePoint = line.p1() + edgeOffset;
	targetPoint = line.p2() - edgeOffset;
	qDebug()<<"----Edge: adjust() "<< sourcePoint.x()<< " "<<sourcePoint.y();
	foreach (EdgeWeight *wgt, weightList) 		//Move the weight of this edge
		wgt->setPos( (source->x()+target->x())/2.0, (source->y()+target->y())/2.0 );
}



QPainterPath Edge::shape () const {
	QPainterPath path;
	//path.addRegion(boundingRegion(QTransform()));
	path.addRect(boundingRect());
	return path;
} 


QRectF Edge::boundingRect() const {

	if (!source || !target)
		return QRectF();
	
	qreal penWidth = 1;
	qreal extra = (penWidth + m_arrowSize) / 2.0;

	if (source==target) {
			return QRectF (
			sourcePoint-QPointF(30,30), QSizeF(60,30)).normalized().adjusted(-extra, -extra, extra, extra);
	}
	return QRectF (
			sourcePoint, QSizeF(targetPoint.x() - sourcePoint.x(),
			targetPoint.y() - sourcePoint.y())
			).normalized().adjusted(-extra, -extra, extra, extra);
}



void Edge::makeReciprocal(){
 	prepareGeometryChange();
	m_reciprocal= true;
}



void Edge::unmakeReciprocal(){
 	prepareGeometryChange();
	m_reciprocal= false;
}


void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
	if (!source || !target)
		return;
	Q_UNUSED(option); //	painter->setClipRect( option->exposedRect );
	qDebug()<<"***EDGE::paint() edge from "<< sourceNodeNumber()<< " to "<<targetNodeNumber();

	//Define the path upon which we'' ll draw the line 
	QPainterPath line(sourcePoint);
	qDebug()<<"SourceNode x and y :"<<(sourceNode())->x() <<","<< (sourceNode())->y();
	//Construct the path
	if (source!=target) {
		if ( !m_Bezier){
			qDebug("Edge::paint(). Constructing a line");
			line.lineTo(targetPoint);
		}
		else {
			qDebug("Edge::paint(). Constructing a bezier curve");
		}
	}
	else { //self-link
		QPointF c1 = QPointF( targetPoint.x() -30,  targetPoint.y() -30 );
		QPointF c2 = QPointF( targetPoint.x() +30,  targetPoint.y() -30 );
		qDebug()<<"Edge::paint(). Constructing a bezier self curve c1 "<<c1.x()<<","<<c1.y()<< " and c2 "<<c2.x()<<","<<c2.y();
		line.cubicTo( c1, c2, targetPoint);
	}
	painter->setPen(QPen(QColor(m_color), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

	//Draw the arrows only if we have different nodes.
	if (m_drawArrows && source!=target) {
		qDebug("Edge: Building arrows for this edge. First create Arrow at target node");
// 		double angle = ::acos(line.dx() / line.length());
		double angle = ::acos((targetPoint.x()-sourcePoint.x()) / line.length());
//		if (line.dy() >= 0)
		if ((targetPoint.y()-sourcePoint.y())  >= 0)
			angle = TwoPi - angle;

		QPointF destArrowP1 = targetPoint + QPointF(sin(angle - Pi / 3) * m_arrowSize,
                	                              cos(angle - Pi / 3) * m_arrowSize);
		QPointF destArrowP2 = targetPoint + QPointF(sin(angle - Pi + Pi / 3) * m_arrowSize,
                                              cos(angle - Pi + Pi / 3) * m_arrowSize);
		painter->setBrush(QColor(m_color));
		QPolygonF destP;
		destP << targetPoint << destArrowP1 << destArrowP2;
		line.addPolygon ( destP);
		//painter->drawPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);
		if (m_reciprocal) { 
			qDebug("Edge: This edge is SYMMETRIC! So, we need to create Arrow at src node as well");
			QPointF srcArrowP1 = sourcePoint + QPointF(sin(angle +Pi / 3) * m_arrowSize,
                	                              cos(angle +Pi / 3) * m_arrowSize);
			QPointF srcArrowP2 = sourcePoint + QPointF(sin(angle +Pi - Pi  / 3) * m_arrowSize,
                                              cos(angle +Pi - Pi / 3) * m_arrowSize);
			QPolygonF srcP;
			srcP << sourcePoint<< srcArrowP1<< srcArrowP2;
			line.addPolygon ( srcP);
			
//			painter->drawPolygon(QPolygonF() << line.p1() << srcArrowP1 << srcArrowP2);
		}
		else qDebug("Edge: This edge is not symmetric. Therefore, I dont have anything else to do...");
	}
	qDebug()<<"Arrow created. We'll draw the edge now...";
	painter->drawPath(line);
}


/** Controls the width of the edge; is a function of edge weight
*/
int Edge::lineWidth(){
	if (weight()<0)
		return abs(weight());
else if (weight() == 0) {
		return 1; //?
	}
	else if (weight() > 0 && weight() <=5) {
		return weight();
	}
	else if (weight() > 5 && weight() <=10) {
		 return 6;
	}
	else if (weight() >10 && weight() <=20) {
		return 7;
	}
	else if (weight() >20 && weight() <=30) {
		return 8;
	}
	else return 9;
}




/** handles the events of a click on an edge*/
void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event) {  
	qDebug("Edge: pressEvent() emitting edgeClicked");
	graphicsWidget->edgeClicked(this);
	if ( event->button()==Qt::LeftButton ) {
		qDebug("Edge: edge pressEvent() left click > ");
	//	graphicsWidget->startNodeMovement(0);
	}
	if ( event->button()==Qt::RightButton ) {
		qDebug("Edge: Right-click on an edge,  at %i, %i", event->screenPos().x(), event->screenPos().y()); 
		graphicsWidget->openEdgeContextMenu();
	}
}





