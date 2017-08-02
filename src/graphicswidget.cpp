/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt

                        graphicswidget.cpp description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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
#include "edgelabel.h"

/** 
    Constructor method. Called when a GraphicsWidget object is created in MW
*/
GraphicsWidget::GraphicsWidget( QGraphicsScene *sc, MainWindow* par)  : QGraphicsView ( sc,par) {
    setScene(sc);
    secondDoubleClick=false;
    transformationActive = false;
    m_nodeLabel="";
    m_zoomIndex=250;
    m_currentScaleFactor = 1;
    m_currentRotationAngle = 0;
    markedNodeExist=false; //used in findNode()
    clickedEdgeExists = false; //used in selecting and edge
    edgesHash.reserve(1000);
    nodeHash.reserve(1000);


    connect ( sc , &QGraphicsScene::selectionChanged,
                 this, &GraphicsWidget::getSelectedItems);


}



/**
    http://thesmithfam.org/blog/2007/02/03/qt-improving-qgraphicsview-performance/#comment-7215
*/
void GraphicsWidget::paintEvent ( QPaintEvent * event ){
//    qDebug()<<"GraphicsWidget::paintEvent ";
//    QPaintEvent *newEvent=new QPaintEvent(event->region().boundingRect());
//    QGraphicsView::paintEvent(newEvent);
//    delete newEvent;
     QGraphicsView::paintEvent(event);
}




/**
 * @brief GraphicsWidget::clear
 * Clears the scene and all hashes, lists, var etc
 */
void GraphicsWidget::clear() {
    nodeHash.clear();
    edgesHash.clear();
    m_selectedNodes.clear();
    m_selectedEdges.clear();
    scene()->clear();
    m_curRelation=0;
    markedNodeExist=false;
    clickedEdgeExists = false;
    firstNode=0;
    secondDoubleClick=false;
    qDebug() << "GW::clear() - finished clearing hashes";
}

/**
 * @brief GraphicsWidget::relationSet
 * Called from Graph::signalRelationChangedToGW(int) signal
 * @param relation
 */
void GraphicsWidget::relationSet(int relation) {
    qDebug() << "GraphicsWidget::relationSet() to " << relation;
    m_curRelation = relation;
}



/**
 * @brief GraphicsWidget::drawNode
 * Adds a new node onto the scene
 * Called from Graph::vertexCreate method primarily when we load files
 * It is also called in the end when the user presses "Add Node" button or
 * the user double clicks (mouseDoubleClickEvent() calls Graph::vertexCreate)
 * @param num
 * @param nodeSize
 * @param nodeColor
 * @param numberColor
 * @param numberSize
 * @param nodeLabel
 * @param labelColor
 * @param labelSize
 * @param p
 * @param nodeShape
 * @param showLabels
 * @param numberInsideNode
 * @param showNumbers
 */
void GraphicsWidget::drawNode( const int &num, const int &nodeSize,
                               const QString &nodeShape, const QString &nodeColor,
                               const bool &showNumbers,const bool &numberInsideNode,
                               const QString &numberColor, const int &numberSize,
                               const int &numberDistance,
                               const bool &showLabels, const QString &nodeLabel,
                               const QString &labelColor, const int &labelSize,
                               const int &labelDistance,
                               const QPointF &p
                                ) {
    qDebug()<< "GW: drawNode(): drawing new node " << num
            << " at: " << p.x() << ", "<< p.y() ;

    //Draw node
    Node *jim= new Node (
                this, num, nodeSize, nodeColor, nodeShape,
                showNumbers, numberInsideNode, numberColor, numberSize, numberDistance,
                showLabels, nodeLabel, labelColor, labelSize, labelDistance,
                p
                );

    //add new node to a container to ease finding, edge creation etc
    nodeHash.insert(num, jim);
}







/**
 * @brief GraphicsWidget::drawEdge
 * Draws an edge from source to target Node.
 * Used when we do not have references to nodes but only nodeNumbers:
    a) when we load a network file
    b) when the user clicks on the AddLink button on the MW.
 * @param source
 * @param target
 * @param weight
 * @param reciprocal
 * @param drawArrows
 * @param color
 * @param bezier
 */
void GraphicsWidget::drawEdge(const int &source, const int &target,
                              const float &weight,
                              const QString &label,
                              const QString &color,
                              const int &type,
                              const bool &drawArrows,
                              const bool &bezier,
                              const bool &weightNumbers){

    edgeName = QString::number(m_curRelation) + QString(":")
            + QString::number(source) + QString(">")+ QString::number(target);
    qDebug()<<"GW::drawEdge() - "<< edgeName
           << " weight "<<weight
           << " label " << label
           << " type " << type

           << " - nodeHash reports "<< nodeHash.size()<<" nodes.";


    Edge *edge=new Edge (this,
                         nodeHash.value(source), nodeHash.value(target),
                         weight, label, color,
                         Qt::SolidLine,
                         type,
                         drawArrows,
                         (source==target) ? true: bezier,
                         weightNumbers);

    edgesHash.insert(edgeName, edge);
    if (type == EDGE_DIRECTED_OPPOSITE_EXISTS ) {
        edgeName = QString::number(m_curRelation) + QString(":") +
                QString::number(target) + QString(">")+ QString::number(source);
        qDebug("GW::drawEdge() - making existing edge between %i and %i reciprocal. Name: "+edgeName.toUtf8(), source, target );
        edgesHash.value(edgeName)->setDirectedWithOpposite();

    }
    //	qDebug()<< "Scene items now: "<< scene()->items().size() << " - GW items now: "<< items().size();
}





