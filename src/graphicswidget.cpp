/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                        graphicswidget.cpp description
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

#include "graphicswidget.h"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QtMath>
#include <QDebug>
#include <QWheelEvent>

#ifndef QT_NO_OPENGL
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#else
#include <QtWidgets>
#endif

#include "mainwindow.h"
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "graphicsnodenumber.h"
#include "graphicsnodelabel.h"
#include "graphicsguide.h"
#include "graphicsedgeweight.h"
#include "graphicsedgelabel.h"


/**
 * @brief Constructs a GraphicsWidget object
 * @param sc
 * @param m_parent
 */
GraphicsWidget::GraphicsWidget(QGraphicsScene *sc, MainWindow* m_parent)  :
        QGraphicsView ( sc,m_parent) {

        qDebug() << "Constructing GraphicsWidget...";

        qRegisterMetaType<SelectedEdge>("SelectedEdge");
        qRegisterMetaType<QList<SelectedEdge> >();
        qRegisterMetaType<QList<int> >();

        secondDoubleClick=false;
        m_isTransformationActive = false;
        m_nodeLabel="";

        m_width = 0;
        m_height = 0;
        m_zoomIndex=250;
        m_currentScaleFactor = 1;
        m_currentRotationAngle = 0;

        clickedEdge=0;
        edgesHash.reserve(150000);
        nodeHash.reserve(10000);

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
        scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex); //NoIndex (for anime)

        // Connect scene change signal to the slot that handles selected items
        connect ( scene() , &QGraphicsScene::selectionChanged,
                     this, &GraphicsWidget::handleSelectionChanged);


}



/**
 * @brief Toggles openGL
 *
 * @param enabled
 */
void GraphicsWidget::setOptionsOpenGL(const bool &enabled)
{
#ifndef QT_NO_OPENGL
    if (enabled) {
        QOpenGLWidget *gl = new QOpenGLWidget();
        QSurfaceFormat format;
        format.setSamples(4);
        gl->setFormat(format);
        setViewport(gl);
        qDebug() << "Enabled openGL in GraphicsWidget.";
    }
    else {
        setViewport(new QWidget);
        qDebug() << "Disabled openGL in GraphicsWidget.";
    }
#else
    qWarning() << "No OpenGL support! Cannot enable OpenGL in GraphicsWidget.";
#endif
}


/**
 * @brief Toggles QPainter render hints for primitive edges and text antialiasing
 *
 * @param toggle
 */
void GraphicsWidget::setOptionsAntialiasing(const bool &toggle)
{
    setRenderHint(QPainter::Antialiasing, toggle );
    setRenderHint(QPainter::TextAntialiasing, toggle );
}


/**
 * @brief Toggles QGraphicsView's antialiasing auto-adjustment of exposed areas.
 *
 * Default: false
 * Items that render antialiased lines on the boundaries of their QGraphicsItem::boundingRect()
 * can end up rendering parts of the line outside.
 * To prevent rendering artifacts, QGraphicsView expands all exposed regions by 2 pixels in all directions.
 * If you enable this flag, QGraphicsView will no longer perform these adjustments, minimizing the areas that require redrawing, which improves performance. A common side effect is that items that do draw with antialiasing can leave painting traces behind on the scene as they are moved.
 *
 * @param toggle
 */
void GraphicsWidget::setOptionsNoAntialiasingAutoAdjust(const bool &toggle)
{
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, toggle);

}




/**
 * @brief Creates a QString with the edge name.
 *
 * Edge names are used in edgesHash
 *
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
 * @brief Clears the scene and all hashes, lists, var etc
 */
void GraphicsWidget::clear() {
    qDebug() << "Clearing graphics widget data and resetting scene bounding rect to zero...";
    nodeHash.clear();
    edgesHash.clear();
    m_selectedNodes.clear();
    m_selectedEdges.clear();
    scene()->clear();
    m_curRelation=0;
    clickedEdge=0;
    firstNode=0;
    secondDoubleClick=false;
    scene()->setSceneRect(0, 0, 0, 0);
    qDebug() << "Finished clearing graphics widget";
}



/**
 * @brief Changes the current relation
 *
 * @param relation
 */
void GraphicsWidget::relationSet(int relation) {
    qDebug() << "Setting relation to" << relation;
    m_curRelation = relation;
}



/**
 * @brief Draws a new node in the scene
 *
 * Called when we load files, or when the user presses "Add Node" button or
 * the user double clicks on the canvas.
 *
 * @param nodeNum
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
void GraphicsWidget::drawNode( const QPointF &p,
                               const int &nodeNum,
                               const int &nodeSize,
                               const QString &nodeShape,
                               const QString &nodeIconPath,
                               const QString &nodeColor,
                               const QString &numberColor, const int &numberSize,
                               const int &numberDistance,
                               const QString &nodeLabel,
                               const QString &labelColor, const int &labelSize,
                               const int &labelDistance

                               ) {
    qDebug()<< "Will draw new node:" << nodeNum
            << " at:" << p.x() << ", "<< p.y()
            << "shape"<<nodeShape
            << "nodeIconPath"<<nodeIconPath;

    // Draw node
    GraphicsNode *jim= new GraphicsNode ( this,
                                          nodeNum,
                                          nodeSize,
                                          nodeColor,
                                          nodeShape,
                                          nodeIconPath,
                                          m_nodeNumberVisibility,
                                          m_nodeNumbersInside,
                                          numberColor,
                                          numberSize,
                                          numberDistance,
                                          m_nodeLabelVisibility,
                                          nodeLabel,
                                          labelColor,
                                          labelSize,
                                          labelDistance,
                                          m_edgeHighlighting,
                                          p
                                          );

    // Add new node to a container to ease finding, edge creation etc
    nodeHash.insert(nodeNum, jim);
}





/**
 * @brief Draws a new edge in the scene, from node with sourceNum to node with targetNum.
 *
 * Used when we do not have references to nodes but only node numbers:
 * a) when we load a network file
 * b) when the user clicks on the AddLink button on the MW.
 *
 * @param sourceNum
 * @param targetNum
 * @param weight
 * @param label
 * @param color
 * @param type
 * @param drawArrows
 * @param bezier
 * @param weightNumbers
 */
