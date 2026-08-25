/**
 * @file dialogcentralitykatz.cpp
 * @brief Implements the DialogCentralityKatz class for collecting the Katz Centrality attenuation factor (alpha) in SocNetV.
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

#include "dialogcentralitykatz.h"
#include <QPushButton>
#include <QDebug>
#include "forms_logging.h"

DialogCentralityKatz::DialogCentralityKatz(QWidget *parent, const qreal alpha) : QDialog(parent)
{
    ui.setupUi(this);

    ui.alphaDoubleSpinBox->setValue(alpha);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

    (ui.buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);

    ui.alphaDoubleSpinBox->setFocus();
}

void DialogCentralityKatz::getUserChoices()
{
    const qreal alpha = ui.alphaDoubleSpinBox->value();
    qCDebug(lcForms) << "DialogCentralityKatz: emitting userChoices, alpha =" << alpha;
    emit userChoices(alpha);
}
