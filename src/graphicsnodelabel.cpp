/**
 * @file graphicsnodelabel.cpp
 * @brief Implements the GraphicsNodeLabel class for rendering labels associated with nodes in the network graph visualization.
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


#include "graphicsnodelabel.h"

#include "graphicsnode.h"
#include <QFont>
#include <QDebug>

GraphicsNodeLabel::GraphicsNodeLabel(GraphicsNode *jim , const QString &text,  const int &size) :
    QGraphicsTextItem(jim) {

    source=jim;
    setParentItem(jim); //auto disables child items like this, when node is disabled.
    setPlainText( text );
    setFont( QFont ("Times", size, QFont::Light, true) );
    setZValue(ZValueNodeLabel);
    setAcceptHoverEvents(false);
    qDebug() << "GraphicsNodeLabel() - initialized";
}


void GraphicsNodeLabel::setSize(const int &size) {
    prepareGeometryChange();
    setFont( QFont ("Times", size, QFont::Black, false) );
    //update();
}

void GraphicsNodeLabel::removeRefs(){
    source->deleteLabel();
}


GraphicsNodeLabel::~GraphicsNodeLabel(){
}
