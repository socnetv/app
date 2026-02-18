#include "graph.h"
#include <QDebug>
#include <queue>      // std::priority_queue
#include <vector>     // std::vector (used as the container in priority_queue)

// Qt Charts types used here 
#include <QtCharts/QChart>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QAbstractSeries>
#include <QSplineSeries>
#include <QAreaSeries>

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
    emit statusMessage(pMsg);

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
        emit statusMessage(tr("Creating prominence index distribution line chart..."));
        prominenceDistributionSpline(discreteClasses, seriesName, distImageFileName);
        break;
    case ChartType::Area:
        emit statusMessage(tr("Creating prominence index distribution area chart..."));
        prominenceDistributionArea(discreteClasses, seriesName, distImageFileName);
        break;
    case ChartType::Bars:
        emit statusMessage(tr("Creating prominence index distribution bar chart..."));
        prominenceDistributionBars(discreteClasses, seriesName, distImageFileName);
        break;
    }
}

/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QSplineSeries series to MW
 * which in turn displays them on a Spline Chart
 * @param index
 * @param series
 */
void Graph::prominenceDistributionSpline(const H_StrToInt &discreteClasses,
                                         const QString &seriesName,
                                         const QString &distImageFileName)
{

    qDebug() << "Computing prominence distribution as spline chart...";

    QLineSeries *series = new QLineSeries();
    series->setName(seriesName);
    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();

    // Used only for the large chart exported to PNG for the HTML report
    QLineSeries *series1 = new QLineSeries();
    series1->setName(seriesName);
    QValueAxis *axisX1 = new QValueAxis();
    QValueAxis *axisY1 = new QValueAxis();

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i)
    {

        qDebug() << "discreteClasses: "
                 << i.key() << ": " << i.value();

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));
    }

    unsigned int initialSize = seriesPQ.size();
    qreal min = 0;
    qreal max = 0;
    qreal value = 0;

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty())
    {
        qDebug() << "seriesPQ top is:"
                 << seriesPQ.top().value << " : "
                 << seriesPQ.top().frequency;

        value = seriesPQ.top().value;
        frequency = seriesPQ.top().frequency;

        series->append(value, frequency);
        series1->append(value, frequency);

        if (frequency < minF)
        {
            minF = frequency;
        }
        if (frequency > maxF)
        {
            maxF = frequency;
        }

        if (initialSize == seriesPQ.size())
        {
            min = value;
        }
        if (seriesPQ.size() == 1)
        {
            max = value;
        }

        seriesPQ.pop();
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
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving prominence distribution image to" << distImageFileName;

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView(chart);

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        // chartView->show();

        chart->addSeries(series1);

        chart->setTitle(series1->name() + " distribution");
        chart->setTitleFont(QFont("Times", 12));

        chart->legend()->hide();

        // chart->createDefaultAxes();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();

        p.save(distImageFileName, "PNG");

        chartView->hide();
        // Do not delete the ChartView
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        //        chartView->deleteLater();
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution spline chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}

/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QAreaSeries series to MW
 * which in turn displays them on a Area Chart
 * @param index
 * @param series
 */
void Graph::prominenceDistributionArea(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName)
{

    qDebug() << "Computing prominence distribution as area chart...";

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

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i)
    {

        qDebug() << "discreteClasses: " << i.key() << ": " << i.value() << "\n";

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));
    }

    unsigned int initialSize = seriesPQ.size();
    qreal min = 0;
    qreal max = 0;
    qreal value = 0;

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty())
    {

        qDebug() << seriesPQ.top().value << " : "
                 << seriesPQ.top().frequency << "\n";

        value = seriesPQ.top().value;
        frequency = seriesPQ.top().frequency;

        upperSeries->append(value, frequency);

        if (frequency < minF)
        {
            minF = frequency;
        }
        if (frequency > maxF)
        {
            maxF = frequency;
        }

        if (initialSize == seriesPQ.size())
        {
            min = value;
        }
        if (seriesPQ.size() == 1)
        {
            max = value;
        }

        seriesPQ.pop();
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
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving distribution image to" << distImageFileName;

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        series1->setUpperSeries(upperSeries);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView(chart);

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        // chartView->show();

        chart->addSeries(series1);

        chart->setTitle(series->name() + " distribution");
        chart->setTitleFont(QFont("Times", 12));

        chart->legend()->hide();

        // chart->createDefaultAxes();

        chart->addAxis(axisX1, Qt::AlignBottom);
        series1->attachAxis(axisX1);
        chart->addAxis(axisY1, Qt::AlignLeft);
        series1->attachAxis(axisY1);

        chart->axes(Qt::Vertical).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setMin(0);
        chart->axes(Qt::Horizontal).first()->setLabelsAngle(-90);

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();

        p.save(distImageFileName, "PNG");

        chartView->hide();

        // Do not delete the ChartView
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        //        chartView->deleteLater();
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution area chart";
    emit signalPromininenceDistributionChartUpdate(series, axisX, min, max, axisY, minF, maxF);
}

