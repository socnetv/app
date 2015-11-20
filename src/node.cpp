/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 1.9
 Written in Qt

                        node.cpp  -  description
                        -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
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


#include "node.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <QtGlobal>  //for QT_VERSION
#include <QDebug>
#include "graphicswidget.h"
#include "edge.h"
#include "nodelabel.h"
#include "nodenumber.h"
#include <math.h> //sqrt

Node::Node( GraphicsWidget* gw, int num, int size, 
			QString col, QString shape, bool numIn, 
			int ldist, int ndist, QPointF p 
			) : graphicsWidget (gw) 
{
	Q_UNUSED(p);
	graphicsWidget->scene()->addItem(this); //Without this nodes don't appear on the screen...

//ItemSendsGeometryChanges  introduced in Qt 4.6...
#if QT_VERSION >= 0x040600
	setFlags(ItemSendsGeometryChanges | ItemIsSelectable | ItemIsMovable);
    setCacheMode(QGraphicsItem::ItemCoordinateCache); //QT < 4.6 if a cache mode is set, nodes do not respond to hover events
//DeviceCoordinateCache
#else
	setFlags(ItemIsSelectable | ItemIsMovable ); //Without this, the node cannot move nor be selected ...
	setCacheMode(QGraphicsItem::NoCache); //QT < 4.6 if a cache mode is set, nodes do not respond to hover events
#endif
    setAcceptHoverEvents(true);

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
	qDebug()<< "Node: constructor: initial position at: "
			<< this->x()<<", "<<this->y()
			<< " Will move at: "<< p.x()<<", "<<p.y();;

} 



/** 
	Used by MW::slotChangeNodeColor
*/
void Node::setColor(QString str) {
	prepareGeometryChange();
	m_col=QColor(str);
    update();
}

/** 
	Used by MW::slotFindNode() 
*/
void Node::setColor(QColor color){
    prepareGeometryChange();
	m_col=color;
    m_col_str = m_col.name();
    update();
}


QString Node::color() {
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
void Node::setShape(const QString shape) {
	qDebug("Node: setShape()");
	prepareGeometryChange();
	m_shape=shape;
	qDebug ("Node: setShape(): node is at x=%f and y=%f", x(), y());
    update();
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

    //if the node is being dragged around, darken it!
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




void Node::setLabelText ( QString label) {
	prepareGeometryChange();
	m_label->setPlainText(label);
	m_hasLabel=true;
}


QString Node::labelText ( ) { 
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
					else { 		//move it inside node
						this->setSize(m_size+2); //increase size to display nicely the number  
						m_number -> setZValue(255);
						m_number->setPos( myPos.x() - m_size-2, myPos.y() - m_size-2 );
					}	 
			}
}




/** 
 *	Propagates the changes to connected elements, i.e. edges, numbers, etc. 
 *  Checks if the node is inside the scene.
 */
QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
	QPointF newPos = value.toPointF();

    switch (change) {
    case ItemPositionHasChanged :
    {
        foreach (Edge *edge, inEdgeList)  //Move each inEdge of this node
            edge->adjust();
        foreach (Edge *edge, outEdgeList) //Move each outEdge of this node
            edge->adjust();
        //Move its graphic number
        if ( m_hasNumber )
        {
            if (!m_isNumberInside) 	{ //move it outside
                m_number -> setZValue(254);
                m_number -> setPos( m_size+m_nd, 0);
            }
            else { 	//move it inside node
                m_number -> setZValue(255);
                m_number -> setPos(  - m_size, - m_size-3);
            }
        }
        if (m_hasLabel) {
            m_label->setPos( -2, m_ld+m_size);
        }
        if ( newPos.x() !=0 && newPos.y() != 0 ){
            graphicsWidget->nodeMoved(nodeNumber(), (int) newPos.x(), (int) newPos.y());
        }
        else qDebug()<<  "Node: ItemChange():  Not emitting nodeMoved. Node "
                      << nodeNumber()<<" is at 0,0";
        break;
    }
    case ItemEnabledHasChanged:{
        if (ItemEnabledHasChanged) {
            return 1;
        }
        else{
            return 0;
        }
    }
    case ItemVisibleHasChanged:
    {
        if (ItemVisibleHasChanged){
            return 1;
        }
        else{
            return 0;
        }
    }
    default:
    {
        break;
    }
    };
    return QGraphicsItem::itemChange(change, value);
}



/** handles the events of a click on a node */
void Node::mousePressEvent(QGraphicsSceneMouseEvent *event) {  
    qDebug() << "Node::mousePressEvent() "
                << " set selected and emitting nodeClicked";
    this->setSelected(true);
//	emit nodeClicked(this);
	graphicsWidget->nodeClicked(this);
	if ( event->button()==Qt::LeftButton ) {
        qDebug("Node::mousePressEvent() left click ");

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
	qDebug() << "Node:  addInLink() for "<<  m_num;
	inEdgeList.push_back( edge); 
	//qDebug ("Node:  %i inEdgeList has now %i edges", m_num, inEdgeList.size());
}


void Node::deleteInLink( Edge *link ){
	qDebug () << "Node:  deleteInLink for "<< m_num;
	//qDebug ("Node: %i inEdgeList has %i edges", m_num, inEdgeList.size());
    inEdgeList.remove( link);
	//qDebug ("Node:  %i inEdgeList has now %i edges", m_num, inEdgeList.size());
}



void Node::addOutLink( Edge *edge ) {
	qDebug("Node: addOutLink()");
	outEdgeList.push_back( edge); 
//	qDebug ("Node: outEdgeList has now %i edges", outEdgeList.size());
}



void Node::deleteOutLink(Edge *link){
	qDebug () << "Node: deleteOutLink() from " <<  m_num;
//	qDebug ("Node: %i outEdgeList has %i edges", m_num, outEdgeList.size());
    outEdgeList.remove( link);
//	qDebug ("Node: %i outEdgeList has now %i edges", m_num, outEdgeList.size());
}



void Node::addLabel (NodeLabel* gfxLabel  )  { 
	//qDebug("NODE: add label");
	m_label=gfxLabel ;
	m_hasLabel=true;
}


NodeLabel* Node::label(){
	return m_label;
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






void Node::addNumber (NodeNumber *gfxNum ) {
	 m_number=gfxNum ;
	 m_hasNumber=true;
}


void Node::deleteNumber( ){
	qDebug ("Node: deleteNumber ");
	m_number->hide();
	graphicsWidget->removeItem(m_number);	
}

 Node::~Node(){
    qDebug() << "\n\n\n *** ~Node() "<< nodeNumber();
    foreach (Edge *edge, inEdgeList) {
        qDebug("~Node: removing edges in inEdgeList");
        //edge->remove();
        graphicsWidget->removeItem(edge);
    }
    foreach (Edge *edge, outEdgeList) {
        qDebug("~Node: removing edges in outEdgeList");
        //edge->remove();
        graphicsWidget->removeItem(edge);
    }

    this->deleteNumber();
    this->deleteLabel();
    inEdgeList.clear();
    outEdgeList.clear();
    this->hide();
    graphicsWidget->removeItem(this);

 }