/** 	
    Called when the user middle-clicks on two nodes consecutively. .
    It saves the source & target nodes that were clicked
    On the second middle-click, it emits the userMiddleClicked() signal to MW,
    which will notify activeGraph, which in turn will signal back to drawEdge()...
*/
void GraphicsWidget::startEdge(Node *node){
    if (secondDoubleClick){
        qDebug()<< "GW::startEdge() - this is the second double click. "
                   "Emitting userMiddleClicked() to create edge";
        secondNode=node;
        emit userMiddleClicked(firstNode->nodeNumber(), secondNode->nodeNumber() );
        //( (MainWindow*)parent() )->setCursor(Qt::ArrowCursor);
        emit setCursor(Qt::ArrowCursor);
        secondDoubleClick=false;
    }
    else{
        qDebug()<<"GW::startEdge() - this is the first double click.";
        firstNode=node;
        secondDoubleClick=true;
        //( (MainWindow*)parent() )->setCursor( Qt::PointingHandCursor);
        emit setCursor( Qt::PointingHandCursor );
    }
}



/** 
    This is called from each node when the user clicks on it.
    It emits the userClickedNode signal to MW which is used to
    - display node info on the status bar
    - notify context menus for the clicked node.
*/
void GraphicsWidget::nodeClicked(Node *node){
    qDebug () << "GW::nodeClicked() - Emitting userClickedNode()";
    if (clickedEdgeExists) edgeClicked(0);
    emit userClickedNode(node->nodeNumber());
}



/** 
    This is called from each edge when the user clicks on it.
    It emits the userClickedEdge signal to Graph which is used to
    - display edge info on the status bar
    - notify context menus for the clicked edge.
    Also, it makes source and target nodes to stand out of other nodes.
*/
void GraphicsWidget::edgeClicked(Edge *edge){
    //qDebug() <<"GW::edgeClicked()";
    if (clickedEdgeExists) {
        //unselect them, restore their color
        markedEdgeSource->setSelected(false);
        markedEdgeTarget->setSelected(false);
        //restore their size
        markedEdgeSource->setSize(markedEdgeSourceOrigSize);
        markedEdgeTarget->setSize(markedEdgeTargetOrigSize);
        clickedEdgeExists=false;
        return;
    }
    markedEdgeSource=edge->sourceNode();
    markedEdgeTarget=edge->targetNode();
    clickedEdgeExists=true;
    // select nodes to change their color
    markedEdgeSource->setSelected(true);
    markedEdgeTarget->setSelected(true);
    // save their original size
    markedEdgeSourceOrigSize=markedEdgeSource->size();
    markedEdgeTargetOrigSize=markedEdgeTarget->size();
    //now, make them larger
    markedEdgeSource->setSize(2*markedEdgeSourceOrigSize-1);
    markedEdgeTarget->setSize(2*markedEdgeTargetOrigSize-1);

    emit userClickedEdge(edge->sourceNode()->nodeNumber(),
                         edge->targetNode()->nodeNumber()
                         );
}










/**
 * @brief GraphicsWidget::moveNode
 * Called from activeGraph to update node coordinates on the canvas
 * @param num
 * @param x
 * @param y
 */
void GraphicsWidget::moveNode(const int &num, const qreal &x, const qreal &y){
    qDebug() << "   GW: moveNode() " << num << ": " << x << y;
    nodeHash.value(num)->setPos(x,y);
    //    qDebug() << "GW: moveNode() node reports x, y as "
    //             << nodeHash.value(number)->x() << nodeHash.value(number)->x();
}



/**
 * @brief Called from Graph signal eraseNode(int)
 * @param number
 */
void GraphicsWidget::eraseNode(const long int &number){
        qDebug() << "GW::eraseNode() - node " << number
                 << " scene items: " << scene()->items().size()
                 << " view items: " << items().size()
                 << " nodeHash items: "<< nodeHash.count();

    if ( nodeHash.contains(number) ) {
        qDebug() << "GW::eraseNode() - found number "
                 <<  number<< " Deleting :)" ;

        if ( nodeHash.value(number) == markedNode1 ) {

            qDebug() << "WARNING DOOMED NODE IS ALREADY MARKED!! UNMARKING IT";
            setMarkedNode(""); 	// call setMarkedNode to just unmark it.

        }
        else {
            qDebug() << "OK - DOOMED NODE IS NOT MARKED!";
        }
        delete nodeHash.value(number);
    }

    qDebug() << "GW::eraseNode() - node erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size()
             << " nodeHash items: "<< nodeHash.count()
             << " edgesHash items: "<< edgesHash.count() ;
}




