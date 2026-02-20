/**
 * @file dialogsimilaritymatches.cpp
 * @brief Implements the DialogSimilarityMatches class for calculating similarity between nodes using matching coefficients in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

 
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
