/**
 * @file graphicsnode.h
 * @brief Declares the GraphicsNode class for rendering nodes in the network graph visualization.
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


#ifndef GRAPHICSNODE_H
#define GRAPHICSNODE_H


#include <QGraphicsItem>
#include <QObject>
#include <QPolygon>

class GraphicsWidget;
class QGraphicsSceneMouseEvent;
class GraphicsEdge;
class GraphicsNodeLabel;
class GraphicsNodeNumber;

using namespace std;

static const int TypeNode = QGraphicsItem::UserType+1;
static const int ZValueNode = 100;
static const int ZValueNodeHighlighted = 110;



/**
*  This is actually a container-class.
*  Contains the graphical objects called Nodes,
*  which are displayed as triangles, boxes, circles, etc, on the GraphicsWidget.
*  Each node "knows" the others with which she is connected.
*/
//

class GraphicsNode :  public QObject,  public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES (QGraphicsItem)

public:

    GraphicsNode ( GraphicsWidget* gw,
                   const int &num,
                   const int &size,
                   const QString &color,
                   const QString &shape,
                   const QString &iconPath,
                   const bool &showNumbers,
                   const bool &numbersInside,
                   const QString &numberColor,
                   const int &numberSize,
                   const int &numDistance,
                   const bool &showLabels,
                   const QString &label,
                   const QString &labelColor,
                   const int &labelSize,
                   const int &labelDistance,
                   const bool &edgeHighlighting,
                   QPointF p
                   );

    ~GraphicsNode();

    enum { Type = UserType + 1 };
    int type() const { return Type; }

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    int nodeNumber() {return m_num;}

    void addInEdge( GraphicsEdge *edge ) ;
    void removeInEdge(GraphicsEdge*);
    void addOutEdge( GraphicsEdge *edge ) ;
    void removeOutEdge(GraphicsEdge*);

    void setSize(const int &);
    int size() const;

    void setShape (const QString, const QString &iconPath=QString());
    QString nodeShape() {return m_shape;}

    void setColor(const QString &colorName);
    void setColor(QColor color);
    QString color ();

    void addLabel();
    GraphicsNodeLabel* label();
    void deleteLabel();
    void setLabelVisibility(const bool &toggle);
    void setLabelSize(const int &size);
    void setLabelText ( const QString &label) ;
    void setLabelColor (const QString &color) ;
    QString labelText();
    void setLabelDistance(const int &distance);

    void addNumber () ;
    GraphicsNodeNumber* number();
    void deleteNumber();
    void setNumberVisibility(const bool &toggle);
    void setNumberInside(const bool &toggle);
    void setNumberSize(const int &size);
    void setNumberDistance(const int &distance);
    void setNumberColor(const QString &color);

    void setEdgeHighLighting(const bool &toggle) ;


protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
//    void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
//    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
//    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

signals: 

    void nodeClicked(GraphicsNode*);


private:

    GraphicsWidget *graphicsWidget;
    QPainterPath m_path;
    QPointF newPos;
    QPolygonF *m_poly_t;
    int m_num;
    int m_size, m_size_orig;
    int m_state;
    int m_numSize;
    int m_labelSize;
    int m_numberDistance;
    int m_labelDistance;
    QString m_shape, m_iconPath;
    QString m_numColor;
    QColor m_col, m_col_orig;
    QColor m_col_outline;
    QString m_labelText, m_labelColor;
    bool m_hasNumber, m_hasLabel, m_hasNumberInside, m_edgeHighLighting;
    /**Lists of elements attached to this node */
    list<GraphicsEdge*> inEdgeList, outEdgeList;
    GraphicsNodeLabel *m_label;
    GraphicsNodeNumber *m_number;
};

#endif
