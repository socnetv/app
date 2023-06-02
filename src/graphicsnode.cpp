/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                        graphicsnode.cpp  -  description
                        -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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

#include "graphicsnode.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <QtGlobal>
#include <QDebug>
#include "graphicswidget.h"
#include "graphicsedge.h"
#include "graphicsnodelabel.h"
#include "graphicsnodenumber.h"



/**
 * @brief Constructs a new node object (which is a graphics item)
 *
 * @param gw
 * @param num
 * @param size
 * @param color
 * @param shape
 * @param iconPath
 * @param showNumbers
 * @param numbersInside
 * @param numberColor
 * @param numberSize
 * @param numDistance
 * @param showLabels
 * @param label
 * @param labelColor
 * @param labelSize
 * @param labelDistance
 * @param edgeHighlighting
 * @param p
 */
GraphicsNode::GraphicsNode ( GraphicsWidget* gw,
                             const int &num,
                             const int &size,
                             const QString &color,
                             const QString &shape,
                             const QString &iconPath,
                             const bool &showNumbers,
                             const bool &numbersInside,
                             const QString &numberColor,
                             const int &numberSize,
                             const int &numDistance,
                             const bool &showLabels,
                             const QString &label,
                             const QString &labelColor,
                             const int &labelSize,
                             const int &labelDistance,
                             const bool &edgeHighlighting,
                             QPointF p
                             ) : graphicsWidget (gw)
{

//    qDebug()<<"Constructing new node:"<< num;

    graphicsWidget->scene()->addItem(this); // Without this nodes don't appear on the screen...

    setFlags(ItemSendsGeometryChanges | ItemIsSelectable | ItemIsMovable );

    // Leave the default cache option (NoCache)
    // setCacheMode(QGraphicsItem::NoCache);

    setAcceptHoverEvents(true);

    m_num=num;
    m_size=size;

    m_shape=shape;
    m_iconPath = iconPath;

    m_col_str=color;
    m_col_orig=m_col=QColor(color);

    m_hasNumber=showNumbers;
    m_hasNumberInside = numbersInside;
    m_numSize = numberSize;
    m_numColor = numberColor;
    m_numberDistance=numDistance;

    m_hasLabel=showLabels;
    m_labelText = label;
    m_labelSize = labelSize;
    m_labelColor = labelColor;

    m_labelDistance=labelDistance;

    if (m_hasLabel) {
        addLabel();
    }

    if (!m_hasNumberInside && m_hasNumber) {
        addNumber();
    }

    m_edgeHighLighting = edgeHighlighting;

    setZValue(ZValueNode);
    setShape(m_shape,m_iconPath);

    setPos(p);

    qDebug()<< "Constructed new node at pos:"  << x()<<","<<y()
            << "m_numSize" << m_numSize;

} 



/**
 * @brief Changes the color of the node
 *
 * @param colorStr
 */
void GraphicsNode::setColor(const QString &colorStr) {
    qDebug()<< "Changing the node color to:" << colorStr;
    prepareGeometryChange();
    m_col=QColor(colorStr);
    update();
}


/**
 * @brief Changes the color of the node (overloaded)
 *
 * Also used when the user searches for a node
 *
 * @param color
 */
void GraphicsNode::setColor(QColor color){
    qDebug()<< "Changing the node color to:" << color;
    prepareGeometryChange();
    m_col=color;
    m_col_str = m_col.name();
    update();
}


/**
 * @brief Returns the node color string
 *
 * @return QString
 */
QString GraphicsNode::color() {
    return m_col_str;
}



/**
 * @brief Changes the size of the node
 *
 * @param size
 */
void GraphicsNode::setSize(const int &size){
    qDebug()<< "Changing the node size to:" << size;
    prepareGeometryChange();
    m_size=size;
    foreach (GraphicsEdge *edge, inEdgeList) {
        qDebug()<< "Informing inbound edges";
        edge->setTargetNodeSize(size);
    }
    foreach (GraphicsEdge *edge, outEdgeList) {
        qDebug()<< "Informing oubound edges";
        edge->setSourceNodeSize(size);
    }
    setShape(m_shape);
}




