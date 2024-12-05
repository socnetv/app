/**
 * @file graphicsnodelabel.h
 * @brief Declares the GraphicsNodeLabel class for rendering labels associated with nodes in the network graph visualization.
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


#ifndef GRAPHICSNODELABEL_H
#define GRAPHICSNODELABEL_H

#include <QGraphicsTextItem>
class GraphicsNode;

static const int TypeLabel = QGraphicsItem::UserType+4;
static const int ZValueNodeLabel = 80;

class GraphicsNodeLabel : public QGraphicsTextItem{
public: 
    GraphicsNodeLabel(GraphicsNode * , const QString &text, const int &size  );
	
	void removeRefs();
 	enum { Type = UserType + 4 };
	int type() const { return Type; }
    void setSize(const int &size);
    ~GraphicsNodeLabel();
	GraphicsNode* node() { return source; }
private:
	GraphicsNode *source;	
};

#endif
