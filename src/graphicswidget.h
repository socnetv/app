/**
 * @file graphicswidget.h
 * @brief Declares the GraphicsWidget class for handling the main visualization widget in the network graph interface.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
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
#include <QTimer>
#include <QLoggingCategory>
#include "global.h"

SOCNETV_USE_NAMESPACE

// WS14: shared across graphicswidget.cpp and the small canvas item files (graphicsnode.cpp,
// graphicsedgelabel.cpp, graphicsedgeweight.cpp, graphicsnodelabel.cpp, graphicsnodenumber.cpp) --
// promoted from graphicswidget.cpp-local since those files are all part of the same WS10 canvas
// subsystem and each has too few qDebug() calls to justify a category of its own.
Q_DECLARE_LOGGING_CATEGORY(lcGW)

class MainWindow;
class GraphicsNode;
class GraphicsEdge;
class GraphicsNodeNumber;
class GraphicsNodeLabel;
class GraphicsGuide;
class GraphicsEdgeWeight;
class GraphicsEdgeLabel;

typedef QHash<quint64, GraphicsEdge*> H_KeyToEdge;
typedef QHash <int, GraphicsNode*> H_NumToNode;



class GraphicsWidget : public QGraphicsView {
    Q_OBJECT

public:

     GraphicsWidget(QGraphicsScene *m_scene, MainWindow *m_parent);
    ~GraphicsWidget();

    void clear();

    /**
     * @brief True while a bulk clear() is in progress; false otherwise.
     *
     * GraphicsNode/GraphicsEdge destructors check this to skip per-item unlinking work
     * (edge-list removal from neighbours, individual scene/hash removal) that's pointless
     * during a bulk teardown - scene()->clear() destroys every item anyway, so proactively
     * unlinking each one from its neighbours' edge lists is pure wasted O(degree) work per
     * edge, repeated across the whole network. See #260.
     */
    bool isClearing() const { return m_isClearing; }

    quint64 edgeKey(const int &v1,
                     const int &v2,
                     const int &relation=-1);

    void setInitNodeSize(int);

    void setInitZoomIndex (const int &);
    void setMaxZoomIndex (const int &);

    void setSelectedNodes(const QList<int> &list);

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

    // Bulk counterpart of setEdgeVisibility (WS3 M2): applies a whole batch in one queued
    // dispatch instead of one per edge - see Graph::signalSetEdgesVisibilityBatch.
    void setEdgesVisibilityBatch(const QList<EdgeVisibilityChange> &changes);

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
    void setEdgeArrowSize(const int &size);
    void setEdgeWeightNumbersVisibility (const bool &toggle);
    void setEdgeLabelsVisibility(const bool &toggle);

    void setEdgesBezier(const bool &toggle);

    void setEdgeHighlighting(const bool &toggle);

    void selectPath(const QList<int> &path);

    void handleDoubleClickOnNode(GraphicsNode *node);

    void clearGuides();
    void addGuideCircle( const double&x0, const double&y0, const double&radius);
    void addGuideHLine(const double &y0);

    void zoomIn(const int step = 1);
    void zoomOut(const int step = 1);
    void zoomToFit();
    void rotateLeft();
    void rotateRight();
    void changeMatrixScale(const int value);
    void changeMatrixRotation(int angle);
    void reset();

    void setOptionsOpenGL(const bool &enabled=false);
    void setOptionsAntialiasing(const bool &toggle);
    void setOptionsNoAntialiasingAutoAdjust(const bool &toggle);

protected:
    void scrollContentsBy(int dx, int dy) override;

signals:
    /** @brief Double-click on empty canvas space; MW creates a new node at this scene position. */
    void userDoubleClickNewNode(const QPointF &);

    /** @brief Two consecutive middle-clicks (or double-clicks) on two different nodes;
     *  MW/Graph creates a new edge from the first node to the second. */
    void userMiddleClicked(const int &sourceNum, const int &targetNum, const qreal &weight=1);

    /** @brief Left-click on empty canvas space, at scene position p. */
    void userClickOnEmptySpace(const QPointF &p);

    /** @brief Right-click on a node; MW opens the node context menu for the currently clicked node. */
    void openNodeMenu();

    /** @brief Right-click on empty canvas space, at scene position p; MW opens the general context menu. */
    void openContextMenu(const QPointF p);

    /** @brief One node from the current selection finished being dragged to a new position
     *  (nodeNumber, x, y); emitted once per selected node on mouse release. */
    void userNodeMoved(const int &, const int &, const int &);

    /** @brief The full current node/edge selection, emitted whenever it changes
     *  (single emission per change, not per item). */
    void userSelectedItems(const QList<int> selectedNodes,
                           const QList<SelectedEdge> selectedEdges);

    /** @brief A node was clicked (nodeNumber, scene position); also used with nodeNumber=0
     *  as a generic "selection changed" notification from selectAll()/selectNone(). */
    void userClickedNode(const int &nodeNumber, const QPointF &p);

    /** @brief An edge was clicked (source, target); openMenu requests MW open its context menu. */
    void userClickedEdge(const int &source, const int &target, const bool &openMenu=false);

    /** @brief Current zoom index changed; keeps the MW zoom slider in sync. */
    void zoomChanged(const int);

    /** @brief Current rotation angle changed; keeps the MW rotation slider in sync. */
    void rotationChanged(const int);

    /** @brief Canvas viewport was resized (debounced to fire once, 150ms after the last resize event). */
    void resized(const int, const int);

    /** @brief Requests MW change the application cursor shape (e.g. while creating an edge). */
    void setCursor(Qt::CursorShape);


private:

    H_NumToNode nodeHash;	//Our basic hash table for node items
    H_KeyToEdge edgesHash; // Our basic hash table for edge items
    QList<int> m_selectedNodes;
    QList<SelectedEdge> m_selectedEdges;
    int m_curRelation, m_nodeSize;
    int m_currentRotationAngle;
    int m_zoomIndex, m_zoomIndexInit, m_zoomIndexMax;
    int markedEdgeSourceOrigSize, markedEdgeTargetOrigSize;
    int m_edgeMinOffsetFromNode;
    int m_arrowSize;
    double m_currentScaleFactor;
    qreal fX,fY, factor;
    QString m_nodeLabel, m_numberColor, m_labelColor;
    quint64 m_edgeKey;
    QPointF m_viewCenter;
    bool m_viewCenterValid;
    bool m_isZooming;
    bool m_isTransformationActive;
    bool m_isClearing;
    bool hasDoubleClickedNode, clickedEdgeExists;
    bool m_nodeNumbersInside, m_nodeNumberVisibility, m_nodeLabelVisibility;
    bool m_edgeHighlighting;
    bool m_edgesBezier;
    QTimer m_resizeTimer;
    QList<GraphicsGuide *> m_guides;
    GraphicsNode *firstDoubleClickedNode, *secondDoubleClickedNode;
    GraphicsNode *markedEdgeSource;
    GraphicsNode *markedEdgeTarget;
    GraphicsEdge *clickedEdge;
};

#endif
