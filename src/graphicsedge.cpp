/**
 * @file graphicsedge.cpp
 * @brief Implements the GraphicsEdge class, which visualizes edges in the network graph.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#include "graphicsedge.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QtDebug>                  //used for qDebug messages
#include <cmath>

#include "global.h"
#include "graphicswidget.h"
#include "graphicsnode.h"
#include "graphicsedgeweight.h"
#include "graphicsedgelabel.h"
// #include <QPaintEngine>

SOCNETV_USE_NAMESPACE


GraphicsEdge::GraphicsEdge(GraphicsWidget *gw,
                           GraphicsNode *from,
                           GraphicsNode *to,
                           const qreal &weight,
                           const QString &label,
                           const QString &color,
                           const Qt::PenStyle &style,
                           const int &type,
                           const bool &drawArrows,
                           const bool &bezier,
                           const bool &weightNumbers, const bool &highlighting) : graphicsWidget(gw)
{

    graphicsWidget->scene()->addItem(this);  //add edge to scene to be displayed

    from->addOutEdge( this );       // adds this new edge to sourceNode
    to->addInEdge( this );          // adds this new edge to targetNode

    source=from;                    // saves the source node
    target=to;                      // saves the target node

    m_style = style;
    m_state = EDGE_STATE_REGULAR ;

    m_color=QColor(color);                  // saves the color of the edge

    m_drawArrows=drawArrows;        // controls if edge will have arrows or not

    m_edgeDirType=type;

    m_minOffsetFromNode = 6;           // controls the minimum offset from source/target node center

    m_offsetFromSourceNode=source->size()+m_minOffsetFromNode;  // offsets edge from the centre of source node
    m_offsetFromTargetNode=target->size()+m_minOffsetFromNode;  // offsets edge from the centre of target node

    m_arrowSize=4;                   // controls the width of the edge arrow

    m_weight = weight ;              // saves the weight/value of this edge

    if ( fabs(m_weight) > 1  )  {
        m_width = 1+log ( 1+ log(fabs(m_weight) )) ;
    }
    else {
        m_width = fabs(m_weight) ;
    }

    m_Bezier = bezier;               // controls if it will appear as line or curve

    m_label = label;
    m_drawLabel = !m_label.isEmpty();

    m_drawWeightNumber = weightNumbers;     // controls if weight number will be shown

//    qDebug()<< "Constructed graphics edge:"
//            << source->nodeNumber()
//            << "->"
//            << target->nodeNumber()
//            <<" = " << m_weight
//           <<" label " << m_label
//          <<" edgeType " << m_edgeDirType;

    if (m_drawWeightNumber) {
        addWeightNumber();
    }
    if (m_drawLabel)
        addLabel();

    m_hoverHighlighting = highlighting;

    setAcceptHoverEvents(true);

    setFlags(QGraphicsItem::ItemIsSelectable);

    //Edges have lower z than nodes. Nodes always appear above edges.
    setZValue(ZValueEdge);

    setBoundingRegionGranularity(0);

    // Leave the default cache option (NoCache)
    // setCacheMode(QGraphicsItem::NoCache);

    adjust();
}



/**
 * @brief Toggles displaying edge arrow
 * @param drawArrows
 */
void GraphicsEdge::showArrows(const bool &drawArrows){
    prepareGeometryChange();
    m_drawArrows=drawArrows;
    adjust();
}



/**
 * @brief Removes any references to this edge in source and target nodes.
 */
void GraphicsEdge::removeRefs(){
//    qDebug() << "Removing edge refs...";
    source->removeOutEdge(this);
    target->removeInEdge(this);
}


/**
 * @brief Sets the edge color
 * @param str
 */
void GraphicsEdge::setColor( const QString &str) {
    m_color=QColor(str);
    prepareGeometryChange();
}


/**
 * @brief Returns the edge QColor.
 * @return
 */
QColor GraphicsEdge::color() const{
    return m_color;
}


/**
 * @brief Returns the edge color in pajek-accepted format
 *
 * Called from Graph
 *
 * @return
 */
QString GraphicsEdge::colorToPajek() {
    QString m_colorStr = m_color.name();
    if (m_colorStr.startsWith("#")) {
        return  ("RGB"+m_colorStr.right( m_colorStr.size()-1 )).toUpper()  ;
    }
    return m_colorStr;
}


/**
 * @brief Changes the edge weight - Updates both the width and the weightNumber
 *
 * Called from MW when user wants to change an edge's weight.

 * @param w
 */
