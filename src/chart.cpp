/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt

                         chart.h  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/



#include "chart.h"

#include <QDebug>
#include <QtCharts/QChart>
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
#include <QtCharts/QBarCategoryAxis>
#include <QtCore/QTime>



Chart::Chart(QWidget *parent) :
    QChartView (parent ),
    m_chart(new QChart)
{
    qDebug() << "Chart(QWidget *parent) ";

    setChart(m_chart);

    m_chart->setAnimationOptions(QChart::SeriesAnimations);
}

Chart::Chart(QChart *ch, QWidget *parent) :
    QChartView (ch, parent )
{

    qDebug() << "Chart(QChart *ch, QWidget *parent) ";

}



Chart::~Chart(){
    qDebug()<< "~Chart() - deleting pointers";
    delete m_chart;
    delete m_series;
}


/**
 * @brief Set the range of the (first) horizontal axis
 * @param from
 * @param to
 */
void Chart::setAxisXRange(const int &from, const int &to){
    m_chart->axes(Qt::Horizontal).first()->setRange(from, to);
}


/**
 * @brief Set the range of the vertical axis
 * @param from
 * @param to
 */
void Chart::setAxisYRange(const int &from, const int &to){
    m_chart->axes(Qt::Vertical).first()->setRange(from, to);
}



/**
 * @brief Set the label font of the horizontal axis
 * @param font
 */
void Chart::setAxisXLabelFont(const QFont &font){
    m_chart->axisX()->setLabelsFont(font);
}



/**
 * @brief Set the label font of the vertical axis
 * @param font
 */
void Chart::setAxisYLabelFont(const QFont &font){
    m_chart->axisY()->setLabelsFont(font);
}




/**
 * @brief Set the line pen of the horizontal axis
 * @param font
 */
void Chart::setAxisXLinePen(const QPen &pen){
    m_chart->axisX()->setLinePen(pen);
}



/**
 * @brief Set the line pen of the vertical axis
 * @param font
 */
void Chart::setAxisYLinePen(const QPen &pen){
    m_chart->axisY()->setLinePen(pen);

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
void Chart::setTitle(const QString &title, const QFont &font){
    qDebug() << "Chart::setTitle()" ;
    m_chart->setTitleFont(font);
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
void Chart::addSeries(QAbstractSeries *series) {
    qDebug() << "Chart::addSeries()" ;
  //  m_series = series;
    if (series) {
     m_chart->addSeries(series);
    }
    else {
        m_series = new QSplineSeries();
        *m_series << QPointF(0,0);
        m_chart->addSeries(m_series);
    }

}



/**
 * @brief Adds the data point p(qreal,qreal) to the series.
 * @param p
 */
void Chart::appendToSeries(const QPointF &p) {
    m_series->append(p);
    // *m_series<<p;
}


/**
 * @brief Removes and deletes all series objects that have been added to the chart.
 */
void Chart::removeAllSeries() {
    m_chart->removeAllSeries();
}



void Chart::resetToTrivial() {
    removeAllSeries();
    addSeries();
    createDefaultAxes();
    setTitle("Chart", QFont("Times",8));

    setMargins(QMargins());

    setAxisXRange(0,1);
    setAxisYRange(0,1);
    setAxisXLabelFont(QFont("Times", 7));
    setAxisYLabelFont(QFont("Times", 7));

    QPen axisPen;
    axisPen.setBrush( QBrush(QColor(0,0,0,0)) );
    axisPen.setWidthF(0.5);
    axisPen.setStyle(Qt::SolidLine);
    setAxisYLinePen(axisPen);
    setMargins(QMargins());

}