/**
 * @brief GraphicsWidget::eraseEdge
 * Called from MW/Graph when erasing edges using vertex numbers
 * Also called when transforming directed edges to undirected.
 * @param sourceNode
 * @param targetNode
 */
void GraphicsWidget::eraseEdge(const long int &source, const long int &target){
    qDebug() << "GW::eraseEdge(): " << source << " -> " << target
             << " scene items: " << scene()->items().size()
             << " view items: " << items().size()
             << " edgesHash.count: " << edgesHash.count();


    edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source )
            + QString(">")
            + QString::number( target );

    if ( edgesHash.contains(edgeName) ) {
        delete edgesHash.value(edgeName);
    }


    qDebug() << "GW::eraseEdge() - deleted "
             << " scene items: " << scene()->items().size()
             << " view items: " << items().size()
             << " edgesHash.count: " << edgesHash.count();

}








/**
 * @brief GraphicsWidget::removeItem
 * @param node
 * Called from Node::~Node() to remove itself from nodeHash, scene and
 * be deleted
 */
void GraphicsWidget::removeItem( Node *node){
    long int i=node->nodeNumber();
    qDebug() << "GW::removeItem(node) - number: " <<  i;
    if (firstNode == node) {
        qDebug() << "GW::removeItem(node) - number: " <<  i
                 << "previously set as source node for a new edge. Unsetting.";
        secondDoubleClick = false;
       //( (MainWindow*)parent() )->setCursor(Qt::ArrowCursor);
        emit setCursor(Qt::ArrowCursor);
    }
    nodeHash.remove(i);
    scene()->removeItem(node);
    node->deleteLater ();
    qDebug() << "GW::removeItem(node) - node erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}





/**
 * @brief GraphicsWidget::removeItem
 * @param edge
 * Called from Edge::~Edge() to remove itself from edgesHash and scene and
 * be deleted
 */
void GraphicsWidget::removeItem( Edge * edge){
    qDebug() << "GW::removeItem(edge)" ;
    edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( edge->sourceNodeNumber() )
            + QString(">")
            + QString::number( edge->targetNodeNumber() );
    edgesHash.remove(edgeName);
    scene()->removeItem(edge);
    edge->deleteLater();
    qDebug() << "GW::removeItem(edge) - edge erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}


/**
 * @brief GraphicsWidget::removeItem
 * @param edgeWeight
 */
void GraphicsWidget::removeItem( EdgeWeight *edgeWeight){
    qDebug() << "GW::removeItem(edgeWeight) -  of edge"
             << "GW items now:  " << items().size();
    scene()->removeItem(edgeWeight);
    edgeWeight->deleteLater();
    qDebug() << "GW::removeItem(edgeWeight) - edgeWeight erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}

/**
 * @brief GraphicsWidget::removeItem
 * @param edgeLabel
 */
void GraphicsWidget::removeItem( EdgeLabel *edgeLabel){
    qDebug() << "GW::removeItem(edgeLabel) -  of edge"
             << "GW items now:  " << items().size();
    scene()->removeItem(edgeLabel);
    edgeLabel->deleteLater();
    qDebug() << "GW::removeItem(edgeLabel) - edgeLabel erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}

/**
 * @brief GraphicsWidget::removeItem
 * @param nodeLabel
 */
void GraphicsWidget::removeItem( NodeLabel *nodeLabel){
    qDebug() << "GW::removeItem(label) -  of node " <<  nodeLabel->node()->nodeNumber()
             << "GW items now:  " << items().size();
    scene()->removeItem(nodeLabel);
    nodeLabel->deleteLater();
    qDebug() << "GW::removeItem(label) - label erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();


}


/**
 * @brief GraphicsWidget::removeItem
 * @param nodeNumber
 */
void GraphicsWidget::removeItem( NodeNumber *nodeNumber){
    qDebug() << "GW::removeItem(number) -  of node " <<  nodeNumber->node()->nodeNumber()
             << "GW items now:  " << items().size();
    scene()->removeItem(nodeNumber);
    nodeNumber->deleteLater();
    qDebug() << "GW::removeItem(number) - number erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();

}








/**
 * @brief GraphicsWidget::setNodeColor
 * Sets the color of an node.
 * Called from MW when the user changes the color of a node (right-clicking).
 * @param nodeNumber
 * @param color
 * @return
 */
bool GraphicsWidget::setNodeColor(const long int &nodeNumber,
                                  const QString &color){
    qDebug() << "GW::setNodeColor() : " << color;
    nodeHash.value(nodeNumber) -> setColor(color);
    return true;
}


/**
    Sets the shape of an node.
    Called from MW when the user changes the shape of a node
*/
bool GraphicsWidget::setNodeShape(const long &nodeNumber, const QString &shape){
    qDebug() << "GW::setNodeShape() : " << shape;
    nodeHash.value(nodeNumber) -> setShape(shape);
    return true;

}





void GraphicsWidget::setNodeNumberVisibility(const bool &toggle){
    qDebug()<< "GW::setNodeNumberVisibility()" << toggle;
    foreach ( Node *m_node, nodeHash) {
        m_node->setNumberVisibility(toggle);
    }
}

void GraphicsWidget::setNodeLabelsVisibility (const bool &toggle){
    qDebug()<< "GW::setNodeLabelsVisibility()" << toggle;
    foreach ( Node *m_node, nodeHash) {
        m_node->setLabelVisibility(toggle);
    }
}


/**
 * @brief GraphicsWidget::setNodeLabel
 * Sets the label of an node. Called from MW.
 * @param nodeNumber
 * @param label
 * @return
 */
bool GraphicsWidget::setNodeLabel(long int nodeNumber, QString label){
    qDebug() << "GW::setNodeLabel() : " << label;
    nodeHash.value(nodeNumber) -> setLabelText (label);
    return true;

}



/**
 * @brief GraphicsWidget::setNumbersInsideNodes
 * Toggles node numbers displayed inside or out of nodes
 * Called from MW
 * @param numIn
 */
void   GraphicsWidget::setNumbersInsideNodes(bool numIn){
    qDebug()<< "GW::setNumbersInsideNodes" << numIn;
    foreach ( Node *m_node, nodeHash) {
        m_node->setNumberInside(numIn);
    }
}







/**
 * @brief GraphicsWidget::setEdgeLabel
 * Sets the label of an edge.
 * Called from MW when the user changes the label of an edge (right-clicking).
  * @param source
 * @param target
 * @param label
 */
void GraphicsWidget::setEdgeLabel(const long int &source,
                                  const long int &target,
                                  const QString &label){

    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    qDebug()<<"GW::setEdgeLabel() -" << edgeName <<  " new label "  << label;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName) -> setLabel(label);
    }


}

/**
 * @brief GraphicsWidget::setEdgeColor
 * Sets the color of an edge.
 * Called from MW when the user changes the color of an edge (right-clicking).
 * Also called from Graph when all edge colors need to be changed.
 * @param source
 * @param target
 * @param color
 */
void GraphicsWidget::setEdgeColor(const long int &source,
                                  const long int &target,
                                  const QString &color){

    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    qDebug()<<"GW::setEdgeColor() -" << edgeName <<  " new color "  << color;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName) -> setColor(color);
    }


}






