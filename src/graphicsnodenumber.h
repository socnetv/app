/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 3.0-rc1
 Written in Qt
 
                         graphicsnodenumber.h  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

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
