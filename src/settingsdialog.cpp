/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
 Written in Qt

                         settingsdialog.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QFileDialog>
#include <QColorDialog>
#include <QTextStream>
#include <QMap>

SettingsDialog::SettingsDialog(
                                      QMap<QString, QString> &appSettings ,
        QWidget *parent,
                                     QString settingsFilePath,
                                     QString *dataDir,
                                     QColor *bgColor,
                                     QString *lastPath) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    m_appSettings = appSettings;

    m_settingsPath = settingsFilePath;

    m_dataDir = new QString ;
    m_dataDir  = dataDir;
    ui->dataDirEdit->setText(*dataDir);
    connect (ui->dataDirSelectButton, &QToolButton::clicked, this, &SettingsDialog::getDataDir);
    connect (ui->progressBarsChkBox, &QCheckBox::stateChanged, this, &SettingsDialog::setProgressBars);
    connect (ui->toolBarChkBox, &QCheckBox::stateChanged, this, &SettingsDialog::setToolBars);
    connect (ui->statusBarChkBox, &QCheckBox::stateChanged, this, &SettingsDialog::setStatusBars);
    connect (ui->debugChkBox, &QCheckBox::stateChanged, this, &SettingsDialog::setDebugMsgs);
    connect (ui->antialiasingChkBox, &QCheckBox::stateChanged, this, &SettingsDialog::setAntialiasing);


//    m_bgColor = new QColor;
//    m_bgColor = bgColor;
//    m_pixmap = QPixmap(60,20) ;
//    m_pixmap.fill(*m_bgColor);
    m_bgColor = QColor (appSettings["initBackgroundColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_bgColor );
    ui->bgColorButton->setIcon(QIcon(m_pixmap));

    connect (ui->bgColorButton, &QToolButton::clicked,
             this, &SettingsDialog::getBgColor);

    m_lastPath = new QString;
    m_lastPath = lastPath;
    connect (ui->bgImageSelectButton, &QToolButton::clicked, this, &SettingsDialog::getBgImage);


    connect ( ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings );
}


void SettingsDialog::getDataDir(){

    *m_dataDir = QFileDialog::getExistingDirectory(this, tr("Select a new data dir"),
                                                    ui->dataDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->dataDirEdit->setText(*m_dataDir);
}



void SettingsDialog::getBgColor(){

    m_bgColor = QColorDialog::getColor(
                m_bgColor, this, tr("Select canvas background color") );
    if ( m_bgColor.isValid()) {
        m_pixmap.fill(m_bgColor);
        ui->bgColorButton->setIcon(QIcon(m_pixmap));
        ui->bgImageSelectEdit->setText("");
        m_appSettings["initBackgroundColor"] = m_bgColor.name();
        emit setBgColor(m_bgColor);
    }
    else {
        // user pressed Cancel
    }

}


/*
 *
 */
void SettingsDialog::getBgImage(){
    QString m_bgImage = QFileDialog::getOpenFileName(
                this, tr("Select one image for background"), *m_lastPath,
                tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                );
    if (!m_bgImage.isEmpty() ) {
        ui->bgImageSelectEdit->setText(m_bgImage);
        emit setBgImage(m_bgImage);
    }
    else { //user pressed Cancel

    }

}

void SettingsDialog::saveSettings(){
    QFile file ( m_settingsPath );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        //qDebug()<< "Error opening settings file!";
        //emit statusMessage (QString(tr("Could not write to %1")).arg(fileName) );
        return;
    }
    QTextStream outText ( &file );

    outText.setCodec("UTF-8");
    outText << "Default save folder == " << ui->dataDirEdit->text();
    outText << "Show progress bars" << ui->progressBarsChkBox->isChecked();
    outText << ui->toolBarChkBox->isChecked();
    outText << ui->statusBarChkBox->isChecked();
    outText << ui->debugChkBox->isChecked();
    outText << ui->antialiasingChkBox->isChecked();

    file.close();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
