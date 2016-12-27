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


DialogClusteringHierarchical::DialogClusteringHierarchical (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    matrixList
            << "Adjacency"
            << "Distances"
            << "Adjacency Similarity"
            << "Distances Similarity";

    similarityMeasuresList << "Simple / Exact matching"
                          <<"Jaccard index"
                         <<"Hamming distance"
                        <<"Cosine similarity"
                          <<"Pearson coefficients";


    linkageCriteriaList
              << "Single-linkage (minimum)"
              << "Complete-linkage (maximum)"
              << "Average-linkage (UPGMA)";


    (ui.matrixSelect) -> insertItems( 1, matrixList );

    ui.similarityMeasureSelect ->insertItems(1, similarityMeasuresList);
    ui.similarityMeasureSelect ->setEnabled(false);

    (ui.linkageCriterionSelect) -> insertItems( 1, linkageCriteriaList );

    (ui.diagonalCheckBox)->setChecked(false);

    connect ( ui.matrixSelect, SIGNAL(highlighted(QString)), this, SLOT(matrixChanged(QString)) );


}


void DialogClusteringHierarchical::matrixChanged(const QString &matrix) {
    qDebug()<< "DialogClusteringHierarchical::matrixChanged()"
            << matrix;

    if (matrix.contains("similarity", Qt::CaseInsensitive)) {
        ui.similarityMeasureSelect ->setEnabled(true);
    }
    else {
        ui.similarityMeasureSelect ->setEnabled(false);
    }

}

void DialogClusteringHierarchical::gatherData(){
    qDebug()<< "DialogClusteringHierarchical: gathering Data!...";
    QString matrix = (ui.matrixSelect) ->currentText();
    QString similarityMeasure= (ui.similarityMeasureSelect) ->currentText();
    QString linkageCriterion = (ui.linkageCriterionSelect) ->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();
    qDebug()<< "DialogClusteringHierarchical: user selected: "
            << matrix
            << similarityMeasure
            << linkageCriterion;
    emit userChoices( matrix, similarityMeasure, linkageCriterion,diagonal );
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
     similarityMeasuresList.clear();
     linkageCriteriaList.clear();
}
