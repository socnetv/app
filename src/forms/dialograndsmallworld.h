/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                         dialograndsmallworld.h  -  description
                             -------------------
    copyright            : (C) 2005-2023 by Dimitris B. Kalamaras
    project site         : https://socnetv.org

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
    explicit DialogRandSmallWorld(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors();
    void getUserChoices();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();
    void modifyDegree(int value);

signals:
    void userChoices( const int nodes,
                      const int degree,
                      const qreal prob,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, degree;
    qreal bprob;
    bool diag;
    Ui::DialogRandSmallWorld *ui;

};

#endif
