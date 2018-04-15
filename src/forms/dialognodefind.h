/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialognodefind.h  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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

#ifndef DIALOGNODEFIND_H
#define DIALOGNODEFIND_H

#include <QDialog>

namespace Ui {
class DialogNodeFind;
}

class DialogNodeFind : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNodeFind(QWidget *parent = 0, QStringList indexList=QStringList());
    ~DialogNodeFind();
public slots:
    void setError(const bool &toggle);
    void getIndex(const QString &indexStr);
    void checkErrors ();
    void getUserChoices ();
signals:
    void userChoices( const QStringList &list,
                      const QString &type,
                      const QString &selectedIndex=QString::null);
private:
    Ui::DialogNodeFind *ui;
    QStringList list;
    QString searchType;
    QStringList tempListA;
    QStringList tempListB;
    QString str;
    QString selectedIndex;
};

#endif // DIALOGNODEFIND_H
