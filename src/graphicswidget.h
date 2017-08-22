/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt
 
                         graphicswidget.h  -  description
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

#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H



#include <QGraphicsView>

class MainWindow;

class Node;
class Edge;
class NodeNumber;
class NodeLabel;
class Guide;
class EdgeWeight;
class EdgeLabel;

typedef QHash<QString, Edge*> H_StrToEdge;
typedef QHash <long int, Node*> H_NumToNode;

using namespace std;


typedef pair<int, int> SelectedEdge;

class GraphicsWidget : public QGraphicsView {
    Q_OBJECT
public:

    GraphicsWidget(QGraphicsScene *m_scene, MainWindow *m_parent);

    ~GraphicsWidget();
    void clear();

    void setInitNodeSize(int);
    void setInitZoomIndex (int);


    Node* hasNode(QString text);
    bool setMarkedNode(QString text);

    QList<QGraphicsItem *> selectedItems();
    QList<int> selectedNodes();
    QList<SelectedEdge> selectedEdges();

    void selectAll();
    void selectNone();

    void removeItem(Edge*);
    void removeItem(EdgeWeight *edgeWeight);
    void removeItem(EdgeLabel *edgeLabel);
    void removeItem(Node*);
    void removeItem(NodeNumber*);
    void removeItem(NodeLabel*);

    void setNumbersInsideNodes(bool);

    void setAllItemsVisibility(int, bool);

    void removeAllItems(int);
protected:
    void wheelEvent(QWheelEvent *event);
    void mouseDoubleClickEvent ( QMouseEvent * e );
    void mousePressEvent ( QMouseEvent * e );
    void mouseReleaseEvent(QMouseEvent * e );
    void resizeEvent( QResizeEvent *e );
    void paintEvent ( QPaintEvent * event );

public slots:

    void getSelectedItems();

    void relationSet(int relation);

    void drawNode(const int &num, const int &nodeSize,
                   const QString &nodeShape, const QString &nodeColor,
                   const bool &showNumbers, const bool &numberInsideNode,
                   const QString &numberColor, const int &numberSize,
                  const int &numberDistance,
                   const bool &showLabels, const QString &nodeLabel,
                   const QString &labelColor, const int &labelSize,
                  const int &labelDistance,
                   const QPointF &p
                    );
    void eraseNode(const long int &number);
    void setNodeVisibility(long int, bool );	//Called from Graph via MW
    void nodeClicked(Node *);
    void moveNode(const int &num, const qreal &x, const qreal &y);	//Called from Graph when creating random nets.

    bool setNodeSize(const long int &nodeNumber, const int &size=0);
    void setAllNodeSize(const int &size=0);

    bool setNodeShape(const long int &nodeNumber, const QString &shape);
    bool setNodeColor(const long int &, const QString &color);

    void setNodeNumberVisibility(const bool &toggle);
    bool setNodeNumberSize(const long int &, const int &size=0);
    bool setNodeNumberDistance(const long int &, const int &distance=0);

    void setNodeLabelsVisibility(const bool &toggle);
    bool setNodeLabelColor(const long int &number, const QString &color="green");
    bool setNodeLabelSize(const long int &, const int &size=0);
    bool setNodeLabel(long int , QString );
    bool setNodeLabelDistance(const long int &, const int &distance=0);

    void drawEdge(const int &source, const int &target,
                  const float &weight,
                  const QString &label="",
                  const QString &color="black",
                  const int &type=0,
                  const bool &drawArrows=true,
                  const bool &bezier=false,
                  const bool &weightNumbers=false);
    void eraseEdge(const long int &source, const long int &target);
    void setEdgeVisibility (int relation, int, int, bool);
    bool setEdgeUndirected(const long int &, const long int &, const float &);
    bool setEdgeWeight(const long int &, const long int &, const float &);
    void setEdgeLabel(const long int &, const long int&, const QString &);
    void setEdgeColor(const long int &, const long int&, const QString &);
    void edgeClicked(Edge *);
    void setEdgeWeightNumbersVisibility (const bool &toggle);
    void setEdgeLabelsVisibility(const bool &toggle);

    void startEdge(Node *node);

    void clearGuides();
    void addGuideCircle( const double&x0, const double&y0, const double&radius);
    void addGuideHLine(const double &y0);

    void zoomIn(int level = 1);
    void zoomOut(int level = 1);
    void rotateLeft();
    void rotateRight();
    void changeMatrixScale(const int value);
    void changeMatrixRotation(int angle);
    void reset();

signals:
    void userDoubleClickNewNode(const QPointF &);
    void userMiddleClicked(const int &, const int &);
    void userClickOnEmptySpace(const QPointF &p);
    void openNodeMenu();
    void openEdgeMenu();
    void openContextMenu(const QPointF p);
    void userNodeMoved(const int &, const int &, const int &);
    //void userSelectedItems(const int nodes, const int edges);
    void userSelectedItems(const QList<int> &selectedNodes,
                           const QList<SelectedEdge> &selectedEdges);
    void userClickedNode(const int &nodeNumber);
    void userClickedEdge(const int &source, const int &target);
    void zoomChanged(const int);
    void rotationChanged(const int);
    void resized(const int, const int);
    void setCursor(Qt::CursorShape);



private:

    H_NumToNode nodeHash;	//This is used in drawEdge() method
    H_StrToEdge edgesHash; // helper hash to easily find edges
    QList<int> m_selectedNodes;
    QList<SelectedEdge> m_selectedEdges;
    int m_curRelation, m_nodeSize;
    int m_currentRotationAngle;
    int m_zoomIndex, markedNodeOrigSize,markedEdgeSourceOrigSize, markedEdgeTargetOrigSize;
    double m_currentScaleFactor;
    qreal fX,fY, factor;
    QString m_nodeLabel, m_numberColor, m_labelColor;
    QString edgeName;
    bool transformationActive;
    bool secondDoubleClick, markedNodeExist, clickedEdgeExists;
    Node *firstNode, *secondNode, *markedNode1, *markedEdgeSource;
    Node *markedEdgeTarget, *tempNode ;
};

#endif
