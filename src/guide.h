/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         Guide.h  -  description
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

#ifndef Guide_H
#define Guide_H



#include <QGraphicsItem>
#include <QObject>


class GraphicsWidget;

static const int TypeGuide = QGraphicsItem::UserType+6;

class Guide : public QObject, public  QGraphicsItem {
	Q_OBJECT
	Q_INTERFACES (QGraphicsItem)

public:
    Guide(GraphicsWidget *, int, int, int );
    Guide(GraphicsWidget *, int, int );
	enum { Type = UserType + 6 };
	int type() const { return Type; }
	void die();
	

protected:
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private: 
	GraphicsWidget *graphicsWidget;
	int m_x0, m_y0, m_radius, width;
	bool circle;

};

#endif