/**
 * @brief Computes the distribution of a centrality index scores.
 * The distribution data are returned as QBarSeries series (with a QBarSet attached)
 * to MW  which in turn displays them on a Bar Chart
 * @param index
 * @param series
 * @param set
 * @param strX
 */
void Graph::prominenceDistributionBars(const H_StrToInt &discreteClasses,
                                       const QString &name,
                                       const QString &distImageFileName)
{

    qDebug() << "Computing prominence distribution as bar chart...";

    QBarSeries *series = new QBarSeries();
    series->setName(name);
    QBarSet *barSet = new QBarSet("");
    QValueAxis *axisY = new QValueAxis;
    QBarCategoryAxis *axisX = new QBarCategoryAxis();

    // Used only for the large chart exported to PNG for the HTML report
    QBarSeries *series1 = new QBarSeries();
    series1->setName(name);
    QBarSet *barSet1 = new QBarSet("");
    QValueAxis *axisY1 = new QValueAxis;
    QBarCategoryAxis *axisX1 = new QBarCategoryAxis();

    // The seriesPQ is a priority queue which will hold
    // the (value,frequency) pairs ordered from smallest to larger value
    // This is used further below to insert pairs to the series in an ordered way
    priority_queue<PairVF, vector<PairVF>, PairVFCompare> seriesPQ;

    QHash<QString, int>::const_iterator i;

    for (i = discreteClasses.constBegin(); i != discreteClasses.constEnd(); ++i)
    {

        qDebug() << "discreteClasses: " << i.key() << ": " << i.value() << "\n";

        seriesPQ.push(PairVF(i.key().toDouble(), i.value()));
    }

    unsigned int initialSize = seriesPQ.size();

    QString min = QString();
    QString max = QString();
    QString value = QString();

    qreal frequency = 0;
    qreal minF = RAND_MAX;
    qreal maxF = 0;

    while (!seriesPQ.empty())
    {

        value = QString::number(seriesPQ.top().value, 'f', 6);

        frequency = seriesPQ.top().frequency;

        qDebug() << "value:" << value << " : "
                 << "frequency:" << frequency << "\n";

        axisX->append(value);
        barSet->append(frequency);

        if (!distImageFileName.isEmpty())
        {
            axisX1->append(value);
            barSet1->append(frequency);
        }

        if (frequency < minF)
        {
            minF = frequency;
        }
        if (frequency > maxF)
        {
            maxF = frequency;
        }

        if (initialSize == seriesPQ.size())
        {
            min = value;
        }
        if (seriesPQ.size() == 1)
        {
            max = value;
        }

        seriesPQ.pop();

    } // end while

    axisX->setMin(min);
    axisX->setMax(max);

    axisY->setMin(minF);
    axisY->setMax(maxF + 1.0);

    qDebug() << "axisX min: " << axisX->min() << " max: " << axisX->max();

    series->append(barSet);

    QPen sPen(QColor("#666"));
    sPen.setWidthF(0.2);
    QBrush sBrush(QColor("#209fdf"));

    barSet->setBrush(sBrush);
    barSet->setPen(sPen);

    if (!distImageFileName.isEmpty())
    {
        // Filename given. Need to save the chart to an image file.

        qDebug() << "saving distribution image to" << distImageFileName;

        series1->append(barSet1);

        axisX1->setMin(min);
        axisX1->setMax(max);

        axisY1->setMin(minF);
        axisY1->setMax(maxF + 1.0);

        QChart *chart = new QChart();
        QChartView *chartView = new QChartView(chart);

        // Not needed?
        // If we do show it, then it will show a brief flash of the window to the user!
        // chartView->show();

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

        //        m_chart->axes(Qt::Horizontal).first()->setShadesVisible(false);

        chart->resize(2560, 1440);
        chartView->resize(2561, 1441);

        QPixmap p = chartView->grab();

        p.save(distImageFileName, "PNG");

        chartView->hide();
        // Do not delete the ChartView if the axes / series are the same!
        // If we do delete it, then it will also delete the axes
        // which we have sent to MW to be displayed on the miniChart.
        // The result will be app crash...
        delete chartView;
    }

    qDebug() << "emitting signal to MW update the prominence distribution bar chart";
    emit signalPromininenceDistributionChartUpdate(series,
                                                   axisX, min.toDouble(), max.toDouble(),
                                                   axisY, minF, maxF);
}