/**
 * @brief Returns the esoteric size of the node.
 *
 * @return
 */
int GraphicsNode::size() const{
    return m_size;
}




/**
 * @brief Sets the shape of the node and prepares the corresponding QPainterPath
 * m_path which will be drawn by our painter (see paint()).
 *
 * The only exception is when the shape is 'custom'. In that case, the painter
 * will paint a pixmap with the custom node icon (loaded from iconPath).
 * However, even in that case we are still creating a QPainterPath, because this
 * is needed by QGraphicsNode::shape() function which is responsible for collision
 * detection and needs to return a path with an accurate outline of the item's shape.
 * Called every time the user needs to change the shape of an node.
 *
 * @param shape
 * @param iconPath
 */
void GraphicsNode::setShape(const QString shape, const QString &iconPath) {

    prepareGeometryChange();

    m_shape=shape;

    qDebug()<< "Setting shape for node:" << nodeNumber()
            << "shape:" << m_shape
            << "iconPath" << iconPath
            << "pos:"<<  x() << "," <<  y();

    QPainterPath path;

    if ( m_shape == "circle") {
        path.addEllipse (-m_size, -m_size, 2*m_size, 2*m_size);
    }
    else if ( m_shape == "ellipse") {
        path.addEllipse(-m_size, -m_size, 2*m_size, 1.7* m_size);
    }
    else if ( m_shape == "box" || m_shape == "rectangle" || m_shape == "square"  ) {  //rectangle: for GraphML/GML compliance
        path.addRect (-m_size , -m_size , 1.8*m_size , 1.8*m_size );
    }
    else if (m_shape == "roundrectangle"  ) {  //roundrectangle: GraphML only
        path.addRoundedRect (-m_size , -m_size , 1.8*m_size , 1.8*m_size, 60.0, 60.0, Qt::RelativeSize );
    }
    else if ( m_shape == "triangle") {
        path.moveTo(-m_size,0.95* m_size) ;
        path.lineTo(m_size,0.95*m_size);
        path.lineTo( 0,-1*m_size);
        path.lineTo(-m_size,0.95*m_size) ;
        path.closeSubpath();
    }
    else if ( m_shape == "star") {
        path.setFillRule(Qt::WindingFill);
        path.moveTo(-0.8*m_size,0.6* m_size) ;
        path.lineTo(+0.8*m_size,0.6*m_size);
        path.lineTo( 0,-1*m_size);
        path.lineTo(-0.8*m_size,0.6*m_size) ;
        path.closeSubpath();

        path.moveTo(0, 1* m_size) ;
        path.lineTo(+0.8*m_size,-0.6*m_size);
        path.lineTo(-0.8*m_size,-0.6*m_size) ;
        path.lineTo(0, 1* m_size);
        path.closeSubpath();
    }
    else if ( m_shape == "diamond"){
        path.moveTo(-m_size, 0);
        path.lineTo( 0,-1*m_size);
        path.lineTo( m_size, 0);
        path.lineTo( 0, 1*m_size);
        path.lineTo(-m_size, 0) ;
        path.closeSubpath();
    }
    else if ( m_shape == "custom" ) {
        path.addRect (-m_size , -m_size , 2*m_size , 2*m_size );
        if (!iconPath.isEmpty()) {
            m_iconPath = iconPath;
        }
    }
    else if (m_shape == "bugs"   ||
             m_shape == "heart"  ||
             m_shape == "dice"   ||
             m_shape == "person" ||
             m_shape == "person-b" ) {
        path.addRect (-m_size , -m_size , 2*m_size , 2*m_size );
        // we update iconPath only if it's not empty
        // this is to allow internal GraphicsNode methods to call us
        // without always passing the current iconPath again and again.
        if (!iconPath.isEmpty()) {
            m_iconPath = iconPath;
        }
        else {
            if ( m_shape == "person" ) {
                m_iconPath = ":/images/person.svg";
            }

            if ( m_shape == "bugs" ) {
                m_iconPath = ":/images/bugs.png";
            }
            else if ( m_shape == "heart") {
                m_iconPath = ":/images/heart.svg";
            }
            else if ( m_shape == "dice" ) {
                m_iconPath = ":/images/random.png";
            }
        }
    }
    else {
        // If shape is not supported, we draw a circle.
        path.addEllipse (-m_size, -m_size, 2*m_size, 2*m_size);
    }
    m_path = path;
    update();
}



