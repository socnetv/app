/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.90
 Written in Qt 4.4
 
                         graphicswidget.h  -  description
                             -------------------
    copyright            : (C) 2005-2010 by Dimitris B. Kalamaras
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
class EdgeWeight;

typedef QMap<QString, Edge*> StringToEdgeMap;
typedef QHash <long int, Node*> hash_in;

class GraphicsWidget : public QGraphicsView {
	Q_OBJECT
public:
 	GraphicsWidget(QGraphicsScene*, MainWindow* parent);
	~GraphicsWidget();
	void clear();

	Node* hasNode(QString text);
//	Node* hasNode(int number);
	bool setMarkedNode(QString text);
	
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

	void setNumbersInsideNodes(bool);
	
	bool setNodeColor(long int, QString);
	
	bool setEdgeColor(int, int, QString);
	
	bool setEdgeWeight(int, int, float);
	
	void setAllItemsVisibility(int, bool);
	
	void removeAllItems(int);
protected:
	void wheelEvent(QWheelEvent *event);
	void mouseDoubleClickEvent ( QMouseEvent * e );
	void mousePressEvent ( QMouseEvent * e );
	//void mouseReleaseEvent(QMouseEvent * e );
	void resizeEvent( QResizeEvent *e );
	void paintEvent ( QPaintEvent * event );

public slots:
	void drawNode(	int i, int size, QString aColor, 
					QString nColor, int nSize, 
					QString label, QString lColor, int lSize, 
					QPointF p, 
					QString nodeShape, 
					bool showLabels, bool labelIn, bool showNumbers
					);
	void eraseNode(long int doomedJim);
	void drawEdge(int, int, float, bool, bool, QString, bool);
	void eraseEdge(int, int);
	void setEdgeVisibility ( int, int, bool);
	void setNodeVisibility(long int, bool );	//Called from Graph via MW
	
	void nodeClicked(Node *);
	void edgeClicked(Edge *);
	void openNodeContextMenu();
	void openEdgeContextMenu();
	
	void moveNode(int, int, int);			//Called from Graph when creating random nets.
	void changeZoom(const int value); 
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
	void userMiddleClicked(int, int, float);
	void openNodeMenu();
	void openEdgeMenu();
	void updateNodeCoords(int, int, int);
	void selectedNode(Node *);
	void selectedEdge(Edge *);
	void zoomChanged(int);
	
private:
	hash_in nodeHash;			//This is used in drawEdge() method
	StringToEdgeMap edgesMap;
	int timerId,  layoutType, m_nodeSize, m_numberDistance, m_labelDistance;
	double m_currentScaleFactor;
	int m_currentRotationAngle;
	int zoomIndex, originalNodeSize;
	QString m_nodeLabel, m_numberColor, m_nodeColor, m_labelColor, m_linkColor;
	bool secondDoubleClick, dynamicMovement, markedNodeExists;
	QGraphicsItem *moving;
	QPointF startPoint, endPoint;
	Node *firstNode, *secondNode, *markedNode, *tempNode ;

};

#endif
