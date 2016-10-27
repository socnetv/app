/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                         settingsdialog.h  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
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
    ~SettingsDialog();

public slots:
    void getDataDir();
    void getBgColor();
    void getBgImage();
    void validateSettings();
    void getNodeColor();
    void getNodeShape();
    void getNodeSize(int);
    void getNodeNumbersVisibility (bool toggle);
    void getNodeNumbersInside(bool toggle);
    void getNodeNumberColor();
    void getNodeNumberSize(const int);
    void getNodeNumberDistance(const int);
    void getNodeLabelsVisibility (bool toggle);
    void getNodeLabelColor();
    void getNodeLabelSize(const int);
    void getNodeLabelDistance(const int);
    void getEdgesVisibility (const bool &toggle);
    void getEdgeArrowsVisibility (const bool &toggle);
    void getEdgeColor();
    void getEdgeColorNegative();
    void getEdgeShape();
    void getEdgeWeightNumbersVisibility(const bool &toggle);
    void getEdgeLabelsVisibility(const bool &toggle);

signals:
    void setProgressBars(bool);
    void setToolBar(bool);
    void setStatusBar(bool);
    void setAntialiasing(bool);
    void setPrintLogo(bool);
    void setDebugMsgs(bool);
    void setBgColor(const QColor);
    void setBgImage();
    void setRightPanel(bool);
    void setLeftPanel(bool);
    void setNodeColor(QColor);
    void setNodeShape(const QString, const long int);
    void setNodeSize(int, const bool &);
    void setNodeNumbersVisibility(bool);
    void setNodeNumbersInside(bool);
    void setNodeNumberSize(const int v, const int &size, const bool prompt);
    void setNodeNumberDistance(const int v, const int &);
    void setNodeNumberColor(const QColor);
    void setNodeLabelsVisibility(const bool &);
    void setNodeLabelColor(const QColor);
    void setNodeLabelSize(const int v, const int &);
    void setNodeLabelDistance(const int v, const int &);
    void setEdgesVisibility (const bool &toggle);
    void setEdgeArrowsVisibility (const bool &toggle);
    void setEdgeColor(const QColor, const int &);
    void setEdgeShape(const QString, const long int);
    void setEdgeWeightNumbersVisibility(const bool &toggle);
    void setEdgeLabelsVisibility(const bool &toggle);
    void saveSettings();
private:
     QMap<QString, QString> &m_appSettings ;
     Ui::SettingsDialog *ui;
     QPixmap m_pixmap;
     //QString m_nodeShape;
     QColor m_bgColor, m_nodeColor, m_nodeNumberColor, m_nodeLabelColor;
     QColor m_edgeColor, m_edgeColorNegative, m_edgeWeightNumberColor;
};

#endif // SETTINGSDIALOG_H
