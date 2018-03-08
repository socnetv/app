/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialognodefind.cpp  -  description
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

#include "dialognodefind.h"
#include "ui_dialognodefind.h"

#include <QDebug>
#include <QPushButton>

DialogNodeFind::DialogNodeFind(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNodeFind)
{
    ui->setupUi(this);

    ui->labelsRadioBtn->setAutoExclusive(true);
    ui->numbersRadioBtn->setAutoExclusive(true);

    ui->numbersRadioBtn->setChecked(true);

    connect ( ui->plainTextEdit,SIGNAL(textChanged()), this, SLOT(checkErrors()) );
    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);


}

DialogNodeFind::~DialogNodeFind()
{
    delete ui;
}

void DialogNodeFind::checkErrors()
{
    QString needle = ui->plainTextEdit->toPlainText();
    qDebug()<< "DialogNodeFind::checkErrors() text entered:"
            << ui->plainTextEdit->toPlainText();

    list.clear();

    if (ui->numbersRadioBtn->isChecked()) {
        if (needle.contains("\n") && needle.contains(",")) {
            // error you cannot enter both?
            return;
        }
        if (needle.contains("\n")) { // user entered multiple lines
           list = needle.split("\n");
        }
        else if (needle.contains(",")) { // user entered multiple lines
           list = needle.split(",");
        }
        else if (needle.contains("-")  ) {

        }
        else { // user entered only one number
            list << needle;
        }


    }
    else {
        if (needle.contains("\n")) { // user entered multiple lines
           list = needle.split("\n");
        }
        else if (needle.contains(",")) { // user entered multiple lines
           list = needle.split(",");
        }
        else { // user entered only one label
            list << needle;
        }
    }
}

void DialogNodeFind::gatherData()
{
   qDebug()<< "DialogNodeFind::gatherData()" << list;
   QString searchType = ui->labelsRadioBtn->isChecked() ? "labels" : "numbers";
   qDebug()<< "DialogNodeFind::gatherData() type" << searchType;
   emit userChoices( list, searchType  );

}