void GraphicsWidget::drawEdge(const int &sourceNum, const int &targetNum,
                              const qreal &weight,
                              const QString &label,
                              const QString &color,
                              const int &type,
                              const bool &drawArrows,
                              const bool &bezier,
                              const bool &weightNumbers){

    edgeName = createEdgeName(sourceNum, targetNum);

    qDebug()<<"Will draw new edge"<< edgeName
           << "weight:"<<weight
           << "label:" << label
           << "direction type:" << type
           << " - nodeHash reports "<< nodeHash.size()<<" nodes.";

    if ( type != EdgeType::Reciprocated ) {

        GraphicsEdge *edge=new GraphicsEdge (
                    this,
                    nodeHash.value(sourceNum), nodeHash.value(targetNum),
                    weight, label, color,
                    Qt::SolidLine,
                    type,
                    drawArrows,
                    (sourceNum==targetNum) ? true: bezier,
                    weightNumbers,
                    m_edgeHighlighting);

        edgesHash.insert(edgeName, edge);
    }
    else {
        // if type is EdgeType::Reciprocated, we just need to change the direction type
        // of the existing opposite edge.
        edgeName = createEdgeName(targetNum,sourceNum);
        qDebug()<< "Reciprocating existing directed edge"<<edgeName;
        edgesHash.value(edgeName)->setDirectionType(type);

    }

}





/**
 * @brief Creates a new edge, when the user middle-clicks on two nodes consecutively
 *
 * On the first middle-click, it saves the first node (source).
 * On the second middle-click, it saves the second node as target and emits the signal
 * userMiddleClicked() to MW which will notify activeGraph,
 * which in turn will signal back to drawEdge().
 *
 * @param node
 */
void GraphicsWidget::startEdge(GraphicsNode *node){
    if (secondDoubleClick){
        qDebug()<< "Got second consecutive double click. "
                   "Emitting userMiddleClicked() to create edge";
        secondNode=node;
        emit userMiddleClicked(firstNode->nodeNumber(), secondNode->nodeNumber() );
        emit setCursor(Qt::ArrowCursor);
        secondDoubleClick=false;
    }
    else{
        qDebug()<<"Got first double click to create a new edge...";
        firstNode=node;
        secondDoubleClick=true;
        emit setCursor( Qt::PointingHandCursor );
    }
}



/**
 * @brief Clears clickedEdge and emits a signal to Graph.
 *
 * The signal is used to
 * - display node info on the status bar
 * - notify context menus for the clicked node.
 *
 * Called when the user clicks or double-clicks on a node.
 *
 * @param node
 */
void GraphicsWidget::setNodeClicked(GraphicsNode *node){
    if ( node ) {
        qDebug () << "Emitting userClickedNode()";
        if (clickedEdge) {
            qDebug () << "Also unsetting previously clicked edge";
            setEdgeClicked(0);
        }
        emit userClickedNode(
                    node->nodeNumber(),
                    QPointF(node->x(), node->y())
                   );
    }
}



/**
 * @brief Sets the clicked edge.
 *
 * Emits signal to Graph to:
 * - display edge info on the status bar
 * - notify context menus for the clicked edge.
 *
 * @param edge
 * @param openMenu
 */
void GraphicsWidget::setEdgeClicked(GraphicsEdge *edge, const bool &openMenu){
    if (edge) {
        qDebug() << "Setting new clicked edge"
                 << edge->sourceNode()->nodeNumber()
                 << "->"
                 << edge->targetNode()->nodeNumber()
                 << "signalling...";
        clickedEdge=edge;
        emit userClickedEdge(edge->sourceNode()->nodeNumber(),
                             edge->targetNode()->nodeNumber(),
                             openMenu);
    }
    else {
        qDebug() <<"Empty edge parameter. Unsetting clickedEdge...";
        clickedEdge=0;
        emit userClickedEdge(0,0,openMenu);
    }

}






/**
 * @brief Moves the node with the given number to the new coordinates
 *
 * Called while creating random networks.
 *
 * @param nodeNum
 * @param x
 * @param y
 */
void GraphicsWidget::moveNode(const int &nodeNum, const qreal &x, const qreal &y){
    qDebug() << "Moving node" << nodeNum << "to pos:" << x << "," << y;
    nodeHash.value(nodeNum)->setPos(x,y);
}



/**
 * @brief Deletes the node with the given number from the scene, if exists
 *
 * Called from Graph
 *
 * @param nodeNum
 */
