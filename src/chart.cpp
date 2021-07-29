/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0
 Written in Qt

                         chart.h  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
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
#include <QVBoxLayout>

Chart::Chart(QWidget *parent) :
    QChartView (parent ),
    m_chart(new QChart)
{
    qDebug() << "Constructing a Chart";

    setChart(m_chart);

    m_chart->setAnimationOptions(QChart::SeriesAnimations);
}


Chart::~Chart(){
    qDebug()<< "Deleting m_chart pointer";
    delete m_chart;
    //m_series->clear();
}




/**
 * @brief Add QAbstractSeries series to chart
 * If no series are passed, a new QSplineSeries is created with 1 point at (0,0)
 * @param series
 */
void Chart::addSeries(QAbstractSeries *series) {
    qDebug() << "Adding a series to chart" ;
    if (series) {
        m_chart->addSeries(series);
        qDebug() << "added series with name"<< series->name() ;
    }
    else {
        // default: a trivial series with one point
        // We need this for resetToTrivial()
        // to be able to call createDefaultAxes()
        m_series = new QSplineSeries();
        *m_series << QPointF(0,0);
        m_series->setName("trivial");
        m_chart->addSeries(m_series);
        qDebug() << "trivial series with one point created.";
    }

}



/**
 * @brief Adds the data point p(qreal,qreal) to the series.
 * @param p
 */
void Chart::appendToSeries(const QPointF &p) {
    qDebug() <<"Appending a QPoint to series"  ;
    m_series->append(p);
}


/**
 * @brief Removes and deletes all series objects that have been added to the chart.
 */
void Chart::removeAllSeries() {
    qDebug() <<"Removing all series... "  ;

    if ( ! m_chart->series().empty() ) {
        m_chart->removeAllSeries();
        qDebug() <<"series count:"  << m_chart->series().size();
    }

}




/**
 * @brief Creates default axes. Must be called AFTER loading a series to the chart
 */
void Chart::createDefaultAxes(){
    qDebug() << "Creating default axes..." ;
    m_chart->createDefaultAxes();

}


QList<QAbstractAxis *> Chart::axes(Qt::Orientations orientation,
                            QAbstractSeries *series) const {
    qDebug() << "Chart::axes()" ;
    if (series == Q_NULLPTR) {
        qDebug() << "Chart::axes() - no series defined" ;
        return m_chart->axes(orientation);
    }
    else {
        qDebug() << "Chart::axes() - a series was defined" ;
        return m_chart->axes(orientation, series);
    }

}

/**
 * @brief Removes all previously attached X,Y axes from the QChart
 */
void Chart::removeAllAxes(){

    qDebug() << "Removing all axes";

    if ( ! axes(Qt::Horizontal).isEmpty() )  {
        qDebug() << "Looping over horizontal axes to delete - count: "<< m_chart->axes(Qt::Horizontal).size();
        foreach ( QAbstractAxis *axe, axes(Qt::Horizontal) ) {
            m_chart->removeAxis(axe);
        }
    }

    if ( ! axes(Qt::Vertical).isEmpty() )  {
        qDebug() << "Looping over vertical axes to delete - count: "<< m_chart->axes(Qt::Vertical).size();
        foreach ( QAbstractAxis *axe, axes(Qt::Vertical) ) {
            m_chart->removeAxis(axe);
        }
    }


}



/**
 * @brief Adds the axis axis to the chart and attaches it to the series series
 * as a bottom-aligned horizontal axis.
 * The chart takes ownership of both the axis  and the series.
  * @param axis
 * @param series
 */
void Chart::setAxisX(QAbstractAxis *axis, QAbstractSeries *series) {
    qDebug()<<"Adding axis X to chart";
    addAxis(axis, Qt::AlignBottom);
    qDebug()<<"Attaching axis X to current series " << series->name();
    series->attachAxis(axis);
}


/**
 * @brief Adds the axis axis to the chart and attaches it to the series series
 * as a left-aligned horizontal axis.
 * The chart takes ownership of both the axis  and the series.
  * @param axis
 * @param series
 */
void Chart::setAxisY(QAbstractAxis *axis, QAbstractSeries *series) {
    qDebug()<<"Adding axis Y to chart";
    addAxis(axis, Qt::AlignLeft);
    qDebug()<<"Attaching axis Y to current series " << series->name();
    series->attachAxis(axis);

}



/**
 * @brief Add Axis axis to the QChart. Does not delete previously attached axis
 * @param axis
 * @param alignment
 */
void Chart::addAxis(QAbstractAxis *axis, Qt::Alignment alignment) {
    qDebug()<< "Adding axis to chart";
    m_chart->addAxis(axis,alignment);
    // We could also check if m_series and do:
    // barSeries->attachAxis(axisY);
}



/**
 * @brief Set the range of the (first) horizontal axis
 * @param min
 * @param max
 */
void Chart::setAxisXRange(const QVariant &min, const QVariant &max){
    qDebug()<< "Setting axis X range...";
    m_chart->axes(Qt::Horizontal).first()->setRange(min, max);
}


/**
 * @brief Sets the minimum value shown on the horizontal axis.
 * @param min
 */
void Chart::setAxisXMin(const QVariant &min){
    qDebug()<< "Setting axis X min...";
    m_chart->axes(Qt::Horizontal).first()->setMin(min);
}



/**
 * @brief Set the range of the vertical axis
 * @param min
 * @param max
 */
