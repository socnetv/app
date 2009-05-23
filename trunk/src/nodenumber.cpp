/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.6
 Written in Qt 4.4   

                        nodenumber.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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

#include "nodenumber.h"
#include "node.h"
#include <QFont>

NodeNumber::NodeNumber( Node *jim , int size, QString labelText, QGraphicsScene *scene ) :QGraphicsTextItem(0,scene) {
	source=jim;
	jim -> addNumber(this);
	setPlainText( labelText ); 
	setFont( QFont ("Times", size, QFont::Black, FALSE) );
	setZValue(254);
}

void NodeNumber::removeRefs(){
	source->deleteNumber();

}

NodeNumber::~NodeNumber(){

}
