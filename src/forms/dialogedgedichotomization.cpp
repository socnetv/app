/**
 * @file dialogedgedichotomization.cpp
 * @brief Implements the DialogEdgeDichotomization class for managing edge dichotomization in SocNetV.
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

#include "dialogedgedichotomization.h"
#include <QPushButton>
#include <QDebug>

DialogEdgeDichotomization::DialogEdgeDichotomization (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    (ui.buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);


} 



void DialogEdgeDichotomization::getUserChoices(){
    qDebug()<< "Dialog: gathering Data!...";
    qreal my_threshold = ui.weightThreshold->value() ;
    qDebug()<< "DialogEdgeDichotomization::getUserChoices() - We will dichotomize edges according to threshold: " << my_threshold;
    qDebug()<< "Dialog: emitting userChoices" ;
    emit userChoices( my_threshold );
}
