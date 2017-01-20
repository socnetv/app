/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         node.h  -  description
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

#ifndef NODE_H
#define NODE_H


#include <QGraphicsItem>
#include <QObject>
#include <QPolygon>

class GraphicsWidget;
class QGraphicsSceneMouseEvent;
class Edge;
class NodeLabel;
class NodeNumber;

using namespace std;

static const int TypeNode = QGraphicsItem::UserType+1;
static const int ZValueNode = 100;
static const int ZValueNodeHighlighted = 110;

/**
*  This is actually a container-class.
*  Contains the graphical objects called Nodes,
*  which are displayed as triangles, boxes, circles, etc, on the canvas.
*  Each node "knows" the others with which she is connected.
*/
//

class Node :  public QObject,  public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES (QGraphicsItem)

public:
    Node (GraphicsWidget* gw, const int &num, const int &size,
          const QString &color, const QString &shape,
          const bool &showNumbers, const bool &numbersInside,
          const QString &numberColor, const int &numberSize, const int &numDistance,
          const bool &showLabels, const QString &label, const QString &labelColor,
          const int &labelSize, const int &labelDistance,
          QPointF p
          );
    ~Node();

    enum { Type = UserType + 1 };
    int type() const { return Type; }

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    long int nodeNumber() {return m_num;}

    void addInLink( Edge *edge ) ;
    void deleteInLink(Edge*);
    void addOutLink( Edge *edge ) ;
    void deleteOutLink(Edge*);

    void setSize(const int &);
    int size() const;

    void setShape (const QString);
    QString nodeShape() {return m_shape;}

    void setColor(QString str);
    void setColor(QColor color);
    QString color ();

    void addLabel();
    NodeLabel* label();
    void deleteLabel();
    void setLabelVisibility(const bool &toggle);
    void setLabelSize(const int &size);
    void setLabelText ( QString label) ;
    void setLabelColor (const QString &color) ;
    QString labelText();
    void setLabelDistance(const int &distance);

    void addNumber () ;
    NodeNumber* number();
    void deleteNumber();
    void setNumberVisibility(const bool &toggle);
    void setNumberInside(const bool &toggle);
    void setNumberSize(const int &size);
    void setNumberDistance(const int &distance);
    void setNumberColor(const QString &color);

    void toggleAntialiasing(bool);


protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
signals: 
    void nodeClicked(Node*);
    void startEdge(Node *);
    void adjustOutEdge();
    void adjustInEdge();
    void removeOutEdge();
    void removeInEdge();
private:
    GraphicsWidget *graphicsWidget;
    QPainterPath *m_path;
    QPointF newPos;
    QPolygonF *m_poly_t;
    int  m_size, m_numSize, m_labelSize, m_numberDistance, m_labelDistance;
    long int m_num;
    QString  m_shape,  m_col_str, m_numColor, m_labelText, m_labelColor;
    QColor m_col;
    bool m_hasNumber, m_hasLabel, m_hasNumberInside;
    /**Lists of elements attached to this node */
    list<Edge*> inEdgeList, outEdgeList;
    NodeLabel *m_label;
    NodeNumber *m_number;
};

#endif
