/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialogexportimage.cpp  -  description
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

#include "dialogexportimage.h"
#include "ui_dialogexportimage.h"

#include <QPushButton>
#include <QImageWriter>
#include <QDebug>
#include <QGraphicsColorizeEffect>
#include <QFileDialog>


DialogExportImage::DialogExportImage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogExportImage)
{
    ui->setupUi(this);
    QStringList imgFormats;
    QByteArray bytes;
    foreach (bytes, QImageWriter::supportedImageFormats()) {
        imgFormats << QString(bytes);
    }

    ui->formatSelect->addItems(imgFormats);

    /**
     * dialog signals to slots
     */

    connect (ui->fileDirSelectButton, &QToolButton::clicked,
             this, &DialogExportImage::getFilename);

    connect(ui->formatSelect, SIGNAL(currentIndexChanged (const QString &)),
          this, SLOT ( getFormat(const QString &)) );


    /**
      * set default button
      */
    (ui->buttonBox) -> button (QDialogButtonBox::Cancel) -> setDefault(true);
    (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);

}

DialogExportImage::~DialogExportImage()
{
    delete ui;
}


/**
 * @brief Gets the filename of the Image
 */
void DialogExportImage::getFilename(){

    QString m_format = ui->formatSelect->currentText().toLower();
    QString m_filter = m_format.toUpper() + " (*." + m_format + ")";
    QString m_fileName = QFileDialog::getSaveFileName(this, tr("Save to image"),
                                                    "",
                                                    m_filter);



    if (!m_fileName.isEmpty() && QFileInfo(m_fileName).absoluteDir().exists() ) {
        if ( QFileInfo(m_fileName).suffix().isEmpty() ) {
            m_fileName.append("." +m_format);
        }
        else if (  QString::compare(
                      QFileInfo(m_fileName).suffix() , m_format, Qt::CaseInsensitive
                      )
                  ) {
            m_fileName.append("." +m_format);
        }
        ui->fileEdit->setText(m_fileName);
        ui->fileEdit->setGraphicsEffect(0);
        (ui->buttonBox)->button (QDialogButtonBox::Ok) -> setEnabled(true);
        (ui->buttonBox)->button (QDialogButtonBox::Ok) -> setDefault(true);
    }
    else {
        qDebug() << " empty or dir does not exist";
        // TODO Error message on form.
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->fileEdit->setGraphicsEffect(effect);
        ui->fileDirSelectButton->setGraphicsEffect(effect);
        (ui->buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
        (ui->buttonBox) -> button (QDialogButtonBox::Cancel) -> setDefault(true);

    }

}



/**
 * @brief Gets the file format of the Image
 */
void DialogExportImage::getFormat(const QString &format){
    QString m_format = format.toLower();
    qDebug() << m_format;
    QString m_fileName = ui->fileEdit->text();
    qDebug() << m_fileName;
    qDebug() << QFileInfo(m_fileName).suffix();
    if (  QString::compare(
                          QFileInfo(m_fileName).suffix() , m_format, Qt::CaseInsensitive
                          )
          ) {
        m_fileName = QFileInfo(m_fileName).absolutePath() + QDir::separator() + QFileInfo(m_fileName).completeBaseName().append("."+m_format);
        qDebug() << m_fileName;
    }

    ui->fileEdit->setText(m_fileName);
}




