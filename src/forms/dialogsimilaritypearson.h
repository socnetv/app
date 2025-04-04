/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt
 
                         dialogsimilaritypearson.h  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

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
