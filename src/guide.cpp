/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.1
 Written in Qt

                         Guide.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
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

Guide::Guide ( GraphicsWidget *gw,
               const double &x0,
               const double &y0,
               const double &radius ) : graphicsWidget ( gw ){
    graphicsWidget->scene()->addItem ( this );
    m_radius=radius;
    setZValue ( 250 );
    circle=true;
    setPos(x0, y0);
}



Guide::Guide ( GraphicsWidget *gw,
               const double &y0, const int &width)
    : graphicsWidget ( gw ){
    graphicsWidget->scene()->addItem ( this );
    setPos(0, y0);
    m_width= width;
    setZValue ( 250 );
    circle=false;
}



double Guide::radius() {
    return m_radius;
}

bool Guide::isCircle() {
    return (circle);
}

void Guide::setCircle(const QPointF &center,
                      const double &radius ) {
    setPos(center);
    m_radius=radius;
    circle = true;
    update();
}

void Guide::setHorizontalLine(const QPointF &origin, const int &width){
    setPos(origin);
    m_width= width;
    circle=false;
    update();
}

int Guide::width() {
    return m_width;
}

/** Returns the bounding rectangle of the background circle*/
QRectF Guide::boundingRect() const {
    if (circle) {
        return QRectF ( - m_radius-1,  - m_radius-1,  + 2 * m_radius + 1,  + 2* m_radius +1 );
    }
    else  {
        return QRectF ( 1, -1,  m_width, + 1 );
    }
}


void Guide::paint ( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * ){
    Q_UNUSED(option);
    painter->setPen ( QPen ( QColor ( "red" ), 1, Qt::DotLine ) );
    if (circle) {
        painter->drawEllipse ( QPointF(0,0), m_radius, m_radius );
    }
    else {
        painter->drawLine ( 0 , 0, m_width , 0);
    }
}



void Guide::die (){
    this->prepareGeometryChange();
    this->hide();
    this->update();
    graphicsWidget->scene()->removeItem(this);
    this->update();
}