void GraphicsWidget::removeNode(const int &nodeNum){
    if ( nodeHash.contains(nodeNum) ) {
        delete nodeHash.value(nodeNum);
        qDebug() << "Removed node with number:" << nodeNum;
    }
}




/**
 * @brief Removes the edge from node sourceNum to node targetNum from the scene.
 *
 * Called when erasing edges using vertex numbers.
 * Also called when transforming directed edges to undirected.
 *
 * @param sourceNum
 * @param targetNum
 * @param removeReverse
 */
void GraphicsWidget::removeEdge(const int &sourceNum,
                                const int &targetNum,
                                const bool &removeReverse){

    edgeName = createEdgeName(sourceNum, targetNum);

    if ( edgesHash.contains(edgeName) ) {
        int directionType = edgesHash.value(edgeName)->directionType();
        delete edgesHash.value(edgeName);
        qDebug() << "Removed edge" << edgeName;
        // Check if it was reciprocated
        if (directionType == EdgeType::Reciprocated) {
            // The deleted edge was reciprocated, draw the reverse edge.
            if (!removeReverse) {
                qDebug() << "Drawing the reverse edge.";
                drawEdge(targetNum, sourceNum, 1,"");

            }
        }
    }
    else {
        // Check opposite edge. If it exists, then transform it to directed
        edgeName = createEdgeName(targetNum, sourceNum);
        qDebug() << "Edge did not exist, checking for opposite:"
                 << edgeName;
        if ( edgesHash.contains(edgeName) ) {
            qDebug() << "Opposite edge exists. Check if it is reciprocated";
            if ( edgesHash.value(edgeName)->directionType() == EdgeType::Reciprocated ) {
                edgesHash.value(edgeName)->setDirectionType(EdgeType::Directed);
                return;
            }
        }
        qDebug() << "No such edge to remove";
    }


}





/**
 * @brief Removes a node item from the scene.
 *
 * Called from GraphicsNode::~GraphicsNode() to remove itself from nodeHash, scene and be deleted
 *
 * @param node
  */
void GraphicsWidget::removeItem( GraphicsNode *node){
    int i=node->nodeNumber();
    qDebug() << "Removing node with number: " <<  i;
    if (firstNode == node) {
        qDebug() << "Node" <<  i
                 << "previously set as source node for a new edge. Unsetting.";
        secondDoubleClick = false;
        emit setCursor(Qt::ArrowCursor);
    }
    nodeHash.remove(i);
    scene()->removeItem(node);
    node->deleteLater ();
    qDebug() << "Node removed. "
             << "scene items now: " << scene()->items().size()
             << "view items: " << items().size();
}





/**
 * @brief Removes an edge item from the scene.
 *
 * Called from GraphicsEdge::~GraphicsEdge() to remove itself from edgesHash, scene and be deleted
 *
 * @param edge
 *
 */
void GraphicsWidget::removeItem( GraphicsEdge * edge){
    edgeName = createEdgeName(edge->sourceNodeNumber(), edge->targetNodeNumber() ) ;
    qDebug() << "Removing edge"<< edgeName << "Calling edgeClicked(0)" ;
    setEdgeClicked(0);
    qDebug() << "removing edge from edges hash" ;
    edgesHash.remove(edgeName);
    qDebug() << "removing edge from scene" ;
    scene()->removeItem(edge);
    qDebug() << "Calling edge->deleteLater()" ;
    edge->deleteLater();
}


/**
 * @brief Removes an edge weight number item from the scene.
 *
 * @param edgeWeight
 */
void GraphicsWidget::removeItem( GraphicsEdgeWeight *edgeWeight){
    qDebug() << "Removing edge weight";
    scene()->removeItem(edgeWeight);
    edgeWeight->deleteLater();
}



/**
 * @brief Removes an edge label item from the scene.
 *
 * @param edgeLabel
 */
void GraphicsWidget::removeItem( GraphicsEdgeLabel *edgeLabel){
    qDebug() << "Removing edgeLabel";
    scene()->removeItem(edgeLabel);
    edgeLabel->deleteLater();
}



/**
 * @brief Removes a node label item from the scene.
 * @param nodeLabel
 */
void GraphicsWidget::removeItem( GraphicsNodeLabel *nodeLabel){
    qDebug() << "Removing label of node" << nodeLabel->node()->nodeNumber();
    scene()->removeItem(nodeLabel);
    nodeLabel->deleteLater();
}


/**
 * @brief Removes a node number item from the scene.
 * @param nodeNumber
 */
void GraphicsWidget::removeItem( GraphicsNodeNumber *nodeNumber){
    qDebug() << "removing number of node " <<  nodeNumber->node()->nodeNumber();
    scene()->removeItem(nodeNumber);
    nodeNumber->deleteLater();
}






/**
 * @brief Sets the color of an node.
 *
 * Called when the user changes the color of a node (right-clicking).
 *
 * @param nodeNumber
 * @param color
 * @return
 */
bool GraphicsWidget::setNodeColor(const int &nodeNum, const QString &color){
    qDebug() << "Setting node"<< nodeNum << "new color to:" << color;
    nodeHash.value(nodeNum)->setColor(color);
    return true;
}



/**
 * @brief Sets the shape of an node.
 *
 * @param nodeNumber
 * @param shape
 * @param iconPath
 * @return
 */
