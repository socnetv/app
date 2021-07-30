/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.3
 Written in Qt
 
                         dialogdissimilarities.h  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
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
