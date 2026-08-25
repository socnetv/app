/**
 * @file dialogcentralitykatz.h
 * @brief Declares the DialogCentralityKatz class for collecting the Katz Centrality attenuation factor (alpha) in SocNetV.
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

#ifndef DIALOGCENTRALITYKATZ_H
#define DIALOGCENTRALITYKATZ_H

#include <QDialog>

#include "ui_dialogcentralitykatz.h"

class DialogCentralityKatz : public QDialog
{
    Q_OBJECT
public:
    explicit DialogCentralityKatz(QWidget *parent = Q_NULLPTR, const qreal alpha = 0.1);

public slots:
    void getUserChoices();

signals:
    void userChoices(const qreal alpha);

private:
    Ui::DialogCentralityKatz ui;
};

#endif
