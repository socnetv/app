/**
 * @file dialogsimilaritypearson.cpp
 * @brief Implements the DialogSimilarityPearson class for computing Pearson correlation similarity between nodes in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */
 

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
