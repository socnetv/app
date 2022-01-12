/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 3.1.0-dev
 Written in Qt

                        graphicsedgeweight.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2022 by Dimitris B. Kalamaras
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

#include "graphicsedgeweight.h"

#include "graphicsedge.h"
#include <QDebug>
#include <QFont>


GraphicsEdgeWeight::GraphicsEdgeWeight( GraphicsEdge *link , int size, QString labelText)
: QGraphicsTextItem( 0)
{
    qDebug()<< "GraphicsEdgeWeight:: creating new edgeweight and attaching it to link";
	setPlainText( labelText );
    setParentItem(link); //auto disables child items like this, when link is disabled.
    this->setFont( QFont ("Courier", size, QFont::Light, true) );
    setZValue(ZValueEdgeWeight);
}


GraphicsEdgeWeight::~GraphicsEdgeWeight()
{
}


