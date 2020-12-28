/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.7-dev
 Written in Qt

                         chart.h  -  description
                             -------------------
    copyright         : (C) 2005-2020 by Dimitris B. Kalamaras
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
#include <QTest>
#include <QVBoxLayout>

Chart::Chart(QWidget *parent) :
    QChartView (parent ),
    m_chart(new QChart)
{
    qDebug() << "Chart(QWidget *parent) ";

    setChart(m_chart);

    m_chart->setAnimationOptions(QChart::SeriesAnimations);
}

//Chart::Chart(QChart *ch, QWidget *parent) :
//    QChartView (ch, parent )
//{

//    qDebug() << "Chart(QChart *ch, QWidget *parent) ";

//}



Chart::~Chart(){
    qDebug()<< "~Chart() - deleting pointers";
    delete m_chart;
    //m_series->clear();
}




/**
 * @brief Add QAbstractSeries series to chart
 * If no series are passed, a new QSplineSeries is created with 1 point at (0,0)
 * @param series
 */
void Chart::addSeries(QAbstractSeries *series) {
    qDebug() << "Chart::addSeries()" ;
  //  m_series = series;
    if (series) {
        m_chart->addSeries(series);
        qDebug() << "Chart::addSeries() - added series with name"<< series->name() ;
    }
    else {
        // default: a trivial series with one point
        // We need this for resetToTrivial()
        // to be able to call createDefaultAxes()
        m_series = new QSplineSeries();
        *m_series << QPointF(0,0);
        m_series->setName("trivial");
        m_chart->addSeries(m_series);
        qDebug() << "Chart::addSeries() - trivial series with one point created.";
    }

}



/**
 * @brief Adds the data point p(qreal,qreal) to the series.
 * @param p
 */
void Chart::appendToSeries(const QPointF &p) {
    qDebug() <<"Chart::appendToSeries() "  ;
    m_series->append(p);
    // *m_series<<p;
}


/**
 * @brief Removes and deletes all series objects that have been added to the chart.
 */
void Chart::removeAllSeries() {
    qDebug() <<"Chart::removeAllSeries() "  ;

    if ( ! m_chart->series().empty() ) {
        qDebug() <<"Chart::removeAllSeries() - series count:"  << m_chart->series().size();
        qDebug() <<"Chart::removeAllSeries() - 1st series name:"  << m_chart->series().first()->name();
        m_chart->removeAllSeries();

        qDebug() <<"Chart::removeAllSeries() - series count:"  << m_chart->series().size();
    }


}




/**
 * @brief Creates default axes. Must be called AFTER loading a series to the chart
 */