bool GraphicsWidget::setNodeShape(const int &nodeNum,
                                  const QString &shape,
                                  const QString &iconPath){
    qDebug() << "Setting node"<< nodeNum << "new shape to:" << shape;
    nodeHash.value(nodeNum)->setShape(shape,iconPath);
    return true;
\
}




/**
 * @brief GraphicsWidget::setNodeLabelsVisibility
 * @param toggle
 */
void GraphicsWidget::setNodeLabelsVisibility (const bool &toggle){
    qDebug()<< "Toggling node labels visibility to:" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setLabelVisibility(toggle);
    }
    m_nodeLabelVisibility = toggle;
}


/**
 * @brief Sets the label of an node.
 *
 * @param nodeNumber
 * @param label
 * @return
 */
bool GraphicsWidget::setNodeLabel(const int &nodeNumber, const QString &label){
    qDebug() << "Setting node"<< nodeNumber << "new label to:" << label;
    nodeHash.value(nodeNumber)->setLabelText (label);
    return true;

}



/**
 * @brief Toggles node numbers displayed inside or out of nodes
 *
 * @param numIn
 */
void   GraphicsWidget::setNumbersInsideNodes(const bool &toggle){
    qDebug()<< "Toggling node numbers inside:" << toggle;
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
    qDebug()<< "Setting default node size to" << size;
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
 * @brief Changes the visibility of a Node
 * @param nodeNum
 * @param toggle
 */
void GraphicsWidget::setNodeVisibility(const int &nodeNum, const bool &toggle){
    if  ( nodeHash.contains (nodeNum) ) {
        qDebug() << "Setting visibility of node"
                 << nodeNum << "to" << toggle;
        nodeHash.value(nodeNum)->setVisible(toggle);
        nodeHash.value(nodeNum)->setEnabled(toggle);
    }
}


/**
 * @brief Changes the size of a node
 *
 * @param nodeNum
 * @param size
 * @return bool
 */
bool GraphicsWidget::setNodeSize(const int &nodeNum, const int &size ){
    qDebug() << "Setting size of node"
             << nodeNum << "to" << size;
    if  ( nodeHash.contains (nodeNum) ) {
        if (size>0){
            nodeHash.value(nodeNum)->setSize(size);
            return true;

        }
        else {
            qDebug() << "Setting size of node"
                     << nodeNum << "to default" << m_nodeSize;
            nodeHash.value(nodeNum)->setSize(m_nodeSize);
            return true;

        }
    }
    return false;
}


/**
 * @brief Changes the size of all nodes.
 *
 * @param size
 */
void GraphicsWidget::setNodeSizeAll(const int &size ){
    qDebug() << "Changing all nodes size... ";
    foreach ( GraphicsNode *m_node, nodeHash ) {
            m_node->setSize(size);
    }
}




/**
 * @brief Toggles the visibility of node numbers
 *
 * @param toggle
 */
void GraphicsWidget::setNodeNumberVisibility(const bool &toggle){
    qDebug()<< "toggling visibility of all node numbers to" << toggle;
    foreach ( GraphicsNode *m_node, nodeHash) {
        m_node->setNumberVisibility(toggle);
    }
    m_nodeNumberVisibility = toggle;
}




/**
 * @brief Changes the color of a node number
 *
 * @param nodeNumber
 * @param color
 */
void GraphicsWidget::setNodeNumberColor(const int &nodeNum, const QString &color) {
    qDebug() << "Setting color of node"
             << nodeNum << "to" << color;
    if  ( nodeHash.contains (nodeNum) ) {
        if (!color.isNull()){
            nodeHash.value(nodeNum)->setNumberColor(color) ;
        }
    }
}


/**
 * @brief Changes the size of the number of a node
 *
 * @param nodeNum
 * @param size
 */
bool GraphicsWidget::setNodeNumberSize(const int &nodeNum, const int &size){
    qDebug() << "Setting number size of node"
             << nodeNum << "to" << size;
    if  ( nodeHash.contains (nodeNum) ) {
        if (size>0){
            nodeHash.value(nodeNum)->setNumberSize(size) ;
            return true;
        }
    }
    return false;
}



/**
 * @brief Changes the distance of the number of a node.
 * @param nodeNum
 * @param distance
 */
bool GraphicsWidget::setNodeNumberDistance(const int &nodeNum, const int &distance ){
    qDebug() << "Setting number distance of node"
             << nodeNum << "to" << distance;
    if  ( nodeHash.contains (nodeNum) ) {
        if (distance>=0){
            nodeHash.value(nodeNum)->setNumberDistance(distance) ;
            return true;
        }
    }
    return false;
}




/**
 * @brief Changes the label color of a node to 'color'.
 * @param nodeNum
 * @param color
 */
bool GraphicsWidget::setNodeLabelColor(const int &nodeNum, const QString &color){
    qDebug() << "Setting label color of node"
             << nodeNum << "to" << color;
    if  ( nodeHash.contains (nodeNum) ) {
            nodeHash.value(nodeNum)->setLabelColor(color);
            return true;
    }
    return false;
}




/**
 * @brief Changes the label size of a node to 'size'.
 * @param nodeNum
 * @param size
 */
bool GraphicsWidget::setNodeLabelSize(const int &nodeNum, const int &size){
    qDebug() << "Setting label size of node"
             << nodeNum << "to" << size;
    if  ( nodeHash.contains (nodeNum) ) {
        if (size>0){
            nodeHash.value(nodeNum)->setLabelSize(size);
            return true;
        }
    }
    return false;
}





/**
 * @brief Changes the distance of the label of the given node
 *
 * @param nodeNum
 * @param distance
 */
bool GraphicsWidget::setNodeLabelDistance( const int &nodeNum, const int &distance ){
    qDebug() << "Setting label distance of node"
             << nodeNum << "to" << distance;
    if  ( nodeHash.contains (nodeNum) ) {
        if (distance>=0){
            nodeHash.value(nodeNum)->setLabelDistance(distance) ;
            return true;
        }
    }
    return false;
}




/**
 * @brief Checks if a node with label or nodeNum 'text' exists and returns it
 *
 * @param text
 * @return
 */
GraphicsNode* GraphicsWidget::hasNode( QString text ){
    bool ok = false;
    foreach ( GraphicsNode *candidate, nodeHash) {
        if ( 	candidate->nodeNumber()==text.toInt(&ok, 10)  ||
                ( candidate->labelText() == text)
                ) {
            qDebug() << "Node" << text << " found!";
            return candidate;
            break;
        }

    }
    return 0;	//dummy return.
}



/**
 * @brief Selects all nodes in the given list
 *
 * @param list
 */
void GraphicsWidget::setSelectedNodes(QList<int> list){
    qDebug() << "Marking" << list.count() << "nodes as selected";
    foreach ( int nodeNum, list) {
        if  ( nodeHash.contains (nodeNum) ) {
            qDebug() << "Selecting node"<< nodeNum;
            nodeHash.value(nodeNum)->setSelected(true) ;
        }
    }
}



/**
 * @brief Sets the label of an edge.
 *
 * Called when the user changes the label of an edge (right-clicking).
 *
 * @param source
 * @param target
 * @param label
 */
void GraphicsWidget::setEdgeLabel(const int &source,
                                  const int &target,
                                  const QString &label){

    edgeName = createEdgeName( source, target );

    qDebug()<<"Setting label for edge" << edgeName <<  "new label"  << label;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName)->setLabel(label);
    }


}

