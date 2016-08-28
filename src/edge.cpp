/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
 Written in Qt

                        edge.cpp  -  description
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

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QtDebug>		//used for qDebug messages
#include <cmath>

#include "graphicswidget.h"
#include "edge.h"
#include "node.h"
#include "edgeweight.h"


static const double Pi = 3.14159265;
static double TwoPi = 2.0 * Pi;


Edge::Edge(  GraphicsWidget *gw,
             Node *from,
             Node *to,
             const Qt::PenStyle &style,
             const float &weight,
             const int &nodeSize,
             const QString &color,
             const bool &reciprocal,
             const bool &drawArrows,
             const bool &bez ) : graphicsWidget(gw)
{

    qDebug("Edge: Edge()");
    Q_UNUSED(nodeSize);
    graphicsWidget->scene()->addItem(this);  //add edge to scene to be displayed

    from->addOutLink( this );	//adds this Edge to sourceNode
    to->addInLink( this );		//adds this Edge to targetNode

    source=from;			//saves the sourceNode
    target=to;			//Saves the targetNode
    m_style = style;
    m_color=color;
    m_drawArrows=drawArrows;
    m_reciprocal=reciprocal;
    m_startOffset=source->size();  //used to offset edge from the centre of node
    m_endOffset=target->size();  //used to offset edge from the centre of node
    //	qDebug("Edge() m_startOffset %i",(int) m_startOffset);
    //	qDebug("Edge() m_endOffset %i",(int) m_endOffset);

    m_arrowSize=4;		//controls the width of the edge arrow

    eFrom = source->nodeNumber() ;
    eTo = target->nodeNumber() ;
    m_weight = weight ;
    m_Bezier = bez;
    setAcceptHoverEvents(true);
//    setFlags(QGraphicsItem::ItemIsSelectable);
    adjust();
}



void Edge::showArrows(bool drawArrows){
    prepareGeometryChange();
    m_drawArrows=drawArrows;
}



void Edge::removeRefs(){
    //FIXME Need to disconnect signals from node to this "erased" edge
    qDebug("Edge: removeRefs()");
    source->deleteOutLink(this);
    target->deleteInLink(this);
}


void Edge::setColor( const QString &str) {
    m_color=str;
    prepareGeometryChange();
}



QString Edge::color() const{
    return m_color;
}

/**
 * @brief Called from Graph::saveGraphToPajekFormat()
 * @return
 */
QString Edge::colorToPajek() {
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}

/**
 * @brief Called from MW when user wants to change an edge's weight.
    Updates both the width and the EdgeWeight
 * @param w
 */
void Edge::setWeight(const float &w) {
    m_weight = w;
    foreach (EdgeWeight *wgt, weightList) 		///update the edgeWeight of this edge as well.
        wgt->setPlainText (QString::number(w));

}

