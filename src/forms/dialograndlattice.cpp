/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0
 Written in Qt

                         dialograndlattice.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : https://socnetv.org

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

    ui.circularCheckBox -> setText("false");
    ui.nodesSpinBox->setEnabled(false);

    connect ( ui.circularCheckBox, &QCheckBox::toggled ,
              this, &DialogRandLattice::circularChanged);

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &DialogRandLattice::getUserChoices );


    connect(ui.lengthSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(lengthChanged(int)));


    ui.buttonBox -> button (QDialogButtonBox::Ok) -> setDefault(true);

}

void DialogRandLattice::circularChanged(const bool &toggle) {
    if (toggle) {
        ui.circularCheckBox -> setText("true");
    }
    else {
        ui.circularCheckBox -> setText("false");
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
    circular = (ui.circularCheckBox -> isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "length " << length;
    qDebug() << "dimension " << dimension;
    qDebug() << "neighLength" << neighLength;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << circular;

    emit userChoices(nodes, length, dimension, neighLength, mode, circular);

}