void GraphicsEdge::setWeight(const qreal &w) {
//    qDebug() << "Setting edge weight:" << w;
    prepareGeometryChange();
    m_weight = w;
    if ( fabs(m_weight) > 1  )  {
        m_width = 1+log ( 1+ log(fabs(m_weight) )) ;
    }
    else {
        m_width = fabs(m_weight) ;
    }
    if (m_drawWeightNumber)
        weightNumber->setPlainText (QString::number(w));
}


/**
 * @brief Returns the weight/value of this edge
 * @return
 */
qreal GraphicsEdge::weight() const {
    return m_weight;
}


/**
 * @brief Adds a graphics edge weight to this edge
 */
void GraphicsEdge::addWeightNumber (){
    // create edge weight item
    double x = -20 + ( source->x() + target->x() ) / 2.0;
    double y = -20 + ( source->y() + target->y() ) / 2.0;
    weightNumber = new  GraphicsEdgeWeight (this, 7, QString::number(m_weight) );
    weightNumber->setPos(x,y);
    weightNumber->setDefaultTextColor (m_color);
    m_drawWeightNumber = true;
}


/**
 * @brief Toggles visibility of weight numbers
 * @param toggle
 */
void GraphicsEdge::setWeightNumberVisibility (const bool &toggle) {
    if (m_drawWeightNumber) {
        if (toggle)
            weightNumber->show();
        else
            weightNumber->hide();
    }
    else{
        if (toggle)
            addWeightNumber();
    }

}




/**
 * @brief Changes the edge label.
 *
 * Called from MW when user wants to change an edge's label
 *
 * @param label
 */
void GraphicsEdge::setLabel(const QString &label) {
//    qDebug() << "Setting graphics edge label:" << label;
    prepareGeometryChange();
    m_label = label;
    if (m_drawLabel)
        edgeLabel->setPlainText (m_label);
}


/**
 * @brief Returns the edge label text
 * @return QString
 */
QString GraphicsEdge::label() const {
    return m_label;
}


/**
 * @brief Adds a graphics edge label to this edge
 */
void GraphicsEdge::addLabel (){
    // create edge label item
    double x =  5+ ( source->x() + target->x() ) / 2.0;
    double y =  5+ ( source->y() + target->y() ) / 2.0;
    edgeLabel = new  GraphicsEdgeLabel (this, 7, m_label );
    edgeLabel->setPos(x,y);
    edgeLabel->setDefaultTextColor (m_color);
    m_drawLabel = true;
}

/**
 * @brief Toggles the graphics edge label visibility
 * @param toggle
 */
void GraphicsEdge::setLabelVisibility (const bool &toggle) {
    if (m_drawLabel) {
        if (toggle)
            edgeLabel->show();
        else
            edgeLabel->hide();
    }
    else{
        if (toggle)
            addLabel();
    }

}


/**
 * @brief Returns the source node of this graphics edge
 * @return
 */
GraphicsNode *GraphicsEdge::sourceNode() const {
    return source;
}


/**
 * @brief Sets the source node of this graphics edge
 * @param node
 */
void GraphicsEdge::setSourceNode(GraphicsNode *node) {
    source = node;
    adjust();
}


/**
 * @brief Called from graphicsNode to update edge offset from source node (i.e. when node size changes)
 * @param offset
 */
void GraphicsEdge::setSourceNodeSize(const int &size){
    m_offsetFromSourceNode=size + m_minOffsetFromNode;
    adjust();
}


/**
 * Returns the source node number
 * @return int
 */
int GraphicsEdge::sourceNodeNumber () {
    return source->nodeNumber();
}


/**
 * @brief Returns the target node.
 * @return
 */
GraphicsNode *GraphicsEdge::targetNode() const {
    return target;
}


/**
 * @brief Sets the target node.s
 * @param node
 */
void GraphicsEdge::setTargetNode(GraphicsNode *node){
    target = node;
    adjust();
}

/**
 * @brief Called from graphicsNode to update edge offset from target node (i.e. when node size changes)
 * @param offset
 */
void GraphicsEdge::setTargetNodeSize(const int & size){
    m_offsetFromTargetNode=size + m_minOffsetFromNode;
    adjust();
}

/**
 * Returns the target node number
 * @return int
 */
int GraphicsEdge::targetNodeNumber() {
    return target->nodeNumber();
}


/**
* @brief Updates Minimum Offset From Node and calls adjust to update the edge
* @param offset
*/
void GraphicsEdge::setMinimumOffsetFromNode(const int &offset) {
    m_minOffsetFromNode = offset;
    m_offsetFromTargetNode = target->size() + m_minOffsetFromNode;
    adjust();
}