/**
 * @brief GraphicsWidget::setEdgeUndirected
 * Makes an edge undirected
 * @param source
 * @param target
 * @param weight
 * @return
 */
bool GraphicsWidget::setEdgeUndirected(const long int &source,
                                       const long int &target,
                                       const float &weight){
    qDebug() << "GW::setEdgeUndirected() : " << source << "->" << target
             << " = " << weight;

    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    qDebug()<<"GW::setEdgeUndirected() - checking edgesHash for:" << edgeName ;
    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<"GW::setEdgeUndirected() - edge exists in edgesHash. "
                  << " Transforming it to undirected";
        edgesHash.value(edgeName) -> setUndirected();

        qDebug()<<"GW::setEdgeUndirected() - removing opposite edge "
               << target << " -> " << source ;
        eraseEdge(target, source);

        return true;
    }
    return false;

}



/** 
    Changes/Sets the weight of an edge.
    Called from MW when the user changes the weight of an edge (right-clicking).
*/
bool GraphicsWidget::setEdgeWeight(const long int &source,
                                   const long int &target,
                                   const float &weight){
    qDebug() << "GW::setEdgeWeight() : " << source << "->" << target
             << " = " << weight;

    QString edgeName =  QString::number(m_curRelation) + QString(":") +
            QString::number( source ) + QString(">")+ QString::number( target );

    qDebug()<<"GW::setEdgeWeight() -" << edgeName <<  " new weight "  << weight;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName) -> setWeight(weight);
        return true;
    }
    return false;

}



void GraphicsWidget::setEdgeWeightNumbersVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeWeightNumbersVisibility()" << toggle;
    foreach ( Edge *m_edge, edgesHash) {
        m_edge->setWeightNumberVisibility(toggle);
    }
}




void GraphicsWidget::setEdgeLabelsVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeLabelsVisibility()" << toggle
               << "edgesHash.count: " << edgesHash.count();
    foreach ( Edge *m_edge, edgesHash) {
        m_edge->setLabelVisibility(toggle);
    }
}


