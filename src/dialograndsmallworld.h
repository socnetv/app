/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                         dialograndsmallworld.h  -  description
                             -------------------
    copyright            : (C) 2005-2017 by Dimitris B. Kalamaras
    project site         : http://socnetv.org

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

#ifndef DIALOGRANDSMALLWORLD_H
#define DIALOGRANDSMALLWORLD_H

#include <QDialog>

#include "ui_dialograndsmallworld.h"

class DialogRandSmallWorld : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandSmallWorld(QWidget *parent = 0);

public slots:
    void checkErrors();
    void gatherData();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();
    void modifyDegree(int value);

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
    Ui::DialogRandSmallWorld *ui;

};

#endif
