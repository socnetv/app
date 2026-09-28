/**
 * @file dialogcentralitypn.h
 * @brief Declares the DialogCentralityPN class for collecting the PN Centrality mode in SocNetV.
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

#ifndef DIALOGCENTRALITYPN_H
#define DIALOGCENTRALITYPN_H

#include <QDialog>

#include "global.h"
#include "ui_dialogcentralitypn.h"

SOCNETV_USE_NAMESPACE

class DialogCentralityPN : public QDialog
{
    Q_OBJECT
public:
    // isDirected selects which mode radio buttons are enabled: undirected graphs only ever
    // allow All; directed graphs only ever allow Out/In (All is disabled and unchecked).
    explicit DialogCentralityPN(QWidget *parent, const bool isDirected);

public slots:
    void getUserChoices();

signals:
    void userChoices(const PNMode mode);

private:
    Ui::DialogCentralityPN ui;
};

#endif
