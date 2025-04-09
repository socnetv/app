/**
 * @file dialogranderdosrenyi.h
 * @brief Declares the DialogRandErdosRenyi class for generating Erdős–Rényi random graphs in SocNetV.
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


#include <QDebug>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QGraphicsColorizeEffect>

#include "dialogranderdosrenyi.h"

DialogRandErdosRenyi::DialogRandErdosRenyi(QWidget *parent, const qreal eprob) :

    QDialog(parent)
{
    qDebug() << "::DialogRandErdosRenyi() " ;

    ui.setupUi(this);

    nodes = 0;
    model = "";
    edges = 0;
    ui.probDoubleSpinBox->setValue(eprob);
    mode = "";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &DialogRandErdosRenyi::getUserChoices );

    ui.buttonBox->button (QDialogButtonBox::Ok)->setDefault(true);

    (ui.nodesSpinBox )->setFocus();

    connect (ui.gnpRadioButton, &QRadioButton::clicked,
             this, &DialogRandErdosRenyi::checkErrors);

    connect (ui.gnmRadioButton, &QRadioButton::clicked,
             this, &DialogRandErdosRenyi::checkErrors);

    ui.gnpRadioButton->setChecked(true);
    ui.probDoubleSpinBox->setEnabled(true);
    ui.edgesSpinBox->setDisabled(true);
    ui.undirectedRadioButton->setChecked(true);
    ui.diagCheckBox->setChecked(false);


    connect (ui.gnpRadioButton, &QRadioButton::clicked,
             this, &DialogRandErdosRenyi::gnpModel );

    connect (ui.gnmRadioButton, &QRadioButton::clicked,
             this, &DialogRandErdosRenyi::gnmModel );

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &DialogRandErdosRenyi::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &DialogRandErdosRenyi::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &DialogRandErdosRenyi::setDiag);
}

void DialogRandErdosRenyi::gnpModel (){
    ui.gnmRadioButton->setChecked(false);
    ui.probDoubleSpinBox->setEnabled(true);
    ui.edgesSpinBox->setDisabled(true);

}

void DialogRandErdosRenyi::gnmModel (){
    ui.gnpRadioButton->setChecked(false);
    ui.probDoubleSpinBox->setDisabled(true);
    ui.edgesSpinBox->setEnabled(true);
}

void DialogRandErdosRenyi::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;

}

void DialogRandErdosRenyi::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
}

void DialogRandErdosRenyi::setDiag (){
    if (ui.diagCheckBox->isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void DialogRandErdosRenyi::checkErrors() {
    qDebug()<< " DialogRandErdosRenyi::checkErrors()" ;

    if ( !ui.gnpRadioButton->isChecked() &&  !ui.gnmRadioButton->isChecked())
    {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        QGraphicsColorizeEffect *effect2 = new QGraphicsColorizeEffect;
        effect2->setColor(QColor("red"));
        ui.gnpRadioButton->setGraphicsEffect(effect);
        ui.gnmRadioButton->setGraphicsEffect(effect2);
        (ui.buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    else {
        ui.gnpRadioButton->setGraphicsEffect(0);
        ui.gnmRadioButton->setGraphicsEffect(0);
        (ui.buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void DialogRandErdosRenyi::getUserChoices() {
    qDebug() << "DialogRandErdosRenyi::getUserChoices() " ;
    nodes = ui.nodesSpinBox->value();
    model = ( ui.gnpRadioButton->isChecked() ) ? "G(n,p)" : "G(n,M)";
    if (  ui.gnpRadioButton->isChecked() ) {
//        eprob = ui.probDoubleSpinBox->value();
    }
    else {
        edges = ui.edgesSpinBox->value();
    }
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
    diag = (ui.diagCheckBox->isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "model " << model;
    qDebug() << "eprob " << ui.probDoubleSpinBox->value();
    qDebug() << "edges " << edges;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, model, edges, ui.probDoubleSpinBox->value(), mode, diag);

}
