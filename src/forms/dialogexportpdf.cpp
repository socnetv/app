/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                         dialogexportpdf.cpp  -  description
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

#include "dialogexportpdf.h"
#include "graphicswidget.h"
#include "ui_dialogexportpdf.h"

#include <QPushButton>
#include <QDebug>
#include <QGraphicsColorizeEffect>
#include <QFileDialog>

DialogExportPDF::DialogExportPDF (QWidget *parent ) :
    QDialog (parent),
    ui(new Ui::DialogExportPDF)
{
    ui->setupUi(this);

    m_fileName = "";

    m_dpi = 75;
    m_printerMode = QPrinter::ScreenResolution;
    m_orientation = QPageLayout::Portrait;

    // Populate printer modes
    QStringList resList;
    resList << "Screen" << "Print";
    ui->qualitySelect->addItems(resList);

    // Populate dpi (currently only 75dpi is supported)
    QStringList dpiList;
    dpiList << "75" << "300" << "600" << "1200";
    ui->resolutionSelect->addItems(dpiList);
    ui->resolutionSelect->setDisabled(true);

    QStringList orientationList;
    orientationList << "Portrait" << "Landscape";
    ui->orientationSelect->addItems(orientationList);

    // Connect dialog signals to slots
    connect (ui->fileDirSelectButton, &QToolButton::clicked,
             this, &DialogExportPDF::getFilename);

    connect(ui->qualitySelect, SIGNAL ( currentTextChanged (const QString &)),
          this, SLOT(getPrinterMode(const QString &)) );

    connect ( ui->buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    // Set Cancel as default button
    // The OK button disabled until user selects a file.
    (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);
    (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);

    // Set which widget will have focus
    ui->fileDirSelectButton->setFocus(Qt::OtherFocusReason);

}



DialogExportPDF::~DialogExportPDF()
{
    delete ui;
}


void DialogExportPDF::checkFilename(const QString &fileName){

    m_fileName = fileName;

    if (!m_fileName.isEmpty() && QFileInfo(m_fileName).absoluteDir().exists() ) {
        if ( QFileInfo(m_fileName).suffix().isEmpty() ) {
            m_fileName.append(".pdf");
        }
        else if (  QString::compare(
                      QFileInfo(m_fileName).suffix() , "pdf", Qt::CaseInsensitive
                      )
                  ) {
            qDebug() << "suffix() : " << QFileInfo(m_fileName).suffix();
            m_fileName.append(".pdf");
        }
        ui->fileEdit->setText(m_fileName);
        ui->fileEdit->setGraphicsEffect(0);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);
    }
    else {
        qDebug() << " empty or dir does not exist";
        // TODO Error message on form.
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->fileEdit->setGraphicsEffect(effect);
        ui->fileDirSelectButton->setGraphicsEffect(effect);
        (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
        (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);

    }

}

/**
 * @brief Gets the filename of the PDF
 */
void DialogExportPDF::getFilename(){

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to pdf"),
                                                    "",
                                                    tr("PDF (*.pdf)"));

    checkFilename(fileName);
}



/**
 * @brief Gets printer quality mode
 */
void DialogExportPDF::getPrinterMode(const QString &mode){
    if (!mode.isEmpty() ) {
//        m_appSettings["canvasUpdateMode"] = mode;
        if ( mode == "screen" ){
            m_printerMode = QPrinter::ScreenResolution;
        }
        else if ( mode == "print" ) {
            m_printerMode = QPrinter::PrinterResolution;

        }
    }
}




void DialogExportPDF::getUserChoices(){
    qDebug()<< "Dialog: gathering Data!...";

    // User might have entered the filename manually!
    if (m_fileName.isEmpty()) {
        getFilename();
    }

    if ( ui->qualitySelect->currentText().contains("Screen")){
        m_printerMode = QPrinter::ScreenResolution;
    }
    else {
        m_printerMode = QPrinter::PrinterResolution;
    }

    m_dpi = ui->resolutionSelect->currentText().toInt();

    if ( ui->orientationSelect->currentText().contains("Portrait")) {
        m_orientation = QPageLayout::Portrait;
    }
    else {
        m_orientation = QPageLayout::Landscape;
    }

    qDebug()<< "Dialog: emitting userChoices" ;

    emit userChoices( m_fileName, m_orientation, m_dpi, m_printerMode );
}
