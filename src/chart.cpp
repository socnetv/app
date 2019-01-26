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
    this->setChart(m_chart);

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


/**
 * @brief Toggle the legend of the QChart
 * @param toggle
 */
void Chart::toggleLegend(const bool &toggle){
    if (toggle) {
        m_chart->legend()->show();
    }
    else {
        m_chart->legend()->hide();
    }

}
/**
 * @brief Sets the background brush of the QChart
 * @param brush
 */
void Chart::setChartBackgroundBrush(const QBrush & brush) {
   m_chart->setBackgroundBrush(brush);
}


/**
 * @brief Sets the background pen of the QChart
 * @param brush
 */
void Chart::setChartBackgroundPen(const QBrush & brush) {
   m_chart->setBackgroundBrush(brush);
}


/**
 * @brief Set the theme of the QChart
 * @param theme
 */
void Chart::setTheme(QChart::ChartTheme theme) {
    m_chart->setTheme(theme);
}
/**
 * @brief Set the margins of the QChart
 * @param margins
 */
void Chart::setMargins(const QMargins &margins){
    m_chart->setMargins(margins);
}


/**
 * @brief Set the title of the QChart
 * @param title
 */
void Chart::setTitle(const QString &title){
    qDebug() << "Chart::setTitle()" ;
    m_chart->setTitleFont(QFont("Times",7));
    m_chart->setTitle(title);
}


/**
 * @brief Creates default axes. Must be called AFTER loading a series to the chart
 */
void Chart::createDefaultAxes(){
    qDebug() << "Chart::createDefaultAxes()" ;
    m_chart->createDefaultAxes();
}

/**
 * @brief Add series to chart
 * @param series
 */
void Chart::addSeries(QSplineSeries *series) {
    qDebug() << "Chart::addSeries()" ;
  //  m_series = series;
    m_chart->addSeries(series);
}

/**
 * @brief Chart::appendToSeries ??
 * @param p
 */
void Chart::appendToSeries(const QPointF &p) {
    m_series->append(p);
//    m_chart->addSeries();

}

