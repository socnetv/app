/**
 * @file graph_prominence_distribution.cpp
 * @brief Implements prominence index distribution and visualization routines
 *        (bars, spline, area charts) for the Graph class.
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
#include <queue>  // std::priority_queue
#include <vector> // std::vector (used as the container in priority_queue)



namespace {

struct PromDistData {
    std::vector<std::pair<double,double>> points; // (value, frequency)
    double min  = 0.0;
    double max  = 0.0;
    double minF = 0.0;
    double maxF = 0.0;
};

static PromDistData computePromDistData(const H_StrToInt &discreteClasses) {
    // priority queue (value, frequency) ordered ascending by value
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    for (auto it = discreteClasses.constBegin(); it != discreteClasses.constEnd(); ++it) {
        seriesPQ.push(PairVF(it.key().toDouble(), it.value()));
    }

    PromDistData out;
    const size_t initialSize = seriesPQ.size();

    if (initialSize == 0) {
        out.minF = 0.0;
        out.maxF = 0.0;
        return out;
    }

    out.minF = static_cast<double>(RAND_MAX);
    out.maxF = 0.0;

    size_t idx = 0;
    while (!seriesPQ.empty()) {
        const double value = seriesPQ.top().value;
        const double freq  = seriesPQ.top().frequency;

        out.points.emplace_back(value, freq);

        if (freq < out.minF) out.minF = freq;
        if (freq > out.maxF) out.maxF = freq;

        if (idx == 0) out.min = value;
        if (seriesPQ.size() == 1) out.max = value;

        seriesPQ.pop();
        ++idx;
    }

    return out;
}

static QVector<QPair<qreal,qreal>> toQPoints(const std::vector<std::pair<double,double>> &pts) {
    QVector<QPair<qreal,qreal>> out;
    out.reserve(static_cast<int>(pts.size()));
    for (const auto &p : pts) {
        out.append(qMakePair(static_cast<qreal>(p.first), static_cast<qreal>(p.second)));
    }
    return out;
}

} // namespace


/**
 * @brief Returns the IndexType of the given prominence index name
 * Called from MW::slotEditNodeFind, MW::slotLayoutRadialByProminenceIndex etc
 * @param prominenceIndexName
 */
int Graph::getProminenceIndexByName(const QString &prominenceIndexName)
{

    qDebug() << "Returning index type for index named: " << prominenceIndexName;

    if (prominenceIndexName.contains("Degree Centr"))
    {
        return IndexType::DC;
    }
    else if (prominenceIndexName.contains("Closeness Centr") &&
             !prominenceIndexName.contains("IR"))
    {
        return IndexType::CC;
    }
    else if (prominenceIndexName.contains("Influence Range Closeness Centrality") ||
             prominenceIndexName.contains("IR Closeness"))
    {
        return IndexType::IRCC;
    }
    else if (prominenceIndexName.contains("Betweenness Centr"))
    {
        return IndexType::BC;
    }
    else if (prominenceIndexName.contains("Stress Centr"))
    {
        return IndexType::SC;
    }
    else if (prominenceIndexName.contains("Eccentricity Centr"))
    {
        return IndexType::EC;
    }
    else if (prominenceIndexName.contains("Power Centr"))
    {
        return IndexType::PC;
    }
    else if (prominenceIndexName.contains("Information Centr"))
    {
        return IndexType::IC;
    }
    else if (prominenceIndexName.contains("Eigenvector Centr"))
    {
        return IndexType::EVC;
    }
    else if (prominenceIndexName.contains("Degree Prestige"))
    {
        return IndexType::DP;
    }
    else if (prominenceIndexName.contains("PageRank Prestige"))
    {
        return IndexType::PRP;
    }
    else if (prominenceIndexName.contains("Proximity Prestige"))
    {
        return IndexType::PP;
    }
    else
        return 0;
}

/**
 * @brief Computes the distribution of a centrality index score.
 * The distribution is stored as Qt Series depending on the SeriesType parameter type
 * It is send to MW through signal/slot
 * @param index
 * @param type
 */
void Graph::prominenceDistribution(const int &index,
                                   const ChartType &type,
                                   const QString &distImageFileName)
{

    qDebug() << "Request to compute prominence distribution. "
             << "index" << index
             << "chart type: " << type
             << "distImageFileName" << distImageFileName;

    QString pMsg = tr("Computing Centrality Distribution. \nPlease wait...");
    progressStatus(pMsg);

    H_StrToInt discreteClasses;

    QString seriesName;

    qDebug() << "setting prominence distribution series name and classes...";
    switch (index)
    {
    case 0:
    {
        break;
    }
    case IndexType::DC:
    {
        seriesName = ("(out)Degree");
        discreteClasses = discreteSDCs;
        break;
    }
    case IndexType::CC:
    {
        seriesName = ("Closeness");
        discreteClasses = discreteCCs;
        break;
    }
    case IndexType::IRCC:
    {
        seriesName = ("IRCC");
        discreteClasses = discreteIRCCs;
        break;
    }
    case IndexType::BC:
    {
        seriesName = ("Betweenness");
        discreteClasses = discreteBCs;
        break;
    }
    case IndexType::SC:
    {
        seriesName = ("Stress");
        discreteClasses = discreteSCs;
        break;
    }
    case IndexType::EC:
    {
        seriesName = ("Eccentricity");
        discreteClasses = discreteECs;
        break;
    }
    case IndexType::PC:
    {
        seriesName = ("Power");
        discreteClasses = discretePCs;
        break;
    }
    case IndexType::IC:
    {
        seriesName = ("Information");
        discreteClasses = discreteICs;
        break;
    }
    case IndexType::EVC:
    {
        seriesName = ("Eigenvector");
        discreteClasses = discreteEVCs;
        break;
    }
    case IndexType::DP:
    {
        seriesName = ("Prestige Degree");
        discreteClasses = discreteDPs;
        break;
    }
    case IndexType::PRP:
    {
        seriesName = ("Pagerank");
        discreteClasses = discretePRPs;
        break;
    }
    case IndexType::PP:
    {
        seriesName = ("Proximity");
        discreteClasses = discretePPs;
        break;
    }
    }

    qDebug() << "calling the relevant prominence distribution computation method...";
    switch (type)
    {
    case ChartType::None:
        emit signalPromininenceDistributionChartUpdate(Q_NULLPTR, Q_NULLPTR);
        break;
    case ChartType::Spline:
        progressStatus(tr("Creating prominence index distribution line chart..."));
        prominenceDistributionSpline(discreteClasses, seriesName, distImageFileName);
        break;
    case ChartType::Area:
        progressStatus(tr("Creating prominence index distribution area chart..."));
        prominenceDistributionArea(discreteClasses, seriesName, distImageFileName);
        break;
    case ChartType::Bars:
        progressStatus(tr("Creating prominence index distribution bar chart..."));
        prominenceDistributionBars(discreteClasses, seriesName, distImageFileName);
        break;
    }
}



