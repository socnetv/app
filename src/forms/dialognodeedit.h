/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         dialognodeedit.h  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
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


#ifndef DIALOGNODEEDIT_H
#define DIALOGNODEEDIT_H

#include <QDialog>


namespace Ui {
class DialogNodeEdit;
}

class DialogNodeEdit : public QDialog
{
    Q_OBJECT
public:
    explicit DialogNodeEdit(QWidget *parent = Q_NULLPTR,
                            const QString &label = "",
                            const int &size = 8,
                            const QColor &color= QColor("red"),
                            const QString &shape = "circle",
                            const QString &path=QString::null);
    ~DialogNodeEdit();
public slots:
    void checkErrors ();
    void getNodeShape(const int &nodeShapeIndex);
    void getUserChoices ();
    void selectColor();
signals:
    void userChoices( const QString &label,
                      const int &size,
                      const QString &value,
                      const QColor &color,
                      const QString &shape,
                      const QString &iconPath=QString::null);
    void nodeEditDialogError(QString);

private:
    QColor nodeColor;
    QString nodeShape;
    QString iconPath;
    QString nodeValue;
    QString nodeLabel;
    int nodeSize;
    QPixmap pixmap;
    Ui::DialogNodeEdit *ui;
};

#endif
