/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.90
 Written in Qt 4.4
 
                         datasetrecreatordialog.cpp  -  description
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

 

#include "datasetrecreatordialog.h"

#include <QDebug>
#include <QPushButton>

DataSetRecreatorDialog::DataSetRecreatorDialog (QWidget *parent) : QDialog (parent)
{
	ui.setupUi(this);	
	connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );
	(ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

	QStringList datasets_list;
	datasets_list   << "Krackhardt\'s High-tech managers - Advice relation " 
			<< "Krackhardt\'s High-tech managers - Friendship relation"
			<< "Krackhardt\'s High-tech managers - Who reports to"
			<< "Padgett\'s Florentine Families - Business relation"
			<< "Padgett\'s Florentine Families - Marital relation<F11>"
			<< "Freeman\'s EIES network - Acquaintanceship at time 1"
			<< "Freeman\'s EIES network - Acquaintanceship at time 2"
			<< "Freeman\'s EIES network - Messages";

	(ui.comboBox) -> insertItems( 1, datasets_list );
}



void DataSetRecreatorDialog::gatherData(){
	qDebug()<< "Dialog: gathering Data!...";
	QString dataset_name = (ui.comboBox) -> currentText(); 
	qDebug()<< "Dialog: emitting userChoises signal ";
	emit userChoices( dataset_name );		
}
