/**
 * @file dialogfilternodes.cpp
 * @brief Implements the DialogFilterNodesByCentrality class for filtering nodes based on their attributes.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
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


DialogFilterNodesByCentrality::DialogFilterNodesByCentrality(const QStringList &indexNames,
                                                             QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.indexComboBox->addItems(indexNames);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));
    ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    ui.overThresholdBt->setChecked(true);
}


void DialogFilterNodesByCentrality::getUserChoices()
{
    const float threshold     = static_cast<float>(ui.weightThreshold->value());
    const bool overThreshold  = ui.overThresholdBt->isChecked();

    // prominenceIndexList is 0-based but IndexType starts at 1
    const IndexType centralityIndex =
        static_cast<IndexType>(ui.indexComboBox->currentIndex() + 1);

    qDebug() << "DialogFilterNodesByCentrality:"
             << "index:" << ui.indexComboBox->currentText()
             << "(" << static_cast<int>(centralityIndex) << ")"
             << "threshold:" << threshold
             << "overThreshold:" << overThreshold;

    emit userChoices(threshold, overThreshold, centralityIndex);
}
