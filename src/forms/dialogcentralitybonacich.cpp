/**
 * @file dialogcentralitybonacich.cpp
 * @brief Implements the DialogCentralityBonacich class for collecting the Bonacich Power Centrality parameters (alpha, beta) in SocNetV.
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

#include "dialogcentralitybonacich.h"
#include <QPushButton>
#include <QDebug>
#include "forms_logging.h"

DialogCentralityBonacich::DialogCentralityBonacich(QWidget *parent, const qreal alpha, const qreal beta)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.alphaDoubleSpinBox->setValue(alpha);
    ui.betaDoubleSpinBox->setValue(beta);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

    (ui.buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);

    ui.alphaDoubleSpinBox->setFocus();
}

void DialogCentralityBonacich::getUserChoices()
{
    const qreal alpha = ui.alphaDoubleSpinBox->value();
    const qreal beta = ui.betaDoubleSpinBox->value();
    qCDebug(lcForms) << "DialogCentralityBonacich: emitting userChoices, alpha ="
                     << alpha << "beta =" << beta;
    emit userChoices(alpha, beta);
}
