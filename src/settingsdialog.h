/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.0
 Written in Qt

                         settingsdialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QMap<QString, QString> &appSettings, QWidget *parent = 0 );
    void getDataDir();
    void getBgColor();
    void getBgImage();
    void validateSettings();
    ~SettingsDialog();

signals:
    void setProgressBars(bool);
    void setToolBar(bool);
    void setStatusBar(bool);
    void setAntialiasing(bool);
    void setDebugMsgs(bool);
    void setBgColor(QColor);
    void setBgImage();
    void setRightPanel(bool);
    void setLeftPanel(bool);
    void saveSettings();
private:
     QMap<QString, QString> &m_appSettings ;
     Ui::SettingsDialog *ui;
     QPixmap m_pixmap;
     QColor m_bgColor;
};

#endif // SETTINGSDIALOG_H
