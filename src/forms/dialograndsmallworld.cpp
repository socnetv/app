/**
 * @file dialograndsmallworld.cpp
 * @brief Implements the DialogRandSmallWorld class for generating small-world networks using the Watts-Strogatz model in SocNetV.
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
#include <QtMath>

#include "dialograndsmallworld.h"

DialogRandSmallWorld::DialogRandSmallWorld(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRandSmallWorld)
{
    qDebug() << "DialogRandSmallWorld::DialogRandSmallWorld() " ;

    ui->setupUi(this);

    nodes = 100;
    degree = qCeil ( qLn (nodes) );
    bprob = 0;
    mode = "undirected";
    diag = false;

    connect ( ui->buttonBox, &QDialogButtonBox::accepted,
              this, &DialogRandSmallWorld::getUserChoices );

    ui->buttonBox->button (QDialogButtonBox::Ok)->setDefault(true);

    ui->probDoubleSpinBox->setEnabled(true);
    ui->degreeSpinBox->setEnabled(true);
    ui->undirectedRadioButton->setChecked(true);
    ui->directedRadioButton->setEnabled(false);
    ui->diagCheckBox->setChecked(false);
    ui->diagCheckBox->setEnabled(false);

    connect ( ui->undirectedRadioButton,&QRadioButton::clicked,
              this, &DialogRandSmallWorld::setModeUndirected );
    connect ( ui->directedRadioButton,&QRadioButton::clicked,
              this, &DialogRandSmallWorld::setModeDirected );

    connect ( ui->diagCheckBox,&QCheckBox::clicked,
              this, &DialogRandSmallWorld::setDiag);

    connect(ui->nodesSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(modifyDegree(int)));

    ui->nodesSpinBox->setFocus();
    ui->nodesSpinBox->setValue(nodes);
    ui->degreeSpinBox->setValue( degree );

}


void DialogRandSmallWorld::modifyDegree(int value) {
    ui->degreeSpinBox->setValue( qCeil ( qLn (value) ));
    ui->degreeSpinBox->setMaximum( value );
}

void DialogRandSmallWorld::setModeDirected (){
    ui->directedRadioButton->setChecked(true) ;
    ui->undirectedRadioButton->setChecked(false) ;

}

void DialogRandSmallWorld::setModeUndirected (){
    ui->directedRadioButton->setChecked(false) ;
    ui->undirectedRadioButton->setChecked(true) ;
}

void DialogRandSmallWorld::setDiag (){
    if (ui->diagCheckBox->isChecked())
        ui->diagCheckBox->setText("Yes, allow");
    else
        ui->diagCheckBox->setText("No, set zero");
}

void DialogRandSmallWorld::checkErrors() {
    qDebug()<< " DialogRandSmallWorld::checkErrors()" ;

    //     if ( !ui->gnpRadioButton->isChecked() &&  !ui->gnmRadioButton->isChecked())
    //     {
    //         QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    //         effect->setColor(QColor("red"));
    //         ui->gnpRadioButton->setGraphicsEffect(effect);
    //         (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
    //     }
    //     else {
    //         ui->gnpRadioButton->setGraphicsEffect(0);
    //         ui->gnmRadioButton->setGraphicsEffect(0);
    //         (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
    //     }
    //getUserChoices();
}

void DialogRandSmallWorld::getUserChoices() {
    qDebug() << "DialogRandSmallWorld::getUserChoices() " ;
    nodes = ui->nodesSpinBox->value();
    bprob = ui->probDoubleSpinBox->value();
    degree= ui->degreeSpinBox->value();
    mode = (ui->directedRadioButton->isChecked() ? "digraph" : "graph" );
    diag = (ui->diagCheckBox->isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "bprob " << bprob;
    qDebug() << "degree" << degree;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, degree, bprob, mode, diag);

}

