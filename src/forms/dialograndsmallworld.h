/**
 * @file dialograndsmallworld.h
 * @brief Declares the DialogRandSmallWorld class for generating small-world networks based on the Watts-Strogatz model in SocNetV.
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


#ifndef DIALOGRANDSMALLWORLD_H
#define DIALOGRANDSMALLWORLD_H

#include <QDialog>

#include "ui_dialograndsmallworld.h"

class DialogRandSmallWorld : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandSmallWorld(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors();
    void getUserChoices();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();
    void modifyDegree(int value);

signals:
    void userChoices( const int nodes,
                      const int degree,
                      const qreal prob,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, degree;
    qreal bprob;
    bool diag;
    Ui::DialogRandSmallWorld *ui;

};

#endif
