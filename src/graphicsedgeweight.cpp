/**
 * @file graphicsedgeweight.cpp
 * @brief Implements the GraphicsEdgeWeight class for managing and rendering edge weights in the network graph.
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


#include "graphicsedgeweight.h"

#include "graphicsedge.h"
#include <QDebug>
#include <QFont>


GraphicsEdgeWeight::GraphicsEdgeWeight( GraphicsEdge *link , int size, QString labelText)
: QGraphicsTextItem( 0)
{
    qDebug()<< "GraphicsEdgeWeight:: creating new edgeweight and attaching it to link";
	setPlainText( labelText );
    setParentItem(link); //auto disables child items like this, when link is disabled.
    this->setFont( QFont ("Courier", size, QFont::Light, true) );
    setZValue(ZValueEdgeWeight);
}


GraphicsEdgeWeight::~GraphicsEdgeWeight()
{
}


