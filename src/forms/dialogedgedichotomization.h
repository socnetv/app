/**
 * @file dialogedgedichotomization.h
 * @brief Declares the DialogEdgeDichotomization class for managing edge dichotomization in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#ifndef DIALOGEDGEDICHOTOMIZATION_H
#define DIALOGEDGEDICHOTOMIZATION_H

#include <QDialog>

#include "ui_dialogedgedichotomization.h"


class DialogEdgeDichotomization : public QDialog
{
    Q_OBJECT
public:
    explicit DialogEdgeDichotomization (QWidget *parent = Q_NULLPTR);
public slots:
    void getUserChoices ();
signals:
    void userChoices( qreal threshold);
private:
    Ui::DialogEdgeDichotomization ui;

};


#endif 