/**
 * @brief Sets the color of an edge.
 *
 * Called when the user changes the color of an edge (right-clicking).
 * Also called from Graph when all edge colors need to be changed.
 *
 * @param source
 * @param target
 * @param color
 */
void GraphicsWidget::setEdgeColor(const int &source,
                                  const int &target,
                                  const QString &color){

    edgeName =  createEdgeName( source, target );

    qDebug()<<"Setting color of edge" << edgeName <<  "new color:"  << color;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName)->setColor(color);
    }

}




/**
 * @brief Changes the direction type of an existing edge
 *
 * @param source
 * @param target
 * @param dirType
 * @return
 */
bool GraphicsWidget::setEdgeDirectionType(const int &source,
                                          const int &target,
                                          const int &dirType){
    qDebug() << "Changing the direction type of edge: "
             << source
             << "->" << target
             << "to type:" << dirType;

    edgeName = createEdgeName( source, target );

    if  ( edgesHash.contains (edgeName) ) {
        qDebug()<<" Found edge in edgesHash. ";
        edgesHash.value(edgeName)->setDirectionType(dirType);
        return true;
    }
    return false;

}



/**
 * @brief Sets the weight of an edge.
 *
 * Called when the user changes the weight of an edge (right-clicking).
 *
 * @param source
 * @param target
 * @param weight
 * @return
 */
bool GraphicsWidget::setEdgeWeight(const int &source,
                                   const int &target,
                                   const qreal &weight){

    edgeName = createEdgeName( source, target );

    qDebug()<<"GW::setEdgeWeight() -" << edgeName <<  " new weight "  << weight;
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName)->setWeight(weight);
        return true;
    }

    else {
        //check opposite edge. If it exists, then transform it to directed
        edgeName = createEdgeName(target, source);
        qDebug() << "GW::setEdgeWeight() - Edge did not exist, checking for opposite:"
                 << edgeName;
        if ( edgesHash.contains(edgeName) ) {
            qDebug() << "GW::setEdgeWeight() - Opposite edge exists. Check if it is reciprocated";
            edgesHash.value(edgeName)->setWeight(weight);
            return true;
        }
        qDebug() << "GW::setEdgeWeight() - No such edge to delete";
    }

    return false;

}


/**
 * @brief Toggles the visibility of all edge arrows
 * @param toggle
 */
void GraphicsWidget::setEdgeArrowsVisibility(const bool &toggle){
    qDebug()<< "GW::setEdgeArrowsVisibility()" << toggle;
    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->showArrows(toggle);
    }

}



/**
 * @brief Changes the offset of an edge (or all edges) from source and target nodes.
 *
 * @param source
 * @param target
 * @param offset
 */
