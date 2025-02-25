/**
 * @file dialogdissimilarities.h
 * @brief Declares the DialogDissimilarities class for managing dissimilarity calculations in SocNetV.
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

#ifndef DIALOGDISSIMILARITIES_H
#define DIALOGDISSIMILARITIES_H

#include <QDialog>
#include "ui_dialogdissimilarities.h"
 

class DialogDissimilarities: public QDialog
{
	Q_OBJECT
public:
    DialogDissimilarities (QWidget *parent = Q_NULLPTR);
    ~DialogDissimilarities();
public slots:
	void getUserChoices();
signals:
    void userChoices(const QString &metric,
                     const QString &varLocation,
                     const bool &diagonal);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::DialogDissimilarities ui;
    QStringList variablesLocationList, metricList;

};



#endif
