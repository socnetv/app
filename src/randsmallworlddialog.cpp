/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                         randsmallworlddialog.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net

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

#include "randsmallworlddialog.h"

RandSmallWorldDialog::RandSmallWorldDialog(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "::RandSmallWorldDialog() " ;

    ui.setupUi(this);

    nodes = 0;
    degree = 0;
    bprob = 0;
    mode = "";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &RandSmallWorldDialog::gatherData );

    ui.buttonBox -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui.nodesSpinBox )->setFocus();

    ui.probDoubleSpinBox->setEnabled(true);
    ui.degreeSpinBox-> setEnabled(true);
    ui.undirectedRadioButton->setChecked(true);
    ui.directedRadioButton->setEnabled(false);
    ui.diagCheckBox ->setChecked(false);
    ui.diagCheckBox -> setEnabled(false);

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &RandSmallWorldDialog::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &RandSmallWorldDialog::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &RandSmallWorldDialog::setDiag);
}


void RandSmallWorldDialog::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;

}

void RandSmallWorldDialog::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
}

void RandSmallWorldDialog::setDiag (){
    if (ui.diagCheckBox -> isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void RandSmallWorldDialog::checkErrors() {
    qDebug()<< " RandSmallWorldDialog::checkErrors()" ;

    //     if ( !ui.gnpRadioButton->isChecked() &&  !ui.gnmRadioButton->isChecked())
    //     {
    //         QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    //         effect->setColor(QColor("red"));
    //         ui.gnpRadioButton->setGraphicsEffect(effect);
    //         (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
    //     }
    //     else {
    //         ui.gnpRadioButton->setGraphicsEffect(0);
    //         ui.gnmRadioButton->setGraphicsEffect(0);
    //         (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(true);
    //     }
    //gatherData();
}

void RandSmallWorldDialog::gatherData() {
    qDebug() << "RandSmallWorldDialog::gatherData() " ;
    nodes = ui.nodesSpinBox->value();
    bprob = ui.probDoubleSpinBox->value();
    degree= ui.degreeSpinBox->value();
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
    diag = (ui.diagCheckBox -> isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "bprob " << bprob;
    qDebug() << "degree" << degree;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, degree, bprob, mode, diag);

}

