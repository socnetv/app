/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                        nodelabel.cpp  -  description
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

#include "nodelabel.h"
#include "node.h"
#include <QFont>


NodeLabel::NodeLabel(Node *jim , const QString &text,  const int &size) :
    QGraphicsTextItem(jim) {
    source=jim;
    setParentItem(jim); //auto disables child items like this, when node is disabled.
    setPlainText( text );
    setFont( QFont ("Times", size, QFont::Light, true) );
    setZValue(ZValueNodeLabel);
    setAcceptHoverEvents(false);
}


void NodeLabel::setSize(const int &size) {
    prepareGeometryChange();
    setFont( QFont ("Times", size, QFont::Black, false) );
    //update();
}

void NodeLabel::removeRefs(){
    source->deleteLabel();
}


NodeLabel::~NodeLabel(){
}
