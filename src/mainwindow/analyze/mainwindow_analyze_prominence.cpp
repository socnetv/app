/**
 * @file mainwindow_analyze_prominence.cpp
 * @brief Implements MainWindow Analyze menu prominence distribution chart update and its toolbox combo box handler.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
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

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "texteditor.h"
#include "chart.h"
#include "widgets/graphtablewidget.h"
#include "forms/dialogsimilaritypearson.h"
#include "forms/dialogsimilaritymatches.h"
#include "forms/dialogdissimilarities.h"
#include "forms/dialogclusteringhierarchical.h"

#include <QtWidgets>
#include <QtCharts>

/**
 * @brief Called when user selects something in the Prominence selectbox
 *  of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisProminenceSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected prominence analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotAnalyzeCentralityDegree();
        break;
    case 2:
        slotAnalyzeCentralityCloseness();
        break;
    case 3:
        slotAnalyzeCentralityClosenessIR();
        break;
    case 4:
        slotAnalyzeCentralityBetweenness();
        break;
    case 5:
        slotAnalyzeCentralityStress();
        break;
    case 6:
        slotAnalyzeCentralityEccentricity();
        break;
    case 7:
        slotAnalyzeCentralityPower();
        break;
    case 8:
        slotAnalyzeCentralityInformation();
        break;
    case 9:
        slotAnalyzeCentralityEigenvector();
        break;
    case 10:
        slotAnalyzeCentralityKatz();
        break;
    case 11:
        slotAnalyzeCentralityBonacich();
        break;
    case 12:
        slotAnalyzePrestigeDegree();
        break;
    case 13:
        slotAnalyzePrestigePageRank();
        break;
    case 14:
        slotAnalyzePrestigeProximity();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Updates the prominence distribution miniChart
 * Called from Graph after computing the prominence index distribution.
 * @param series
 * @param axisX
 * @param min
 * @param max
 */
void MainWindow::slotAnalyzeProminenceDistributionChartUpdate(QAbstractSeries *series,
                                                              QAbstractAxis *axisX,
                                                              const qreal &min,
                                                              const qreal &max,
                                                              QAbstractAxis *axisY,
                                                              const qreal &minF,
                                                              const qreal &maxF)
{

    Q_UNUSED(minF);

    qCDebug(lcMainWindow) << "Updating the prominence distribution miniChart";

    if (series == Q_NULLPTR)
    {
        qCDebug(lcMainWindow) << "series is null! Resetting to trivial";
        miniChart->resetToTrivial();
        return;
    }

    // Set the style of the lines and bars
    switch (series->type())
    {
    case QAbstractSeries::SeriesTypeBar:
        qCDebug(lcMainWindow) << "this an BarSeries";
        break;
    case QAbstractSeries::SeriesTypeArea:
        qCDebug(lcMainWindow) << "this an AreaSeries";

        break;
    default:
        break;
    }

    // Clear miniChart from old series.
    miniChart->removeAllSeries();

    // Remove all axes
    miniChart->removeAllAxes();

    // Add series to miniChart
    miniChart->addSeries(series);

    // Set Chart title and remove legend
    miniChart->setTitle(series->name() + QString(" distribution"), QFont("Times", 8));

    miniChart->toggleLegend(false);

    QString chartHelpMsg = tr("Distribution of %1 values:\n"
                              "Min value: %2 \n"
                              "Max value: %3 \n"
                              "Please note that, due to the small size of this widget, \n"
                              "if you display a distribution in Bar Chart where there are \n"
                              "more than 10 values, the widget will not show all bars. \n"
                              "In this case, use Line or Area Chart (from Settings). \n"
                              "In any case, the large chart in the HTML report \n"
                              "is better than this widget...")
                               .arg(series->name())
                               .arg(min, 0, 'g', appSettings["initReportsRealNumberPrecision"].toInt(0, 10))
                               .arg(max, 0, 'g', appSettings["initReportsRealNumberPrecision"].toInt(0, 10));

    miniChart->setToolTip(chartHelpMsg);

    miniChart->setWhatsThis(chartHelpMsg);

    // if true, then bar chart appears with default X axis (1,2,3 ...)
    bool useDefaultAxes = false;

    if (!useDefaultAxes)
    {
        if (axisX != Q_NULLPTR)
        {
            qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisX not null. Setting it to miniChart";
            miniChart->setAxisX(axisX, series);

            miniChart->setAxisXMin(0);
            miniChart->setAxisXLabelFont();
            miniChart->setAxisXLinePen();
            miniChart->setAxisXGridLinePen();
            miniChart->setAxisXLabelsAngle(-90);
        }
        if (axisY != Q_NULLPTR)
        {
            qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisY not null. Setting it to miniChart";
            miniChart->setAxisY(axisY, series);
            miniChart->setAxisYMin(0);
            miniChart->setAxisYLabelFont();
            miniChart->setAxisYLinePen();
            miniChart->setAxisYGridLinePen();
        }
    }

    if ((axisX == Q_NULLPTR && axisY == Q_NULLPTR) || useDefaultAxes)
    {

        qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                    "axisX and axisY null. Calling createDefaultAxes()";
        miniChart->createDefaultAxes();

        qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - setting axis min";
        miniChart->setAxisYMin(0);
        miniChart->setAxisXMin(0);

        // Apply our theme to axes:
        miniChart->setAxesThemeDefault();
        miniChart->axes(Qt::Vertical).first()->setMax(maxF + 1.0);
        miniChart->setAxisXLabelsAngle(-90);
        //    axisX->setShadesVisible(false);
    }
}
