/**
 * @file dialogfilterednodes.h
 * @brief Defines the DialogFilterNodesByCentrality class for filtering nodes based on some attribute.
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

#ifndef DIALOGFILTERNODESBYCENTRALITY_H
#define DIALOGFILTERNODESBYCENTRALITY_H


#include <QDialog>

#include "ui_dialogfilternodesbycentrality.h"


class DialogFilterNodesByCentrality : public QDialog
{
    Q_OBJECT
public:
    explicit DialogFilterNodesByCentrality (QWidget *parent = Q_NULLPTR);
public slots:
    void getUserChoices ();
signals:
    void userChoices(const qreal, const bool);
private:
    Ui::DialogFilterNodesByCentrality ui;

};


#endif
