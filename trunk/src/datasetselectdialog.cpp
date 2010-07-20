/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.90
 Written in Qt 4.4
 
                         datasetselectdialog.cpp  -  description
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

 

#include "datasetselectdialog.h"

#include <QDebug>
#include <QPushButton>

DataSetSelectDialog::DataSetSelectDialog (QWidget *parent) : QDialog (parent)
{
	ui.setupUi(this);	
	connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );
	(ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

	QStringList datasets_list;
	datasets_list   << "Krackhardt_High-tech_managers_Advice_relation.sm" 
			<< "Krackhardt_High-tech_managers_Friendship_relation.sm"
			<< "Krackhardt_High-tech_managers_ReportsTo_relation.sm"
			<< "Padgett_Florentine_Families_Marital_relation.sm"
			<< "Padgett_Florentine_Families_Business_relation.sm"
			<< "Zachary_Karate_Club_Simple_Ties.sm"
			<< "Zachary_Karate_Club_Weighted_Ties.sm"
			<< "Bernard_Killworth_Fraternity_Symmetric_Observer_Data.sm"
			<< "Bernard_Killworth_Fraternity_Non_Symmetric_Cognitive_Data.sm"
			<< "Galaskiewicz_CEOs_and_clubs_two_mode_data.sm"
			<< "Freeman_EIES_network_Acquaintanceship_at_time-1"
			<< "Freeman_EIES_network_Acquaintanceship_at_time-2"
			<< "Freeman_EIES_network_Messages";

	(ui.comboBox) -> insertItems( 1, datasets_list );
}



void DataSetSelectDialog::gatherData(){
	qDebug()<< "Dialog: gathering Data!...";
	QString dataset_name = (ui.comboBox) -> currentText(); 
	qDebug()<< "Dialog: emitting userChoises signal ";
	emit userChoices( dataset_name );
			
}

