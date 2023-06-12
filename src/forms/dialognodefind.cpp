/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1-dev
 Written in Qt

                         dialognodefind.cpp  -  description
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

    connect ( ui->labelsRadioBtn, SIGNAL(clicked(bool)), this, SLOT( checkErrors() ) );
    connect ( ui->numbersRadioBtn, SIGNAL(clicked(bool)), this, SLOT( checkErrors() ) );
    connect ( ui->indexRadioBtn, SIGNAL(clicked(bool)), this, SLOT( checkErrors() ) );

    connect (ui->indexCombo, &QComboBox::currentTextChanged,
             this, &DialogNodeFind::getIndex);

//    connect ( ui->indexCombo, SIGNAL(currentTextChanged(QString)), this, SLOT( checkErrors(QString) ) );

    connect ( ui->plainTextEdit,SIGNAL(textChanged()), this, SLOT(checkErrors()) );

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    ui->plainTextEdit->setFocus();

    (ui->buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);

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
         ui->buttonBox->button (QDialogButtonBox::Ok)->setEnabled(false);
     }
     else {
        ui->plainTextEdit->setGraphicsEffect(0);
        ui->buttonBox->button (QDialogButtonBox::Ok)->setEnabled(true);
     }
}

/**
 * @brief Gets the selected index
 * @param indexStr
 */
void DialogNodeFind::getIndex(const QString &indexStr) {

    selectedIndex = ui->indexCombo->currentText();

    qDebug() << "DialogNodeFind::getIndex() str"<<indexStr << "index" << selectedIndex;

}

/**
 * @brief Checks for various input errors
 */
void DialogNodeFind::checkErrors()
{

    QString textEntered = ui->plainTextEdit->toPlainText();

    qDebug()<< "DialogNodeFind::checkErrors() - raw text entered:"
            << textEntered;

    if ( ui->numbersRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Search for these numbers (enter line by line or csv):");
        searchType = "numbers";
        ui->indexLabel->setEnabled(false);
        ui->indexCombo->setEnabled(false);
    }
    else if ( ui->labelsRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Search for these labels (enter line by line or csv):");
        searchType = "labels";
        ui->indexLabel->setEnabled(false);
        ui->indexCombo->setEnabled(false);
    }
    else if ( ui->indexRadioBtn->isChecked() ) {
        ui->textEditLabel->setText("Search for nodes with this index score (i.e. > 0.5)");
        ui->indexLabel->setEnabled(true);
        ui->indexCombo->setEnabled(true);
        searchType = "score";
    }

    qDebug()<< "DialogNodeFind::checkErrors() - search type:" << searchType;

    list.clear();
    tempListA.clear();
    tempListB.clear();

    if (  textEntered.isEmpty()  ) {
        setError(true);
    }
     else {
        setError(false);
     }


    if (! ui->indexRadioBtn->isChecked()) {

        // user wants to search by numbers or labels
        // user has to enter a CSV or line separated list of values

        if (textEntered.contains("\n") && textEntered.contains(",")) {
            // error you cannot enter both?
           // return;
        }

        // check if user entered multiple lines
        tempListA = textEntered.split("\n", Qt::SkipEmptyParts);

        for (int i = 0; i < tempListA.size(); ++i) {

            // take every linefeed separated value
            str = tempListA.at(i).toLocal8Bit().constData();

            qDebug()<< "DialogNodeFind::checkErrors() - line:" << i << "str:" << str;

            // check if user has entered comma
            tempListB = str.split(",", Qt::SkipEmptyParts);

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

                        if (str.contains(QRegularExpression("\\D+"))) {
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
        // and select the desired index.
        selectedIndex = ui->indexCombo->currentText();
        // check if user entered multiple lines
        tempListA = textEntered.split("\n", Qt::SkipEmptyParts);

        for (int i = 0; i < tempListA.size(); ++i) {

            // take every linefeed separated value
            str = tempListA.at(i).toLocal8Bit().constData();
            if (str.contains (">") || str.contains ("<") || str.contains ("=")) {
                list << str;
            }
            else {
                qDebug()<< "DialogNodeFind::checkErrors() - error! search by index without > or <" ;
                setError(true);
            }
        }


    }

}


/**
 * @brief Gathers user input and emits userChoices signal
 */
void DialogNodeFind::getUserChoices()
{
   qDebug()<< "DialogNodeFind::getUserChoices()" << list;
      qDebug()<< "DialogNodeFind::getUserChoices() type" << searchType;
   emit userChoices( list, searchType, selectedIndex );

}
