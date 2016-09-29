/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                         randregulardialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net

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

#ifndef RANDREGULARDIALOG_H
#define RANDREGULARDIALOG_H

#include <QDialog>

#include "ui_randregulardialog.h"

class RandRegularDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RandRegularDialog(QWidget *parent = 0);

public slots:
    void checkErrors(const int &i);
    void gatherData();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();
    void modifyDegree(int value);

signals:
    void userChoices( const int nodes,
                      const int degree,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, degree;
    bool diag;
    Ui::RandRegularDialog ui;

};

#endif // RANDREGULARDIALOG_H
