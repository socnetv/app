/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.6-dev
 Written in Qt
 
                         graphicsguide.h  -  description
                             -------------------
    copyright         : (C) 2005-2019 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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

#ifndef GRAPHICSGUIDE_H
#define GRAPHICSGUIDE_H



#include <QGraphicsItem>
#include <QObject>


class GraphicsWidget;

static const int TypeGuide = QGraphicsItem::UserType+7;
static const int ZValueGuide = 10;

class GraphicsGuide :  public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES (QGraphicsItem)

public:
    GraphicsGuide(GraphicsWidget *,
          const double &x0, const double &y0, const double &radius );
    GraphicsGuide(GraphicsWidget *,
          const double &y0, const int &width);
    bool isCircle();
    void setCircle(const QPointF &center, const double &radius) ;
    void setHorizontalLine(const QPointF &origin, const int &width) ;
    double radius();
    int width();
    enum { Type = UserType + 7 };
	int type() const { return Type; }
	void die();
	

protected:
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private: 
	GraphicsWidget *graphicsWidget;
    double m_radius;
    int m_width;
	bool circle;

};

#endif



