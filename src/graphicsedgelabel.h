/**
 * @file graphicsedgelabel.h
 * @brief Declares the GraphicsEdgeLabel class for managing and rendering edge labels in the network graph.
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


#ifndef GRAPHICSEDGELABEL_H
#define GRAPHICSEDGELABEL_H


#include <QGraphicsTextItem>
class GraphicsEdge;

static const int TypeEdgeLabel = QGraphicsItem::UserType+6;
static const int ZValueEdgeLabel = 80;

class GraphicsEdgeLabel: public QGraphicsTextItem
{
public:
    GraphicsEdgeLabel(GraphicsEdge * , int, QString);
    void removeRefs();

    enum { Type = UserType + 6 };
    int type() const { return Type; }

    ~GraphicsEdgeLabel();
private:
};

#endif
