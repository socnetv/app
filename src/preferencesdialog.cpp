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


PreferencesDialog::PreferencesDialog(QWidget *parent, QString *dataDir) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    m_dataDir = new QString ;
    m_dataDir  = dataDir;
    ui->dataDirEdit->setText(*dataDir);
    connect (ui->dataDirSelectButton, &QToolButton::clicked, this, &PreferencesDialog::getDataDir);
    connect (ui->progressBarsChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setProgressBars);
    connect (ui->toolBarChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setToolBars);
    connect (ui->statusBarChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setStatusBars);
    connect (ui->antialiasingChkBox, &QCheckBox::stateChanged, this, &PreferencesDialog::setAntialiasing);
}


void PreferencesDialog::getDataDir(){

//        QString newDataDir = QFileDialog::getOpenFileName(
//                    this,
//                    tr("Select a new data dir "),
//                    ui->dataDirEdit->text());
    *m_dataDir = QFileDialog::getExistingDirectory(this, tr("Select a new data dir"),
                                                    ui->dataDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    ui->dataDirEdit->setText(*m_dataDir);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}
