/**
 * @file dialograndlattice.h
 * @brief Declares the DialogRandLattice class for configuring random lattice generation in SocNetV.
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


#ifndef DIALOGRANDLATTICE_H
#define DIALOGRANDLATTICE_H

#include <QDialog>

#include "ui_dialograndlattice.h"

class DialogRandLattice : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandLattice(QWidget *parent = Q_NULLPTR);

signals:
    void userChoices( const int &nodes,
                      const int &length,
                      const int &dimension,
                      const int &neighLength,
                      const QString &mode,
                      const bool &diag);

public slots:
//    void checkErrors(const int &i);
    void getUserChoices();
    void circularChanged(const bool &toggle);
    void lengthChanged(int l);
//    void setModeDirected();
//    void setModeUndirected();
//    void setDiag();
//    void modifyDegree(int value);

private:

    Ui::DialogRandLattice ui;
    int nodes;
    int length;
    int dimension;
    int neighLength;
    QString mode;
    bool circular;
};

#endif
