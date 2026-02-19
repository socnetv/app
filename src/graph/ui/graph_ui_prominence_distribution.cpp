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

    for (const auto &pt : points) {
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

    if (!distImageFileName.isEmpty()) {
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