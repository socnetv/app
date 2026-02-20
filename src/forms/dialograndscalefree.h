/**
 * @file randscalefreeddialog.h
 * @brief Declares the RandScaleFreeDialog class for generating scale-free networks based on the Barabási–Albert model in SocNetV.
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


#ifndef DIALOGRANDSCALEFREE_H
#define DIALOGRANDSCALEFREE_H

#include <QDialog>

#include "ui_dialograndscalefree.h"

class DialogRandScaleFree : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandScaleFree(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors();
    void getUserChoices();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int &nodes,
                      const int &power,
                      const int &initialNodes,
                      const int &edgesPerStep,
                      const qreal &zeroAppeal,
                      const QString &mode);
private:
    QString mode;
    int nodes; // n
    int initialNodes; // m0
    int edgesPerStep; //m
    int power;
    qreal zeroAppeal; // a
    bool diag;
    Ui::DialogRandScaleFree ui;

};

#endif
