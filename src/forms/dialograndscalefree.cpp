/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                         randscalefreeddialog.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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

#include "dialograndscalefree.h"

#include <QDebug>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QGraphicsColorizeEffect>

DialogRandScaleFree::DialogRandScaleFree(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "DialogRandScaleFree::DialogRandScaleFree() " ;

    ui.setupUi(this);

    nodes = 0;
    initialNodes = 0;
    mode = "";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &DialogRandScaleFree::getUserChoices );

    ui.buttonBox->button (QDialogButtonBox::Ok)->setDefault(true);

    (ui.nodesSpinBox )->setFocus();

    ui.initialNodesSpinBox->setEnabled(true);
    ui.undirectedRadioButton->setChecked(false);
    ui.directedRadioButton->setEnabled(true);
    ui.directedRadioButton->setChecked(true);
    ui.diagCheckBox->setText("No, set zero");
    ui.diagCheckBox->setChecked(false);
    ui.diagCheckBox->setEnabled(false);

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &DialogRandScaleFree::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &DialogRandScaleFree::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &DialogRandScaleFree::setDiag);

}


void DialogRandScaleFree::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;

}

void DialogRandScaleFree::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
}

void DialogRandScaleFree::setDiag (){
    if (ui.diagCheckBox->isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void DialogRandScaleFree::checkErrors() {
    qDebug()<< " DialogRandSmallWorld::checkErrors()" ;

    //     if ( !ui.gnpRadioButton->isChecked() &&  !ui.gnmRadioButton->isChecked())
    //     {
    //         QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    //         effect->setColor(QColor("red"));
    //         ui.gnpRadioButton->setGraphicsEffect(effect);
    //         (ui.buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
    //     }
    //     else {
    //         ui.gnpRadioButton->setGraphicsEffect(0);
    //         ui.gnmRadioButton->setGraphicsEffect(0);
    //         (ui.buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
    //     }
    //getUserChoices();
}

void DialogRandScaleFree::getUserChoices() {
    qDebug() << "DialogRandScaleFree::getUserChoices() " ;
    nodes = ui.nodesSpinBox->value();
    power = ui.powerSpinBox->value();
    initialNodes = ui.initialNodesSpinBox->value();
    edgesPerStep = ui.edgesPerStepSpinBox->value();
    zeroAppeal = ui.zeroAppealSpinBox->value();
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
 //   diag = (ui.diagCheckBox->isChecked() ? true : false);

    qDebug() << "nodes " << nodes ;
    qDebug() << "initialNodes " << initialNodes;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, power, initialNodes, edgesPerStep,zeroAppeal, mode);

}


