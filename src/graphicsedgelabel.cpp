/**
 * @file graphicsedgelabel.cpp
 * @brief Implements the GraphicsEdgeLabel class for rendering edge labels in the network graph.
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


#include "graphicsedgelabel.h"

#include "graphicsedge.h"
#include <QDebug>
#include <QFont>


GraphicsEdgeLabel::GraphicsEdgeLabel( GraphicsEdge *link , int size, QString labelText)
: QGraphicsTextItem( 0)
{
    qDebug()<< "GraphicsEdgeLabel:: creating new edgelabel and attaching it to link";
	setPlainText( labelText );
    setParentItem(link); //auto disables child items like this, when link is disabled.
    this->setFont( QFont ("Courier", size, QFont::Light, true) );
    setZValue(ZValueEdgeLabel);
}


GraphicsEdgeLabel::~GraphicsEdgeLabel()
{
}


