/**
 * @file dialogfilteredgesbyweight.h
 * @brief Defines the DialogFilterEdgesByWeight class for filtering edges based on weight in the social network visualizer.
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

#ifndef DIALOGFILTEREDGESBYWEIGHT_H
#define DIALOGFILTEREDGESBYWEIGHT_H


#include <QDialog>

#include "ui_dialogfilteredgesbyweight.h"


class DialogFilterEdgesByWeight : public QDialog
{
    Q_OBJECT
public:
    explicit DialogFilterEdgesByWeight (QWidget *parent = Q_NULLPTR);
public slots:
    void getUserChoices ();
signals:
    void userChoices(const qreal, const bool);
private:
    Ui::DialogFilterEdgesByWeight ui;

};


#endif 
