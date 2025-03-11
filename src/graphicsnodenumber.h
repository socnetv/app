/**
 * @file graphicsnodenumber.h
 * @brief Declares the GraphicsNodeNumber class for rendering node numbers in the network graph visualization.
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


#ifndef GRAPHICSNODENUMBER_H
#define GRAPHICSNODENUMBER_H

#include <QGraphicsTextItem>

class GraphicsNode;

static const int TypeNumber=QGraphicsItem::UserType+3;
static const int ZValueNodeNumber = 90;

class GraphicsNodeNumber : public QGraphicsTextItem {
public:
    GraphicsNodeNumber(GraphicsNode * ,  const QString &labelText, const int &size);
    enum { Type = UserType + 3 };
	void removeRefs();
	int type() const { return Type; }
	GraphicsNode* node() { return source; }
    void setSize(const int size);
    ~GraphicsNodeNumber();
private:
	GraphicsNode *source;
};

#endif
