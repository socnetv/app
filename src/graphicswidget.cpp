/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                        graphicswidget.cpp description
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
#include "guide.h"
#include "edgeweight.h"

/** 
    Constructor method. Called when a GraphicsWidget object is created in MW
*/
GraphicsWidget::GraphicsWidget( QGraphicsScene *sc, MainWindow* par)  : QGraphicsView ( sc,par) {
    setScene(sc);
    secondDoubleClick=false;
    dynamicMovement=false;
    moving=0;
    timerId=0;
    layoutType=0;
    m_nodeLabel="";
    zoomIndex=3;
    m_currentScaleFactor = 1;
    m_currentRotationAngle = 0;
    markedNodeExist=false; //used in findNode()
    markedEdgeExist = false; //used in selecting and edge
    edgesHash.reserve(1000);
    nodeHash.reserve(1000);
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
    qDebug() << " clear GW";
    nodeHash.clear();
    edgesHash.clear();
    scene()->clear();
    m_curRelation=0;
    markedNodeExist=false;
    markedEdgeExist = false;

}

/**
 * @brief GraphicsWidget::changeRelation
 * Called from Graph::relationChanged(int) signal
 * @param relation
 */
void GraphicsWidget::changeRelation(int relation) {
    qDebug() << "GraphicsWidget::changeRelation() to " << relation;
    m_curRelation = relation;
}



/**	
    Adds a new node onto the scene
    Called from Graph::createVertex method when:
        we load files or
        the user presses "Add Node" button or
        the user double clicks (mouseDoubleClickEvent() calls Graph::createVertex
*/
void GraphicsWidget::drawNode(
        int num, int size, QString nodeColor,
        QString numberColor, int numberSize,
        QString nodeLabel, QString labelColor, int labelSize,
        QPointF p,
        QString shape,
        bool showLabels, bool numberInsideNode, bool showNumbers
        ) {
    qDebug()<< "GW: drawNode(): drawing new node at: "
            << p.x() << ", "<< p.y() ;

    if (numberInsideNode)
        size = size +3;

    Node *jim= new Node (
                this, num, size, nodeColor, shape,
                numberInsideNode, m_labelDistance, m_numberDistance,
                p
                );

    //Drawing node label - label will be moved by the node movement (see last code line in this method)
    NodeLabel *labelJim = new  NodeLabel (jim, labelSize, nodeLabel );
    labelJim -> setDefaultTextColor (labelColor);
    labelJim -> setTextInteractionFlags(Qt::TextEditorInteraction);

    if (showLabels) {
        //qDebug()<< "GW: drawNode: display label " <<  nodeLabel.toUtf8() << " for node " << num;
    }
    else {
        //qDebug()<<"GW: drawNode: hiding label for node " << num;
        labelJim->hide();
    }

    // drawing node number - label will be moved by the node movement (see last code line in this method)
    if (numberInsideNode)
        numberSize = size-2;

    NodeNumber *numberJim = new  NodeNumber ( jim, numberSize, QString::number(num));
    numberJim -> setDefaultTextColor (numberColor);

    if (!showNumbers){
        numberJim->hide();
    }
    nodeHash.insert(num, jim);//add new node to a container to ease finding, edge creation etc
    jim -> setPos( p.x(), p.y());	//finally, move the node where it belongs!
}