float Edge::weight() const {
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


/**
 * @brief Called from EdgeWeight objects to 'connect' them to this edge.
 * @param canvasWeight
 */
void Edge::addWeight (EdgeWeight* canvasWeight  )  {
    weightList.push_back( canvasWeight) ;
}


void Edge::clearWeightList(){
    foreach (EdgeWeight *wgt, weightList) 		//Delete this weight
        wgt->deleteLater();

    weightList.clear();
}


/**
 * @brief Leaves some empty space (offset) from node -
 * make the edge weight appear on the centre of the edge
 */
void Edge::adjust(){
    // qDebug("Edge: adjust()");
    if (!source || !target)
        return;
    //QLineF line(mapFromItem(source, 0, 0), mapFromItem(target, 0, 0));
    QLineF line(source->x(), source->y(), target->x(), target->y());
    QPointF edgeOffset;
    line_length = line.length();
    line_dx = line.dx();
    line_dy = line.dy();
    if (source!=target) {
        edgeOffset = QPointF(
                    (line_dx * m_endOffset) / line_length,
                    (line_dy * m_endOffset) / line_length);
    }
    else edgeOffset = QPointF(0, 0);

    prepareGeometryChange();

    sourcePoint = line.p1() + edgeOffset;
    targetPoint = line.p2() - edgeOffset;
//    qDebug()<<"----Edge: adjust()  ("<< sourcePoint.x()<< ","<<sourcePoint.y() << ") -> ("
//                                   << targetPoint.x()<< " "<<targetPoint.y() << ")";
    foreach (EdgeWeight *wgt, weightList) 		//Move the weight of this edge
        wgt->setPos( (source->x()+target->x())/2.0, (source->y()+target->y())/2.0 );



    //Define the path upon which we' ll draw the line
    //QPainterPath line(sourcePoint);
    m_path = new QPainterPath(sourcePoint);

    //Construct the path
    if (source!=target) {
        if ( !m_Bezier){
            //   qDebug()<< "*** Edge::paint(). Constructing a line";
            m_path->lineTo(targetPoint);
        }
        else {
            qDebug() << "*** Edge::paint(). Constructing a bezier curve";
        }
    }
    else { //self-link
        QPointF c1 = QPointF( targetPoint.x() -30,  targetPoint.y() -30 );
        QPointF c2 = QPointF( targetPoint.x() +30,  targetPoint.y() -30 );
//        qDebug()<< "*** Edge::paint(). Constructing a bezier self curve c1 "
//                <<c1.x()<<","<<c1.y()  << " and c2 "<<c2.x()<<","<<c2.y();
        m_path->cubicTo( c1, c2, targetPoint);
    }

    //Draw the arrows only if we have different nodes.
    if (m_drawArrows && source!=target) {
        angle = 0;
//        line_length = m_path->length();
//        line_dx = targetPoint.x()-sourcePoint.x();
//        line_dy = targetPoint.y()-sourcePoint.y();
        if ( line_length > 0 )
            angle = ::acos( line_dx / line_length );
        //		qDebug() << " acos() " << ::acos( line_dx  / line_length ) ;

        if ( line_dy  >= 0)
            angle = TwoPi - angle;

        if (line_length > 10) {
//            qDebug() << "*** Edge::paint(). Constructing arrows. "
//                        "First Arrow at target node"
//                     << "target-source: " << line_dx
//                     << " length: " << line_length
//                     << " angle: "<< angle;

            QPointF destArrowP1 = targetPoint + QPointF(sin(angle - Pi / 3) * m_arrowSize,
                                                        cos(angle - Pi / 3) * m_arrowSize);
            QPointF destArrowP2 = targetPoint + QPointF(sin(angle - Pi + Pi / 3) * m_arrowSize,
                                                        cos(angle - Pi + Pi / 3) * m_arrowSize);
//            qDebug() << "*** Edge::paint() destArrowP1 "
//                     <<  destArrowP1.x() << "," << destArrowP1.y()
//                      << "  destArrowP2 " <<  destArrowP2.x() << "," << destArrowP2.y();

            m_path->addPolygon ( QPolygonF()
                                 << targetPoint
                                 << destArrowP1
                                 << destArrowP2
                                 << targetPoint
                                 );

            if (m_reciprocal ) {
    //            qDebug() << "**** Edge::paint() This edge is SYMMETRIC! "
    //                     << " So, we need to create Arrow at src node as well";
                QPointF srcArrowP1 = sourcePoint + QPointF(sin(angle +Pi / 3) * m_arrowSize,
                                                           cos(angle +Pi / 3) * m_arrowSize);
                QPointF srcArrowP2 = sourcePoint + QPointF(sin(angle +Pi - Pi  / 3) * m_arrowSize,
                                                           cos(angle +Pi - Pi / 3) * m_arrowSize);

                m_path->addPolygon ( QPolygonF()
                                     << sourcePoint
                                     << srcArrowP1
                                     << srcArrowP2
                                     <<sourcePoint
                                     );

            }
            else {
                // qDebug() << "*** Edge::paint() Not symmetric edge. Finish";
            }
        }

    }
    else {
//        qDebug()<< "*** Edge::paint(). This edge is self-link - CONTINUE!";
    }


}



/**
 * @brief  Returns the shape of this edge as a QPainterPath in local coordinates.
 * The shape is used for many things, including collision detection, hit tests,
 * and for the QGraphicsScene::items() functions.
 * The default implementation calls boundingRect() to return a simple rectangular shape,
 * but we reimplement it to return a more accurate shape for non-rectangular items.
 * @return QPainterPath
 */
QPainterPath Edge::shape () const {
    //qDebug()<<"Edge::shape()";		//too many debug messages...
//    QPainterPath path;
//    qreal extra = ( width() + m_arrowSize);
//    QLineF line(sourcePoint, targetPoint);
//    QPolygonF poly;
//    line.translate(extra,extra);
//    poly.push_back(line.p1());
//    poly.push_back(line.p2());
//    line.translate(-extra,-extra);
//    poly.push_back(line.p1());
//    poly.push_back(line.p2());
//    path.addPolygon(poly);
//    path.closeSubpath();
//    return path;
    return *m_path;
} 


/**
 * @brief Defines the outer bounds of the edge as a rectangle;
 * All painting will be restricted to inside the edge's bounding rect.
 * Qt uses this bounding rect to determine whether the edge requires redrawing.
 * @return
 */
QRectF Edge::boundingRect() const {
    if (!source || !target)
        return QRectF();
//    qreal penWidth = 1;
//    qreal extra = ( penWidth + m_arrowSize) / 2.0;
//    QRectF a = QRectF (
//                sourcePoint,
//                QSizeF(
//                    targetPoint.x() - sourcePoint.x(), targetPoint.y() - sourcePoint.y())
//                ).normalized().adjusted(-extra, -extra, extra, extra);
//    //qDebug()<<"Edge::boundingRect() extra = " << extra << "QSizeF width "<< a.width() << " QSizeF height "<< a.height();
//    if (source==target) {		//self-edge has different bounding rect.
//        return QRectF (
//                    sourcePoint-QPointF(30,30),
//                    QSizeF(60,30)).normalized().adjusted(-extra, -extra, extra, extra);
//    }
//    return a;
    return m_path->controlPointRect();
}



void Edge::makeReciprocal(){
    qDebug("Edge::makeReciprocal()");
    prepareGeometryChange();
    m_reciprocal= true;
}



void Edge::unmakeReciprocal(){
    qDebug("Edge::unmakeReciprocal()");
    prepareGeometryChange();
    m_reciprocal= false;
}

bool Edge::isReciprocal() {
    return m_reciprocal;
}


void Edge::setStyle( const Qt::PenStyle  &style ) {
    m_style = style;
}

Qt::PenStyle Edge::style() const{
    return m_style;

}

QPen Edge::pen() const {
    if (m_weight < 0 ){
        return  QPen(QColor("red"), width(), Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
    }
    return QPen(QColor(m_color), width(), style(), Qt::RoundCap, Qt::RoundJoin);
}



void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    if (!source || !target)
        return;
    Q_UNUSED(option);//painter->setClipRect( option->exposedRect );
    // qDebug() <<"@@@ Edge::paint()";

    //    qDebug()<<endl <<"@@@ Edge::paint() edge from "<< sourceNodeNumber()
    //           << " at (" <<(sourceNode())->x() <<","<< (sourceNode())->y()
    //           << ") to node "<<targetNodeNumber() << " at ("<<(targetNode())->x()
    //           <<","<< (targetNode())->y() << ") of weight "<< m_weight;

     painter->setPen( pen() );
     // painter->setBrush(QColor(m_color));

     //if the edge is being dragged around, darken it!
     if (option->state & QStyle::State_Selected) {
         //qDebug()<< " node : selected ";
         painter->setPen(
                     QPen(QColor("m_red"), width(), style(), Qt::RoundCap, Qt::RoundJoin)
                     );
     }
     else if (option->state & QStyle::State_MouseOver) {
         //qDebug()<< " node : mouse over";
         painter->setPen(
                     QPen(QColor("red"), width()+5, style(), Qt::RoundCap, Qt::RoundJoin)
                     );
         setZValue(255);
     }
    //	qDebug()<< "### Edge::paint(). DrawPath now....";
    painter->drawPath(*m_path);
}


/** 
    Controls the width of the edge; is a function of edge weight
*/
float Edge::width() const{
    // qDebug()<< "Edge::width() will return "<< fabs(m_weight);
    if ( fabs(m_weight) > 1  )
        return  1  + fabs(m_weight)/10;
    return 1;	//	Default, if  m_weight in (-1, 1) space
}



/**
 * @brief Edge::highlight
 * @param flag
 */
void Edge::highlight(const bool &flag) {
    qDebug()<< "highlight " << flag;
    if (flag) {
        m_tempColor = m_color;
        m_tempweight = m_weight;
        setColor("red");
        setWeight (m_weight+5);
    }
    else setColor(m_tempColor);
    setWeight (m_tempweight);
}

/** handles the events of a click on an edge*/
void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event) {  
    qDebug("Edge: pressEvent() emitting edgeClicked");
    graphicsWidget->edgeClicked(this);
    if ( event->button()==Qt::LeftButton ) {
        qDebug() << "Edge: edge pressEvent() left click > ";
        //	graphicsWidget->startNodeMovement(0);
    }
    if ( event->button()==Qt::RightButton ) {
        qDebug("Edge: Right-click on an edge,  at %i, %i", event->screenPos().x(), event->screenPos().y());
        graphicsWidget->openEdgeContextMenu();
    }
}




Edge::~Edge(){
    qDebug() << "\n\n\n *** ~EDGE() " << sourceNodeNumber()<< "->" << targetNodeNumber();
    removeRefs();
    weightList.clear();
    this->hide();

}

