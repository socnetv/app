/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.90
 Written in Qt 4.4
 
                         webcrawlerdialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2010 by Dimitris B. Kalamaras
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
	
	(ui.goOutCheckBox)-> setChecked(true);
	(ui.websiteLineEdit)->setFocus();
	
}



void WebCrawlerDialog::gatherData(){
	qDebug()<< "Dialog: gathering Data!...";
	bool goOut=false;
	QString website = (ui.websiteLineEdit)->text() ;
	int maxRecursion = (ui.maxRecursionLevelSpinBox) -> value();
	int maxNodes = (ui.maxNodesSpinBox) -> value();
	
	if ( ui.goOutCheckBox -> isChecked() ) {
		qDebug()<< "	We will go out of this site... " ;
		goOut = true;
	}
	else {
		qDebug()<< "	We will not go out of this site... ";
		goOut = false;
	}	
	qDebug()<< "	Website: " << website;  
	qDebug()<< "	maxRecursion " << maxRecursion << "  maxNodes " << maxNodes  ;
	emit userChoices( website, maxNodes, maxRecursion,  goOut );		
}