/**
 * @brief Returns the horizontal difference between target and source nodes.
 * @return
 */
qreal GraphicsEdge::dx() const
{
    return target->x() - source->x();
}

/**
 * @brief Returns the vertical difference between target and source nodes.
 * @return
 */
qreal GraphicsEdge::dy() const
{
    return target->y() - source->y();
}


/**
 * @brief Returns the euclidean length of the edge
 * @return
 */
qreal GraphicsEdge::length() const
{ return sqrt ( dx() * dx()  + dy() * dy() ) ; }




/**
 * @brief Leaves some empty space (offset) from node -
 * make the edge weight appear on the centre of the edge
 */
void GraphicsEdge::adjust(){
    // qDebug() << "GraphicsEdge::adjust()";
    if (!source || !target) {
        return;
    }

    //    QLineF line(source->x(), source->y(), target->x(), target->y());
    //QPointF edgeOffset;

    //line_length = line.length();
    //    line_dx = line.dx();
    //    line_dy = line.dy();

    line_length = length();
    line_dx = dx();
    line_dy = dy();

    if (source!=target) {
        edgeOffset = QPointF(
                    (line_dx * m_offsetFromTargetNode) / line_length,
                    (line_dy * m_offsetFromTargetNode) / line_length);
    }
    else edgeOffset = QPointF(0, 0);

    prepareGeometryChange();

    //    sourcePoint = line.p1() + edgeOffset ;
    //    targetPoint = line.p2() - edgeOffset ;

    sourcePoint = source->pos() + edgeOffset ;
    targetPoint = target->pos() - edgeOffset ;


    if (m_drawWeightNumber) {
        weightNumber->setPos(
                    -20 + (source->x()+target->x())/2.0,
                    -20+ (source->y()+target->y())/2.0 );
    }
    if (m_drawLabel) {
        edgeLabel->setPos(
                    5+ (source->x()+target->x())/2.0,
                    5+ (source->y()+target->y())/2.0 );
    }

    //Define the path upon which we' ll draw the line
    QPainterPath path(sourcePoint);

    //Construct the path
    if (source!=target) {
        if ( !m_Bezier){
            //   qDebug()<< "*** GraphicsEdge::paint(). Constructing a line";
            path.lineTo(targetPoint);
        }
        else {
            qDebug() << "*** GraphicsEdge::paint(). Constructing a bezier curve";
            QPointF c = QPointF( targetPoint.x() - sourcePoint.x(),
                                 targetPoint.y() - targetPoint.y());
            path.cubicTo( sourcePoint, c, targetPoint);
        }
    }
    else { //self-link
        QPointF c1 = QPointF( targetPoint.x() -30,  targetPoint.y() -30 );
        QPointF c2 = QPointF( targetPoint.x() +30,  targetPoint.y() -30 );
        //        qDebug()<< "*** GraphicsEdge::paint(). Constructing a bezier self curve c1 "
        //                <<c1.x()<<","<<c1.y()  << " and c2 "<<c2.x()<<","<<c2.y();
        path.cubicTo( c1, c2, targetPoint);
    }

    //Draw the arrows only if we have different nodes
    //and the nodes are enough far apart from each other
    if (m_drawArrows && source!=target && line_length > 10) {

        angle = 0;

        if ( line_length > 0 )
            angle = ::acos( line_dx / line_length );
        //		qDebug() << " acos() " << ::acos( line_dx  / line_length ) ;

        if ( line_dy  >= 0)
            angle = M_PI_X_2 - angle;


        //            qDebug() << "*** GraphicsEdge::paint(). Constructing arrows. "
        //                        "First Arrow at target node"
        //                     << "target-source: " << line_dx
        //                     << " length: " << line_length
        //                     << " angle: "<< angle;

        QPointF destArrowP1 = targetPoint + QPointF(sin(angle - M_PI_3) * m_arrowSize,
                                                    cos(angle - M_PI_3) * m_arrowSize);
        QPointF destArrowP2 = targetPoint + QPointF(sin(angle - M_PI + M_PI_3) * m_arrowSize,
                                                    cos(angle - M_PI + M_PI_3) * m_arrowSize);
        //            qDebug() << "*** GraphicsEdge::paint() destArrowP1 "
        //                     <<  destArrowP1.x() << "," << destArrowP1.y()
        //                      << "  destArrowP2 " <<  destArrowP2.x() << "," << destArrowP2.y();

        path.addPolygon ( QPolygonF()
                          << targetPoint
                          << destArrowP1
                          << destArrowP2
                          << targetPoint
                          );

        if (m_edgeDirType == EdgeType::Undirected || m_edgeDirType == EdgeType::Reciprocated ) {
            //            qDebug() << "**** GraphicsEdge::paint() This edge is SYMMETRIC! "
            //                     << " So, we need to create Arrow at src node as well";
            QPointF srcArrowP1 = sourcePoint + QPointF(sin(angle +M_PI_3) * m_arrowSize,
                                                       cos(angle +M_PI_3) * m_arrowSize);
            QPointF srcArrowP2 = sourcePoint + QPointF(sin(angle +M_PI - M_PI_3) * m_arrowSize,
                                                       cos(angle +M_PI - M_PI_3) * m_arrowSize);

            path.addPolygon ( QPolygonF()
                              << sourcePoint
                              << srcArrowP1
                              << srcArrowP2
                              <<sourcePoint
                              );

        }
        else {
            // qDebug() << "*** GraphicsEdge::paint() Not symmetric edge. Finish";
        }


    }
    else {
        //        qDebug()<< "*** GraphicsEdge::paint(). This edge is self-link - CONTINUE!";
    }


    m_path =  path;
}



