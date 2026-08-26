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

void DialogCentralityBonacich::setBetaBound(qreal bound)
{
    if (bound > 0)
    {
        const qreal maxBeta = bound * 0.999;
        ui.betaDoubleSpinBox->setMinimum(-maxBeta);
        ui.betaDoubleSpinBox->setMaximum(maxBeta);
        if (ui.betaDoubleSpinBox->value() > maxBeta)
        {
            ui.betaDoubleSpinBox->setValue(maxBeta);
        }
        else if (ui.betaDoubleSpinBox->value() < -maxBeta)
        {
            ui.betaDoubleSpinBox->setValue(-maxBeta);
        }
        ui.hintLabel->setText(
            tr("Beta must be smaller in absolute value than %1 (1 / this network's largest "
               "eigenvalue) - the spinbox above already enforces this. With negative beta, "
               "scores can come out negative - that is expected, not an error.").arg(bound));
    }
    else
    {
        ui.hintLabel->setText(
            tr("This network has no such bound on beta - any value converges. With negative "
               "beta, scores can come out negative - that is expected, not an error."));
    }
}

void DialogCentralityBonacich::getUserChoices()
{
    const qreal alpha = ui.alphaDoubleSpinBox->value();
    const qreal beta = ui.betaDoubleSpinBox->value();
    qCDebug(lcForms) << "DialogCentralityBonacich: emitting userChoices, alpha ="
                     << alpha << "beta =" << beta;
    emit userChoices(alpha, beta);
}
