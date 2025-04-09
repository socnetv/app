/**
 * @file dialogranderdosrenyi.h
 * @brief Declares the DialogRandErdosRenyi class for generating random graphs based on the Erdős–Rényi model in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#ifndef DIALOGRANDERDOSRENYI_H
#define DIALOGRANDERDOSRENYI_H

#include <QDialog>

#include "ui_dialogranderdosrenyi.h"

class DialogRandErdosRenyi  : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandErdosRenyi ( QWidget *parent = Q_NULLPTR, const qreal eprob = 0);

public slots:
    void checkErrors();
    void getUserChoices();
    void gnmModel();
    void gnpModel();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int nodes,
                      const QString model,
                      const int edges,
                      const qreal eprob,
                      const QString mode,
                      const bool diag);
private:
    QString model;
    QString mode;
    int nodes, edges;
    bool diag;
    Ui::DialogRandErdosRenyi ui;
};

#endif
