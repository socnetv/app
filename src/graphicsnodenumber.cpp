/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt

                        graphicsnodenumber.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
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

#include "graphicsnodenumber.h"

#include "graphicsnode.h"
#include <QFont>
#include <QDebug>


GraphicsNodeNumber::GraphicsNodeNumber( GraphicsNode *jim , const QString &labelText, const int &size)
    :QGraphicsTextItem(jim) {
    source=jim;
    setParentItem(jim); //auto disables child items like this, when node is disabled.
    setPlainText( labelText );
    setFont( QFont ("Times", size, QFont::Black, false) );
    setZValue(ZValueNodeNumber);
    setAcceptHoverEvents(false);
    qDebug() << "GraphicsNodeNumber() - initialized";
}

void GraphicsNodeNumber::setSize(const int size) {
    prepareGeometryChange();
    setFont( QFont ("Times", size, QFont::Black, false) );
    //update();
}

void GraphicsNodeNumber::removeRefs(){
    source->deleteNumber();

}

GraphicsNodeNumber::~GraphicsNodeNumber(){

}
