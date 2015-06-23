/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 1.9
 Written in Qt
 
                         nodelabel.h  -  description
                             -------------------

    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#ifndef NODELABEL_H
#define NODELABEL_H

#include <QGraphicsTextItem>
class Node;

static const int TypeLabel = QGraphicsItem::UserType+4;

class NodeLabel : public QGraphicsTextItem{
public: 
    NodeLabel(Node * , int size,  QString);
	
	void removeRefs();
 	enum { Type = UserType + 4 };
	int type() const { return Type; }
	~NodeLabel();
	Node* node() { return source; }
private:
	Node *source;	
};

#endif