/**
 * @brief  Returns the shape of this edge as a QPainterPath in local coordinates.
 * The shape is used for many things, including collision detection, hit tests,
 * and for the QGraphicsScene::items() functions.
 * The default implementation calls boundingRect() to return a simple rectangular shape,
 * but we reimplement it to return a more accurate shape for non-rectangular items.
 * @return QPainterPath
 */
QPainterPath GraphicsEdge::shape () const {
    //qDebug()<<"GraphicsEdge::shape()";		//too many debug messages...
    //    QPainterPath m_path_shape = m_path;
    //    m_path_shape.addPath(m_path.translated(1,1));
    //    m_path_shape.addPath(m_path.translated(-1,-1));
    //    return m_path_shape;
    return m_path;
} 


/**
 * @brief Defines the outer bounds of the edge as a rectangle;
 * All painting will be restricted to inside the edge's bounding rect.
 * Qt uses this bounding rect to determine whether the edge requires redrawing.
 * @return
 */
QRectF GraphicsEdge::boundingRect() const {
    //qDebug()<<"GraphicsEdge::boundingRect()";		//too many debug messages...
    if (!source || !target)
        return QRectF();
    return m_path.controlPointRect();
}


/**
 * @brief Changes the direction type of edge A->B
  */
void GraphicsEdge::setDirectionType(const int &dirType){
//    qDebug()<< "Edge"
//            << source->nodeNumber()
//            << "->"
//            << target->nodeNumber()
//            << "new direction type"
//            << dirType;
    prepareGeometryChange();
    m_edgeDirType = dirType;
    m_drawArrows = true;
    if (m_edgeDirType==EdgeType::Undirected) {
        m_drawArrows = false;
    }
    adjust();
}


/**
 * @brief returns the direction type of this edge
 * @return
 */
int GraphicsEdge::directionType() {
    return m_edgeDirType ;
}



/**
 * @brief Sets the PenStyle of this edge
 * @param style
 */
void GraphicsEdge::setStyle( const Qt::PenStyle  &style ) {
    m_style = style;
}


/**
 * @brief Returns the PenStyle of this edge
 * @return
 */
Qt::PenStyle GraphicsEdge::style() const{
    return m_style;

}

/**
 * @brief Returns the QPen for this edge -- the pen changes when the edge state changes/
 * @return
 */
