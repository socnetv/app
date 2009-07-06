/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.80
 Written in Qt 4.4 with KDevelop

                         backgrcircle.h  -  description
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

#include "backgrcircle.h"
#include "graphicswidget.h"

BackgrCircle::BackgrCircle ( GraphicsWidget *gw, int x0, int y0, int radius ) : graphicsWidget ( gw ){
	graphicsWidget->scene()->addItem ( this );
	m_x0=x0;
	m_y0=y0;
	m_radius=radius;
	setZValue ( 250 );
	circle=true;
}



BackgrCircle::BackgrCircle ( GraphicsWidget *gw,  int y0, int w) : graphicsWidget ( gw ){
	graphicsWidget->scene()->addItem ( this );
	m_y0=y0;
	width= w;
	setZValue ( 250 );
	circle=false;
}


/** Returns the bounding rectangle of the background circle*/
QRectF BackgrCircle::boundingRect() const {
	qreal adjust = 4;
	if (circle) {
	 return QRectF ( -m_x0 - m_radius-adjust, -m_y0 - m_radius-adjust,10+m_x0 + m_radius+adjust, 10+m_y0 + m_radius+adjust );
	}
	else  {
	 return QRectF ( 1, m_y0 -adjust,  width, m_y0 + adjust );
	}
}


void BackgrCircle::paint ( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * ){
//	painter->setClipRect( option->exposedRect );
	Q_UNUSED(option);
	painter->setPen ( QPen ( QColor ( "red" ), 1, Qt::DotLine ) );
	if (circle) {
		painter->drawArc ( m_x0-m_radius, m_y0-m_radius, 2*m_radius, 2*m_radius, 0, 5760 );
	}
	else {
		painter->drawLine ( 10 , m_y0, width-10 , m_y0);	
	}
}







