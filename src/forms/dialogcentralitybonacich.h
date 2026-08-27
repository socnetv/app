/**
 * @file dialogcentralitybonacich.h
 * @brief Declares the DialogCentralityBonacich class for collecting the Bonacich Power Centrality parameters (alpha, beta) in SocNetV.
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

#ifndef DIALOGCENTRALITYBONACICH_H
#define DIALOGCENTRALITYBONACICH_H

#include <QDialog>

#include "ui_dialogcentralitybonacich.h"

class DialogCentralityBonacich : public QDialog
{
    Q_OBJECT
public:
    explicit DialogCentralityBonacich(QWidget *parent = Q_NULLPTR,
                                      const qreal alpha = 1.0,
                                      const qreal beta = 0.1);

    // Call before exec(). bound > 0 clamps the beta spinbox to +-(bound * 0.999) (the
    // convergence condition is a strict "<", not "<=") and shows the actual number in the hint
    // label. bound == 0 means the network has no such bound (e.g. nilpotent/DAG-like) - the
    // spinbox keeps its default generous range and the hint label says so. alpha is never
    // bounded - it's a free outer scale factor.
    void setBetaBound(qreal bound);

public slots:
    void getUserChoices();

signals:
    void userChoices(const qreal alpha, const qreal beta);

private:
    Ui::DialogCentralityBonacich ui;
};

#endif
