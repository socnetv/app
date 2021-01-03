/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.9-dev
 Written in Qt

                         randscalefreeddialog.h  -  description
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

#ifndef DIALOGRANDSCALEFREE_H
#define DIALOGRANDSCALEFREE_H

#include <QDialog>

#include "ui_dialograndscalefree.h"

class DialogRandScaleFree : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandScaleFree(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors();
    void getUserChoices();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int &nodes,
                      const int &power,
                      const int &initialNodes,
                      const int &edgesPerStep,
                      const qreal &zeroAppeal,
                      const QString &mode);
private:
    QString mode;
    int nodes; // n
    int initialNodes; // m0
    int edgesPerStep; //m
    int power;
    qreal zeroAppeal; // a
    bool diag;
    Ui::DialogRandScaleFree ui;

};

#endif
