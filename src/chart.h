#ifndef CHART_H
#define CHART_H

#include <QObject>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>

QT_BEGIN_NAMESPACE

QT_END_NAMESPACE


QT_CHARTS_BEGIN_NAMESPACE
class QSplineSeries;
class QChart;
QT_CHARTS_END_NAMESPACE



QT_CHARTS_USE_NAMESPACE

class Chart : public QChartView
{
    Q_OBJECT
public:
    explicit Chart (QWidget *parent = Q_NULLPTR );
    explicit Chart ( QChart *ch = Q_NULLPTR, QWidget *parent = Q_NULLPTR );
    ~Chart();

    void setSeries(QSplineSeries *series);
    void appendToSeries (const QPointF &p);
private:
    QChart *m_chart;
    QSplineSeries *m_series;
};

#endif // CHART_H
