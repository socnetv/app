/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt
 
                         graphicsedge.h  -  description
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

#ifndef GRAPHICSEDGE_H
#define GRAPHICSEDGE_H


#include <QGraphicsItem>
#include <QObject>
#include <utility> //declares pair construct


class GraphicsWidget;
class QGraphicsSceneMouseEvent;
class GraphicsNode;
class GraphicsEdgeWeight;
class GraphicsEdgeLabel;

using namespace std;


static const int TypeEdge= QGraphicsItem::UserType+2;
static const int ZValueEdge = 50;
static const int ZValueEdgeHighlighted = 99;

static const int EDGE_STATE_REGULAR = 0;
static const int EDGE_STATE_HIGHLIGHT = 1;
static const int EDGE_STATE_HOVER = 2;





class GraphicsEdge : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES (QGraphicsItem)

public:
    GraphicsEdge(GraphicsWidget *, GraphicsNode*, GraphicsNode*, const qreal &weight,
         const QString &label, const QString &color,
         const Qt::PenStyle &style,
         const int&type, const bool & drawArrows, const bool &bezier,
         const bool &weightNumbers=false,
                 const bool &highlighting=true);
    ~GraphicsEdge();

    enum { Type = UserType + 2 };
    int type() const { return Type; }

    GraphicsNode *sourceNode() const;
    void setSourceNode(GraphicsNode *node);

    GraphicsNode *targetNode() const;
    void setTargetNode(GraphicsNode *node);

    int sourceNodeNumber();
    int targetNodeNumber();

    void setSourceNodeSize(const int & size);
    void setTargetNodeSize(const int & size);

    void setMinimumOffsetFromNode(const int & offset);

    void removeRefs();

    void setWeight( const qreal  &w) ;
    qreal weight() const;
    void addWeightNumber ();
    //void deleteWeightNumber();
    void setWeightNumberVisibility  (const bool &toggle);

    void setLabel( const QString &label) ;
    QString label() const;
    void addLabel();
    //void deleteLabel();
    void setLabelVisibility  (const bool &toggle);

    void showArrows(const bool &);

    void setDirectionType(const int &dirType=0);
    int directionType();

    qreal width() const;

    QPen pen() const;
    void setState(const int &state);

    void setStyle( const Qt::PenStyle  &style);
    Qt::PenStyle style() const;

    void setColor( const QString &str) ;
    QString color() const ;
    QString colorToPajek();

    void setHighlighted (const bool &flag);
    void setHighlighting (const bool &toggle);

    QPainterPath shape() const;


public slots:
    void adjust ();

protected:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);



private:
    GraphicsWidget *graphicsWidget;
    GraphicsNode *source, *target;
    QPainterPath m_path, *m_path_up, *m_path_down, *m_path_shape;
    QPointF sourcePoint, targetPoint;
    qreal m_arrowSize;

    qreal m_minOffsetFromNode;
    qreal m_offsetFromTargetNode, m_offsetFromSourceNode;

    Qt::PenStyle m_style;
    int m_state;
    GraphicsEdgeWeight* weightNumber;
    GraphicsEdgeLabel* edgeLabel;

    QString m_color, m_colorNegative, m_label;

    qreal m_weight;
    int tox1, tox2, toy1, toy2, size;
    int sourceOrigSize;
    int targetOrigSize;
    int m_edgeDirType;
    double rad, theta, theta1, theta2;
    qreal angle, line_length, line_dx, line_dy;
    bool m_Bezier, m_drawArrows, m_drawWeightNumber;
    bool m_drawLabel, m_hoverHighlighting;
    bool m_isClicked;
};

#endif
