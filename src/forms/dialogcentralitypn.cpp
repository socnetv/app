/**
 * @file dialogcentralitypn.cpp
 * @brief Implements the DialogCentralityPN class for collecting the PN Centrality mode in SocNetV.
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

#include "dialogcentralitypn.h"
#include <QPushButton>
#include <QDebug>
#include "forms_logging.h"

DialogCentralityPN::DialogCentralityPN(QWidget *parent, const bool isDirected) : QDialog(parent)
{
    ui.setupUi(this);

    if (isDirected)
    {
        ui.allRadioButton->setEnabled(false);
        ui.outRadioButton->setChecked(true);
        ui.hintLabel->setText(
            tr("This network is directed - \"All\" is disabled; pick Out or In."));
    }
    else
    {
        ui.outRadioButton->setEnabled(false);
        ui.inRadioButton->setEnabled(false);
        ui.allRadioButton->setChecked(true);
        ui.hintLabel->setText(
            tr("This network is undirected - only \"All\" is valid."));
    }

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

    (ui.buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);
}

void DialogCentralityPN::getUserChoices()
{
    PNMode mode = PNMode::All;
    if (ui.outRadioButton->isChecked())
        mode = PNMode::Out;
    else if (ui.inRadioButton->isChecked())
        mode = PNMode::In;

    qCDebug(lcForms) << "DialogCentralityPN: emitting userChoices, mode ="
             << static_cast<int>(mode);
    emit userChoices(mode);
}
