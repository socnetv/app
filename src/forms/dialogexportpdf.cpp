/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialogexportpdf.cpp  -  description
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

#include "dialogexportpdf.h"
#include <QPushButton>
#include <QDebug>

#include <QFileDialog>

DialogExportPDF::DialogExportPDF (QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    m_printerMode = QPrinter::ScreenResolution;

    QStringList resList;
    resList << "Screen" << "Print";
    ui.qualitySelect->addItems(resList);

    /**
     * dialog signals to slots
     */
    connect (ui.fileDirSelectButton, &QToolButton::clicked,
             this, &DialogExportPDF::getFilename);

    connect(ui.qualitySelect, SIGNAL ( currentIndexChanged (const QString &)),
          this, SLOT(getPrinterMode(const QString &)) );

    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);


}


/**
 * @brief Gets the the name of the PDF
 */
void DialogExportPDF::getFilename(){

    QString m_fileName = QFileDialog::getSaveFileName(this, tr("Save to pdf"),
                                                    "",
                                                    tr("PDF (*.pdf)"));

    if (!m_fileName.isEmpty()) {
        if (QFileInfo(m_fileName).suffix().isEmpty()) {
            m_fileName.append(".pdf");
        }
        ui.fileEdit->setText(m_fileName);
    }

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

    if (m_fileName.isEmpty()) {
        getFilename();
    }

    qDebug()<< "Dialog: emitting userChoices" ;
    emit userChoices( m_fileName, m_dpi, m_printerMode );
}
