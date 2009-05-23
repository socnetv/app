/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.6
 Written in Qt 4.4
 
                         node.h  -  description
                          -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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

#ifndef NODE_H
#define NODE_H

using namespace std;

#include <QGraphicsItem>
#include <QObject>
#include <QPolygon>

class GraphicsWidget;
class QGraphicsSceneMouseEvent;
class Edge;
class NodeLabel;
class NodeNumber;



static const int TypeNode = QGraphicsItem::UserType+1;


/**
*  This is actually a container-class.
*  Contains the graphical objects called Nodes,
*  which are displayed as triangles, boxes, circles, etc, on the canvas.
*  Each node "knows" the others with which she is connected.
*/
//

class Node :  public QObject,  public QGraphicsItem {

  Q_OBJECT
	
public:
	Node(GraphicsWidget*, int num, int size, QString col, QString shape, bool, int, int, QPointF p) ;
//	~Node();

	enum { Type = UserType + 1 };
	int type() const { return Type; }

	void calculateForcesSpringEmbedder(bool dynamicMovement);
	void calculateForcesFruchterman(bool dynamicMovement);


	QRectF boundingRect() const;
	QPainterPath shape() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	void die(); 
	int nodeNumber() {return m_num;}

	void setSize(int);
	int size();

	void setShape (QString);
	QString nodeShape() {return m_shape;}

	void setColor(QString str);
	void setColor(QColor color);
	QString color ();
	
	void setLabel ( QString label) ;		//Only used by MW to update graphic labels
	QString label ( ) ;

	void addInLink( Edge *edge ) ;
	void deleteInLink(Edge*);

	void addOutLink( Edge *edge ) ;
	void deleteOutLink(Edge*);

	void addLabel (NodeLabel* gfxLabel  ) ;
	void deleteLabel();
	void clearLabel();
	
	void setNumberInside(bool);

	void addNumber (NodeNumber *gfxNum ) ;
	void deleteNumber();
	
	
	void toggleAntialiasing(bool);
protected:
 	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
signals: 
	void nodeClicked(Node*);
	void startNodeMovement(int);	
	void openNodeContextMenu();
	void startEdge(Node *);
	void adjustOutEdge();
	void adjustInEdge();
	void removeOutEdge();
	void removeInEdge();
private:
	GraphicsWidget *graphicsWidget;
	QPainterPath *m_path;
	QPointF newPos;
	QPolygon *m_poly_t, *m_poly_d;
	int m_num, m_size, m_nd, m_ld;
	QString  m_shape,  m_col_str, m_labelIn;
	QColor m_col, m_col_dark;
	bool m_hasNumber, m_hasLabel, m_isNumberInside;
	/**Lists of elements attached to this node */
	list<Edge*> inEdgeList, outEdgeList;
	list<NodeNumber*> gfxNumberList;
	NodeLabel *m_label;
	NodeNumber *m_number;
};

#endif
