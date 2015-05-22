/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 1.8-dev
 Written in Qt

                        edgeweight.cpp  -  description
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

#include "edgeweight.h"
#include "edge.h"
#include <QDebug>
#include <QFont>


EdgeWeight::EdgeWeight( Edge *link , int size, QString labelText)
: QGraphicsTextItem( 0)
{
    qDebug()<< "EdgeWeight:: creating new edgeweight and attaching it to link";
	link -> addWeight(this);
	setPlainText( labelText );
    setParentItem(link); //auto disables child items like this, when link is disabled.
    this->setFont( QFont ("Courier", size, QFont::Light, true) );
    setZValue(253);
}

void EdgeWeight::move(double x, double y) {
	this -> move(x,y);
}

EdgeWeight::~EdgeWeight()
{
}


