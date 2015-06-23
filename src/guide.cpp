/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                         Guide.cpp  -  description
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

#include "guide.h"
#include "graphicswidget.h"

Guide::Guide ( GraphicsWidget *gw, int x0, int y0, int radius ) : graphicsWidget ( gw ){
	graphicsWidget->scene()->addItem ( this );
	m_x0=x0;
	m_y0=y0;
	m_radius=radius;
	setZValue ( 250 );
	circle=true;
}



Guide::Guide ( GraphicsWidget *gw,  int y0, int w) : graphicsWidget ( gw ){
	graphicsWidget->scene()->addItem ( this );
	m_y0=y0;
	width= w;
	setZValue ( 250 );
	circle=false;
}


/** Returns the bounding rectangle of the background circle*/
QRectF Guide::boundingRect() const {
    if (circle) {
     return QRectF ( m_x0 - m_radius-1, m_y0 - m_radius-1, m_x0 + 2 * m_radius + 1, m_y0 + 2* m_radius +1 );
	}
	else  {
     return QRectF ( 1, m_y0 -1,  width, m_y0 + 1 );
	}
}


void Guide::paint ( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * ){
	Q_UNUSED(option);
	painter->setPen ( QPen ( QColor ( "red" ), 1, Qt::DotLine ) );
	if (circle) {
        painter->drawEllipse ( QPoint(m_x0, m_y0), m_radius, m_radius );
	}
	else {
		painter->drawLine ( 10 , m_y0, width-10 , m_y0);	
	}
}



void Guide::die (){
	this->prepareGeometryChange();
	this->hide();
	this->update();
	graphicsWidget->scene()->removeItem(this);
	this->update();
}



