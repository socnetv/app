/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
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
class EdgeLabel;




static const int TypeEdge= QGraphicsItem::UserType+2;


class Edge : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES (QGraphicsItem)

public:
    Edge(GraphicsWidget *, Node*, Node*, const float &weight,
         const QString &label, const QString &color,
         const Qt::PenStyle &style,
         const int&type, const bool & drawArrows, const bool &bezier,
         const bool &weightNumbers=false);
    ~Edge();
    enum { Type = UserType + 2 };
    int type() const { return Type; }
    Node *sourceNode() const;
    void setSourceNode(Node *node);

    Node *targetNode() const;
    void setTargetNode(Node *node);

    void setStartOffset(const int & );
    void setEndOffset(int );
    void removeRefs();

    int sourceNodeNumber();
    int targetNodeNumber();

    void setWeight( const float  &w) ;
    float weight() const;
    void addWeightNumber ();
    void setWeightNumberVisibility  (const bool &toggle);

    void setLabel( const QString &label) ;
    QString label() const;
    void addLabel();
    void setLabelVisibility  (const bool &toggle);

    void showArrows(const bool &);
    void toggleAntialiasing(bool);

    void setUndirected();
    bool isUndirected();
    void setDirectedWithOpposite();

    float width() const;

    QPen pen() const;
    void setStyle( const Qt::PenStyle  &style);
    Qt::PenStyle style() const;

    void setColor( const QString &str) ;
    QString color() const ;
    QString colorToPajek();

    void highlight (const bool &flag);

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
    QPainterPath *m_path;
    QPointF sourcePoint, targetPoint;
    qreal m_arrowSize, m_startOffset, m_endOffset;
    Qt::PenStyle m_style;
    EdgeWeight* weightNumber;
    EdgeLabel* edgeLabel;

    QString m_color, m_tempColor, m_colorNegative, m_label;
    int eFrom, eTo;
    float m_weight, m_tempweight;
    int tox1, tox2, toy1, toy2, size;
    int m_edgeType;
    double rad, theta, theta1, theta2;
    qreal angle, line_length, line_dx, line_dy;
    bool m_Bezier, m_drawArrows, m_directed_first, m_drawWeightNumber;
    bool m_drawLabel;
};

#endif
