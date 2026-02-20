/**
 * @file dialognodefind.h
 * @brief Declares the DialogNodeFind class used for locating and highlighting nodes by name or ID in the SocNetV network visualization.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


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
    explicit DialogNodeFind(QWidget *parent = Q_NULLPTR, QStringList indexList=QStringList());
    ~DialogNodeFind();
public slots:
    void setError(const bool &toggle);
    void getIndex(const QString &indexStr);
    void checkErrors ();
    void getUserChoices ();
signals:
    void userChoices( const QStringList &list,
                      const QString &type,
                      const QString &selectedIndex=QString());
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
