/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.6
 Written in Qt 4.4
 
                         filteredgesdialog.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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


#include "filteredgesdialog.h"
#include <QPushButton>
#include <QDebug>

FilterEdgesDialog::FilterEdgesDialog (QWidget *parent) : QDialog (parent)
{
	ui.setupUi(this);	
	connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );
	
	(ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);
	
	(ui.overThresholdBt)-> setChecked(true);
	
	//connect( this, SIGNAL( userChoices( float, bool) ), parent, SLOT( slotFilterEdges (float, bool) ) );	
} 




void FilterEdgesDialog::gatherData(){
	qDebug()<< "Dialog: gathering Data!...";
	bool overThreshold=false;
	float my_threshold = static_cast <float> ( (ui.weightTheshold)->value() );
	if ( ui.overThresholdBt -> isChecked() ) {
		qDebug()<< "Dialog: more than threshold " << my_threshold;
		overThreshold = true;
	}
	else {
		qDebug()<< "Dialog: less than threshold " << my_threshold;
		overThreshold = false;
	}	
	qDebug()<< "Dialog: emitting userChoices" ;
	emit userChoices( my_threshold, overThreshold );		
}
