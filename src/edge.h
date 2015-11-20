/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 1.9
 Written in Qt
 
                         edge.h  -  description
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
	Q_INTERFACES (QGraphicsItem)

public:
    Edge(GraphicsWidget *, Node*, Node*,
         const Qt::PenStyle &style,
         const float &, const int &, const QString &,
         const bool&, const bool&, const bool &);
    ~Edge();
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
    void setWeight( const float  &w) ;

    float weight() const;
    void addWeight (EdgeWeight* canvasWeight  ) ;
    void clearWeightList();

    void showArrows(bool);
    void toggleAntialiasing(bool);

    void makeReciprocal();
    void unmakeReciprocal();
    bool isReciprocal();

    float width() const;

    QPen pen() const;
    void setStyle( const Qt::PenStyle  &style);
    Qt::PenStyle style() const;

    void setColor( const QString &str) ;
    QString color() const ;
    QString colorToPajek();

	QPainterPath shape() const;

public slots:
	void adjust ();

protected:
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);


private:
	GraphicsWidget *graphicsWidget;
	Node *source, *target;
	QPointF sourcePoint, targetPoint;
	qreal m_arrowSize, m_startOffset, m_endOffset;
    Qt::PenStyle m_style;
	list<EdgeWeight*> weightList;
	QString m_color;
	int eFrom, eTo;
	float m_weight;
	int tox1, tox2, toy1, toy2, size;
	double rad, theta, theta1, theta2;
	qreal angle, line_length, line_dx, line_dy;
	bool m_Bezier, m_drawArrows, m_reciprocal;
};

#endif
