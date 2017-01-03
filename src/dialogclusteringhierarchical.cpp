/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt
 
                         dialogclusteringhierarchical.cpp  -  description
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

 

#include "dialogclusteringhierarchical.h"

#include <QDebug>
#include <QPushButton>


DialogClusteringHierarchical::DialogClusteringHierarchical (QWidget *parent) :
    QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    matrixList << "Adjacency"
               << "Distances";

    measureList << "None, use raw input matrix"
                << "Jaccard distance"
                << "Hamming distance"
                << "Euclidean distance"
                << "Manhattan distance";


    linkageList << "Single-linkage (minimum)"
                << "Complete-linkage (maximum)"
                << "Average-linkage (UPGMA)";


    (ui.matrixSelect) -> insertItems( 1, matrixList );

    ui.metricSelect ->insertItems(1, measureList);

    (ui.linkageSelect) -> insertItems( 1, linkageList );

    (ui.linkageSelect)-> setCurrentIndex(2);

    (ui.diagonalCheckBox)->setChecked(false);

    connect ( ui.matrixSelect, SIGNAL(highlighted(QString)),
              this, SLOT(matrixChanged(QString)) );


}


void DialogClusteringHierarchical::matrixChanged(const QString &matrix) {
    qDebug()<< "DialogClusteringHierarchical::matrixChanged()"
            << matrix;
}

void DialogClusteringHierarchical::gatherData(){
    qDebug()<< "DialogClusteringHierarchical: gathering Data!...";
    QString matrix = (ui.matrixSelect) ->currentText();

    QString metric= (( ui.metricSelect ->isEnabled() ) ?
                                    ui.metricSelect ->currentText()  :
                                    "-" );

    QString linkage = (ui.linkageSelect) ->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();
    qDebug()<< "DialogClusteringHierarchical: user selected: "
            << matrix
            << metric
            << linkage;
    emit userChoices( matrix, metric, linkage,diagonal );
}


void DialogClusteringHierarchical::on_buttonBox_accepted()
{
    this->gatherData();
    this->accept();
}

void DialogClusteringHierarchical::on_buttonBox_rejected()
{
    this->reject();
}

DialogClusteringHierarchical::~DialogClusteringHierarchical(){
     matrixList.clear();
     measureList.clear();
     linkageList.clear();
}