/**
 * @brief Computes prominence distribution data and delegates Spline chart rendering.
 *
 * Performs the algorithmic portion only:
 *  - Orders (value, frequency) pairs derived from @p discreteClasses
 *  - Computes min/max value and min/max frequency
 *
 * UI construction (Qt Charts series/axes), optional PNG export, and emission of
 * signalPromininenceDistributionChartUpdate(...) are delegated to the UI façade
 * implementation in graph_ui_prominence_distribution.cpp (WS2/F4).
 *
 * Behavior and output semantics are preserved.
 *
 * @param discreteClasses    Map of value (string) -> frequency.
 * @param seriesName         Display name of the series.
 * @param distImageFileName  If non-empty, export chart to this PNG file.
 */
void Graph::prominenceDistributionSpline(const H_StrToInt &discreteClasses,
                                         const QString &seriesName,
                                         const QString &distImageFileName)
{
    qDebug() << "Computing prominence distribution as spline chart...";

    const PromDistData data = computePromDistData(discreteClasses);
    uiProminenceDistributionSpline(
        toQPoints(data.points),
        static_cast<qreal>(data.min),
        static_cast<qreal>(data.max),
        static_cast<qreal>(data.minF),
        static_cast<qreal>(data.maxF),
        seriesName,
        distImageFileName);
}


/**
 * @brief Computes prominence distribution data and delegates Area chart rendering.
 *
 * Performs the algorithmic portion only:
 *  - Orders (value, frequency) pairs derived from @p discreteClasses
 *  - Computes min/max value and min/max frequency
 *
 * UI construction (Qt Charts series/axes), optional PNG export, and emission of
 * signalPromininenceDistributionChartUpdate(...) are delegated to the UI façade
 * implementation in graph_ui_prominence_distribution.cpp (WS2/F4).
 *
 * Behavior and output semantics are preserved.
 *
 * @param discreteClasses    Map of value (string) -> frequency.
 * @param name               Display name of the series.
 * @param distImageFileName  If non-empty, export chart to this PNG file.
 */
void Graph::prominenceDistributionArea(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName)
{
    qDebug() << "Computing prominence distribution as area chart...";

    const PromDistData data = computePromDistData(discreteClasses);

    // UI-only: builds QAreaSeries/QAxes, exports PNG if requested,
    // emits signalPromininenceDistributionChartUpdate(...)
    uiProminenceDistributionArea(toQPoints(data.points),
                                 data.min, data.max,
                                 data.minF, data.maxF,
                                 name,
                                 distImageFileName);
}

/**
 * @brief Computes the prominence distribution and delegates Bar chart rendering.
 *
 * This method performs only the algorithmic portion:
 * it derives ordered category labels (centrality values formatted with 6 decimals),
 * the matching frequencies, and computes basic range statistics:
 *  - min/max value (numeric)
 *  - min/max frequency
 *
 * UI construction (Qt Charts objects, axes, optional PNG export) and emission
 * of the update signal to MainWindow are delegated to the UI façade layer
 * (graph_ui_prominence_distribution.cpp), following WS2/F4 rules.
 *
 * Behavior, rendering semantics, and export output remain unchanged.
 *
 * @param discreteClasses   A map of centrality value (as string) to frequency.
 * @param name              The display name of the distribution series.
 * @param distImageFileName If non-empty, the chart is exported to this PNG file.
 */
void Graph::prominenceDistributionBars(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName)
{
    qDebug() << "Computing prominence distribution as bar chart...";

    const PromDistData data = computePromDistData(discreteClasses);

    // Bars need string categories (old code used QString::number(value, 'f', 6))
    QStringList categories;
    QVector<qreal> freqs;
    categories.reserve(static_cast<int>(data.points.size()));
    freqs.reserve(static_cast<int>(data.points.size()));

    for (const auto &p : data.points) {
        categories.append(QString::number(p.first, 'f', 6));
        freqs.append(static_cast<qreal>(p.second));
    }

    uiProminenceDistributionBars(categories,
                                freqs,
                                static_cast<qreal>(data.min),
                                static_cast<qreal>(data.max),
                                static_cast<qreal>(data.minF),
                                static_cast<qreal>(data.maxF),
                                name,
                                distImageFileName);
}
