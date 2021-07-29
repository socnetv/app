/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0
 Written in Qt
 
                         dialogedgedichotomization.h  -  description
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

#ifndef DIALOGEDGEDICHOTOMIZATION_H
#define DIALOGEDGEDICHOTOMIZATION_H

#include <QDialog>

#include "ui_dialogedgedichotomization.h"


class DialogEdgeDichotomization : public QDialog
{
    Q_OBJECT
public:
    explicit DialogEdgeDichotomization (QWidget *parent = Q_NULLPTR);
public slots:
    void getUserChoices ();
signals:
    void userChoices( qreal threshold);
private:
    Ui::DialogEdgeDichotomization ui;

};


#endif 
