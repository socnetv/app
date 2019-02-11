/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialognodeedit.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
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

#include "dialognodeedit.h"
#include "ui_dialognodeedit.h"

#include <QGlobalStatic>
#include <QDebug>
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>
#include <QRadioButton>
#include <QPushButton>
#include <QColorDialog>
#include <QPixmap>
#include <QGraphicsEffect>


DialogNodeEdit::DialogNodeEdit(QWidget *parent,
                               const QString &label,
                               const int &size,
                               const QColor &color,
                               const QString &shape,
                               const QString &path) :
    QDialog(parent),
    nodeLabel(label),
    nodeSize(size),
    nodeColor(color),
    nodeShape(shape),
    iconPath(path),
    ui(new Ui::DialogNodeEdit)
{
    ui->setupUi(this);

    ui->labelEdit->setText(nodeLabel);
    ui->sizeSpin->setValue(nodeSize);

    QStringList shapesList;
    QStringList iconList;
    shapesList << "Square"
                << "Circle"
                << "Diamond"
                << "Ellipse"
                << "Triangle"
                << "Star"
                << "Bug"
                << "Custom Icon";
    iconList << ":/images/box.png"
             << ":/images/circle.png"
             << ":/images/diamond.png"
             << ":/images/ellipse.png"
             << ":/images/triangle.png"
             << ":/images/star.png"
             << ":/images/bugs.png"
             << ":/images/export_photo_48px.svg";

    ui->nodeShapeComboBox->addItems(shapesList);

    for (int i = 0; i < shapesList.size(); ++i) {
       ui->nodeShapeComboBox->setItemIcon(i, QIcon(iconList[i]));
    }

    ui->nodeIconSelectButton->setEnabled(false);
    ui->nodeIconSelectEdit->setEnabled(false);

    if ( nodeShape == "box"  ){
       ui->nodeShapeComboBox->setCurrentIndex(0);
    }
    else if ( nodeShape == "circle"  ){
        ui->nodeShapeComboBox->setCurrentIndex(1);
     }
    else if ( nodeShape == "diamond"  ){
        ui->nodeShapeComboBox->setCurrentIndex(2);
     }
    else if ( nodeShape == "ellipse"  ){
        ui->nodeShapeComboBox->setCurrentIndex(3);
     }
    else if ( nodeShape == "triangle"  ){
        ui->nodeShapeComboBox->setCurrentIndex(4);
     }
    else if ( nodeShape == "star" ) {
        ui->nodeShapeComboBox->setCurrentIndex(5);
    }
    else if ( nodeShape == "bugs" ) {
        ui->nodeShapeComboBox->setCurrentIndex(6);
    }
    else if ( nodeShape == "custom" ) {
        ui->nodeShapeComboBox->setCurrentIndex(7);
        ui->nodeIconSelectButton->setEnabled(true);
        ui->nodeIconSelectEdit->setEnabled(true);
        ui->nodeIconSelectEdit->setText(iconPath);
        if (!iconPath.isEmpty()) {
            ui->nodeShapeComboBox->setItemIcon(7, QIcon(iconPath));
        }
    }


    pixmap = QPixmap(60,20) ;
    pixmap.fill(nodeColor);
    ui->colorButton->setIcon(QIcon(pixmap));

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui->labelEdit)->setFocus();

    connect (ui->labelEdit, &QLineEdit::editingFinished,
             this, &DialogNodeEdit::checkErrors);

    connect (ui->colorButton, &QToolButton::clicked,
             this, &DialogNodeEdit::selectColor);

    connect (ui->nodeShapeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
             this, &DialogNodeEdit::getNodeShape);
}



/**
 * @brief DialogNodeEdit::getNodeShape
 * @param shape
 */
void DialogNodeEdit::getNodeShape(const int &nodeShapeIndex){


    switch (nodeShapeIndex) {
    case 0:
        nodeShape  = "box";
        break;
    case 1:
        nodeShape  = "circle";
        break;
    case 2:
        nodeShape  = "diamond";
        break;
    case 3:
        nodeShape  = "ellipse";
        break;
    case 4:
        nodeShape  = "triangle";
        break;
    case 5:
        nodeShape  = "star";
        break;
    case 6:
        nodeShape  = "bugs";
        break;
    case 7:
        nodeShape  = "custom";
        break;
    default:
        break;
    }

        qDebug()<< "DialogNodeEdit::getNodeShape() - new node shape " << nodeShape;

    if ( nodeShapeIndex == 7) {
        // enable textedit and file button and raise file dialog

        ui->nodeIconSelectButton->setEnabled(true);
        ui->nodeIconSelectEdit->setEnabled(true);
        ui->nodeIconSelectEdit->setText(iconPath);
        if (!iconPath.isEmpty()) {
            ui->nodeShapeComboBox->setItemIcon(7, QIcon(iconPath));
        }
    }
    else {
        ui->nodeIconSelectButton->setEnabled(false);
        ui->nodeIconSelectEdit->setEnabled(false);
        ui->nodeIconSelectEdit->setText ("");
    }
}




void DialogNodeEdit::getUserChoices(){
    qDebug()<< " DialogNodeEdit::getUserChoices()" ;
    nodeLabel = ui->labelEdit->text();
    nodeSize = ui->sizeSpin->value();
    nodeValue = ui->valueEdit->text();
    nodeShape = "circle";
    int nodeShapeIndex = ui->nodeShapeComboBox->currentIndex();

    switch (nodeShapeIndex) {
    case 0:
        nodeShape  = "box";
        break;
    case 1:
        nodeShape  = "circle";
        break;
    case 2:
        nodeShape  = "diamond";
        break;
    case 3:
        nodeShape  = "ellipse";
        break;
    case 4:
        nodeShape  = "triangle";
        break;
    case 5:
        nodeShape  = "star";
        break;
    case 6:
        nodeShape  = "bugs";
        break;
    case 7:
        nodeShape  = "custom";
        break;
    default:
        break;
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
    //getUserChoices();
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

