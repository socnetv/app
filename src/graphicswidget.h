/**
 * @file graphicswidget.h
 * @brief Declares the GraphicsWidget class for handling the main visualization widget in the network graph interface.
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


#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H


#include <QGraphicsView>
#include <QHash>

class MainWindow;
class GraphicsNode;
class GraphicsEdge;
class GraphicsNodeNumber;
class GraphicsNodeLabel;
class GraphicsGuide;
class GraphicsEdgeWeight;
class GraphicsEdgeLabel;

typedef QHash<QString, GraphicsEdge*> H_StrToEdge;
typedef QHash <int, GraphicsNode*> H_NumToNode;


typedef QPair<int, int> SelectedEdge;

Q_DECLARE_METATYPE(SelectedEdge)


class GraphicsWidget : public QGraphicsView {
    Q_OBJECT

public:

     GraphicsWidget(QGraphicsScene *m_scene, MainWindow *m_parent);
    ~GraphicsWidget();

    void clear();

    QString createEdgeName(const int &v1,
                           const int &v2,
                           const int &relation=-1);

    void setInitNodeSize(int);

    void setInitZoomIndex (const int &);
    void setMaxZoomIndex (const int &);

    GraphicsNode* hasNode(QString text);
    void setSelectedNodes(QList<int> list);

    QList<QGraphicsItem *> selectedItems();
    QList<int> selectedNodes();
    QList<SelectedEdge> selectedEdges();

    void selectAll();
    void selectNone();

    void removeItem(GraphicsEdge*);
    void removeItem(GraphicsEdgeWeight *edgeWeight);
    void removeItem(GraphicsEdgeLabel *edgeLabel);
    void removeItem(GraphicsNode*);
    void removeItem(GraphicsNodeNumber*);
    void removeItem(GraphicsNodeLabel*);

    void setNumbersInsideNodes(const bool &toggle);

    void setAllItemsVisibility(int, bool);

    void removeAllItems(int);

protected:

    void wheelEvent(QWheelEvent *event);
    void mouseDoubleClickEvent ( QMouseEvent * e );
    void mousePressEvent ( QMouseEvent * e );
    void mouseReleaseEvent(QMouseEvent * e );
    void resizeEvent( QResizeEvent *e );

public slots:

    void handleSelectionChanged();

    void relationSet(int relation);

    void drawNode(const QPointF &p,
                  const int &num,
                  const int &nodeSize,
                  const QString &nodeShape,
                  const QString &nodeIconPath,
                  const QString &nodeColor,
                  const QString &numberColor,
                  const int &numberSize,
                  const int &numberDistance,
                  const QString &nodeLabel,
                  const QString &labelColor,
                  const int &labelSize,
                  const int &labelDistance);
    void removeNode(const int &nodeNum);
    void setNodeVisibility(const int &nodeNum, const bool &toggle );	//Called from Graph via MW
    void setNodeClicked(GraphicsNode *);
    void moveNode(const int &num, const qreal &x, const qreal &y);

    bool setNodeSize(const int &nodeNumber, const int &size=0);
    void setNodeSizeAll(const int &size=0);

    bool setNodeShape(const int &nodeNum,
                      const QString &shape,
                      const QString &iconPath=QString());
    bool setNodeColor(const int &nodeNum, const QString &color);

    void setNodeNumberColor(const int &nodeNum, const QString &color);
    void setNodeNumberVisibility(const bool &toggle);
    bool setNodeNumberSize(const int &nodeNum, const int &size=0);
    bool setNodeNumberDistance(const int &, const int &distance=0);

    void setNodeLabelsVisibility(const bool &toggle);
    bool setNodeLabelColor(const int &nodeNum, const QString &color="green");
    bool setNodeLabelSize(const int &, const int &size=0);
    bool setNodeLabel(const int & , const QString &label);
    bool setNodeLabelDistance(const int &, const int &distance=0);

    void drawEdge(const int &source,
                  const int &target,
                  const qreal &weight,
                  const QString &label="",
                  const QString &color="black",
                  const int &type=0,
                  const bool &drawArrows=true,
                  const bool &bezier=false,
                  const bool &weightNumbers=false);

    void removeEdge(const int &source,
                    const int &target,
                    const bool &removeOpposite=false);

    void setEdgeVisibility (const int &relation,
                            const int &sourceNum,
                            const int &targetNum,
                            const bool &visible,
                            const bool &preserveReverseEdge=false,
                            const int &edgeWeight=1,
                            const int &reverseEdgeWeight=1);

    bool setEdgeDirectionType(const int &source,
                              const int &target,
                              const int &dirType=false);

    bool setEdgeWeight(const int &, const int &, const qreal &);

    void setEdgeLabel(const int &source, const int &target, const QString &label);

    void setEdgeColor(const int &, const int&, const QString &);

    void setEdgeClicked(GraphicsEdge *, const bool &openMenu=false);

    void setEdgeOffsetFromNode(const int &source,
                               const int &target,
                               const int &offset);
    void setEdgeArrowsVisibility(const bool &toggle);
    void setEdgeWeightNumbersVisibility (const bool &toggle);
    void setEdgeLabelsVisibility(const bool &toggle);

    void setEdgeHighlighting(const bool &toggle);

    void handleDoubleClickOnNode(GraphicsNode *node);

    void clearGuides();
    void addGuideCircle( const double&x0, const double&y0, const double&radius);
    void addGuideHLine(const double &y0);

    void zoomIn(const int step = 1);
    void zoomOut(const int step = 1);
    void rotateLeft();
    void rotateRight();
    void changeMatrixScale(const int value);
    void changeMatrixRotation(int angle);
    void reset();

    void setOptionsOpenGL(const bool &enabled=false);
    void setOptionsAntialiasing(const bool &toggle);
    void setOptionsNoAntialiasingAutoAdjust(const bool &toggle);

signals:
    void userDoubleClickNewNode(const QPointF &);
    void userMiddleClicked(const int &sourceNum, const int &targetNum, const qreal &weight=1);
    void userClickOnEmptySpace(const QPointF &p);
    void openNodeMenu();
    void openContextMenu(const QPointF p);
    void userNodeMoved(const int &, const int &, const int &);
    //void userSelectedItems(const int nodes, const int edges);
    void userSelectedItems(const QList<int> selectedNodes,
                           const QList<SelectedEdge> selectedEdges);
    void userClickedNode(const int &nodeNumber, const QPointF &p);
    void userClickedEdge(const int &source, const int &target, const bool &openMenu=false);
    void zoomChanged(const int);
    void rotationChanged(const int);
    void resized(const int, const int);
    void setCursor(Qt::CursorShape);


private:

    H_NumToNode nodeHash;	//Our basic hash table for node items
    H_StrToEdge edgesHash; // Our basic hash table for edge items
    QList<int> m_selectedNodes;
    QList<SelectedEdge> m_selectedEdges;
    int m_curRelation, m_nodeSize;
    int m_currentRotationAngle;
    int m_zoomIndex, m_zoomIndexInit, m_zoomIndexMax;
    int markedEdgeSourceOrigSize, markedEdgeTargetOrigSize;
    int m_edgeMinOffsetFromNode;
    double m_currentScaleFactor;
    qreal fX,fY, factor;
    QString m_nodeLabel, m_numberColor, m_labelColor;
    QString edgeName;
    bool m_isTransformationActive;
    bool hasDoubleClickedNode, clickedEdgeExists;
    bool m_nodeNumbersInside, m_nodeNumberVisibility, m_nodeLabelVisibility;
    bool m_edgeHighlighting;
    GraphicsNode *firstDoubleClickedNode, *secondDoubleClickedNode;
    GraphicsNode *markedEdgeSource;
    GraphicsNode *markedEdgeTarget;
    GraphicsEdge *clickedEdge;
};

#endif
