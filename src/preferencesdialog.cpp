/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
 Written in Qt

                         preferencesdialog.cpp  -  description
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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include <QFileDialog>
#include <QColorDialog>
#include <QTextStream>
#include <QMap>

PreferencesDialog::PreferencesDialog(
                                     const QMap<QString, QString> &appSettings ,
        QWidget *parent,
                                     QString settingsFilePath,
                                     QString *dataDir,
                                     QColor *bgColor,
                                     QString *lastPath) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    m_preferencesPath = settingsFilePath;

    m_dataDir = new QString ;
    m_dataDir  = dataDir;
    ui->dataDirEdit->setText(*dataDir);
    connect (ui->dataDirSelectButton, &QToolButton::clicked, this, &PreferencesDialog::getDataDir);
    connect (ui->progressBarsChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setProgressBars);
    connect (ui->toolBarChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setToolBars);
    connect (ui->statusBarChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setStatusBars);
    connect (ui->debugChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setDebugMsgs);
    connect (ui->antialiasingChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setAntialiasing);


//    m_bgColor = new QColor;
//    m_bgColor = bgColor;
//    m_pixmap = QPixmap(60,20) ;
//    m_pixmap.fill(*m_bgColor);
        m_pixmap = QPixmap(60,20) ;
        m_pixmap.fill( appSettings["initBackgroundColor"]);

    ui->bgColorButton->setIcon(QIcon(m_pixmap));

    connect (ui->bgColorButton, &QToolButton::clicked,
             this, &PreferencesDialog::getBgColor);

    m_lastPath = new QString;
    m_lastPath = lastPath;
    connect (ui->bgImageSelectButton, &QToolButton::clicked, this, &PreferencesDialog::getBgImage);


    connect ( ui->buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::savePreferences );
}


void PreferencesDialog::getDataDir(){

    *m_dataDir = QFileDialog::getExistingDirectory(this, tr("Select a new data dir"),
                                                    ui->dataDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->dataDirEdit->setText(*m_dataDir);
}

void PreferencesDialog::getBgColor(){
    *m_bgColor = QColorDialog::getColor(
                *m_bgColor, this, tr("Select canvas background color") );
    if ( m_bgColor->isValid()) {
        m_pixmap.fill(*m_bgColor);
        ui->bgColorButton->setIcon(QIcon(m_pixmap));
        ui->bgImageSelectEdit->setText("");
        emit setBgColor(*m_bgColor);
    }
    else {
        // user pressed Cancel
    }

}


/*
 *
 */
void PreferencesDialog::getBgImage(){
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

void PreferencesDialog::savePreferences(){
    QFile file ( m_preferencesPath );
    if ( !file.open( QIODevice::WriteOnly ) )  {
        //qDebug()<< "Error opening preferences file!";
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

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}
