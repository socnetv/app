/**
 * @file dialograndlattice.cpp
 * @brief Implements the DialogRandLattice class for generating random lattice networks in SocNetV.
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


#include <QDebug>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QGraphicsColorizeEffect>

#include "dialograndlattice.h"


DialogRandLattice::DialogRandLattice(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    ui.circularCheckBox->setText("false");
    ui.nodesSpinBox->setEnabled(false);

    connect ( ui.circularCheckBox, &QCheckBox::toggled ,
              this, &DialogRandLattice::circularChanged);

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &DialogRandLattice::getUserChoices );


    connect(ui.lengthSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(lengthChanged(int)));


    ui.buttonBox->button (QDialogButtonBox::Ok)->setDefault(true);

}

void DialogRandLattice::circularChanged(const bool &toggle) {
    if (toggle) {
        ui.circularCheckBox->setText("true");
    }
    else {
        ui.circularCheckBox->setText("false");
    }

}

void DialogRandLattice::lengthChanged(int l) {
    ui.nodesSpinBox->setValue(l*l);
}



void DialogRandLattice::getUserChoices() {
    qDebug() << "DialogRandSmallWorld::getUserChoices() " ;
    nodes = ui.nodesSpinBox->value();
    length = ui.lengthSpinBox->value();
    dimension = ui.dimSpinBox->value();
    neighLength = ui.neiSpinBox->value();
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
    circular = (ui.circularCheckBox->isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "length " << length;
    qDebug() << "dimension " << dimension;
    qDebug() << "neighLength" << neighLength;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << circular;

    emit userChoices(nodes, length, dimension, neighLength, mode, circular);

}