/**
 * @brief Returns the shape of the node as a path in local coordinates.
 * The shape is used for many things, including collision detection, hit tests,
 * and for the QGraphicsScene::items() functions.
 * We could ommit reimplementing this and have the default QGraphicsItem::shape()
 * return a simple rectangular shape through boundingRect() but we opt to return
 * an accurate outline of the item's shape.
 * @return
 */
QPainterPath GraphicsNode::shape() const {

    //qDebug() << "GraphicsNode::shape()";

    return (m_path);

}


/**
 * @brief Returns the bounding rectangle of the node:
 * The rectangle where all painting will take place.
 * @return
 */
QRectF GraphicsNode::boundingRect() const {
    //qDebug()<< "GraphicsNode::boundingRect() " << m_path.controlPointRect();
    return m_path.controlPointRect();
    //qreal adjust = 5;
    // return QRectF(-m_size -adjust , -m_size-adjust , 2*m_size+adjust , 2*m_size +adjust);
}


/**
 * @brief Does the actual painting using the QPainterPath created by the setShape()
 * Called by GraphicsView and GraphicsNode methods in every update()
 * @param painter
 * @param option
 */
void GraphicsNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) {
    //	painter->setClipRect( option->exposedRect );

    //qDebug()<< "GraphicsNode::paint() " << m_col;

    if (option->state & QStyle::State_MouseOver) {
        qDebug()<< "GraphicsNode::paint() mouse over " << m_col;
        painter->setBrush(m_col.darker(120));
        setZValue(ZValueNodeHighlighted);
    }
    else {
//        qDebug()<< "GraphicsNode::paint() - no mouse over " << m_col;
        painter->setBrush(m_col);
        setZValue(ZValueNode);
    }

    if (m_shape == "custom") {
        QPixmap pix(m_iconPath);
        painter->drawPixmap(-m_size, -m_size, 2*m_size, 2*m_size, pix);
    }
    else if ( m_shape == "person"  ||
              m_shape == "person-b"  ||
              m_shape == "bugs"    ||
              m_shape == "heart"   ||
              m_shape == "dice"     ) {
        // See:
        // https://techbase.kde.org/Development/Tutorials/Graphics/Performance
//        QImage image(m_iconPath);
//        painter->drawImage(QRectF(-m_size, -m_size, 2*m_size, 2*m_size) , image);
        QPixmap pix(m_iconPath);
        painter->drawPixmap(-m_size, -m_size, 2*m_size, 2*m_size, pix);
    }
    else {
        painter->setPen(QPen(QColor("#222"), 0));
        painter->drawPath (m_path);
    }

    //@TODO FIX NUMBER SIZE WHEN TOGGLING IN/OUT OF NODE SHAPE
    if (m_hasNumberInside && m_hasNumber) {
        // m_path->setFillRule(Qt::WindingFill);
        painter->setPen(QPen(QColor(m_numColor), 0));
        if (m_num > 999) {
            painter->setFont(QFont("Sans Serif", (m_numSize)? m_numSize-1: 0.4*m_size, QFont::Normal));
            painter->drawText(-0.8*m_size,m_size/3, QString::number(m_num) );
        }
        else if (m_num > 99) {
            painter->setFont(QFont("Sans Serif", (m_numSize)? m_numSize-1: 0.5*m_size, QFont::Normal));
            painter->drawText(-0.6 * m_size,m_size/3, QString::number(m_num) );
        }
        else if (m_num > 9 ) {
            painter->setFont(QFont("Sans Serif", (m_numSize)? m_numSize: 0.66*m_size, QFont::Normal));
            painter->drawText(-0.5*m_size,m_size/3,QString::number(m_num) );
        }
        else  {
            painter->setFont(QFont("Sans Serif", (m_numSize)? m_numSize: 0.66*m_size, QFont::Normal));
            painter->drawText(-0.33*m_size,m_size/3,QString::number(m_num) );
        }
    }

}