void GraphicsWidget::setEdgeOffsetFromNode(const int &source,
                                           const int &target,
                                           const int &offset){
    qDebug() << "GW::setEdgeOffsetFromNode() : " << source << "->" << target
             << " = " << offset;

    if (source && target) {

        QString edgeName =  QString::number(m_curRelation) + QString(":") +
                QString::number( source ) + QString(">")+ QString::number( target );

        qDebug()<<"GW::setEdgeWeight() -" << edgeName <<  " new offset "  << offset;
        if  ( edgesHash.contains (edgeName) ) {
            edgesHash.value(edgeName)->setMinimumOffsetFromNode(offset);
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

/**
 * @brief Toggles all edge weight numbers visibility
 * @param toggle
 */
void GraphicsWidget::setEdgeWeightNumbersVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeWeightNumbersVisibility()" << toggle;
    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->setWeightNumberVisibility(toggle);
    }
}



/**
 * @brief Toggles all edge labels visibility
 * @param toggle
 */
void GraphicsWidget::setEdgeLabelsVisibility (const bool &toggle){
    qDebug()<< "GW::setEdgeLabelsVisibility() " << toggle
               << "for edgesHash.count: " << edgesHash.count();
    foreach ( GraphicsEdge *m_edge, edgesHash) {
        m_edge->setLabelVisibility(toggle);
    }
}




/**
 * @brief Toggles edge highlighting
 *
 * If enabled, an edge will be highlighted on mouse hover and edges
 * connected to a node will be highlighted when the user selects that node.
 *
 * @param toggle
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
 * @brief Toggles the visibility of the given edge
 *
 * @param relation
 * @param sourceNum
 * @param targetNum
 * @param visible
 */
void GraphicsWidget::setEdgeVisibility(const int &relation, const int &sourceNum, const int &targetNum, const bool &visible, const bool &checkInverse){
    edgeName = createEdgeName( sourceNum, targetNum, relation );
    if  ( edgesHash.contains (edgeName) ) {
        edgesHash.value(edgeName)->setVisible(visible);
        edgesHash.value(edgeName)->setEnabled(visible);
        qDebug()<<"Toggled visibility of edge"<<edgeName<<"to"<<visible;
        return;
    }
    else {
        qDebug()<<"Cannot find edge"<<edgeName<<"in edgesHash to toggle visibility to" << visible;
    }
    if (checkInverse) {
        // TODO: #140 - we need to check if the reverse exists and if not it should be created.
        // Check the reverse edge
        edgeName = createEdgeName( targetNum, sourceNum, relation );
        if  ( edgesHash.contains (edgeName) ) {
            qDebug()<<"Toggled visibility of reverse edge"<<edgeName<<"to"<<!visible;
            edgesHash.value(edgeName)->setVisible(!visible);
            edgesHash.value(edgeName)->setEnabled(!visible);
            return;
        }
        else {
            qDebug()<<"Cannot find reverse edge"<<edgeName<<"in edgesHash to toggle visibility to" << !visible;
        }
    }
}



/**
 * @brief Toggles the visibility of all items of the given type
 *
 * @param type
 * @param visible
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



/**
 * @brief Adds a circular guideline
 * @param x0
 * @param y0
 * @param radius
 */
void GraphicsWidget::addGuideCircle( const double&x0,
                                     const double&y0,
                                     const double&radius){
    GraphicsGuide *circ=new GraphicsGuide (this, x0, y0, radius);
    circ->show();

}


/**
 * @brief Adds a horizonal guideline
 * @param y0
 */
void GraphicsWidget::addGuideHLine(const double &y0){
    GraphicsGuide *line=new GraphicsGuide (this, y0, this->width());
    line->show();
}



/**
 * @brief Removes all items of the given type
 * @param type
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
 * @brief Forces the scene to select all items. Also signals that no node is clicked.
 */
void GraphicsWidget::selectAll(){
    QPainterPath path;
    path.addRect(0,0, this->scene()->width() , this->scene()->height());
    scene()->setSelectionArea(path);
    emit userClickedNode(0, QPointF(0,0));
    qDebug() << "GraphicsWidget::selectAll() - selected items now: "
             << selectedItems().count();
}



/**
 * @brief Clears any item selection from the scene. Also signals that no node is clicked.
 */
void GraphicsWidget::selectNone(){
    qDebug() << "Emptying user selection...";
    emit userClickedNode(0,QPointF(0,0));
    scene()->clearSelection();

}


/**
 * @brief Handles the event of selection change in the scene.
 *
 * Emits selected nodes and edges to Graph
 *
 */
void GraphicsWidget::handleSelectionChanged() {

    qDebug() <<"Scene selection has been changed. Getting selected nodes/edges and passing them to Graph...";
    emit userSelectedItems(selectedNodes(), selectedEdges());

}



/**
 * @brief Returns a list of all selected QGraphicsItem(s)
 *
 * @return QList<QGraphicsItem*>
 */
QList<QGraphicsItem *> GraphicsWidget::selectedItems(){
    qDebug() <<"GW::selectedItems()";
    return scene()->selectedItems();
}


/**
 * @brief Returns a List of selected node numbers
 *
 * @return QList<int>
 */
QList<int> GraphicsWidget::selectedNodes() {

    m_selectedNodes.clear();
    foreach (QGraphicsItem *item, scene()->selectedItems()) {
        if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item) ) {
            m_selectedNodes.append(node->nodeNumber());
        }
    }
    qDebug() <<"Selected nodes count:" << m_selectedNodes.count();
    return m_selectedNodes;
}


/**
 * @brief Returns a QList of selected directed edges structs in the form of v1,v2
 *
 * @return QList<SelectedEdge>
 */
QList<SelectedEdge> GraphicsWidget::selectedEdges() {

    m_selectedEdges.clear();
    foreach (QGraphicsItem *item, scene()->selectedItems()) {
        if (GraphicsEdge *edge= qgraphicsitem_cast<GraphicsEdge *>(item) ) {
            SelectedEdge selEdge = qMakePair( edge->sourceNodeNumber(), edge->targetNodeNumber());
            m_selectedEdges << selEdge;
        }
    }
    qDebug() <<"Selected edges count:" << m_selectedEdges.count();
    return m_selectedEdges;
}




