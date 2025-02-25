/**
 * @file dialogexportimage.cpp
 * @brief Implements the DialogExportImage class for exporting network visualizations as images in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

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

    // Get supported Image formats
    QStringList imgFormats;
    QByteArray bytes;
    foreach (bytes, QImageWriter::supportedImageFormats()) {
        imgFormats << QString(bytes);
    }
    ui->formatSelect->addItems(imgFormats);

    // Connect dialog signals to slots
    connect ( ui->fileDirSelectButton, &QToolButton::clicked,
             this, &DialogExportImage::getFilename);

    connect ( ui->formatSelect, SIGNAL(currentTextChanged (const QString &)),
          this, SLOT ( getFormat(const QString &)) );

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    // Prepare Quality slider and spin box, and connect them
    changeQualityRange(1,100,1);

    connect ( ui->qualitySlider, SIGNAL(valueChanged(int)),
              ui->qualitySpinBox,SLOT(setValue(int)) );

    connect ( ui->qualitySpinBox, SIGNAL(valueChanged(int)),
              ui->qualitySlider,SLOT(setValue(int)) );
    ui->qualitySlider->setValue(100);

    // Prepare Compression slider and spin box, and connect them
    changeCompressionRange(1,100,1);
    connect ( ui->compressionSlider, SIGNAL(valueChanged(int)),
              ui->compressionSpinBox,SLOT(setValue(int)) );

    connect ( ui->compressionSpinBox, SIGNAL(valueChanged(int)),
              ui->compressionSlider,SLOT(setValue(int)) );
    ui->compressionSlider->setValue(0);

    // Set default button
    // OK button is disabled until user has selected a filename.
    (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);
    (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);

    // Set which widget will have focus
    ui->fileDirSelectButton->setFocus(Qt::OtherFocusReason);

}


/**
 * @brief DialogExportImage::~DialogExportImage
 */
DialogExportImage::~DialogExportImage()
{
    delete ui;
}


/**
 * @brief Changes Compression widgets range and stepping
 * @param min
 * @param max
 * @param step
 */
void DialogExportImage::changeCompressionRange(const int &min, const int &max, const int &step) {
    ui->compressionSlider->setSingleStep(step);
    ui->compressionSlider->setTickInterval(step);
    ui->compressionSpinBox->setSingleStep(step);
    ui->compressionSpinBox->setRange(min,max);
}


/**
 * @brief Changes Quality widgets range and stepping
 * @param min
 * @param max
 * @param step
 */
void DialogExportImage::changeQualityRange(const int &min, const int &max, const int &step){
    ui->qualitySlider->setSingleStep(step);
    ui->qualitySlider->setTickInterval(step);
    ui->qualitySpinBox->setSingleStep(step);
    ui->qualitySpinBox->setRange(min,max);

}



/**
 * @brief Opens a dialog to get the filename of the new image to export
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
        ui->fileDirSelectButton->setGraphicsEffect(0);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);
    }
    else {
        qDebug() << "Empty filaname or dir does not exist";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->fileEdit->setGraphicsEffect(effect);
        ui->fileDirSelectButton->setGraphicsEffect(effect);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
        (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);

    }

}



/**
 * @brief Gets the file format of the Image
 */
void DialogExportImage::getFormat(const QString &format){
    QString m_format = format.toLower();
    qDebug() << "format:" << m_format;
    QString m_fileName = ui->fileEdit->text();
    qDebug() << "filename" << m_fileName;
    qDebug() << "suffix" << QFileInfo(m_fileName).suffix();
    if (  QString::compare(QFileInfo(m_fileName).suffix() , m_format, Qt::CaseInsensitive) != 0 ) {
        // User filename suffix differs from selected format. Correct the filename suffix.
        m_fileName = QFileInfo(m_fileName).absolutePath() + QDir::separator() + QFileInfo(m_fileName).completeBaseName().append("."+m_format);
        qDebug() << "Corrected filename:" << m_fileName;
    }

    ui->fileEdit->setText(m_fileName);
}


void DialogExportImage::getUserChoices(){

    QByteArray m_format = ui->formatSelect->currentText().toLower().toUtf8();
    QString m_fileName = ui->fileEdit->text();
    int m_quality = ui->qualitySpinBox->value();
    int m_compression = ui->compressionSpinBox->value();

    qDebug()<< "user choices: "
            << m_fileName
            << m_format
            << m_quality
            << m_compression;

    emit userChoices(m_fileName, m_format, m_quality, m_compression);

}


