/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.9-dev
 Written in Qt

                         dialograndregular.h  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
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

#ifndef DIALOGRANDREGULAR_H
#define DIALOGRANDREGULAR_H

#include <QDialog>

#include "ui_dialograndregular.h"

class DialogRandRegular : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandRegular(QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors(const int &i);
    void getUserChoices();
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
    Ui::DialogRandRegular ui;

};

#endif