/**
 * @brief Called when the node changes, i.e. moves, becomes disabled or changes its visibility
 * Propagates the changes to connected elements, i.e. edges, numbers, etc.
 *  Checks if the node is inside the scene.
 * @param change
 * @param value
 * @return
 */
QVariant GraphicsNode::itemChange(GraphicsItemChange change, const QVariant &value) {

    switch (change) {
    case ItemPositionHasChanged: {
        //setCacheMode( QGraphicsItem::ItemCoordinateCache );
        foreach (GraphicsEdge *edge, inEdgeList)  //Move each inEdge of this node
            edge->adjust();
        foreach (GraphicsEdge *edge, outEdgeList) //Move each outEdge of this node
            edge->adjust();
        //Move its graphic number
        if ( m_hasNumber )
        {
            if (!m_hasNumberInside) 	{ //move it outside
                m_number->setZValue(ZValueNodeNumber);
                m_number->setPos( m_size+m_numberDistance, 0);
            }
        }
        if (m_hasLabel) {
            m_label->setPos( -m_size, m_labelDistance+m_size);
        }
        break;
    }
    case ItemEnabledHasChanged:{
        qDebug() << "GraphicsNode::itemChange - enabled changed";
        break;
    }
    case ItemSelectedHasChanged:{
        if (value.toBool()) {
            setZValue(ZValueNodeHighlighted);
            m_size_orig = m_size;
            setSize(m_size * 2 - 1);
            m_col_orig = m_col;
            setColor(m_col.darker(120));

            if (m_edgeHighLighting) {
                foreach (GraphicsEdge *edge, inEdgeList)
                    edge->setHighlighted(true);
                foreach (GraphicsEdge *edge, outEdgeList)
                    edge->setHighlighted(true);
            }

        }
        else{
            setZValue(ZValueNode);
            setSize(m_size_orig);
            setColor(m_col_orig);

            if (m_edgeHighLighting) {
                    foreach (GraphicsEdge *edge, inEdgeList)
                        edge->setHighlighted(false);
                    foreach (GraphicsEdge *edge, outEdgeList)
                        edge->setHighlighted(false);
            }
        }
        break;
    }
    case ItemVisibleHasChanged:
    {
        break;
    }
    default:
    {
        break;
    }
    };
    return QGraphicsItem::itemChange(change, value);
}



///** handles the events of a click on a node */
//void GraphicsNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
//    QGraphicsItem::mousePressEvent(event);
//}


//void GraphicsNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
//    update();
//    QGraphicsItem::mouseReleaseEvent(event);
//}

///**
// * @brief GraphicsNode::hoverEnterEvent
// * on hover on node, it highlights all connected edges
// * @param event
// */
//void GraphicsNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {

//    QGraphicsItem::hoverEnterEvent(event);
//}

///**
// * @brief GraphicsNode::hoverLeaveEvent
// * Stops the connected edges highlighting
// * @param event
// */
//void GraphicsNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event){
//    QGraphicsItem::hoverLeaveEvent(event);
//}



