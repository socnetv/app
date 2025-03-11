/**
 * @file graphicsnodenumber.cpp
 * @brief Implements the GraphicsNodeNumber class for rendering node numbers in the network graph visualization.
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


#include "graphicsnodenumber.h"

#include "graphicsnode.h"
#include <QFont>
#include <QDebug>


GraphicsNodeNumber::GraphicsNodeNumber( GraphicsNode *jim , const QString &labelText, const int &size)
    :QGraphicsTextItem(jim) {
    source=jim;
    setParentItem(jim); //auto disables child items like this, when node is disabled.
    setPlainText( labelText );
    setFont( QFont ("Times", size, QFont::Black, false) );
    setZValue(ZValueNodeNumber);
    setAcceptHoverEvents(false);
    qDebug() << "GraphicsNodeNumber() - initialized";
}

void GraphicsNodeNumber::setSize(const int size) {
    prepareGeometryChange();
    setFont( QFont ("Times", size, QFont::Black, false) );
    //update();
}

void GraphicsNodeNumber::removeRefs(){
    source->deleteNumber();

}

GraphicsNodeNumber::~GraphicsNodeNumber(){

}