/**
 * @brief Handles user double-clicks.
 *
 * If the double-click was on empty space, it initiates the new node creation process.
 * Otherwise, it the user double-clicks on a node, starts the edge creation process.
 *
 * @param QMouseEvent
 */
void GraphicsWidget::mouseDoubleClickEvent ( QMouseEvent * e ) {

    if (this->dragMode() == QGraphicsView::RubberBandDrag ) {

        if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
            if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {
                qDebug() << "Double-click on a node. Starting new edge...";
                startEdge(node);
                QGraphicsView::mouseDoubleClickEvent(e);
                return;
            }
            else if ( (*item).type() == TypeLabel){
                QGraphicsView::mouseDoubleClickEvent(e);
                return;
            }
            qDebug() << "Double-click on something (not node)";
        }

        QPointF p = mapToScene(e->pos());
        qDebug() << "Double-click on empty space. Signalling to create a new vertex/node at:"
            << e->pos() << "~"<< p;
        emit userDoubleClickNewNode(p);
    }

    QGraphicsView::mouseDoubleClickEvent(e);

}



/**
 * @brief Handles the mouse press event
 * @param e
 */
void GraphicsWidget::mousePressEvent( QMouseEvent * e ) {

    if (this->dragMode() == QGraphicsView::RubberBandDrag ) {

        QPointF p = mapToScene(e->pos());

        if ( QGraphicsItem *item = itemAt(e->pos() )   ) {

            //
            // User clicked on some item
            //

            if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {

                //
                // User clicked on a node
                //

                qDebug() << "Clicked on a node at:"  << e->pos() << "~"<< p << "Setting it as clicked node...";

                setNodeClicked(node);

                if ( e->button()==Qt::RightButton ) {
                    qDebug() << "This was a right-click on node. Signaling to open node menu";
                    emit openNodeMenu();
                }
                if ( e->button()==Qt::MiddleButton) {
                    qDebug() << "This was a middle-click on node. Calling to start or conclude a new edge...";
                    startEdge(node);
                }
                QGraphicsView::mousePressEvent(e);
                return;
            }

            if (GraphicsEdge *edge = qgraphicsitem_cast<GraphicsEdge *>(item)) {
                //
                // User clicked on an edge
                //
                qDebug() << "Clicked on an edge at:" << e->pos() << "~"<< p;

                if ( e->button()==Qt::LeftButton ) {
                    qDebug() << "This was a left click on the edge. Setting clicked edge...";
                    setEdgeClicked(edge);
                }
                else if ( e->button()==Qt::RightButton ) {
                    qDebug() << "This was a right click on the edge. Signaling to open context menu...";
                    setEdgeClicked(edge, true);
                }
                else {
                    setEdgeClicked(edge);
                }
                QGraphicsView::mousePressEvent(e);
                return;
            }
        }
        else {

            //
            // User clicked on empty space
            //

            if ( e->button() == Qt::RightButton   ) {
                //
                // User clicked on empty space, with right button
                // so we must open the context menu
                //
                qDebug() << "Right click on empty space at:" << e->pos() << "~"<< p << "Signalling to open context menu";
                emit openContextMenu(p);
            }
            else {
                //
                //  user clicked on empty space, with left button.
                //
                qDebug() << "Left click on empty space at:"
                     << e->pos() << "~"<< p << "Setting clickedEdge=0 and emitting signal...";
                clickedEdge=0;
                emit userClickOnEmptySpace(p);
            }
        }
    }

    QGraphicsView::mousePressEvent(e);


}



/**
 * @brief Handles the mouse release events.
 *
 * Called when the user releases a mouse button, after a click.
 * First sees what was in the position where the user clicked
 * If a node was underneath, it signals for every node
 * in scene selectedItems
 *
 * @param e
 */
void GraphicsWidget::mouseReleaseEvent( QMouseEvent * e ) {

    if (this->dragMode() == QGraphicsView::RubberBandDrag ) {

        QPointF p = mapToScene(e->pos());

        if ( QGraphicsItem *item= itemAt(e->pos() ) ) {
            if (GraphicsNode *node = qgraphicsitem_cast<GraphicsNode *>(item)) {
                qDebug() << "Mouse released at:" << e->pos() << "~"<< p
                     << "on a node. Signalling to move all selected nodes";
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
                qDebug() << "Mouse released at:" << e->pos() << "~"<< p << "on an edge.";
                QGraphicsView::mouseReleaseEvent(e);
                return;
            }
        }
        else{
            qDebug() << "Mouse released at:" << e->pos() << "~"<< p << "on empty space.";
        }

    }

    QGraphicsView::mouseReleaseEvent(e);
}





/**
 * @brief Handles the mouse wheel event. If CTRL is pressed, zooms in or out.
 *
 * @param e
 */
void GraphicsWidget::wheelEvent(QWheelEvent *e) {
    bool ctrlKey = (e->modifiers() == Qt::ControlModifier);
    QPoint numDegrees =  e->angleDelta() / 8;
    qDebug() << "Mouse wheel changeded by numDegrees = " << numDegrees;
    if (ctrlKey) {
        if ( numDegrees.x() > 0 || numDegrees.y() > 0)
            zoomIn(1);
        else if ( numDegrees.x() < 0 || numDegrees.y() < 0)
            zoomOut(1);
    }

}