/** Draws an edge from source to target Node. 
    This is used when we do not have references to nodes - only nodeNumbers:
    a) when we load a network file (check = FALSE)
    b) when the user clicks on the AddLink button on the MW.
*/
void GraphicsWidget::drawEdge(int i, int j, float weight, bool reciprocal, bool drawArrows, QString color, bool bezier){

    QString edgeName = QString::number(m_curRelation) + QString(":")
            + QString::number(i) + QString(">")+ QString::number(j);
    qDebug()<<"GW: drawEdge "<< edgeName << "weight "<<weight
           << " - nodeHash reports "<< nodeHash.size()<<" nodes.";
    if (i == j ) {
        bezier = true;
    }
    //	qDebug()<< "GW: drawEdge() drawing edge now!"<< " From node "
    //			<<  nodeHash.value(i)->nodeNumber()<< " to node "
    //			<<  nodeHash.value(j)->nodeNumber() << " weight "
    //			<< weight << " nodesize "
    //			<<  m_nodeSize << " edgecolor "<< color ;
    Edge *edge=new Edge (this, nodeHash.value(i), nodeHash.value(j),
                         Qt::SolidLine,
                         weight,
                         m_nodeSize, color, reciprocal, drawArrows, bezier);
    edge -> setZValue(253);		//Edges have lower z than nodes. Nodes always appear above edges.
    // Keep it here so that it doesnt interfere with dashed lines.
    edge->setBoundingRegionGranularity(0.05);	// Slows down the universe...Keep it 0.05...
    //edge->setCacheMode (QGraphicsItem::DeviceCoordinateCache);  //Also slows down the universe...

    //    qDebug()<<"GW: drawEdge() - adding new edge between "<<i << " and "<< j
    //           << " to edgesHash. Name: "<<edgeName.toUtf8();
    edgesHash.insert(edgeName, edge);

    //    qDebug()<< "GW: drawEdge(): drawing edge weight number...";
    double x = ( (nodeHash.value(i))->x() + (nodeHash.value(j))->x() ) / 2.0;
    double y = ( (nodeHash.value(i))->y() + (nodeHash.value(j))->y() ) / 2.0;
    //    qDebug()<< "GW: drawEdge(): edge weight will be at " << x << ", " << y;
    EdgeWeight *edgeWeight = new  EdgeWeight (edge, 7, QString::number(weight) );
    edgeWeight-> setPos(x,y);
    edgeWeight-> setDefaultTextColor (color);
    edgeWeight-> hide();

    //	qDebug()<< "Scene items now: "<< scene()->items().size() << " - GW items now: "<< items().size();
}




/**
    Called from Graph to make an existing arc symmetric (reciprocal)
*/
void GraphicsWidget::drawEdgeReciprocal(int source, int target){
    qDebug("GW: drawEdgeReciprocal ()");
    QString edgeName = QString::number(m_curRelation) + QString(":") +
            QString::number(source) + QString(">")+ QString::number(target);
    //    qDebug("GW: making existing edge between %i and %i reciprocal. Name: "+edgeName.toUtf8(), source, target );
    edgesHash.value(edgeName)->makeReciprocal();
}



/**
    Called from Graph to unmake an existing symmetric (reciprocal) edge to one-directed only.
*/
void GraphicsWidget::unmakeEdgeReciprocal(int source, int target){
    qDebug("GW: unmakeEdgeReciprocal ()");
    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number(source) + QString(">")+ QString::number(target);
    //    qDebug("GW: removing edge between %i and %i. Name: "+edgeName.toUtf8(), source, target );
    edgesHash.value(edgeName)->unmakeReciprocal();
}



