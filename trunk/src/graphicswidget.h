/***************************************************************************
 SocNetV: Social Networks Visualiser
 version: 0.48
 Written in Qt 4.4 with KDevelop 
 
                         graphicswidget.h  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
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

#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H


#include <QtGui/QGraphicsView>
#include "graph.h"
#include <QGraphicsScene>
#include <QMap>

class MainWindow;
//class QGraphicsSceneMouseEvent;
class Node;
class Edge;
class NodeNumber;
class NodeLabel;
class BackgrCircle;

typedef QMap<QString, Edge*> StringToEdgeMap;


class GraphicsWidget : public QGraphicsView {
	Q_OBJECT
public:
 	GraphicsWidget(QGraphicsScene*, MainWindow* parent);
	~GraphicsWidget();
	void clear();
	void removeNode(int doomedJim);
	void removeEdge(int, int);
	void removeItem(Edge*);
	void removeItem(Node*);
	void removeItem(NodeNumber*);
	void removeItem(NodeLabel*);
	void nodeMoved(int, int, int);

	void setInitNodeColor(QString);
	void setInitLinkColor(QString);
	void setInitNodeSize(int);
	void setInitNumberDistance(int);
	void setInitLabelDistance(int);
	bool setNodeColor(int, QString);
	bool setEdgeColor(int, int, QString);
	void setAllItemsVisibility(int, bool);

protected:
	void timerEvent(QTimerEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mouseDoubleClickEvent ( QMouseEvent * e );
	void resizeEvent( QResizeEvent *e );

public slots:
	void drawNode(int i, int size, QString aColor, QString label, QString lColor, QPointF p, QString nodeShape, bool showLabels, bool showNumbers);
	void drawEdge(int, int, bool, bool, QString, bool, bool);
	void nodeClicked(Node *);
	void edgeClicked(Edge *);
	void openNodeContextMenu();
	void openEdgeContextMenu();
	void nodeMovement(int state, int type); //used by dynamic layouts: Spring Embedder, Fruchterman, etc...
	void moveNode(int, int, int);			//Called from Graph when creating random nets.
	void changeZoom(int value); 
	void startEdge(Node *node);	
	void drawEdgeReciprocal(int, int);
	void unmakeEdgeReciprocal(int, int);

	void clearBackgrCircles();
	void addBackgrCircle( int x0, int y0, int radius);
	void addBackgrHLine(int y0);
	void zoomIn();
	void zoomOut();
	void rot(int angle);

signals:
	void windowResized(int,int);
	void userDoubleClicked(int, QPointF);
	void userMiddleClicked(int, int, int);
	void openNodeMenu();
	void openEdgeMenu();
	void updateNodeCoords(int, int, int);
	void selectedNode(Node *);
	void selectedEdge(Edge *);
 	void changed();
	void zoomChanged(int);
	
private:
	int timerId,  layoutType, m_nodeSize, m_numberDistance, m_labelDistance;
	QTransform init_Transform;
	int zoomIndex;
	QString m_nodeLabel, m_numberColor, m_nodeColor, m_labelColor, m_linkColor;
	bool secondDoubleClick, dynamicMovement;
	QGraphicsItem *moving;
	QPointF moving_start;
	Node *firstNode, *secondNode ;
	vector<Node*> nodeVector;	//used by addEdge() method
	StringToEdgeMap edgesMap;
};

#endif
