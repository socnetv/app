#include "chart.h"

#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>


#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QAbstractBarSeries>
#include <QtCharts/QPercentBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QLegend>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtCore/QTime>
#include <QtCharts/QBarCategoryAxis>

#include <QtDebug>

Chart::Chart(QWidget *parent) :
    QChartView (parent ),
    m_chart(new QChart)
{

    qDebug() << "Chart(QWidget *parent) ";

    m_chart->legend()->hide();
    m_chart->setTitle("Simple");
    m_chart->setTitleFont(QFont("Times",7));


}

Chart::Chart(QChart *ch, QWidget *parent) :
    QChartView (ch, parent )
{

    qDebug() << "Chart(QChart *ch, QWidget *parent) ";

    QSplineSeries *m_series = new QSplineSeries;


    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->addSeries(m_series);
    m_chart->setTitle("Simple");
    m_chart->setTitleFont(QFont("Times",7));
    m_chart->createDefaultAxes();
    m_chart->axes(Qt::Vertical).first()->setRange(0, 10);
    m_chart->setTheme(QChart::ChartThemeQt);
    m_chart->setBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setBackgroundPen(QPen(Qt::transparent));
    m_chart->setMargins(QMargins(0,0,0,0));
    m_chart->axisX()->setLabelsFont(QFont("Times", 7));
    m_chart->axisY()->setLabelsFont(QFont("Times", 7));
    QPen axisPen;
    axisPen.setBrush( QBrush(QColor(0,0,0,0)) );
    axisPen.setWidthF(0.5);
    axisPen.setStyle(Qt::SolidLine);
    m_chart->axisY()->setLinePen(axisPen);

    this->setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumWidth(300);
    setMinimumHeight(200);
    setFrameShape(QFrame::NoFrame);


}


Chart::~Chart(){

}

void Chart::setSeries(QSplineSeries *series) {
  //  m_series = series;
    m_chart->addSeries(series);
}

void Chart::appendToSeries(const QPointF &p) {
    m_series->append(p);
//    m_chart->addSeries();

}