/** 	
    Called when the user middle-clicks on two nodes consecutively. .
    It saves the source & target nodes that were clicked
    On the second middle-click, it emits the userMiddleClicked() signal to MW,
    which will notify activeGraph, which in turn will signal back to drawEdge()...
*/
void GraphicsWidget::startEdge(Node *node){
    if (secondDoubleClick){
        qDebug("GW: startEdge(): this is the second double click. Emitting userMiddleClicked() to create edge");
        secondNode=node;
        emit userMiddleClicked(firstNode->nodeNumber(), secondNode->nodeNumber(), 1.0);
        ( (MainWindow*)parent() )->setCursor(Qt::ArrowCursor);
        secondDoubleClick=false;
    }
    else{
        qDebug("GW: startEdge(): this is the first double click.");
        firstNode=node;
        secondDoubleClick=true;
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
    Also, it makes source and target nodes to stand out of other nodes.
*/
void GraphicsWidget::edgeClicked(Edge *edge){
    qDebug ("GW: Emitting selectedEdge()");
    if (markedEdgeExist) {
        //unselect them, restore their color
        markedEdgeSource->setSelected(false);
        markedEdgeTarget->setSelected(false);
        //restore their size
        markedEdgeSource->setSize(markedEdgeSourceOrigSize);
        markedEdgeTarget->setSize(markedEdgeTargetOrigSize);
        markedEdgeExist=false;
        return;
    }
    markedEdgeSource=edge->sourceNode();
    markedEdgeTarget=edge->targetNode();
    markedEdgeExist=true;
    // select nodes to change their color
    markedEdgeSource->setSelected(true);
    markedEdgeTarget->setSelected(true);
    // save their original size
    markedEdgeSourceOrigSize=markedEdgeSource->size();
    markedEdgeTargetOrigSize=markedEdgeTarget->size();
    //now, make them larger
    markedEdgeSource->setSize(2*markedEdgeSourceOrigSize-1);
    markedEdgeTarget->setSize(2*markedEdgeTargetOrigSize-1);
    emit selectedEdge(edge);
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
    Edge is already passed with selectedEdge(Node *) signal
    The position of the menu is determined by QMouse:pos()...
*/
void GraphicsWidget::openEdgeContextMenu(){
    qDebug("GW: emitting openEdgeMenu()");
    emit openEdgeMenu();
}




/** 
    Called from each node when they move.
    Updates
    - node coordinates in activeGraph (via updateNodeCoords() signal)

*/
void GraphicsWidget::nodeMoved(int number, int x, int y){
    //qDebug ("GW: nodeMoved() for %i with %i, %i. Emitting updateNodeCoords() signal", number, x,y);
    emit updateNodeCoords(number, x, y);
}



/** 
    Called from activeGraph to update node coordinates
    on the canvas
*/
void GraphicsWidget::moveNode(int number, qreal x, qreal y){
    qDebug() << "   GW: moveNode() " << number << ": " << x << y;
    nodeHash.value(number)->setPos(x,y);
    //    qDebug() << "GW: moveNode() node reports x, y as "
    //             << nodeHash.value(number)->x() << nodeHash.value(number)->x();
}


/**
    Called from Graph signal eraseNode(int)
*/
void GraphicsWidget::eraseNode(long int doomedJim){
    qDebug() << "GW: Deleting node "<< doomedJim;
    QList<QGraphicsItem *> list=scene()->items();
    qDebug("GW: Scene items= %i - View items : %i",scene()->items().size(), items().size());
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!=list.end(); it++) {
        if ( (*it)->type()==TypeNode) {
            Node *jim=(Node*) (*it);
            if ( jim->nodeNumber()==doomedJim)	{
                qDebug() << "GW: found doomedJim " <<  jim->nodeNumber() << " Demanding node->die() :)" ;
                delete *it;
                break;
            }
        }
    }
    qDebug("GW: Scene items now= %i - View items now= %i ", scene()->items().size(), items().size() );
}



/**
    Called from MainWindow when erasing edges using vertex numbers
*/
void GraphicsWidget::eraseEdge(int sourceNode, int targetNode){
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
    Called from Node::die() to removeItem from nodeHash...
    FIXME : Do we use it ????
*/
void GraphicsWidget::removeItem( Node *node){
    long int i=node->nodeNumber();
    foreach ( Node *candidate, nodeHash) {
        if ( candidate->nodeNumber() == i )
            nodeHash.remove( i );
    }
    node->deleteLater ();
    qDebug() << "GW items now:  " << items().size();
}




/** Called from Node::die() to remove Edge edge ... */
void GraphicsWidget::removeItem( Edge * edge){
    //edge->remove();
    delete (edge);
}


void GraphicsWidget::removeItem( NodeLabel *nodeLabel){
    qDebug() << "GW items now:  " << items().size();
    delete (nodeLabel);
    qDebug() << "GW items now:  " << items().size();
}



void GraphicsWidget::removeItem( NodeNumber *nodeNumber){
    delete (nodeNumber);
}






/** 
    Accepts initial node color from MW.
    It is called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNodeColor(QString color){
    qDebug("GW setting initNodeColor");
    m_nodeColor=color;
}



/** 
    Sets initial edge color.
    Called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitLinkColor(QString color){
    qDebug("GW setting initLinkColor");
    m_linkColor=color;
}



/** 
    Sets the color of an node.
    Called from MW when the user changes the color of a node (right-clicking).
*/
bool GraphicsWidget::setNodeColor(long int nodeNumber, QString color){
    qDebug() << "GraphicsWidget::setNodeColor() : " << color;
    nodeHash.value(nodeNumber) -> setColor(color);
    return true;

}


/**
    Sets the label of an node.
    Called from MW when the user changes it
*/
bool GraphicsWidget::setNodeLabel(long int nodeNumber, QString label){
    qDebug() << "GraphicsWidget::setNodeLabel() : " << label;
    nodeHash.value(nodeNumber) -> setLabelText (label);
    return true;

}


/** 
    Makes node label appear inside node.
    Called from MW on user request.
*/
void   GraphicsWidget::setNumbersInsideNodes(bool numIn){
    qDebug("GW setting initNumberDistance");
    foreach ( Node *m_node, nodeHash) {
        m_node->setNumberInside(numIn);
        if (numIn)
            this->setInitNodeSize(m_nodeSize+2);
        else
            this->setInitNodeSize(m_nodeSize-2);

    }
}



/** 
    Changes/Sets the color of an edge.
    Called from MW when the user changes the color of an edge (right-clicking).
*/

void GraphicsWidget::setEdgeColor(long int source, long int target, QString color){

    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    qDebug()<<"GW::setEdgeColor() -" << edgeName << " to "  << color;;

    //    if  ( edgesHash.contains (edgeName) ) { // VERY SLOW
    edgesHash.value(edgeName) -> setColor(color);

}




/** 
    Changes/Sets the weight of an edge.
    Called from MW when the user changes the weight of an edge (right-clicking).
*/
bool GraphicsWidget::setEdgeWeight(int source, int target, float weight){
    QList<QGraphicsItem *> list=scene()->items();
    for (QList<QGraphicsItem *>::iterator it=list.begin(); it!= list.end() ; it++){
        if ( (*it)->type()==TypeEdge) {
            Edge *edge=(Edge*) (*it);
            if ( edge->sourceNodeNumber()==source && edge->targetNodeNumber()==target ) {
                edge->setWeight(weight);
                edge->update();
                return true;
            }
        }
    }
    return false;
}



/** 
    Sets initial node size from MW.
    It is Called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNodeSize(int size){
    qDebug("GW setting initNodeSize");
    m_nodeSize=size;
}




/** 
    Sets initial number distance from node
    Called from MW on startup and when user changes it.
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
*	Changes the visibility of an GraphicsView edge (number, label, edge, etc)
*/
void GraphicsWidget::setEdgeVisibility(int relation, int source, int target, bool toggle){
    QString edgeName =  QString::number(relation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<"GW: setEdgeVisibility(). relation " << relation
               << " : " << source  << " ->  "<< target << " to " << toggle;
        edgesHash.value(edgeName) -> setVisible(toggle);
        edgesHash.value(edgeName) -> setEnabled(toggle);
        return;
    }
    qDebug()<<"GW: setEdgeVisibility(). Cannot find edge " << relation
           << " : " << source  << " ->  "<< target ;

}



/**
*	Changes the visibility of a  Node
*/
void GraphicsWidget::setNodeVisibility(long int number, bool toggle){
    if  ( nodeHash.contains (number) ) {
        qDebug() << "GW: setNodeVisibility(): for  "
                 << number << " to " << toggle;
        nodeHash.value(number) -> setVisible(toggle);
        nodeHash.value(number) -> setEnabled(toggle);
        return;
    }
    qDebug() << "GW: setNodeVisibility(): cannot find node " << number;
}


/**
 * @brief GraphicsWidget::setNodeSize
 * @param number
 * @param size
 * @return
 */
bool GraphicsWidget::setNodeSize(long int number, int size ){
    qDebug () << " GraphicsWidget::setNodeSize() node: "<< number
              << " new size "<< size;
    if  ( nodeHash.contains (number) ) {
        if (size>0){
            qDebug() << "GW: setNodeSize(): for "<< number << " to " << size ;
            nodeHash.value(number) -> setSize(size);
            return true;

        }
        else {
            qDebug() << "GW: setNodeSize(): for "<< number
                     << " to initial size" << m_nodeSize;
            nodeHash.value(number) -> setSize(m_nodeSize);
            return true;

        }
    }
    qDebug() << "GW: setNodeSize(): cannot find node " << number;
    return false;
}

/**
 * @brief GraphicsWidget::setAllNodeSize
 * @param size
 * @return
 */
void GraphicsWidget::setAllNodeSize(int size ){
    qDebug() << "GW: setAllNodeSize() ";
    foreach ( Node *m_node, nodeHash ) {
        if (size>0){
            qDebug() << "GW: setAllNodeSize(): "<< m_node->nodeNumber() << " to new size " << size ;
            m_node -> setSize(size);
        }
        else {
            qDebug() << "GW: setAllNodeSize(): "<< m_node->nodeNumber() << " to initial size " << m_nodeSize;
            m_node -> setSize(m_nodeSize);
        }
    }
}


/*
 * Used by findNode.
 * Returns, if found, the node with label or number 'text'
 */
Node* GraphicsWidget::hasNode( QString text ){

    bool ok = false;
    foreach ( Node *candidate, nodeHash) {
        if ( 	candidate->nodeNumber()==text.toInt(&ok, 10)  ||
                ( candidate->labelText() == text)
                ) {
            qDebug() << "GW: hasNode(): Node " << text << " found!";
            markedNodeExist=true;
            return candidate;
            break;
        }

    }
    return markedNode1;	//dummy return. We check markedNodeExist flag first...
}



/**
     Marks (by double-sizing and highlighting) or unmarks a node, given its number or label.
     Called by MW:slotFindNode()
*/
bool GraphicsWidget::setMarkedNode(QString nodeText){
    qDebug ("GW: setMarkedNode()");
    if (markedNodeExist) {
        markedNode1->setSelected(false);		//unselect it, so that it restores its color
        markedNode1->setSize(markedNodeOrigSize);	//restore its size
        markedNodeExist=false;
        return true;
    }

    markedNode1 = hasNode (nodeText);
    if (!markedNodeExist)
        return false;

    markedNode1->setSelected(true);		//select it, so that it changes color
    markedNodeOrigSize=markedNode1->size(); // save its original size
    markedNode1->setSize(2*markedNodeOrigSize-1);	//now, make it larger
    return true;
}




/**
*	Changes the visibility of all items of certain type (i.e. number, label, edge, etc)
*/
void GraphicsWidget::setAllItemsVisibility(int type, bool visible){
    QList<QGraphicsItem *> list = scene()->items();
    for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
        qDebug()<< "GW::setAllItemsVisibility. item type is " << (*item)->type();
        if ( (*item)->type() == type){
            if (visible)	(*item)->show();
            else	(*item)->hide();
        }
    }
}



void GraphicsWidget::addGuideCircle( int x0, int y0, int radius){
    Guide *circ=new Guide (this, x0, y0, radius);
    circ->show();

}


void GraphicsWidget::addGuideHLine( int y0){
    Guide *line=new Guide (this, y0, 	this->width());
    line->show();
}


/**
*	Removes all items of certain type (i.e. number, label, edge, etc)
*/
void GraphicsWidget::removeAllItems(int type){
    qDebug()<< "GW: removeAllItems";
    QList<QGraphicsItem *> list = scene()->items();
    for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
        if ( (*item)->type() == type){
            Guide *guide = qgraphicsitem_cast<Guide *>  (*item);
            qDebug()<< "GW: removeAllItems - located element";
            guide->die();
            guide->deleteLater ();
            delete *item;
        }
    }
}



void GraphicsWidget::clearGuides(){
    qDebug()<< "GW: clearGuides";
    this->removeAllItems(TypeGuide);
}



/* Called from MW */
void GraphicsWidget::selectAll()
{
    qDebug() << "GraphicsWidget::selectAll()";
    QPainterPath path;
    path.addRect(0,0, this->scene()->width() , this->scene()->height());
    this->scene()->setSelectionArea(path);
    qDebug() << "selected items now: "
             << selectedItems().count();
}


/* Called from MW */
void GraphicsWidget::selectNone()
{
    qDebug() << "GraphicsWidget::selectNone()";

    this->scene()->clearSelection();

}

/* Called from MW */
QList<QGraphicsItem *> GraphicsWidget::selectedItems(){
     return this->scene()->selectedItems();

}

/** 	
    Starts a new node when the user double-clicks somewhere
    Emits userDoubleClicked to MW slot addNodeWithMouse() which
        - displays node info on MW status bar and
        - calls Graph::createVertex(), which in turn calls this->drawNode()...
        Yes, we make a full circle! :)
*/
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            Q_UNUSED(node);
            qDebug() << "GW: mouseDoubleClickEvent() double click on a node detected!"
                     << " Cant create new one!";
            return;
        }
    }

    QPointF p = mapToScene(e->pos());
    qDebug()<< "GW::mouseDoubleClickEvent()"
            << " double click on empty space. "
            << " Signaling MW to create a new vertex in graph. e->pos() "
            << e->pos().x() << ","<< e->pos().y() << ", "<< p.x() << "," <<p.y();
    emit userDoubleClicked(-1, p);
    qDebug() << "GW::mouseDoubleClickEvent() "
             << "Scene items: "<< scene()->items().size()
             << " GW items: " << items().size();
}



