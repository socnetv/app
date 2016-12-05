/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         pearsoncorrelationdialog.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2016 by Dimitris B. Kalamaras
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

 

#include "pearsoncorrelationdialog.h"

#include <QDebug>
#include <QPushButton>


PearsonCorrelationDialog::PearsonCorrelationDialog (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    matrixList
            << "Adjacency"
            << "Distances";

    variablesLocationList
            << "Rows"
            << "Columns"
            << "Both";

    (ui.matrixSelect) -> insertItems( 1, matrixList );
    (ui.variablesLocationSelect) -> insertItems( 1, variablesLocationList );

}



void PearsonCorrelationDialog::gatherData(){
    qDebug()<< "PearsonCorrelationDialog: gathering Data!...";
    QString matrix = (ui.matrixSelect) ->currentText();
    QString varLocation = (ui.variablesLocationSelect) ->currentText();

    qDebug()<< "PearsonCorrelationDialog: user selected: "
            << matrix
            << varLocation;
    emit userChoices( matrix, varLocation  );
			
}


void PearsonCorrelationDialog::on_buttonBox_accepted()
{
    this->gatherData();
    this->accept();
}

void PearsonCorrelationDialog::on_buttonBox_rejected()
{
    this->reject();
}

PearsonCorrelationDialog::~PearsonCorrelationDialog(){
     matrixList.clear();
     variablesLocationList.clear();
}
