/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0-dev
 Written in Qt

                         dialogsettings.h  -  description
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

#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>
#include <QMap>



namespace Ui {
 class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QMap<QString, QString> &appSettings,
                            const QStringList &nodeShapeList,
                            const QStringList &iconPathList,
                            QWidget *parent = Q_NULLPTR );
    ~DialogSettings();

public slots:
    void getDataDir();

    void getReportsRealNumberPrecision(const int &precision);
    void getReportsLabelsLength(const int &length);
    void getReportsChartType(const int &type);

    void getCanvasBgColor();
    void getCanvasBgImage();
    void getCanvasUpdateMode(const QString &text);
    void getCanvasIndexMethod(const QString &text);
    void validateSettings();
    void getNodeColor();
    void getNodeShapeIndex(const int &shape);
    void getNodeIconFile();
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
    void getEdgeColorZero();
    void getEdgeShape();
    void getEdgeOffsetFromNode( int offset);
    void getEdgeWeightNumbersVisibility(const bool &toggle);
    void getEdgeLabelsVisibility(const bool &toggle);

signals:
    void setReportsDataDir (const QString &dir);

    void setReportsRealNumberPrecision(const int &precision);
    void setReportsLabelLength(const int &length);
    void setReportsChartType(const int &type);

    void setStyleSheetDefault(const bool &toggle);

    void setProgressDialog(bool);
    void setToolBar(bool);
    void setStatusBar(bool);
    void setPrintLogo(bool);
    void setDebugMsgs(bool);
    void setRightPanel(bool);
    void setLeftPanel(bool);

    void setCanvasBgColor(const QColor);
    void setCanvasBgImage();
    void setCanvasOpenGL(const bool &);
    void setCanvasAntialiasing(const bool &);
    void setCanvasAntialiasingAutoAdjust(const bool &);
    void setCanvasSmoothPixmapTransform(bool);
    void setCanvasSavePainterState(bool);
    void setCanvasCacheBackground(bool);
    void setCanvasEdgeHighlighting(bool);
    void setCanvasUpdateMode(const QString &text);
    void setCanvasIndexMethod(const QString &text);

    void setNodeColor(QColor);
    void setNodeShape(const int &num, QString , QString nodeIconPath=QString());
    void setNodeSize(int, const bool &);
    void setNodeNumbersVisibility(bool);
    void setNodeNumbersInside(bool);
    void setNodeNumberSize(const int v, const int &size, const bool prompt);
    void setNodeNumberDistance(const int v, const int &);
    void setNodeNumberColor(const int &v, const QColor);
    void setNodeLabelsVisibility(const bool &);
    void setNodeLabelColor(const QColor);
    void setNodeLabelSize(const int v, const int &);
    void setNodeLabelDistance(const int v, const int &);

    void setEdgesVisibility (const bool &toggle);
    void setEdgeArrowsVisibility (const bool &toggle);
    void setEdgeColor(const QColor, const int &);
    void setEdgeShape(const QString, const long int);
    void setEdgeOffsetFromNode(const int&offset, const int &v1=0, const int &v2=0);
    void setEdgeWeightNumbersVisibility(const bool &toggle);
    void setEdgeLabelsVisibility(const bool &toggle);
    void saveSettings();
private:
     QMap<QString, QString> &m_appSettings ;
     QPixmap m_pixmap;
     //QString m_nodeShape;
     QColor m_bgColor, m_nodeColor, m_nodeNumberColor, m_nodeLabelColor;
     QColor m_edgeColor, m_edgeColorNegative,m_edgeColorZero, m_edgeWeightNumberColor;
     QStringList m_shapeList;
     QStringList m_iconList;
     Ui::DialogSettings *ui;
};

#endif