/**
    Sets initial node size from MW.
    It is Called from MW on startup and when user changes it.
*/
void GraphicsWidget::setInitNodeSize(int size){
    qDebug()<< "GW::setInitNodeSize() " << size;
    m_nodeSize=size;
}







/**
 * @brief GraphicsWidget::setInitZoomIndex
 * Passes initial zoom setting
 * Called from MW on startup
 */
void GraphicsWidget::setInitZoomIndex(int zoomIndex) {
    m_zoomIndex = zoomIndex;
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
bool GraphicsWidget::setNodeSize(const long int &number, const int &size ){
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
void GraphicsWidget::setAllNodeSize(const int &size ){
    qDebug() << "GW: setAllNodeSize() ";
    foreach ( Node *m_node, nodeHash ) {
            qDebug() << "GW: setAllNodeSize(): "<< m_node->nodeNumber() << " to new size " << size ;
            m_node -> setSize(size);
    }
}



/**
 * @brief GraphicsWidget::setNodeNumberSize
 * @param number
 * @param size
 */
bool GraphicsWidget::setNodeNumberSize(const long int &number, const int &size){
    qDebug () << " GraphicsWidget::setNodeNumberSize() node number: "<< number
              << " new number size "<< size;
    if  ( nodeHash.contains (number) ) {
        if (size>0){
            qDebug() << "GW: setNodeNumberSize(): for "<< number << " to " << size ;
            nodeHash.value(number) ->setNumberSize(size) ;
            return true;

        }
    }
    qDebug() << "GW: setNodeSize(): cannot find node " << number;
    return false;
}



/**
 * @brief GraphicsWidget::setNodeNumberDistance
 * @param number
 * @param distance
 */
bool GraphicsWidget::setNodeNumberDistance( const long int &number, const int &distance ){
    qDebug () << "GW::setNodeNumberDistance() node number: "<< number
              << " new number distance "<< distance;
    if  ( nodeHash.contains (number) ) {
        if (distance>=0){
            qDebug() << "GW::setNodeNumberDistance(): for "<< number
                     << " to " << distance ;
            nodeHash.value(number) ->setNumberDistance(distance) ;
            return true;

        }
    }
    qDebug() << "GW::setNodeNumberSize(): cannot find node " << number;
    return false;
}




/**
 * @brief GraphicsWidget::setNodeLabelColor
 * @param number
 * @param color
 */
bool GraphicsWidget::setNodeLabelColor(const long int &number, const QString &color){
    qDebug () << "GW::setNodeLabelColor() - node number: "<< number
              << " new Label color"<< color;
    if  ( nodeHash.contains (number) ) {
            nodeHash.value(number) ->setLabelColor(color);
            return true;
    }
    qDebug() << "GW:setNodeLabelColor() - cannot find node " << number;
    return false;
}




/**
 * @brief GraphicsWidget::setNodeLabelSize
 * @param number
 * @param size
 */
bool GraphicsWidget::setNodeLabelSize(const long int &number, const int &size){
    qDebug () << "GW::setNodeLabelSize() - node number: "<< number
              << " new Label size "<< size;
    if  ( nodeHash.contains (number) ) {
        if (size>0){
            qDebug() << "GW::setNodeLabelSize(): for "<< number << " to " << size ;
            nodeHash.value(number) ->setLabelSize(size);
            return true;

        }
    }
    qDebug() << "GW:setNodeLabelSize() - cannot find node " << number;
    return false;
}





/**
 * @brief GraphicsWidget::setNodeLabelDistance
 * @param number
 * @param distance
 */
bool GraphicsWidget::setNodeLabelDistance( const long int &number, const int &distance ){
    qDebug () << "GW::setNodeLabelDistance() - node number: "<< number
              << " new label distance "<< distance;
    if  ( nodeHash.contains (number) ) {
        if (distance>=0){
            qDebug() << "GW::setNodeLabelDistance(): for "<< number
                     << " to " << distance ;
            nodeHash.value(number) ->setLabelDistance(distance) ;
            return true;
        }
    }
    qDebug() << "GW::setNodeLabelDistance() - cannot find node " << number;
    return false;
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
     Called by MW:slotEditNodeFind()
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



void GraphicsWidget::addGuideCircle( const double&x0,
                                     const double&y0,
                                     const double&radius){
    Guide *circ=new Guide (this, x0, y0, radius);
    circ->show();

}


void GraphicsWidget::addGuideHLine(const double &y0){
    Guide *line=new Guide (this, y0, this->width());
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



/**
 * @brief GraphicsWidget::selectAll
 * Called from MW. Clears any clickedNode info and sets a selection rect
 * in the scene, which signals QGraphicsScene::selectionChanged signal to update
 * selectedNodes and selectedEdges.
 */
void GraphicsWidget::selectAll(){
    QPainterPath path;
    path.addRect(0,0, this->scene()->width() , this->scene()->height());
    this->scene()->setSelectionArea(path);
    emit userClickedNode(0);
    qDebug() << "GraphicsWidget::selectAll() - selected items now: "
             << selectedItems().count();
}



/**
 * @brief GraphicsWidget::selectNone
 * Called from MW. Clears any clickedNode info and any previous selection rect
 * in the scene, which again signals selectionChanged() to update selectedNodes
 * and selectedEdges to zero.
 */
void GraphicsWidget::selectNone(){
    qDebug() << "GraphicsWidget::selectNone()";
    emit userClickedNode(0);
    this->scene()->clearSelection();

}


/**
 * @brief Emits selected nodes and edges to Graph and MW
 * Called by QGraphicsScene::selectionChanged signal whenever the user
 * makes a selection on the canvas.
 * Emits selectedNodes and selectedEdges lists to
 * Graph::graphSelectionChanged() which then signals to
 * MW::slotEditSelectionChanged to display counts on app window.
 */
void GraphicsWidget::getSelectedItems() {

    qDebug() <<"GW::getSelectedItems()";

    if (!clickedEdgeExists)
       // emit userSelectedItems(nodes, edges);
        emit userSelectedItems(selectedNodes(), selectedEdges());
}



/**
 * @brief Returns a QList of all selected QGraphicsItem(s)
 * @return a QList of all selected QGraphicsItem(s)
 */
QList<QGraphicsItem *> GraphicsWidget::selectedItems(){
    qDebug() <<"GW::selectedItems()";
    return this->scene()->selectedItems();
}

/**
 * @brief Returns a QList of selected node numbers
 * Called by GW::getSelectedItems and MW::selectedNodes
 * @return a QList of integers: the selected node numbers
 */
QList<int> GraphicsWidget::selectedNodes() {
        m_selectedNodes.clear();
        foreach (QGraphicsItem *item, scene()->selectedItems()) {
            if (Node *node = qgraphicsitem_cast<Node *>(item) ) {
                m_selectedNodes.append(node->nodeNumber());
            }
        }
        return m_selectedNodes;
}


/**
 * @brief Returns a QList of selected directed edges structs in the form of v1,v2
 *
 * @return a QList of selected directed edges structs
 */
QList<SelectedEdge> GraphicsWidget::selectedEdges() {
        m_selectedEdges.clear();
        foreach (QGraphicsItem *item, scene()->selectedItems()) {
            if (Edge *edge= qgraphicsitem_cast<Edge *>(item) ) {
                SelectedEdge selEdge = make_pair( edge->sourceNodeNumber(), edge->targetNodeNumber());
                m_selectedEdges << selEdge;
            }
        }
        return m_selectedEdges;
}



/** 	
    Starts a new node when the user double-clicks somewhere
    Emits userDoubleClicked to MW::slotEditNodeAddWithMouse() which
        - displays node info on MW status bar and
        - calls Graph::vertexCreate(), which in turn calls this->drawNode()...
        Yes, we make a full circle! :)
*/
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            qDebug() << "GW: mouseDoubleClickEvent() - on a node!"
                     << "Scene items: "<< scene()->items().size()
                     << "GW items: " << items().size()
                     << "Starting new edge!";
            node->setSelected(true);
            nodeClicked(node);
            startEdge(node);
            QGraphicsView::mouseDoubleClickEvent(e);
            return;
        }
        else if ( (*item).type() == TypeLabel){
            QGraphicsView::mouseDoubleClickEvent(e);
            return;
        }
        qDebug() << "GW: mouseDoubleClickEvent() - on something, not a node!"
                 << "Scene items: "<< scene()->items().size()
                 << "GW items: " << items().size();

    }

    QPointF p = mapToScene(e->pos());
    qDebug()<< "GW::mouseDoubleClickEvent() - on empty space. "
            << "Scene items: "<< scene()->items().size()
            << " GW items: " << items().size()
            << " Signaling MW to create a new vertex in graph. e->pos() "
            << e->pos().x() << ","<< e->pos().y() << ", "<< p.x() << "," <<p.y();
    emit userDoubleClickNewNode(p);

}



void GraphicsWidget::mousePressEvent( QMouseEvent * e ) {

    QPointF p = mapToScene(e->pos());

   // bool ctrlKey = (e->modifiers() == Qt::ControlModifier);

  //  emit selectedItems(m_selectedItems);

    if ( QGraphicsItem *item= itemAt(e->pos() )   ) {
        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            qDebug() << "GW::mousePressEvent() - click at "
                        << e->pos().x() << ","<< e->pos().y()
                        << " or "<<  p.x() << ","<< p.y()
                        << " selectedItems " << selectedItems().count()
                        << "Single click on a node. "
                     << "Setting selected and emitting nodeClicked";
            node->setSelected(true);
            nodeClicked(node);
            if ( e->button()==Qt::RightButton ) {
                qDebug() << "GW::mousePressEvent() - Right-click on node. "
                         << "emitting openNodeMenu() ";
                emit openNodeMenu();
            }
            if ( e->button()==Qt::MidButton) {
                qDebug() << "GW::mousePressEvent() - Middle-click on node.  "
                         << "Calling startEdge() ";
                startEdge(node);
            }
            QGraphicsView::mousePressEvent(e);
            return;
        }
        if (Edge *edge= qgraphicsitem_cast<Edge *>(item)) {
            qDebug() << "GW::mousePressEvent() - click at "
                        << e->pos().x() << ","<< e->pos().y()
                        << " or "<<  p.x() << ","<< p.y()
                        << " selectedItems " << selectedItems().count()
                        << "Single click on an edge. "
                     << "Emitting edgeClicked";
            edgeClicked(edge);
            if ( e->button()==Qt::LeftButton ) {
                qDebug() << "GW::mousePressEvent() - left click on an edge ";
                //	graphicsWidget->startNodeMovement(0);
            }
            else if ( e->button()==Qt::RightButton ) {
                qDebug() << "GW::mousePressEvent() - right click on an edge."
                         << "Emitting openEdgeContextMenu()";
                emit openEdgeMenu();
            }
            QGraphicsView::mousePressEvent(e);
            return;
        }
    }

//        if ( selectedItems().count() > 0 && ctrlKey ) {
//            qDebug() << "GW::mousePressEvent() - opening selection context menu ";
//            emit openContextMenu(p);
//        }

        else if ( e->button()==Qt::RightButton   ) {
        qDebug() << "GW::mousePressEvent() - click at "
                << e->pos().x() << ","<< e->pos().y()
                << " or "<<  p.x() << ","<< p.y()
                << " selectedItems " << selectedItems().count()
                   << "Right click on empty space. Emitting openContextMenu()";
            emit openContextMenu(p);
        }
        else {
        qDebug() << "GW::mousePressEvent() - click at "
                << e->pos().x() << ","<< e->pos().y()
                << " or "<<  p.x() << ","<< p.y()
                << " selectedItems " << selectedItems().count()
                   << "Right click on empty space. Emitting userClickOnEmptySpace()";
            emit userClickOnEmptySpace(p);
        }
        QGraphicsView::mousePressEvent(e);


}



