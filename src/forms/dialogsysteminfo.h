/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.9
 Written in Qt
 
                         dialogsysteminfo.h  -  description
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

#ifndef DIALOGSYSTEMINFO_H
#define DIALOGSYSTEMINFO_H

#include <QDialog>
#include "ui_dialogsysteminfo.h"

class DialogSystemInfo: public QDialog
{
    Q_OBJECT
public:
    explicit DialogSystemInfo (QWidget *parent = Q_NULLPTR);

private:
    Ui::DialogSystemInfo *ui;
};



#endif
