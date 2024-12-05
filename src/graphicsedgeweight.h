/**
 * @file graphicsedgeweight.h
 * @brief Declares the GraphicsEdgeWeight class for managing and rendering edge weights in the network graph.
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


#ifndef GRAPHICSEDGEWEIGHT_H
#define GRAPHICSEDGEWEIGHT_H


#include <QGraphicsTextItem>
class GraphicsEdge;

static const int TypeEdgeWeight = QGraphicsItem::UserType+5;
static const int ZValueEdgeWeight = 80;

class GraphicsEdgeWeight: public QGraphicsTextItem
{
public:
    GraphicsEdgeWeight(GraphicsEdge * , int, QString);
    void removeRefs();

    enum { Type = UserType + 5 };
    int type() const { return Type; }

    ~GraphicsEdgeWeight();
private:
};

#endif
