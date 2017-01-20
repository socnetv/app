/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 2.2
 Written in Qt
 
                         nodenumber.h  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
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
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#ifndef NODENUMBER_H
#define NODENUMBER_H

#include <QGraphicsTextItem>

class Node;


static const int TypeNumber=QGraphicsItem::UserType+3;
static const int ZValueNodeNumber = 90;

class NodeNumber : public QGraphicsTextItem {
public:
    NodeNumber(Node * ,  const QString &labelText, const int &size);
    enum { Type = UserType + 3 };
	void removeRefs();
	int type() const { return Type; }
	Node* node() { return source; }
    void setSize(const int size);
	~NodeNumber();
private:
	Node *source;
};

#endif
