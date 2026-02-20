/**
 * @file graph_prominence_distribution.cpp
 * @brief Implements visualization routines (bars, spline, area charts)
 *  for prominence index distribution for the Graph class.
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

#include "graph.h"

#include <QDebug>
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QFont>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>

void Graph::uiProminenceDistributionSpline(const QVector<QPair<qreal, qreal>> &points,
                                           qreal min, qreal max,
                                           qreal minF, qreal maxF,
                                           const QString &seriesName,
                                           const QString &distImageFileName)
{
    qDebug() << "Computing prominence distribution as spline chart...";

    auto *series = new QLineSeries();
    series->setName(seriesName);

    auto *axisX = new QValueAxis();
    auto *axisY = new QValueAxis();

    // Used only for large chart export
    auto *series1 = new QLineSeries();
    series1->setName(seriesName);
    auto *axisX1 = new QValueAxis();
    auto *axisY1 = new QValueAxis();

    for (const auto &pt : points)
    {
        series->append(pt.first, pt.second);
        series1->append(pt.first, pt.second);
    }

    axisX->setMin(min);
    axisX->setMax(max);
    axisY->setMin(minF);
    axisY->setMax(maxF + 1.0);

    QPen sPen(QColor("#209fdf"));
    sPen.setWidthF(0.9);
    QBrush sBrush(QColor("#ff0000"));
    series->setBrush(sBrush);
    series->setPen(sPen);

    if (!distImageFileName.isEmpty())
    {
        qDebug() << "saving prominence distribution image to" << distImageFileName;

        axisX1->setMin(min);
        axisX1->setMax(max);
        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        auto *chart = new QChart();
        auto *chartView = new QChartView(chart);

        chart->addSeries(series1);
        chart->setTitle(series1->name() + " distribution");
        chart->setTitleFont(QFont("Times", 12));
        chart->legend()->hide();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();
        p.save(distImageFileName, "PNG");

        chartView->hide();
        delete chartView; // matches your old “don’t delete axes too early” rationale
    }

    qDebug() << "emitting signal to MW update the prominence distribution spline chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}

void Graph::uiProminenceDistributionArea(const QVector<QPair<qreal, qreal>> &points,
                                         const qreal min,
                                         const qreal max,
                                         const qreal minF,
                                         const qreal maxF,
                                         const QString &name,
                                         const QString &distImageFileName)
{
    QAreaSeries *series = new QAreaSeries();
    series->setName(name);

    QLineSeries *upperSeries = new QLineSeries();
    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();

    // Used only for the large chart exported to PNG for the HTML report
    QAreaSeries *series1 = new QAreaSeries();
    series1->setName(name);
    QValueAxis *axisX1 = new QValueAxis();
    QValueAxis *axisY1 = new QValueAxis();

    // Fill series points (same semantics: upperSeries holds the curve)
    for (const auto &pt : points)
    {
        upperSeries->append(pt.first, pt.second);
    }

    axisX->setMin(min);
    axisX->setMax(max);

    axisY->setMin(minF);
    axisY->setMax(maxF + 1.0);

    series->setUpperSeries(upperSeries);

    QPen sPen(QColor("#666"));
    sPen.setWidthF(0.2);
    QBrush sBrush(QColor("#209fdf"));
    series->setBrush(sBrush);
    series->setPen(sPen);

    if (!distImageFileName.isEmpty())
    {
        qDebug() << "saving distribution image to" << distImageFileName;

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        // Keep export behavior identical: export uses its own chart/axes,
        // but reuses the computed upperSeries data semantics.
        // IMPORTANT: attach the same upperSeries instance to series1
        // to preserve output; if you prefer deep-copy, we can clone points.
        series1->setUpperSeries(upperSeries);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView(chart);

        chart->addSeries(series1);

        chart->setTitle(series->name() + " distribution");
        chart->setTitleFont(QFont("Times", 12));
        chart->legend()->hide();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);

        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();
        p.save(distImageFileName, "PNG");

        chartView->hide();
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution area chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}

void Graph::uiProminenceDistributionBars(const QStringList &categories,
                                         const QVector<qreal> &frequencies,
                                         const qreal min,
                                         const qreal max,
                                         const qreal minF,
                                         const qreal maxF,
                                         const QString &name,
                                         const QString &distImageFileName)
{
    qDebug() << "Computing prominence distribution as bar chart (UI layer)...";

    auto *series = new QBarSeries();
    series->setName(name);

    auto *barSet = new QBarSet("");
    auto *axisY = new QValueAxis();
    auto *axisX = new QBarCategoryAxis();

    // Used only for large chart export (keep identical behavior)
    auto *series1 = new QBarSeries();
    series1->setName(name);

    auto *barSet1 = new QBarSet("");
    auto *axisY1 = new QValueAxis();
    auto *axisX1 = new QBarCategoryAxis();

    // Fill categories + bars
    // Old code appended categories to axisX and values to barSet in lockstep.
    axisX->append(categories);
    for (const auto &f : frequencies)
    {
        barSet->append(f);
    }

    // axis ranges (same semantics as old)
    axisX->setMin(QString::number(min, 'f', 6));
    axisX->setMax(QString::number(max, 'f', 6));
    axisY->setMin(minF);
    axisY->setMax(maxF + 1.0);

    series->append(barSet);

    QPen sPen(QColor("#666"));
    sPen.setWidthF(0.2);
    QBrush sBrush(QColor("#209fdf"));
    barSet->setBrush(sBrush);
    barSet->setPen(sPen);

    if (!distImageFileName.isEmpty())
    {
        qDebug() << "saving distribution image to" << distImageFileName;

        // Export copies (old code only built these when filename non-empty)
        axisX1->append(categories);
        for (const auto &f : frequencies)
        {
            barSet1->append(f);
        }
        series1->append(barSet1);

        axisX1->setMin(QString::number(min, 'f', 6));
        axisX1->setMax(QString::number(max, 'f', 6));
        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        auto *chart = new QChart();
        auto *chartView = new QChartView(chart);

        chart->addSeries(series1);
        chart->setTitle(series->name() + " distribution");
        chart->setTitleFont(QFont("Times", 12));
        chart->legend()->hide();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);

        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();
        p.save(distImageFileName, "PNG");

        chartView->hide();
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution bar chart";
    emit signalPromininenceDistributionChartUpdate(series,
                                                   axisX, min, max,
                                                   axisY, minF, maxF);
}