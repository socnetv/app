/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt

                        graphicswidget.cpp description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
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
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "graphicsnodenumber.h"
#include "graphicsnodelabel.h"
#include "graphicsguide.h"
#include "graphicsedgeweight.h"
#include "graphicsedgelabel.h"

/** 
    Constructor method. Called when a GraphicsWidget object is created in MW
*/

GraphicsWidget::GraphicsWidget(QGraphicsScene *sc, MainWindow* m_parent)  :
        QGraphicsView ( sc,m_parent) {

        qDebug() << "GW::GraphicsWidget(*sc, *MW)";

        qRegisterMetaType<SelectedEdge>("SelectedEdge");
        qRegisterMetaType<QList<SelectedEdge> >();

        secondDoubleClick=false;
        transformationActive = false;
        m_nodeLabel="";
        m_zoomIndex=250;
        m_currentScaleFactor = 1;
        m_currentRotationAngle = 0;
        markedNodeExist=false; //used in findNode()
        clickedEdge=0;
//        edgesHash.reserve(1000);
//        nodeHash.reserve(1000);

        m_edgeHighlighting = true;
        m_edgeMinOffsetFromNode=6;
        m_nodeNumberVisibility = true;
        m_nodeLabelVisibility = true;


        /* "QGraphicsScene applies an indexing algorithm to the scene, to speed up
         * item discovery functions like items() and itemAt().
         * Indexing is most efficient for static scenes (i.e., where items don't move around).
         * For dynamic scenes, or scenes with many animated items, the index bookkeeping
         * can outweight the fast lookup speeds."
         * The user can change this from Settings.
        */
        scene() -> setItemIndexMethod(QGraphicsScene::BspTreeIndex); //NoIndex (for anime)

        connect ( scene() , &QGraphicsScene::selectionChanged,
                     this, &GraphicsWidget::getSelectedItems);



}


/**
 * @brief creates a QString of the edge name - used for indexing edgesHash
 * @param v1
 * @param v2
 * @param relation
 * @return
 */