void Chart::setAxisYRange(const QVariant &min, const QVariant &max){
    qDebug()<< "Setting axis Y range...";
    m_chart->axes(Qt::Vertical).first()->setRange(min, max);
}



/**
 * @brief Sets the minimum value shown on the vertical axis.
 * @param min
 */
void Chart::setAxisYMin(const QVariant &min){
    qDebug()<< "Setting axis X min...";
    m_chart->axes(Qt::Vertical).first()->setMin(min);
}



void Chart::setAxisXLabelsAngle (const int &angle){
    qDebug()<< "Setting axis X label angle...";
    m_chart->axes(Qt::Horizontal).first()->setLabelsAngle(angle);

}

/**
 * @brief Set the label font of the horizontal axis
 * @param font
 */
void Chart::setAxisXLabelFont(const QFont &font){
    qDebug()<< "Setting axis X label font...";
    m_chart->axes(Qt::Horizontal).first()->setLabelsFont(font);
}



/**
 * @brief Set the label font of the vertical axis
 * @param font
 */
void Chart::setAxisYLabelFont(const QFont &font){
    qDebug()<< "Setting axis Y label font...";
    m_chart->axes(Qt::Vertical).first()->setLabelsFont(font);
}




/**
 * @brief Set the line pen of the horizontal axis
 * @param font
 */
void Chart::setAxisXLinePen(const QPen &pen){
    qDebug()<< "Setting axis X line pen...";
    m_chart->axes(Qt::Horizontal).first()->setLinePen(pen);
}



/**
 * @brief Set the line pen of the vertical axis
 * @param font
 */
void Chart::setAxisYLinePen(const QPen &pen){
    qDebug()<< "Setting axis Y line pen...";
    m_chart->axes(Qt::Vertical).first()->setLinePen(pen);

}




/**
 * @brief Set the grid line pen of the horizontal axis
 * @param font
 */
void Chart::setAxisXGridLinePen(const QPen &pen){
    qDebug()<< "Setting axis X grid line pen...";
    m_chart->axes(Qt::Horizontal).first()->setGridLinePen(pen);
}



/**
 * @brief Set the grid line pen of the vertical axis
 * @param font
 */
void Chart::setAxisYGridLinePen(const QPen &pen){
    qDebug()<< "Setting axis Y grid line pen...";
    m_chart->axes(Qt::Vertical).first()->setGridLinePen(pen);

}





/**
 * @brief Toggle the legend of the QChart
 * @param toggle
 */
void Chart::toggleLegend(const bool &toggle){
    qDebug()<< "toggling chart legend...";
    if (toggle) {
        m_chart->legend()->show();
    }
    else {
        m_chart->legend()->hide();
    }

}
/**
 * @brief Sets the background brush of the QChart
 * If no brush defined, it uses a transparent brush.
 * @param brush
 */
void Chart::setChartBackgroundBrush(const QBrush & brush) {
    qDebug()<< "Setting chart background brush...";
   m_chart->setBackgroundBrush(brush);
}


/**
 * @brief Sets the background pen of the QChart
 * If no pen defined, it uses a transparent pen.
 * @param brush
 */
void Chart::setChartBackgroundPen(const QPen & pen) {
   qDebug()<< "Setting chart background pen...";
   m_chart->setBackgroundPen(pen);
}


/**
 * @brief Set the theme of the QChart
 * @param theme
 */
void Chart::setTheme(QChart::ChartTheme theme) {
    qDebug()<< "Setting chart theme...";
    m_chart->setTheme(theme);
}


/**
 * @brief Set the theme for when our chart widget is small
 * @param chartHeight
 */
void Chart::setThemeSmallWidget(const int minWidth, const int minHeight) {
    qDebug()<< "Setting small chart widget theme...";
    setTheme();
    setBackgroundBrush(QBrush(Qt::transparent));
    setChartBackgroundBrush();
    setChartBackgroundPen();

    toggleLegend(false);
    setRenderHint(QPainter::Antialiasing);

    setMinimumWidth(minWidth);
    setMaximumHeight(1.5*minHeight);
    setMinimumHeight(minHeight);
    setFrameShape(QFrame::NoFrame);

}


/**
 * @brief Set the margins of the QChart
 * @param margins
 */
void Chart::setMargins(const QMargins &margins){
    qDebug()<< "Setting chart margins...";
    m_chart->setMargins(margins);
}


/**
 * @brief Set the title of the QChart
 * @param title
 */
void Chart::setTitle(const QString &title, const QFont &font){
    qDebug() << "Setting chart title..." ;
    m_chart->setTitleFont(font);
    m_chart->setTitle(title);
}


/**
 * @brief Applies a simple theme to axes (default label fonts, line and grid line pen).
 * WARNING: Axes must be already attached to m_chart
 */
void Chart::setAxesThemeDefault() {
    qDebug()<< "Setting a simple theme to chart axes...";
    setAxisXLabelFont();
    setAxisXLinePen();
    setAxisXGridLinePen();
    setAxisYLabelFont();
    setAxisYLinePen();
    setAxisYGridLinePen();

    setMargins(QMargins());

}


void Chart::resetToTrivial() {
    qDebug()<< "Resetting chart to trivial...";
    removeAllSeries();
    addSeries();
    createDefaultAxes();
    axes(Qt::Horizontal).first()->setLabelsAngle(-90);
    setTitle("Chart", QFont("Times",8));

    setAxisXRange(0,1);
    setAxisYRange(0,1);

    setAxesThemeDefault();

}


