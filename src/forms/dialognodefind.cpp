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

#include <QGraphicsColorizeEffect>


DialogNodeFind::DialogNodeFind(QWidget *parent, QStringList indexList) :
    QDialog(parent),
    ui(new Ui::DialogNodeFind)
{
    ui->setupUi(this);

    ui->labelsRadioBtn->setAutoExclusive(true);
    ui->numbersRadioBtn->setAutoExclusive(true);
    ui->indexRadioBtn->setAutoExclusive(true);

    ui->numbersRadioBtn->setChecked(true);

    ui->indexCombo->insertItems(0, indexList);
    ui->indexLabel->setEnabled(false);
    ui->indexCombo->setEnabled(false);


    connect ( ui->labelsRadioBtn,SIGNAL(clicked(bool)), this, SLOT(checkErrors()) );
    connect ( ui->numbersRadioBtn,SIGNAL(clicked(bool)), this, SLOT(checkErrors()) );
    connect ( ui->indexRadioBtn,SIGNAL(clicked(bool)), this, SLOT(checkErrors()) );

    connect ( ui->plainTextEdit,SIGNAL(textChanged()), this, SLOT(checkErrors()) );

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);


}

DialogNodeFind::~DialogNodeFind()
{
    tempListA.clear();
    tempListB.clear();

    delete ui;
}


void DialogNodeFind::setError(const bool &toggle) {

    if (  toggle  ) {
         QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
         effect->setColor(QColor("red"));
         ui->plainTextEdit->setGraphicsEffect(effect);
         ui->buttonBox -> button (QDialogButtonBox::Ok) -> setEnabled(false);
     }
     else {
        ui->plainTextEdit->setGraphicsEffect(0);
        ui->buttonBox -> button (QDialogButtonBox::Ok) -> setEnabled(true);
     }




}

void DialogNodeFind::checkErrors()
{

    QString needle = ui->plainTextEdit->toPlainText();

    qDebug()<< "DialogNodeFind::checkErrors() - raw text entered:"
            << needle;

    if ( ui->numbersRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Enter node numbers to find (line by line or csv)");
        searchType = "numbers";
        ui->indexLabel->setEnabled(false);
        ui->indexCombo->setEnabled(false);
    }
    else if ( ui->labelsRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Enter node labels to find (line by line or csv)");
        searchType = "labels";
        ui->indexLabel->setEnabled(false);
        ui->indexCombo->setEnabled(false);
    }
    else if ( ui->indexRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Enter index score to search (i.e. > 0.5)");
        ui->indexLabel->setEnabled(true);
        ui->indexCombo->setEnabled(true);
        searchType = "index";
    }

    qDebug()<< "DialogNodeFind::checkErrors() - search type:" << searchType;

    list.clear();
    tempListA.clear();
    tempListB.clear();

    if (  needle.isEmpty()  ) {
        setError(true);
    }
     else {
        setError(false);
     }


    if (! ui->indexRadioBtn->isChecked()) {

        // user wants to search by numbers or labels
        // user has to enter a CSV or line separated list of values

        if (needle.contains("\n") && needle.contains(",")) {
            // error you cannot enter both?
           // return;
        }

        // check if user entered multiple lines
        tempListA = needle.split("\n",QString::SkipEmptyParts);

        for (int i = 0; i < tempListA.size(); ++i) {

            // take every linefeed separated value
            str = tempListA.at(i).toLocal8Bit().constData();

            qDebug()<< "DialogNodeFind::checkErrors() - line:" << i << "str:" << str;

            // check if user has entered comma
            tempListB = str.split(",",QString::SkipEmptyParts);

            for (int j = 0; j < tempListB.size(); ++j) {

                // take every comma separated value
                str = tempListB.at(j).toLocal8Bit().constData();

                qDebug()<< "DialogNodeFind::checkErrors() - line:" << i
                        << "element at pos:" << j << "is:" << str;

                if (ui->numbersRadioBtn->isChecked()) {
                    // user wants to search by numbers
                    // check if str contains a dash
                    if (str.contains ("-")) {
                        //str.split("-")
                    }
                    else {

                        if (str.contains(QRegExp("\\D+"))) {
                            qDebug()<< "DialogNodeFind::checkErrors() - error! not number" << str;
                            setError(true);
                        }
                        else {
                            qDebug()<< "DialogNodeFind::checkErrors() - adding number" << str;
                            list << str;
                        }
                    }

                }
                else {
                    // user wants to search by labels
                    qDebug()<< "DialogNodeFind::checkErrors() - adding label" << str;
                    list << str;
                }

            }
        }

    }
    else  {
        // user wants to search nodes by their index score
        // user has to enter > or < and a threshold
        // and to select the desired index.



    }

}

void DialogNodeFind::gatherData()
{
   qDebug()<< "DialogNodeFind::gatherData()" << list;
      qDebug()<< "DialogNodeFind::gatherData() type" << searchType;
   emit userChoices( list, searchType  );

}
