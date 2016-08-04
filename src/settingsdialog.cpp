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
        QMap<QString, QString> &appSettings,
        QWidget *parent) :
    QDialog(parent),
    m_appSettings(appSettings),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

   // m_appSettings = appSettings; //only use if var passed by pointer

    ui->dataDirEdit->setText(  (m_appSettings)["dataDir"]);

    ui->debugChkBox->setChecked(
                (appSettings["printDebug"] == "true") ? true:false
            );

    m_bgColor = QColor (m_appSettings["initBackgroundColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_bgColor );
    ui->bgColorButton->setIcon(QIcon(m_pixmap));


    connect (ui->dataDirSelectButton, &QToolButton::clicked,
             this, &SettingsDialog::getDataDir);
    connect (ui->progressBarsChkBox, &QCheckBox::stateChanged,
             this, &SettingsDialog::setProgressBars);
    connect (ui->toolBarChkBox, &QCheckBox::stateChanged,
             this, &SettingsDialog::setToolBars);

    connect (ui->statusBarChkBox, &QCheckBox::stateChanged,
             this, &SettingsDialog::setStatusBars);

    connect (ui->debugChkBox, &QCheckBox::stateChanged,
             this, &SettingsDialog::setDebugMsgs);

    connect (ui->antialiasingChkBox, &QCheckBox::stateChanged,
             this, &SettingsDialog::setAntialiasing);


    connect (ui->bgColorButton, &QToolButton::clicked,
             this, &SettingsDialog::getBgColor);

    connect (ui->bgImageSelectButton, &QToolButton::clicked,
             this, &SettingsDialog::getBgImage);

    connect ( ui->buttonBox, &QDialogButtonBox::accepted,
              this, &SettingsDialog::validateSettings );
}


/**
 * @brief SettingsDialog::validateSettings
 * Validates form data and signals saveSettings to MW
 */
void SettingsDialog::validateSettings(){
    emit saveSettings();

}

void SettingsDialog::getDataDir(){

    QString m_dataDir = QFileDialog::getExistingDirectory(this, tr("Select a new data dir"),
                                                    ui->dataDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!m_dataDir.isEmpty()) {
        if (!m_dataDir.endsWith( QDir::separator() )) {
            m_dataDir += QDir::separator();
        }
       ui->dataDirEdit->setText(m_dataDir);
       m_appSettings["dataDir"]= m_dataDir;
    }

}



void SettingsDialog::getBgColor(){

    m_bgColor = QColorDialog::getColor(
                m_bgColor, this, tr("Select canvas background color") );
    if ( m_bgColor.isValid()) {
        m_pixmap.fill(m_bgColor);
        ui->bgColorButton->setIcon(QIcon(m_pixmap));
        ui->bgImageSelectEdit->setText("");
        (m_appSettings)["initBackgroundColor"] = m_bgColor.name();
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
                this, tr("Select one image for background"), (m_appSettings)["lastUsedDirPath"],
                tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                );
    if (!m_bgImage.isEmpty() ) {
        ui->bgImageSelectEdit->setText(m_bgImage);
        (m_appSettings)["initBackgroundImage"] = m_bgImage ;
        emit setBgImage();
    }
    else { //user pressed Cancel

    }

}


SettingsDialog::~SettingsDialog()
{
    delete ui;
}
