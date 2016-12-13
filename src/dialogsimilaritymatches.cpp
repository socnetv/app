/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         dialogsimilaritymatches.cpp  -  description
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

 

#include "dialogsimilaritymatches.h"

#include <QDebug>
#include <QPushButton>


DialogSimilarityMatches::DialogSimilarityMatches (QWidget *parent) : QDialog (parent)
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

    methodsList << "Simple / Exact matching"
                <<"Jaccard index"
                <<"Hamming distance"
                <<"Cosine similarity";


    ui.matrixSelect -> insertItems( 1, matrixList );
    (ui.variablesLocationSelect) -> insertItems( 1, variablesLocationList );
    (ui.methodSelect) -> insertItems( 1, methodsList );

    (ui.diagonalCheckBox)->setChecked(false);

}



void DialogSimilarityMatches::gatherData(){
    qDebug()<< "DialogSimilarityMatches: gathering Data!...";
    QString matrix = (ui.matrixSelect) ->currentText();
    QString varLocation = (ui.variablesLocationSelect) ->currentText();
    int method = (ui.methodSelect)->currentIndex();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();

    qDebug()<< "DialogSimilarityMatches: user selected: "
            << matrix
            << varLocation
            << method;
    emit userChoices( matrix, varLocation, method, diagonal  );
			
}


void DialogSimilarityMatches::on_buttonBox_accepted()
{
    this->gatherData();
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