void GraphicsNode::setEdgeHighLighting(const bool &toggle) {
    m_edgeHighLighting = toggle;
}

/**
 * @brief GraphicsNode::addInLink
 * Called from a new connected in-link to acknowloedge itself to this node.
 * @param edge
 */
void GraphicsNode::addInLink( GraphicsEdge *edge ) {
    qDebug() << "GraphicsNode:  addInLink() for "<<  m_num;
    inEdgeList.push_back( edge);
    //qDebug ("GraphicsNode:  %i inEdgeList has now %i edges", m_num, inEdgeList.size());
}


void GraphicsNode::deleteInLink( GraphicsEdge *link ){
    qDebug () << "GraphicsNode::deleteInLink() - to " <<  m_num
              << " inEdgeList size: " << inEdgeList.size();
    inEdgeList.remove( link );
    qDebug () << "GraphicsNode::deleteInLink() - deleted to " <<  m_num
              << " inEdgeList size: " << inEdgeList.size();
}



void GraphicsNode::addOutLink( GraphicsEdge *edge ) {
    qDebug("GraphicsNode: addOutLink()");
    outEdgeList.push_back(edge);
    //	qDebug ("GraphicsNode: outEdgeList has now %i edges", outEdgeList.size());
}



void GraphicsNode::deleteOutLink(GraphicsEdge *link){
    qDebug () << "GraphicsNode::deleteOutLink() - from " <<  m_num
              << " outEdgeList size: " << outEdgeList.size();
    outEdgeList.remove( link);
    qDebug () << "GraphicsNode::deleteOutLink() - deleted from " <<  m_num
              << " outEdgeList size now: " << outEdgeList.size();
}



void GraphicsNode::addLabel ()  {
    qDebug()<< "GraphicsNode::addLabel()" ;
    m_label = new  GraphicsNodeLabel (this, m_labelText, m_labelSize);
    m_label->setDefaultTextColor (m_labelColor);
    m_label->setPos( m_size, m_labelDistance+m_size);
    m_hasLabel = true;
}



GraphicsNodeLabel* GraphicsNode::label(){
    if (!m_hasLabel) {
        addLabel();
    }
    return m_label;
}

void GraphicsNode::deleteLabel(){
    qDebug ("GraphicsNode: deleteLabel ");
    if (m_hasLabel) {
        m_hasLabel=false;
        m_label->hide();
        graphicsWidget->removeItem(m_label);
    }
    qDebug () << "GraphicsNode::deleteLabel() - finished";

}


void GraphicsNode::setLabelText (const QString &label) {
    qDebug()<< "GraphicsNode::setLabelText()";
    prepareGeometryChange();
    m_labelText = label;
    if (m_hasLabel)
        m_label->setPlainText(label);
    else
        addLabel();
    m_hasLabel=true;
}



void GraphicsNode::setLabelColor ( const QString &color) {
    qDebug()<< "GraphicsNode::setLabelColor()";
    prepareGeometryChange();
    m_labelColor= color;
    if (m_hasLabel)
        m_label->setDefaultTextColor(color);
    else
        addLabel();
    m_hasLabel=true;
}


void GraphicsNode::setLabelVisibility(const bool &toggle) {
    if (toggle){
        if (m_hasLabel) {
            m_label->show();
        }
        else {
            addLabel();
        }
    }
    else {
        if (m_hasLabel) {
            m_label->hide();
        }
    }

    m_hasLabel=toggle;
}

void GraphicsNode::setLabelSize(const int &size) {
    m_labelSize = size;
    if (!m_hasLabel) {
        addLabel();
    }
    m_label->setSize(m_labelSize);

}

/**
 * @brief GraphicsNode::labelText
 * @return QString
 */
QString GraphicsNode::labelText ( ) {
    return m_labelText;
}



/**
 * @brief GraphicsNode::setLabelDistance
 * @param distance
 */
