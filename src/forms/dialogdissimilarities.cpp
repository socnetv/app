/**
 * @file dialogdissimilarities.cpp
 * @brief Implements the DialogDissimilarities class for managing dissimilarity calculations in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */
 

#include "dialogdissimilarities.h"

#include <QDebug>
#include <QPushButton>


DialogDissimilarities::DialogDissimilarities (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);

    variablesLocationList
            << "Rows"
            << "Columns"
            << "Both";

    metricList  << tr("Euclidean distance")
                << tr("Manhattan distance")
                << tr("Hamming distance")
                << tr("Jaccard distance")
                << tr("Chebyshev distance");



    (ui.variablesLocationSelect)->insertItems( 1, variablesLocationList );
    (ui.metricSelect)->insertItems( 1, metricList );

    (ui.diagonalCheckBox)->setChecked(false);

}



void DialogDissimilarities::getUserChoices(){
    qDebug()<< "DialogDissimilarities: gathering Data!...";
    QString varLocation = (ui.variablesLocationSelect)->currentText();
    QString metric = (ui.metricSelect)->currentText();
    bool diagonal = (ui.diagonalCheckBox)->isChecked();

    qDebug()<< "DialogDissimilarities: user selected: "
            << varLocation
            << metric;
    emit userChoices( metric, varLocation, diagonal  );
}


void DialogDissimilarities::on_buttonBox_accepted()
{
    this->getUserChoices();
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