/**
 * @brief GraphicsWidget::mouseReleaseEvent
 * @param e
 * Called when user releases a mouse button, after a click.
 * First sees what was in the canvas position where the user clicked
 * If a node was underneath, it calls userNodeMoved() signal for every node
 * in scene selectedItems
 */
void GraphicsWidget::mouseReleaseEvent( QMouseEvent * e ) {
    QPointF p = mapToScene(e->pos());

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
        if (Node *node = qgraphicsitem_cast<Node *>(item)) {
            qDebug() << "GW::mouseReleaseEvent() at "
                     << e->pos().x() << ","<< e->pos().y()
                     << " or "<<  p.x() << ","<<p.y()
                     << "On a node. Selected items: "
                     << selectedItems().count()
                     << "Emitting userNodeMoved() for all selected nodes";
            Q_UNUSED(node);
            foreach (QGraphicsItem *item, scene()->selectedItems()) {
                if (Node *nodeSelected = qgraphicsitem_cast<Node *>(item) ) {
                    emit userNodeMoved(nodeSelected->nodeNumber(),
                                          nodeSelected->x(),
                                          nodeSelected->y());
                }
            }
            QGraphicsView::mouseReleaseEvent(e);
        }
        if (Edge *edge= qgraphicsitem_cast<Edge *>(item)) {
            Q_UNUSED(edge);
            qDebug() << "GW::mouseReleaseEvent() at "
                     << e->pos().x() << ","<< e->pos().y()
                     << " or "<<  p.x() << ","<<p.y()
                     << "On an edge. Selected items: "
                     << selectedItems().count();
            QGraphicsView::mouseReleaseEvent(e);
            return;
        }
    }
    else{
        qDebug() << "GW::mouseReleaseEvent() at "
                 << e->pos().x() << ","<< e->pos().y()
                 << " or "<<  p.x() << ","<<p.y()
                 <<"on empty space. Selected items: "
                << selectedItems().count();

    }


}





