/**
 * @file dialogsimilaritypearson.h
 * @brief Declares the DialogSimilarityPearson class for calculating Pearson similarity between nodes in SocNetV.
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

#ifndef DIALOGSIMILARITYPEARSON_H
#define DIALOGSIMILARITYPEARSON_H

#include <QDialog>
#include "ui_dialogsimilaritypearson.h"


class DialogSimilarityPearson: public QDialog
{
    Q_OBJECT
public:
    DialogSimilarityPearson (QWidget *parent = Q_NULLPTR);
    ~DialogSimilarityPearson();
public slots:
    void getUserChoices();
signals:
    void userChoices(const QString &matrix,
                     const QString &varLocation,
                     const bool &diagonal);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
private:
    Ui::DialogSimilarityPearson ui;
    QStringList matrixList, variablesLocationList;

};



#endif