QString GraphicsWidget::createEdgeName(const int &v1, const int &v2, const int &relation) {
    edgeName = QString::number((relation != -1) ? relation : m_curRelation) + QString(":")
            + QString::number(v1) + QString(">")+ QString::number(v2);
    return edgeName;
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
 * @brief Clears the scene and all hashes, lists, var etc
 */
void GraphicsWidget::clear() {
    qDebug() << "GW::clear() - clearing hashes";
    nodeHash.clear();
    edgesHash.clear();
    m_selectedNodes.clear();
    m_selectedEdges.clear();
    scene()->clear();
    m_curRelation=0;
    markedNodeExist=false;
    clickedEdge=0;
    firstNode=0;
    secondDoubleClick=false;
    qDebug() << "GW::clear() - finished clearing hashes";
}



/**
 * @brief Changes the current relation
 * Called from Graph::signalRelationChangedToGW(int) signal.
  * @param relation
 */
void GraphicsWidget::relationSet(int relation) {
    qDebug() << "GraphicsWidget::relationSet() to " << relation;
    m_curRelation = relation;
}



/**
 * @brief Adds a new node onto the scene
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
                               const QString &numberColor, const int &numberSize,
                               const int &numberDistance,
                               const QString &nodeLabel,
                               const QString &labelColor, const int &labelSize,
                               const int &labelDistance,
                               const QPointF &p
                                ) {
    qDebug()<< "GW::drawNode() - Draw new node:" << num
            << " at:" << p.x() << ", "<< p.y() ;

    // Draw node
    GraphicsNode *jim= new GraphicsNode (
                this, num, nodeSize, nodeColor, nodeShape,
                m_nodeNumberVisibility, m_nodeNumbersInside, numberColor, numberSize, numberDistance,
                m_nodeLabelVisibility, nodeLabel, labelColor, labelSize, labelDistance,
                m_edgeHighlighting,
                p
                );

    // Add new node to a container to ease finding, edge creation etc
    nodeHash.insert(num, jim);
}







/**
 * @brief Draws an edge from source to target Node.
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

    edgeName = createEdgeName(source, target);

    qDebug()<<"GW::drawEdge() - "<< edgeName
           << "weight:"<<weight
           << "label:" << label
           << "direction type:" << type
           << " - nodeHash reports "<< nodeHash.size()<<" nodes.";

    if (type != EDGE_RECIPROCATED ) {

        GraphicsEdge *edge=new GraphicsEdge (
                    this,
                    nodeHash.value(source), nodeHash.value(target),
                    weight, label, color,
                    Qt::SolidLine,
                    type,
                    drawArrows,
                    (source==target) ? true: bezier,
                    weightNumbers,
                    m_edgeHighlighting);

        edgesHash.insert(edgeName, edge);
    }
    else {
        // if type is EDGE_RECIPROCATED, we just need to change the direction type
        // of the existing opposite edge.
        edgeName = createEdgeName(target,source);
        qDebug()<< "GW::drawEdge() - Reciprocating existing directed edge"<<edgeName;
        edgesHash.value(edgeName)->setDirectionType(type);

    }
    //	qDebug()<< "Scene items now: "<< scene()->items().size() << " - GW items now: "<< items().size();
}





/**
 * @brief Creates a new edge, when the user middle-clicks on two nodes consecutively
 * On the first middle-click, it saves the first node (source).
 * On the second middle-click, it saves the second node as target and emits the signal
 * userMiddleClicked() to MW which will notify activeGraph,
 * which in turn will signal back to drawEdge().
 * @param node
 */
void GraphicsWidget::startEdge(GraphicsNode *node){
    if (secondDoubleClick){
        qDebug()<< "GW::startEdge() - this is the second double click. "
                   "Emitting userMiddleClicked() to create edge";
        secondNode=node;
        emit userMiddleClicked(firstNode->nodeNumber(), secondNode->nodeNumber() );
        emit setCursor(Qt::ArrowCursor);
        secondDoubleClick=false;
    }
    else{
        qDebug()<<"GW::startEdge() - this is the first double click.";
        firstNode=node;
        secondDoubleClick=true;
        emit setCursor( Qt::PointingHandCursor );
    }
}



/**
 * @brief Called when the user clicks or double-clicks on a node.
 * Clears clickedEdge and emits the userClickedNode signal to Graph to
    - display node info on the status bar
    - notify context menus for the clicked node.
 * @param node
 */
void GraphicsWidget::nodeClicked(GraphicsNode *node){
    qDebug () << "GW::nodeClicked() - Emitting userClickedNode()";
    if (clickedEdge) edgeClicked(0);
    emit userClickedNode(node->nodeNumber());
}



/**
 * @brief Called when the user clicks on an edge.
    Emits the userClickedEdge signal to Graph which is used to:
    - display edge info on the status bar
    - notify context menus for the clicked edge.
    Also, it highlights source and target nodes
 * @param edge
 */
void GraphicsWidget::edgeClicked(GraphicsEdge *edge, const bool &openMenu){
    qDebug() <<"### GW::edgeClicked()";
    if (edge) {
        if (clickedEdge) {
            if (clickedEdge != edge) {
                qDebug() <<"### GW::edgeClicked() - unmarking previously clicked edge";
                clickedEdge->setClicked(false);
                qDebug() <<"### GW::edgeClicked() - marking newly clicked edge";
                clickedEdge=edge;
                clickedEdge->setClicked(true);

            }
            else if (clickedEdge == edge && !openMenu){
                clickedEdge->setClicked(false);
            }

        }
        else {
            qDebug() <<"### GW::edgeClicked() - no previously clicked edge";
            qDebug() <<"### GW::edgeClicked() - setting new clicked edge";
            clickedEdge=edge;
            edge->setClicked(true);
        }

        qDebug() <<"### GW::edgeClicked() - emitting userClickedEdge() to MW";
        emit userClickedEdge(edge->sourceNode()->nodeNumber(),
                             edge->targetNode()->nodeNumber(),
                             openMenu);

    }
    else {
        if (clickedEdge) {
            qDebug() <<"### GW::edgeClicked() - with zero parameters. Clearing previously clicked edge";
            clickedEdge->setClicked(false);
        }

        qDebug() <<"### GW::edgeClicked() - with zero parameters. Unsetting clickedEdge";
        clickedEdge=0;
        emit userClickedEdge(0,0,openMenu);

    }



}






/**
 * @brief Called from activeGraph to update node coordinates on the canvas
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
 * @brief Removes a node from the scene.
 * Called from Graph signalEraseNode(int)
 * @param number
 */
void GraphicsWidget::removeNode(const long int &number){
        qDebug() << "GW::removeNode() - node " << number
                 << " scene items: " << scene()->items().size()
                 << " view items: " << items().size()
                 << " nodeHash items: "<< nodeHash.count();

    if ( nodeHash.contains(number) ) {
        qDebug() << "GW::removeNode() - found node"
                 <<  number<< " Deleting :)" ;

        if ( nodeHash.value(number) == markedNode1 ) {
            qDebug() << "GW::removeNode() - node marked by the user. Unmarking then deleting.";
            setMarkedNode("");

        }
        else {
            qDebug() << "GW::removeNode() - node not marked by the user. Deleting.";
        }
        delete nodeHash.value(number);
    }

    qDebug() << "GW::removeNode() - node erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size()
             << " nodeHash items: "<< nodeHash.count()
             << " edgesHash items: "<< edgesHash.count() ;
}




/**
 * @brief Remove an edge from the graphics widget.
 * Called from MW/Graph when erasing edges using vertex numbers
 * Also called when transforming directed edges to undirected.
 * @param sourceNode
 * @param targetNode
 */
void GraphicsWidget::removeEdge(const long int &source,
                                const long int &target,
                                const bool &removeOpposite){

    edgeName = createEdgeName(source,target);

    qDebug() << "GW::removeEdge() - " << edgeName
             << "removeOpposite"<<removeOpposite
             << " scene items: " << scene()->items().size()
             << " view items: " << items().size()
             << " edgesHash.count: " << edgesHash.count();

    if ( edgesHash.contains(edgeName) ) {
        int directionType = edgesHash.value(edgeName)->directionType();
        delete edgesHash.value(edgeName);
        if (directionType == EDGE_RECIPROCATED) {
            if (!removeOpposite) {
                drawEdge(target, source, 1,"");
            }
        }
            qDebug() << "GW::removeEdge() - Deleted edge" << edgeName
                 << " scene items: " << scene()->items().size()
                 << " view items: " << items().size()
                 << " edgesHash.count: " << edgesHash.count();

    }
    else {
        //check opposite edge. If it exists, then transform it to directed
        edgeName = createEdgeName(target, source);
        qDebug() << "GW::removeEdge() - Edge did not exist, checking for opposite:"
                 << edgeName;
        if ( edgesHash.contains(edgeName) ) {
            qDebug() << "GW::removeEdge() - Opposite edge exists. Check if it is reciprocated";
            if ( edgesHash.value(edgeName)->directionType() == EDGE_RECIPROCATED ) {
                edgesHash.value(edgeName)->setDirectionType(EDGE_DIRECTED);
                return;
            }
        }
        qDebug() << "GW::removeEdge() - No such edge to delete";
    }


}





/**
 * @brief Removes a node item from the scene.
 * Called from GraphicsNode::~GraphicsNode() to remove itself from nodeHash, scene and
 * be deleted
 * @param node
  */
void GraphicsWidget::removeItem( GraphicsNode *node){
    long int i=node->nodeNumber();
    qDebug() << "GW::removeItem(node) - number: " <<  i;
    if (firstNode == node) {
        qDebug() << "GW::removeItem(node) - number: " <<  i
                 << "previously set as source node for a new edge. Unsetting.";
        secondDoubleClick = false;
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
 * @brief Removes an edge item from the scene.
 * Called from GraphicsEdge::~GraphicsEdge() to remove itself from edgesHash and scene and
 * be deleted
 * @param edge
 *
 */
void GraphicsWidget::removeItem( GraphicsEdge * edge){
    qDebug() << "GW::removeItem(edge) - calling edgeClicked(0)" ;
    edgeClicked(0);
    edgeName = createEdgeName(edge->sourceNodeNumber(), edge->targetNodeNumber() ) ;
    qDebug() << "GW::removeItem(edge) - removing edge from edges hash" ;
    edgesHash.remove(edgeName);
    qDebug() << "GW::removeItem(edge) - removing edge scene" ;
    scene()->removeItem(edge);
    qDebug() << "GW::removeItem(edge) - calling edge->deleteLater()" ;
    edge->deleteLater();
    qDebug() << "GW::removeItem(edge) - edge erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}


/**
 * @brief Removes an edge weight number item from the scene.
 * @param edgeWeight
 */
void GraphicsWidget::removeItem( GraphicsEdgeWeight *edgeWeight){
    qDebug() << "GW::removeItem(edgeWeight) -  of edge"
             << "GW items now:  " << items().size();
    scene()->removeItem(edgeWeight);
    edgeWeight->deleteLater();
    qDebug() << "GW::removeItem(edgeWeight) - edgeWeight erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}



/**
 * @brief Removes an edge label item from the scene.
 * @param edgeLabel
 */
void GraphicsWidget::removeItem( GraphicsEdgeLabel *edgeLabel){
    qDebug() << "GW::removeItem(edgeLabel) -  of edge"
             << "GW items now:  " << items().size();
    scene()->removeItem(edgeLabel);
    edgeLabel->deleteLater();
    qDebug() << "GW::removeItem(edgeLabel) - edgeLabel erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();
}


/**
 * @brief Removes a node label item from the scene.
 * @param nodeLabel
 */
void GraphicsWidget::removeItem( GraphicsNodeLabel *nodeLabel){
    qDebug() << "GW::removeItem(label) -  of node " <<  nodeLabel->node()->nodeNumber()
             << "GW items now:  " << items().size();
    scene()->removeItem(nodeLabel);
    nodeLabel->deleteLater();
    qDebug() << "GW::removeItem(label) - label erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();


}


/**
 * @brief Removes a node number item from the scene.
 * @param nodeNumber
 */
void GraphicsWidget::removeItem( GraphicsNodeNumber *nodeNumber){
    qDebug() << "GW::removeItem(number) -  of node " <<  nodeNumber->node()->nodeNumber()
             << "GW items now:  " << items().size();
    scene()->removeItem(nodeNumber);
    nodeNumber->deleteLater();
    qDebug() << "GW::removeItem(number) - number erased! "
             << " scene items now: " << scene()->items().size()
             << " view items: " << items().size();

}






/**
 * @brief Sets the color of an node.
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
 * @brief Sets the shape of an node.
    Called from MW when the user changes the shape of a node
 * @param nodeNumber
 * @param shape
 * @return
 */
bool GraphicsWidget::setNodeShape(const long &nodeNumber, const QString &shape){
    qDebug() << "GW::setNodeShape() : " << shape;
    nodeHash.value(nodeNumber) -> setShape(shape);
    return true;

}




/**
 * @brief GraphicsWidget::setNodeNumberVisibility
 * @param toggle
 */
void GraphicsWidget::setNodeNumberVisibility(const bool &toggle){
    qDebug()<< "GW::setNodeNumberVisibility()" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setNumberVisibility(toggle);
    }
    m_nodeNumberVisibility = toggle;
}


/**
 * @brief GraphicsWidget::setNodeLabelsVisibility
 * @param toggle
 */
void GraphicsWidget::setNodeLabelsVisibility (const bool &toggle){
    qDebug()<< "GW::setNodeLabelsVisibility()" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setLabelVisibility(toggle);
    }
    m_nodeLabelVisibility = toggle;
}


/**
 * @brief Sets the label of an node. Called from MW.
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
 * @brief Toggles node numbers displayed inside or out of nodes
 * Called from MW
 * @param numIn
 */
void   GraphicsWidget::setNumbersInsideNodes(const bool &toggle){
    qDebug()<< "GW::setNumbersInsideNodes" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setNumberInside(toggle);
    }
    m_nodeNumbersInside=toggle;
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
 * @brief Passes initial zoom setting
 * Called from MW on startup
 * @param zoomIndex
 */
void GraphicsWidget::setInitZoomIndex(int zoomIndex) {
    m_zoomIndex = zoomIndex;
}







/**
*	Changes the visibility of a Node
*/
void GraphicsWidget::setNodeVisibility(long int number, bool toggle){
    if  ( nodeHash.contains (number) ) {
        qDebug() << "GW::setNodeVisibility() - node"
                 << number << " set to " << toggle;
        nodeHash.value(number) -> setVisible(toggle);
        nodeHash.value(number) -> setEnabled(toggle);
        return;
    }
    qDebug() << "GW: setNodeVisibility(): cannot find node " << number;
}


/**
 * @brief Changes the size of a node
 * @param number
 * @param size
 * @return
 */
bool GraphicsWidget::setNodeSize(const long int &number, const int &size ){
    qDebug () << "GW::setNodeSize() node: "<< number
              << " new size "<< size;
    if  ( nodeHash.contains (number) ) {
        if (size>0){
            qDebug() << "GW::setNodeSize(): for "<< number << " to " << size ;
            nodeHash.value(number) -> setSize(size);
            return true;

        }
        else {
            qDebug() << "GW::setNodeSize(): for "<< number
                     << " to initial size" << m_nodeSize;
            nodeHash.value(number) -> setSize(m_nodeSize);
            return true;

        }
    }
    qDebug() << "GW::setNodeSize(): cannot find node " << number;
    return false;
}

/**
 * @brief GraphicsWidget::setAllNodeSize
 * @param size
 * @return
 */
void GraphicsWidget::setAllNodeSize(const int &size ){
    qDebug() << "GW::setAllNodeSize() ";
    foreach ( GraphicsNode *m_node, nodeHash ) {
            qDebug() << "GW::setAllNodeSize(): "<< m_node->nodeNumber() << " to new size " << size ;
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
GraphicsNode* GraphicsWidget::hasNode( QString text ){

    bool ok = false;
    foreach ( GraphicsNode *candidate, nodeHash) {
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

    edgeName = createEdgeName( source, target );

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

    edgeName =  createEdgeName( source, target );

    qDebug()<<"GW::setEdgeColor() -" << edgeName <<  " new color "  << color;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName) -> setColor(color);
    }

}






/**
 * @brief Changes the direction type of an existing edge
  * @param source
 * @param target
 * @param directionType
 * @return
 */
bool GraphicsWidget::setEdgeDirectionType(const long int &source,
                                          const long int &target,
                                          const int &dirType){
    qDebug() << "GW::setEdgeDirectionType() : "
             << source
             << "->" << target
             << "type" << dirType;

    edgeName = createEdgeName( source, target );
    qDebug()<<"GW::setEdgeDirectionType() - checking edgesHash for:" << edgeName ;

    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<"GW::setEdgeDirectionType() - edge exists in edgesHash. "
                  << " Transforming it to reciprocated";
        edgesHash.value(edgeName) -> setDirectionType(dirType);

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

    edgeName = createEdgeName( source, target );

    qDebug()<<"GW::setEdgeWeight() -" << edgeName <<  " new weight "  << weight;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName) -> setWeight(weight);
        return true;
    }

    else {
        //check opposite edge. If it exists, then transform it to directed
        edgeName = createEdgeName(target, source);
        qDebug() << "GW::setEdgeWeight() - Edge did not exist, checking for opposite:"
                 << edgeName;
        if ( edgesHash.contains(edgeName) ) {
            qDebug() << "GW::setEdgeWeight() - Opposite edge exists. Check if it is reciprocated";
            edgesHash.value(edgeName) -> setWeight(weight);
            return true;
        }
        qDebug() << "GW::setEdgeWeight() - No such edge to delete";
    }

    return false;

}



void GraphicsWidget::setEdgeArrowsVisibility(const bool &toggle){
    qDebug()<< "GW::setEdgeArrowsVisibility()" << toggle;
    QList<QGraphicsItem *> list = scene()->items();
    for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
        if ( (*item)->type() ==TypeEdge){
            GraphicsEdge *edge = (GraphicsEdge*) (*item);
            edge->showArrows(toggle);
        }
    }

}



/**
 * @brief Changes the Offset of an edge (or all edges) from source and target nodes.
 * @param source
 * @param target
 * @param offset
 */
void GraphicsWidget::setEdgeOffsetFromNode(const long int &source,
                                           const long int &target,
                                           const int &offset){
    qDebug() << "GW::setEdgeOffsetFromNode() : " << source << "->" << target
             << " = " << offset;

    if (source && target) {

        QString edgeName =  QString::number(m_curRelation) + QString(":") +
                QString::number( source ) + QString(">")+ QString::number( target );

        qDebug()<<"GW::setEdgeWeight() -" << edgeName <<  " new offset "  << offset;
        if  ( edgesHash.contains (edgeName) ) {
            edgesHash.value(edgeName) -> setMinimumOffsetFromNode(offset);
            return;
        }


    }
    // if source == target == 0, then we change all edges'offset.
    else {
        QList<QGraphicsItem *> list = scene()->items();
        for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
            if ( (*item)->type() ==TypeEdge){
                GraphicsEdge *edge = (GraphicsEdge*) (*item);
                edge->setMinimumOffsetFromNode(offset);
            }
        }


    }


}