/** 
    Calls the scaleView() when the user spins the mouse wheel
    It passes delta as new m_scale
*/
void GraphicsWidget::wheelEvent(QWheelEvent *e) {
    bool ctrlKey = (e->modifiers() == Qt::ControlModifier);
    qDebug("GW: Mouse wheel event");
    qDebug() << "GW: delta = " << e->delta();
    if (ctrlKey) {
        float m_scale = e->delta() / qreal(600);
        qDebug("GW: m_scale = %f", m_scale);
        if ( m_scale > 0)
            zoomIn(1);
        else  if ( m_scale < 0)
            zoomOut(1);
        else m_scale=1;
    }

}


/**
 * @brief GraphicsWidget::zoomOut
 * @param level
 * Called from MW magnifier button
 */
void GraphicsWidget::zoomOut (int level){

    qDebug() << "GW: ZoomOut(): zoom index "<< m_zoomIndex
             << " - level " << level;
    m_zoomIndex-=level;
    if (m_zoomIndex <= 0) {
        m_zoomIndex = 0;
    }
    emit zoomChanged(m_zoomIndex);

}



/**
 * @brief GraphicsWidget::zoomIn
 * @param level
 * Called from MW magnifier button
 */
void GraphicsWidget::zoomIn(int level){

    qDebug() << "GW: ZoomIn(): index "<< m_zoomIndex << " + level " << level;
    m_zoomIndex+=level;
    if (m_zoomIndex > 500) {
        m_zoomIndex=500;
    }
    if (m_zoomIndex < 0) {
        m_zoomIndex = 0;
    }
    emit zoomChanged(m_zoomIndex);
}





