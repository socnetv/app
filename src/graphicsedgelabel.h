/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt
                         graphicsedgelabel.h  -  description
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
*                                                                               *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

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
