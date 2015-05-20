/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.7
 Written in Qt

                         previewform.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net

    comment              : code borrowed from Qt5 codecs example

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

#include <QtWidgets>
#include "previewform.h"

PreviewForm::PreviewForm(QWidget *parent) :
    QDialog(parent)
{
    encodingComboBox = new QComboBox;

    encodingLabel = new QLabel(tr("&Encoding:"));
    encodingLabel->setBuddy(encodingComboBox);

    textEdit = new QTextEdit;
    textEdit->setToolTip(tr("In this area you can preview your file.\n")
                           + (" Select the correct encoding from the menu.\n")
                           + (" Mac and Linux users select UTF-8\n")
                           + (" Windows users select Windows-1253 or UTF-8\n")
                           + (" Windows users in Russia select KOI8-R\n"));

    textEdit->setLineWrapMode(QTextEdit::NoWrap);
    textEdit->setReadOnly(true);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                    | QDialogButtonBox::Cancel);

    connect(encodingComboBox, SIGNAL(activated(int)),
            this, SLOT(updateTextEdit()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(encodingLabel, 0, 0);
    mainLayout->addWidget(encodingComboBox, 0, 1);
    mainLayout->addWidget(textEdit, 1, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 2, 0, 1, 2);
    setLayout(mainLayout);

    setWindowTitle(tr("Preview file & Choose Encoding"));
    resize(600, 400);
}

void PreviewForm::setCodecList(const QList<QTextCodec *> &list)
{
    encodingComboBox->clear();
    foreach (QTextCodec *codec, list)
        encodingComboBox->addItem(codec->name(), codec->mibEnum());
}

void PreviewForm::setEncodedData(const QByteArray &data,
                                 const QString m_fileName,
                                 const int m_format)
{
    fileName = m_fileName;
    format = m_format;
    encodedData = data;
    updateTextEdit();
}

void PreviewForm::updateTextEdit()
{
    int mib = encodingComboBox->itemData(
                      encodingComboBox->currentIndex()).toInt();
    QTextCodec *codec = QTextCodec::codecForMib(mib);
    qDebug () << " PreviewForm::updateTextEdit() " << codec->name();
    QTextStream in(&encodedData);
    in.setAutoDetectUnicode(false);
    in.setCodec(codec);
    decodedStr = in.readAll();

    textEdit->setPlainText(decodedStr);
}

void PreviewForm::accept() {
    int mib = encodingComboBox->itemData(
                      encodingComboBox->currentIndex()).toInt();
    QTextCodec *codec = QTextCodec::codecForMib(mib);
    qDebug () << " PreviewForm::accept() returning codec name " << codec->name();
    emit userCodec(fileName, codec->name(), format);
    QDialog::accept();

}
