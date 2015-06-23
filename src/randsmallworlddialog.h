/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                         randsmallworlddialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
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

#ifndef RANDSMALLWORLDDIALOG_H
#define RANDSMALLWORLDDIALOG_H

#include <QDialog>

#include "ui_randsmallworlddialog.h"

class RandSmallWorldDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RandSmallWorldDialog(QWidget *parent = 0);

public slots:
    void checkErrors();
    void gatherData();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int nodes,
                      const int degree,
                      const float prob,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, degree;
    float bprob;
    bool diag;
    Ui::RandSmallWorldDialog ui;

};

#endif // RANDSMALLWORLDDIALOG_H
