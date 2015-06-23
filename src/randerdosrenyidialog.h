/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt

                         randerdosrenyidialog.h  -  description
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

#ifndef RANDERDOSRENYIDIALOG_H
#define RANDERDOSRENYIDIALOG_H

#include <QDialog>

#include "ui_randerdosrenyidialog.h"

class RandErdosRenyiDialog  : public QDialog
{
    Q_OBJECT
public:
    explicit RandErdosRenyiDialog(QWidget *parent=0);

public slots:
    void checkErrors();
    void gatherData();
    void gnmModel();
    void gnpModel();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int nodes,
                      const QString model,
                      const int edges,
                      const float eprob,
                      const QString mode,
                      const bool diag);
private:
    QString model;
    QString mode;
    int nodes, edges;
    float eprob;
    bool diag;
    Ui::RandErdosRenyiDialog ui;
};

#endif // RANDERDOSRENYIDIALOG_H
