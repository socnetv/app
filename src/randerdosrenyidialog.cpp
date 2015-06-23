/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                         randerdosrenyidialog.h  -  description
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

#include "randerdosrenyidialog.h"

RandErdosRenyiDialog::RandErdosRenyiDialog(QWidget *parent ) :

    QDialog(parent)
{
    qDebug() << "::RandErdosRenyiDialog() " ;

    ui.setupUi(this);

    nodes = 0;
    model = "";
    edges = 0;
    eprob = 0;
    mode = "";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &RandErdosRenyiDialog::gatherData );

    ui.buttonBox -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui.nodesSpinBox )->setFocus();

    connect (ui.gnpRadioButton, &QRadioButton::clicked,
             this, &RandErdosRenyiDialog::checkErrors);

    connect (ui.gnmRadioButton, &QRadioButton::clicked,
             this, &RandErdosRenyiDialog::checkErrors);

    //ui.gnpRadioButton->setChecked(true);
    ui.probDoubleSpinBox->setEnabled(true);
    ui.edgesSpinBox-> setDisabled(true);
    ui.undirectedRadioButton->setChecked(true);
    ui.diagCheckBox ->setChecked(true);


    connect (ui.gnpRadioButton, &QRadioButton::clicked,
             this, &RandErdosRenyiDialog::gnpModel );

    connect (ui.gnmRadioButton, &QRadioButton::clicked,
             this, &RandErdosRenyiDialog::gnmModel );

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &RandErdosRenyiDialog::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &RandErdosRenyiDialog::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &RandErdosRenyiDialog::setDiag);
}

void RandErdosRenyiDialog::gnpModel (){
    ui.gnmRadioButton -> setChecked(false);
    ui.probDoubleSpinBox -> setEnabled(true);
    ui.edgesSpinBox-> setDisabled(true);

}

void RandErdosRenyiDialog::gnmModel (){
    ui.gnpRadioButton -> setChecked(false);
    ui.probDoubleSpinBox -> setDisabled(true);
    ui.edgesSpinBox-> setEnabled(true);
}

void RandErdosRenyiDialog::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;

}

void RandErdosRenyiDialog::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
}

void RandErdosRenyiDialog::setDiag (){
    if (ui.diagCheckBox -> isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void RandErdosRenyiDialog::checkErrors() {
    qDebug()<< " RandErdosRenyiDialog::checkErrors()" ;

    if ( !ui.gnpRadioButton->isChecked() &&  !ui.gnmRadioButton->isChecked())
    {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        QGraphicsColorizeEffect *effect2 = new QGraphicsColorizeEffect;
        effect2->setColor(QColor("red"));
        ui.gnpRadioButton->setGraphicsEffect(effect);
        ui.gnmRadioButton->setGraphicsEffect(effect2);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
    }
    else {
        ui.gnpRadioButton->setGraphicsEffect(0);
        ui.gnmRadioButton->setGraphicsEffect(0);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(true);
    }
    //gatherData();
}

void RandErdosRenyiDialog::gatherData() {
    qDebug() << "RandErdosRenyiDialog::gatherData() " ;
    nodes = ui.nodesSpinBox->value();
    model = ( ui.gnpRadioButton->isChecked() ) ? "G(n,p)" : "G(n,M)";
    if (  ui.gnpRadioButton->isChecked() ) {
        eprob = ui.probDoubleSpinBox->value();
    }
    else {
        edges = ui.edgesSpinBox->value();
    }
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
    diag = (ui.diagCheckBox -> isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "model " << model;
    qDebug() << "eprob " << eprob;
    qDebug() << "edges " << edges;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, model, edges, eprob, mode, diag);

}