void GraphicsWidget::setEdgeWeightNumbersVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeWeightNumbersVisibility()" << toggle;
    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->setWeightNumberVisibility(toggle);
    }
}




void GraphicsWidget::setEdgeLabelsVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeLabelsVisibility()" << toggle
               << "edgesHash.count: " << edgesHash.count();
    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->setLabelVisibility(toggle);
    }
}





/**
 * @brief Toggles edge highlighting when hovering over a single edge or all edges
 * connected to a node when the user hovers over that node.
 * Called from MW
 * @param numIn
 */
void   GraphicsWidget::setEdgeHighlighting(const bool &toggle){
    qDebug()<< "GW::setEdgeHighlighting" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setEdgeHighLighting(toggle);
    }

    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->setHighlighting(toggle);
    }

    m_edgeHighlighting = toggle;
}


/**
*	Changes the visibility of an GraphicsView edge (number, label, edge, etc)
*/
void GraphicsWidget::setEdgeVisibility(int relation, int source, int target, bool toggle){

    edgeName = createEdgeName( source, target, relation );

    qDebug()<<"GW::setEdgeVisibility() - trying to set edge"<<edgeName<<"to"<<toggle;

    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<"GW::setEdgeVisibility() - edge" << edgeName
               << "set to" << toggle;
        edgesHash.value(edgeName) -> setVisible(toggle);
        edgesHash.value(edgeName) -> setEnabled(toggle);
        return;
    }
    edgeName = createEdgeName( target, source, relation );
    qDebug()<<"GW: setEdgeVisibility() - trying to set edge"<<edgeName<<"to"<<toggle;
    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<"GW::setEdgeVisibility() - reciprocated edge" << edgeName
               << "set to" << toggle;
        edgesHash.value(edgeName) -> setVisible(toggle);
        edgesHash.value(edgeName) -> setEnabled(toggle);
        return;
    }
    qDebug()<<"GW::setEdgeVisibility() - Cannot find edge" << edgeName << "or the opposite in the edgesHash";

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
    GraphicsGuide *circ=new GraphicsGuide (this, x0, y0, radius);
    circ->show();

}


