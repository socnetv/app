/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.3
 Written in Qt
 
                         dialogdissimilarities.h  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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
    DialogDissimilarities (QWidget *parent = 0);
    ~DialogDissimilarities();
public slots:
	void gatherData();
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
