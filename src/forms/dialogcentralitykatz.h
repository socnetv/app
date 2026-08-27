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

    // Call before exec(). bound > 0 clamps the spinbox to +-(bound * 0.999) (the convergence
    // condition is a strict "<", not "<=") and shows the actual number in the hint label.
    // bound == 0 means the network has no such bound (e.g. nilpotent/DAG-like) - the spinbox
    // keeps its default generous range and the hint label says so.
    void setAlphaBound(qreal bound);

public slots:
    void getUserChoices();

signals:
    void userChoices(const qreal alpha);

private:
    Ui::DialogCentralityKatz ui;
};

#endif
