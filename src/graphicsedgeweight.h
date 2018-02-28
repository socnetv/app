/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt
                         graphicsedgeweight.h  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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
