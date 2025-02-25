/**
 * @file dialogdatasetselect.h
 * @brief Declares the DialogDatasetSelect class for selecting datasets in SocNetV.
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

#ifndef DIALOGDATASETSELECT_H
#define DIALOGDATASETSELECT_H

#include <QDialog>
#include "ui_dialogdatasetselect.h"


class DialogDataSetSelect: public QDialog
{
    Q_OBJECT
public:
    DialogDataSetSelect (QWidget *parent = Q_NULLPTR);
    ~DialogDataSetSelect();
public slots:
    void getUserChoices();
signals:
    void userChoices(QString);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::DialogDataSetSelect *ui;
    QStringList datasets_list, datasets_filenames;

};



#endif
