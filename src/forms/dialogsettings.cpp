/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1.0-dev
 Written in Qt

                         dialogsettings.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
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

#include "dialogsettings.h"
#include "ui_dialogsettings.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QSpinBox>
#include <QTextStream>
#include <QPushButton>
#include <QMap>
#include <QtDebug>

#include <QGraphicsColorizeEffect>


#include "global.h"


SOCNETV_USE_NAMESPACE


DialogSettings::DialogSettings(QMap<QString, QString> &appSettings,
                                const QStringList &nodeShapeList,
                                const QStringList &iconPathList,
                                QWidget *parent) :
    QDialog(parent),
    m_appSettings(appSettings),
    m_shapeList(nodeShapeList),
    m_iconList(iconPathList),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

   // m_appSettings = appSettings; //only use if var passed by pointer

    //data export
    ui->dataDirEdit->setText(  (m_appSettings)["dataDir"]);

    ui->printLogoChkBox->setChecked(
                (appSettings["printLogo"] == "true") ? true:false
                );

    // reports
    ui->reportsRealNumberPrecisionSpin->
            setValue(m_appSettings["initReportsRealNumberPrecision"].toInt(0, 10) );

    ui->reportsLabelsLengthSpin->
            setValue(m_appSettings["initReportsLabelsLength"].toInt(0, 10) );


    QStringList chartTypesList;
    chartTypesList << "None" << "Lines" << "Area" << "Bars" ;
    ui->reportsChartTypeSelect->addItems(chartTypesList);

    switch (appSettings["initReportsChartType"].toInt()) {
    case ChartType::None:
        ui->reportsChartTypeSelect->setCurrentText( "None");
        break;
    case ChartType::Spline:
        ui->reportsChartTypeSelect->setCurrentText( "Lines");
        break;
    case ChartType::Area:
        ui->reportsChartTypeSelect->setCurrentText( "Area");
        break;
    case ChartType::Bars:
        ui->reportsChartTypeSelect->setCurrentText( "Bars");
        break;
    default:
        ui->reportsChartTypeSelect->setCurrentText( "Lines");
        break;
    }

    qDebug() << "reportsChartTypeSelect"
             << ui->reportsChartTypeSelect->currentText();


    //debugging
    ui->printDebugChkBox->setChecked(
                (appSettings["printDebug"] == "true") ? true:false
            );

    ui->progressDialogChkBox->setChecked(
                (appSettings["showProgressBar"] == "true") ? true:false
                );


    /**
      * Style options
      */
    ui->stylesheetDefaultChkBox->setChecked(true);


    /**
     * window options
     */
    ui->leftPanelChkBox->setChecked(
                ( appSettings["showLeftPanel"] == "true") ? true:false
                );

    ui->rightPanelChkBox->setChecked(
                ( appSettings["showRightPanel"] == "true") ? true:false
                );


    /**
     * GraphicsWidget (canvas) options
     */

    m_bgColor = QColor (m_appSettings["initBackgroundColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_bgColor );
    ui->bgColorButton->setIcon(QIcon(m_pixmap));

    ui->bgImageSelectEdit->setText((m_appSettings)["initBackgroundImage"]);

    ui->canvasUseOpenGLChkBox->setChecked(
                (appSettings["opengl"] == "true" ) ? true : false );
    ui->canvasAntialiasingChkBox->setChecked(
                (appSettings["antialiasing"] == "true") ? true:false
                );

    ui->canvasAntialiasingAutoAdjustChkBox->setChecked(
                (appSettings["canvasAntialiasingAutoAdjustment"] == "true") ? true:false
                );
    ui->canvasSmoothPixmapTransformChkBox->setChecked(
                (appSettings["canvasSmoothPixmapTransform"] == "true") ? true:false
                );

    ui->canvasSavePainterStateChkBox->setChecked(
                (appSettings["canvasPainterStateSave"] == "true") ? true:false
                );

    ui->canvasCacheBackgroundChkBox->setChecked(
                (appSettings["canvasCacheBackground"] == "true") ? true:false
                );
    ui->canvasEdgeHighlightingChkBox->setChecked(
                (appSettings["canvasEdgeHighlighting"] == "true") ? true:false
                );


    QStringList optionsList;
    optionsList << "Full" << "Minimal" << "Smart" << "Bounding Rectangle" << "None";
    ui->canvasUpdateModeSelect->addItems(optionsList);

    if ( appSettings["canvasUpdateMode"] == "Full" ) {
        ui->canvasUpdateModeSelect->setCurrentText( "Full");
    }
    else if (appSettings["canvasUpdateMode"] == "Minimal" ) {
        ui->canvasUpdateModeSelect->setCurrentText("Minimal" );
    }
    else if (appSettings["canvasUpdateMode"] == "Smart" ) {
        ui->canvasUpdateModeSelect->setCurrentText("Smart" );
    }
    else if (appSettings["canvasUpdateMode"] == "Bounding Rectangle" ) {
        ui->canvasUpdateModeSelect->setCurrentText("Bounding Rectangle" );
    }
    else if (appSettings["canvasUpdateMode"] == "None" ) {
        ui->canvasUpdateModeSelect->setCurrentText("None" );
    }
    else { //
        ui->canvasUpdateModeSelect->setCurrentText("Minimal" );
    }
    qDebug() << "canvasUpdateModeSelect" << appSettings["canvasUpdateMode"];


    optionsList.clear();
    optionsList << "BspTreeIndex" << "NoIndex" ;
    ui->canvasIndexMethodSelect->addItems(optionsList);

    if ( appSettings["canvasIndexMethod"] == "BspTreeIndex" ) {
        ui->canvasIndexMethodSelect->setCurrentText( "BspTreeIndex");
    }
    else if (appSettings["canvasIndexMethod"] == "NoIndex" ) {
        ui->canvasIndexMethodSelect->setCurrentText("NoIndex" );
    }
    else { //
        ui->canvasIndexMethodSelect->setCurrentText("BspTreeIndex" );
    }
    qDebug() << "canvasIndexMethodSelect" << appSettings["canvasIndexMethod"];






    /**
     * node options
     */
    m_nodeColor = QColor (m_appSettings["initNodeColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_nodeColor );
    ui->nodeColorBtn->setIcon(QIcon(m_pixmap));



    ui->nodeShapeComboBox->addItems(m_shapeList);

    for (int i = 0; i < m_shapeList.size(); ++i) {
       ui->nodeShapeComboBox->setItemIcon(i, QIcon(m_iconList[i]));
    }

    ui->nodeIconSelectButton->setEnabled(false);
    ui->nodeIconSelectEdit->setEnabled(false);

    int index = -1;
    if ( (index = m_shapeList.indexOf(m_appSettings["initNodeShape"])) != -1 ){

        ui->nodeShapeComboBox->setCurrentIndex(index);

        if ( index == NodeShape::Custom ) {

            ui->nodeShapeComboBox->setCurrentIndex(NodeShape::Custom);
            ui->nodeIconSelectButton->setEnabled(true);
            ui->nodeIconSelectEdit->setEnabled(true);
            ui->nodeIconSelectEdit->setText (m_appSettings["initNodeIconPath"]);
            if ( ! m_appSettings["initNodeIconPath"].isEmpty() ) {
                ui->nodeShapeComboBox->setItemIcon(
                            NodeShape::Custom,
                            QIcon(m_appSettings["initNodeIconPath"]));
            }
            else {
                QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
                effect->setColor(QColor("red"));
                ui->nodeIconSelectButton->setGraphicsEffect(effect);
                ui->nodeIconSelectEdit->setGraphicsEffect(effect);
                (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);
                (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
            }
        }
    }
    else {
        // default -- should never happen...
        ui->nodeShapeComboBox->setCurrentIndex(NodeShape::Circle);
    }


    ui->nodeSizeSpin->setValue( m_appSettings["initNodeSize"].toInt(0, 10) );

    ui->nodeNumbersChkBox->setChecked(
                ( m_appSettings["initNodeNumbersVisibility"] == "true") ? true : false
                );

    ui->nodeNumbersInsideChkBox->setChecked(
                (m_appSettings["initNodeNumbersInside"] == "true" ) ? true:false
                );
    if (m_appSettings["initNodeNumbersInside"] == "true") {
        ui->nodeNumberDistanceSpin->setEnabled(false);
        ui->nodeNumberSizeSpin->setValue( 0 );

    }
    m_nodeNumberColor = QColor (m_appSettings["initNodeNumberColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_nodeNumberColor );
    ui->nodeNumberColorBtn->setIcon(QIcon(m_pixmap));

    ui->nodeNumberSizeSpin->setValue( m_appSettings["initNodeNumberSize"].toInt(0, 10) );
    ui->nodeNumberDistanceSpin->setValue( m_appSettings["initNodeNumberDistance"].toInt(0, 10) );


    ui->nodeLabelsChkBox->setChecked(
                ( m_appSettings["initNodeLabelsVisibility"] == "true") ? true : false
                );

    ui->nodeLabelSizeSpin->setValue( m_appSettings["initNodeLabelSize"].toInt(0, 10) );

    m_nodeLabelColor = QColor (m_appSettings["initNodeLabelColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_nodeLabelColor );
    ui->nodeLabelColorBtn->setIcon(QIcon(m_pixmap));

    ui->nodeLabelDistanceSpin->setValue( m_appSettings["initNodeLabelDistance"].toInt(0, 10) );


    /**
     * edge options
     */

    ui->edgesChkBox->setChecked(
                (m_appSettings["initEdgesVisibility"] == "true") ? true: false
                                                                  );

    ui->edgeArrowsChkBox->setChecked(
                (m_appSettings["initEdgeArrows"] == "true") ? true: false
                                                                   );

    m_edgeColor = QColor (m_appSettings["initEdgeColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeColor );
    ui->edgeColorBtn->setIcon(QIcon(m_pixmap));


    m_edgeColorNegative = QColor (m_appSettings["initEdgeColorNegative"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeColorNegative );
    ui->edgeColorNegativeBtn->setIcon(QIcon(m_pixmap));


    m_edgeColorZero = QColor (m_appSettings["initEdgeColorZero"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeColorZero);
    ui->edgeColorZeroBtn->setIcon(QIcon(m_pixmap));



    if (m_appSettings["initEdgeShape"] == "line") {
        ui->edgeShapeRadioStraightLine->setChecked(true);
    }
    else if (m_appSettings["initEdgeShape"] == "bezier") {
        ui->edgeShapeRadioBezier->setChecked(true);
    }
    else {
        ui->edgeShapeRadioStraightLine->setChecked(true);
    }


    ui->edgeOffsetFromNodeSpin->setValue( m_appSettings["initEdgeOffsetFromNode"].toInt(0, 10) );

    ui->edgeWeightNumbersChkBox->setChecked(
                (m_appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true: false
                                                                  );
    m_edgeWeightNumberColor = QColor (m_appSettings["initEdgeWeightNumberColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeWeightNumberColor );
    ui->edgeWeightNumberColorBtn->setIcon(QIcon(m_pixmap));

    ui->edgeWeightNumberSizeSpin->setValue( m_appSettings["initEdgeWeightNumberSize"].toInt(0, 10) );


    ui->edgeLabelsChkBox->setChecked(
                (m_appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                                                                        );
    /**
     * dialog signals to slots
     */
    connect (ui->dataDirSelectButton, &QToolButton::clicked,
             this, &DialogSettings::getDataDir);

    connect (ui->printDebugChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setDebugMsgs);


    connect(ui->reportsRealNumberPrecisionSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getReportsRealNumberPrecision(int)) );

    connect (ui->reportsLabelsLengthSpin, SIGNAL(valueChanged(int)),
             this, SLOT(getReportsLabelsLength(int)));


    connect(ui->reportsChartTypeSelect, SIGNAL ( currentIndexChanged (const int &)),
          this, SLOT(getReportsChartType(const int &)) );


    connect (ui->printLogoChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setPrintLogo);


    connect( ui->stylesheetDefaultChkBox,&QCheckBox::clicked,
             this,  &DialogSettings::setStyleSheetDefault);


    connect (ui->progressDialogChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setProgressDialog);

    connect (ui->showToolBarChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setToolBar);

    connect (ui->showStatusBarChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setStatusBar);

    connect (ui->leftPanelChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setLeftPanel);

    connect (ui->rightPanelChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setRightPanel);


    connect (ui->bgColorButton, &QToolButton::clicked,
             this, &DialogSettings::getCanvasBgColor);

    connect (ui->bgImageSelectButton, &QToolButton::clicked,
             this, &DialogSettings::getCanvasBgImage);

    connect (ui->canvasUseOpenGLChkBox , &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasOpenGL);

    connect (ui->canvasAntialiasingChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasAntialiasing);

    connect (ui->canvasAntialiasingAutoAdjustChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasAntialiasingAutoAdjust);

    connect (ui->canvasSmoothPixmapTransformChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasSmoothPixmapTransform);


    connect (ui->canvasSavePainterStateChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasSavePainterState);

    connect (ui->canvasCacheBackgroundChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasCacheBackground);


    connect (ui->canvasEdgeHighlightingChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setCanvasEdgeHighlighting);


    connect(ui->canvasUpdateModeSelect, SIGNAL ( currentIndexChanged (const QString &)),
          this, SLOT(getCanvasUpdateMode(const QString &)) );


    connect(ui->canvasIndexMethodSelect, SIGNAL ( currentIndexChanged (const QString &)),
          this, SLOT(getCanvasIndexMethod(const QString &)) );



    connect (ui->nodeShapeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
             this, &DialogSettings::getNodeShapeIndex);

    connect (ui->nodeIconSelectButton, &QToolButton::clicked,
             this, &DialogSettings::getNodeIconFile);


    connect(ui->nodeSizeSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getNodeSize(int)) );

    connect (ui->nodeColorBtn, &QToolButton::clicked,
             this, &DialogSettings::getNodeColor);


    connect (ui->nodeNumbersChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getNodeNumbersVisibility);
    connect (ui->nodeNumbersInsideChkBox, &QCheckBox::stateChanged,
             this,  &DialogSettings::getNodeNumbersInside);
    connect (ui->nodeNumberColorBtn, &QToolButton::clicked,
             this, &DialogSettings::getNodeNumberColor);
    connect(ui->nodeNumberSizeSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getNodeNumberSize(int)) );
    connect(ui->nodeNumberDistanceSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getNodeNumberDistance(int)) );

    connect (ui->nodeLabelsChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getNodeLabelsVisibility);
    connect(ui->nodeLabelSizeSpin, SIGNAL(valueChanged(int)),
                this, SLOT(getNodeLabelSize(int)) );
    connect (ui->nodeLabelColorBtn, &QToolButton::clicked,
             this, &DialogSettings::getNodeLabelColor);
    connect(ui->nodeLabelDistanceSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getNodeLabelDistance(int)) );


    connect (ui->edgesChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgesVisibility);
    connect (ui->edgeArrowsChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgeArrowsVisibility);
    connect (ui->edgeColorBtn, &QToolButton::clicked,
             this, &DialogSettings::getEdgeColor);
    connect (ui->edgeColorNegativeBtn, &QToolButton::clicked,
             this, &DialogSettings::getEdgeColorNegative);
    connect (ui->edgeColorZeroBtn, &QToolButton::clicked,
             this, &DialogSettings::getEdgeColorZero);

    connect (ui->edgeShapeRadioStraightLine, &QRadioButton::clicked,
             this, &DialogSettings::getEdgeShape);
    connect (ui->edgeShapeRadioBezier, &QRadioButton::clicked,
             this, &DialogSettings::getEdgeShape);

    connect(ui->edgeOffsetFromNodeSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getEdgeOffsetFromNode(int)) );

    connect (ui->edgeWeightNumbersChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgeWeightNumbersVisibility);


    connect (ui->edgeLabelsChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgeLabelsVisibility);

    connect ( ui->buttonBox, &QDialogButtonBox::accepted,
              this, &DialogSettings::validateSettings );

}


/**
 * @brief DialogSettings::validateSettings
 * Validates form data and signals saveSettings to MW
 */
void DialogSettings::validateSettings(){
    emit saveSettings();

}

void DialogSettings::getDataDir(){

    QString m_dataDir = QFileDialog::getExistingDirectory(this, tr("Select a new data dir"),
                                                    ui->dataDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!m_dataDir.isEmpty()) {
        if (!m_dataDir.endsWith( QDir::separator() )) {
            m_dataDir += QDir::separator();
        }
       ui->dataDirEdit->setText(m_dataDir);
       m_appSettings["dataDir"]= m_dataDir;

       emit setReportsDataDir (m_dataDir);
    }

}




/**
 * @brief Get the real number precision
 * @param size
 */
void DialogSettings::getReportsRealNumberPrecision( const int &precision) {
    m_appSettings["initReportsRealNumberPrecision"]= QString::number(precision);
    emit setReportsRealNumberPrecision(precision);
}


/**
 * @brief Get the real number precision
 * @param size
 */
void DialogSettings::getReportsLabelsLength( const int &length) {
    m_appSettings["initReportsLabelsLength"]= QString::number(length);
    emit setReportsLabelLength(length);
}



/**
 * @brief Gets the chart type in reports
 */
void DialogSettings::getReportsChartType(const int &type){
    //if (!type.isEmpty() ) {
    qDebug() << "DialogSettings::getReportsChartType() - type: " << type;
        m_appSettings["initReportsChartType"] = QString::number(type-1);
        emit setReportsChartType(type-1);
    //}
}



/**
 * @brief DialogSettings::getCanvasBgColor
 * Opens a QColorDialog for the user to select a new bg color
 */
void DialogSettings::getCanvasBgColor(){

    m_bgColor = QColorDialog::getColor(
                m_bgColor, this, tr("Select a background color") );
    if ( m_bgColor.isValid()) {
        m_pixmap.fill(m_bgColor);
        ui->bgColorButton->setIcon(QIcon(m_pixmap));
        ui->bgImageSelectEdit->setText("");
        m_appSettings["initBackgroundColor"] = m_bgColor.name();
        m_appSettings["initBackgroundImage"] = "";
        emit setCanvasBgColor(m_bgColor);
    }
    else {
        // user pressed Cancel
    }

}



/**
 * @brief DialogSettings::getCanvasBgImage
 */
void DialogSettings::getCanvasBgImage(){
    QString m_bgImage = QFileDialog::getOpenFileName(
                this, tr("Select a background image "), (m_appSettings)["lastUsedDirPath"],
                tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                );
    if (!m_bgImage.isEmpty() ) {
        (m_appSettings)["initBackgroundImage"] = m_bgImage ;
        ui->bgImageSelectEdit->setText((m_appSettings)["initBackgroundImage"]);
        emit setCanvasBgImage();
    }
    else { //user pressed Cancel

    }

}




/**
 * @brief Gets Canvas Update Mode
 */
void DialogSettings::getCanvasUpdateMode(const QString &mode){
    if (!mode.isEmpty() ) {
        m_appSettings["canvasUpdateMode"] = mode;
        emit setCanvasUpdateMode(mode);
    }
}



/**
 * @brief Gets canvas Index Method
 */
void DialogSettings::getCanvasIndexMethod(const QString &method){
    if (!method.isEmpty() ) {
        m_appSettings["canvasIndexMethod"] = method;
        emit setCanvasIndexMethod(method);
    }
}


/**
 * @brief DialogSettings::getNodeColor
 * * Opens a QColorDialog for the user to select a new node color
 */
void DialogSettings::getNodeColor(){
    m_nodeColor = QColorDialog::getColor(
                m_nodeColor, this, tr("Select a color for Nodes") );
    if ( m_nodeColor.isValid()) {
        m_pixmap.fill(m_nodeColor);
        ui->nodeColorBtn->setIcon(QIcon(m_pixmap));
        (m_appSettings)["initNodeColor"] = m_nodeColor.name();
        emit setNodeColor(m_nodeColor);
    }
    else {
        // user pressed Cancel
    }
}



/**
 * @brief Gets the index of the selected shape in the ui::nodeShapeComboBox
 * If custom shape, it enables and sets the nodeIconSelectEdit/nodeIconSelectButton
 * Then it emits setNodeShape
 * @param shape
 */
void DialogSettings::getNodeShapeIndex(const int &shape){

    m_appSettings["initNodeShape"] = m_shapeList[shape];

    qDebug()<< "DialogSettings::getNodeShapeIndex() - "
               "new default shape"
            << m_shapeList[shape];

     if ( shape == NodeShape::Custom ) {

        // enable textedit and file button and raise file dialog
         ui->nodeIconSelectButton->setEnabled(true);
         ui->nodeIconSelectEdit->setEnabled(true);
         ui->nodeIconSelectEdit->setText (m_appSettings["initNodeIconPath"]);

         if (!m_appSettings["initNodeIconPath"].isEmpty()) {
             emit setNodeShape(0, m_appSettings["initNodeShape"], m_appSettings["initNodeIconPath"]);
         }
         else {
             QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
             effect->setColor(QColor("red"));
             ui->nodeIconSelectButton->setGraphicsEffect(effect);
             ui->nodeIconSelectEdit->setGraphicsEffect(effect);
             (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);
             (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
         }

     }
     else {
         ui->nodeIconSelectButton->setEnabled(false);
         ui->nodeIconSelectEdit->setEnabled(false);
         ui->nodeIconSelectEdit->setText ("");
         ui->nodeIconSelectButton->setGraphicsEffect(0);
         ui->nodeIconSelectEdit->setGraphicsEffect(0);
         (ui->buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);
         (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);

         // emit signal
         // instead of empty iconPath string, we always emit iconPathList[shape]
         // this is to allow passing the path to built-in icons i.e. hearts.
         emit setNodeShape(0, m_appSettings["initNodeShape"], m_iconList[shape] );
     }

}



void DialogSettings::getNodeIconFile(){

    QString m_nodeIconFile = QFileDialog::getOpenFileName(this, tr("Select a new icon"),
                                                    ui->nodeIconSelectEdit->text(),
                                                    tr("Images (*.png *.jpg *.jpeg *.svg);;All (*.*)")
                                                           );
    if (!m_nodeIconFile.isEmpty()) {
        qDebug() << m_nodeIconFile;
       ui->nodeIconSelectEdit->setText(m_nodeIconFile);
       m_appSettings["initNodeIconPath"]= m_nodeIconFile;
       ui->nodeShapeComboBox->setItemIcon(NodeShape::Custom, QIcon(m_nodeIconFile));
       (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(true);
       emit setNodeShape(0, m_appSettings["initNodeShape"],  m_appSettings["initNodeIconPath"]);
    }
    else {
        // user pressed Cancel ?
        // stop
        if ( ui->nodeIconSelectEdit->text().isEmpty() ) {
            (ui->buttonBox)->button (QDialogButtonBox::Cancel)->setDefault(true);
            (ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
        }
    }

}



/**
 * @brief DialogSettings::getNodeSize
 * @param size
 */
void DialogSettings::getNodeSize( int size) {
    m_appSettings["initNodeSize"]= QString::number(size);
    emit setNodeSize(size, false);
}


/**
 * @brief DialogSettings::getNodeNumbersVisibility
 * @param toggle
 */
void DialogSettings::getNodeNumbersVisibility (bool toggle){
    m_appSettings["initNodeNumbersVisibility"]= (toggle) ? "true" : "false";
    emit setNodeNumbersVisibility(toggle);
}

/**
 * @brief DialogSettings::getNodeNumbersInside
 * @param toggle
 */
void DialogSettings::getNodeNumbersInside(bool toggle) {
    m_appSettings["initNodeNumbersInside"]= (toggle) ? "true" : "false";
    ui->nodeNumbersChkBox->setChecked(true);
    ui->nodeNumberDistanceSpin->setEnabled(!toggle);
    ui->nodeNumberSizeSpin->setValue( ( (toggle) ? 0 : 7) );
    emit setNodeNumbersInside(toggle);
}

/**
 * @brief DialogSettings::getNodeNumberSize
 * @param size
 */
void DialogSettings::getNodeNumberSize( const int size) {
    m_appSettings["initNodeNumberSize"]= QString::number(size);
    emit setNodeNumberSize(0, size, false);
}

/**
 * @brief DialogSettings::getNodeNumberDistance
 * @param distance
 */
void DialogSettings::getNodeNumberDistance(const int distance) {
    m_appSettings["initNodeNumberDistance"]= QString::number(distance);
    emit setNodeNumberDistance(0, distance);
}

/**
 * @brief DialogSettings::getNodeNumberColor
 * * Opens a QColorDialog for the user to select a new node number color
 */
void DialogSettings::getNodeNumberColor(){
    m_nodeNumberColor = QColorDialog::getColor(
                m_nodeNumberColor, this, tr("Select color for Node Numbers") );
    if ( m_nodeNumberColor.isValid()) {
        m_pixmap.fill(m_nodeNumberColor);
        ui->nodeNumberColorBtn->setIcon(QIcon(m_pixmap));
        (m_appSettings)["initNodeNumberColor"] = m_nodeNumberColor.name();
        emit setNodeNumberColor(0,m_nodeNumberColor);
    }
    else {
        // user pressed Cancel
    }
}




/**
 * @brief DialogSettings::getNodeLabelsVisibility
 * @param toggle
 */
void DialogSettings::getNodeLabelsVisibility (bool toggle){
    m_appSettings["initNodeLabelsVisibility"]= (toggle) ? "true" : "false";
    emit setNodeLabelsVisibility(toggle);
}


/**
 * @brief DialogSettings::getNodeLabelColor
 * * Opens a QColorDialog for the user to select a new node Label color
 */
void DialogSettings::getNodeLabelColor(){
    m_nodeLabelColor = QColorDialog::getColor(
                m_nodeLabelColor, this, tr("Select color for Node Labels") );
    if ( m_nodeLabelColor.isValid()) {
        m_pixmap.fill(m_nodeLabelColor);
        ui->nodeLabelColorBtn->setIcon(QIcon(m_pixmap));
        (m_appSettings)["initNodeLabelColor"] = m_nodeLabelColor.name();
        emit setNodeLabelColor(m_nodeLabelColor);
    }
    else {
        // user pressed Cancel
    }
}

/**
 * @brief DialogSettings::getNodeLabelSize
 * @param size
 */
void DialogSettings::getNodeLabelSize( const int size) {
    m_appSettings["initNodeLabelSize"]= QString::number(size);
    emit setNodeLabelSize(0, size);
}


/**
 * @brief DialogSettings::getNodeLabelDistance
 * @param distance
 */
void DialogSettings::getNodeLabelDistance(const int distance) {
    m_appSettings["initNodeLabelDistance"]= QString::number(distance);
    emit setNodeLabelDistance(0, distance);
}




/**
 * @brief DialogSettings::getEdgesVisibility
 * @param toggle
 */
void DialogSettings::getEdgesVisibility (const bool &toggle){
    m_appSettings["initEdgesVisibility"]= (toggle) ? "true" : "false";
    emit setEdgesVisibility(toggle);
}




/**
 * @brief DialogSettings::getEdgeArrowsVisibility
 * @param toggle
 */
void DialogSettings::getEdgeArrowsVisibility(const bool &toggle){
    m_appSettings["initEdgeArrows"]= (toggle) ? "true" : "false";
    emit setEdgeArrowsVisibility(toggle);
}



/**
 * @brief DialogSettings::getEdgeColor
 * * Opens a QColorDialog for the user to select a new edge color
 */
void DialogSettings::getEdgeColor(){
    m_edgeColor = QColorDialog::getColor(
                m_edgeColor, this, tr("Select color for Edges ") );
    if ( m_edgeColor.isValid()) {
        m_pixmap.fill(m_edgeColor);
        ui->edgeColorBtn->setIcon(QIcon(m_pixmap));
        m_appSettings["initEdgeColor"] = m_edgeColor.name();
        emit setEdgeColor(m_edgeColor, RAND_MAX);
    }
    else {
        // user pressed Cancel
    }
}


/**
 * @brief DialogSettings::getEdgeColorNegative
 * * Opens a QColorDialog for the user to select a new negative edge color
 */
void DialogSettings::getEdgeColorNegative(){
    m_edgeColorNegative = QColorDialog::getColor(
                m_edgeColorNegative, this, tr("Select color for negative Edges") );
    if ( m_edgeColorNegative.isValid()) {
        m_pixmap.fill(m_edgeColorNegative);
        ui->edgeColorNegativeBtn->setIcon(QIcon(m_pixmap));
        m_appSettings["initEdgeColorNegative"] = m_edgeColorNegative.name();
        emit setEdgeColor(m_edgeColorNegative, -1);
    }
    else {
        // user pressed Cancel
    }
}



/**
 * @brief DialogSettings::getEdgeColorZero
 * * Opens a QColorDialog for the user to select a new zero edge color
 */
void DialogSettings::getEdgeColorZero(){
    m_edgeColorZero = QColorDialog::getColor(
                m_edgeColorZero, this, tr("Select color for negative Edges") );
    if ( m_edgeColorZero.isValid()) {
        m_pixmap.fill(m_edgeColorZero);
        ui->edgeColorZeroBtn->setIcon(QIcon(m_pixmap));
        m_appSettings["initEdgeColorZero"] = m_edgeColorZero.name();
        emit setEdgeColor(m_edgeColorZero, 0);
    }
    else {
        // user pressed Cancel
    }
}



/**
 * @brief DialogSettings::getEdgeShape
 */
void DialogSettings::getEdgeShape(){

    if ( ui->edgeShapeRadioStraightLine->isChecked () ){
       m_appSettings["initEdgeShape"]  = "line";
    }
    else if ( ui->edgeShapeRadioBezier->isChecked() ){
       m_appSettings["initEdgeShape"]  = "bezier";
    }
    qDebug()<< "DialogSettings::getEdgeShape() - new default shape " <<
               m_appSettings["initEdgeShape"];
    emit setEdgeShape(m_appSettings["initEdgeShape"], 0);
}




/**
 * @brief Changes the edge offset from source and target nodes
 * @param size
 */
void DialogSettings::getEdgeOffsetFromNode( int offset) {
    qDebug()<< "DialogSettings::getEdgeOffsetFromNode() - new offset:" << offset;
    m_appSettings["initEdgeOffsetFromNode"]= QString::number(offset);
    emit setEdgeOffsetFromNode(offset);
}



/**
 * @brief DialogSettings::getEdgeWeightNumbersVisibility
 * @param toggle
 */
void DialogSettings::getEdgeWeightNumbersVisibility(const bool &toggle){
    m_appSettings["initEdgeWeightNumbersVisibility"]= (toggle) ? "true" : "false";
    emit setEdgeWeightNumbersVisibility(toggle);
}



/**
 * @brief DialogSettings::getEdgeLabelsVisibility
 * @param toggle
 */
void DialogSettings::getEdgeLabelsVisibility(const bool &toggle){
    m_appSettings["initEdgeLabelsVisibility"]= (toggle) ? "true" : "false";
    emit setEdgeLabelsVisibility(toggle);
}



DialogSettings::~DialogSettings()
{
    delete ui;
}