void Chart::createDefaultAxes(){
    qDebug() << "Chart::createDefaultAxes()" ;
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

    qDebug() << "Chart::removeAllAxes()";

    if ( ! axes(Qt::Horizontal).isEmpty() )  {

        qDebug() << "Chart::removeAllAxes() - m_chart axes: "<< m_chart->axes(Qt::Horizontal).size();
        foreach ( QAbstractAxis *axe, axes(Qt::Horizontal) ) {
            m_chart->removeAxis(axe);
        }
    }

    if ( ! axes(Qt::Vertical).isEmpty() )  {
        qDebug() << "Chart::removeAllAxes() - m_chart axes: "<< m_chart->axes(Qt::Vertical).size();
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
    qDebug()<<"Chart::setAxisX() - Adding axis to chart";
    addAxis(axis, Qt::AlignBottom);
    qDebug()<<"Chart::setAxisX() - Attaching axis to series " << series->name();
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
    qDebug()<<"Chart::setAxisY() - Adding axis to chart";
    addAxis(axis, Qt::AlignLeft);
    qDebug()<<"Chart::setAxisY() - Attaching axis to series " << series->name();
    series->attachAxis(axis);

    // THIS IS QT_DEPRECATED
    // DO NOT USE THIS
    // m_chart->setAxisY(axis, series);

}



/**
 * @brief Add Axis axis to the QChart. Does not delete previously attached axis
 * @param axis
 * @param alignment
 */
void Chart::addAxis(QAbstractAxis *axis, Qt::Alignment alignment) {
    qDebug()<< "Chart::addAxis()";
    m_chart->addAxis(axis,alignment);
    // We could also check if m_series and do:
    // barSeries->attachAxis(axisY);
}



/**
 * @brief Set the range of the (first) horizontal axis
 * @param from
 * @param to
 */
void Chart::setAxisXRange(const QVariant &min, const QVariant &max){
    qDebug()<< "Chart::setAxisXRange()";
    m_chart->axes(Qt::Horizontal).first()->setRange(min, max);
}


/**
 * @brief Sets the minimum value shown on the horizontal axis.
 * @param from
 * @param to
 */
void Chart::setAxisXMin(const QVariant &min){
    qDebug()<< "Chart::setAxisXMin()";
    m_chart->axes(Qt::Horizontal).first()->setMin(min);
}



/**
 * @brief Set the range of the vertical axis
 * @param from
 * @param to
 */
void Chart::setAxisYRange(const QVariant &min, const QVariant &max){
    qDebug()<< "Chart::setAxisYRange()";
    m_chart->axes(Qt::Vertical).first()->setRange(min, max);
}



/**
 * @brief Sets the minimum value shown on the vertical axis.
 * @param from
 * @param to
 */
void Chart::setAxisYMin(const QVariant &min){
    qDebug()<< "Chart::setAxisYMin()";
    m_chart->axes(Qt::Vertical).first()->setMin(min);
}



void Chart::setAxisXLabelsAngle (const int &angle){
    qDebug()<< "Chart::setAxisXLabelsAngle()";
    m_chart->axes(Qt::Horizontal).first()->setLabelsAngle(angle);

}

/**
 * @brief Set the label font of the horizontal axis
 * @param font
 */
void Chart::setAxisXLabelFont(const QFont &font){
    qDebug()<< "Chart::setAxisXLabelFont()";
    m_chart->axes(Qt::Horizontal).first()->setLabelsFont(font);
}



/**
 * @brief Set the label font of the vertical axis
 * @param font
 */
void Chart::setAxisYLabelFont(const QFont &font){
    qDebug()<< "Chart::setAxisYLabelFont()";
    m_chart->axes(Qt::Vertical).first()->setLabelsFont(font);
}




/**
 * @brief Set the line pen of the horizontal axis
 * @param font
 */
void Chart::setAxisXLinePen(const QPen &pen){
    qDebug()<< "Chart::setAxisXLinePen()";
    m_chart->axes(Qt::Horizontal).first()->setLinePen(pen);
}



/**
 * @brief Set the line pen of the vertical axis
 * @param font
 */
void Chart::setAxisYLinePen(const QPen &pen){
    qDebug()<< "Chart::setAxisYLinePen()";
    m_chart->axes(Qt::Vertical).first()->setLinePen(pen);

}




/**
 * @brief Set the grid line pen of the horizontal axis
 * @param font
 */
void Chart::setAxisXGridLinePen(const QPen &pen){
    qDebug()<< "Chart::setAxisXGridLinePen()";
    m_chart->axes(Qt::Horizontal).first()->setGridLinePen(pen);
}



/**
 * @brief Set the grid line pen of the vertical axis
 * @param font
 */
void Chart::setAxisYGridLinePen(const QPen &pen){
    qDebug()<< "Chart::setAxisYGridLinePen()";
    m_chart->axes(Qt::Vertical).first()->setGridLinePen(pen);

}





/**
 * @brief Toggle the legend of the QChart
 * @param toggle
 */
void Chart::toggleLegend(const bool &toggle){
    qDebug()<< "Chart::toggleLegend()";
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
    qDebug()<< "Chart::setChartBackgroundBrush()";
   m_chart->setBackgroundBrush(brush);
}


/**
 * @brief Sets the background pen of the QChart
 * If no pen defined, it uses a transparent pen.
 * @param brush
 */
void Chart::setChartBackgroundPen(const QBrush & brush) {
    qDebug()<< "Chart::setChartBackgroundPen()";
   m_chart->setBackgroundBrush(brush);
}


/**
 * @brief Set the theme of the QChart
 * @param theme
 */
void Chart::setTheme(QChart::ChartTheme theme) {
    qDebug()<< "Chart::setTheme()";
    m_chart->setTheme(theme);
}


/**
 * @brief Set the theme for when our chart widget is small
 * @param chartHeight
 */
void Chart::setThemeSmallWidget(const int minWidth, const int minHeight) {
    qDebug()<< "Chart::setThemeSmallWidget()";
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
    qDebug()<< "Chart::setMargins()";
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
 * @brief Applies a simple theme to axes (default label fonts, line and grid line pen).
 * WARNING: Axes must be already attached to m_chart
 */
void Chart::setAxesThemeDefault() {
    qDebug()<< "Chart::setAxesThemeDefault()";
    setAxisXLabelFont();
    setAxisXLinePen();
    setAxisXGridLinePen();
    setAxisYLabelFont();
    setAxisYLinePen();
    setAxisYGridLinePen();

    setMargins(QMargins());

}


void Chart::resetToTrivial() {
    qDebug()<< "Chart::resetToTrivial()";
    removeAllSeries();
    addSeries();
    createDefaultAxes();
    axes(Qt::Horizontal).first()->setLabelsAngle(-90);
    setTitle("Chart", QFont("Times",8));

    setAxisXRange(0,1);
    setAxisYRange(0,1);

    setAxesThemeDefault();

}


QPixmap Chart::getPixmap()
{
    //create a temporary widget, which will display the chart
    QWidget* w = new QWidget;

    w->resize(1024, 768);
    QVBoxLayout *vl;
    vl = new QVBoxLayout(w);
    vl->addWidget(this);

    //show the widget, resized so we can grab it with correct dimensions
    w->show();

    // wait for the graph to be drawn otherwise we have size problems
    QTest::qWait(500);

    //save windows to a pixmap
    QPixmap pixmap = w->grab();

    //hide the widget
    w->hide();

    qDebug()<< "Chart::getPixmap() ends!";
    w->deleteLater();
    return pixmap;
}

