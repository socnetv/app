/**
 * @file dialogsysteminfo.h
 * @brief Declares the DialogSystemInfo class for displaying system and environment information in SocNetV.
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


#ifndef DIALOGSYSTEMINFO_H
#define DIALOGSYSTEMINFO_H

#include <QDialog>
#include "ui_dialogsysteminfo.h"

class DialogSystemInfo: public QDialog
{
    Q_OBJECT
public:
    explicit DialogSystemInfo (QWidget *parent = Q_NULLPTR);

private:
    Ui::DialogSystemInfo *ui;
};



#endif
