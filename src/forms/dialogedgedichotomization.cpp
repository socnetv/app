/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt
 
                         dialogedgedichotomization.cpp  -  description
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

#include "dialogedgedichotomization.h"
#include <QPushButton>
#include <QDebug>

DialogEdgeDichotomization::DialogEdgeDichotomization (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);


} 



void DialogEdgeDichotomization::getUserChoices(){
    qDebug()<< "Dialog: gathering Data!...";
    qreal my_threshold = ui.weightThreshold->value() ;
    qDebug()<< "DialogEdgeDichotomization::getUserChoices() - We will dichotomize edges according to threshold: " << my_threshold;
    qDebug()<< "Dialog: emitting userChoices" ;
    emit userChoices( my_threshold );
}
