/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt

                         dialograndlattice.h  -  description
                             -------------------
    copyright            : (C) 2005-2023 by Dimitris B. Kalamaras
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


#ifndef DIALOGRANDLATTICE_H
#define DIALOGRANDLATTICE_H

#include <QDialog>

#include "ui_dialograndlattice.h"

class DialogRandLattice : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRandLattice(QWidget *parent = Q_NULLPTR);

signals:
    void userChoices( const int &nodes,
                      const int &length,
                      const int &dimension,
                      const int &neighLength,
                      const QString &mode,
                      const bool &diag);

public slots:
//    void checkErrors(const int &i);
    void getUserChoices();
    void circularChanged(const bool &toggle);
    void lengthChanged(int l);
//    void setModeDirected();
//    void setModeUndirected();
//    void setDiag();
//    void modifyDegree(int value);

private:

    Ui::DialogRandLattice ui;
    int nodes;
    int length;
    int dimension;
    int neighLength;
    QString mode;
    bool circular;
};

#endif
