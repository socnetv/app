/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.6
 Written in Qt
 
                         webcrawlerdialog.cpp  -  description
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



#include "webcrawlerdialog.h"
#include  <QDebug>
#include <QPushButton>

WebCrawlerDialog::WebCrawlerDialog(QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);
    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui.seedUrlEdit)->setFocus();

    connect (ui.extLinksCheckBox, &QCheckBox::stateChanged,
             this, &WebCrawlerDialog::checkErrors);

    connect (ui.intLinksCheckBox, &QCheckBox::stateChanged,
             this, &WebCrawlerDialog::checkErrors);

    if ( !ui.extLinksCheckBox->isChecked()  &&!ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
    }

}

void WebCrawlerDialog::checkErrors(){
    qDebug()<< "WebCrawlerDialog::checkErrors...";
    if ( !ui.extLinksCheckBox->isChecked()  && !ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
    }
    else
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);

}

void WebCrawlerDialog::gatherData(){
    qDebug()<< "WebCrawlerDialog::gatherData()...";
    bool extLinks=true, intLinks=false;
    QString website = (ui.seedUrlEdit)->text() ;
    int maxRecursion = (ui.maxRecursionLevelSpinBox) -> value();
    int maxNodes = (ui.maxNodesSpinBox) -> value();

    if ( ui.extLinksCheckBox -> isChecked() ) {
        qDebug()<< "	External links will be crawled... " ;
        extLinks = true;
    }
    else {
        qDebug()<< "	No external links... ";
        extLinks = false;
    }
    if ( ui.intLinksCheckBox -> isChecked() ) {
        qDebug()<< "	Internal links will be crawled too. " ;
        intLinks = true;
    }
    else {
        qDebug()<< "	No internal links. ";
        intLinks = false;
        if (!intLinks && !extLinks)
            return;
    }

    qDebug()<< "	Website: " << website;
    qDebug()<< "	maxRecursion " << maxRecursion << "  maxNodes " << maxNodes  ;
    emit userChoices( website, maxNodes, maxRecursion,  extLinks, intLinks );
}
