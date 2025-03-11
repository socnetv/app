/**
 * @file graphicsguide.cpp
 * @brief Implements the GraphicsGuide class for drawing guides in the network graph visualization.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#include "graphicsguide.h"

#include "graphicswidget.h"

GraphicsGuide::GraphicsGuide ( GraphicsWidget *gw,
               const double &x0,
               const double &y0,
               const double &radius ) : graphicsWidget ( gw ){
    graphicsWidget->scene()->addItem ( this );
    m_radius=radius;
    setZValue(ZValueGuide);
    circle=true;
    setPos(x0, y0);
}



GraphicsGuide::GraphicsGuide ( GraphicsWidget *gw,
               const double &y0, const int &width)
    : graphicsWidget ( gw ){
    graphicsWidget->scene()->addItem ( this );
    setPos(0, y0);
    m_width= width;
    setZValue(ZValueGuide);
    circle=false;
}



double GraphicsGuide::radius() {
    return m_radius;
}

bool GraphicsGuide::isCircle() {
    return (circle);
}

void GraphicsGuide::setCircle(const QPointF &center,
                      const double &radius ) {
    setPos(center);
    m_radius=radius;
    circle = true;
    update();
}

void GraphicsGuide::setHorizontalLine(const QPointF &origin, const int &width){
    setPos(origin);
    m_width= width;
    circle=false;
    update();
}

int GraphicsGuide::width() {
    return m_width;
}

/** Returns the bounding rectangle of the background circle*/
QRectF GraphicsGuide::boundingRect() const {
    if (circle) {
        return QRectF ( - m_radius-1,  - m_radius-1,  + 2 * m_radius + 1,  + 2* m_radius +1 );
    }
    else  {
        return QRectF ( 1, -1,  m_width, + 1 );
    }
}


void GraphicsGuide::paint ( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * ){
    Q_UNUSED(option);
    painter->setPen ( QPen ( QColor ( "red" ), 1, Qt::DotLine ) );
    if (circle) {
        painter->drawEllipse ( QPointF(0,0), m_radius, m_radius );
    }
    else {
        painter->drawLine ( 0 , 0, m_width , 0);
    }
}



void GraphicsGuide::die (){
    this->prepareGeometryChange();
    this->hide();
    this->update();
    graphicsWidget->scene()->removeItem(this);
    this->update();
}



