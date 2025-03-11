/**
 * @file dialogfilteredgesbyweight.cpp
 * @brief Implements the DialogFilterEdgesByWeight class for filtering edges based on weight in the social network visualizer.
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

#include "dialogfilteredgesbyweight.h"
#include <QPushButton>
#include <QDebug>

DialogFilterEdgesByWeight::DialogFilterEdgesByWeight(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

	(ui.buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);

	(ui.overThresholdBt)->setChecked(true);
}

void DialogFilterEdgesByWeight::getUserChoices(){
	qDebug()<< "Dialog: gathering Data!...";
	bool overThreshold=false;
	float my_threshold = static_cast <float> ( (ui.weightThreshold)->value() );
	if ( ui.overThresholdBt->isChecked() ) {
		qDebug()<< "Dialog: We will filter edges weighted more than threshold: " << my_threshold;
		overThreshold = true;
	}
	else {
		qDebug()<< "Dialog: We will filter edges weighted less than threshold: " << my_threshold;
		overThreshold = false;
	}	
	qDebug()<< "Dialog: emitting userChoices" ;
	emit userChoices( my_threshold, overThreshold );		
}
