/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         datasetselectdialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#ifndef DATASETSELECTDIALOG_H
#define DATASETSELECTDIALOG_H

#include <QDialog>
#include "ui_datasetselectdialog.h"
 

class DataSetSelectDialog: public QDialog
{
	Q_OBJECT
public:
	DataSetSelectDialog (QWidget *parent = 0);
    ~DataSetSelectDialog();
public slots:
	void gatherData();
signals:
	void userChoices(QString);	
private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::DataSetSelectDialog ui;
    QStringList datasets_list, datasets_filenames;

};



#endif
