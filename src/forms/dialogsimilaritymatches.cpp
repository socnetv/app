/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt
 
                         dialogsimilaritymatches.cpp  -  description
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

 

#include "dialogsimilaritymatches.h"

#include <QDebug>
#include <QPushButton>


DialogSimilarityMatches::DialogSimilarityMatches (QWidget *parent) : QDialog (parent)
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

    measureList << "Simple / Exact matching"
                <<"Jaccard index"
                <<"Hamming distance"
                <<"Cosine similarity"
                <<"Euclidean distance";


    ui.matrixSelect->insertItems( 1, matrixList );
    (ui.variablesLocationSelect)->insertItems( 1, variablesLocationList );
    (ui.measureSelect)->insertItems( 1, measureList );

    (ui.diagonalCheckBox)->setChecked(false);

}



void DialogSimilarityMatches::getUserChoices(){
    qDebug()<< "DialogSimilarityMatches: gathering Data!...";
    QString matrix = (ui.matrixSelect)->currentText();
    QString varLocation = (ui.variablesLocationSelect)->currentText();
    QString measure = (ui.measureSelect)->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();

    qDebug()<< "DialogSimilarityMatches: user selected: "
            << matrix
            << varLocation
            << measure;
    emit userChoices( matrix, varLocation, measure, diagonal  );
			
}


void DialogSimilarityMatches::on_buttonBox_accepted()
{
    this->getUserChoices();
    this->accept();
}

void DialogSimilarityMatches::on_buttonBox_rejected()
{
    this->reject();
}

DialogSimilarityMatches::~DialogSimilarityMatches(){
     matrixList.clear();
     variablesLocationList.clear();
}
