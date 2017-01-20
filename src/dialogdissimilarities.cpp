/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         dialogdissimilarities.cpp  -  description
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

 

#include "dialogdissimilarities.h"

#include <QDebug>
#include <QPushButton>


DialogDissimilarities::DialogDissimilarities (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    variablesLocationList
            << "Rows"
            << "Columns"
            << "Both";

    metricList  << tr("Euclidean distance")
                << tr("Manhattan distance")
                << tr("Hamming distance")
                << tr("Jaccard distance")
                << tr("Chebyshev distance");



    (ui.variablesLocationSelect) -> insertItems( 1, variablesLocationList );
    (ui.metricSelect) -> insertItems( 1, metricList );

    (ui.diagonalCheckBox)->setChecked(false);

}



void DialogDissimilarities::gatherData(){
    qDebug()<< "DialogDissimilarities: gathering Data!...";
    QString varLocation = (ui.variablesLocationSelect) ->currentText();
    QString metric = (ui.metricSelect)->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();

    qDebug()<< "DialogDissimilarities: user selected: "
            << varLocation
            << metric;
    emit userChoices( metric, varLocation, diagonal  );
}


void DialogDissimilarities::on_buttonBox_accepted()
{
    this->gatherData();
    this->accept();
}

void DialogDissimilarities::on_buttonBox_rejected()
{
    this->reject();
}

DialogDissimilarities::~DialogDissimilarities(){
     metricList.clear();
     variablesLocationList.clear();
}
