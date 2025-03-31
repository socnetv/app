/**
 * @file dialogfilternodes.cpp
 * @brief Implements the DialogFilterNodesByCentrality class for filtering nodes based on their attributes.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialogfilternodesbycentrality.h"
#include <QPushButton>
#include <QDebug>

DialogFilterNodesByCentrality::DialogFilterNodesByCentrality(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

	(ui.buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);

	(ui.overThresholdBt)->setChecked(true);
}

void DialogFilterNodesByCentrality::getUserChoices(){
	qDebug()<< "Dialog: gathering Data!...";
	bool overThreshold=false;
	float my_threshold = static_cast <float> ( (ui.weightThreshold)->value() );
	if ( ui.overThresholdBt->isChecked() ) {
        qDebug()<< "Dialog: We will filter nodes with index score more than threshold: " << my_threshold;
		overThreshold = true;
	}
	else {
        qDebug()<< "Dialog: We will filter nodes with index score less than threshold: " << my_threshold;
		overThreshold = false;
	}	
	qDebug()<< "Dialog: emitting userChoices" ;
	emit userChoices( my_threshold, overThreshold );		
}