void GraphicsNode::setLabelDistance(const int &distance) {
    m_labelDistance = distance;
    if (!m_hasLabel) {
        addLabel();
    }
    m_label->setPos( -m_size,  m_size+m_labelDistance);;

}





void GraphicsNode::addNumber () {
    qDebug()<<"GraphicsNode::addNumber () " ;
    m_hasNumber=true;
    m_hasNumberInside = false;
    m_number= new  GraphicsNodeNumber ( this, QString::number(m_num), m_numSize);
    m_number->setDefaultTextColor (m_numColor);
    m_number->setPos(m_size+m_numberDistance, 0);

}

GraphicsNodeNumber* GraphicsNode::number(){
    return m_number;
}


void GraphicsNode::deleteNumber( ){
    qDebug () << "GraphicsNode::deleteNumber()";
    if (m_hasNumber && !m_hasNumberInside) {
        m_number->hide();
        graphicsWidget->removeItem(m_number);
        m_hasNumber=false;
    }
    qDebug () << "GraphicsNode::deleteNumber() - finished";
}

void GraphicsNode::setNumberVisibility(const bool &toggle) {
    qDebug() << "GraphicsNode::setNumberVisibility() " << toggle;
    if (toggle) { //show
        if (!m_hasNumber) {
            m_hasNumber=toggle;
            if ( !m_hasNumberInside )
                addNumber();
            else {
                setShape(m_shape);
            }
        }
    }
    else { // hide
        deleteNumber();
        m_hasNumber=toggle;
        setShape(m_shape);
    }

}

void GraphicsNode::setNumberInside (const bool &toggle){
    qDebug()<<"GraphicsNode::setNumberInside() " << toggle;
    if (toggle) { // set number inside
        deleteNumber();
    }
    else {
        addNumber();
    }
    m_hasNumber = true;
    m_hasNumberInside = toggle;
    setShape(m_shape);
}


/**
 * @brief GraphicsNode::setNumberSize
 * @param size
 */
void GraphicsNode::setNumberSize(const int &size) {
    m_numSize = size;
    if (m_hasNumber && !m_hasNumberInside) {
        m_number->setSize(m_numSize);
    }
    else if (m_hasNumber && m_hasNumberInside) {
        setShape(m_shape);
    }
    else {
        // create a nodeNumber ?
    }

}


/**
 * @brief GraphicsNode::setNumberColor
 * @param color
 */
void GraphicsNode::setNumberColor(const QString &color) {
    m_numColor = color;
    if (m_hasNumber){
        if (m_hasNumberInside) {
            setShape(m_shape);
        }
        else {
            m_number->setDefaultTextColor (m_numColor);
        }
    }

}

/**
 * @brief GraphicsNode::setNumberDistance
 * @param distance
 */
void GraphicsNode::setNumberDistance(const int &distance) {
    m_numberDistance = distance;
    if (m_hasNumber && !m_hasNumberInside) {
        m_number->setPos( m_size+m_numberDistance, 0);
    }
    else if (m_hasNumber && m_hasNumberInside) {
        // do nothing
    }
    else {
        // create a nodeNumber ?
    }

}





GraphicsNode::~GraphicsNode(){
    qDebug() << "Destructing node "<< nodeNumber()
                << "inEdgeList.size = " << inEdgeList.size()
                << "outEdgeList.size = " << outEdgeList.size();

    qDebug()<< "Removing edges in inEdgeList";
    foreach (GraphicsEdge *edge, inEdgeList) {  // same as using qDeleteAll
        delete edge;
    }
    qDebug()<< "Removing edges in outEdgeList";
    foreach (GraphicsEdge *edge, outEdgeList) { // same as using qDeleteAll
        delete edge;
    }
    if ( m_hasNumber )
        deleteNumber();
    if ( m_hasLabel )
        deleteLabel();
    inEdgeList.clear();
    outEdgeList.clear();
    this->hide();

    graphicsWidget->removeItem(this);

}