/**
 * @brief GraphicsWidget::changeMatrixScale
 * @param value
 * Initiated from MW zoomSlider and rotateSlider widgets
 */
void GraphicsWidget::changeMatrixScale(int value) {
    transformationActive = true;
    qreal scaleFactor = pow(qreal(2), ( value - 250) / qreal(50) );
    m_currentScaleFactor = scaleFactor ;
    qDebug() << "GW: changeMatrixScale(): value " <<  value
             << " m_currentScaleFactor " << m_currentScaleFactor
              << " m_currentRotationAngle " << m_currentRotationAngle;

    resetMatrix();
    scale(m_currentScaleFactor, m_currentScaleFactor);
    rotate(m_currentRotationAngle);

}


/**
 * @brief GraphicsWidget::rotateLeft
 */
void GraphicsWidget::rotateLeft(){
    m_currentRotationAngle-=5;
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief GraphicsWidget::rotateRight
 */
void GraphicsWidget::rotateRight() {
    m_currentRotationAngle+=5;
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief GraphicsWidget::changeMatrixRotation
 * @param angle
 */
void GraphicsWidget::changeMatrixRotation(int angle){
    transformationActive = true;
    m_currentRotationAngle = angle;
    qDebug() << "GW: changeMatrixRotation(): angle " <<  angle
              << " m_currentRotationAngle " << m_currentRotationAngle
              << " m_currentScaleFactor " << m_currentScaleFactor;
    resetMatrix();
    scale(m_currentScaleFactor, m_currentScaleFactor);
    rotate(angle);

}

/**
 * @brief GraphicsWidget::reset
 * Resets the transformation matrix to the identity matrix ( default zoom and scale )
 */
void GraphicsWidget::reset() {
    m_currentRotationAngle=0;
    m_currentScaleFactor = 1;
    m_zoomIndex=250;
    emit zoomChanged(m_zoomIndex);
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief GraphicsWidget::resizeEvent
 * @param e
 * Repositions guides then emits resized() signal to MW and eventually Graph
 * which does the node repositioning maintaining proportions
 */
void GraphicsWidget::resizeEvent( QResizeEvent *e ) {
    if (transformationActive)  {
        transformationActive = false;
        return;
    }
    int w=e->size().width();
    int h=e->size().height();
    int w0=e->oldSize().width();
    int h0=e->oldSize().height();
    fX=  (double)(w)/(double)(w0);
    fY= (double)(h)/(double)(h0);
    foreach (QGraphicsItem *item, scene()->items()) {
        if ( (item)->type() == TypeGuide ){
            if (Guide *guide = qgraphicsitem_cast<Guide *>  (item) ) {
                if (guide->isCircle()) {
                    guide->die();
                    guide->deleteLater ();
                    delete item;
                }
                else {
                    qDebug()<< "GW::resizeEvent() - Horizontal Guide "
                            << " original position ("
                            <<  guide->x() << "," << guide->y()
                             << ") - width " << guide->width()
                             << ") - will move to ("
                             << guide->x()*fX << ", " << guide->y()*fY << ")"
                                << " new width " << (int) ceil( (guide->width() *fX ));
                    guide->setHorizontalLine(
                                mapToScene(guide->pos().x()*fX,
                                           guide->pos().y()*fY),
                                (int) ceil( (guide->width() *fX )));
                }
            }
        }
    }

    //update the scene width and height with that of the graphicsWidget
    scene()->setSceneRect(0, 0, (qreal) ( w ), (qreal) ( h  ) );

    qDebug () << "GW::resizeEvent() - old size: ("
              << w0 << "," << h0
              << ") - new size: (" << w << "," << h << ")"
              << " fX,fY: (" <<  fX << ","<< fY
              << ") scene size: ("
             << scene()->width() << "," << scene()->height() << ")";

    emit resized( w ,  h );
}



/** 
    Destructor.
*/
GraphicsWidget::~GraphicsWidget(){
    clear();
}

