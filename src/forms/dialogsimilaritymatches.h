/**
 * @file dialogsimilaritymatches.h
 * @brief Declares the DialogSimilarityMatches class for computing node similarity based on matching coefficients in SocNetV.
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

#ifndef DIALOGSIMILARITYMATCHES_H
#define DIALOGSIMILARITYMATCHES_H

#include <QDialog>
#include "ui_dialogsimilaritymatches.h"


class DialogSimilarityMatches: public QDialog
{
    Q_OBJECT
public:
    DialogSimilarityMatches (QWidget *parent = Q_NULLPTR);
    ~DialogSimilarityMatches();
public slots:
    void getUserChoices();
signals:
    void userChoices(const QString &matrix,
                     const QString &varLocation,
                     const QString &method,
                     const bool &diagonal);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
private:
    Ui::DialogSimilarityMatches ui;
    QStringList matrixList, variablesLocationList, measureList;

};



#endif