void GraphicsWidget::addGuideHLine(const double &y0){
    GraphicsGuide *line=new GraphicsGuide (this, y0, this->width());
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
            GraphicsGuide *guide = qgraphicsitem_cast<GraphicsGuide *>  (*item);
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
 * @brief Clears any clickedNode info and sets a selection rectangle
 * in the scene, which signals QGraphicsScene::selectionChanged signal to update
 * selectedNodes and selectedEdges.
 * Called from MW.
 */
void GraphicsWidget::selectAll(){
    QPainterPath path;
    path.addRect(0,0, this->scene()->width() , this->scene()->height());
    scene()->setSelectionArea(path);
    emit userClickedNode(0);
    qDebug() << "GraphicsWidget::selectAll() - selected items now: "
             << selectedItems().count();
}



/**
 * @brief Clears any clickedNode info and any previous selection rect
 * in the scene, which again signals selectionChanged() to update selectedNodes
 * and selectedEdges to zero.
 * Called from MW.
 */
void GraphicsWidget::selectNone(){
    qDebug() << "GraphicsWidget::selectNone()";
    emit userClickedNode(0);
    scene()->clearSelection();

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

    qDebug() <<"GW::getSelectedItems() -"
            << "selectedNodes"<< selectedNodes()
            << "selectedEdges"<< selectedEdges();

    emit userSelectedItems(selectedNodes(), selectedEdges());

}



/**
 * @brief Returns a QList of all selected QGraphicsItem(s)
 * @return a QList of all selected QGraphicsItem(s)
 */
QList<QGraphicsItem *> GraphicsWidget::selectedItems(){
    qDebug() <<"GW::selectedItems()";
    return scene()->selectedItems();
}


/**
 * @brief Returns a QList of selected node numbers
 * Called by GW::getSelectedItems and MW::selectedNodes
 * @return a QList of integers: the selected node numbers
 */
QList<int> GraphicsWidget::selectedNodes() {

    m_selectedNodes.clear();
    foreach (QGraphicsItem *item, scene()->selectedItems()) {
        if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item) ) {
            m_selectedNodes.append(node->nodeNumber());
        }
    }

    qDebug() <<"GW::selectedNodes() - " << m_selectedNodes.count();

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
        if (GraphicsEdge *edge= qgraphicsitem_cast<GraphicsEdge *>(item) ) {
            SelectedEdge selEdge = qMakePair( edge->sourceNodeNumber(), edge->targetNodeNumber());
            m_selectedEdges << selEdge;
        }
    }
    qDebug() <<"GW::selectedEdges() - " << m_selectedEdges.count();
    return m_selectedEdges;
}




