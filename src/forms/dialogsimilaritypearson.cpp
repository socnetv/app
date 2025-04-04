/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt
 
                         dialogsimilaritypearson.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
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

 

#include "dialogsimilaritypearson.h"

#include <QDebug>
#include <QPushButton>


DialogSimilarityPearson::DialogSimilarityPearson (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);

    matrixList
            << "Adjacency"
            << "Distances";

    variablesLocationList
            << "Rows"
            << "Columns"
            << "Both";

    (ui.matrixSelect)->insertItems( 1, matrixList );
    (ui.variablesLocationSelect)->insertItems( 1, variablesLocationList );
    (ui.diagonalCheckBox)->setChecked(false);

}



void DialogSimilarityPearson::getUserChoices(){
    qDebug()<< "DialogSimilarityPearson: gathering Data!...";
    QString matrix = (ui.matrixSelect)->currentText();
    QString varLocation = (ui.variablesLocationSelect)->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();
    qDebug()<< "DialogSimilarityPearson: user selected: "
            << matrix
            << varLocation;
    emit userChoices( matrix, varLocation,diagonal );
}


void DialogSimilarityPearson::on_buttonBox_accepted()
{
    this->getUserChoices();
    this->accept();
}

void DialogSimilarityPearson::on_buttonBox_rejected()
{
    this->reject();
}

DialogSimilarityPearson::~DialogSimilarityPearson(){
     matrixList.clear();
     variablesLocationList.clear();
}
