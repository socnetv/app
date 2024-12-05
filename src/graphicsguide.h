/**
 * @file graphicsguide.h
 * @brief Declares the GraphicsGuide class for drawing guides in the network graph visualization.
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



