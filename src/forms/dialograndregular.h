/**
 * @file dialograndregular.h
 * @brief Declares the DialogRandRegular class for generating random regular graphs in SocNetV.
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


#ifndef DIALOGRANDREGULAR_H
#define DIALOGRANDREGULAR_H

#include <QDialog>

#include "ui_dialograndregular.h"

class DialogRandRegular : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandRegular(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors(const int &i);
    void getUserChoices();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();
    void modifyDegree(int value);

signals:
    void userChoices( const int nodes,
                      const int degree,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, degree;
    bool diag;
    Ui::DialogRandRegular ui;

};

#endif
