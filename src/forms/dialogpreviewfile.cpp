/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt

                         dialogpreviewfile.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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
#include <QTextCodec>
#include "dialogpreviewfile.h"

DialogPreviewFile::DialogPreviewFile(QWidget *parent) :
    QDialog(parent)
{
    encodingComboBox = new QComboBox;

    encodingLabel = new QLabel(tr("&Encoding:"));
    encodingLabel->setBuddy(encodingComboBox);

    textEdit = new QTextEdit;
    textEdit->setToolTip(
                tr("<p>In this area you can preview your text file before actually loading it.</p> "
                    "<p>SocNetV uses UTF-8 for saving and loading network files, by default. </p>"
                    "<p>If your file is encoded in another encoding, "
                    "select the correct encoding from the menu and "
                    "see if strings appear correctly.</p>")
                         );

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

void DialogPreviewFile::setCodecList(const QList<QTextCodec *> &list)
{
    encodingComboBox->clear();
    foreach (QTextCodec *codec, list)
        encodingComboBox->addItem(codec->name(), codec->mibEnum());
}

void DialogPreviewFile::setEncodedData(const QByteArray &data,
                                 const QString &fileName,
                                 const int &fileFormat)
{
    m_fileName = fileName;
    m_fileFormat = fileFormat;
    encodedData = data;
    updateTextEdit();
}

void DialogPreviewFile::updateTextEdit()
{
    int mib = encodingComboBox->itemData(
                      encodingComboBox->currentIndex()).toInt();
    QTextCodec *codec = QTextCodec::codecForMib(mib);
    qDebug () << "Selected codec name: " << codec->name();
    QTextStream in(&encodedData);

    decodedStr = codec->toUnicode(encodedData);

//    // FOR FUTURE REFERENCE (IF QTextCodec Class GETS REMOVED FROM QT6 QT5 CORE COMPAT MODULE)
//    // Check whether QStringConverter supports user selected encoding
//    std::optional<QStringConverter::Encoding> test_support = QStringConverter::encodingForName(codec->name());
//    if ( test_support.has_value()) {
//        // Encoding supported
//        in.setEncoding(test_support.value());
//        qDebug () << " - codec: " << codec->name()
//                 << " supported by QStringConverter. QTextStream Encoding set to: "
//                 <<  QStringConverter::nameForEncoding(test_support.value());
//    }
//    else {
//        // Encoding not supported. Retreat to UTF-9
//        qDebug () << " - codec: " << codec->name()
//                 << " NOT supported by QStringConverter. QTextStream set to autoDetectUnicode. ";
//        in.setAutoDetectUnicode(false);
//    }

//     Read the text stream
//    decodedStr = in.readAll();

    // Update text in dialog
    textEdit->setPlainText(decodedStr);
}

void DialogPreviewFile::accept() {
    int mib = encodingComboBox->itemData(
                      encodingComboBox->currentIndex()).toInt();
    QTextCodec *codec = QTextCodec::codecForMib(mib);
    qDebug () << "User accepted. Returning codec name:" << codec->name();
    emit loadNetworkFileWithCodec(m_fileName, codec->name(), m_fileFormat);
    QDialog::accept();

}
