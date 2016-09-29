/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                         randregulardialog.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
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
#include <QtMath>

#include "randregulardialog.h"

RandRegularDialog::RandRegularDialog(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "::RandRegularDialog() " ;

    ui.setupUi(this);

    nodes = 100;
    degree = 2;
    mode = "undirected";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &RandRegularDialog::gatherData );

    ui.buttonBox -> button (QDialogButtonBox::Ok) -> setDefault(true);

    ui.degreeSpinBox-> setEnabled(true);
    ui.undirectedRadioButton->setChecked(true);
    ui.diagCheckBox ->setChecked(false);
    ui.diagCheckBox -> setEnabled(false);

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &RandRegularDialog::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &RandRegularDialog::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &RandRegularDialog::setDiag);

    ui.nodesSpinBox->setFocus();
    ui.nodesSpinBox->setValue(nodes);
    ui.degreeSpinBox->setValue( degree );

    connect(ui.nodesSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(checkErrors(int)));

    connect(ui.degreeSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(checkErrors(int)));


}


void RandRegularDialog::modifyDegree(int value) {
    ui.degreeSpinBox->setValue( qCeil ( qLn (value) ));
    ui.degreeSpinBox->setMaximum( value );
}

void RandRegularDialog::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;
    ui.degreeLabel->setText("inDegree=outDegree <em>d</em>");

}

void RandRegularDialog::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
    ui.degreeLabel->setText("Degree <em>d</em>");
}

void RandRegularDialog::setDiag (){
    if (ui.diagCheckBox -> isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void RandRegularDialog::checkErrors(const int &i) {
    Q_UNUSED(i);
    qDebug()<< " RandRegularDialog::checkErrors()" ;
        if ( ( ui.degreeSpinBox->value() * ui.nodesSpinBox->value() )  % 2 !=0 ) {
             QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
             effect->setColor(QColor("red"));
             ui.degreeSpinBox->setGraphicsEffect(effect);
             ui.nodesSpinBox->setGraphicsEffect(effect);
             (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
         }
         else {
             ui.degreeSpinBox->setGraphicsEffect(0);
             ui.nodesSpinBox->setGraphicsEffect(0);
             (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(true);
         }
}

void RandRegularDialog::gatherData() {
    qDebug() << "RandRegularDialog::gatherData() " ;
    nodes = ui.nodesSpinBox->value();
    degree= ui.degreeSpinBox->value();
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
    diag = (ui.diagCheckBox -> isChecked() ? true : false);
    qDebug() << "nodes " << nodes ;
    qDebug() << "degree" << degree;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, degree, mode, diag);

}

