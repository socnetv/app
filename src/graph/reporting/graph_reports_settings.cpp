
/**
 * @file graph_reports_settings.cpp
 * @brief Implements reporting configuration setters for the Graph class
 *        (reports output directory, numeric precision, label length, chart type).
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


/**
 * @brief Sets the directory where reports are saved
 * This is used when exporting prominence distribution images to be used in
 * HTML reports.
 * @param dir
 */
void Graph::setReportsDataDir(const QString &dir)
{
    m_reportsDataDir = dir;
}

/**
 * @brief Sets the precision (number of fraction digits) the app will use
 * when writing real numbers in reports.
 * @param precision
 */
void Graph::setReportsRealNumberPrecision(const int &precision)
{
    m_reportsRealPrecision = precision;
}

/**
 * @brief Sets the length of labels in reports
 * @param length
 */
void Graph::setReportsLabelLength(const int &length)
{
    m_reportsLabelLength = length;
}

/**
 * @brief Sets the chart type in reports
 * @param type
 */
void Graph::setReportsChartType(const int &type)
{
    qDebug() << "Graph::setReportsChartType() - type:" << type;
    if (type == -1)
    {
        m_reportsChartType = ChartType::None;
    }
    else if (type == 0)
    {
        m_reportsChartType = ChartType::Spline;
    }
    else if (type == 1)
    {
        m_reportsChartType = ChartType::Area;
    }
    else if (type == 2)
    {
        m_reportsChartType = ChartType::Bars;
    }
}
