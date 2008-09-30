/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.48
 Written in Qt 4.4
 
                         edge.h  -  description
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

#ifndef EDGE_H
#define EDGE_H

using namespace std;

#include <QGraphicsItem>
#include <QObject>
#include <utility> //declares pair construct


class GraphicsWidget;
class QGraphicsSceneMouseEvent;
class Node;
class EdgeWeight;


static const int TypeEdge= QGraphicsItem::UserType+2;


class Edge : public QObject, public QGraphicsItem {
	Q_OBJECT
public:
	Edge(GraphicsWidget *, Node*, Node*, int, int, QString, bool, bool, bool);

	enum { Type = UserType + 2 };
	int type() const { return Type; }
	Node *sourceNode() const;
	void setSourceNode(Node *node);

	Node *targetNode() const;
	void setTargetNode(Node *node);
	
	void setStartOffset(int );
	void setEndOffset(int );
	void removeRefs();

	int sourceNodeNumber();
	int targetNodeNumber();
	void setWeight( int w) ;
	int weight();
	void setColor( QString str) ;
	QString color() ;
	void addWeight (EdgeWeight* canvasWeight  ) ;
	void clearWeightList();
	int lineWidth(); 
	void showArrows(bool);
	void toggleAntialiasing(bool);

	void makeReciprocal();
	void unmakeReciprocal();

	QPainterPath shape() const;

public slots:
	void adjust ();
	void remove();
protected:
	QRectF boundingRect() const;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);


private:
	GraphicsWidget *graphicsWidget;
	Node *source, *target;
	QPointF sourcePoint, targetPoint;
	qreal m_arrowSize, m_startOffset, m_endOffset;
	list<EdgeWeight*> weightList;
	QString m_color;
	int eFrom, eTo, m_weight;
	int tox1, tox2, toy1, toy2, size;
	double rad, theta, theta1, theta2;
	bool m_Bezier, m_drawArrows, m_reciprocal;

};

#endif