/**
 * @brief Starts the new node creation process when the user double-clicks somewhere:
    Emits userDoubleClicked to Graph::vertexCreate(),
    which in turn calls this->drawNode() to create and displays node info on MW status bar
    Yes, it's a full circle!
 * @param e
 */
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {

    if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
        if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {
            qDebug() << "GW::mouseDoubleClickEvent() - on a node!"
                     << "Scene items:"<< scene()->items().size()
                     << "GW items:" << items().size()
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
        qDebug() << "GW::mouseDoubleClickEvent() - on something, not a node!"
                 << "Scene items:"<< scene()->items().size()
                 << "GW items:" << items().size();

    }

    QPointF p = mapToScene(e->pos());
    qDebug()<< "GW::mouseDoubleClickEvent() - on empty space. "
            << "Scene items:"<< scene()->items().size()
            << " GW items:" << items().size()
            << " Emit userDoubleClickNewNode to create new vertex at:"
            << e->pos() << "~"<< p;
    emit userDoubleClickNewNode(p);

}



void GraphicsWidget::mousePressEvent( QMouseEvent * e ) {

    QPointF p = mapToScene(e->pos());

    // bool ctrlKey = (e->modifiers() == Qt::ControlModifier);

    if ( QGraphicsItem *item = itemAt(e->pos() )   ) {

        //
        // if user clicked on some item
        //

        if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {

            //
            // if user clicked on a node
            //

            qDebug() << "GW::mousePressEvent() - Single click on a node at:"
                     << e->pos() << "~"<< p
                     << "SelectedItems " << scene()->selectedItems().count()
                     << "Setting selected and emitting nodeClicked";

            node->setSelected(true);
            nodeClicked(node);

            if ( e->button()==Qt::RightButton ) {
                qDebug() << "GW::mousePressEvent() - Right-click on node. "
                            "Emitting openNodeMenu() ";
                emit openNodeMenu();
            }
            if ( e->button()==Qt::MidButton) {
                qDebug() << "GW::mousePressEvent() - Middle-click on node. "
                            "Calling startEdge() ";
                startEdge(node);
            }
            QGraphicsView::mousePressEvent(e);
            return;
        }

        if (GraphicsEdge *edge = qgraphicsitem_cast<GraphicsEdge *>(item)) {
            //
            // if user clicked on an edge
            //
            qDebug() << "GW::mousePressEvent() - Single click on edge at:"
                     << e->pos() << "~"<< p
                     << "SelectedItems:" << scene()->selectedItems().count();
            emit userClickedNode(0);
            if ( e->button()==Qt::LeftButton ) {
                qDebug() << "GW::mousePressEvent() - Left click on an edge ";
                edgeClicked(edge);
            }
            else if ( e->button()==Qt::RightButton ) {
                qDebug() << "GW::mousePressEvent() - Right click on an edge."
                         << "Emitting openEdgeContextMenu()";
                edgeClicked(edge, true);
            }
            else {
                edgeClicked(edge);
            }
            QGraphicsView::mousePressEvent(e);
            return;
        }
    }    
    else {
        if ( e->button() == Qt::RightButton   ) {
            //
            // user clicked on empty space, but with right button
            // so we open the context menu
            //
            qDebug() << "GW::mousePressEvent() - Right click on empty space at:"
                     << e->pos() << "~"<< p
                     << "SelectedItems:" << scene()->selectedItems().count()
                     << "Emitting openContextMenu(p)";
            emit openContextMenu(p);
        }
        else {
            //
            //  user left-clicked on empty space
            //
            qDebug() << "GW::mousePressEvent() - Left click on empty space at:"
                     << e->pos() << "~"<< p
                     << "SelectedItems:" << scene()->selectedItems().count()
                     << "Emitting userClickOnEmptySpace(p)";
            edgeClicked(0);
            emit userClickOnEmptySpace(p);
        }
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
        if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {
            qDebug() << "GW::mouseReleaseEvent() - at:"
                     << e->pos() << "~"<< p
                     << "On a node. Selected items:"
                     << scene()->selectedItems().count()
                     << "Emitting userNodeMoved() for all selected nodes";
            Q_UNUSED(node);
            foreach (QGraphicsItem *item, scene()->selectedItems()) {
                if (GraphicsNode *nodeSelected = qgraphicsitem_cast<GraphicsNode *>(item) ) {
                    emit userNodeMoved(nodeSelected->nodeNumber(),
                                          nodeSelected->x(),
                                          nodeSelected->y());
                }
            }
            QGraphicsView::mouseReleaseEvent(e);
        }
        if (GraphicsEdge *edge= qgraphicsitem_cast<GraphicsEdge *>(item)) {
            Q_UNUSED(edge);
            qDebug() << "GW::mouseReleaseEvent() at:"
                     << e->pos() << "~"<< p
                     << "On an edge. Selected items:"
                     << scene()->selectedItems().count();
            QGraphicsView::mouseReleaseEvent(e);
            return;
        }
    }
    else{
        qDebug() << "GW::mouseReleaseEvent() at:"
                 << e->pos() << "~"<< p
                 <<"On empty space. Selected items:"
                << scene()->selectedItems().count();

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
 * @brief Changes the zoom level of the scene.
 * Called from MW magnifier button
 * @param level
 *
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
 * @brief Resets the transformation matrix to the identity matrix ( default zoom and scale )
 */
void GraphicsWidget::reset() {
    m_currentRotationAngle=0;
    m_currentScaleFactor = 1;
    m_zoomIndex=250;
    emit zoomChanged(m_zoomIndex);
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief Repositions guides then emits resized() signal to MW and eventually Graph
 * which does the node repositioning maintaining proportions
 * @param e
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
            if (GraphicsGuide *guide = qgraphicsitem_cast<GraphicsGuide *>  (item) ) {
                if (guide->isCircle()) {
                    guide->die();
                    guide->deleteLater ();
                    delete item;
                }
                else {
                    qDebug()<< "GW::resizeEvent() - Horizontal GraphicsGuide "
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
    qDebug() << "GW::~GraphicsWidget() - calling clear()";
    clear();
}

