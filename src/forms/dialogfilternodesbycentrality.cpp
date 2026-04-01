/**
 * @file dialogfilternodesbycentrality.cpp
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
#include <QStandardItemModel>


DialogFilterNodesByCentrality::DialogFilterNodesByCentrality(
        const QStringList &indexNames,
        const QVector<bool> &computedMask,
        QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.indexComboBox->addItems(indexNames);

    auto *model = qobject_cast<QStandardItemModel *>(ui.indexComboBox->model());

    bool atLeastOneEnabled = false;

    if (model)
    {
        const int count = ui.indexComboBox->count();

        for (int i = 0; i < count; ++i)
        {
            const bool computed =
                (i < computedMask.size()) ? computedMask[i] : false;

            if (!computed)
            {
                if (auto *item = model->item(i))
                {
                    item->setEnabled(false);
                    item->setToolTip(
                        tr("Not computed yet. Run the analysis first."));
                }
            }
            else
            {
                atLeastOneEnabled = true;
            }
        }
    }

    // Auto-select first enabled index
    if (atLeastOneEnabled)
    {
        for (int i = 0; i < ui.indexComboBox->count(); ++i)
        {
            if (ui.indexComboBox->model()->index(i, 0)
                    .flags()
                    .testFlag(Qt::ItemIsEnabled))
            {
                ui.indexComboBox->setCurrentIndex(i);
                break;
            }
        }
    }
    else
    {
        // No indices computed
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui.indexComboBox->setEnabled(false);
    }

    connect(ui.buttonBox, SIGNAL(accepted()),
            this, SLOT(getUserChoices()));

    ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    ui.overThresholdBt->setChecked(true);
}


void DialogFilterNodesByCentrality::getUserChoices()
{
    const float threshold     = static_cast<float>(ui.weightThreshold->value());
    const bool overThreshold  = ui.overThresholdBt->isChecked();

    // NOTE: prominenceIndexList is 0-based but IndexType starts at 1
    const IndexType centralityIndex =
        static_cast<IndexType>(ui.indexComboBox->currentIndex() + 1);

    qDebug() << "DialogFilterNodesByCentrality:"
             << "index:" << ui.indexComboBox->currentText()
             << "(" << static_cast<int>(centralityIndex) << ")"
             << "threshold:" << threshold
             << "overThreshold:" << overThreshold;

    emit userChoices(threshold, overThreshold, centralityIndex);
}
