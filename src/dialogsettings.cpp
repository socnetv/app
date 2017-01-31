/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.3
 Written in Qt

                         dialogsettings.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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
#include <QMap>
#include <QtDebug>

DialogSettings::DialogSettings(
        QMap<QString, QString> &appSettings,
        QWidget *parent) :
    QDialog(parent),
    m_appSettings(appSettings),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

   // m_appSettings = appSettings; //only use if var passed by pointer

    //data export
    ui->dataDirEdit->setText(  (m_appSettings)["dataDir"]);

    //debugging
    ui->printDebugChkBox->setChecked(
                (appSettings["printDebug"] == "true") ? true:false
            );

    ui->progressBarsChkBox->setChecked(
                (appSettings["showProgressBar"] == "true") ? true:false
                );

    /**
     * canvas options
     */
    ui->antialiasingChkBox->setChecked(
                (appSettings["antialiasing"] == "true") ? true:false
                );
    ui->printLogoChkBox->setChecked(
                (appSettings["printLogo"] == "true") ? true:false
                );

    m_bgColor = QColor (m_appSettings["initBackgroundColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_bgColor );
    ui->bgColorButton->setIcon(QIcon(m_pixmap));

    ui->bgImageSelectEdit->setText((m_appSettings)["initBackgroundImage"]);

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
     * node options
     */
    m_nodeColor = QColor (m_appSettings["initNodeColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_nodeColor );
    ui->nodeColorBtn->setIcon(QIcon(m_pixmap));

    if (m_appSettings["initNodeShape"] == "box") {
        ui->nodeShapeRadioBox->setChecked(true);
    }
    else if (m_appSettings["initNodeShape"] == "circle") {
        ui->nodeShapeRadioCircle->setChecked(true);
    }
    else if (m_appSettings["initNodeShape"] == "diamond") {
        ui->nodeShapeRadioDiamond->setChecked(true);
    }
    else if (m_appSettings["initNodeShape"] == "ellipse") {
        ui->nodeShapeRadioEllipse->setChecked(true);
    }
    else if (m_appSettings["initNodeShape"] == "triangle") {
        ui->nodeShapeRadioTriangle->setChecked(true);
    }
    else if (m_appSettings["initNodeShape"] == "star") {
        ui->nodeShapeRadioStar->setChecked(true);
    }
    else { // default
       ui->nodeShapeRadioCircle->setChecked(true);
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

    ui->edgesChkBox-> setChecked(
                (m_appSettings["initEdgesVisibility"] == "true") ? true: false
                                                                  );

    ui->edgeArrowsChkBox-> setChecked(
                (m_appSettings["initEdgeArrows"] == "true") ? true: false
                                                                   );

    m_edgeColor = QColor (m_appSettings["initEdgeColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeColor );
    ui->edgeColorBtn->setIcon(QIcon(m_pixmap));

    m_edgeColorNegative = QColor (m_appSettings["initEdgeColorNegative"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeColorNegative );
    ui->edgeColorNegativeBtn ->setIcon(QIcon(m_pixmap));


    if (m_appSettings["initEdgeShape"] == "line") {
        ui->edgeShapeRadioStraightLine->setChecked(true);
    }
    else if (m_appSettings["initEdgeShape"] == "bezier") {
        ui->edgeShapeRadioBezier->setChecked(true);
    }
    else {
        ui->edgeShapeRadioStraightLine->setChecked(true);
    }


    ui->edgeWeightNumbersChkBox-> setChecked(
                (m_appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true: false
                                                                  );
    m_edgeWeightNumberColor = QColor (m_appSettings["initEdgeWeightNumberColor"]);
    m_pixmap = QPixmap(60,20) ;
    m_pixmap.fill( m_edgeWeightNumberColor );
    ui->edgeWeightNumberColorBtn->setIcon(QIcon(m_pixmap));

    ui->edgeWeightNumberSizeSpin->setValue( m_appSettings["initEdgeWeightNumberSize"].toInt(0, 10) );


    ui->edgeLabelsChkBox-> setChecked(
                (m_appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                                                                        );
    /**
     * dialog signals to slots
     */
    connect (ui->dataDirSelectButton, &QToolButton::clicked,
             this, &DialogSettings::getDataDir);

    connect (ui->printDebugChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setDebugMsgs);

    connect (ui->antialiasingChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setAntialiasing);

    connect (ui->printLogoChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setPrintLogo);

    connect (ui->bgColorButton, &QToolButton::clicked,
             this, &DialogSettings::getBgColor);

    connect (ui->bgImageSelectButton, &QToolButton::clicked,
             this, &DialogSettings::getBgImage);

    connect (ui->progressBarsChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setProgressBars);

    connect (ui->showToolBarChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setToolBar);

    connect (ui->showStatusBarChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setStatusBar);

    connect (ui->leftPanelChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setLeftPanel);

    connect (ui->rightPanelChkBox, &QCheckBox::stateChanged,
             this, &DialogSettings::setRightPanel);

    connect (ui->nodeShapeRadioBox, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);
    connect (ui->nodeShapeRadioCircle, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);
    connect (ui->nodeShapeRadioDiamond, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);
    connect (ui->nodeShapeRadioEllipse, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);
    connect (ui->nodeShapeRadioTriangle, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);
    connect (ui->nodeShapeRadioStar, &QRadioButton::clicked,
             this, &DialogSettings::getNodeShape);


    connect(ui->nodeSizeSpin, SIGNAL(valueChanged(int)),
            this, SLOT(getNodeSize(int)) );

    connect ( ui->buttonBox, &QDialogButtonBox::accepted,
              this, &DialogSettings::validateSettings );

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
    connect (ui->edgeShapeRadioStraightLine, &QRadioButton::clicked,
             this, &DialogSettings::getEdgeShape);
    connect (ui->edgeShapeRadioBezier, &QRadioButton::clicked,
             this, &DialogSettings::getEdgeShape);

    connect (ui->edgeWeightNumbersChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgeWeightNumbersVisibility);


    connect (ui->edgeLabelsChkBox, &QCheckBox::stateChanged,
                     this, &DialogSettings::getEdgeLabelsVisibility);

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
    }

}


/**
 * @brief DialogSettings::getBgColor
 * Opens a QColorDialog for the user to select a new bg color
 */
void DialogSettings::getBgColor(){

    m_bgColor = QColorDialog::getColor(
                m_bgColor, this, tr("Select a background color") );
    if ( m_bgColor.isValid()) {
        m_pixmap.fill(m_bgColor);
        ui->bgColorButton->setIcon(QIcon(m_pixmap));
        ui->bgImageSelectEdit->setText("");
        m_appSettings["initBackgroundColor"] = m_bgColor.name();
        m_appSettings["initBackgroundImage"] = "";
        emit setBgColor(m_bgColor);
    }
    else {
        // user pressed Cancel
    }

}



/**
 * @brief DialogSettings::getBgImage
 */
void DialogSettings::getBgImage(){
    QString m_bgImage = QFileDialog::getOpenFileName(
                this, tr("Select a background image "), (m_appSettings)["lastUsedDirPath"],
                tr("All (*);;PNG (*.png);;JPG (*.jpg)")
                );
    if (!m_bgImage.isEmpty() ) {
        (m_appSettings)["initBackgroundImage"] = m_bgImage ;
        ui->bgImageSelectEdit->setText((m_appSettings)["initBackgroundImage"]);
        emit setBgImage();
    }
    else { //user pressed Cancel

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
 * @brief DialogSettings::getNodeShape
 */
void DialogSettings::getNodeShape(){

    QString nodeShape;
    if ( ui->nodeShapeRadioBox->isChecked () ){
       m_appSettings["initNodeShape"]  = "box";
    }
    else if ( ui->nodeShapeRadioCircle->isChecked() ){
       m_appSettings["initNodeShape"]  = "circle";
    }
    else if ( ui->nodeShapeRadioDiamond->isChecked() ){
       m_appSettings["initNodeShape"]  = "diamond";
    }
    else if ( ui->nodeShapeRadioEllipse->isChecked() ){
        m_appSettings["initNodeShape"]  = "ellipse";
    }
    else if ( ui->nodeShapeRadioTriangle->isChecked() ){
        m_appSettings["initNodeShape"]  = "triangle";
    }
    else if ( ui->nodeShapeRadioStar->isChecked() ){
        m_appSettings["initNodeShape"]  = "star";
    }
    else {
        m_appSettings["initNodeShape"] = "box";
    }
    qDebug()<< "DialogSettings::getNodeShape() - new default shape " << nodeShape;
    emit setNodeShape(m_appSettings["initNodeShape"], 0);
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
        emit setNodeNumberColor(m_nodeNumberColor);
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
        (m_appSettings)["initEdgeColor"] = m_edgeColor.name();
        emit setEdgeColor(m_edgeColor, RAND_MAX);
    }
    else {
        // user pressed Cancel
    }
}


/**
 * @brief DialogSettings::getEdgeColorNegative
 * * Opens a QColorDialog for the user to select a new edge color
 */
void DialogSettings::getEdgeColorNegative(){
    m_edgeColorNegative = QColorDialog::getColor(
                m_edgeColorNegative, this, tr("Select color for negative Edges") );
    if ( m_edgeColorNegative.isValid()) {
        m_pixmap.fill(m_edgeColorNegative);
        ui->edgeColorNegativeBtn->setIcon(QIcon(m_pixmap));
        (m_appSettings)["initEdgeColorNegative"] = m_edgeColorNegative.name();
        emit setEdgeColor(m_edgeColorNegative, 0);
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