QPen GraphicsEdge::pen() const {
    //qDebug() << "GraphicsEdge::pen() - returning pen "  ;
    switch (m_state) {
    case EDGE_STATE_REGULAR:
        //qDebug() << "GraphicsEdge::pen() - returning pen for state REGULAR"  ;
        if (m_weight < 0 ){
            return  QPen(m_color, m_width, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        }
        return QPen(m_color, m_width, m_style, Qt::RoundCap, Qt::RoundJoin);
        break;
    case EDGE_STATE_HIGHLIGHT: // selected
        //qDebug() << "GraphicsEdge::pen() - returning pen for state HIGHLIGHTED"  ;
        return QPen( QColor("red"), m_width, m_style, Qt::RoundCap, Qt::RoundJoin);
    case EDGE_STATE_HOVER: // hover
        //qDebug() << "GraphicsEdge::pen() - returning pen for state HOVER"  ;
        return QPen(QColor("red"), m_width+1, m_style, Qt::RoundCap, Qt::RoundJoin);
    default:
        //qDebug() << "GraphicsEdge::pen() - returning pen for state DEFAULT"  ;
        return QPen(m_color, m_width, m_style, Qt::RoundCap, Qt::RoundJoin);
    }

}


/**
 * @brief Sets the edge state
 * @param state
 */
void GraphicsEdge::setState(const int &state) {
    //NOTE: DO NOT USE HERE: prepareGeometryChange()
    m_state=state;
}


/**
 * @brief Pains the edge
 *
 * @param painter
 * @param option
 */
void GraphicsEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    if (!source || !target)
        return;

    //qDebug() <<"@@@ GraphicsEdge::paint() on" << painter->paintEngine()->type();
    //painter->setClipRect();

    //if the edge is being dragged around, darken it!
    if (option->state & QStyle::State_Selected) {
        //setZValue(ZValueEdgeHighlighted);
        setState(EDGE_STATE_HOVER);
    }
    else if (option->state & QStyle::State_MouseOver) {
        if (m_hoverHighlighting) {
            setZValue(ZValueEdgeHighlighted);
            setState(EDGE_STATE_HOVER);
        }
    }
    else if (m_state==EDGE_STATE_HIGHLIGHT){
        if (m_hoverHighlighting) {
            setZValue(ZValueEdgeHighlighted);
            setState(EDGE_STATE_HIGHLIGHT);
        }
    }
    else {
        setZValue(ZValueEdge);
        setState(EDGE_STATE_REGULAR);
    }
    // set painter pen to correct edge pen
    painter->setPen(pen());

    // set painter brush to paint inside the arrow
    painter->setBrush( m_color );

    painter->drawPath(m_path);
}


/**
 * @brief Called when the edge changes, i.e. moves, becomes disabled or changes its visibility
 * @param change
 * @param value
 * @return
 */
QVariant GraphicsEdge::itemChange(GraphicsItemChange change, const QVariant &value){
    switch (change) {
    case ItemPositionHasChanged: {
        break;
    }
    case ItemEnabledHasChanged: {
        break;
    }
    case ItemSelectedHasChanged: {
        if (value.toBool()) {
            setZValue(ZValueEdgeHighlighted);
            setHighlighted(true);
            //source->setSelected(true);
            //target->setSelected(true);
        }
        else{
            setZValue(ZValueEdge);
            setHighlighted(false);
            //source->setSelected(false);
            //target->setSelected(false);
        }
        break;
    }
    case ItemVisibleHasChanged: {
        break;
    }
    default: {
        break;
    }
    };
    return QGraphicsItem::itemChange(change, value);
}



/**
 * @brief Returns the width of the edge as a function of edge weight
 * @return
 */
qreal GraphicsEdge::width() const{
    return m_width;
}



/**
 * @brief Toggles the highlighted state of the the edge, if highlighting is allowed.
 *
 * Called from GraphicsNode when the user hovers over the node.
 *
 * @param flag
 */
void GraphicsEdge::setHighlighted(const bool &flag) {
    //qDebug()<< "GraphicsEdge::setHighlighted() - " << flag;
    if (flag && m_hoverHighlighting) {
        prepareGeometryChange();
        setState(EDGE_STATE_HIGHLIGHT);
    }
    else {
        prepareGeometryChange();
        setState(EDGE_STATE_REGULAR);
    }
}


/**
 * @brief Toggles edge highlighting on or off
 *
 * If enabled, the edge can be highlighted.
 *
 * @param toggle
 */
void GraphicsEdge::setHighlighting(const bool &toggle) {
    m_hoverHighlighting = toggle;

}




///**
// * @brief handles the events of a click on an edge
// * @param event
// */
//void GraphicsEdge::mousePressEvent(QGraphicsSceneMouseEvent *e) {
//    qDebug() << "GraphicsEdge::mousePressEvent() - click on an edge ";
//    //setClicked();
//    QGraphicsItem::mousePressEvent(e);
//}




GraphicsEdge::~GraphicsEdge(){
//    qDebug() << "self-destructing edge:"
//             << sourceNodeNumber()<< "->" << targetNodeNumber()
//             << "will remove refs first...";

    removeRefs();

//    qDebug() << "removing edge weight number, if any...";
    if (m_drawWeightNumber)
        graphicsWidget->removeItem(weightNumber);

//    qDebug() << "removing edge label, if any...";
    if (m_drawLabel)
        graphicsWidget->removeItem(edgeLabel);

    this->hide();

//    qDebug() << "calling GW removeItem for this edge";
    graphicsWidget->removeItem(this);


}