void GraphicsWidget::mousePressEvent( QMouseEvent * e ) {

    QPointF p = mapToScene(e->pos());

    bool ctrlKey = (e->modifiers() == Qt::ControlModifier);


    qDebug() << "GW::mousePressEvent() click at "
                << e->pos().x() << ","<< e->pos().y()
                << " or "<<  p.x() << ","<< p.y()
                << " selectedItems " << selectedItems().count();

  //  emit selectedItems(m_selectedItems);

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {

        qDebug() << "GW::mousePressEvent() click on an item" ;

        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            Q_UNUSED(node);
            qDebug() << "GW::mousePressEvent() single click on a node " ;
            QGraphicsView::mousePressEvent(e);
            return;
        }
        if (Edge *edge= qgraphicsitem_cast<Edge *>(item)) {
            Q_UNUSED(edge);
            qDebug() << "GW::mousePressEvent() single click on an edge ";
            QGraphicsView::mousePressEvent(e);
            return;
        }
    }
    else{
        qDebug() << "GW::mousePressEvent()  click on empty space. ";

        if ( selectedItems().count() > 0 && ctrlKey ) {
            qDebug() << " opening selection context menu ";
            emit openContextMenu(p);
        }

        else if ( e->button()==Qt::RightButton   ) {
            qDebug() << "GW::mousePressEvent() Right-click on empty space ";
            emit openContextMenu(p);
        }
        QGraphicsView::mousePressEvent(e);
    }

}




void GraphicsWidget::mouseReleaseEvent( QMouseEvent * e ) {
    QPointF p = mapToScene(e->pos());
    qDebug() << "GW::mouseReleaseEvent() at "
             << e->pos().x() << ","<< e->pos().y()
             << " or "<<  p.x() << ","<<p.y();

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            qDebug() << "GW::mouseReleaseEvent() on a node ";
            Q_UNUSED(node);
            QGraphicsView::mouseReleaseEvent(e);
        }
    }
    else{
        qDebug() << "GW::mouseReleaseEvent() on empty space. ";

    }

    qDebug() << "GW::mouseReleaseEvent() - selected items now: "
             << selectedItems().count();

}







/** 
    Calls the scaleView() when the user spins the mouse wheel
    It passes delta as new m_scale
*/
void GraphicsWidget::wheelEvent(QWheelEvent *e) {
    qDebug("GW: Mouse wheel event");
    qDebug() << "GW: delta = " << e->delta();
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
    resetMatrix();
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

