/**
 * @file dialogfilternodesbycentrality.h
 * @brief Defines the DialogFilterNodesByCentrality class for filtering nodes based on some attribute.
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

#ifndef DIALOGFILTERNODESBYCENTRALITY_H
#define DIALOGFILTERNODESBYCENTRALITY_H

#include "ui_dialogfilternodesbycentrality.h"
#include <QDialog>
#include <QStringList>

#include "global.h" // for IndexType

SOCNETV_USE_NAMESPACE

class DialogFilterNodesByCentrality : public QDialog
{
    Q_OBJECT
public:
    explicit DialogFilterNodesByCentrality(const QStringList &indexNames,
                                           const QVector<bool> &computedMask,
                                           QWidget *parent = nullptr);

public slots:
    void getUserChoices();

signals:
    void userChoices(const float threshold,
                     const bool overThreshold,
                     const IndexType centralityIndex);

private:
    Ui::DialogFilterNodesByCentrality ui;
};

#endif // DIALOGFILTERNODESBYCENTRALITY_H