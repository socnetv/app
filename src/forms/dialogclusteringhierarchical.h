/**
 * @file dialogclusteringhierarchical.h
 * @brief Defines the DialogClusteringHierarchical class for hierarchical clustering settings in the social network visualizer.
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

#ifndef DIALOGCLUSTERINGHIERARCHICAL_H
#define DIALOGCLUSTERINGHIERARCHICAL_H

#include <QDialog>
#include "ui_dialogclusteringhierarchical.h"


class DialogClusteringHierarchical: public QDialog
{
    Q_OBJECT
public:
    DialogClusteringHierarchical (QWidget *parent = Q_NULLPTR, QString preselectMatrix = "");
    ~DialogClusteringHierarchical();
public slots:
    void getUserChoices();
signals:
    void userChoices(const QString &matrix,
                     const QString &varLocation,
                     const QString &similarityMeasure,
                     const QString &linkageCriterion,
                     const bool &diagonal,
                     const bool &diagram);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void matrixChanged(const QString &matrix);
private:
    Ui::DialogClusteringHierarchical ui;
    QStringList matrixList, measureList, linkageList, variablesLocationList;

};



#endif
