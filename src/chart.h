/**
 * @file chart.h
 * @brief Declares the Chart class for generating and displaying statistical charts in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2024 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#ifndef CHART_H
#define CHART_H

#include <QObject>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>


class QSplineSeries;
class QChart;
class QAbstractAxis;

class Chart : public QChartView
{
    Q_OBJECT
public:
    explicit Chart (QWidget *parent = Q_NULLPTR );
    //explicit Chart ( QChart *ch = Q_NULLPTR, QWidget *parent = Q_NULLPTR );
    ~Chart();

    void setTitle(const QString &title = QString(), const QFont &font=QFont());
    void addSeries(QAbstractSeries *series = Q_NULLPTR );
    void appendToSeries (const QPointF &p);
    void removeAllSeries();

    void createDefaultAxes();
    QList<QAbstractAxis*> axes(Qt::Orientations orientation = Qt::Horizontal|Qt::Vertical,
                               QAbstractSeries *series = Q_NULLPTR) const;
    void removeAllAxes();

    void addAxis(QAbstractAxis *axis, Qt::Alignment alignment);

    void setAxisX(QAbstractAxis *axis, QAbstractSeries *series = Q_NULLPTR);
    void setAxisY(QAbstractAxis *axis, QAbstractSeries *series = Q_NULLPTR);

    void setAxisXRange(const QVariant &min, const QVariant &max);
    void setAxisXMin(const QVariant &min);
    void setAxisYRange(const QVariant &min, const QVariant &max);
    void setAxisYMin(const QVariant &min);

    void setAxesThemeDefault ();

    void setAxisXLabelsAngle (const int &angle);

    void setAxisXLabelFont(const QFont &font=QFont("Helvetica", 6 ));
    void setAxisYLabelFont(const QFont &font=QFont("Helvetica", 6 ));

    void setAxisXLinePen(const QPen &pen = QPen(QColor("#d0d0d0"), 1,Qt::SolidLine) );
    void setAxisYLinePen(const QPen &pen = QPen(QColor("#d0d0d0"), 1,Qt::SolidLine) );

    void setAxisXGridLinePen(const QPen &pen = QPen(QColor("#e0e0e0"), 1,Qt::DotLine) );
    void setAxisYGridLinePen(const QPen &pen = QPen(QColor("#e0e0e0"), 1,Qt::DotLine) );


    void setTheme(QChart::ChartTheme theme=QChart::ChartThemeLight);
    void setThemeSmallWidget(const int minWidth, const int minHeight);

    void setChartBackgroundBrush(const QBrush & brush = QBrush(Qt::transparent));
    void setChartBackgroundPen(const QPen & pen = QPen(Qt::transparent));
    void setMargins(const QMargins &margins = QMargins());
    void toggleLegend(const bool &toggle = false);

    void resetToTrivial();

private:
    QChart *m_chart;
    QSplineSeries *m_series;
};

#endif // CHART_H
