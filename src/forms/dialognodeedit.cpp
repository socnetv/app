/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialognodeedit.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>
#include <QRadioButton>
#include <QPushButton>
#include <QColorDialog>
#include <QPixmap>
#include <QGraphicsEffect>

#include "dialognodeedit.h"

DialogNodeEdit::DialogNodeEdit(QWidget *parent,
                               const QString &l,
                               const int &s,
                               const QColor &col,
                               const QString &sh) :
    QDialog(parent),
    ui(new Ui::DialogNodeEdit)
{
    ui->setupUi(this);
    nodeSize = s;
    nodeColor = col;
    nodeShape = sh;
    nodeLabel = l;

    ui->labelEdit->setText(nodeLabel);
    ui->sizeSpin->setValue(nodeSize);

    if ( nodeShape == "box"  ){
       ui->boxRadio->setChecked (true);
    }
    else if ( nodeShape == "circle"  ){
        ui->circleRadio->setChecked (true);
     }
    else if ( nodeShape == "diamond"  ){
        ui->diamondRadio->setChecked (true);
     }
    else if ( nodeShape == "ellipse"  ){
        ui->ellipseRadio->setChecked (true);
     }
    else if ( nodeShape == "triangle"  ){
        ui->triangleRadio->setChecked (true);
     }

    pixmap = QPixmap(60,20) ;
    pixmap.fill(nodeColor);
    ui->colorButton->setIcon(QIcon(pixmap));

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui->labelEdit)->setFocus();

    connect (ui->labelEdit, &QLineEdit::editingFinished,
             this, &DialogNodeEdit::checkErrors);

    connect (ui->colorButton, &QToolButton::clicked,
             this, &DialogNodeEdit::selectColor);

}



void DialogNodeEdit::gatherData(){
    qDebug()<< " DialogNodeEdit::gatherData()" ;
    nodeLabel = ui->labelEdit->text();
    nodeSize = ui->sizeSpin->value();
    nodeValue = ui->valueEdit->text();
    nodeShape = "circle";
    if ( ui->boxRadio->isChecked () ){
       nodeShape  = "box";
    }
    else if ( ui->circleRadio->isChecked() ){
       nodeShape  = "circle";
    }
    else if ( ui->diamondRadio->isChecked() ){
       nodeShape  = "diamond";
    }
    else if ( ui->ellipseRadio->isChecked() ){
        nodeShape  = "ellipse";
    }
    else if ( ui->triangleRadio->isChecked() ){
        nodeShape  = "triangle";
    }
    else if ( ui->starRadio->isChecked() ){
        nodeShape  = "star";
    }
    else {
        nodeShape  = "box";
    }

    emit userChoices(nodeLabel,nodeSize,nodeValue,nodeColor,nodeShape);
}

void DialogNodeEdit::checkErrors() {
    qDebug()<< " DialogNodeEdit::checkErrors()" ;
    QString userLabel = ui->labelEdit->text();
    userLabel = userLabel.simplified();
    ui->labelEdit->setText(userLabel);

    if ( ui->labelEdit->text().isEmpty() ) {
        qDebug() << "empty label!";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->labelEdit->setGraphicsEffect(effect);
        //(ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
    }
    else {
        ui->labelEdit->setGraphicsEffect(0);
        (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(true);
    }
    //gatherData();
}

void DialogNodeEdit::selectColor() {
    qDebug()<< " DialogNodeEdit::selectColor()" ;
    nodeColor = QColorDialog::getColor(
                Qt::red, this, tr("Select node color") );
    if ( nodeColor.isValid()) {
        qDebug() << " color selected " << nodeColor.name();
        pixmap.fill(nodeColor);
        ui->colorButton->setIcon(QIcon(pixmap));
    }
    else {
        // user pressed Cancel
        qDebug() << " Aborted node color";
    }

}


DialogNodeEdit::~DialogNodeEdit()
{
    delete ui;
}