/**
 * @brief Decreases the numerical zoom index of the scene by the given step
 *
 * Signals to MW to update the UI and do the rest.
 *
 * @param level
 */
void GraphicsWidget::zoomOut (const int step){

    qDebug() << "Zooming out from "<< m_zoomIndex << "-" << step ;
    m_zoomIndex-=step;
    if (m_zoomIndex <= 0) {
        m_zoomIndex = 0;
    }
    emit zoomChanged(m_zoomIndex);

}



/**
 * @brief Increases the numerical zoom index of the scene by the given step
 *
 * Signals to MW to update the UI and do the rest.
 *
 * @param level
 */
void GraphicsWidget::zoomIn(const int step){
    qDebug() << "Zooming in from "<< m_zoomIndex << "+" << step ;
    m_zoomIndex+=step;
    if (m_zoomIndex > 500) {
        m_zoomIndex=500;
    }
    if (m_zoomIndex < 0) {
        m_zoomIndex = 0;
    }
    emit zoomChanged(m_zoomIndex);
}



/**
 * @brief Does the actual zoom in or out while preserving current rotation
 *
 * Scales the view transformation by the given value (0..500)
 *
 * @param value
 */
void GraphicsWidget::changeMatrixScale(int value) {

    qDebug () << "Scaling the view transformation matrix by value" << value
               << " - GW dimensions: " << width() << "x" << height()
              << "  Scene dimensions:" << scene()->width() << "x" << scene()->height();

    // Raise a flag that a non-trivial transformation is applied on the view
    m_isTransformationActive = true;

    // Since the max value will be 500, the scaleFactor will be max 2 ^ 5 = 32
    m_currentScaleFactor = pow(qreal(2), (value - 250) / qreal(50) );

    qDebug() << "Scaling view transformation by new scale factor:" << m_currentScaleFactor
              << "rotation unchanged:" << m_currentRotationAngle;

    resetTransform();
    scale(m_currentScaleFactor, m_currentScaleFactor);
    rotate(m_currentRotationAngle);

    qDebug () << "Finished scaling the view."
               << " - GW dimensions: " << width() << "x" << height()
              << "  Scene dimensions:" << scene()->width() << "x" << scene()->height();


}


/**
 * @brief Decreases the numerical rotation Rotates the view to the left, by a fixed angle
 */
void GraphicsWidget::rotateLeft(){
    m_currentRotationAngle-=5;
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief Rotates the view to the right, by a fixed angle
 */
void GraphicsWidget::rotateRight() {
    m_currentRotationAngle+=5;
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief Rotates the view transformation by angle degrees clockwise, while preserving the current scale
 *
 * @param angle
 */
void GraphicsWidget::changeMatrixRotation(int angle){
    m_isTransformationActive = true;
    m_currentRotationAngle = angle;
    qDebug() << "Rotating clockwise by angle" <<  angle
              << " m_currentRotationAngle " << m_currentRotationAngle
              << " m_currentScaleFactor " << m_currentScaleFactor;
    resetTransform();
    scale(m_currentScaleFactor, m_currentScaleFactor);
    rotate(angle);

}



/**
 * @brief Resets to default rotation, zoom and scale
 */
void GraphicsWidget::reset() {
    m_currentRotationAngle=0;
    m_currentScaleFactor = 1;
    m_zoomIndex=250;
    this->ensureVisible(QRectF(0, 0, 0, 0));
    emit zoomChanged(m_zoomIndex);
    emit rotationChanged(m_currentRotationAngle);
}


/**
 * @brief Handles the canvas resize event
 *
 * Repositions guides then emits resized() signal to MW and eventually Graph
 * which does the node repositioning maintaining proportions
 *
 * @param e
 */
void GraphicsWidget::resizeEvent( QResizeEvent *e ) {

    m_width=e->size().width();
    m_height=e->size().height();
    m_w0=e->oldSize().width();
    m_h0=e->oldSize().height();

    qDebug () << "GW resized:" << m_w0 << "x" << m_h0
            << "-->" << m_width << "x" << m_height;

    if (m_isTransformationActive)  {
        m_isTransformationActive = false;
        return;
    }

    // Compute resize factors
    fX = (double)(m_width)/(double)(m_w0);
    fY = (double)(m_height)/(double)(m_h0);
    // reposition  guides
    foreach (QGraphicsItem *item, scene()->items()) {
        if ( (item)->type() == TypeGuide ){
            if (GraphicsGuide *guide = qgraphicsitem_cast<GraphicsGuide *>  (item) ) {
                if (guide->isCircle()) {
                    guide->die();
                    guide->deleteLater ();
                    delete item;
                }
                else {
                    qDebug()<< "Horizontal GraphicsGuide "
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

    // Force updating the scene width and height to match the new graphicsWidget dimensions
//    scene()->setSceneRect(0, 0, (qreal) m_width, (qreal) m_height );

    qDebug () << "Scene dimensions now:" << scene()->width() << "x" << scene()->height();

    emit resized(m_width, m_height);

    QGraphicsView::resizeEvent(e);
}




/**
 * @brief Destructor. Calls the method to clear the data.
 */
GraphicsWidget::~GraphicsWidget(){
    qDebug() << "Terminating. Calling clear()";
    clear();
}

