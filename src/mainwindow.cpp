/**
 * @file mainwindow.cpp
 * @brief Implements the MainWindow class, which serves as the primary interface for the SocNetV application.
 * @details This file contains the logic for the main application window, including menus, toolbars, and user interactions for network visualization and analysis.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtWidgets>
#include <QtGlobal>
#include <QtDebug>
#include <QPageSize>
#include <QPrintDialog>
#include <QProgressDialog>
#include <QKeySequence>
#include <QDateTime>

#include <QtSvg>  // for SVG icons
#include <QLoggingCategory>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QSplineSeries>
#include <QAreaSeries>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QBarSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QAbstractSeries>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextCodec>

#include "mainwindow.h"
#include "graph.h"
#include "texteditor.h"
#include "graphicswidget.h"
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "chart.h"

#include "forms/dialogpreviewfile.h"
#include "forms/dialogwebcrawler.h"
#include "forms/dialogdatasetselect.h"
#include "forms/dialogranderdosrenyi.h"
#include "forms/dialograndsmallworld.h"
#include "forms/dialograndscalefree.h"
#include "forms/dialograndregular.h"
#include "forms/dialograndlattice.h"
#include "forms/dialogexportpdf.h"
#include "forms/dialogexportimage.h"
#include "forms/dialognodefind.h"
#include "forms/dialognodeedit.h"
#include "forms/dialogfilternodesbycentrality.h"
#include "forms/dialogfilteredgesbyweight.h"
#include "forms/dialogedgedichotomization.h"
#include "forms/dialogsimilaritypearson.h"
#include "forms/dialogsimilaritymatches.h"
#include "forms/dialogclusteringhierarchical.h"
#include "forms/dialogdissimilarities.h"
#include "forms/dialogsettings.h"
#include "forms/dialogsysteminfo.h"



/**
 * @brief Constructs the MainWindow (MW) object
 *
 * @param m_fileName
 * @param forceProgress
 * @param maximized
 * @param fullscreen
 * @param debugLevel
 */
MainWindow::MainWindow(const QString & m_fileName, const bool &forceProgress, const bool &maximized, const bool &fullscreen, const int &debugLevel) {

    qDebug() << "=========== MainWindow (MW) constructor starting on thread:"<< thread();

    //
    // Setup debug messages/level
    //
    switch (debugLevel) {
    case 0:
        // Debugging disabled by command line parameter
        // Set messages pattern to trivial
        qSetMessagePattern("");
        // Disable debugging messages with filter rule
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                             "socnetv.debug=false");
        break;
    case 1:
        // Debugging set to minimum by command line parameter
        qSetMessagePattern("[%{type}] (%{file}:%{line}) %{function} - %{message}");
        break;
    case 2:
        // Debugging set to maximum by command line parameter
        qSetMessagePattern("[%{type} %{category}] %{time yyyyMMdd h:mm:ss.zzz t} %{threadid} (%{file}:%{line}) %{function} - %{message}");
        break;
    default:
        // No debug parameter -- set message pattern to bare minimum
        qSetMessagePattern("%{time} t:%{threadid} (%{file}:%{line}) %{function} - %{message}");
        break;
    }


    //
    // Setup window icon
    //
    setWindowIcon (QIcon(":/images/socnetv_logo_white_bg_128px.svg"));


    //
    // Initialize/load app settings and store them to memory
    //
    appSettings = initSettings(debugLevel, forceProgress);


    //
    // Initialize app window (minimum) size
    //

    // Get host screen width and height
    int primaryScreenWidth = QApplication::primaryScreen()->availableSize().width();
    int primaryScreenHeight = QApplication::primaryScreen()->availableSize().height();

    // Set a default min width and height
    int windowMinWidth = 1024;
    int windowMinHeight = 750;

    // For large screens, use more generous min height and width.
    if (primaryScreenWidth >= 1920) {
        windowMinWidth = 1440;
    }
    else if (primaryScreenWidth >= 1280) {
        windowMinWidth = 1024;
    }
    if (primaryScreenHeight >= 1440) {
        windowMinHeight = 960;
    }
    else if (primaryScreenHeight >= 1024) {
        windowMinHeight = 800;
    }


    qDebug() << "primaryScreen: "<<primaryScreenWidth<<"x"<<primaryScreenHeight
             << "Set Minimum MW size to:"<<windowMinWidth<<"x"<<windowMinHeight;

    // Set MW minimum size, before creating the graphics widget
    #ifdef Q_OS_LINUX
        setMinimumSize(windowMinWidth,windowMinHeight);
    #elif defined(Q_OS_MACOS)
        setMinimumSize(windowMinWidth,windowMinHeight);
    #elif defined(Q_OS_WIN)
        setMinimumSize(windowMinWidth,windowMinHeight);
    #else
        setMinimumSize(windowMinWidth,windowMinHeight);
    #endif

    //
    // Initialize devices
    //
    qDebug() << "Initialize devices...";

    // Create printer devices
    printer = new QPrinter;
    printerPDF = new QPrinter;

    // Create our network manager
    networkManager = new QNetworkAccessManager;

    // Store the max zoom index
    // The initial zoom will be the half of it
    // @see zoomSlider
    // @see GraphicsWidget::setInitZoomIndex()
    // @see GraphicsWidget::setMaxZoomIndex()
    maxZoomIndex = 1000;

    //
    // Initialize widgets
    //
    qDebug() << "Setup canvas, graph, widgets (actions, menus, panels, signal/slots) and init app...";
    initView();         // Init our network view

    initGraph();        // Init the graph model

    initActions();      // Register and construct menu Actions

    initMenuBar();      // Construct the menu

    initToolBar();      // Build the toolbar

    initPanels();       // Build the toolbox

    initWindowLayout(); // Init the application window, set layout etc

    initSignalSlots();  // Connect signals and slots between app components


    if (maximized) {
        qDebug() << "maximizing window as per user request.";
        showMaximized();
    }
    if (fullscreen) {
        showFullScreen();
    }



    initApp();          // Load and initialise default app parameters

    graphicsWidget->setFocus();

    //
    // Load user-provided network file, if any
    //
    qDebug() << "Checking if user provided file on startup...";
    if (!m_fileName.isEmpty()) {
        qDebug() << "Loading user provided file" << m_fileName;
        slotNetworkFileChoose( m_fileName );
    }

    QString welcomeMsg = tr("Welcome to %1, version %2").arg(qApp->applicationName(), VERSION);

    statusMessage( welcomeMsg );

    qDebug() << "@@@@ MW Constructor finished, on thread:" << thread();

}



/**
 * @brief Deletes variables on MW closing
 */
MainWindow::~MainWindow() {

    qDebug() << "Destructor for MW running...";

    // Init app to clear all maps etc.
    initApp();

    // Terminate any threads running
    terminateThreads("~MainWindow()");

    // Delete devices
    delete printer;
    delete printerPDF;

    delete scene;
    delete graphicsWidget;

    foreach ( TextEditor *ed, m_textEditors) {
        ed->close();
        delete ed;
    }

    m_textEditors.clear();

    codecs.clear();

    qDebug() << "Destruct function finished - bye!";
}






/**
 * @brief Called when the application closes. Asks to write any unsaved network data.
 * @param ce
 */
void MainWindow::closeEvent( QCloseEvent* ce ) {

    //
    // Show a status message
    //
    qDebug() << "Received close event. Show a status message to user...";
    statusMessage( tr("Closing SocNetV. Bye!") );

    //
    // Check if the graph has been saved
    //
    bool userCancelled=false;
    qDebug() << "Checking if current graph is saved...";
    if ( activeGraph->isSaved()  )  {
        ce->accept();
        qDebug() << "Graph is already saved. Nothing to do.";
    }
    else {
        qDebug() << "Graph NOT saved. Asking the user what to do.";
        switch( slotHelpMessageToUser(
                    USER_MSG_QUESTION,
                    tr("Save changes"),
                    tr("Modified network has not been saved!"),
                    tr("Do you want to save the changes to the network file?"),
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Cancel
                    ) )
        {
        case QMessageBox::Yes:
            slotNetworkSave();
            ce->accept();
            break;
        case QMessageBox::No:
            ce->accept();
            break;
        case QMessageBox::Cancel:
            ce->ignore();
            userCancelled = true;
            break;
        case QMessageBox::NoButton:
        default: // just for sanity
            ce->ignore();
            break;
        }
    }
    if (userCancelled) {
        qDebug() << "User canceled (while saving graph). Returning without closing the app.";
        return;
    }

    //
    // Terminate running threads
    //
    qDebug() << "I will terminate any running threads...";
    terminateThreads("closeEvent()");

    //
    // Delete other objects and pointers
    //
    qDebug() << "Deleting other objects and pointers...";

    qDebug() << "Deleting printer";
    delete printer;
    qDebug() << "Deleting printerPDF";
    delete printerPDF;
    qDebug() << "Deleting graphicsWidget";
    delete graphicsWidget;
    qDebug() << "Deleting activeGraph";
    delete activeGraph;
    qDebug() << "Deleting Scene";
    delete scene;

//    delete miniChart;

    qDebug() << "Clearing and deleting text editors...";
    foreach ( TextEditor *ed, m_textEditors) {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();

    qDebug() <<" Checking if networkManager thread is running...";
    if (networkManager->thread()->isRunning()) {
        qDebug() << "networkManager thread running"
                 << "Calling deleteLater();";
        networkManager->deleteLater();
    }

    delete editNodePropertiesAct;
    delete editNodeRemoveAct;

    qDebug() << "Clearing codecs...";
    codecs.clear();

    qDebug() << "Finished. Bye!";
}




/**
 * @brief Terminates any remaining threads.
 *
 * @param reason
 */
void MainWindow::terminateThreads(const QString &reason) {
    qDebug() << "Terminating threads (those started from MW). Reason:" << reason
             <<" Checking if graphThread is running...";
    if (graphThread.isRunning() ) {
        qDebug() << "graphThread running."
                 << "Calling graphThread.quit();";
        graphThread.quit();
        qDebug() << "deleting activeGraph and pointer";
        delete activeGraph;
        activeGraph = 0;  // see why here: https://goo.gl/tQxpGA
    }

}




/**
 * @brief Called whenever the app window is resized.
 */
void MainWindow::resizeEvent( QResizeEvent *e ) {

    Q_UNUSED(e);
//    int w0=e->oldSize().width();
//    int h0=e->oldSize().height();
//    int w=width();
//    int h=height();

//    qDebug () << "MW resized:" << w0 << "x" << h0
//              << "-->" << w << "x" << h;

//    statusMessage(
//                 tr("Window resized to (%1, %2)px.")
//                .arg(w).arg(h)
//                );

}



/**
  * @brief Reads user-defined settings (or uses defaults) and initializes some app settings
  */
QMap<QString,QString> MainWindow::initSettings(const int &debugLevel, const bool &forceProgress) {

    qDebug() << "Initializing settings - debugLevel"<<debugLevel;

    //
    // Read used-defined settings or use defaults
    //

    // App settings are always saved to this folder.
    settingsDir = QDir::homePath() +QDir::separator() + "socnetv-data" + QDir::separator() ;
    settingsFilePath = settingsDir + "settings.conf";

    // dataDir is where our built-in datasets and reports are saved by default
    // initially dataDir and settingsDir are the same, but dataDir may be
    // changed by the user through Settings...
    QString dataDir= settingsDir ;

    // hard-coded default settings to use only on first app load,
    // when there are no user defined values
    appSettings["initNodesEstimatedSize"] = "5000";
    appSettings["initEdgesPerNodeEstimatedSize"] = "500";
    appSettings["initNodeSize"]= "10";
    appSettings["initNodeColor"]="red";
    appSettings["initNodeShape"]="circle";
    appSettings["initNodeIconPath"]="";

    appSettings["initNodeNumbersVisibility"] = "true";
    appSettings["initNodeNumberSize"]="0";
    appSettings["initNodeNumberColor"]="#333";
    appSettings["initNodeNumbersInside"] = "true";
    appSettings["initNodeNumberDistance"] = "2";

    appSettings["initNodeLabelsVisibility"] = "false";
    appSettings["initNodeLabelSize"]="8";
    appSettings["initNodeLabelColor"]="#8d8d8d";
    appSettings["initNodeLabelDistance"] = "6";

    appSettings["initEdgesVisibility"]="true";
    appSettings["initEdgeShape"]="line"; //bezier
    appSettings["initEdgeColor"]="#666666";
    appSettings["initEdgeColorNegative"]="red";
    appSettings["initEdgeColorZero"]="blue";
    appSettings["initEdgeArrows"]="true";
    appSettings["initEdgeOffsetFromNode"] = "7";
    appSettings["initEdgeThicknessPerWeight"]="true";
    appSettings["initEdgeWeightNumbersVisibility"]="false";
    appSettings["initEdgeWeightNumberSize"] = "7";
    appSettings["initEdgeWeightNumberColor"] = "#00aa00";
    appSettings["initEdgeLabelsVisibility"] = "false";

    appSettings["initBackgroundColor"]="white"; //"gainsboro";
    appSettings["initBackgroundImage"]="";
    appSettings["printDebug"] = "false";
    appSettings["viewReportsInSystemBrowser"] = "true";
    appSettings["showProgressBar"] = "true";
    appSettings["showToolBar"] = "true";
    appSettings["showStatusBar"] = "true";
    appSettings["useCustomStyleSheet"]="true";
    appSettings["opengl"] = "true";
    appSettings["antialiasing"] = "true";
    appSettings["canvasAntialiasingAutoAdjustment"] = "true";
    appSettings["canvasSmoothPixmapTransform"] = "true";
    appSettings["canvasPainterStateSave"] = "false";
    appSettings["canvasCacheBackground"] = "false";
    appSettings["canvasUpdateMode"] = "Full";
    appSettings["canvasIndexMethod"] = "BspTreeIndex";
    appSettings["canvasEdgeHighlighting"] = "true";
    appSettings["canvasNodeHighlighting"] = "true";
    appSettings["dataDir"]= dataDir ;
    appSettings["lastUsedDirPath"]= dataDir ;
    appSettings["showRightPanel"] = "true";
    appSettings["showLeftPanel"] = "true";
    appSettings["printLogo"] = "true";
    appSettings["initStatusBarDuration"] = "5000";
    appSettings["randomErdosEdgeProbability"] = "0.04";
    appSettings["initReportsRealNumberPrecision"] = "6";
    appSettings["initReportsLabelsLength"] = "16";
    appSettings["initReportsChartType"] = "0";

    appSettings["saveZeroWeightEdges"] = "false";

    // Try to load settings from previously-saved file
    // First check if our settings folder exist
    QDir socnetvDir(settingsDir);
    if ( !socnetvDir.exists() ) {
        qDebug() << "socnetv settings dir does not exist. Creating it...";
        socnetvDir.mkdir(settingsDir);
    }
    // Then check if the settings file exists inside the folder
    if (!socnetvDir.exists(settingsFilePath)) {
        qDebug() << "Settings file does not exist. Creating it with defaults at: "
                  << settingsFilePath;
        saveSettings();
    }
    else {
        qDebug()<< "Settings file exist. Reading it...";
        QFile file(settingsFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug () << "Could not open (for reading) file:" << settingsFilePath;
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error loading settings file"),
                                  tr("Error loading settings"),
                                  tr("Error! \n"
                                     "I cannot read the settings file "
                                     "in \n %1 \n"
                                     "You can continue using SocNetV with default "
                                     "settings but any changes to them will not "
                                     " be saved for future sessions \n"
                                     "Please, check permissions in your home folder "
                                     " and contact the developer team."
                                     ).arg(settingsFilePath.toLocal8Bit())
                                  );
            return appSettings;
        }
        // Read the previously-stored settings from the file and update appSettings
        QTextStream in(&file);
        QStringList setting;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty()) {
                setting = line.simplified().split('=');
                if (setting[0].simplified().startsWith("recentFile_")) {
                    recentFiles += setting[1].simplified();
                }
                else {
                    appSettings.insert (setting[0].simplified() , setting[1].simplified() );
                }
            }
        }
        file.close();
    }

    // Override progress bar setting if the user has requested it (through a command-line parameter)
    appSettings["showProgressBar"] = forceProgress ? "true" : appSettings["showProgressBar"];

    // Override debug messages setting if the user has requested it (through a command-line parameter)
    if (debugLevel > 0 ) {
        appSettings["printDebug"] = "true";
    }
    else if (debugLevel==0) {
        appSettings["printDebug"] = "false";
        slotOptionsDebugMessages(false);
    }
    else {
       // do not override appSettings["printDebug"]
    }

    if ( appSettings["printDebug"] == "true") {
        slotOptionsDebugMessages(true);
    }
    else {
        slotOptionsDebugMessages(false);
    }


    //
    // Create fortune cookies and tips
    //
    createFortuneCookies();
    slotHelpCreateTips();

    //
    // Populate icons and shapes lists
    //
    // Note: When you add a new shape and icon, you must also:
    // 1. Add a new enum in NodeShape (global.h)
    // 2. Add a new branch in GraphicsNode::setShape() and paint()
    // 3. Add a new branch in DialogNodeEdit: getNodeShape() and getUserChoices()
    nodeShapeList  << "box"
                   << "circle"
                   << "diamond"
                   << "ellipse"
                   << "triangle"
                   << "star"
                   << "person"
                   << "person-b"
                   << "bugs"
                   << "heart"
                   << "dice"
                   << "custom";

    iconPathList << ":/images/box.png"
                 << ":/images/circle.png"
                 << ":/images/diamond.png"
                 << ":/images/ellipse.png"
                 << ":/images/triangle.png"
                 << ":/images/star.png"
                 << ":/images/person.svg"
                 << ":/images/person-bw.svg"
                 << ":/images/bugs.png"
                 << ":/images/heart.svg"
                 << ":/images/random.png"
                 << ":/images/export_photo_48px.svg";


    //Max nodes used by createRandomNetwork dialogues
    maxRandomlyCreatedNodes=5000;

    //
    // Initialize list of supported text codecs and prepare the preview file dialog
    //
    qDebug() << "initializing text codecs list.." ;
    initNetworkAvailableTextCodecs();

    qDebug() << "creating preview file dialog and passing the codecs list: " << codecs ;
    m_dialogPreviewFile = new DialogPreviewFile(this);
    m_dialogPreviewFile->setCodecList(codecs);

    connect (m_dialogPreviewFile, &DialogPreviewFile::loadNetworkFileWithCodec,
             this, &MainWindow::slotNetworkFileLoad );

    // return the setting
    return appSettings;
}




/**
 * @brief Saves default (or user-defined) app settings
 */
void MainWindow::saveSettings() {
    qDebug() << "Saving app settings to file: "<< settingsFilePath;
    QFile file(settingsFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text ) ) {
        qDebug () << "Could not open (for writing) file:" << settingsFilePath;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error writing settings file"),
                              tr("Error writing settings"),
                              tr("I cannot write the settings file "
                                 "in \n %1 \n"
                                 "You can continue using SocNetV with default "
                                 "settings but any changes to them will not "
                                 " be saved for future sessions \n"
                                 "Please, check permissions in your home folder "
                                 " and contact the developer team."
                                 ).arg(settingsFilePath.toLocal8Bit())
                              );
        return;
    }

    QTextStream out(&file);
    qDebug()<< "Writing settings to settings file first ";
    QMap<QString, QString>::const_iterator it = appSettings.constBegin();
    while (it != appSettings.constEnd()) {
        // qDebug() << "   setting: " <<  it.key() << " = " << it.value();
        out << it.key() << " = " << it.value() << "\n";
        ++it;
    }


    // save recent files
    for (int i = 0 ; i < recentFiles.size() ; ++i) {
        out << "recentFile_"+ QString::number(i+1)
            << " = "
            << recentFiles.at(i) << "\n";
    }

    file.close();

}




/**
 * @brief Opens the Settings dialog
 */
void MainWindow::slotOpenSettingsDialog() {

    // build dialog

    m_settingsDialog = new DialogSettings( appSettings, nodeShapeList, iconPathList, this);

    connect( m_settingsDialog, &DialogSettings::saveSettings,
             this, &MainWindow::saveSettings);

    connect (m_settingsDialog, &DialogSettings::setReportsDataDir,
             activeGraph, &Graph::setReportsDataDir);

    connect (m_settingsDialog,&DialogSettings::setReportsRealNumberPrecision,
             activeGraph, &Graph::setReportsRealNumberPrecision);

    connect (m_settingsDialog,&DialogSettings::setReportsLabelLength,
             activeGraph, &Graph::setReportsLabelLength);

    connect (m_settingsDialog, &DialogSettings::setReportsChartType,
             activeGraph, &Graph::setReportsChartType);

    connect( m_settingsDialog, &DialogSettings::setDebugMsgs,
             this, &MainWindow::slotOptionsDebugMessages);

    connect( m_settingsDialog, &DialogSettings::setProgressDialog,
             this, &MainWindow::slotOptionsProgressDialogVisibility);

    connect( m_settingsDialog, &DialogSettings::setPrintLogo,
             this, &MainWindow::slotOptionsEmbedLogoExporting);

    connect (m_settingsDialog, &DialogSettings::setCustomStylesheet,
             this, &MainWindow::slotOptionsCustomStylesheet);

    connect( m_settingsDialog, &DialogSettings::setToolBar,
             this, &MainWindow::slotOptionsWindowToolbarVisibility);

    connect( m_settingsDialog, &DialogSettings::setStatusBar,
             this, &MainWindow::slotOptionsWindowStatusbarVisibility);

    connect( m_settingsDialog, &DialogSettings::setLeftPanel,
             this, &MainWindow::slotOptionsWindowLeftPanelVisibility);

    connect( m_settingsDialog, &DialogSettings::setRightPanel,
             this, &MainWindow::slotOptionsWindowRightPanelVisibility);

    connect( m_settingsDialog, &DialogSettings::setCanvasBgColor,
             this, &MainWindow::slotOptionsBackgroundColor);

    connect( m_settingsDialog, &DialogSettings::setCanvasBgImage,
             this, &MainWindow::slotOptionsBackgroundImage);

    connect( m_settingsDialog, &DialogSettings::setCanvasOpenGL,
             this, &MainWindow::slotOptionsCanvasOpenGL);

    connect( m_settingsDialog, &DialogSettings::setCanvasAntialiasing,
             this, &MainWindow::slotOptionsCanvasAntialiasing);

    connect( m_settingsDialog, &DialogSettings::setCanvasAntialiasingAutoAdjust,
             this, &MainWindow::slotOptionsCanvasAntialiasingAutoAdjust);

    connect( m_settingsDialog, &DialogSettings::setCanvasSmoothPixmapTransform,
             this, &MainWindow::slotOptionsCanvasSmoothPixmapTransform);

    connect( m_settingsDialog, &DialogSettings::setCanvasSavePainterState,
             this, &MainWindow::slotOptionsCanvasSavePainterState);

    connect( m_settingsDialog, &DialogSettings::setCanvasCacheBackground,
             this, &MainWindow::slotOptionsCanvasCacheBackground);


    connect( m_settingsDialog, &DialogSettings::setCanvasEdgeHighlighting,
             this, &MainWindow::slotOptionsCanvasEdgeHighlighting);


    connect( m_settingsDialog, &DialogSettings::setCanvasUpdateMode,
             this, &MainWindow::slotOptionsCanvasUpdateMode);


    connect( m_settingsDialog, &DialogSettings::setCanvasIndexMethod,
             this, &MainWindow::slotOptionsCanvasIndexMethod);

    connect(m_settingsDialog, SIGNAL(setNodeColor(QColor)),
            this, SLOT(slotEditNodeColorAll(QColor)) );

    connect( m_settingsDialog, &DialogSettings::setNodeShape,
             this, &MainWindow::slotEditNodeShape);

    connect( m_settingsDialog, &DialogSettings::setNodeSize,
             this, &MainWindow::slotEditNodeSizeAll);

    connect( m_settingsDialog, &DialogSettings::setNodeNumbersVisibility,
             this, &MainWindow::slotOptionsNodeNumbersVisibility);

    connect( m_settingsDialog, &DialogSettings::setNodeNumbersInside,
             this, &MainWindow::slotOptionsNodeNumbersInside);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberColor,
             this, &MainWindow::slotEditNodeNumbersColor);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberSize,
             this, &MainWindow::slotEditNodeNumberSize);

    connect( m_settingsDialog, &DialogSettings::setNodeNumberDistance,
             this, &MainWindow::slotEditNodeNumberDistance);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelsVisibility,
             this, &MainWindow::slotOptionsNodeLabelsVisibility);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelSize,
             this, &MainWindow::slotEditNodeLabelSize);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelColor,
             this, &MainWindow::slotEditNodeLabelsColor);

    connect( m_settingsDialog, &DialogSettings::setNodeLabelDistance,
             this, &MainWindow::slotEditNodeLabelDistance);

    connect( m_settingsDialog, &DialogSettings::setEdgesVisibility,
             this, &MainWindow::slotOptionsEdgesVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeArrowsVisibility,
             this, &MainWindow::slotOptionsEdgeArrowsVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeOffsetFromNode,
             this, &MainWindow::slotOptionsEdgeOffsetFromNode);

    connect( m_settingsDialog, &DialogSettings::setEdgeColor,
             this, &MainWindow::slotEditEdgeColorAll);

    connect( m_settingsDialog, &DialogSettings::setEdgeWeightNumbersVisibility,
             this, &MainWindow::slotOptionsEdgeWeightNumbersVisibility);

    connect( m_settingsDialog, &DialogSettings::setEdgeLabelsVisibility,
             this, &MainWindow::slotOptionsEdgeLabelsVisibility);

    connect( m_settingsDialog, &DialogSettings::setSaveZeroWeightEdges,
             this, &MainWindow::slotOptionsSaveZeroWeightEdges);

    // show settings dialog
    m_settingsDialog->exec();


}



/**
 * @brief Fixes known bugs in QProgressDialog class.
   i.e. Workaround for macOS-only Qt bug: QTBUG-65750, QTBUG-70357.
   QProgressDialog too small and too narrow to fit the text of its label
 * @param dialog
 */
void MainWindow::polishProgressDialog(QProgressDialog* dialog)
{
#ifdef Q_OS_MAC
    // Workaround for macOS-only Qt bug; see: QTBUG-65750, QTBUG-70357.
    const int margin = dialog->fontMetrics().maxWidth();
    dialog->resize(dialog->width() + 2 * margin, dialog->height());
    dialog->show();
#else
    Q_UNUSED(dialog);
#endif
}



/**
 * @brief Initializes our graphics widget, the canvas where we draw networks
 *
 * The widget is a QGraphicsView, with a scene, and is the 'main' widget of the application.
 */
void MainWindow::initView() {

    qDebug()<< "Creating graphics widget...";

    // Create our scene
    scene=new QGraphicsScene();

    // Create a view widget and pass the scene and the our object as parent
    graphicsWidget=new GraphicsWidget(scene,this);
    graphicsWidget->setObjectName("graphicsWidget");

    bool toggle = false;

    toggle = (appSettings["opengl"] == "true" ) ? true:false;
    graphicsWidget->setOptionsOpenGL(toggle);

    toggle = (appSettings["antialiasing"] == "true" ) ? true:false;
    graphicsWidget->setOptionsAntialiasing(toggle);

    //Disables QGraphicsView's antialiasing auto-adjustment of exposed areas.
    toggle = (appSettings["canvasAntialiasingAutoAdjustment"] == "true" ) ? false:true;
    graphicsWidget->setOptionsNoAntialiasingAutoAdjust(toggle);

    toggle = (appSettings["canvasSmoothPixmapTransform"] == "true" ) ? true:false;
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle );

    //if items do restore their state, it's not needed for graphicsWidget to do the same...
    toggle = (appSettings["canvasPainterStateSave"] == "true" ) ? false:true;
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, toggle);

    if ( appSettings["canvasUpdateMode"] == "Full" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::FullViewportUpdate );
    }
    else if (appSettings["canvasUpdateMode"] == "Minimal" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::MinimalViewportUpdate );
    }
    else if (appSettings["canvasUpdateMode"] == "Smart" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::SmartViewportUpdate );
    }
    else if (appSettings["canvasUpdateMode"] == "Bounding Rectangle" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::BoundingRectViewportUpdate );
    }
    else if (appSettings["canvasUpdateMode"] == "None" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::NoViewportUpdate );
    }
    else { //
        graphicsWidget->setViewportUpdateMode( QGraphicsView::MinimalViewportUpdate );
    }

    //QGraphicsView can cache pre-rendered content in a QPixmap, which is then drawn onto the viewport.
    if ( appSettings["canvasCacheBackground"] == "true" ) {
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
    }
    else {
        graphicsWidget->setCacheMode(QGraphicsView::CacheNone);
    }

    graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    //graphicsWidget->setTransformationAnchor(QGraphicsView::NoAnchor);
    graphicsWidget->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // sets dragging the mouse over the scene while the left mouse button is pressed.
    graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);

    graphicsWidget->setFocusPolicy(Qt::StrongFocus);
    graphicsWidget->setFocus();

    graphicsWidget->setWhatsThis(tr("<p><b>The canvas of SocNetV</b></p>"
                                    "<p>Inside this area you create and edit networks, "
                                    "load networks from files and visualize them "
                                    "according to the selected metrics. </p>"
                                    "<p>To create a new node, <em>double-click</em> anywhere.</p>"
                                    "<p>To add an edge between two nodes, <em>double-click</em>"
                                    " on the first node (source) then double-click on the second (target) .</p>"
                                    "<p>To move around the canvas, use the keyboard arrows.</p>"
                                    "<p>To change network appearance, <em>right click on empty space</em>. </p>"
                                    "<p>To edit the properties of a node, <em>right-click</em> on it. </p>"
                                    "<p>To edit the properties of an edge, <em>right-click</em> on it.</p>")
                                 );

    qDebug() << "Finished initialization of graphics widget. Dimensions:"
             << graphicsWidget->width() << "x" << graphicsWidget->height();
}




/**
 * @brief Initializes the Graph
 */
void MainWindow::initGraph() {

    qDebug() << "creating activeGraph object...";

    bool ok1;
    nodesEstimatedSize = (appSettings["initNodesEstimatedSize"]).toInt(&ok1, 10);
    if ( !ok1 ) {
        nodesEstimatedSize = 0;
    }

    bool ok2;
    edgesPerNodeEstimatedSize  = (appSettings["initEdgesPerNodeEstimatedSize"]).toInt(&ok2, 10);
    if ( !ok2 ) {
        edgesPerNodeEstimatedSize = 0;
    }

    activeGraph = new Graph(nodesEstimatedSize, edgesPerNodeEstimatedSize);

    qDebug() << "activeGraph created on thread:" << activeGraph->thread()
             << "moving it to new thread ";

    activeGraph->moveToThread(&graphThread);

    qDebug() << "activeGraph moved to thread:" << activeGraph->thread()
             << "starting new activeGraph thread...";

    graphThread.start();

    qDebug() << "activeGraph thread now:" << activeGraph->thread();

}

/**
 * @brief Initializes all QActions of the application
 *
 * Take a breath, the listing below is HUGE.
 *
 */
void MainWindow::initActions(){

    qDebug()<< "initializing actions...";

    /**
    Network menu actions
    */
    networkNewAct = new QAction(QIcon(":/images/new_folder_48px.svg"), tr("&New"),  this);
    networkNewAct->setShortcut(Qt::CTRL | Qt::Key_N);
    networkNewAct->setStatusTip(tr("Create a new network"));
    networkNewAct->setToolTip(tr("New network"));
    networkNewAct->setWhatsThis(tr("New\n\n"
                                "Creates a new social network. "
                                "First, checks if current network needs to be saved."));
    connect(networkNewAct, SIGNAL(triggered()), this, SLOT(slotNetworkNew()));

    networkOpenAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&Open"), this);
    networkOpenAct->setShortcut(Qt::CTRL | Qt::Key_O);
    networkOpenAct->setToolTip(tr("Open network"));
    networkOpenAct->setStatusTip(tr("Open a GraphML formatted file of social network data."));
    networkOpenAct->setWhatsThis(tr("Open\n\n"
                                 "Opens a file of a social network in GraphML format"));
    connect(networkOpenAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileChoose()));


    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(slotNetworkFileLoadRecent()));
    }

    networkImportGMLAct = new QAction( QIcon(":/images/open_48px.svg"), tr("&GML"), this);
    networkImportGMLAct->setStatusTip(tr("Import GML-formatted file"));
    networkImportGMLAct->setWhatsThis(tr("Import GML\n\n"
                                      "Imports a social network from a GML-formatted file"));
    connect(networkImportGMLAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportGML()));


    networkImportPajekAct = new QAction( QIcon(":/images/open_48px.svg"), tr("&Pajek"), this);
    networkImportPajekAct->setStatusTip(tr("Import Pajek-formatted file"));
    networkImportPajekAct->setWhatsThis(tr("Import Pajek \n\n"
                                        "Imports a social network from a Pajek-formatted file"));
    connect(networkImportPajekAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportPajek()));


    networkImportAdjAct = new QAction( QIcon(":/images/open_48px.svg"), tr("&Adjacency Matrix"), this);
    networkImportAdjAct->setStatusTip(tr("Import Adjacency matrix"));
    networkImportAdjAct->setWhatsThis(tr("Import Sociomatrix \n\n"
                                     "Imports a social network from an Adjacency matrix-formatted file"));
    connect(networkImportAdjAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportAdjacency()));

    networkImportGraphvizAct = new QAction( QIcon(":/images/open_48px.svg"), tr("Graph&Viz (.dot)"), this);
    networkImportGraphvizAct->setStatusTip(tr("Import dot file"));
    networkImportGraphvizAct->setWhatsThis(tr("Import GraphViz \n\n"
                                      "Imports a social network from a GraphViz formatted file"));
    connect(networkImportGraphvizAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportGraphviz()));


    networkImportUcinetAct = new QAction( QIcon(":/images/open_48px.svg"), tr("&UCINET (.dl)..."), this);
    networkImportUcinetAct->setStatusTip(tr("ImportDL-formatted file (UCINET)"));
    networkImportUcinetAct->setWhatsThis(tr("Import UCINET\n\n"
                                     "Imports social network data from a DL-formatted file"));
    connect(networkImportUcinetAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportUcinet()));


    networkImportListAct = new QAction( QIcon(":/images/open_48px.svg"), tr("&Edge list"), this);
    networkImportListAct->setStatusTip(tr("Import an edge list file. "));
    networkImportListAct->setWhatsThis(
                tr("Import edge list\n\n"
                   "Import a network from an edgelist file. "
                   "SocNetV supports EdgeList files with edge weights "
                   "as well as simple EdgeList files where the edges are non-value (see manual)"
                   ));
    connect(networkImportListAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportEdgeList()));


    networkImportTwoModeSM = new QAction( QIcon(":/images/open_48px.svg"), tr("&Two Mode Sociomatrix"), this);
    networkImportTwoModeSM->setStatusTip(tr("Import two-mode sociomatrix (affiliation network) file"));
    networkImportTwoModeSM->setWhatsThis(tr("Import Two-Mode Sociomatrix \n\n"
                                            "Imports a two-mode network from a sociomatrix file. "
                                            "Two-mode networks are described by affiliation "
                                            "network matrices, where A(i,j) codes the "
                                            "events/organizations each actor is affiliated with."));
    connect(networkImportTwoModeSM, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportTwoModeSM()));


    networkSaveAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&Save"),  this);
    networkSaveAct->setShortcut(QKeySequence::Save);
    networkSaveAct->setStatusTip(tr("Save social network to a file"));
    networkSaveAct->setWhatsThis(tr("Save.\n\n"
                                 "Saves the social network to file"));
    connect(networkSaveAct, SIGNAL(triggered()), this, SLOT(slotNetworkSave()));

    networkSaveAsAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("Save As..."),  this);
    networkSaveAsAct->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    networkSaveAsAct->setStatusTip(tr("Save network under a new filename"));
    networkSaveAsAct->setWhatsThis(tr("Save As\n\n"
                                   "Saves the social network under a new filename"));
    connect(networkSaveAsAct, SIGNAL(triggered()), this, SLOT(slotNetworkSaveAs()));

    networkExportImageAct = new QAction(QIcon(":/images/export_photo_48px.svg"), tr("Export to I&mage..."), this);
    networkExportImageAct->setStatusTip(tr("Export the visible part of the network to image"));
    networkExportImageAct->setWhatsThis(tr("Export to Image\n\n"
                                      "Exports the visible part of the current social network to an image"));
    connect(networkExportImageAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportImageDialog()));

    networkExportPDFAct = new QAction( QIcon(":/images/export_pdf_48px.svg"), tr("E&xport to PDF..."), this);
    networkExportPDFAct->setStatusTip(tr("Export the visible part of the network to a PDF file"));
    networkExportPDFAct->setWhatsThis(tr("Export to PDF\n\n"
                                      "Exports the visible part of the current social network to a PDF document."));
    connect(networkExportPDFAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportPDFDialog()));

    networkExportSMAct = new QAction( QIcon(":/images/file_download_48px.svg"), tr("&Adjacency Matrix"), this);
    networkExportSMAct->setStatusTip(tr("Export social network to an adjacency/sociomatrix file"));
    networkExportSMAct->setWhatsThis(tr("Export network to Adjacency format\n\n"
                                     "Exports the social network to an "
                                     "adjacency matrix-formatted file"));
    connect(networkExportSMAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportSM()));

    networkExportPajek = new QAction( QIcon(":/images/file_download_48px.svg"), tr("&Pajek"), this);
    networkExportPajek->setStatusTip(tr("Export social network to a Pajek-formatted file"));
    networkExportPajek->setWhatsThis(tr("Export Pajek \n\n"
                                        "Exports the social network to a Pajek-formatted file"));
    connect(networkExportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkExportPajek()));


    networkExportListAct = new QAction( QIcon(":/images/file_download_48px.svg"), tr("&List"), this);
    networkExportListAct->setStatusTip(tr("Export to List-formatted file. "));
    networkExportListAct->setWhatsThis(tr("Export List\n\n"
                                       "Exports the network to a List-formatted file"));
    connect(networkExportListAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportList()));

    networkExportDLAct = new QAction( QIcon(":/images/file_download_48px.svg"), tr("&DL..."), this);
    networkExportDLAct->setStatusTip(tr("Export network to UCINET-formatted file"));
    networkExportDLAct->setWhatsThis(tr("Export UCINET\n\n"
                                     "Exports the active network to a DL-formatted"));
    connect(networkExportDLAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportDL()));

    networkExportGWAct = new QAction( QIcon(":/images/file_download_48px.svg"), tr("&GW..."), this);
    networkExportGWAct->setStatusTip(tr("Export to GW-formatted file"));
    networkExportGWAct->setWhatsThis(tr("Export\n\n"
                                     "Exports the active network to a GW formatted file"));
    connect(networkExportGWAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportGW()));

    networkCloseAct = new QAction(QIcon(":/images/close_24px.svg"), tr("&Close"), this);
    networkCloseAct->setShortcut(QKeySequence::Close);
    networkCloseAct->setStatusTip(tr("Close the actual network"));
    networkCloseAct->setWhatsThis(tr("Close \n\nCloses the actual network"));
    connect(networkCloseAct, SIGNAL(triggered()), this, SLOT(slotNetworkClose()));

    networkPrintAct = new QAction(QIcon(":/images/print_48px.svg"), tr("&Print"), this);
    networkPrintAct->setShortcut(QKeySequence::Print);
    networkPrintAct->setStatusTip(tr("Send the currrent social network to the printer"));
    networkPrintAct->setWhatsThis(tr("Print \n\n"
                                  "Sends whatever is viewable on "
                                  "the canvas to your printer. \n"
                                  "To print the whole social network, "
                                  "you might want to zoom-out."));
    connect(networkPrintAct, SIGNAL(triggered()), this, SLOT(slotNetworkPrint()));

    networkQuitAct = new QAction(QIcon(":/images/exit_24px.svg"), tr("E&xit"), this);
    networkQuitAct->setShortcut(QKeySequence::Quit);
    networkQuitAct->setStatusTip(tr("Quit SocNetV. Are you sure?"));
    networkQuitAct->setWhatsThis(tr("Exit\n\n"
                                 "Quits the application"));
    connect(networkQuitAct, SIGNAL(triggered()), this, SLOT(close()));


    openTextEditorAct = new QAction(QIcon(":/images/text_edit_48px.svg"),
                                    tr("Open &Text Editor"),this);
    openTextEditorAct->setShortcut(Qt::SHIFT | Qt::Key_F5);
    openTextEditorAct->setStatusTip(tr("Open a text editor "
                                       "to take notes, copy/paste network data, etc"));
    openTextEditorAct->setWhatsThis(
                tr("<p><b>Text Editor</b></p>"
                   "<p>Opens a simple text editor where you can "
                   "copy paste network data, of any supported format, "
                   "and save to a file. Then you can import that file to SocNetV. </p>"));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotNetworkTextEditor()));


    networkViewFileAct = new QAction(QIcon(":/images/code_48px.svg"),
                                     tr("&View Loaded File"),this);
    networkViewFileAct->setShortcut(Qt::Key_F5);
    networkViewFileAct->setStatusTip(tr("Display the loaded social network file."));
    networkViewFileAct->setWhatsThis(tr("View Loaded File\n\n"
                                        "Displays the loaded social network file "));
    connect(networkViewFileAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileView()));

    networkViewSociomatrixAct = new QAction(QIcon(":/images/sociomatrix_48px.svg"),
                                            tr("View &Adjacency Matrix"),  this);
    networkViewSociomatrixAct->setShortcut(Qt::Key_F6);
    networkViewSociomatrixAct->setStatusTip(tr("Display the adjacency matrix of the network."));
    networkViewSociomatrixAct->setWhatsThis(
                tr("<p><b>View Adjacency Matrix</b></p>"
                   "<p>Displays the adjacency matrix of the active network. </p>"
                   "<p>The adjacency matrix of a social network is a matrix "
                   "where each element a(i,j) is equal to the weight "
                   "of the arc from actor (node) i to actor j. "
                   "<p>If the actors are not connected, then a(i,j)=0. </p>"));
    connect(networkViewSociomatrixAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrix()));


    networkViewSociomatrixPlotAct = new QAction(QIcon(":/images/adjacencyplot.png"),
                                                tr("P&lot Adjacency Matrix (text)"),  this);
    networkViewSociomatrixPlotAct->setShortcut(Qt::SHIFT | Qt::Key_F6);
    networkViewSociomatrixPlotAct->setStatusTip(
                tr("Plots the adjacency matrix in a text file using unicode characters."));
    networkViewSociomatrixPlotAct->setWhatsThis(
                tr("<p><b>Plot Adjacency Matrix (text)</b></p>"
                   "<p>Plots the adjacency matrix in a text file using "
                   "unicode characters. </p>"
                   "<p>In every element (i,j) of the \"image\", "
                   "a black square means actors i and j are connected"
                   "whereas a white square means they are disconnected.</p>"
                   ));
    connect(networkViewSociomatrixPlotAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrixPlotText()));


    networkDataSetSelectAct = new QAction(QIcon(":/images/science_48px.svg"),
                                          tr("Create From &Known Data Sets"),  this);
    networkDataSetSelectAct->setShortcut(Qt::Key_F7);
    networkDataSetSelectAct->setStatusTip(
                tr("Load one of the \'famous\' social network data sets included in SocNetV."));
    networkDataSetSelectAct->setWhatsThis(
                tr("<p><b>Famous Data Sets</b></p>"
                   "<p>SocNetV includes a number of known "
                   "(also called famous) data sets in Social Network Analysis, "
                   "such as Krackhardt's high-tech managers, etc. "
                   "Click this menu item or press F7 to load a data set.</p> "
                   ));
    connect(networkDataSetSelectAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkDataSetSelect()));




    networkRandomScaleFreeAct = new QAction(
                QIcon(":/images/scalefree.png"), tr("Scale-free"),	this);

    networkRandomScaleFreeAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_S)
                );
    networkRandomScaleFreeAct->setStatusTip(
                tr("Create a random network with a power-law degree distribution."));
    networkRandomScaleFreeAct->setWhatsThis(
                tr("<p><b>Scale-free (power-law)</b></p>"
                   "<p>A scale-free network is a network whose degree distribution "
                   "follows a power law."
                   " SocNetV generates random scale-free networks according to the "
                   " Barabási–Albert (BA) model using a preferential attachment mechanism.</p>"));
    connect(networkRandomScaleFreeAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomScaleFreeDialog()));


    networkRandomSmallWorldAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"),	this);
    networkRandomSmallWorldAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_M)
                );
    networkRandomSmallWorldAct->setStatusTip(tr("Create a small-world random network, according to the Watts & Strogatz model."));
    networkRandomSmallWorldAct->setWhatsThis(
                tr("<p><b>Small World </b></p>"
                   "<p>Creates a random small-world network, according to the "
                   "Watts & Strogatz model. </p>"
                   "<p>A small-world network has short average path lengths and "
                   "high clustering coefficient.</p>"));
    connect(networkRandomSmallWorldAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomSmallWorldDialog()));


    networkRandomErdosRenyiAct = new QAction(QIcon(":/images/erdos.png"),
                                             tr("Erdős–Rényi"),  this);
    networkRandomErdosRenyiAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_E)
                );
    networkRandomErdosRenyiAct->setStatusTip(
                tr("Create a random network according to the Erdős–Rényi model"));
    networkRandomErdosRenyiAct->setWhatsThis(
                tr("<p><b>Erdős–Rényi </b></p>"
                   "<p>Creates a random network either of G(n, p) model or G(n,M) model. </p>"
                   "<p>The former model creates edges with Bernoulli trials (probability p).</p>"
                   "<p>The latter creates a graph of exactly M edges.</p>"));
    connect(networkRandomErdosRenyiAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomErdosRenyiDialog()));





    networkRandomLatticeAct = new QAction(QIcon(":/images/lattice.png"), tr("Lattice"), this);
    networkRandomLatticeAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_T)
                );
    networkRandomLatticeAct->setStatusTip(tr("Create a lattice network."));
    networkRandomLatticeAct->setWhatsThis(
                tr("<p><b>Lattice </b></p>"
                   "<p>Creates a random lattice network</p>"
                    "<p>A lattice is a network whose drawing forms a regular tiling. "
                    "Lattices are also known as meshes or grids.</p>"
                ));
    connect(networkRandomLatticeAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomLatticeDialog()));


    networkRandomRegularSameDegreeAct = new QAction(QIcon(":/images/net.png"), tr("d-Regular"), this);
    networkRandomRegularSameDegreeAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_R)
                );
    networkRandomRegularSameDegreeAct->setStatusTip(
                tr("Create a d-regular random network, "
                   "where every actor has the same degree d."));
    networkRandomRegularSameDegreeAct->setWhatsThis(
                tr("<p><b>d-Regular</b></p>"
                   "<p>Creates a random network where each actor has the same "
                   "number <em>d</em> of neighbours, aka the same degree d.</p>"));
    connect(networkRandomRegularSameDegreeAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomRegularDialog()));



    networkRandomLatticeRingAct = new QAction( QIcon(":/images/net1.png"),
                                               tr("Ring Lattice"), this);
    networkRandomLatticeRingAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_L)
                );
    networkRandomLatticeRingAct->setStatusTip(tr("Create a ring lattice random network."));
    networkRandomLatticeRingAct->setWhatsThis(
                tr("<p><b>Ring Lattice </b></p>"
                   "<p>Creates a ring lattice random network. </p>"
                   "<p>A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side.</p>"));
    connect(networkRandomLatticeRingAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomRingLattice()));



    networkRandomGaussianAct = new QAction(tr("Gaussian"),	this);
    networkRandomGaussianAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_G)
                );
    networkRandomGaussianAct->setStatusTip(tr("Create a Gaussian distributed random network."));
    networkRandomGaussianAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(networkRandomGaussianAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomGaussian()));




    networkWebCrawlerAct = new QAction(QIcon(":/images/webcrawler_48px.svg"), tr("&Web Crawler"),	this);
    networkWebCrawlerAct->setShortcut(Qt::SHIFT | Qt::Key_C);
    networkWebCrawlerAct->setEnabled(true);
    networkWebCrawlerAct->setStatusTip(tr("Use the web crawler to create a network from all links found in a given website"));
    networkWebCrawlerAct->setWhatsThis(
                tr("<p><b>Web Crawler </b></p>"
                   "<p>Creates a network of linked webpages, starting "
                   "from an initial webpage using the built-in Web Crawler. </p>"
                   "<p>The web crawler visits the given URL (website or webpage) "
                   "and parses its contents to find links to other pages (internal or external). "
                   "If there are such links, it adds them to a queue of URLs. "
                   "Then, all the URLs in the queue list are visited in a FIFO order "
                   "and parsed to find more links which are also added to the url queue. "
                   "The process repeats until it reaches user-defined "
                   "limits: </p>"
                   "<p>Maximum urls to visit (max nodes in the resulting network)</p> "
                   "<p>Maximum links per page</p>"
                   "<p>Except the initial url and the limits, you can also "
                   "specify patterns of urls to include or exclude, "
                   "types of links to follow (internal, external or both) as well as "
                   "if you want delay between requests (strongly advised)</p>."));

    connect(networkWebCrawlerAct, SIGNAL(triggered()), this, SLOT(slotNetworkWebCrawlerDialog()));


    /**
    Edit menu actions
    */


    editMouseModeInteractiveAct = new QAction(QIcon(":/images/cursor-pointer.svg"),
                                  tr("Select/Move"),  this);
    editMouseModeInteractiveAct->setCheckable(true);
    editMouseModeInteractiveAct->setChecked(true);
    editMouseModeInteractiveAct->setToolTip(tr("<p><b>Mouse mode: Interactive</b></p> "
                                          "<p>In this interactive mode, you can click on nodes/edges and move them around with your mouse. </p>"
                                           "<p>Also, you can select multiple items with a rubber band selection area. To move the canvas, use the keyboard arrows.</p>"));
    editMouseModeInteractiveAct->setStatusTip(tr("Enable the interactive mouse mode to be able to click and move items and select them with a rubber band."));
    editMouseModeInteractiveAct->setWhatsThis(tr("<p><b>Mouse Mode: Interactive</b></p>"
                                    "<p>In this mode, you can interact with the items on the canvas using the mouse: </p>"
                                    "<p>a) double-click to create new nodes, "
                                    "<p>b) left-click or right-click on items (i.e. nodes, edges) to edit their properties</p>"
                                    "<p>c) move nodes by dragging them with your mouse.  </p>"
                                    "<p>d) select multiple items with a rubber band.</p>"
                                    "<p>To move the canvas (up/down, left/right), use the keyboard arrows."));


    editMouseModeScrollAct = new QAction(QIcon(":/images/cursor-hand-drag.svg"),
                                  tr("Scroll/Pan"),  this);
    editMouseModeScrollAct->setCheckable(true);
    editMouseModeScrollAct->setChecked(false);
    editMouseModeScrollAct->setToolTip(tr("<p><b>Mouse mode: Scrolling</b></p> "
                                         "<p>In this non-interactive mode, you can easily scroll the canvas by dragging the mouse around. All mouse actions are disabled.</p>"));
    editMouseModeScrollAct->setStatusTip(tr("Enable this non-interactive mode to easily scroll the canvas by dragging the mouse around."));
    editMouseModeScrollAct->setWhatsThis(tr("<p><b>Mouse mode: Scrolling</b></p>"
                                        "<p>In this mode, you cannot interact with the canvas using the mouse.</p>"
                                        "<p>The cursor changes into a pointing hand, and dragging the mouse around will only scroll the scrolbars.</p> "
                                        "<p>You will not be able to select any items or move them around with the mouse.</p>"
                                        "<p>Note: You will still be able to edit the network using the menu or the toolbar actions and icons.</p>"));



    editRelationNextAct = new QAction(QIcon(":/images/chevron_right_48px.svg"),
                                      tr("Next Relation"),  this);
    editRelationNextAct->setShortcut(Qt::CTRL | Qt::Key_Right);
    editRelationNextAct->setToolTip(tr("Goto the next relation of the network (if any)."));
    editRelationNextAct->setStatusTip(tr("Goto the next relation of the network (if any)."));
    editRelationNextAct->setWhatsThis(tr("Next Relation\n\nLoads the next relation of the network (if any)"));
    editRelationNextAct->setEnabled(false);

    editRelationPreviousAct = new QAction(QIcon(":/images/chevron_left_48px.svg"),
                                          tr("Previous Relation"),  this);
    editRelationPreviousAct->setShortcut(Qt::CTRL | Qt::Key_Left);
    editRelationPreviousAct->setToolTip(
                tr("Goto the previous relation of the network (if any)."));
    editRelationPreviousAct->setStatusTip(
                tr("Goto the previous relation of the network (if any)."));
    editRelationPreviousAct->setWhatsThis(
                tr("Previous Relation\n\n"
                   "Loads the previous relation of the network (if any)"));
    editRelationPreviousAct->setEnabled(false);

    editRelationAddAct = new QAction(QIcon(":/images/add_48px.svg"),
                                     tr("Add New Relation"),  this);
    editRelationAddAct->setShortcut(Qt::ALT | Qt::CTRL | Qt::Key_N);
    editRelationAddAct->setToolTip(
                tr("Add a new relation to the network. Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setStatusTip(
                tr("Add a new relation to the network. Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setWhatsThis(
                tr("Add New Relation\n\n"
                   "Adds a new relation to the active network. "
                   "Nodes will be preserved, edges will be removed. "));

    editRelationRenameAct = new QAction(QIcon(":/images/relation_edit_48px.svg"),
                                        tr("Rename Relation"),  this);
    editRelationRenameAct->setToolTip(tr("Rename current relation"));
    editRelationRenameAct->setStatusTip(tr("Rename the current relation of the network."));
    editRelationRenameAct->setWhatsThis(tr("Rename Relation\n\n"
                                           "Renames the current relation of the network."));


    zoomInAct = new QAction(QIcon(":/images/zoom_in_24px.svg"), tr("Zoom In"), this);
    zoomInAct->setShortcut(Qt::CTRL | Qt::Key_Plus);
    zoomInAct->setStatusTip(tr("Zoom In.\n\nZooms in the network")) ;
    zoomInAct->setWhatsThis(tr("Zoom in the network. Alternatives: use the canvas button, or press Ctrl++, or use mouse wheel while pressing Ctrl."));


    zoomOutAct = new QAction(QIcon(":/images/zoom_in_24px.svg"), tr("Zoom Out"), this);
    zoomOutAct->setShortcut(Qt::CTRL | Qt::Key_Minus);
    zoomOutAct->setStatusTip(tr("Zoom Out.\n\nZooms out of the actual network"));
    zoomOutAct->setWhatsThis(tr("Zoom out the network. Alternatives: use the canvas button, or press Ctrl+-, or use mouse wheel while pressing Ctrl."));


    editRotateLeftAct = new QAction(QIcon(":/images/rotate_left_48px.svg"), tr("Rotate counterclockwise"), this);
    editRotateLeftAct->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::LeftArrow);
    editRotateLeftAct->setStatusTip(tr("Rotate counterclockwise. You can also use the button underneath the canvas."));
    editRotateLeftAct->setWhatsThis(tr("Rotates the network counterclockwise. You can also use the far left button below the canvas."));


    editRotateRightAct = new QAction(QIcon(":/images/rotate_right_48px.svg"), tr("Rotate clockwise"), this);
    editRotateRightAct->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::RightArrow);
    editRotateRightAct->setStatusTip(tr("Rotate clockwise. You can also use the button underneath the canvas."));
    editRotateRightAct->setWhatsThis(tr("Rotates the network clockwise. You can also use the far right button below the canvas."));


    editResetSlidersAct = new QAction(QIcon(":/images/refresh_48px.svg"), tr("Reset Zoom and Rotation"), this);
    editResetSlidersAct ->setShortcut(Qt::CTRL | Qt::Key_0);
    editResetSlidersAct->setStatusTip(tr("Reset zoom and rotation to zero."));
    editResetSlidersAct->setWhatsThis(tr("Resets any zoom and rotation transformations to zero."));



    editNodeSelectAllAct = new QAction(QIcon(":/images/select_all_48px.svg"), tr("Select All"), this);
    editNodeSelectAllAct->setShortcut(QKeySequence::SelectAll);
    editNodeSelectAllAct->setStatusTip(tr("Select all nodes"));
    editNodeSelectAllAct->setWhatsThis(tr("Select All\n\nSelects all nodes and edges in the network"));
    connect(editNodeSelectAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectAll()));

    editNodeSelectNoneAct = new QAction(QIcon(":/images/select_none_48px.svg"), tr("Select None"), this);
    editNodeSelectNoneAct->setShortcut(tr("Ctrl+Alt+A"));
    editNodeSelectNoneAct->setStatusTip(tr("Deselect all nodes and edges"));
    editNodeSelectNoneAct->setWhatsThis(tr("Deselect all\n\n Clears the node selection"));
    connect(editNodeSelectNoneAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectNone()));

    editNodeFindAct = new QAction(QIcon(":/images/search_48px.svg"), tr("Find Nodes "), this);
    editNodeFindAct->setShortcut(QKeySequence::Find);
    editNodeFindAct->setToolTip(tr("Find and select one or more nodes by their number or label."));
    editNodeFindAct->setStatusTip(tr("Find and select one or more nodes by their number or label."));
    editNodeFindAct->setWhatsThis(tr("Find Node\n\n"
                                     "Finds one or more nodes by their number or label and "
                                     "highlights them by doubling its size. "));
    connect(editNodeFindAct, SIGNAL(triggered()), this, SLOT(slotEditNodeFindDialog()) );

    editNodeAddAct = new QAction(QIcon(":/images/node_add_48px.svg"), tr("Add Node"), this);
    editNodeAddAct->setShortcut(tr("Ctrl+."));
    editNodeAddAct->setStatusTip(tr("Add a new node to the network in a random position. Alternately, double-click on a specific position the canvas. "));
    editNodeAddAct->setToolTip(
                tr("Add a new node to the network in a random position.\n\n"
                   "Alternately, create a new node by double-clicking on a specific position the canvas. ")
                );
    editNodeAddAct->setWhatsThis(
                tr("Add new node\n\n"
                   "Add a new node to the network in a random position. \n\n"
                   "Alternately, you can create a new node by double-clicking on a specific position the canvas.")
                );

    connect(editNodeAddAct, SIGNAL(triggered()), this, SLOT(slotEditNodeAdd()));

    editNodeRemoveAct = new QAction(QIcon(":/images/node_remove_48px.svg"),tr("Remove Node"), this);
    editNodeRemoveAct->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Period);
    //Single key shortcuts with backspace or del do no work in Mac http://goo.gl/7hz7Dx
    editNodeRemoveAct->setToolTip(tr("Remove selected node(s). \n\n"
                                     "If no nodes are selected, you will be prompted for a node number. "));

    editNodeRemoveAct->setStatusTip(tr("Remove selected node(s). If no nodes are selected, you will be prompted for a node number. "));
    editNodeRemoveAct->setWhatsThis(
                tr("Remove node\n\n"
                   "Removes selected node(s) from the network. \n"
                   "Alternately, you can remove a node by right-clicking on it. \n"
                   "If no nodes are selected, you will be prompted for a node number. ")
                );

    connect(editNodeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditNodeRemove()));

    editNodePropertiesAct = new QAction(QIcon(":/images/node_properties_24px.svg"),tr("Selected Node Properties"), this);
    editNodePropertiesAct->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Period);
    editNodePropertiesAct->setToolTip(tr("Change the properties of the selected node(s) \n\n"
                                         "There must be some nodes on the canvas!"));
    editNodePropertiesAct->setStatusTip(tr("Change the basic properties of the selected node(s). There must be some nodes on the canvas!"));
    editNodePropertiesAct->setWhatsThis(tr("Selected Node Properties\n\n"
                                           "If there are one or more nodes selected, "
                                           "it opens a properties dialog to edit "
                                           "their label, size, color, shape etc. \n"
                                           "You must have some node selected."));
    connect(editNodePropertiesAct, SIGNAL(triggered()), this, SLOT(slotEditNodePropertiesDialog()));


    editNodeSelectedToCliqueAct = new QAction(QIcon(":/images/cliquenew.png"),
                                              tr("Create a clique from selected nodes "), this);
    editNodeSelectedToCliqueAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_C));
    editNodeSelectedToCliqueAct->setStatusTip(tr("Connect all selected nodes with edges to create a clique -- "
                                                 "There must be some nodes selected!"));
    editNodeSelectedToCliqueAct->setWhatsThis(tr("Clique from Selected Nodes\n\n"
                                                 "Adds all possible edges between selected nodes, "
                                                 "so that they become a complete subgraph (clique)\n"
                                                 "You must have some nodes selected."));
    connect(editNodeSelectedToCliqueAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToClique()));


    editNodeSelectedToStarAct = new QAction(QIcon(":/images/subgraphstar_128px.svg"),
                                            tr("Create a star from selected nodes "), this);
    editNodeSelectedToStarAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_S));
    editNodeSelectedToStarAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                               "There must be some nodes selected!"));
    editNodeSelectedToStarAct->setWhatsThis(tr("Star from Selected Nodes\n\n"
                                               "Adds edges between selected nodes, "
                                               "so that they become a star subgraph.\n"
                                               "You must have some nodes selected."));
    connect(editNodeSelectedToStarAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToStar()));


    editNodeSelectedToCycleAct = new QAction(QIcon(":/images/subgraphcycle_48px.svg"),
                                             tr("Create a cycle from selected nodes "), this);
    editNodeSelectedToCycleAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_Y));
    editNodeSelectedToCycleAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                                "There must be some nodes selected!"));
    editNodeSelectedToCycleAct->setWhatsThis(tr("Cycle from Selected Nodes\n\n"
                                                "Connect selected nodes "
                                                "so that they become a cycle subgraph.\n"
                                                "A cycle graph or circular graph is a graph that consists of a single cycle, or in other words, the vertices are connected in a closed chain. The cycle graph with n vertices is called Cₙ\n"
                                                "You must have some nodes selected."));
    connect(editNodeSelectedToCycleAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToCycle()));


    editNodeSelectedToLineAct = new QAction(QIcon(":/images/subgraphline.png"),
                                            tr("Create a line from selected nodes "), this);
    editNodeSelectedToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_L));
    editNodeSelectedToLineAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a line-- "
                                               "There must be some nodes selected!"));
    editNodeSelectedToLineAct->setWhatsThis(tr("Line from Selected Nodes\n\n"
                                               "Adds edges between selected nodes, "
                                               "so that they become a line subgraph.\n"
                                               "You must have some nodes selected."));
    connect(editNodeSelectedToLineAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToLine()));


    editNodeColorAll = new QAction(QIcon(":/images/colorize_48px.svg"), tr("Change All Nodes Color (this session)"),	this);
    editNodeColorAll->setStatusTip(tr("Choose a new color for all nodes (in this session only)."));
    editNodeColorAll->setWhatsThis(tr("Nodes Color\n\n"
                                      "Changes all nodes color at once. \n"
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, go to Settings."));
    connect(editNodeColorAll, SIGNAL(triggered()), this, SLOT(slotEditNodeColorAll()) );

    editNodeSizeAllAct = new QAction(QIcon(":/images/size_select_24px.svg"), tr("Change All Nodes Size (this session)"),	this);
    editNodeSizeAllAct->setStatusTip(tr("Change the size of all nodes (in this session only)"));
    editNodeSizeAllAct->setWhatsThis(tr("Change All Nodes Size\n\n"
                                        "Click to select and apply a new size for all nodes at once. \n"
                                        "This setting will apply to this session only. \n"
                                        "To permanently change it, go to Settings."));
    connect(editNodeSizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSizeAll()) );

    editNodeShapeAll = new QAction(QIcon(":/images/format_shapes_48px.svg"), tr("Change All Nodes Shape (this session)"),	this);
    editNodeShapeAll->setStatusTip(tr("Change the shape of all nodes (this session only)"));
    editNodeShapeAll->setWhatsThis(tr("Change All Nodes Shape\n\n"
                                      "Click to select and apply a new shape for all nodes at once."
                                      "This setting will apply to this session only. \n"
                                      "To permanently change it, go to Settings."));
    connect(editNodeShapeAll, SIGNAL(triggered()), this, SLOT(slotEditNodeShape()) );


    editNodeNumbersSizeAct = new QAction(QIcon(":/images/nodenumbersize_48px.svg"),
                                         tr("Change All Node Numbers Size (this session)"),	this);
    editNodeNumbersSizeAct->setStatusTip(tr("Change the font size of the numbers of all nodes"
                                            "(in this session only)"));
    editNodeNumbersSizeAct->setWhatsThis(tr("Change Node Numbers Size\n\n"
                                            "Click to select and apply a new font size for all node numbers"
                                            "This setting will apply to this session only. \n"
                                            "To permanently change it, go to Settings."));
    connect(editNodeNumbersSizeAct, SIGNAL(triggered()),
            this, SLOT( slotEditNodeNumberSize()) );


    editNodeNumbersColorAct = new QAction(QIcon(":/images/format_color_text_48px.svg"),
                                          tr("Change All Node Numbers Color (this session)"),	this);
    editNodeNumbersColorAct->setStatusTip(tr("Change the color of the numbers of all nodes."
                                             "(in this session only)"));
    editNodeNumbersColorAct->setWhatsThis(tr("Node Numbers Color\n\n"
                                             "Click to select and apply a new color "
                                             "to all node numbers."
                                             "This setting will apply to this session only. \n"
                                             "To permanently change it, go to Settings."));
    connect(editNodeNumbersColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeNumbersColor()));

    editNodeLabelsSizeAct = new QAction(QIcon(":/images/format_textsize_48px.svg"), tr("Change All Node Labels Size (this session)"), this);
    editNodeLabelsSizeAct->setStatusTip(tr("Change the font size of the labels of all nodes"
                                           "(this session only)"));
    editNodeLabelsSizeAct->setWhatsThis(tr("Node Labels Size\n\n"
                                           "Click to select and apply a new font-size to all node labels"
                                           "This setting will apply to this session only. \n"
                                           "To permanently change it, go to Settings."));
    connect(editNodeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelSize()) );

    editNodeLabelsColorAct = new QAction(QIcon(":/images/format_color_text_48px.svg"), tr("Change All Node Labels Color (this session)"),	this);
    editNodeLabelsColorAct->setStatusTip(tr("Change the color of the labels of all nodes "
                                            "(for this session only)"));
    editNodeLabelsColorAct->setWhatsThis(tr("Labels Color\n\n"
                                            "Click to select and apply a new color to all node labels."
                                            "This setting will apply to this session only. \n"
                                            "To permanently change it, go to Settings."));
    connect(editNodeLabelsColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelsColor()));

    editEdgeAddAct = new QAction(QIcon(":/images/edge_add_48px.svg"), tr("Add Edge (arc)"),this);
    editEdgeAddAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    editEdgeAddAct->setStatusTip(tr("Add a directed edge (arc) from a node to another. "));
    editEdgeAddAct->setToolTip(
                tr("Add a new edge from a node to another.\n\n"
                   "You can also create an edge between two nodes \n"
                   "by double-clicking on them consecutively."));
    editEdgeAddAct->setWhatsThis(
                tr("Add edge\n\n"
                   "Adds a new edge from a node to another.\n\n"
                   "Alternately, you can create a new edge between two nodes "
                   "by double-clicking on them consecutively.")
                );
    connect(editEdgeAddAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeAdd()));

    editEdgeRemoveAct = new QAction(QIcon(":/images/edge_remove_48px.svg"), tr("Remove Edge"), this);
    editEdgeRemoveAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Slash));
    editEdgeRemoveAct->setToolTip(tr("Remove selected edges from the network. \n\n"
                                      "If no edge has been clicked or selected, you will be prompted \n"
                                      "to enter edge source and target nodes for the edge to remove."));
    editEdgeRemoveAct->setStatusTip(tr("Remove selected Edge(s)"));
    editEdgeRemoveAct->setWhatsThis(tr("Remove Edge\n\n"
                                       "Removes edges from the network. \n"
                                       "If one or more edges has been clicked or selected, they are removed. "
                                       "Otherwise, you will be prompted to enter edge source and target "
                                       "nodes for the edge to remove."));
    connect(editEdgeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeRemove()));

    editEdgeLabelAct = new QAction(QIcon(":/images/format_textsize_48px.svg"), tr("Change Edge Label"), this);
    editEdgeLabelAct->setStatusTip(tr("Change the Label of an Edge"));
    editEdgeLabelAct->setWhatsThis(tr("Change Edge Label\n\n"
                                      "Changes the label of an Edge"));
    connect(editEdgeLabelAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeLabel()));


    editEdgeColorAct = new QAction(QIcon(":/images/colorize_48px.svg"),tr("Change Edge Color"),	this);
    editEdgeColorAct->setStatusTip(tr("Change the Color of an Edge"));
    editEdgeColorAct->setWhatsThis(tr("Change Edge Color\n\n"
                                      "Changes the Color of an Edge"));
    connect(editEdgeColorAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColor()));

    editEdgeWeightAct = new QAction(QIcon(":/images/line_weight_48px.svg") ,tr("Change Edge Weight"), this);
    editEdgeWeightAct->setStatusTip(tr("Change the weight of an Edge"));
    editEdgeWeightAct->setWhatsThis(tr("Edge Weight\n\n"
                                       "Changes the Weight of an Edge"));
    connect(editEdgeWeightAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeWeight()));

    editEdgeColorAllAct = new QAction(QIcon(":/images/colorize_48px.svg"), tr("Change All Edges Color"), this);
    editEdgeColorAllAct->setStatusTip(tr("Change the color of all Edges."));
    editEdgeColorAllAct->setWhatsThis(tr("All Edges Color\n\n"
                                         "Changes the color of all Edges"));
    connect(editEdgeColorAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColorAll()));



    editEdgeSymmetrizeAllAct= new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize All Edges"), this);
    editEdgeSymmetrizeAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_S));
    editEdgeSymmetrizeAllAct->setStatusTip(tr("Make all directed ties to be reciprocated (thus, a symmetric graph)."));
    editEdgeSymmetrizeAllAct->setWhatsThis(
                tr("<p><b>Symmetrize All Edges</b></p>"
                   "<p>Forces all edges in this relation to be reciprocated: "
                   "<p>If there is a directed edge from node A to node B \n"
                   "then a new directed edge from node B to node A will be \n"
                   " created, with the same weight. </p>"
                   "<p>The result is a symmetric network.</p>"));
    connect(editEdgeSymmetrizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeSymmetrizeAll()));


    editEdgeSymmetrizeStrongTiesAct= new QAction(QIcon(":/images/symmetrize_48px.svg"), tr("Symmetrize by Strong Ties"), this);
    editEdgeSymmetrizeStrongTiesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_T));
    editEdgeSymmetrizeStrongTiesAct->setStatusTip(tr("Create a new symmetric relation by counting reciprocated ties only (strong ties)."));
    editEdgeSymmetrizeStrongTiesAct->setWhatsThis(
                tr("<p><b>Symmetrize Edges by Strong Ties:</b></p>"
                     "<p>Creates a new symmetric relation by keeping strong ties only. </p>"
                     "<p>A tie between actors A and B is considered strong if both A -> B and B -> A exist. "
                     "Therefore, in the new relation, a reciprocated edge will be created between actors A and B "
                     "only if both arcs A->B and B->A were present in the current or all relations. </p>"
                     "<p>If the network is multi-relational, it will ask you whether "
                      "ties in the current relation or all relations are to be considered.</p>"));
    connect(editEdgeSymmetrizeStrongTiesAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeStrongTies()));


    //TODO Separate action for Directed/Undirected graph drawing (without changing all existing edges).
    editEdgeUndirectedAllAct= new QAction( tr("Undirected Edges"), this);
    editEdgeUndirectedAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_U));
    editEdgeUndirectedAllAct->setStatusTip(tr("Enable to transform all arcs to undirected edges and hereafter work with undirected edges ."));
    editEdgeUndirectedAllAct->setWhatsThis(
                tr("Undirected Edges\n\n"
                   "Transforms all directed arcs to undirected edges. \n"
                   "The result is a undirected and symmetric network."
                   "After that, every new edge you add, will be undirected too."
                   "If you disable this, then all edges become directed again."));
    editEdgeUndirectedAllAct->setCheckable(true);
    editEdgeUndirectedAllAct->setChecked(false);
    connect(editEdgeUndirectedAllAct, SIGNAL(triggered(bool)),
            this, SLOT(slotEditEdgeUndirectedAll(bool)));



    editEdgesCocitationAct= new QAction(QIcon(":/images/cocitation_48px.svg"), tr("Cocitation Network"), this);
    editEdgesCocitationAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_C));
    editEdgesCocitationAct->setStatusTip(tr("Create a new symmetric relation by "
                                            "connecting actors that are cocitated by others."));
    editEdgesCocitationAct->setWhatsThis(
                tr("<p><b>Symmetrize Edges by examining Cocitation:</b></p>"
                     "<p>Creates a new symmetric relation by connecting actors "
                     "that are cocitated by others. "
                     "In the new relation, an edge will be created between actor i and "
                     "actor j only if C(i,j) > 0, where C the Cocitation Matrix. </p>"
                    "<p>Thus the actor pairs cited by more common neighbors will appear "
                     "with a stronger tie between them than pairs those cited by fewer "
                     "common neighbors. "
                     "The resulting relation is symmetric.</p>"));
    connect(editEdgesCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeCocitation()));


    editEdgeDichotomizeAct= new QAction(QIcon(":/images/filter_list_48px.svg"), tr("Dichotomize Valued Edges"), this);
    editEdgeDichotomizeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_D));
    editEdgeDichotomizeAct->setStatusTip(tr("Create a new binary relation/graph in a valued network "
                                            "using edge dichotomization."));
    editEdgeDichotomizeAct->setWhatsThis(
                tr("Dichotomize Edges\n\n"
                   "Creates a new binary relation in a valued network using "
                   "edge dichotomization according to a given threshold value. \n"
                   "In the new dichotomized relation, an edge will exist between actor i and "
                   "actor j only if e(i,j) > threshold, where threshold is a user-defined value."
                   "Thus the dichotomization procedure is as follows: "
                   "Choose a threshold value, set all ties with equal or higher values "
                   "to equal one, and all lower to equal zero."
                   "The result is a binary (dichotomized) graph. "
                   "The process is also known as compression and slicing"));
    connect(editEdgeDichotomizeAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeDichotomizationDialog()));




    transformNodes2EdgesAct = new QAction( tr("Transform Nodes to Edges"),this);
    transformNodes2EdgesAct->setStatusTip(tr("Transforms the network so that "
                                             "nodes become Edges and vice versa"));
    transformNodes2EdgesAct->setWhatsThis(tr("Transform Nodes EdgesAct\n\n"
                                             "Transforms network so that nodes become Edges and vice versa"));
    connect(transformNodes2EdgesAct, SIGNAL(triggered()),
            this, SLOT(slotEditTransformNodes2Edges()));



    filterNodesByCentralityAct = new QAction(tr("Filter Nodes By Centrality"), this);
    filterNodesByCentralityAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_E));
    filterNodesByCentralityAct->setStatusTip(tr("Temporarily filter out nodes according to their centrality score."));
    filterNodesByCentralityAct->setWhatsThis(tr("Filter Nodes By Centrality\n\n"
                                    "Filters out nodes according to their score in a user-selected centrality index."));
    connect(filterNodesByCentralityAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesDialogByCentrality()));

    editFilterNodesIsolatesAct = new QAction(tr("Disable Isolate Nodes"), this);
    editFilterNodesIsolatesAct->setEnabled(true);
    editFilterNodesIsolatesAct->setCheckable(true);
    editFilterNodesIsolatesAct->setChecked(false);
    editFilterNodesIsolatesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_I));
    editFilterNodesIsolatesAct->setStatusTip(tr("Temporarily filter out nodes with no edges"));
    editFilterNodesIsolatesAct->setWhatsThis(tr("Filter Isolate Nodes\n\n"
                                                  "Enables or disables displaying of isolate nodes. "
                                                  "Isolate nodes are those with no edges..."));
    connect(editFilterNodesIsolatesAct, SIGNAL(toggled(bool)),
            this, SLOT(slotEditFilterNodesIsolates(bool)));

    editFilterEdgesByWeightAct = new QAction(QIcon(":/images/filter_list_48px.svg"), tr("Filter Edges by Weight"), this);
    editFilterEdgesByWeightAct->setEnabled(true);
    editFilterEdgesByWeightAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_F));
    editFilterEdgesByWeightAct->setStatusTip(tr("Temporarily filter edges of some weight out of the network"));
    editFilterEdgesByWeightAct->setWhatsThis(tr("Filter Edges\n\n"
                                                  "Filters edges according to their weight."));
    connect(editFilterEdgesByWeightAct , SIGNAL(triggered()),
            this, SLOT(slotEditFilterEdgesByWeightDialog()));

    editFilterEdgesUnilateralAct = new QAction(tr("Disable unilateral edges"), this);
    editFilterEdgesUnilateralAct->setEnabled(true);
    editFilterEdgesUnilateralAct->setCheckable(true);
    editFilterEdgesUnilateralAct->setChecked(false);
    editFilterEdgesUnilateralAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_R));
    editFilterEdgesUnilateralAct->setStatusTip(tr("Temporarily disable all unilateral (non-reciprocal) edges in this relation. Keeps only \"strong\" ties."));
    editFilterEdgesUnilateralAct->setWhatsThis(tr("Unilateral edges\n\n"
                                                    "In directed networks, a tie between two actors "
                                                    "is unilateral when only one actor identifies the other "
                                                    "as connected (i.e. friend, vote, etc). "
                                                    "A unilateral tie is depicted as a single arc. "
                                                    "These ties are considered weak, as opposed to "
                                                    "reciprocal ties where both actors identify each other as connected. "
                                                    "Strong ties are depicted as either a single undirected edge "
                                                    "or as two reciprocated arcs between two nodes. "
                                                    "By selecting this option, all unilateral edges in this relation will be disabled."));
    connect(editFilterEdgesUnilateralAct , SIGNAL(triggered(bool)),
            this, SLOT(slotEditFilterEdgesUnilateral(bool)));




    /**
    Layout menu actions
    */
    strongColorationAct = new QAction ( tr("Strong Structural"), this);
    strongColorationAct->setStatusTip( tr("Nodes are assigned the same color if they have identical in and out neighborhoods") );
    strongColorationAct->setWhatsThis( tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
    connect(strongColorationAct, SIGNAL(triggered() ), this, SLOT(slotLayoutColorationStrongStructural()) );

    regularColorationAct = new QAction ( tr("Regular"), this);
    regularColorationAct->
            setStatusTip(
                tr("Nodes are assigned the same color if they have "
                   "neighborhoods of the same set of colors") );
    regularColorationAct
          ->setWhatsThis(
                tr("Click this to colorize nodes; "
                   "Nodes are assigned the same color if they have neighborhoods "
                   "of the same set of colors"));
    connect(regularColorationAct, SIGNAL(triggered() ), this, SLOT(slotLayoutColorationRegular()) );//TODO

    layoutRandomAct = new QAction( tr("Random"),this);
    layoutRandomAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_0));
    layoutRandomAct->setStatusTip(tr("Layout the network actors in random positions."));
    layoutRandomAct->setWhatsThis(tr("Random Layout\n\n "
                                       "This layout algorithm repositions all "
                                       "network actors in random positions."));
    connect(layoutRandomAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandom()));


    layoutRandomRadialAct = new QAction(tr("Random Circles"),	this);
    layoutRandomRadialAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_0));
    layoutRandomRadialAct->setStatusTip(tr("Layout the network in random concentric circles"));
    layoutRandomRadialAct->
            setWhatsThis(
                tr("Random Circles Layout\n\n Repositions the nodes randomly on circles"));
    connect(layoutRandomRadialAct, SIGNAL(triggered()), this, SLOT(slotLayoutRadialRandom()));




    layoutRadialProminence_DC_Act = new QAction( tr("Degree Centrality"),	this);
    layoutRadialProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_1));
    layoutRadialProminence_DC_Act
          ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Degree Centrality."));
    layoutRadialProminence_DC_Act->
            setWhatsThis(
                tr( "Degree Centrality (DC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Degree Centrality score. "
                    "Nodes with higher DC are closer to the centre."
                    ));
    connect(layoutRadialProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()) );

    layoutRadialProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutRadialProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_2));
    layoutRadialProminence_CC_Act
         ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Closeness Centrality."));
    layoutRadialProminence_CC_Act->
            setWhatsThis(
                tr( "Closeness Centrality (CC) Radial Layout\n\n"
                    "Repositions all nodes on concentric circles of radius "
                    "inversely proportional to their Closeness Centrality. "
                    "Nodes having higher CC are closer to the centre."
                    ));
    connect(layoutRadialProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutRadialProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_3));
    layoutRadialProminence_IRCC_Act
          ->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Influence Range Closeness Centrality."));
    layoutRadialProminence_IRCC_Act->
            setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their IRCC score. "
                   "Nodes having higher IRCC are closer to the centre."
                   ));
    connect(layoutRadialProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutRadialProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_4));
    layoutRadialProminence_BC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Betweenness Centrality."));
    layoutRadialProminence_BC_Act->
            setWhatsThis(
                tr("Betweenness Centrality (BC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Betweenness Centrality. "
                   "Nodes having higher BC are closer to the centre."
                   ));
    connect(layoutRadialProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutRadialProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_5));
    layoutRadialProminence_SC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Stress Centrality."));
    layoutRadialProminence_SC_Act->
            setWhatsThis(
                tr("Stress Centrality (SC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Stress Centrality score. "
                   "Nodes having higher SC are closer to the centre."
                   ));
    connect(layoutRadialProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutRadialProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_6));
    layoutRadialProminence_EC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutRadialProminence_EC_Act->
            setWhatsThis(
                tr("Eccentricity Centrality (EC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Eccentricity Centrality "
                   "(aka Harary Graph Centrality) score. "
                   "Nodes having higher EC are closer to the centre."
                   ));
    connect(layoutRadialProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutRadialProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_7));
    layoutRadialProminence_PC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Power Centrality."));
    layoutRadialProminence_PC_Act->
            setWhatsThis(
                tr("Power Centrality (PC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Power Centrality score. "
                   "Nodes having higher PC are closer to the centre."
                   ));
    connect(layoutRadialProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutRadialProminence_IC_Act->setEnabled(true);
    layoutRadialProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_8));
    layoutRadialProminence_IC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Information Centrality."));
    layoutRadialProminence_IC_Act->
            setWhatsThis(
                tr("Information Centrality (IC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Information Centrality score. "
                   "Nodes of higher IC are closer to the centre."
                   ));
    connect(layoutRadialProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutRadialProminence_EVC_Act->setEnabled(true);
    layoutRadialProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_9));
    layoutRadialProminence_EVC_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Eigenvector Centrality."));
    layoutRadialProminence_EVC_Act->
            setWhatsThis(
                tr("Eigenvector Centrality (EVC) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their Eigenvector Centrality score. "
                   "Nodes of higher EVC are closer to the centre."
                   ));
    connect(layoutRadialProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutRadialProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_I));
    layoutRadialProminence_DP_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Degree Prestige (inDegree)."));
    layoutRadialProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their inDegree score. "
                   "Nodes having higher DP are closer to the centre."
                   ));
    connect(layoutRadialProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutRadialProminence_PRP_Act->setEnabled(true);
    layoutRadialProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_K));
    layoutRadialProminence_PRP_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their PRP index."));
    layoutRadialProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their PageRank score. "
                   "Nodes having higher PRP are closer to the centre."
                   ));
    connect(layoutRadialProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));


    layoutRadialProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutRadialProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_Y));
    layoutRadialProminence_PP_Act->setStatusTip(
                tr("Place all nodes on concentric circles of radius inversely "
                   "proportional to their Proximity Prestige."));
    layoutRadialProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Radial Layout\n\n"
                   "Repositions all nodes on concentric circles of radius "
                   "inversely proportional to their PP index. "
                   "Nodes having higher PP score are closer to the centre."
                   ));
    connect(layoutRadialProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));




    layoutLevelProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutLevelProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_1));
    layoutLevelProminence_DC_Act
          ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                   "proportional to their Degree Centrality."));
    layoutLevelProminence_DC_Act->
            setWhatsThis(
                tr("Degree Centrality (DC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their DC score. "
                   "Nodes having higher DC are closer to the top.\n\n"
                   )
                );
    connect(layoutLevelProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()) );

    layoutLevelProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutLevelProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_2));
    layoutLevelProminence_CC_Act
         ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                   "proportional to their Closeness Centrality."));
    layoutLevelProminence_CC_Act->
            setWhatsThis(
                tr("Closeness Centrality (CC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Closeness Centrality score. "
                   "Nodes of higher CC are closer to the top.\n\n"
                   "This layout can be computed only for connected graphs. "
                   ));
    connect(layoutLevelProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutLevelProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_3));
    layoutLevelProminence_IRCC_Act
          ->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                   "proportional to their Influence Range Closeness Centrality."));
    layoutLevelProminence_IRCC_Act->
            setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their IRCC score. "
                   "Nodes having higher IRCC are closer to the top.\n\n"
                   "This layout can be computed for not connected graphs. "
                   ));
    connect(layoutLevelProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutLevelProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_4));
    layoutLevelProminence_BC_Act->setStatusTip(
                tr("Place all nodes on horizontal levels of height "
                   "proportional to their Betweenness Centrality."));
    layoutLevelProminence_BC_Act->
            setWhatsThis(
                tr("Betweenness Centrality (BC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Betweenness Centrality score. "
                   "Nodes having higher BC are closer to the top."
                   ));
    connect(layoutLevelProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutLevelProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_5));
    layoutLevelProminence_SC_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Stress Centrality."));
    layoutLevelProminence_SC_Act->
            setWhatsThis(
                tr("Stress Centrality (SC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Stress Centrality score. "
                   "Nodes having higher SC are closer to the top."
                   ));
    connect(layoutLevelProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutLevelProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_6));
    layoutLevelProminence_EC_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutLevelProminence_EC_Act->
            setWhatsThis(
                tr("Eccentricity Centrality (EC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Eccentricity Centrality "
                   "(aka Harary Graph Centrality) score. "
                   "Nodes having higher EC are closer to the top."
                   ));
    connect(layoutLevelProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutLevelProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_7));
    layoutLevelProminence_PC_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Power Centrality."));
    layoutLevelProminence_PC_Act->
            setWhatsThis(
                tr("Power Centrality (PC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Power Centrality score. "
                   "Nodes having higher PC are closer to the top."
                   ));
    connect(layoutLevelProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutLevelProminence_IC_Act->setEnabled(true);
    layoutLevelProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_8));
    layoutLevelProminence_IC_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Information Centrality."));
    layoutLevelProminence_IC_Act->
            setWhatsThis(
                tr("Information Centrality (IC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Information Centrality score. "
                   "Nodes having higher IC are closer to the top."
                   ));
    connect(layoutLevelProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutLevelProminence_EVC_Act->setEnabled(true);
    layoutLevelProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_9));
    layoutLevelProminence_EVC_Act->setStatusTip(
                tr(
                    "Place nodes on horizontal levels of height "
                    "proportional to their Eigenvector Centrality."));
    layoutLevelProminence_EVC_Act->
            setWhatsThis(
                tr("Eigenvector Centrality (EVC) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Eigenvector Centrality score. "
                   "Nodes having higher EVC are closer to the top."
                   ));
    connect(layoutLevelProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));



    layoutLevelProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutLevelProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_I));
    layoutLevelProminence_DP_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Degree Prestige."));
    layoutLevelProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Degree Prestige score. "
                   "Nodes having higher DP are closer to the top."
                   ));
    connect(layoutLevelProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutLevelProminence_PRP_Act->setEnabled(true);
    layoutLevelProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_K));
    layoutLevelProminence_PRP_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their PageRank Prestige."));
    layoutLevelProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their PageRank Prestige score. "
                   "Nodes having higher PRP are closer to the top."
                   ));
    connect(layoutLevelProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));


    layoutLevelProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutLevelProminence_PP_Act->setEnabled(true);
    layoutLevelProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_Y));
    layoutLevelProminence_PP_Act->setStatusTip(
                tr("Place nodes on horizontal levels of height "
                   "proportional to their Proximity Prestige."));
    layoutLevelProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Levels Layout\n\n"
                   "Repositions all nodes on horizontal levels of height"
                   "proportional to their Proximity Prestige score. "
                   "Nodes having higher PP are closer to the top."
                   ));
    connect(layoutLevelProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));




    layoutNodeSizeProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutNodeSizeProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_1));
    layoutNodeSizeProminence_DC_Act
          ->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Degree Centrality."));
    layoutNodeSizeProminence_DC_Act->
            setWhatsThis(
                tr(
                    "Degree Centrality (DC) Node Size Layout\n\n"
                    "Changes the size of all nodes to be "
                    "proportional to their DC (inDegree) score. \n\n"
                    "Nodes having higher DC will appear bigger."
                    )
                );
    connect(layoutNodeSizeProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()) );

    layoutNodeSizeProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutNodeSizeProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_2));
    layoutNodeSizeProminence_CC_Act
         ->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Closeness Centrality."));
    layoutNodeSizeProminence_CC_Act->
            setWhatsThis(
                tr("Closeness Centrality (CC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their CC score. "
                   "Nodes of higher CC will appear bigger.\n\n"
                   "This layout can be computed only for connected graphs. "
                   ));
    connect(layoutNodeSizeProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutNodeSizeProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_3));
    layoutNodeSizeProminence_IRCC_Act
          ->setStatusTip(
                tr("Resize all nodes to be proportional "
                   "to their Influence Range Closeness Centrality."));
    layoutNodeSizeProminence_IRCC_Act->
            setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their IRCC score. "
                   "Nodes having higher IRCC will appear bigger.\n\n"
                   "This layout can be computed for not connected graphs. "
                   ));
    connect(layoutNodeSizeProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutNodeSizeProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_4));
    layoutNodeSizeProminence_BC_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Betweenness Centrality."));
    layoutNodeSizeProminence_BC_Act->
            setWhatsThis(
                tr("Betweenness Centrality (BC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Betweenness Centrality score. "
                   "Nodes having higher BC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutNodeSizeProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_5));
    layoutNodeSizeProminence_SC_Act->setStatusTip(
                tr( "Resize all nodes to be  "
                    "proportional to their Stress Centrality."));
    layoutNodeSizeProminence_SC_Act->
            setWhatsThis(
                tr("Stress Centrality (SC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Stress Centrality score. "
                   "Nodes having higher SC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutNodeSizeProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_6));
    layoutNodeSizeProminence_EC_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutNodeSizeProminence_EC_Act->
            setWhatsThis(
                tr("Eccentricity Centrality (EC) NodeSizes Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Eccentricity Centrality (aka Harary Graph Centrality) score. "
                   "Nodes having higher EC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutNodeSizeProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_7));
    layoutNodeSizeProminence_PC_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Power Centrality."));
    layoutNodeSizeProminence_PC_Act->
            setWhatsThis(
                tr("Power Centrality (PC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Power Centrality score. "
                   "Nodes having higher PC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutNodeSizeProminence_IC_Act->setEnabled(true);
    layoutNodeSizeProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_8));
    layoutNodeSizeProminence_IC_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Information Centrality."));
    layoutNodeSizeProminence_IC_Act->
            setWhatsThis(
                tr("Information Centrality (IC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Information Centrality score. "
                   "Nodes having higher IC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutNodeSizeProminence_EVC_Act->setEnabled(true);
    layoutNodeSizeProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_9));
    layoutNodeSizeProminence_EVC_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Eigenvector Centrality."));
    layoutNodeSizeProminence_EVC_Act->
            setWhatsThis(
                tr("Eigenvector Centrality (EVC) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Eigenvector Centrality score. "
                   "Nodes having higher EVC will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));



    layoutNodeSizeProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutNodeSizeProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_I));
    layoutNodeSizeProminence_DP_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Degree Prestige."));
    layoutNodeSizeProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Degree Prestige score. "
                   "Nodes having higher DP will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutNodeSizeProminence_PRP_Act->setEnabled(true);
    layoutNodeSizeProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_K));
    layoutNodeSizeProminence_PRP_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their PageRank Prestige."));
    layoutNodeSizeProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their PageRank Prestige score. "
                   "Nodes having higher PRP will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));


    layoutNodeSizeProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutNodeSizeProminence_PP_Act->setEnabled(true);
    layoutNodeSizeProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_Y));
    layoutNodeSizeProminence_PP_Act->setStatusTip(
                tr("Resize all nodes to be "
                   "proportional to their Proximity Prestige."));
    layoutNodeSizeProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Node Size Layout\n\n"
                   "Changes the size of all nodes to be "
                   "proportional to their Proximity Prestige score. "
                   "Nodes having higher PP will appear bigger."
                   ));
    connect(layoutNodeSizeProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));





    layoutNodeColorProminence_DC_Act = new QAction( tr("Degree Centrality"), this);
    layoutNodeColorProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_1));
    layoutNodeColorProminence_DC_Act
          ->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Degree Centrality."));
    layoutNodeColorProminence_DC_Act->
            setWhatsThis(
                tr("Degree Centrality (DC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their DC (inDegree) score. \n\n"
                   "Nodes having higher DC will have warmer color (i.e. red)."
                   )
                );
    connect(layoutNodeColorProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()) );

    layoutNodeColorProminence_CC_Act = new QAction( tr("Closeness Centrality"), this);
    layoutNodeColorProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_2));
    layoutNodeColorProminence_CC_Act
         ->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Closeness Centrality."));
    layoutNodeColorProminence_CC_Act->
            setWhatsThis(
                tr("Closeness Centrality (CC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their CC score. "
                   "Nodes of higher CC will have warmer color (i.e. red).\n\n"
                   "This layout can be computed only for connected graphs. "
                   ));
    connect(layoutNodeColorProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_IRCC_Act = new QAction(
                tr("Influence Range Closeness Centrality"),	this);
    layoutNodeColorProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_3));
    layoutNodeColorProminence_IRCC_Act
          ->setStatusTip(
                tr("Change the color of all nodes to proportional "
                   "to their Influence Range Closeness Centrality."));
    layoutNodeColorProminence_IRCC_Act->
            setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their IRCC score. "
                   "Nodes having higher IRCC will have warmer color (i.e. red).\n\n"
                   "This layout can be computed for not connected graphs. "
                   ));
    connect(layoutNodeColorProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_BC_Act = new QAction( tr("Betweenness Centrality"), this);
    layoutNodeColorProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_4));
    layoutNodeColorProminence_BC_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Betweenness Centrality."));
    layoutNodeColorProminence_BC_Act->
            setWhatsThis(
                tr("Betweenness Centrality (BC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Betweenness Centrality score. "
                   "Nodes having higher BC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_SC_Act = new QAction( tr("Stress Centrality"),	this);
    layoutNodeColorProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_5));
    layoutNodeColorProminence_SC_Act->setStatusTip(
                tr( "Change the color of all nodes to  "
                    "reflect their Stress Centrality."));
    layoutNodeColorProminence_SC_Act->
            setWhatsThis(
                tr("Stress Centrality (SC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Stress Centrality score. "
                   "Nodes having higher SC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EC_Act = new QAction( tr("Eccentricity Centrality"),	this);
    layoutNodeColorProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_6));
    layoutNodeColorProminence_EC_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutNodeColorProminence_EC_Act->
            setWhatsThis(
                tr("Eccentricity Centrality (EC) NodeColors Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Eccentricity Centrality (aka Harary Graph Centrality) score. "
                   "Nodes having higher EC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_PC_Act = new QAction( tr("Power Centrality"),	this);
    layoutNodeColorProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_7));
    layoutNodeColorProminence_PC_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Power Centrality."));
    layoutNodeColorProminence_PC_Act->
            setWhatsThis(
                tr("Power Centrality (PC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Power Centrality score. "
                   "Nodes having higher PC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_IC_Act = new QAction( tr("Information Centrality"),	this);
    layoutNodeColorProminence_IC_Act->setEnabled(true);
    layoutNodeColorProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_8));
    layoutNodeColorProminence_IC_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Information Centrality."));
    layoutNodeColorProminence_IC_Act->
            setWhatsThis(
                tr("Information Centrality (IC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Information Centrality score. "
                   "Nodes having higher IC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EVC_Act = new QAction( tr("Eigenvector Centrality"),	this);
    layoutNodeColorProminence_EVC_Act->setEnabled(true);
    layoutNodeColorProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_9));
    layoutNodeColorProminence_EVC_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Eigenvector Centrality."));
    layoutNodeColorProminence_EVC_Act->
            setWhatsThis(
                tr("Eigenvector Centrality (EVC) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Eigenvector Centrality score. "
                   "Nodes having higher EVC will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));



    layoutNodeColorProminence_DP_Act = new QAction( tr("Degree Prestige"),	this);
    layoutNodeColorProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_I));
    layoutNodeColorProminence_DP_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Degree Prestige."));
    layoutNodeColorProminence_DP_Act->
            setWhatsThis(
                tr("Degree Prestige (DP) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their Degree Prestige score. "
                   "Nodes having higher DP will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_PRP_Act = new QAction( tr("PageRank Prestige"),	this);
    layoutNodeColorProminence_PRP_Act->setEnabled(true);
    layoutNodeColorProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_K));
    layoutNodeColorProminence_PRP_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their PageRank Prestige."));
    layoutNodeColorProminence_PRP_Act->
            setWhatsThis(
                tr("PageRank Prestige (PRP) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their PageRank Prestige score. "
                   "Nodes having higher PRP will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));


    layoutNodeColorProminence_PP_Act = new QAction( tr("Proximity Prestige"),	this);
    layoutNodeColorProminence_PP_Act->setEnabled(true);
    layoutNodeColorProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_Y));
    layoutNodeColorProminence_PP_Act->setStatusTip(
                tr("Change the color of all nodes to "
                   "reflect their Proximity Prestige."));
    layoutNodeColorProminence_PP_Act->
            setWhatsThis(
                tr("Proximity Prestige (PP) Node Color Layout\n\n"
                   "Changes the color of all nodes to "
                   "reflect their PageRank Prestige score. "
                   "Nodes of higher PP will have warmer color (i.e. red)."
                   ));
    connect(layoutNodeColorProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));





    layoutFDP_Eades_Act= new QAction(tr("Spring Embedder (Eades)"), this);
    layoutFDP_Eades_Act->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_E));
    layoutFDP_Eades_Act->setStatusTip(
                tr("Layout Eades Spring-Gravitational model."));
    layoutFDP_Eades_Act->setWhatsThis(
                tr("Spring Embedder Layout\n\n "
                   "The Spring Embedder model (Eades, 1984), part of the "
                   "Force Directed Placement (FDP) family, embeds a mechanical "
                   "system in the graph by replacing nodes with rings and edges "
                   "with springs. \n"
                   "In our implementation, nodes are replaced by physical bodies "
                   "(i.e. electrons) which exert repelling forces to each other, "
                   "while edges are replaced by springs which exert attractive "
                   "forces to the adjacent nodes. "
                   "The nodes are placed in some initial layout and let go "
                   "so that the spring forces move the system to a minimal energy state. "
                   "The algorithm continues until the system retains an equilibrium state "
                   "in which all forces cancel each other. "));
    connect(layoutFDP_Eades_Act, SIGNAL(triggered(bool)), this, SLOT(slotLayoutSpringEmbedder()));

    layoutFDP_FR_Act= new QAction( tr("Fruchterman-Reingold"),	this);
    layoutFDP_FR_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_F));
    layoutFDP_FR_Act->setStatusTip(
                tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    layoutFDP_FR_Act->setWhatsThis(
                tr("Fruchterman-Reingold Layout\n\n "
                   "Embeds a layout all nodes according to a model in which	repelling "
                   "forces are used between every pair of nodes, while attracting "
                   "forces are used only between adjacent nodes. "
                   "The algorithm continues until the system retains its equilibrium "
                   "state where all forces cancel each other."));
    connect(layoutFDP_FR_Act, SIGNAL(triggered()), this, SLOT(slotLayoutFruchterman()));

    layoutFDP_KamadaKawai_Act= new QAction( tr("Kamada-Kawai"),	this);
    layoutFDP_KamadaKawai_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_K));
    layoutFDP_KamadaKawai_Act->setStatusTip(
                tr("Embeds the Kamada-Kawai FDP layout model, the best variant of the Spring Embedder family of models."));
    layoutFDP_KamadaKawai_Act->setWhatsThis(
                tr(
                    "<p><em>Kamada-Kawai</em></p>"
                    "<p>The best variant of the Spring Embedder family of models. "
                    "<p>In this the graph is considered to be a dynamic system where "
                    "every edge is between two actors is a 'spring' of a desirable "
                    "length, which corresponds to their graph theoretic distance. </p>"
                    "<p>In this way, the optimal layout of the graph \n"
                    "is the state with the minimum imbalance. The degree of "
                    "imbalance is formulated as the total spring energy: "
                    "the square summation of the differences between desirable "
                    "distances and real ones for all pairs of vertices.</p>"

                    ));
    connect(layoutFDP_KamadaKawai_Act, SIGNAL(triggered()), this, SLOT(slotLayoutKamadaKawai()));




    layoutGuidesAct = new QAction(QIcon(":/images/gridlines.png"), tr("Layout GuideLines"), this);
    layoutGuidesAct->setStatusTip(tr("Toggles layout guidelines on or off."));
    layoutGuidesAct->setWhatsThis(tr("Layout Guidelines\n\n"
                                     "Layout Guidelines are circular or horizontal lines \n"
                                     "usually created when embedding prominence-based \n"
                                     "visualization models on the network.\n"
                                     "Disable this checkbox to hide guidelines"));
    layoutGuidesAct->setCheckable(true);
    layoutGuidesAct->setChecked(true);


    /**
    Analysis menu actions
    */


    analyzeMatrixAdjInvertAct = new QAction(
                QIcon(":/images/invertmatrix.png"), tr("Invert Adjacency Matrix"), this);
    analyzeMatrixAdjInvertAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_I)
                );
    analyzeMatrixAdjInvertAct->setStatusTip(tr("Invert the adjacency matrix, if possible"));
    analyzeMatrixAdjInvertAct->setWhatsThis(tr("Invert  Adjacency Matrix \n\n"
                                               "Inverts the adjacency matrix using linear algebra methods."));
    connect(analyzeMatrixAdjInvertAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyInverse()));


    analyzeMatrixAdjTransposeAct = new QAction(
                QIcon(":/images/transposematrix.png"), tr("Transpose Adjacency Matrix"), this);
    analyzeMatrixAdjTransposeAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_T)
                );
    analyzeMatrixAdjTransposeAct->setStatusTip(tr("View the transpose of adjacency matrix"));
    analyzeMatrixAdjTransposeAct->setWhatsThis(tr("Transpose Adjacency Matrix \n\n"
                                                  "Computes and displays the adjacency matrix tranpose."));
    connect(analyzeMatrixAdjTransposeAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyTranspose()));


    analyzeMatrixAdjCocitationAct = new QAction(
                QIcon(":/images/cocitation.png"), tr("Cocitation Matrix"), this);
    analyzeMatrixAdjCocitationAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_C)
                );
    analyzeMatrixAdjCocitationAct->setStatusTip(tr("Compute the Cocitation matrix of this network."));
    analyzeMatrixAdjCocitationAct->setWhatsThis(tr("Cocitation Matrix \n\n "
                                                   "Computes and displays the cocitation matrix of the network. "
                                                   "The Cocitation matrix, C=A*A^T, is a NxN matrix where "
                                                   "each element (i,j) is the number of actors that have "
                                                   "outbound ties/links to both actors i and j. "));
    connect(analyzeMatrixAdjCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyCocitation()));


    analyzeMatrixDegreeAct = new QAction(
                QIcon(":/images/degreematrix.png"), tr("Degree Matrix"), this);
    analyzeMatrixDegreeAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_D)
                );
    analyzeMatrixDegreeAct->setStatusTip(tr("Compute the Degree matrix of the network"));
    analyzeMatrixDegreeAct->setWhatsThis(tr("Degree Matrix "
                                            "\n\n Compute the Degree matrix of the network."));
    connect(analyzeMatrixDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixDegree()));


    analyzeMatrixLaplacianAct = new QAction(
                QIcon(":/images/laplacian.png"), tr("Laplacian Matrix"), this);
    analyzeMatrixLaplacianAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_L)
                );
    analyzeMatrixLaplacianAct->setStatusTip(tr("Compute the Laplacian matrix of the network"));
    analyzeMatrixLaplacianAct->setWhatsThis(tr("Laplacian Matrix \n\n"
                                               "Compute the Laplacian matrix of the network."));
    connect(analyzeMatrixLaplacianAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixLaplacian()));



    analyzeGraphReciprocityAct = new QAction(
                QIcon(":/images/symmetry-edge.png"), tr("Reciprocity"), this);
    analyzeGraphReciprocityAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_R)
                );
    analyzeGraphReciprocityAct->setStatusTip(tr("Compute the arc and dyad reciprocity of the network."));
    analyzeGraphReciprocityAct->setWhatsThis(
                tr("Arc and Dyad Reciprocity\n\n"
                   "The arc reciprocity of a network/graph is the fraction of "
                   "reciprocated ties over all present ties of the graph. \n"
                   "The dyad reciprocity of a network/graph is the fraction of "
                   "actor pairs that have reciprocated ties over all connected "
                   "pairs of actors. \n"
                   "In a directed network, the arc reciprocity measures the proportion "
                   "of directed edges that are bidirectional. If the reciprocity is 1, \n"
                   "then the adjacency matrix is structurally symmetric. \n"
                   "Likewise, in a directed network, the dyad reciprocity measures "
                   "the proportion of connected actor dyads that have bidirectional ties "
                   "between them. \n"
                   "In an undirected graph, all edges are reciprocal. Thus the "
                   "reciprocity of the graph is always 1. \n"
                   "Reciprocity can be computed on undirected, directed, and weighted graphs."
                   )
                );
    connect(analyzeGraphReciprocityAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeReciprocity()));

    analyzeGraphSymmetryAct = new QAction(
                QIcon(":/images/symmetry_48px.svg"), tr("Symmetry Test"), this);
    analyzeGraphSymmetryAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_S)
                );
    analyzeGraphSymmetryAct->setStatusTip(tr("Check whether the network is symmetric or not"));
    analyzeGraphSymmetryAct->setWhatsThis(
                tr("Symmetry\n\n"
                   "Checks whether the network is symmetric or not. \n"
                   "A network is symmetric when all edges are reciprocal, or, "
                   "in mathematical language, when the adjacency matrix is "
                   "symmetric.")
                );
    connect(analyzeGraphSymmetryAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeSymmetryCheck()));

    analyzeGraphDistanceAct = new QAction(
                QIcon(":/images/distance.png"), tr("Geodesic Distance between 2 nodes"), this
                );
    analyzeGraphDistanceAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_G) );
    analyzeGraphDistanceAct->setStatusTip(
                tr("Compute the length of the shortest path (geodesic distance) between 2 nodes."));
    analyzeGraphDistanceAct->setWhatsThis(
                tr("Distance\n\n"
                   "Computes the geodesic distance between two nodes."
                   "In graph theory, the geodesic distance of two "
                   "nodes is the length (number of edges) of the shortest path "
                   "between them."));
    connect(analyzeGraphDistanceAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDistance()));


    analyzeMatrixDistancesGeodesicAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesic Distances Matrix"),this);
    analyzeMatrixDistancesGeodesicAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_M) );
    analyzeMatrixDistancesGeodesicAct->
            setStatusTip(
                tr("Compute the matrix of geodesic distances between all pair of nodes.")
                );
    analyzeMatrixDistancesGeodesicAct->
            setWhatsThis(
                tr("Distances Matrix\n\n"
                   "Computes the matrix of distances between all "
                   "pairs of actors/nodes in the social network."
                   "A distances matrix is a n x n matrix, in which the "
                   "(i,j) element is the distance from node i to node j"
                   "The distance of two nodes is the length of the shortest path between them.")
                );
    connect(analyzeMatrixDistancesGeodesicAct, SIGNAL(triggered()), this, SLOT( slotAnalyzeMatrixDistances() ) );

    analyzeMatrixGeodesicsAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesics Matrix"),this);
    analyzeMatrixGeodesicsAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_P));
    analyzeMatrixGeodesicsAct->setStatusTip(tr("Compute the number of shortest paths (geodesics) between each pair of nodes "));
    analyzeMatrixGeodesicsAct->setWhatsThis(
                tr(
                    "Geodesics Matrix\n\n"
                    "Displays a n x n matrix, where the (i,j) element "
                    "is the number of shortest paths (geodesics) between "
                    "node i and node j. ")
                );
    connect(analyzeMatrixGeodesicsAct, SIGNAL(triggered()),
            this, SLOT( slotAnalyzeMatrixGeodesics()) );

    analyzeGraphDiameterAct = new QAction(QIcon(":/images/diameter_48px.svg"), tr("Graph Diameter"),this);
    analyzeGraphDiameterAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_D));
    analyzeGraphDiameterAct->setStatusTip(tr("Compute the diameter of the network, "
                                             "the maximum geodesic distance between any actors."));
    analyzeGraphDiameterAct->setWhatsThis(tr("Diameter\n\n "
                                             "The diameter of a network is the maximum geodesic distance "
                                             "(maximum shortest path length) between any two nodes of the network."));
    connect(analyzeGraphDiameterAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDiameter()));

    averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Distance"),this);
    averGraphDistanceAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_A));
    averGraphDistanceAct->setStatusTip(tr("Compute the average graph distance for all possible pairs of nodes."));
    averGraphDistanceAct->setWhatsThis(
                tr("Average Graph Distance\n\n "
                   "This is the average length of shortest paths (geodesics) "
                   "for all possible pairs of nodes. "
                   "It is a measure of the efficiency or compactness of the network."));
    connect(averGraphDistanceAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeDistanceAverage()));

    analyzeGraphEccentricityAct = new QAction(QIcon(":/images/eccentricity.png"), tr("Eccentricity"),this);
    analyzeGraphEccentricityAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_E ) );
    analyzeGraphEccentricityAct->setStatusTip(tr("Compute the Eccentricity of each actor and group Eccentricity"));
    analyzeGraphEccentricityAct->setWhatsThis(tr("Eccentricity\n\n"
                                                 "The eccentricity of each node i in a network "
                                                 "or graph is the largest geodesic distance "
                                                 "between node i and any other node j. "
                                                 "Therefore, it reflects how far, at most, "
                                                 "is each node from every other node. \n"
                                                 "The maximum eccentricity is the graph diameter "
                                                 "while the minimum is the graph radius.\n"
                                                 "This index can be calculated in both graphs "
                                                 "and digraphs but is usually best suited "
                                                 "for undirected graphs. \n"
                                                 "It can also be calculated in weighted graphs "
                                                 "although the weight of each edge (v,u) in E is "
                                                 "always considered to be 1."));
    connect(analyzeGraphEccentricityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeEccentricity()));



    analyzeGraphConnectednessAct = new QAction(QIcon(":/images/distance.png"),  tr("Connectedness"), this);
    analyzeGraphConnectednessAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_C) );
    analyzeGraphConnectednessAct->setStatusTip(tr("Check whether the network is a connected "
                                                  "graph, a connected digraph or "
                                                  "a disconnected graph/digraph..."));
    analyzeGraphConnectednessAct->setWhatsThis(tr("Connectedness\n\n In graph theory, a "
                                                  "graph is <b>connected</b> if there is a "
                                                  "path between every pair of nodes. \n"
                                                  "A digraph is <b>strongly connected</b> "
                                                  "if there the a path from i to j and "
                                                  "from j to i for all pairs (i,j).\n"
                                                  "A digraph is weakly connected if at least "
                                                  "a pair of nodes are joined by a semipath.\n"
                                                  "A digraph or a graph is disconnected if "
                                                  "at least one node is isolate."
                                                  ));
    connect(analyzeGraphConnectednessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeConnectedness()));


    analyzeGraphWalksAct = new QAction(QIcon(":/images/walk.png"), tr("Walks of a given length"),this);
    analyzeGraphWalksAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_W) );
    analyzeGraphWalksAct->setStatusTip(tr("Compute the number of walks of a given length between any nodes."));
    analyzeGraphWalksAct->setWhatsThis(tr("Walks of a given length\n\n"
                                          "A walk is a sequence of alternating vertices and edges "
                                          "such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, "
                                          "v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, "
                                          "where each edge, e<sub>i</sub> is defined as "
                                          "e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                                          "This function counts the number of walks of a given "
                                          "length between each pair of nodes, by studying the powers of the sociomatrix.\n"));
    connect(analyzeGraphWalksAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksLength() )  );

    analyzeGraphWalksTotalAct = new QAction(QIcon(":/images/walk.png"), tr("Total Walks"),this);
    analyzeGraphWalksTotalAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_T) );
    analyzeGraphWalksTotalAct->setStatusTip(tr("Calculate the total number of walks of every possible length between all nodes"));
    analyzeGraphWalksTotalAct->setWhatsThis(tr("Total Walks\n\n"
                                               "A walk is a sequence of alternating vertices "
                                               "and edges such as v<sub>0</sub>e<sub>1</sub>, "
                                               "v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, "
                                               "e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> "
                                               "is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                                               "This function counts the number of walks of any length "
                                               "between each pair of nodes, by studying the powers of the sociomatrix. \n"));
    connect(analyzeGraphWalksTotalAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksTotal() )  );


    analyzeMatrixReachabilityAct = new QAction(QIcon(":/images/walk.png"), tr("Reachability Matrix"),this);
    analyzeMatrixReachabilityAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_R));
    analyzeMatrixReachabilityAct->setStatusTip(tr("Compute the Reachability Matrix of the network."));
    analyzeMatrixReachabilityAct->setWhatsThis(tr("Reachability Matrix\n\n"
                                                  "Calculates the reachability matrix X<sup>R</sup> of "
                                                  "the graph where the {i,j} element is 1 if "
                                                  "the vertices i and j are reachable. \n\n"
                                                  "Actually, this just checks whether the corresponding element "
                                                  "of Distances matrix is not zero.\n"));
    connect(analyzeMatrixReachabilityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeReachabilityMatrix() )  );



    clusteringCoefAct = new QAction(QIcon(":/images/clucof.png"), tr("Local and Network Clustering Coefficient"),this);
    clusteringCoefAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_L) );
    clusteringCoefAct->setStatusTip(tr("Compute the Watts & Strogatz Clustering Coefficient for every actor and the network average."));
    clusteringCoefAct->setWhatsThis(tr("Local and Network Clustering Coefficient\n\n"
                                       "The local Clustering Coefficient  (Watts & Strogatz, 1998) "
                                       "of an actor quantifies how close "
                                       "the actor and her neighbors are to being a clique and "
                                       "can be used as an indication of network transitivity. \n"));
    connect(clusteringCoefAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeClusteringCoefficient() )  );





    analyzeCommunitiesCliquesAct = new QAction(QIcon(":/images/clique.png"), tr("Clique Census"),this);
    analyzeCommunitiesCliquesAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_U, Qt::CTRL | Qt::Key_C));
    analyzeCommunitiesCliquesAct->setStatusTip(tr("Compute the clique census: find all maximal connected subgraphs."));
    analyzeCommunitiesCliquesAct->setWhatsThis(tr("Clique Census\n\n"
                                                  "Produces the census of network cliques (maximal connected subgraphs), "
                                                  "along with disaggregation by actor and co-membership information. "));
    connect(analyzeCommunitiesCliquesAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesCliqueCensus() )  );



    analyzeCommunitiesTriadCensusAct = new QAction(QIcon(":/images/triad.png"), tr("Triad Census (M-A-N labeling)"),this);
    analyzeCommunitiesTriadCensusAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_U, Qt::CTRL | Qt::Key_T) );
    analyzeCommunitiesTriadCensusAct->setStatusTip(tr("Calculate the triad census for all actors."));
    analyzeCommunitiesTriadCensusAct->setWhatsThis(tr("Triad Census\n\n"
                                                      "A triad census counts all the different kinds of observed triads "
                                                      "within a network and codes them according to their number of mutual, "
                                                      "asymmetric and non-existent dyads using the M-A-N labeling scheme. \n"));
    connect(analyzeCommunitiesTriadCensusAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesTriadCensus() )  );



    analyzeStrEquivalencePearsonAct = new QAction(QIcon(":/images/similarity.png"),
                                                  tr("Pearson correlation coefficients"),this);
    analyzeStrEquivalencePearsonAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_P)
                );
    analyzeStrEquivalencePearsonAct->setStatusTip(
                tr("Compute Pearson Correlation Coefficients between pairs of actors. "
                   "Most useful with valued/weighted ties (non-binary). "));
    analyzeStrEquivalencePearsonAct->setWhatsThis(
                tr("Pearson correlation coefficients\n\n"
                   "Computes a correlation matrix, where the elements are the "
                   "Pearson correlation coefficients between pairs of actors "
                   "in terms of their tie profiles or distances (in, out or both). \n\n"
                   "The Pearson product-moment correlation coefficient (PPMCC or PCC or Pearson's r)"
                   "is a measure of the linear dependence/association between two variables X and Y. \n\n"
                   "This correlation measure of similarity is particularly useful "
                   "when ties are valued/weighted denoting strength, cost or probability.\n\n"
                   "Note that in very sparse networks (very low density), measures such as"
                   "\"exact matches\", \"correlation\" and \"distance\" "
                   "will show little variation among the actors, causing "
                   "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalencePearsonAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalencePearsonDialog() )  );



    analyzeStrEquivalenceMatchesAct = new QAction(QIcon(":/images/similarity.png"),
                                                  tr("Similarity by measure (Exact, Jaccard, Hamming, Cosine, Euclidean)"),this);
    analyzeStrEquivalenceMatchesAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_E)
                );
    analyzeStrEquivalenceMatchesAct->setStatusTip(tr("Compute a pair-wise actor similarity "
                                                     "matrix based on a measure of their ties (or distances) \"matches\" ."));
    analyzeStrEquivalenceMatchesAct->setWhatsThis(
                tr("Actor Similarity by measure\n\n"
                   "Computes a pair-wise actor similarity matrix, where each element (i,j) is "
                   "the ratio of tie (or distance) matches of actors i and j to all other actors. \n\n"
                   "SocNetV supports the following matching measures: "
                   "Simple Matching (Exact Matches)"
                   "Jaccard Index (Positive Matches or Co-citation)"
                   "Hamming distance"
                   "Cosine similarity"
                   "Euclidean distance"
                   "For instance, if you select Exact Matches, a matrix element (i,j) = 0.5, "
                   "means that actors i and j have the same ties present or absent "
                   "to other actors 50% of the time. \n\n"
                   "These measures of similarity are particularly useful "
                   "when ties are binary (not valued).\n\n"
                   "Note that in very sparse networks (very low density), measures such as"
                   "\"exact matches\", \"correlation\" and \"distance\" "
                   "will show little variation among the actors, causing "
                   "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalenceMatchesAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceSimilarityMeasureDialog() )  );



    analyzeStrEquivalenceTieProfileDissimilaritiesAct = new QAction(QIcon(":/images/dm.png"),
                                                                    tr("Tie Profile Dissimilarities/Distances"),this);
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->setShortcut(
                QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_T) );
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->
            setStatusTip(
                tr("Compute tie profile dissimilarities/distances "
                   "(Euclidean, Manhattan, Jaccard, Hamming) between all pair of nodes.")
                );
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->
            setWhatsThis(
                tr("Tie Profile Dissimilarities/Distances\n\n"
                   "Computes a matrix of tie profile distances/dissimilarities "
                   "between all pairs of actors/nodes in the social network "
                   "using an ordinary metric such as Euclidean distance, "
                   "Manhattan distance, Jaccard distance or Hamming distance)."
                   "The resulted distance matrix is a n x n matrix, in which the "
                   "(i,j) element is the distance or dissimilarity between "
                   "the tie profiles of node i and node j."
                   )
                );
    connect(analyzeStrEquivalenceTieProfileDissimilaritiesAct, SIGNAL(triggered()),
            this, SLOT( slotAnalyzeStrEquivalenceDissimilaritiesDialog() ) );


    analyzeStrEquivalenceClusteringHierarchicalAct = new QAction(QIcon(":/images/hierarchical.png"),
                                                                 tr("Hierarchical clustering"),this);
    analyzeStrEquivalenceClusteringHierarchicalAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_H));

    analyzeStrEquivalenceClusteringHierarchicalAct->setStatusTip(
                tr("Perform agglomerative cluster analysis of the actors in the social network"));
    analyzeStrEquivalenceClusteringHierarchicalAct->setWhatsThis(
                tr("Hierarchical clustering\n\n"
                   "Hierarchical clustering (or hierarchical cluster analysis, HCA) "
                   "is a method of cluster analysis which builds a hierarchy "
                   "of clusters, based on their elements dissimilarity. "
                   "In SNA context these clusters usually consist of "
                   "network actors. \n"

                   "This method takes the social network distance matrix as input and uses "
                   "the Agglomerative \"bottom up\" approach where each "
                   "actor starts in its own cluster (Level 0). In each subsequent Level, "
                   "as we move up the clustering hierarchy, a pair of clusters "
                   "are merged into a larger cluster, until "
                   "all actors end up in the same cluster. "

                   "To decide which clusters should be combined at each level, a measure of "
                   "dissimilarity between sets of observations is required. "
                   "This measure consists of a metric for the distance between actors "
                   "(i.e. manhattan distance) and a linkage criterion (i.e. single-linkage clustering). "
                   "This linkage criterion (essentially a definition of distance between clusters), "
                   "differentiates between the different HCA methods."

                   "Note that the complexity of agglomerative clustering is O( n^2 log(n) ), "
                   "therefore is too slow for large data sets."
                   ));
    connect(analyzeStrEquivalenceClusteringHierarchicalAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceClusteringHierarchicalDialog() )  );


    cDegreeAct = new QAction(tr("Degree Centrality (DC)"),this);
    cDegreeAct->setShortcut(Qt::CTRL | Qt::Key_1);
    cDegreeAct
          ->setStatusTip(tr("Compute Degree Centrality indices for every actor and group Degree Centralization."));
    cDegreeAct
           ->setWhatsThis(
                tr( "Degree Centrality (DC)\n\n"
                    "For each node v, the DC index is the number of edges "
                    "attached to it (in undirected graphs) or the total number "
                    "of arcs (outLinks) starting from it (in digraphs).\n"
                    "This is often considered a measure of actor activity. \n\n"
                    "This index can be calculated in both graphs and digraphs "
                    "but is usually best suited for undirected graphs. "
                    "It can also be calculated in weighted graphs. "
                    "In weighted relations, DC is the sum of weights of all "
                    "edges/outLinks attached to v."));
    connect(cDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityDegree()));


    cClosenessAct = new QAction(tr("Closeness Centrality (CC)"), this);
    cClosenessAct->setShortcut(Qt::CTRL | Qt::Key_2);
    cClosenessAct
          ->setStatusTip(
                tr(
                    "Compute Closeness Centrality indices for every actor and group Closeness Centralization."));
    cClosenessAct
           ->setWhatsThis(
                tr("Closeness Centrality (CC)\n\n"
                   "For each node v, CC the inverse sum of "
                   "the shortest distances between v and every other node. CC is "
                   "interpreted as the ability to access information through the "
                   "\"grapevine\" of network members. Nodes with high closeness "
                   "centrality are those who can reach many other nodes in few steps. "
                   "\n\nThis index can be calculated in both graphs and digraphs. "
                   "It can also be calculated in weighted graphs although the weight of "
                   "each edge (v,u) in E is always considered to be 1. "));
    connect(cClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityCloseness()));

    cInfluenceRangeClosenessAct = new QAction(tr("Influence Range Closeness Centrality (IRCC)"), this);
    cInfluenceRangeClosenessAct->setShortcut(Qt::CTRL | Qt::Key_3);
    cInfluenceRangeClosenessAct
          ->setStatusTip(
                tr("Compute Influence Range Closeness Centrality indices for every actor "
                   "focusing on how proximate each one is"
                   "to others in its influence range"));
    cInfluenceRangeClosenessAct
           ->setWhatsThis(
                tr("Influence Range Closeness Centrality (IRCC)\n\n"
                   "For each node v, IRCC is the standardized inverse average distance "
                   "between v and every reachable node.\n"
                   "This improved CC index is optimized for graphs and directed graphs which "
                   "are not strongly connected. Unlike the ordinary CC, which is the inverted "
                   "sum of distances from node v to all others (thus undefined if a node is isolated "
                   "or the digraph is not strongly connected), IRCC considers only "
                   "distances from node v to nodes in its influence range J (nodes reachable from v). "
                   "The IRCC formula used is the ratio of the fraction of nodes reachable by v "
                   "(|J|/(n-1)) to the average distance of these nodes from v (sum(d(v,j))/|J|"));
    connect(cInfluenceRangeClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityClosenessIR()));

    cBetweennessAct = new QAction(tr("Betweenness Centrality (BC)"), this);
    cBetweennessAct->setShortcut(Qt::CTRL | Qt::Key_4);
    cBetweennessAct->setWhatsThis(tr("Betweenness Centrality (BC)\n\n"
                                     "For each node v, BC is the ratio of all geodesics between pairs of nodes which run through v. "
                                     "It reflects how often an node lies on the geodesics between the other nodes of the network. "
                                     "It can be interpreted as a measure of control. "
                                     "A node which lies between many others is assumed to have a higher likelihood of being able "
                                     "to control information flow in the network. \n\n"
                                     "Note that betweenness centrality assumes that all geodesics "
                                     "have equal weight or are equally likely to be chosen for the flow of information "
                                     "between any two nodes. This is reasonable only on \"regular\" networks where all "
                                     "nodes have similar degrees. On networks with significant degree variance you might want "
                                     "to try informational centrality instead. \n\nThis index can be calculated in both graphs "
                                     "and digraphs but is usually best suited for undirected graphs. It can also be calculated"
                                     " in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    cBetweennessAct->setStatusTip(tr("Compute Betweenness Centrality indices and group Betweenness Centralization."));
    connect(cBetweennessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityBetweenness()));

    cStressAct = new QAction(tr("Stress Centrality (SC)"), this);
    cStressAct->setShortcut(Qt::CTRL | Qt::Key_5);
    cStressAct->setStatusTip(tr("Compute Stress Centrality indices for every actor and group Stress Centralization."));
    cStressAct->setWhatsThis(tr("Stress Centrality (SC)\n\n"
                                "For each node v, SC is the total number of geodesics between all other nodes which run through v. "
                                "A node with high SC is considered 'stressed', since it is traversed by a high number of geodesics. "
                                "When one node falls on all other geodesics between all the remaining (N-1) nodes, "
                                "then we have a star graph with maximum Stress Centrality. \n\n"
                                "This index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. "
                                "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cStressAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityStress()));


    cEccentAct = new QAction(tr("Eccentricity Centrality (EC)"), this);
    cEccentAct->setShortcut(Qt::CTRL | Qt::Key_6);
    cEccentAct->setStatusTip(tr("Compute Eccentricity Centrality (aka Harary Graph Centrality) scores for each node."));
    cEccentAct->setWhatsThis(
                tr("Eccentricity Centrality (EC)\n\n "
                   "This index is also known as Harary Graph Centrality. "
                   "For each node i, "
                   "the EC is the inverse of the maximum geodesic distance "
                   "of that v to all other nodes in the network. \n"
                   "Nodes with high EC have short distances to all other nodes "
                   "This index can be calculated in both graphs and digraphs "
                   "but is usually best suited for undirected graphs. "
                   "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cEccentAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEccentricity()));


    cPowerAct = new QAction(tr("Gil and Schmidt Power Centrality (PC)"), this);
    cPowerAct->setShortcut(Qt::CTRL | Qt::Key_7);
    cPowerAct->setStatusTip(tr("Compute Power Centrality indices (aka Gil-Schmidt Power Centrality) for every actor and group Power Centralization"));
    cPowerAct->setWhatsThis(tr("Power Centrality (PC)\n\n "
                               "For each node v, this index sums its degree (with weight 1), with the size of the 2nd-order neighbourhood (with weight 2), and in general, with the size of the kth order neighbourhood (with weight k). Thus, for each node in the network the most important other nodes are its immediate neighbours and then in decreasing importance the nodes of the 2nd-order neighbourhood, 3rd-order neighbourhood etc. For each node, the sum obtained is normalised by the total numbers of nodes in the same component minus 1. Power centrality has been devised by Gil-Schmidt. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1 (therefore not considered)."));
    connect(cPowerAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityPower()));


    cInformationAct = new QAction(tr("Information Centrality (IC)"),	this);
    cInformationAct->setShortcut(Qt::CTRL | Qt::Key_8);
    cInformationAct->setEnabled(true);
    cInformationAct->setStatusTip(tr("Compute Information Centrality indices and group Information Centralization"));
    cInformationAct->setWhatsThis(
                tr("Information Centrality (IC)\n\n"
                   "Information centrality counts all paths between "
                   "nodes weighted by strength of tie and distance. "
                   "This centrality  measure developed by Stephenson and Zelen (1989) "
                   "focuses on how information might flow through many different paths. \n\n"
                   "This index should be calculated only for  graphs. \n\n"
                   "Note: To compute this index, SocNetV drops all isolated nodes."));
    connect(cInformationAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityInformation()));


    cEigenvectorAct = new QAction(tr("Eigenvector Centrality (EVC)"),	this);
    cEigenvectorAct->setShortcut(Qt::CTRL | Qt::Key_9);
    cEigenvectorAct->setEnabled(true);
    cEigenvectorAct->setStatusTip(tr("Compute Eigenvector Centrality indices and group Eigenvector Centralization"));
    cEigenvectorAct->setWhatsThis(
                tr("Eigenvector Centrality (EVC)\n\n"
                   "Computes the Eigenvector centrality of each node in a social network "
                   "which is defined as the ith element of the leading eigenvector "
                   "of the adjacency matrix. The leading eigenvector is the "
                   "eigenvector corresponding to the largest positive eigenvalue."
                   "The Eigenvector Centrality, proposed by Bonacich (1989), is "
                   "an extension of the simpler Degree Centrality because it gives "
                   "each actor a score proportional to the scores of its neighbors. "
                   "Thus, a node may be important, in terms of its EC, because it "
                   "has lots of ties or it has fewer ties to important other nodes."));
    connect(cEigenvectorAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEigenvector()));



    cInDegreeAct = new QAction(tr("Degree Prestige (DP)"),	 this);
    cInDegreeAct->setStatusTip(tr("Compute Degree Prestige (InDegree) indices "));
    cInDegreeAct->setShortcut(Qt::CTRL | Qt::Key_I);
    cInDegreeAct->setWhatsThis(tr("InDegree (Degree Prestige)\n\n"
                                  "For each node k, this the number of arcs ending at k. "
                                  "Nodes with higher in-degree are considered more prominent among others. "
                                  "In directed graphs, this index measures the prestige of each node/actor. "
                                  "Thus it is called Degree Prestige. "
                                  "Nodes who are prestigious tend to receive many nominations or choices (in-links). "
                                  "The largest the index is, the more prestigious is the node. \n\n"
                                  "This index can be calculated only for digraphs. "
                                  "In weighted relations, DP is the sum of weights of all arcs/inLinks ending at node v."));
    connect(cInDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeDegree()));

    cPageRankAct = new QAction(tr("PageRank Prestige (PRP)"),	this);
    cPageRankAct->setShortcut(Qt::CTRL | Qt::Key_K);
    cPageRankAct->setEnabled(true);
    cPageRankAct->setStatusTip(tr("Compute PageRank Prestige indices for every actor"));
    cPageRankAct->setWhatsThis(tr("PageRank Prestige\n\n"
                                  "An importance ranking for each node based on the link structure of the network. "
                                  "PageRank, developed by Page and Brin (1997), focuses on how nodes are "
                                  "connected to each other, treating each edge from a node as a citation/backlink/vote to another. "
                                  "In essence, for each node PageRank counts all backlinks to it, "
                                  "but it does so by not counting all edges equally while it "
                                  "normalizes each edge from a node by the total number of edges from it. "
                                  "PageRank is calculated iteratively and it corresponds to the principal "
                                  "eigenvector of the normalized link matrix. \n\n"
                                  "This index can be calculated in both graphs and digraphs but is "
                                  "usually best suited for directed graphs since it is a prestige measure. "
                                  "It can also be calculated in weighted graphs. "
                                  "In weighted relations, each backlink to a node v from another node u is "
                                  "considered to have weight=1 but it is normalized by the sum of "
                                  "outLinks weights (outDegree) of u. Therefore, nodes with high outLink "
                                  "weights give smaller percentage of their PR to node v."));
    connect(cPageRankAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigePageRank()));

    cProximityPrestigeAct = new QAction(tr("Proximity Prestige (PP)"),	this);
    cProximityPrestigeAct->setShortcut(Qt::CTRL | Qt::Key_Y);
    cProximityPrestigeAct->setEnabled(true);
    cProximityPrestigeAct->setStatusTip(tr("Calculate and display Proximity Prestige (digraphs only)"));
    cProximityPrestigeAct
           ->setWhatsThis(
                tr("Proximity Prestige (PP) \n\n"
                   "This index measures how proximate a node v is to the nodes "
                   "in its influence domain I (the influence domain I of a node "
                   "is the number of other nodes that can reach it).\n\n"
                   "In PP calculation, proximity is based on distances to rather "
                   "than distances from node v. \n"
                   "To put it simply, in PP what matters is how close are all "
                   "the other nodes to node v. \n\n"
                   "The algorithm takes the average distance to node v of all "
                   "nodes in its influence domain, standardizes it by "
                   "multiplying with (N-1)/I and takes its reciprocal. "
                   "In essence, the formula SocNetV uses to calculate PP "
                   "is the ratio of the fraction of nodes that can reach node v, "
                   "to the average distance of that nodes to v: \n"
                   "PP = (I/(N-1))/(sum{d(u,v)}/I) \n"
                   "where the sum is over all nodes in I."));
    connect(cProximityPrestigeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeProximity()));


    /**
    Options menu actions
    */
    optionsNodeNumbersVisibilityAct = new QAction( tr("Display Node Numbers"), this );
    optionsNodeNumbersVisibilityAct->setStatusTip(
                tr("Toggle displaying of node numbers (this session only)"));
    optionsNodeNumbersVisibilityAct->setWhatsThis(
                tr("Display Node Numbers\n\n"
                   "Enables or disables displaying of node numbers\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsNodeNumbersVisibilityAct->setCheckable (true);
    optionsNodeNumbersVisibilityAct->setChecked (
                ( appSettings["initNodeNumbersVisibility"] == "true" ) ? true: false );
    connect(optionsNodeNumbersVisibilityAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsNodeNumbersVisibility(bool)));


    optionsNodeNumbersInsideAct = new QAction(tr("Display Numbers Inside Nodes"),	this );
    optionsNodeNumbersInsideAct->setStatusTip(
                tr("Toggle displaying of numbers inside nodes (this session only)"));
    optionsNodeNumbersInsideAct->setWhatsThis(
                tr("Display Numbers Inside Nodes\n\n"
                   "Enables or disables displaying node numbers inside nodes.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsNodeNumbersInsideAct->setCheckable (true);
    optionsNodeNumbersInsideAct->setChecked(
                ( appSettings["initNodeNumbersInside"] == "true" ) ? true: false );
    connect(optionsNodeNumbersInsideAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsNodeNumbersInside(bool)));


    optionsNodeLabelsVisibilityAct= new QAction(tr("Display Node Labels"),	this );
    optionsNodeLabelsVisibilityAct->setStatusTip(
                tr("Toggle displaying of node labels (this session only)"));
    optionsNodeLabelsVisibilityAct->setWhatsThis(
                tr("Display Node Labels\n\n"
                   "Enables or disables node labels.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsNodeLabelsVisibilityAct->setCheckable (true);
    optionsNodeLabelsVisibilityAct->setChecked(
                ( appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false );
    connect(optionsNodeLabelsVisibilityAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsNodeLabelsVisibility(bool)));


    optionsEdgesVisibilityAct = new QAction(tr("Display Edges"), this);
    optionsEdgesVisibilityAct->setStatusTip(tr("Toggle displaying edges (this session only)"));
    optionsEdgesVisibilityAct->setWhatsThis(
                tr("Display Edges\n\n"
                   "Enables or disables displaying of edges"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsEdgesVisibilityAct->setCheckable(true);
    optionsEdgesVisibilityAct->setChecked(
                (appSettings["initEdgesVisibility"] == "true") ? true: false
                                                                 );
    connect(optionsEdgesVisibilityAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesVisibility(bool)) );


    optionsEdgeWeightNumbersAct = new QAction(tr("Display Edge Weights"),	this);
    optionsEdgeWeightNumbersAct->setStatusTip(
                tr("Toggle displaying of numbers of edge weights (this session only)"));
    optionsEdgeWeightNumbersAct->setWhatsThis(
                tr("Display Edge Weights\n\n"
                   "Enables or disables displaying edge weight numbers.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsEdgeWeightNumbersAct->setCheckable(true);
    connect(optionsEdgeWeightNumbersAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightNumbersVisibility(bool)) );

    optionsEdgeWeightConsiderAct = new QAction(tr("Consider Edge Weights in Calculations"),	this);
    optionsEdgeWeightConsiderAct->
            setStatusTip(
                tr("Toggle considering edge weights during calculations "
                   "(i.e. distances, centrality, etc) (this session only)"));
    optionsEdgeWeightConsiderAct->
            setWhatsThis(
                tr("Consider Edge Weights in Calculations\n\n"
                   "Enables or disables considering edge weights during "
                   "calculations (i.e. distances, centrality, etc).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsEdgeWeightConsiderAct->setCheckable(true);
    optionsEdgeWeightConsiderAct->setChecked(false);
    connect(optionsEdgeWeightConsiderAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightsDuringComputation(bool)) );


    optionsEdgeLabelsAct = new QAction(tr("Display Edge Labels"),	this);
    optionsEdgeLabelsAct->setStatusTip(
                tr("Toggle displaying of Edge labels, if any (this session only)"));
    optionsEdgeLabelsAct->setWhatsThis(
                tr("Display Edge Labes\n\n"
                   "Enables or disables displaying edge labels.\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsEdgeLabelsAct->setCheckable(true);
    optionsEdgeLabelsAct->setChecked(
                (appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                                                                      );
    connect(optionsEdgeLabelsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeLabelsVisibility(bool)) );


    optionsEdgeArrowsAct = new QAction( tr("Display Edge Arrows"),this);
    optionsEdgeArrowsAct->setStatusTip(
                tr("Toggle displaying directional Arrows on edges (this session only)"));
    optionsEdgeArrowsAct->setWhatsThis(
                tr("Display edge Arrows\n\n"
                   "Enables or disables displaying of arrows on edges.\n\n"
                   "Useful if all links are reciprocal (undirected graph).\n"
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    optionsEdgeArrowsAct->setCheckable(true);
    optionsEdgeArrowsAct->setChecked(
                (appSettings["initEdgeArrows"]=="true") ? true: false
                                                          );
    connect(optionsEdgeArrowsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeArrowsVisibility(bool)) );

    optionsEdgeThicknessPerWeightAct = new QAction( tr("Edge Thickness reflects Weight"), this);
    optionsEdgeThicknessPerWeightAct->setStatusTip(tr("Draw edges as thick as their weights (if specified)"));
    optionsEdgeThicknessPerWeightAct->setWhatsThis(
                tr("Edge thickness reflects weight\n\n"
                   "Click to toggle having all edges as thick as their weight (if specified)"));
    optionsEdgeThicknessPerWeightAct->setCheckable(true);
    optionsEdgeThicknessPerWeightAct->setChecked(
                (appSettings["initEdgeThicknessPerWeight"]=="true") ? true: false
                                                                      );
    connect(optionsEdgeThicknessPerWeightAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeThicknessPerWeight(bool)) );
    optionsEdgeThicknessPerWeightAct->setEnabled(false);

    drawEdgesBezier = new QAction( tr("Bezier Curves"),	this);
    drawEdgesBezier->setStatusTip(tr("Draw Edges as Bezier curves"));
    drawEdgesBezier->setWhatsThis(
                tr("Edges Bezier\n\n"
                   "Enable or disables drawing Edges as Bezier curves."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    drawEdgesBezier->setCheckable(true);
    drawEdgesBezier->setChecked (
                (appSettings["initEdgeShape"]=="bezier") ? true: false
                                                           );
    drawEdgesBezier->setEnabled(false);
    connect(drawEdgesBezier, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesBezier(bool)) );


    changeBackColorAct = new QAction(QIcon(":/images/format_color_fill_48px.svg"), tr("Change Background Color"), this);
    changeBackColorAct->setStatusTip(tr("Change the canvasbackground color"));
    changeBackColorAct->setWhatsThis(tr("Background Color\n\n"
                                        "Changes the background color of the canvas"));
    connect(changeBackColorAct, SIGNAL(triggered()),
            this, SLOT(slotOptionsBackgroundColor()));


    backgroundImageAct = new QAction(QIcon(":/images/wallpaper_48px.svg"), tr("Background Image (this session)"),	this);
    backgroundImageAct->setStatusTip(
                tr("Select and display a custom image in the background"
                   "(for this session only)"));
    backgroundImageAct->setWhatsThis(
                tr("Background image\n\n"
                   "Enable to select an image file from your computer, "
                   "which will be displayed in the background instead of plain color."
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    backgroundImageAct->setCheckable(true);
    backgroundImageAct->setChecked(false);
    connect(backgroundImageAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsBackgroundImageSelect(bool)));


    fullScreenModeAct = new QAction(QIcon(":/images/fullscreen_48px.svg"), tr("Full screen (this session)"),	this);
    fullScreenModeAct->setShortcut(QKeySequence::FullScreen);
    fullScreenModeAct->setStatusTip(
                tr("Toggle full screen mode (for this session only)"));
    fullScreenModeAct->setWhatsThis(
                tr("Full Screen Mode\n\n"
                   "Enable to show application window in full screen mode. "
                   "This setting will apply to this session only. \n"
                   "To permanently change it, go to Settings."));
    fullScreenModeAct->setCheckable(true);
    fullScreenModeAct->setChecked(false);
    connect(fullScreenModeAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsWindowFullScreen(bool)));



    openSettingsAct = new QAction(QIcon(":/images/settings_48px.svg"), tr("Settings"),	this);
    openSettingsAct->setShortcut(Qt::CTRL | Qt::Key_Comma);
    openSettingsAct->setEnabled(true);
    openSettingsAct->setToolTip(
                tr("Open the Settings dialog where you can save your preferences "
                   "for all future sessions"));
    openSettingsAct->setStatusTip(
                tr("Open the Settings dialog to save your preferences "
                   "for all future sessions"));
    openSettingsAct->setWhatsThis(
                tr("Settings\n\n"
                   "Opens the Settings dialog where you can edit and save settings "
                   "permanently for all subsequent sessions."));
    connect(openSettingsAct, SIGNAL(triggered()),
            this, SLOT(slotOpenSettingsDialog()));




    /**
    Help menu actions
    */
    helpApp = new QAction(QIcon(":/images/help_48px.svg"), tr("Manual"),	this);
    helpApp->setShortcut(Qt::Key_F1);
    helpApp->setStatusTip(tr("Read the manual..."));
    helpApp->setWhatsThis(tr("Manual\n\nDisplays the documentation of SocNetV"));
    connect(helpApp, SIGNAL(triggered()), this, SLOT(slotHelp()));

    tipsApp = new QAction(QIcon(":/images/tip_24px.svg"), tr("Tip of the Day"), this);
    tipsApp->setStatusTip(tr("Read useful tips"));
    tipsApp->setWhatsThis(tr("Quick Tips\n\nDisplays some useful and quick tips"));
    connect(tipsApp, SIGNAL(triggered()), this, SLOT(slotHelpTips()));


    helpCheckUpdatesApp = new QAction(
                QIcon(":/images/system_update_alt_48px.svg"), tr("Check for Updates"),	this);
    helpCheckUpdatesApp->setStatusTip(tr("Open a browser to SocNetV website "
                                         "to check for a new version..."));
    helpCheckUpdatesApp->setWhatsThis(tr("Check Updates\n\n"
                                         "Open a browser to SocNetV website so "
                                         "that you can check yourself for updates"));
    connect(helpCheckUpdatesApp, SIGNAL(triggered()),
            this, SLOT(slotHelpCheckUpdateDialog()));


    helpSystemInfoAct = new QAction(QIcon(":/images/about_24px.svg"), tr("System Information"),	this);
    helpSystemInfoAct->setEnabled(true);
    helpSystemInfoAct->setStatusTip(tr("Show information about your system"));
    helpSystemInfoAct->setWhatsThis(
                tr("<p><b>System Information</b></p>"
                   "<p>Shows useful information about your system, "
                   "which you can include in your bug reports. </p>"));

    connect(helpSystemInfoAct, SIGNAL(triggered()), this, SLOT(slotHelpSystemInfo()));


    helpAboutApp = new QAction(QIcon(":/images/about_24px.svg"), tr("About SocNetV"), this);
    helpAboutApp->setStatusTip(tr("About SocNetV"));
    helpAboutApp->setWhatsThis(tr("About\n\nBasic information about SocNetV"));
    connect(helpAboutApp, SIGNAL(triggered()), this, SLOT(slotHelpAbout()));



    helpAboutQt = new QAction(QIcon(":/images/qt.png"), tr("About Qt"), this);
    helpAboutQt->setStatusTip(tr("About Qt"));
    helpAboutQt->setWhatsThis(tr("About\n\nAbout Qt"));
    connect(helpAboutQt, SIGNAL(triggered()), this, SLOT(slotAboutQt() ) );


    qDebug()<< "Finished actions initialization.";
}



/**
 * @brief Populates the menu bar with our menu items.
 */
void MainWindow::initMenuBar() {

    qDebug()<< "Initializing menu bar...";

    /** NETWORK MENU */
    networkMenu = menuBar()->addMenu(tr("&Network"));
    networkMenu->addAction(networkNewAct);
    networkMenu->addAction(networkOpenAct);
    networkMenu->addSeparator();
    recentFilesSubMenu = new QMenu(tr("Recent &files..."));
    recentFilesSubMenu->setIcon(QIcon(":/images/recent_48px.svg"));
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFilesSubMenu->addAction(recentFileActs[i]);
    }

    slotNetworkFileRecentUpdateActions();

    networkMenu->addMenu (recentFilesSubMenu );
    networkMenu->addSeparator();
    importSubMenu = new QMenu(tr("&Import ..."));
    importSubMenu->setIcon(QIcon(":/images/file_upload_48px.svg"));
    importSubMenu->addAction(networkImportGMLAct);
    importSubMenu->addAction(networkImportPajekAct);
    importSubMenu->addAction(networkImportAdjAct);
    importSubMenu->addAction(networkImportTwoModeSM);
    importSubMenu->addAction(networkImportListAct);
    importSubMenu->addAction(networkImportUcinetAct);
    importSubMenu->addAction(networkImportGraphvizAct);
    networkMenu->addMenu (importSubMenu);

    networkMenu->addSeparator();
    networkMenu->addAction (openTextEditorAct);
    networkMenu->addAction (networkViewFileAct);
    networkMenu->addSeparator();
    networkMenu->addAction (networkViewSociomatrixAct);
    networkMenu->addAction (networkViewSociomatrixPlotAct);
    networkMenu->addSeparator();

    networkMenu->addAction (networkDataSetSelectAct);
    networkMenu->addSeparator();

    randomNetworkMenu = new QMenu(tr("Create &Random Network..."));
    randomNetworkMenu->setIcon(QIcon(":/images/random_48px.svg"));
    networkMenu->addMenu (randomNetworkMenu);

    randomNetworkMenu->addAction (networkRandomScaleFreeAct);
    randomNetworkMenu->addAction (networkRandomSmallWorldAct);
    randomNetworkMenu->addAction (networkRandomErdosRenyiAct );
    randomNetworkMenu->addAction (networkRandomLatticeAct);
    randomNetworkMenu->addAction (networkRandomRegularSameDegreeAct);
    randomNetworkMenu->addAction (networkRandomLatticeRingAct);
    // networkRandomGaussianAct->addTo(randomNetworkMenu);
    networkMenu->addSeparator();

    networkMenu->addAction(networkWebCrawlerAct);

    networkMenu->addSeparator();
    networkMenu->addAction(networkSaveAct);
    networkMenu->addAction(networkSaveAsAct);
    networkMenu->addSeparator();

    networkMenu->addAction (networkExportImageAct);
    networkMenu->addAction (networkExportPDFAct);
    networkMenu->addSeparator();
    exportSubMenu = networkMenu->addMenu(tr("Export to other..."));
    exportSubMenu->setIcon ( QIcon(":/images/file_download_48px.svg") );

    exportSubMenu->addAction (networkExportSMAct);
    exportSubMenu->addAction (networkExportPajek);
    //exportSubMenu->addAction (networkExportList);
    //exportSubMenu->addAction (networkExportDL);
    //exportSubMenu->addAction (networkExportGW);

    networkMenu->addSeparator();
    networkMenu->addAction(networkPrintAct);
    networkMenu->addSeparator();
    networkMenu->addAction(networkCloseAct);
    networkMenu->addAction(networkQuitAct);


    // EDIT MENU
    editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu->addAction (editRelationPreviousAct);
    editMenu->addAction (editRelationNextAct);
    editMenu->addAction (editRelationAddAct);
    editMenu->addAction (editRelationRenameAct);

    editMenu->addSeparator();

    editMenu->addAction ( zoomInAct );
    editMenu->addAction ( zoomOutAct );

    editMenu->addSeparator();

    editMenu->addAction ( editRotateLeftAct );
    editMenu->addAction ( editRotateRightAct );

    editMenu->addSeparator();
    editMenu->addAction (editResetSlidersAct );

    editMenu->addSeparator();
    editNodeMenu = new QMenu(tr("Nodes..."));
    editNodeMenu->setIcon(QIcon(":/images/node_48px.svg"));
    editMenu->addMenu ( editNodeMenu );
    editNodeMenu->addAction (editNodeSelectAllAct);
    editNodeMenu->addAction (editNodeSelectNoneAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction (editNodeFindAct);
    editNodeMenu->addAction (editNodeAddAct);
    editNodeMenu->addAction (editNodeRemoveAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction (editNodePropertiesAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction (editNodeSelectedToCliqueAct);
    editNodeMenu->addAction (editNodeSelectedToStarAct);
    editNodeMenu->addAction (editNodeSelectedToCycleAct);
    editNodeMenu->addAction (editNodeSelectedToLineAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction (editNodeColorAll);
    editNodeMenu->addAction (editNodeSizeAllAct);
    editNodeMenu->addAction (editNodeShapeAll);
    editNodeMenu->addSeparator();
    editNodeMenu->addAction (editNodeNumbersSizeAct);
    editNodeMenu->addAction (editNodeNumbersColorAct);
    editNodeMenu->addSeparator();
    editNodeMenu->addAction (editNodeLabelsSizeAct);
    editNodeMenu->addAction (editNodeLabelsColorAct);


    editEdgeMenu = new QMenu(tr("Edges..."));
    editEdgeMenu->setIcon(QIcon(":/images/edges_48px.svg"));
    editMenu->addMenu (editEdgeMenu);
    editEdgeMenu->addAction(editEdgeAddAct);
    editEdgeMenu->addAction(editEdgeRemoveAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction (editEdgeUndirectedAllAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction (editEdgeSymmetrizeAllAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction (editEdgeSymmetrizeStrongTiesAct);
    editEdgeMenu->addAction (editEdgesCocitationAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction (editEdgeDichotomizeAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeLabelAct);
    editEdgeMenu->addAction(editEdgeColorAct);
    editEdgeMenu->addAction(editEdgeWeightAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction (editEdgeColorAllAct);

    //   transformNodes2EdgesAct->addTo (editMenu);

    editMenu->addSeparator();
    filterMenu = new QMenu ( tr("Filter..."));
    filterMenu->setIcon(QIcon(":/images/filter_list_48px.svg"));
    editMenu->addMenu(filterMenu);

    filterMenu->addAction(filterNodesByCentralityAct );
    filterMenu->addAction(editFilterNodesIsolatesAct );
    filterMenu->addAction(editFilterEdgesByWeightAct );
    filterMenu->addAction(editFilterEdgesUnilateralAct);

    // ANALYZE MENU
    analysisMenu = menuBar()->addMenu(tr("&Analyze"));
    matrixMenu = new QMenu(tr("Adjacency Matrix and Matrices..."));
    matrixMenu->setIcon(QIcon(":/images/sociomatrix_48px.svg"));
    analysisMenu->addMenu (matrixMenu);
    matrixMenu->addAction (networkViewSociomatrixAct);
    matrixMenu->addAction (networkViewSociomatrixPlotAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction (analyzeMatrixAdjInvertAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixAdjTransposeAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixAdjCocitationAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction (analyzeMatrixDegreeAct);
    matrixMenu->addAction (analyzeMatrixLaplacianAct);
    //	analysisMenu->addAction (netDensity);

    analysisMenu->addSeparator();
    cohesionMenu = new QMenu(tr("Cohesion..."));
    cohesionMenu->setIcon(QIcon(":/images/assessment_48px.svg"));
    analysisMenu->addMenu(cohesionMenu);
    cohesionMenu->addAction (analyzeGraphReciprocityAct);
    cohesionMenu->addAction (analyzeGraphSymmetryAct);
    cohesionMenu->addSection("Graph distances");
    cohesionMenu->addAction (analyzeGraphDistanceAct);
    cohesionMenu->addAction (averGraphDistanceAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction (analyzeMatrixDistancesGeodesicAct);
    cohesionMenu->addAction (analyzeMatrixGeodesicsAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction (analyzeGraphEccentricityAct);
    cohesionMenu->addAction (analyzeGraphDiameterAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeGraphConnectednessAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction (analyzeGraphWalksAct);
    cohesionMenu->addAction (analyzeGraphWalksTotalAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction (analyzeMatrixReachabilityAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction (clusteringCoefAct);

    analysisMenu->addSeparator();

    // CENTRALITIES
    centrlMenu = new QMenu(tr("Centrality and Prestige indices..."));
    centrlMenu->setIcon(QIcon(":/images/centrality_48px.svg"));
    analysisMenu->addMenu(centrlMenu);

    centrlMenu->addAction (cDegreeAct);
    centrlMenu->addAction (cClosenessAct);
    centrlMenu->addAction (cInfluenceRangeClosenessAct);
    centrlMenu->addAction (cBetweennessAct);
    centrlMenu->addAction (cStressAct);
    centrlMenu->addAction (cEccentAct);
    centrlMenu->addAction (cPowerAct);
    centrlMenu->addAction (cInformationAct);
    centrlMenu->addAction (cEigenvectorAct);
    centrlMenu->addSeparator();
    centrlMenu->addAction (cInDegreeAct);
    centrlMenu->addAction (cPageRankAct);
    centrlMenu->addAction (cProximityPrestigeAct);


    analysisMenu->addSeparator();
    // COMMUNITIES & SUBGROUPS
    communitiesMenu = new QMenu(tr("Communities and Subgroups..."));
    communitiesMenu->setIcon(QIcon(":/images/communities_48px.svg"));
    analysisMenu->addMenu(communitiesMenu);
    communitiesMenu->addAction (analyzeCommunitiesCliquesAct);
    communitiesMenu->addSeparator();
    communitiesMenu->addAction (analyzeCommunitiesTriadCensusAct);


    analysisMenu->addSeparator();
    // STRUCTURAL EQUIVALENCE
    strEquivalenceMenu = new QMenu(tr("Structural Equivalence..."));
    strEquivalenceMenu->setIcon(QIcon(":/images/similarity.png"));
    analysisMenu->addMenu (strEquivalenceMenu);
    strEquivalenceMenu->addAction (analyzeStrEquivalencePearsonAct);
    strEquivalenceMenu->addAction(analyzeStrEquivalenceMatchesAct);
    strEquivalenceMenu->addSeparator();
    strEquivalenceMenu->addAction (analyzeStrEquivalenceTieProfileDissimilaritiesAct);
    strEquivalenceMenu->addSeparator();
    strEquivalenceMenu->addAction (analyzeStrEquivalenceClusteringHierarchicalAct);


    // LAYOUT MENU
    layoutMenu = menuBar()->addMenu(tr("&Layout"));
    //   colorationMenu = new QPopupMenu();
    //   layoutMenu->insertItem (tr("Colorization"), colorationMenu);
    //   strongColorationAct->addTo(colorationMenu);
    //   regularColorationAct->addTo(colorationMenu);
    //   layoutMenu->insertSeparator();
    randomLayoutMenu = new QMenu(tr("Random..."));
    randomLayoutMenu->setIcon(QIcon(":/images/random_48px.svg"));
    layoutMenu->addMenu (randomLayoutMenu );
    randomLayoutMenu->addAction(layoutRandomAct);
    randomLayoutMenu->addAction( layoutRandomRadialAct );
    layoutMenu->addSeparator();

    layoutRadialProminenceMenu = new QMenu(tr("Radial by prominence index..."));
    layoutRadialProminenceMenu->setIcon(QIcon(":/images/radial_layout_48px.svg"));
    layoutMenu->addMenu (layoutRadialProminenceMenu);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_DC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_CC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_IRCC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_BC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_SC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_EC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_PC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_IC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_EVC_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_DP_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_PRP_Act);
    layoutRadialProminenceMenu->addAction (layoutRadialProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutLevelProminenceMenu = new QMenu (tr("On Levels by prominence index..."));
    layoutLevelProminenceMenu->setIcon(QIcon(":/images/layout_levels_24px.svg"));
    layoutMenu->addMenu (layoutLevelProminenceMenu);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_DC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_CC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_IRCC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_BC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_SC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_EC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_PC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_IC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_EVC_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_DP_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_PRP_Act);
    layoutLevelProminenceMenu->addAction (layoutLevelProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeSizeProminenceMenu = new QMenu (tr("Node Size by prominence index..."));
    layoutNodeSizeProminenceMenu->setIcon(QIcon(":/images/node_size_48px.svg"));
    layoutMenu->addMenu (layoutNodeSizeProminenceMenu);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_DC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_CC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_IRCC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_BC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_SC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_EC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_PC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_IC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_EVC_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_DP_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_PRP_Act);
    layoutNodeSizeProminenceMenu->addAction (layoutNodeSizeProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeColorProminenceMenu = new QMenu (tr("Node Color by prominence index..."));
    layoutNodeColorProminenceMenu->setIcon(QIcon(":/images/color_layout_48px.svg"));
    layoutMenu->addMenu (layoutNodeColorProminenceMenu);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_DC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_CC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_IRCC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_BC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_SC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_EC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_PC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_IC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_EVC_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_DP_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_PRP_Act);
    layoutNodeColorProminenceMenu->addAction (layoutNodeColorProminence_PP_Act);


    layoutMenu->addSeparator();

    layoutForceDirectedMenu = new QMenu (tr("Force-Directed Placement..."));
    layoutForceDirectedMenu->setIcon(QIcon(":/images/force.png"));
    layoutMenu->addMenu (layoutForceDirectedMenu);
    layoutForceDirectedMenu->addAction (layoutFDP_KamadaKawai_Act);
    layoutForceDirectedMenu->addAction (layoutFDP_FR_Act);
    layoutForceDirectedMenu->addAction (layoutFDP_Eades_Act);

    layoutMenu->addSeparator();
    layoutMenu->addAction (layoutGuidesAct);


    // OPTIONS MENU
    optionsMenu = menuBar()->addMenu(tr("&Options"));
    nodeOptionsMenu=new QMenu(tr("Nodes..."));
    nodeOptionsMenu->setIcon(QIcon(":/images/node_48px.svg"));

    optionsMenu->addMenu (nodeOptionsMenu);
    nodeOptionsMenu->addAction (optionsNodeNumbersVisibilityAct);
    nodeOptionsMenu->addAction (optionsNodeLabelsVisibilityAct);
    nodeOptionsMenu->addAction (optionsNodeNumbersInsideAct);

    edgeOptionsMenu=new QMenu(tr("Edges..."));
    edgeOptionsMenu->setIcon(QIcon(":/images/edges_48px.svg"));

    optionsMenu->addMenu (edgeOptionsMenu);
    edgeOptionsMenu->addAction (optionsEdgesVisibilityAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction (optionsEdgeWeightNumbersAct);
    edgeOptionsMenu->addAction (optionsEdgeWeightConsiderAct);
    edgeOptionsMenu->addAction (optionsEdgeThicknessPerWeightAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction (optionsEdgeLabelsAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction (optionsEdgeArrowsAct );
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction (drawEdgesBezier);

    viewOptionsMenu = new QMenu (tr("&Canvas..."));
    viewOptionsMenu->setIcon(QIcon(":/images/view.png"));
    optionsMenu->addMenu (viewOptionsMenu);
    viewOptionsMenu->addAction (changeBackColorAct);
    viewOptionsMenu->addAction (backgroundImageAct);

    optionsMenu->addSeparator();
    optionsMenu->addAction(fullScreenModeAct);

    optionsMenu->addSeparator();
    optionsMenu->addAction (openSettingsAct);


    // HELP MENU
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction (helpApp);
    helpMenu->addAction (tipsApp);
    helpMenu->addSeparator();
    helpMenu->addAction (helpCheckUpdatesApp);
    helpMenu->addSeparator();
    helpMenu->addAction(helpSystemInfoAct);
    helpMenu->addAction (helpAboutApp);
    helpMenu->addAction (helpAboutQt);

    qDebug()<< "Finished menu bar init.";
}




/**
 * @brief Initializes the toolbar
 */
void MainWindow::initToolBar(){

    qDebug()<< "Initializing toolbar...";

    toolBar = addToolBar("operations");

    toolBar->addAction (networkNewAct);
    toolBar->addAction (networkOpenAct);
    toolBar->addAction (networkSaveAct);
    toolBar->addAction (networkPrintAct);

    toolBar->addSeparator();

    toolBar->addAction (editMouseModeInteractiveAct);
    toolBar->addAction (editMouseModeScrollAct);

    toolBar->addSeparator();

    //Create relation select widget
//    QLabel *labelRelationSelect= new QLabel;
//    labelRelationSelect->setText(tr("Relations:"));
//    toolBar->addWidget (labelRelationSelect);
    toolBar->addAction (editRelationPreviousAct);
    editRelationChangeCombo = new QComboBox;
    editRelationChangeCombo->setEditable(true);
    editRelationChangeCombo->setInsertPolicy(QComboBox::InsertAtCurrent);
    editRelationChangeCombo->setMinimumWidth(180);
    editRelationChangeCombo->setCurrentIndex(0);
    editRelationChangeCombo->setToolTip(
                tr("<p><b>Current relation<b></p>"
                   "<p>To rename the current relation, click here, enter new name and press Enter.</p>"));
    editRelationChangeCombo->setStatusTip(
                tr("Name of the current relation. "
                   "To rename it, enter a new name and press Enter. To select another relation, click the Down arrow (on the right)."));
    editRelationChangeCombo->setWhatsThis(
                tr("<p><b>Relations combo</b></p>"
                   "<p>This displays the currently selected relation of the network. </p>"
                   "<p>To rename the current relation, click on the name, enter a new name and press Enter. </p>"
                   "<p>To select another relation (if any), click the Down arrow (on the right).</p>"));

    toolBar->addWidget(editRelationChangeCombo);
    toolBar->addAction (editRelationNextAct);
    toolBar->addAction (editRelationAddAct);

    toolBar->addSeparator();

//    QLabel *labelEditNodes= new QLabel;
//    labelEditNodes->setText(tr("Nodes:"));
//    toolBar->addWidget (labelEditNodes);
    toolBar->addAction (editNodeAddAct);
    toolBar->addAction (editNodeRemoveAct);
    toolBar->addAction (editNodeFindAct);
    toolBar->addAction(editNodePropertiesAct );

    toolBar->addSeparator();

//    QLabel *labelEditEdges= new QLabel;
//    labelEditEdges->setText(tr("Edges:"));
//    toolBar->addWidget (labelEditEdges);

    toolBar->addAction (editEdgeAddAct);
    toolBar->addAction (editEdgeRemoveAct);
    toolBar->addAction (editFilterEdgesByWeightAct);

    toolBar->addSeparator();

//    QLabel *labelApplicationIcons = new QLabel;
//    labelApplicationIcons->setText(tr("Settings:"));
//    toolBar->addWidget(labelApplicationIcons);
    toolBar->addAction(openSettingsAct);
    toolBar->addSeparator();
    toolBar->addAction ( QWhatsThis::createAction (this));
    toolBar->setIconSize(QSize(16,16));

    qDebug()<< "Finished toolbar init.";
}








/**
 * @brief Creates docked panels for instant access to main app functionalities
 * and displaying statistics
 */
void MainWindow::initPanels(){

    qDebug()<< "Initializing panels...";

    //
    // create widgets for the Control Panel
    //

    QString helpMessage = "";

    QLabel *toolBoxNetworkAutoCreateSelectLabel  = new QLabel;
    toolBoxNetworkAutoCreateSelectLabel->setText(tr("Auto Create:"));
    toolBoxNetworkAutoCreateSelectLabel->setMinimumWidth(90);
    toolBoxNetworkAutoCreateSelectLabel->setStatusTip(
                tr("Create a network automatically (famous, random, or by using the web crawler)."));
    toolBoxNetworkAutoCreateSelect = new QComboBox;
    toolBoxNetworkAutoCreateSelect->setStatusTip(
                tr("Create a network automatically (famous, random, or by using the web crawler)."));
    helpMessage = tr("<p><b>Auto network creation</b></p> "
                     "<p>Create a new network automatically.</p>"
                     "<p>You may create a random network, recreate famous data-sets "
                     "or use the built-in web crawler to create a network of webpages. </p>"
                     );
    toolBoxNetworkAutoCreateSelect->setToolTip( helpMessage );
    toolBoxNetworkAutoCreateSelect->setWhatsThis( helpMessage );

    toolBoxNetworkAutoCreateSelect->setToolTip( helpMessage);
    toolBoxNetworkAutoCreateSelect->setWhatsThis( helpMessage );
    QStringList networkAutoCreateSelectCommands;
    networkAutoCreateSelectCommands << "Select"
                             << "Famous data sets"
                             << "Random scale-free"
                             << "Random small-world"
                             << "Random Erdős–Rényi"
                             << "Random lattice"
                             << "Random d-regular"
                             << "Random ring-lattice"
                             << "With Web Crawler";
    toolBoxNetworkAutoCreateSelect->addItems(networkAutoCreateSelectCommands);

    toolBoxNetworkAutoCreateSelect->setMinimumWidth(90);




    QLabel *toolBoxEditNodeSubgraphSelectLabel  = new QLabel;
    toolBoxEditNodeSubgraphSelectLabel->setText(tr("Subgraph:"));
    toolBoxEditNodeSubgraphSelectLabel->setMinimumWidth(90);
    toolBoxEditNodeSubgraphSelectLabel->setStatusTip(
                tr("Create a basic subgraph with selected nodes."));
    toolBoxEditNodeSubgraphSelect = new QComboBox;
    toolBoxEditNodeSubgraphSelect->setStatusTip(
                tr("Create a basic subgraph with selected nodes."));
    helpMessage = tr("<p><b>Subgraph creation</b></p> "
                     "<p>Create a basic subgraph from selected nodes.</p>"
                     "<p>Select some nodes with your mouse and then click on one of these"
                     "options to create a basic subgraph with them. </p>"
                     "<p>You can create a star, clique, line, etc subgraph.</p>"
                     "<p>There must be some nodes selected!</p>");
    toolBoxEditNodeSubgraphSelect->setToolTip( helpMessage );
    toolBoxEditNodeSubgraphSelect->setWhatsThis( helpMessage );

    toolBoxEditNodeSubgraphSelectLabel->setToolTip( helpMessage);
    toolBoxEditNodeSubgraphSelectLabel->setWhatsThis( helpMessage );
    QStringList editNodeSubgraphCommands;
    editNodeSubgraphCommands << "Select"
                             << "Clique"
                             << "Star"
                             << "Cycle"
                             << "Line";
    toolBoxEditNodeSubgraphSelect->addItems(editNodeSubgraphCommands);
    toolBoxEditNodeSubgraphSelect->setMinimumWidth(90);


    QLabel *toolBoxEdgeModeSelectLabel  = new QLabel;
    toolBoxEdgeModeSelectLabel->setText(tr("Edge Mode:"));
    toolBoxEdgeModeSelectLabel->setMinimumWidth(90);
    toolBoxEditEdgeModeSelect = new QComboBox;
    toolBoxEditEdgeModeSelect->setStatusTip(
                tr("Select the edge mode: directed or undirected."));
    helpMessage = tr("<p><b>Edge mode</b></p>"
                     "<p>In social networks and graphs, edges can be directed or undirected "
                     "(and the corresponding network is called directed or undirected as well).</p>"
                     "<p>This option lets you choose what the kind of edges you want in your network.<p>"
                     "<p>By selecting an option here, all edges of the network will change automatically. <p>"
                     "<p>For instance, if the network is directed and and you select \"undirected\" "
                     "then all the directed edges will become undirected <p>");
    toolBoxEditEdgeModeSelect->setToolTip( helpMessage );
    toolBoxEditEdgeModeSelect->setWhatsThis( helpMessage );
    QStringList edgeModeCommands;
    edgeModeCommands << "Directed"
                     << "Undirected";
    toolBoxEditEdgeModeSelect->addItems(edgeModeCommands);
    toolBoxEditEdgeModeSelect->setMinimumWidth(120);


    QLabel *toolBoxEditEdgeTransformSelectLabel  = new QLabel;
    toolBoxEditEdgeTransformSelectLabel->setText(tr("Transform:"));
    toolBoxEditEdgeTransformSelectLabel->setMinimumWidth(90);
    toolBoxEditEdgeTransformSelect = new QComboBox;
    toolBoxEditEdgeTransformSelect->setStatusTip(
                tr("Select a method to transform the network, i.e. transform all directed edges to undirected."));
    helpMessage = tr("<p><b>Transform Network Edges </b></p>"
                     "<p>Select a method to transform network edges. Available methods: </p>"

                       "<p><em>Symmetrize All Edges</em></p>"
                       "<p>Forces all edges in this relation to be reciprocated: "
                       "<p>If there is a directed edge from node A to node B "
                       "then a new directed edge from node B to node A will be "
                       " created, with the same weight. </p>"
                       "<p>The result is a symmetric network.</p>"

                     "<p><em>Symmetrize Edges by Strong Ties:</em></p>"
                     "<p>Creates a new symmetric relation by keeping strong ties only. </p>"
                     "<p>A tie between actors A and B is considered strong if both A -> B and B -> A exist. "
                     "Therefore, in the new relation, a reciprocated edge will be created between actors A and B "
                     "only if both arcs A->B and B->A were present in the current or all relations. </p>"
                     "<p>If the network is multi-relational, it will ask you whether "
                      "ties in the current relation or all relations are to be considered.</p>"

                     "<p><em>Symmetrize Edges by examining Cocitation:</em></p>"
                     "<p>Creates a new symmetric relation by connecting actors "
                     "that are cocitated by others. "
                     "In the new relation, an edge will be created between actor i and "
                     "actor j only if C(i,j) > 0, where C the Cocitation Matrix. </p>"
                    "<p>Thus the actor pairs cited by more common neighbors will appear "
                     "with a stronger tie between them than pairs those cited by fewer "
                     "common neighbors. "
                     "The resulting relation is symmetric.</p>"

                     "<p><em>Dichotomize Edges</em></p>"
                     "<p>Creates a new binary relation in a valued network using "
                     "edge dichotomization according to a given threshold value. "
                     "In the new dichotomized relation, an edge will exist between actor i and "
                     "actor j only if e(i,j) > threshold, where threshold is a user-defined value."
                     "The process is also known as compression and slicing.</p>"
                     );
    toolBoxEditEdgeTransformSelect->setToolTip( helpMessage );
    toolBoxEditEdgeTransformSelect->setWhatsThis( helpMessage );

    QStringList edgeTransformCommands;
    edgeTransformCommands << "Select"
                       << "Symmetrize All Ties"
                       << "Symmetrize Strong Ties"
                       << "Cocitation Network"
                       << "Edge Dichotomization";
    toolBoxEditEdgeTransformSelect->addItems(edgeTransformCommands);
    toolBoxEditEdgeTransformSelect->setMinimumWidth(120);


    //create a grid layout for Edit buttons

    QGridLayout *editGrid = new QGridLayout;
    editGrid->addWidget(toolBoxNetworkAutoCreateSelectLabel, 0,0);
    editGrid->addWidget(toolBoxNetworkAutoCreateSelect, 0,1);

    editGrid->addWidget(toolBoxEditNodeSubgraphSelectLabel, 1,0);
    editGrid->addWidget(toolBoxEditNodeSubgraphSelect, 1,1);
    editGrid->addWidget(toolBoxEdgeModeSelectLabel,2,0);
    editGrid->addWidget(toolBoxEditEdgeModeSelect,2,1);
    editGrid->addWidget(toolBoxEditEdgeTransformSelectLabel,3,0);
    editGrid->addWidget(toolBoxEditEdgeTransformSelect,3,1);

    editGrid->setSpacing(5);
    editGrid->setContentsMargins(5, 5, 5, 5);

    //create a groupbox "Network" - Inside, display the grid layout of widgets
    QGroupBox *editGroupBox= new QGroupBox(tr("Network"));
    editGroupBox->setLayout(editGrid);
    editGroupBox->setMaximumWidth(255);
    editGroupBox->setMinimumHeight(130);

    //create widgets for the "Analysis" box
    QLabel *toolBoxAnalysisMatricesSelectLabel = new QLabel;
    toolBoxAnalysisMatricesSelectLabel->setText(tr("Matrix:"));
    toolBoxAnalysisMatricesSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisMatricesSelect = new QComboBox;
    toolBoxAnalysisMatricesSelect->setStatusTip(
                tr("Select which matrix to compute and display, based on the "
                   "adjacency matrix of the current network."));
    helpMessage = tr("<p><b>Matrix Analysis</b></p>"
                     "<p>Compute and display the adjacency matrix and other matrices "
                     "based on the adjacency matrix of the current network. "
                     "Available options:"
                     "<p><em>Adjacency Matrix</em></p>"
                     "<p><em>Adjacency Matrix Plot</em></p>"
                     "<p><em>Inverse of Adjacency Matrix</em></p>"
                     "<p><em>Transpose of Adjacency Matrix</em></p>"
                     "<p><em>Cocitation Matrix </em></p>"
                     "<p><em>Degree Matrix </em></p>"
                     "<p><em>Laplacian Matrix </em></p>"
                     );
    toolBoxAnalysisMatricesSelect->setToolTip( helpMessage );
    toolBoxAnalysisMatricesSelect->setWhatsThis( helpMessage );
    QStringList graphMatricesList;
    graphMatricesList << "Select"
                      << "Adjacency"
                      << "Adjacency Plot"
                      << "Adjacency Inverse"
                      << "Adjacency Transpose"
                      << "Cocitation Matrix"
                      << "Degree Matrix"
                      << "Laplacian Matrix";
    toolBoxAnalysisMatricesSelect->addItems(graphMatricesList);
    toolBoxAnalysisMatricesSelect->setMinimumWidth(120);



    QLabel *toolBoxAnalysisCohesionSelectLabel = new QLabel;
    toolBoxAnalysisCohesionSelectLabel->setText(tr("Cohesion:"));
    toolBoxAnalysisCohesionSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisCohesionSelect = new QComboBox;
    toolBoxAnalysisCohesionSelect->setStatusTip(
                tr("Select a graph-theoretic measure, i.e. distances, walks, graph diameter, eccentricity."));
    helpMessage =
            tr("<p><b>Analyze Cohesion</b></p>"
               "<p><Compute basic graph-theoretic measures. "

               "<p><em>Reciprocity:</em><p>"
               "<p>Measures the likelihood that pairs of nodes in a directed network are mutually linked.</p>"

               "<p><em>Symmetry:</em><p>"
               "<p>Checks if the directed network is symmetric or not.<p>"

               "<p><em>Distances:</em></p>"
               "<p>Computes the matrix of geodesic distances between all pairs of nodes.<p>"

               "<p><em>Average Distance:</em></p>"
               "<p>Computes the average distance between all nodes.<p>"

               "<p><em>Graph Diameter:</em></p>"
               "<p>The maximum distance between any two nodes in the network.</p>"

               "<p><em>Walks:</em></p>"
               "<p>A walk is a sequence of edges and vertices (nodes), where "
               "each edge's endpoints are the two vertices adjacent to it. "
               "In a walk, vertices and edges may repeat."

               "<p><em>Eccentricity:</em></p>"
               "<p>The Eccentricity of each node is how far, at most, is from every other actor in the network.</p>"

               "<p><em>Reachability:</em></p>"
               "<p>Creates a matrix where an element (i,j) = 1 only if the actors i and j are reachable.</p>"

               "<p><em>Clustering Coefficient (CLC):</em></p>"
               "<p>The CLC score of each node  is the proportion of actual links "
               "between its neighbors divided by the number of links that could "
               "possibly exist between them. "
               "Quantifies how close each actor and its neighbors are to form "
               "a complete subgraph (clique)</p>");
    toolBoxAnalysisCohesionSelect->setToolTip( helpMessage );
    toolBoxAnalysisCohesionSelect->setWhatsThis(helpMessage);

    QStringList graphPropertiesList;
    graphPropertiesList << "Select"
                        << "Reciprocity"
                        << "Symmetry"
                        << "Distance"
                        << "Average Distance"
                        << "Distances Matrix"
                        << "Geodesics Matrix"
                        << "Eccentricity"
                        << "Diameter"
                        << "Connectedness"
                        << "Walks of given length"
                        << "Total Walks"
                        << "Reachability Matrix"
                        << "Clustering Coefficient";
    toolBoxAnalysisCohesionSelect->addItems(graphPropertiesList);
    toolBoxAnalysisCohesionSelect->setMinimumWidth(120);



    QLabel *toolBoxAnalysisProminenceSelectLabel  = new QLabel;
    toolBoxAnalysisProminenceSelectLabel->setText(tr("Prominence:"));
    toolBoxAnalysisProminenceSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisProminenceSelect = new QComboBox;
    toolBoxAnalysisProminenceSelect->setStatusTip(
                tr("Select a prominence metric to compute for each actor "
                   "and the whole network. ")
                );
    helpMessage = tr("<p><b>Prominence Analysis</b></p>"
                     "<p>Compute Centrality and Prestige indices, to measure how "
                     "<em>prominent</em> (important) "
                     "each actor (node) is inside the network. </p>"
                     "<p>Centrality measures quantify how central is each node by examining "
                     "its ties and its geodesic distances (shortest path lengths) to other nodes. "
                     "Most Centrality indices were designed for undirected graphs. </p>"

                     "<p>Prestige indices focus on \"choices received\" to a node. "
                     "These indices measure the nominations or ties to each node from all others (or inLinks). "
                     "Prestige indices are suitable (and can be calculated only) on directed graphs.</p>"

                     "<p>Available measures:</p>"

                     "<p><em>Degree Centrality (DC) </em></p>"
                     "<p>The sum of outbound edges or the sum of weights of outbound "
                     "edges from each node <em>i</em> to all adjacent nodes. Note: This is "
                     "the outDegree Centrality. To compute inDegree Centrality, "
                     "use the Degree Prestige measure.</p>"

                     "<p><em>Closeness Centrality (CC):</em></p>"
                     "The inverted sum of geodesic distances from each node <em>u</em> "
                     "to all other nodes. "

                     "<p><em>IR Closeness Centrality (IRCC):</em></p>"
                     "<p>The ratio of the fraction of nodes reachable by each node <em>u</em> "
                     "to the average distance of these nodes from <em>u</em>.</p>"

                     "<p><em>Betweenness Centrality (BC):</em></p>"
                     "<p>The sum of delta<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "delta<sub>(s,t,u)</sub> is the ratio of all geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Stress Centrality (SC):</em></p>"
                     "<p>The sum of sigma<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "sigma<sub>(s,t,u)</sub> is the number of geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Eccentricity Centrality (EC):</em></p>"
                     "<p>Also known as Harary Graph Centrality. The inverse maximum geodesic distance from node <em>u</em> to "
                     "all other nodes in the network."

                     "<p><em>Power Centrality (PC):</em></p>"
                     "<p>The sum of the sizes of all N<sub>th</sub>-order neighbourhoods "
                     "of node <em>u</em> with weight 1/n.</p>"

                     "<p><em>Information Centrality (IC):</em></p>"
                     "<p>Measures the information flow through all paths between actors weighted by "
                     "strength of tie and distance.</p>"

                     "<p><em>Eigenvector Centrality (EVC):</em></p>"
                     "<p>The EVC score of each node <em>i</em> is the i<sub>th</sub> element of the "
                     "leading eigenvector of the adjacency matrix, that is the "
                     "eigenvector corresponding to the largest positive eigenvalue. "

                     "<p><em>Degree Prestige (DP):</em></p>"
                     "<p>Also known as InDegree Centrality, it is the sum of inbound edges to a node <em>u</em> "
                     "from all adjacent nodes. </p>"

                     "<p><em>PageRank Prestige (PRP):</em></p>"
                     "<p>For each node <em>u</em> counts all inbound links (edges) to it, but "
                     "it normalizes each inbound link from another node <em>v</em> by the outDegree of <em>v</em>. </p>"

                     "<p><em>Proximity Prestige (PP):</em></p>"
                     "<p>The ratio of the proportion of nodes who can reach each node <em>u</em> "
                     "to the average distance these nodes are from it. Similar to Closeness Centrality "
                     "but it counts only inbound distances to each actor, thus it is a measure of actor prestige.</p>"
                     );
    toolBoxAnalysisProminenceSelect->setToolTip( helpMessage );
    toolBoxAnalysisProminenceSelect->setWhatsThis( helpMessage);


    // Used in toolBoxAnalysisProminenceSelect and DialogNodeFind
    prominenceIndexList  << "Degree Centrality"
                         << "Closeness Centrality"
                         << "IR Closeness Centrality"
                         << "Betweenness Centrality"
                         << "Stress Centrality"
                         << "Eccentricity Centrality"
                         << "Power Centrality"
                         << "Information Centrality"
                         << "Eigenvector Centrality"
                         << "Degree Prestige"
                         << "PageRank Prestige"
                         << "Proximity Prestige";

    QStringList prominenceCommands;
    prominenceCommands << "Select" << prominenceIndexList;
    toolBoxAnalysisProminenceSelect->addItems(prominenceCommands);
    toolBoxAnalysisProminenceSelect->setMinimumWidth(120);


    QLabel *toolBoxAnalysisCommunitiesSelectLabel  = new QLabel;
    toolBoxAnalysisCommunitiesSelectLabel->setText(tr("Communities:"));
    toolBoxAnalysisCommunitiesSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisCommunitiesSelect = new QComboBox;
    toolBoxAnalysisCommunitiesSelect->setStatusTip(
                tr("Select a community detection measure / cohesive subgroup algorithm, i.e. cliques, triad census etc."));
    helpMessage = tr("<p><b>Community Analysis</b></p>"
                     "<p>Community detection measures and cohesive subgroup algorithms, "
                     "to identify meaningful subgraphs in the graph.</p>"
                     "<p><b>Available measures</b></p>"
                     "<p><em>Clique Census:</em><p>"
                     "<p>Computes aggregate counts of all maximal cliques of actors by size, "
                     " actor by clique analysis, clique co-memberships</p>"
                     "<p><em>Triad Census:</em><p>"
                     "<p>Computes the Holland, Leinhardt and Davis triad census, which "
                     "counts all different classes of triads coded according to their"
                     "number of Mutual, Asymmetric and Non-existest dyads (M-A-N scheme)</p>"
                     );
    toolBoxAnalysisCommunitiesSelect->setToolTip( helpMessage );
    toolBoxAnalysisCommunitiesSelect->setWhatsThis( helpMessage );
    QStringList communitiesCommands;
    communitiesCommands << "Select"
                        << "Cliques"
                        << "Triad Census";
    toolBoxAnalysisCommunitiesSelect->addItems(communitiesCommands);
    toolBoxAnalysisCommunitiesSelect->setMinimumWidth(120);




    QLabel *toolBoxAnalysisStrEquivalenceSelectLabel  = new QLabel;
    toolBoxAnalysisStrEquivalenceSelectLabel->setText(tr("Equivalence:"));
    toolBoxAnalysisStrEquivalenceSelectLabel->setMinimumWidth(90);
    toolBoxAnalysisStrEquivalenceSelect = new QComboBox;
    toolBoxAnalysisStrEquivalenceSelect->setStatusTip(
                tr("Select a method to measure structural equivalence, "
                   "i.e. Pearson Coefficients, tie profile similarities, "
                   "hierarchical clustering, etc."));
    helpMessage =  tr("<p><b>Structural Equivalence Analysis</b></p>"
                      "<p>Select one of the available structural equivalence "
                      "measures and visualization algorithms. <p>"
                      "<p>Available options</p>"
                      "<p><em>Pearson Coefficients<.em></p>"
                      "<p><em>Tie profile similarities</em></p>"
                      "<p><em>Dissimilarities</em></p>"
                      "<p><em>Hierarchical Clustering Analysis</em></p>");
    toolBoxAnalysisStrEquivalenceSelect->setToolTip( helpMessage );
    toolBoxAnalysisStrEquivalenceSelect->setWhatsThis( helpMessage );
    QStringList connectivityCommands;
    connectivityCommands << "Select"
                         << "Pearson Coefficients"
                         << "Similarities"
                         << "Dissimilarities"
                         << "Hierarchical Clustering";
    toolBoxAnalysisStrEquivalenceSelect->addItems(connectivityCommands);
    toolBoxAnalysisStrEquivalenceSelect->setMinimumWidth(120);


    //create layout for analysis options
    QGridLayout *analysisGrid = new QGridLayout();
    analysisGrid->addWidget(toolBoxAnalysisMatricesSelectLabel, 0,0);
    analysisGrid->addWidget(toolBoxAnalysisMatricesSelect, 0,1);
    analysisGrid->addWidget(toolBoxAnalysisCohesionSelectLabel, 1,0);
    analysisGrid->addWidget(toolBoxAnalysisCohesionSelect, 1,1);
    analysisGrid->addWidget(toolBoxAnalysisProminenceSelectLabel, 2,0);
    analysisGrid->addWidget(toolBoxAnalysisProminenceSelect, 2,1);
    analysisGrid->addWidget(toolBoxAnalysisCommunitiesSelectLabel, 3,0);
    analysisGrid->addWidget(toolBoxAnalysisCommunitiesSelect, 3,1);
    analysisGrid->addWidget(toolBoxAnalysisStrEquivalenceSelectLabel, 4,0);
    analysisGrid->addWidget(toolBoxAnalysisStrEquivalenceSelect, 4,1);

    analysisGrid->setSpacing(5);
    analysisGrid->setContentsMargins(5, 5, 5, 5);


    //create a box and set the above layout inside
    QGroupBox *analysisBox= new QGroupBox(tr("Analyze"));
    analysisBox->setMinimumHeight(180);
    analysisBox->setMaximumWidth(255);
    analysisBox->setLayout (analysisGrid );


    //create widgets for the "Visualization By Index" box
    QLabel *toolBoxLayoutByIndexSelectLabel = new QLabel;
    toolBoxLayoutByIndexSelectLabel->setText(tr("Index:"));
    toolBoxLayoutByIndexSelectLabel->setMinimumWidth(70);
    toolBoxLayoutByIndexSelect = new QComboBox;
    toolBoxLayoutByIndexSelect->setStatusTip(tr("Select a prominence-based layout model"));
    helpMessage = tr("<p><b>Visualize by prominence index</b></p>"
                     "<p>Apply a prominence-based layout model to the network.</p>"
                     "<p>For instance, you can apply a degree centrality layout. </p>"

                     "<p>Note: For each prominence index, you must select a layout type (below).</p>"

                     "<p>Available measures:</p>"

                     "<p><em>Degree Centrality (DC) </em></p>"
                     "<p>The sum of outbound edges or the sum of weights of outbound "
                     "edges from each node <em>i</em> to all adjacent nodes. Note: This is "
                     "the outDegree Centrality. To compute inDegree Centrality, "
                     "use the Degree Prestige measure.</p>"

                     "<p><em>Closeness Centrality (CC):</em></p>"
                     "The inverted sum of geodesic distances from each node <em>u</em> "
                     "to all other nodes. "

                     "<p><em>IR Closeness Centrality (IRCC):</em></p>"
                     "<p>The ratio of the fraction of nodes reachable by each node <em>u</em> "
                     "to the average distance of these nodes from <em>u</em>.</p>"

                     "<p><em>Betweenness Centrality (BC):</em></p>"
                     "<p>The sum of delta<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "delta<sub>(s,t,u)</sub> is the ratio of all geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Stress Centrality (SC):</em></p>"
                     "<p>The sum of sigma<sub>(s,t,u)</sub> for all s,t ∈ V where "
                     "sigma<sub>(s,t,u)</sub> is the number of geodesics between nodes "
                     "<em>s</em> and <em>t</em> which run through node <em>u</em>.</p> "

                     "<p><em>Eccentricity Centrality (EC):</em></p>"
                     "<p>Also known as Harary Graph Centrality. The inverse maximum geodesic distance from node <em>u</em> to "
                     "all other nodes in the network."

                     "<p><em>Power Centrality (PC):</em></p>"
                     "<p>The sum of the sizes of all N<sub>th</sub>-order neighbourhoods "
                     "of node <em>u</em> with weight 1/n.</p>"

                     "<p><em>Information Centrality (IC):</em></p>"
                     "<p>Measures the information flow through all paths between actors weighted by "
                     "strength of tie and distance.</p>"

                     "<p><em>Eigenvector Centrality (EVC):</em></p>"
                     "<p>The EVC score of each node <em>i</em> is the i<sub>th</sub> element of the "
                     "leading eigenvector of the adjacency matrix, that is the "
                     "eigenvector corresponding to the largest positive eigenvalue. "

                     "<p><em>Degree Prestige (DP):</em></p>"
                     "<p>Also known as InDegree Centrality, it is the sum of inbound edges to a node <em>u</em> "
                     "from all adjacent nodes. </p>"

                     "<p><em>PageRank Prestige (PRP):</em></p>"
                     "<p>For each node <em>u</em> counts all inbound links (edges) to it, but "
                     "it normalizes each inbound link from another node <em>v</em> by the outDegree of <em>v</em>. </p>"

                     "<p><em>Proximity Prestige (PP):</em></p>"
                     "<p>The ratio of the proportion of nodes who can reach each node <em>u</em> "
                     "to the average distance these nodes are from it. Similar to Closeness Centrality "
                     "but it counts only inbound distances to each actor, thus it is a measure of actor prestige.</p>"
                     );
    toolBoxLayoutByIndexSelect->setToolTip( helpMessage );
    toolBoxLayoutByIndexSelect->setWhatsThis( helpMessage );
    QStringList layoutCommandsList;
    layoutCommandsList << "None" << "Random" << prominenceIndexList;

    toolBoxLayoutByIndexSelect->addItems(layoutCommandsList);
    toolBoxLayoutByIndexSelect->setMinimumHeight(20);
    toolBoxLayoutByIndexSelect->setMinimumWidth(100);


    QLabel *toolBoxLayoutByIndexTypeLabel = new QLabel;
    toolBoxLayoutByIndexTypeLabel->setText(tr("Type:"));
    toolBoxLayoutByIndexTypeLabel->setMinimumWidth(70);
    toolBoxLayoutByIndexTypeSelect = new QComboBox;
    toolBoxLayoutByIndexTypeSelect->setStatusTip(
                tr("Select layout type for the selected model"));
    helpMessage = tr("<p><b>Layout Type</b></p>"
                     "</p>Select a layout type (radial, level, node size or node color) "
                     "for the selected prominence-based model you want to apply to the "
                     "network. Please note that node coloring works only for basic shapes "
                     "(box, circle, etc) not for image icons.</p>");
    toolBoxLayoutByIndexTypeSelect->setToolTip( helpMessage );
    toolBoxLayoutByIndexTypeSelect->setWhatsThis( helpMessage );
    QStringList layoutTypes;
    layoutTypes << "Radial" << "On Levels" << "Node Size"<< "Node Color";
    toolBoxLayoutByIndexTypeSelect->addItems(layoutTypes);
    toolBoxLayoutByIndexTypeSelect->setMinimumHeight(20);
    toolBoxLayoutByIndexTypeSelect->setMinimumWidth(100);

    toolBoxLayoutByIndexApplyButton = new QPushButton(tr("Apply"));
    toolBoxLayoutByIndexApplyButton->setObjectName ("toolBoxLayoutByIndexApplyButton");
    toolBoxLayoutByIndexApplyButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutByIndexApplyButton->setMinimumHeight(20);
    toolBoxLayoutByIndexApplyButton->setMaximumWidth(60);


    //create layout for visualisation by index options
    QGridLayout *layoutByIndexGrid = new QGridLayout();
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexSelectLabel, 0,0);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexSelect, 0,1);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexTypeLabel, 1,0);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexTypeSelect, 1,1);
    layoutByIndexGrid->addWidget(toolBoxLayoutByIndexApplyButton, 2,1);
    layoutByIndexGrid->setSpacing(5);
    layoutByIndexGrid->setContentsMargins(5, 5, 5, 5);

    //create a box and set the above layout inside
    QGroupBox *layoutByIndexBox= new QGroupBox(tr("By Prominence Index"));
    layoutByIndexBox->setMinimumHeight(120);
    helpMessage = tr("<p><b>Visualize by prominence index</b/></p>"
                     "<p>Apply a prominence-based layout model to the network. </p>"
                     "<p>For instance, you can apply a Degree Centrality layout. </p>"
                     "<p>For each prominence index, you must select a layout type:</p>"
                     "<p>Radial, Levels, NodeSize or NodeColor.</p>"
                     "<p>Please note that node coloring works only for basic shapes "
                     "(box, circle, etc) not for image icons.</p>");
    layoutByIndexBox->setToolTip( helpMessage );
    layoutByIndexBox->setMaximumWidth(255);
    layoutByIndexBox->setLayout (layoutByIndexGrid );


    // create widgets for the "Force-Directed Models" Box
    QLabel *toolBoxLayoutForceDirectedSelectLabel = new QLabel;
    toolBoxLayoutForceDirectedSelectLabel->setText(tr("Model:"));
    toolBoxLayoutForceDirectedSelectLabel->setMinimumWidth(70);
    toolBoxLayoutForceDirectedSelect = new QComboBox;
    QStringList modelsList;
    modelsList << tr("None")
               << tr("Kamada-Kawai")
               << tr("Fruchterman-Reingold")
               << tr("Eades Spring Embedder")
                  ;

    toolBoxLayoutForceDirectedSelect->addItems(modelsList);
    toolBoxLayoutForceDirectedSelect->setMinimumHeight(20);
    toolBoxLayoutForceDirectedSelect->setMinimumWidth(100);
    toolBoxLayoutForceDirectedSelect->setStatusTip (
                tr("Select a Force-Directed layout model. "));
    helpMessage = tr("<p><b>Visualize by a Force-Directed Placement layout model.</b></p> "
                     "<p>Available models: </p>"

                     "<p><em>Kamada-Kawai</em></p>"
                     "<p>The best variant of the Spring Embedder family of models. "
                     "<p>In this the graph is considered to be a dynamic system where "
                     "every edge is between two actors is a 'spring' of a desirable "
                     "length, which corresponds to their graph theoretic distance. </p>"
                     "<p>In this way, the optimal layout of the graph \n"
                     "is the state with the minimum imbalance. The degree of "
                     "imbalance is formulated as the total spring energy: "
                     "the square summation of the differences between desirable "
                     "distances and real ones for all pairs of vertices.</p>"

                     "<p><em>Fruchterman-Reingold:</em></p>"
                     "<p>In this model, the vertices behave as atomic particles "
                     "or celestial bodies, exerting attractive and repulsive "
                     "forces to each other. Again, only vertices that are "
                     "neighbours  attract each other but, unlike Eades Spring "
                     "Embedder, all vertices repel each other.</p>"

                     "<p><em>Eades Spring Embedder:</em></p>"
                     "<p>A spring-gravitational model, where each node is "
                     "regarded as physical object (ring) repelling all other non-adjacent "
                     "nodes, while springs between connected nodes attract them.</p>"

                     );
    toolBoxLayoutForceDirectedSelect->setToolTip ( helpMessage );
    toolBoxLayoutForceDirectedSelect->setWhatsThis( helpMessage );

    toolBoxLayoutForceDirectedApplyButton = new QPushButton(tr("Apply"));
    toolBoxLayoutForceDirectedApplyButton->setObjectName ("toolBoxLayoutForceDirectedApplyButton");
    toolBoxLayoutForceDirectedApplyButton->setFocusPolicy(Qt::NoFocus);
    toolBoxLayoutForceDirectedApplyButton->setMinimumHeight(20);
    toolBoxLayoutForceDirectedApplyButton->setMaximumWidth(60);

    //create layout for dynamic visualisation
    QGridLayout *layoutForceDirectedGrid = new QGridLayout();
    layoutForceDirectedGrid->addWidget(toolBoxLayoutForceDirectedSelectLabel, 0,0);
    layoutForceDirectedGrid->addWidget(toolBoxLayoutForceDirectedSelect, 0,1);
    layoutForceDirectedGrid->addWidget(toolBoxLayoutForceDirectedApplyButton, 1,1);
    layoutForceDirectedGrid->setSpacing(5);
    layoutForceDirectedGrid->setContentsMargins(5, 5, 5, 5);

    //create a box for dynamic layout options
    QGroupBox *layoutDynamicBox= new QGroupBox(tr("By Force-Directed Model"));
    layoutDynamicBox->setMinimumHeight(90);
    layoutDynamicBox->setMaximumWidth(255);
    layoutDynamicBox->setLayout (layoutForceDirectedGrid );
    layoutDynamicBox->setContentsMargins(5, 5, 5, 5);


    //Parent box with vertical layout for all layout/visualization boxes
    QVBoxLayout *visualizationBoxLayout = new QVBoxLayout;
    visualizationBoxLayout->addWidget(layoutByIndexBox);
    visualizationBoxLayout->addWidget(layoutDynamicBox);
    visualizationBoxLayout->setContentsMargins(5,5,5,5);

    QGroupBox *visualizationBox= new QGroupBox(tr("Layout"));
    visualizationBox->setMaximumWidth(255);
    visualizationBox->setLayout (visualizationBoxLayout );
    visualizationBox->setContentsMargins(5,5,5,5);

    //Parent box with vertical layout for all boxes of Controls
    QGridLayout *controlGrid = new QGridLayout;
    controlGrid->addWidget(editGroupBox, 0,0);
    controlGrid->addWidget(analysisBox, 1, 0);
    controlGrid->addWidget(visualizationBox, 2, 0);
    controlGrid->setRowStretch(3,1);   //fix stretch
    controlGrid->setContentsMargins(5, 5, 5, 5);
    //create a box with title
    leftPanel = new QGroupBox(tr("Control Panel"));
    leftPanel->setMinimumWidth(240);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setLayout (controlGrid);


    //
    // Create widgets for Properties/Statistics group/tab
    //
    QLabel *rightPanelNetworkHeader = new QLabel;
    QFont labelFont = rightPanelNetworkHeader->font();
    labelFont.setWeight(QFont::Bold);
    rightPanelNetworkHeader->setText (tr("Network"));
    rightPanelNetworkHeader->setFont(labelFont);


    QLabel *rightPanelNetworkTypeLabel = new QLabel;
    rightPanelNetworkTypeLabel->setText ("Type:");
    rightPanelNetworkTypeLabel->setStatusTip(
                tr("The type of the network: directed or undirected. "
                   "Toggle the menu option Edit->Edges->Undirected Edges to change it"));

    rightPanelNetworkTypeLabel->setToolTip(
                tr("The loaded network, if any, is directed and \n"
                   "any link you add between nodes will be a directed arc.\n"
                   "If you want to work with undirected edges and/or \n"
                   "transform the loaded network (if any) to undirected \n"
                   "toggle the option Edit->Edges->Undirected \n"
                   "or press CTRL+E+U"));
    rightPanelNetworkTypeLabel->setWhatsThis(
                tr("The loaded network, if any, is directed and \n"
                   "any link you add between nodes will be a directed arc.\n"
                   "If you want to work with undirected edges and/or \n"
                   "transform the loaded network (if any) to undirected \n"
                   "toggle the option Edit->Edges->Undirected \n"
                   "or press CTRL+E+U"));


    rightPanelNetworkTypeLCD = new QLabel;
    rightPanelNetworkTypeLCD->setAlignment(Qt::AlignRight);
    rightPanelNetworkTypeLCD->setText (tr("Directed"));
    rightPanelNetworkTypeLCD->setStatusTip(
                tr("Directed data mode. "
                   "Toggle the menu option Edit->Edges->Undirected Edges to change it"));

    rightPanelNetworkTypeLCD->setToolTip(
                tr("The loaded network, if any, is directed and \n"
                   "any link you add between nodes will be a directed arc.\n"
                   "If you want to work with undirected edges and/or \n"
                   "transform the loaded network (if any) to undirected \n"
                   "toggle the option Edit->Edges->Undirected."));
    rightPanelNetworkTypeLCD->setWhatsThis(
                tr("The loaded network, if any, is directed and \n"
                   "any link you add between nodes will be a directed arc.\n"
                   "If you want to work with undirected edges and/or \n"
                   "transform the loaded network (if any) to undirected \n"
                   "toggle the option Edit->Edges->Undirected"));

    rightPanelNetworkTypeLCD->setMinimumWidth(75);


    QLabel *rightPanelNodesLabel = new QLabel;
    rightPanelNodesLabel->setText(tr("Nodes:"));
    rightPanelNodesLabel->setStatusTip(
                tr("Each actor in a social netwok is visualized as a node (aka vertex)."));
    rightPanelNodesLabel->setToolTip(
                tr("<p><b>Nodes</b></p>"
                   "<p>Each actor in a social netwok is visualized as a node (aka vertex) "
                   "in a graph. This is total number of actors "
                   "(aka nodes or vertices) in this social network.</p>"));
    rightPanelNodesLabel->setMinimumWidth(80);

    rightPanelNodesLCD=new QLabel;
    rightPanelNodesLCD->setAlignment(Qt::AlignRight);
    rightPanelNodesLCD->setStatusTip(
                tr("The total number of actors (aka nodes or vertices) in the social network."));
    rightPanelNodesLCD->setToolTip(
                tr("This is the total number of actors \n"
                   "(aka nodes or vertices) in the social network."));

    rightPanelEdgesLabel = new QLabel;
    rightPanelEdgesLabel->setText(tr("Arcs:"));
    rightPanelEdgesLabel->setStatusTip(tr("Each link between a pair of actors in a social network is visualized as an edge or arc."));
    rightPanelEdgesLabel->setToolTip(
                tr("<p><b>Edges</b></p>"
                   "Each link between a pair of actors in a social network is visualized as an undirected edge or a directed edge (aka arc)." )
                );

    rightPanelEdgesLCD=new QLabel;
    rightPanelEdgesLCD->setAlignment(Qt::AlignRight);
    rightPanelEdgesLCD->setStatusTip(tr("The total number of directed edges in the social network."));
    rightPanelEdgesLCD->setToolTip(tr("This is the total number of directed edges \n"
                                      "(links between actors) in the social network."));


    QLabel *rightPanelDensityLabel = new QLabel;
    rightPanelDensityLabel->setText(tr("Density:"));
    rightPanelDensityLabel->setStatusTip(tr("The density d is the ratio of existing edges to all possible edges"));
    helpMessage = tr("<p><b>Density</b></p>"
                     "<p>The density <em>d</em> of a social network is the ratio of "
                     "existing edges to all possible edges ( n*(n-1) ) between the "
                     "nodes of the network</p>.");
    rightPanelDensityLabel->setToolTip( helpMessage );
    rightPanelDensityLabel->setWhatsThis( helpMessage );

    rightPanelDensityLCD=new QLabel;
    rightPanelDensityLCD->setAlignment(Qt::AlignRight);
    rightPanelDensityLCD->setStatusTip(tr("The network density, the ratio of existing "
                                          "edges to all possible edges ( n*(n-1) ) between nodes."));
    rightPanelDensityLCD->setToolTip(
                tr("<p>This is the density of the network. "
                   "<p>The density of a network is the ratio of existing "
                   "edges to all possible edges ( n*(n-1) ) between nodes.</p>"));



    QLabel *verticalSpaceLabel1 = new QLabel;
    verticalSpaceLabel1->setText ("");
    QLabel *rightPanelSelectedHeaderLabel = new QLabel;
    rightPanelSelectedHeaderLabel->setText (tr("Selection"));
    rightPanelSelectedHeaderLabel->setFont(labelFont);

    QLabel *rightPanelSelectedNodesLabel = new QLabel;
    rightPanelSelectedNodesLabel->setText(tr("Nodes:"));
    rightPanelSelectedNodesLabel->setStatusTip(tr("Selected nodes."));
    rightPanelSelectedNodesLabel->setToolTip(tr("Selected nodes."));

    rightPanelSelectedNodesLCD=new QLabel;
    rightPanelSelectedNodesLCD->setAlignment(Qt::AlignRight);
    rightPanelSelectedNodesLCD->setText("0");
    rightPanelSelectedNodesLCD->setStatusTip(tr("The number of selected nodes (vertices)."));
    rightPanelSelectedNodesLCD->setToolTip(tr("The number of selected nodes (vertices)."));

    rightPanelSelectedEdgesLabel = new QLabel;
    rightPanelSelectedEdgesLabel->setText(tr("Arcs:"));
    rightPanelSelectedEdgesLabel->setStatusTip(tr("Selected edges."));
    rightPanelSelectedEdgesLabel->setToolTip(tr("Selected edges."));

    rightPanelSelectedEdgesLCD=new QLabel;
    rightPanelSelectedEdgesLCD->setText("0");
    rightPanelSelectedEdgesLCD->setAlignment(Qt::AlignRight);
    rightPanelSelectedEdgesLCD->setStatusTip(tr("The number of selected edges."));
    rightPanelSelectedEdgesLCD->setToolTip(tr("The number of selected edges."));


    QLabel *verticalSpaceLabel2 = new QLabel;
    verticalSpaceLabel2->setText ("");

    rightPanelClickedNodeHeaderLabel = new QLabel;
    rightPanelClickedNodeHeaderLabel->setText (tr("Clicked Node"));
    rightPanelClickedNodeHeaderLabel->setFont(labelFont);

    QLabel *rightPanelClickedNodeLabel = new QLabel;
    rightPanelClickedNodeLabel->setText (tr("Number:"));
    rightPanelClickedNodeLabel->setToolTip (tr("The node number of the last clicked node."));
    rightPanelClickedNodeLabel->setStatusTip( tr("The node number of the last clicked node. Zero means no node clicked."));
    rightPanelClickedNodeLCD = new QLabel;
    rightPanelClickedNodeLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedNodeLCD->setToolTip (tr("This is the node number of the last clicked node. \n"
                                               "Becomes zero when you click on something other than a node."));
    rightPanelClickedNodeLCD->setStatusTip( tr("The node number of the last clicked node. Zero if you clicked something else."));

    QLabel *rightPanelClickedNodeInDegreeLabel = new QLabel;
    rightPanelClickedNodeInDegreeLabel->setText (tr("In-Degree:"));
    rightPanelClickedNodeInDegreeLabel->setToolTip (tr("The inDegree of a node is the sum of all inbound edge weights."));
    rightPanelClickedNodeInDegreeLabel->setStatusTip (tr("The inDegree of a node is the sum of all inbound edge weights."));
    rightPanelClickedNodeInDegreeLCD = new QLabel;
    rightPanelClickedNodeInDegreeLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedNodeInDegreeLCD->setStatusTip (tr("The sum of all inbound edge weights of the last clicked node. "
                                                         "Zero if you clicked something else."));
    rightPanelClickedNodeInDegreeLCD->setToolTip (tr("This is the sum of all inbound edge weights of last clicked node. \n"
                                                       "Becomes zero when you click on something other than a node."));

    QLabel *rightPanelClickedNodeOutDegreeLabel = new QLabel;
    rightPanelClickedNodeOutDegreeLabel->setText (tr("Out-Degree:"));
    rightPanelClickedNodeOutDegreeLabel->setToolTip (tr("The outDegree of a node is the sum of all outbound edge weights."));
    rightPanelClickedNodeOutDegreeLabel->setStatusTip (tr("The outDegree of a node is the sum of all outbound edge weights."));
    rightPanelClickedNodeOutDegreeLCD=new QLabel;
    rightPanelClickedNodeOutDegreeLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedNodeOutDegreeLCD->setStatusTip (tr("The sum of all outbound edge weights of the last clicked node. "
                                                          "Zero if you clicked something else."));
    rightPanelClickedNodeOutDegreeLCD->setToolTip (tr("This is the sum of all outbound edge weights of the last clicked node. \n"
                                                        "Becomes zero when you click on something other than a node."));

    QLabel *verticalSpaceLabel3 = new QLabel;
    verticalSpaceLabel3->setText ("");

    QLabel * rightPanelClickedEdgeHeaderLabel = new QLabel;
    rightPanelClickedEdgeHeaderLabel->setText (tr("Clicked Edge"));
    rightPanelClickedEdgeHeaderLabel->setFont(labelFont);

    rightPanelClickedEdgeNameLabel = new QLabel;
    rightPanelClickedEdgeNameLabel->setText (tr("Name:"));
    rightPanelClickedEdgeNameLabel->setToolTip (tr("The name of the last clicked edge."));
    rightPanelClickedEdgeNameLabel->setStatusTip (tr("The name of the last clicked edge."));
    rightPanelClickedEdgeNameLCD = new QLabel;
    rightPanelClickedEdgeNameLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedEdgeNameLCD->setToolTip (tr("This is the name of the last clicked edge. \n"
                                                   "Becomes zero when you click on somethingto other than an edge"));
    rightPanelClickedEdgeNameLCD->setStatusTip (tr("The name of the last clicked edge."
                                                     "Zero when you click on something else."));


    rightPanelClickedEdgeWeightLabel = new QLabel;
    rightPanelClickedEdgeWeightLabel->setText (tr("Weight:"));
    rightPanelClickedEdgeWeightLabel->setStatusTip (tr("The weight of the clicked edge."));
    rightPanelClickedEdgeWeightLabel->setToolTip (tr("The weight of the clicked edge."));

    rightPanelClickedEdgeWeightLCD =new QLabel;
    rightPanelClickedEdgeWeightLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedEdgeWeightLCD->setToolTip (tr("This is the weight of the last clicked edge. \n"
                                                     "Becomes zero when you click on something other than an edge"));
    rightPanelClickedEdgeWeightLCD->setStatusTip (tr("The weight of the last clicked edge. "
                                                       "Zero when you click on something else."));


    rightPanelClickedEdgeReciprocalWeightLabel = new QLabel;
    rightPanelClickedEdgeReciprocalWeightLabel->setText (tr(""));
    rightPanelClickedEdgeReciprocalWeightLabel->setToolTip (tr("The weight of the reciprocal edge."));
    rightPanelClickedEdgeReciprocalWeightLabel->setStatusTip (tr("The weight of the reciprocal edge."));
    rightPanelClickedEdgeReciprocalWeightLCD =new QLabel;
    rightPanelClickedEdgeReciprocalWeightLCD->setAlignment(Qt::AlignRight);
    rightPanelClickedEdgeReciprocalWeightLCD->setToolTip (tr("This is the reciprocal weight of the last clicked reciprocated edge. \n"
                                                               "Becomes zero when you click on something other than an edge"));
    rightPanelClickedEdgeReciprocalWeightLCD->setStatusTip (tr("The reciprocal weight of the last clicked reciprocated edge. \n"
                                                                 "Becomes zero when you click on something other than an edge"));


    //create a grid layout
    QGridLayout *propertiesGrid = new QGridLayout();
    propertiesGrid->setColumnMinimumWidth(0, 10);
    propertiesGrid->setColumnMinimumWidth(1, 10);

    propertiesGrid->addWidget(rightPanelNetworkHeader , 0,0);
    propertiesGrid->addWidget(rightPanelNetworkTypeLabel , 1,0);
    propertiesGrid->addWidget(rightPanelNetworkTypeLCD , 1,1);
    propertiesGrid->addWidget(rightPanelNodesLabel, 2,0);
    propertiesGrid->addWidget(rightPanelNodesLCD,2,1);
    propertiesGrid->addWidget(rightPanelEdgesLabel, 3,0);
    propertiesGrid->addWidget(rightPanelEdgesLCD,3,1);
    propertiesGrid->addWidget(rightPanelDensityLabel, 4,0);
    propertiesGrid->addWidget(rightPanelDensityLCD,4,1);

    propertiesGrid->addWidget(verticalSpaceLabel1, 5,0);

    propertiesGrid->addWidget(rightPanelSelectedHeaderLabel, 6,0,1,2);
    propertiesGrid->addWidget(rightPanelSelectedNodesLabel , 7,0);
    propertiesGrid->addWidget(rightPanelSelectedNodesLCD ,7,1);
    propertiesGrid->addWidget(rightPanelSelectedEdgesLabel, 8,0);
    propertiesGrid->addWidget(rightPanelSelectedEdgesLCD, 8,1);

    propertiesGrid->addWidget(verticalSpaceLabel2, 9,0);
    propertiesGrid->addWidget(rightPanelClickedNodeHeaderLabel, 10,0,1,2);
    propertiesGrid->addWidget(rightPanelClickedNodeLabel , 11,0);
    propertiesGrid->addWidget(rightPanelClickedNodeLCD ,11,1);
    propertiesGrid->addWidget(rightPanelClickedNodeInDegreeLabel, 12,0);
    propertiesGrid->addWidget(rightPanelClickedNodeInDegreeLCD,12,1);
    propertiesGrid->addWidget(rightPanelClickedNodeOutDegreeLabel, 13,0);
    propertiesGrid->addWidget(rightPanelClickedNodeOutDegreeLCD,13,1);

    propertiesGrid->addWidget(verticalSpaceLabel3, 15,0);
    propertiesGrid->addWidget(rightPanelClickedEdgeHeaderLabel, 16,0,1,2);
    propertiesGrid->addWidget(rightPanelClickedEdgeNameLabel , 17,0);
    propertiesGrid->addWidget(rightPanelClickedEdgeNameLCD ,17,1);
    propertiesGrid->addWidget(rightPanelClickedEdgeWeightLabel , 18,0);
    propertiesGrid->addWidget(rightPanelClickedEdgeWeightLCD ,18,1);
    propertiesGrid->addWidget(rightPanelClickedEdgeReciprocalWeightLabel , 19,0);
    propertiesGrid->addWidget(rightPanelClickedEdgeReciprocalWeightLCD ,19,1);

    // Create our mini miniChart
    miniChart = new Chart(this);
    int chartHeight = 140;
    miniChart->setThemeSmallWidget(chartHeight,chartHeight);

    // Nothing else to do with miniChart.
    // MW::initApp() will populate it with a dummy point.

    propertiesGrid->addWidget(miniChart,20,0,1,2);
    propertiesGrid->setRowMinimumHeight(20, (int) floor( 1.5 * chartHeight ) );
    propertiesGrid->setRowStretch(20,0);

    // We need some margin form the edge of the miniChart to the messageLabel below,
    // but setRowStretch is not enough. So, we add a spacer!
    QSpacerItem *spacer = new QSpacerItem (100, 10,
                                           QSizePolicy::MinimumExpanding,
                                           QSizePolicy::MinimumExpanding);
    propertiesGrid->addItem(spacer, 22,0,3,2);
    propertiesGrid->setRowStretch(22,1);   //allow this row to stretch

    // Add the message label, this will be displayed in the down-right corner.
    QLabel *rightPanelMessageLabel = new QLabel;
    rightPanelMessageLabel->setText ("https://socnetv.org");
    propertiesGrid->addWidget(rightPanelMessageLabel, 25, 0, 1, 2);
    propertiesGrid->setRowStretch(25,0);   // stop row from stretching

    // Create a panel with title
    rightPanel = new QGroupBox(tr("Statistics Panel"));
    rightPanel->setMaximumWidth(190);
    rightPanel->setObjectName("rightPanel");
    rightPanel->setLayout (propertiesGrid);


    qDebug()<< "Finished panels init.";

}




/**
 * @brief Initializes the application window layout
 *
 * Creates helper widgets and sets the main layout of the MainWindow
 */
void MainWindow::initWindowLayout() {

    qDebug() << "Initializing window layout...";

    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);
    iconSize.setHeight(16);
    iconSize.setWidth(16);

    //
    // Zoom slider
    //
    zoomInBtn = new QToolButton;
    zoomInBtn->setToolTip(tr("Zoom in the network."));
    zoomInBtn->setStatusTip(tr("Zoom in the network. Or press Cltr and use mouse wheel."));
    zoomInBtn->setWhatsThis(tr("Zoom In.\n\n"
                               "Zooms in the network (Ctrl++)."
                               "You can also press Cltr and use the mouse wheel."));
    zoomInBtn->setAutoRepeat(true);
    zoomInBtn->setAutoRepeatInterval(33);
    zoomInBtn->setAutoRepeatDelay(0);
    zoomInBtn->setIcon(QPixmap(":/images/zoom_in_24px.svg"));
    zoomInBtn->setIconSize(iconSize);

    zoomOutBtn = new QToolButton;
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setToolTip(tr("Zoom out."));
    zoomOutBtn->setStatusTip(tr("Zoom out of the actual network. Or press Cltr and use mouse wheel."));
    zoomOutBtn->setWhatsThis(tr("Zoom out.\n\n"
                                "Zooms out of the actual network. (Ctrl+-)"
                                "You can also press Cltr and use the mouse wheel."));
    zoomOutBtn->setAutoRepeat(true);
    zoomOutBtn->setAutoRepeatInterval(33);
    zoomOutBtn->setAutoRepeatDelay(0);
    zoomOutBtn->setIcon(QPixmap(":/images/zoom_out_24px.svg"));
    zoomOutBtn->setIconSize(iconSize);

    zoomSlider = new QSlider;
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(maxZoomIndex);
    zoomSlider->setValue((int)maxZoomIndex/2.0);
    zoomSlider->setToolTip(tr("Zoom slider: Drag up to zoom in. \n"
                              "Drag down to zoom out. "));
    zoomSlider->setWhatsThis(tr("Zoom slider: Drag up to zoom in. \n"
                                "Drag down to zoom out. "));
    zoomSlider->setTickPosition(QSlider::TicksBothSides);

    // Zoom slider layout
    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
    zoomSliderLayout->addWidget(zoomInBtn);
    zoomSliderLayout->addWidget(zoomSlider);
    zoomSliderLayout->addWidget(zoomOutBtn);
    //
    // Rotate slider
    //
    rotateLeftBtn = new QToolButton;
    rotateLeftBtn->setAutoRepeat(true);
    rotateLeftBtn->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::Key_Left);
    rotateLeftBtn->setIcon(QPixmap(":/images/rotate_left_48px.svg"));
    rotateLeftBtn->setToolTip(tr("Rotates the canvas counterclockwise"));
    rotateLeftBtn->setStatusTip(tr("Rotate counterclockwise"));
    rotateLeftBtn->setWhatsThis(tr("Rotates the canvas counterclockwise."));
    rotateLeftBtn->setIconSize(iconSize);

    rotateRightBtn = new QToolButton;
    rotateRightBtn->setAutoRepeat(true);
    rotateRightBtn->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::Key_Right);
    rotateRightBtn->setIcon(QPixmap(":/images/rotate_right_48px.svg"));
    rotateRightBtn->setToolTip(tr("Rotates the canvas clockwise."));
    rotateRightBtn->setStatusTip(tr("Rotate clockwise"));
    rotateRightBtn->setWhatsThis(tr("Rotates the canvas clockwise."));
    rotateRightBtn->setIconSize(iconSize);

    rotateSlider = new QSlider;
    rotateSlider->setOrientation(Qt::Horizontal);
    rotateSlider->setMinimum(-180);
    rotateSlider->setMaximum(180);
    rotateSlider->setTickInterval(5);
    rotateSlider->setValue(0);
    rotateSlider->setToolTip(tr("Rotate slider: Drag to left to rotate clockwise. \n"
                                "Drag to right to rotate counterclockwise. "));
    rotateSlider->setWhatsThis(tr("Rotate slider: Drag to left to rotate clockwise. "
                                  "Drag to right to rotate counterclockwise. "));
    rotateSlider->setTickPosition(QSlider::TicksBothSides);

    // Rotate slider layout
    QHBoxLayout *rotateSliderLayout = new QHBoxLayout;
    rotateSliderLayout->addWidget(rotateLeftBtn);
    rotateSliderLayout->addWidget(rotateSlider);
    rotateSliderLayout->addWidget(rotateRightBtn );

    resetSlidersBtn = new QToolButton;
    resetSlidersBtn->setText(tr("Reset"));
    resetSlidersBtn->setShortcut(Qt::CTRL | Qt::Key_0);
    resetSlidersBtn->setStatusTip(tr("Reset zoom and rotation to zero (or press Ctrl+0)"));
    resetSlidersBtn->setToolTip(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setWhatsThis(tr("Reset zoom and rotation to zero (Ctrl+0)"));
    resetSlidersBtn->setIcon(QPixmap(":/images/refresh_48px.svg"));
    resetSlidersBtn->setIconSize(iconSize);
    resetSlidersBtn->setEnabled(true);

    // Create a layout for the toolbox and the canvas.
    // This will be the layout of our MW central widget
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(leftPanel, 0, 0, 2,1);
    layout->addWidget(graphicsWidget,0,1);
    layout->addLayout(zoomSliderLayout, 0, 2);
    layout->addWidget(rightPanel, 0, 3,2,1);
    layout->addLayout(rotateSliderLayout, 1, 1, 1, 1);
    layout->addWidget(resetSlidersBtn, 1, 2, 1, 1);

    //create a dummy widget, and set the above layout
    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    //now set this as central widget of MW
    setCentralWidget(widget);

    // set panels visibility
    if ( appSettings["showRightPanel"] == "false") {
        slotOptionsWindowRightPanelVisibility(false);
    }

    if ( appSettings["showLeftPanel"] == "false") {
        slotOptionsWindowLeftPanelVisibility(false);
    }


    //
    // Load our default stylesheet, if set in the app settings.
    //
    if (appSettings["useCustomStyleSheet"] == "true") {
        slotStyleSheetByName(":/qss/default.qss");
    }


    qDebug() << "Finished window layout init.";

}





/**
 * @brief Connects signals & slots between various parts of the app
 *
 * Signal/slots between:
 * - the GraphicsWidget and the Graph
 * - the GraphicsWidget and the MainWindow
 * This must be called after all widgets have been created.
 *
 */
void MainWindow::initSignalSlots() {
    qDebug()<< "setting up signals/slots between widgets (graphicsWidget, activeGraph and MW)...";

    // Signals between graphicsWidget and MainWindow

    connect( graphicsWidget, &GraphicsWidget::setCursor,
             this,&MainWindow::setCursor);


    connect( graphicsWidget, &GraphicsWidget::userMiddleClicked,
            this,&MainWindow::slotEditEdgeCreate);

    connect( graphicsWidget, SIGNAL( openNodeMenu() ),
             this, SLOT( slotEditNodeOpenContextMenu() ) ) ;

    connect (graphicsWidget, &GraphicsWidget::openContextMenu,
             this, &MainWindow::slotEditOpenContextMenu);

    connect( graphicsWidget, SIGNAL(userNodeMoved(const int &, const int &, const int &)),
             this, SLOT( slotEditNodePosition(const int &, const int &, const int &) ) );

    connect( graphicsWidget, SIGNAL(zoomChanged(const int &)),
             zoomSlider, SLOT( setValue(const int &)) );

    connect(zoomSlider, SIGNAL(valueChanged(const int &)),
            graphicsWidget, SLOT(changeMatrixScale(const int &)));

    connect( zoomInBtn, SIGNAL(clicked()), graphicsWidget, SLOT( zoomIn() ) );
    connect( zoomOutBtn, SIGNAL(clicked()), graphicsWidget, SLOT( zoomOut() ) );

    connect( graphicsWidget, SIGNAL(rotationChanged(const int &)),
             rotateSlider, SLOT( setValue(const int &)) );

    connect(rotateSlider, SIGNAL(valueChanged(const int &)),
            graphicsWidget, SLOT(changeMatrixRotation(const int &)));

    connect(rotateLeftBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateLeft()));
    connect(rotateRightBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateRight()));

    connect(resetSlidersBtn, SIGNAL(clicked()), graphicsWidget, SLOT(reset()));


    //
    //SIGNALS BETWEEN ACTIVEGRAPH AND MAINWINDOW
    //
    connect( activeGraph, &Graph::signalSelectionChanged,
             this, &MainWindow::slotEditSelectionChanged);


    connect( activeGraph, &Graph::signalNodeClickedInfo ,
             this, &MainWindow::slotEditNodeInfoStatusBar );

    connect ( activeGraph, &Graph::signalEdgeClicked,
              this, &MainWindow::slotEditEdgeClicked );

    connect (activeGraph, &Graph::signalGraphModified,
             this, &MainWindow::slotNetworkChanged);

    connect (activeGraph, &Graph::signalGraphLoaded,
             this, &MainWindow::slotNetworkFileLoaded);

    connect( activeGraph, &Graph::signalGraphSavedStatus,
             this, &MainWindow::slotNetworkSavedStatus);

    connect( activeGraph, SIGNAL( statusMessage (QString) ),
             this, SLOT( statusMessage (QString) ) ) ;

    connect( activeGraph, SIGNAL( signalDatasetDescription (QString) ),
             this, SLOT( slotHelpMessageToUserInfo (QString) ) ) ;


    connect( editRelationNextAct, &QAction::triggered,
             activeGraph, &Graph::relationNext );

    connect( editRelationPreviousAct, &QAction::triggered,
             activeGraph, &Graph::relationPrev );

    connect( editRelationChangeCombo , SIGNAL( activated(int) ) ,
             activeGraph, SLOT( relationSet(int) ) );

    connect( editRelationChangeCombo , SIGNAL( currentTextChanged(const QString&) ),
             activeGraph, SLOT( relationCurrentRename(const QString &) )  );

//    connect( editRelationChangeCombo, &QComboBox::currentTextChanged,
//             activeGraph, QOverload<const QString &>::of(&Graph::relationCurrentRename));

    connect( this , &MainWindow::signalRelationAddAndChange,
             activeGraph, &Graph::relationAdd );

    connect ( activeGraph, &Graph::signalRelationChangedToMW,
              this, &MainWindow::slotEditRelationChange );

    connect ( activeGraph, &Graph::signalRelationsClear,
              this, &MainWindow::slotEditRelationsClear );

    connect ( activeGraph, &Graph::signalRelationAddToMW,
              this, &MainWindow::slotEditRelationAdd  );

    connect ( activeGraph, &Graph::signalRelationRenamedToMW,
              editRelationChangeCombo, &QComboBox::setCurrentText );

    connect ( activeGraph, &Graph::signalProgressBoxCreate,
              this, &MainWindow::slotProgressBoxCreate);

    connect ( activeGraph, &Graph::signalProgressBoxKill,
              this, &MainWindow::slotProgressBoxDestroy);


    connect ( activeGraph, &Graph::signalPromininenceDistributionChartUpdate,
              this, &MainWindow::slotAnalyzeProminenceDistributionChartUpdate);

    connect ( activeGraph, &Graph::signalNetworkManagerRequest,
              this, &MainWindow::slotNetworkManagerRequest);



    //
    // Signals between activeGraph and graphicsWidget
    //

    connect( activeGraph, &Graph::addGuideCircle,
             graphicsWidget, &GraphicsWidget::addGuideCircle ) ;

    connect( activeGraph, &Graph::addGuideHLine,
             graphicsWidget, &GraphicsWidget::addGuideHLine) ;

    connect( activeGraph, &Graph::setNodePos,
             graphicsWidget, &GraphicsWidget::moveNode) ;

    connect( activeGraph, &Graph::signalNodesFound,
             graphicsWidget,  &GraphicsWidget::setSelectedNodes  );

    connect( activeGraph, &Graph::signalDrawNode,
             graphicsWidget, &GraphicsWidget::drawNode) ;

    connect( activeGraph, &Graph::signalRemoveNode,
             graphicsWidget, &GraphicsWidget::removeNode  );

    connect( activeGraph, &Graph::setVertexVisibility,
             graphicsWidget, &GraphicsWidget::setNodeVisibility);

    connect( activeGraph, &Graph::setNodeSize,
             graphicsWidget, &GraphicsWidget::setNodeSize);

    connect( activeGraph, &Graph::setNodeColor,
             graphicsWidget, &GraphicsWidget::setNodeColor );

    connect( activeGraph, &Graph::setNodeShape,
             graphicsWidget, &GraphicsWidget::setNodeShape);

    connect( activeGraph, &Graph::setNodeNumberColor,
              graphicsWidget, &GraphicsWidget::setNodeNumberColor);

    connect( activeGraph, &Graph::setNodeNumberSize,
             graphicsWidget, &GraphicsWidget::setNodeNumberSize);

    connect( activeGraph, &Graph::setNodeNumberDistance,
             graphicsWidget, &GraphicsWidget::setNodeNumberDistance);

    connect( activeGraph, &Graph::setNodeLabel ,
             graphicsWidget, &GraphicsWidget::setNodeLabel );

    connect( activeGraph,&Graph::setNodeLabelColor,
             graphicsWidget,  &GraphicsWidget::setNodeLabelColor );

    connect( activeGraph, &Graph::setNodeLabelSize,
             graphicsWidget, &GraphicsWidget::setNodeLabelSize );

    connect( activeGraph, &Graph::setNodeLabelDistance,
             graphicsWidget, &GraphicsWidget::setNodeLabelDistance);

    connect( activeGraph, &Graph::signalRemoveEdge,
             graphicsWidget,&GraphicsWidget::removeEdge);

    connect (activeGraph, &Graph::signalDrawEdge,
             graphicsWidget,&GraphicsWidget::drawEdge);

    connect( activeGraph, &Graph::setEdgeWeight,
             graphicsWidget, &GraphicsWidget::setEdgeWeight);

    connect( activeGraph, &Graph::signalEdgeType,
             graphicsWidget, &GraphicsWidget::setEdgeDirectionType );

    connect( activeGraph, &Graph::setEdgeColor,
             graphicsWidget, &GraphicsWidget::setEdgeColor);

    connect( activeGraph, &Graph::setEdgeLabel,
             graphicsWidget, &GraphicsWidget::setEdgeLabel );

    connect( activeGraph, &Graph::signalSetEdgeVisibility,
             graphicsWidget, &GraphicsWidget::setEdgeVisibility);

    connect( activeGraph, &Graph::signalRelationChangedToGW,
             graphicsWidget, &GraphicsWidget::relationSet) ;

    connect( graphicsWidget,  &GraphicsWidget::userClickOnEmptySpace,
             activeGraph, &Graph::graphClickedEmptySpace ) ;

    connect( graphicsWidget, &GraphicsWidget::resized,
             activeGraph, &Graph::canvasSizeSet) ;

    connect( graphicsWidget, &GraphicsWidget::userDoubleClickNewNode,
             activeGraph, &Graph::vertexCreateAtPos) ;

    connect( graphicsWidget, &GraphicsWidget::userSelectedItems,
             activeGraph,&Graph::setSelectionChanged);

    connect( graphicsWidget, &GraphicsWidget::userClickedNode,
             activeGraph, &Graph::vertexClickedSet );

    connect( graphicsWidget, &GraphicsWidget::userClickedEdge,
             activeGraph, &Graph::edgeClickedSet );


    //
    // Signals and slots inside MainWindow
    //

#ifndef QT_NO_SSL
    connect( networkManager, &QNetworkAccessManager::sslErrors,
            this, &MainWindow::slotNetworkManagerSslErrors);
#endif

    connect( editMouseModeInteractiveAct, &QAction::triggered,
             this, &MainWindow::slotEditDragModeSelection );

    connect( editMouseModeScrollAct, &QAction::triggered,
             this, &MainWindow::slotEditDragModeScroll );


    connect( editRelationAddAct, SIGNAL(triggered()),
             this, SLOT(slotEditRelationAddPrompt()) );

    connect( editRelationRenameAct,SIGNAL(triggered()),
             this, SLOT(slotEditRelationRename()) ) ;

    connect(zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomIn()) );
    connect(zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT( zoomOut()) );
    connect(editRotateLeftAct, SIGNAL(triggered()), graphicsWidget, SLOT( rotateLeft()) );
    connect(editRotateRightAct, SIGNAL(triggered()), graphicsWidget, SLOT( rotateRight()) );
    connect(editResetSlidersAct, SIGNAL(triggered()), graphicsWidget, SLOT( reset()) );

    connect( layoutGuidesAct, SIGNAL(triggered(bool)),
             this, SLOT(slotLayoutGuides(bool)));


    connect(toolBoxNetworkAutoCreateSelect, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::toolBoxNetworkAutoCreateSelectChanged);

    connect(toolBoxEditNodeSubgraphSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxEditNodeSubgraphSelectChanged(int) ) );


    connect(toolBoxEditEdgeModeSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(slotEditEdgeMode(int) ) );

    connect(toolBoxEditEdgeTransformSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxEditEdgeTransformSelectChanged(int) ) );

    connect(toolBoxAnalysisMatricesSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisMatricesSelectChanged(int) ) );

    connect(toolBoxAnalysisCohesionSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisCohesionSelectChanged(int) ) );

    connect(toolBoxAnalysisStrEquivalenceSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisStrEquivalenceSelectChanged(int) ) );

    connect(toolBoxAnalysisCommunitiesSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisCommunitiesSelectChanged(int) ) );

    connect(toolBoxAnalysisProminenceSelect, SIGNAL (currentIndexChanged(int) ),
            this, SLOT(toolBoxAnalysisProminenceSelectChanged(int) ) );


    connect(toolBoxLayoutByIndexApplyButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutByIndexApplyBtnPressed() ) );

    connect(toolBoxLayoutForceDirectedApplyButton, SIGNAL (clicked() ),
            this, SLOT(toolBoxLayoutForceDirectedApplyBtnPressed() ) );

}






/**
 * @brief Initializes the default app parameters.
 *
 * Used on app start and when erasing a network to start a new one
 */
void MainWindow::initApp(){

    qDebug()<<"### Application initialization starts, on thread" << thread();

    statusMessage( tr("Application initialization. Please wait..."));

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // first select none
    graphicsWidget->selectNone();

    // Init basic variables
    inverseWeights=false;
    askedAboutWeights=false;

    previous_fileName=fileName;
    fileName="";

    initTextCodecName= "UTF-8";

    networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
    networkSaveAct->setEnabled(false);

    /** Clear previous network data and reset user-selected settings */
    qDebug()<<"### Clearing current graph. Please wait...";
    activeGraph->clear();

    activeGraph->vertexShapeSetDefault(appSettings["initNodeShape"], appSettings["initNodeIconPath"]);
    activeGraph->vertexSizeInit(appSettings["initNodeSize"].toInt(0, 10));
    activeGraph->vertexColorInit( appSettings["initNodeColor"] );

    activeGraph->vertexNumberSizeInit(appSettings["initNodeNumberSize"].toInt(0,10));
    activeGraph->vertexNumberColorInit(appSettings["initNodeNumberColor"]);
    activeGraph->vertexNumberDistanceInit(appSettings["initNodeNumberDistance"].toInt(0,10));

    activeGraph->vertexLabelColorInit(appSettings["initNodeLabelColor"]);
    activeGraph->vertexLabelSizeInit(appSettings["initNodeLabelSize"].toInt(0,10));
    activeGraph->vertexLabelDistanceInit(appSettings["initNodeLabelDistance"].toInt(0,10));

    activeGraph->edgeColorInit(appSettings["initEdgeColor"]);

    activeGraph->edgeWeightNumbersVisibilitySet(
                (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true:false
                                                                             );
    activeGraph->setReportsRealNumberPrecision(appSettings["initReportsRealNumberPrecision"].toInt());

    activeGraph->setReportsLabelLength(appSettings["initReportsLabelsLength"].toInt());
    activeGraph->setReportsChartType(appSettings["initReportsChartType"].toInt());

    emit signalSetReportsDataDir(appSettings["dataDir"]);

    /** Clear graphicsWidget and reset settings and transformations **/
    qDebug()<<"### Clearing graphicsWidget and resetting transformations. Please wait...";
    graphicsWidget->clear();
    rotateSlider->setValue(0);
    zoomSlider->setValue((int) maxZoomIndex/2.0);
//    graphicsWidget->setInitZoomIndex((int) maxZoomIndex/2.0);
    graphicsWidget->setMaxZoomIndex(maxZoomIndex);

    graphicsWidget->setInitNodeSize(appSettings["initNodeSize"].toInt(0, 10));
    graphicsWidget->setNodeNumberVisibility(
                ( appSettings["initNodeNumbersVisibility"] == "true" ) ? true: false
                                                                         );
    graphicsWidget->setNodeLabelsVisibility(
                (appSettings["initNodeLabelsVisibility"] == "true" ) ? true: false
                                                                       );

    graphicsWidget->setNumbersInsideNodes(
                ( appSettings["initNodeNumbersInside"] == "true" ) ? true: false
                                                                     );
    graphicsWidget->setEdgeHighlighting(
                ( appSettings["canvasEdgeHighlighting"] == "true" ) ? true: false
                                                                      );

    if (appSettings["initBackgroundImage"] != ""
            && QFileInfo::exists(appSettings["initBackgroundImage"])) {
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage( tr("BackgroundImage on.") );
    }
    else {
        graphicsWidget->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"]))
                );
    }

    slotOptionsCanvasIndexMethod (appSettings["canvasIndexMethod"]) ;

    /** Clear Chart */
    miniChart->resetToTrivial();

    /** Clear LCDs **/
    qDebug()<<"### Clearing Statistics panel LCDs. Please wait...";

    rightPanelClickedNodeInDegreeLCD->setText("-");
    rightPanelClickedNodeOutDegreeLCD->setText("-");
    rightPanelClickedNodeLCD->setText("-");
    rightPanelClickedEdgeNameLCD->setText("-");
    rightPanelClickedEdgeWeightLCD->setText("-");
    rightPanelClickedEdgeReciprocalWeightLCD->setText("");


    /** Clear toolbox and menu checkboxes **/
    qDebug()<<"### Resetting toolbox. Please wait...";
    toolBoxEditEdgeTransformSelect->setCurrentIndex(0);
    toolBoxEditEdgeModeSelect->setCurrentIndex(0);

    initComboBoxes();

    toolBoxLayoutByIndexSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);

    optionsEdgeWeightNumbersAct->setChecked(
                (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true:false
                                                                             );
    optionsEdgeWeightConsiderAct->setChecked( false ) ;

    optionsEdgeArrowsAct->setChecked(
                (appSettings["initEdgeArrows"] == "true") ? true: false
                                                            );

    optionsEdgeLabelsAct->setChecked (
                (appSettings["initEdgeLabelsVisibility"] == "true") ? true: false
                                                                      );
    editFilterNodesIsolatesAct->setChecked(false); // re-init orphan nodes menu item

    editFilterEdgesUnilateralAct->setChecked(false);

    //editRelationChangeCombo->clear();


    qDebug()<<"### Clearing textEditors. Current count: " <<m_textEditors.size() << "textEditors";
    foreach ( TextEditor *ed, m_textEditors) {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();

    QApplication::restoreOverrideCursor();
    // Do it again, to catch any older overriden cursor
    QApplication::restoreOverrideCursor();

    setCursor(Qt::ArrowCursor);

    setWindowTitle("SocNetV");

    statusMessage( tr("Ready"));

    qDebug()<< "#### APP INITIALISATION FINISHED, ON THREAD" << thread();


}

/**
 * @brief Initializes combo boxes in the MW
 */
void MainWindow::initComboBoxes() {
    toolBoxAnalysisCommunitiesSelect->setCurrentIndex(0);
    toolBoxAnalysisStrEquivalenceSelect->setCurrentIndex(0);
    toolBoxAnalysisCohesionSelect->setCurrentIndex(0);
    toolBoxAnalysisProminenceSelect->setCurrentIndex(0);
    toolBoxAnalysisMatricesSelect->setCurrentIndex(0);
    toolBoxNetworkAutoCreateSelect->setCurrentIndex(0);
    toolBoxEditNodeSubgraphSelect->setCurrentIndex(0);
}



/**
 * @brief Updates the Recent Files QActions in the menu
 */
void MainWindow::slotNetworkFileRecentUpdateActions() {

    int numRecentFiles = qMin(recentFiles.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1  %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(recentFiles[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    //separatorAct->setVisible(numRecentFiles > 0);
}



/**
 * @brief  Shows a message in the status bar, with the given duration
 *
 * Called by Graph::statusMessage to display some message to the user
 *
 * @param message
 */
void MainWindow::statusMessage(const QString message){
    statusBar()->showMessage( message, appSettings["initStatusBarDuration"].toInt(0));
}



/**
 * @brief Helper function to display a popup with useful info
 * @param text
 */
void MainWindow::slotHelpMessageToUserInfo(const QString text) {
    slotHelpMessageToUser(USER_MSG_INFO,tr("Useful information"), text  );
}


/**
 * @brief Helper function to display a popup with an error message
 * @param text
 */
void MainWindow::slotHelpMessageToUserError(const QString text) {
    slotHelpMessageToUser(USER_MSG_CRITICAL ,tr("Error"), text  );
}


/**
 * @brief Displays a popup with the given text/info and a status message
 *
 * @param type
 * @param statusMsg
 * @param text
 * @param info
 * @param buttons
 * @param defBtn
 * @param btn1
 * @param btn2
 * @return
 */
int MainWindow::slotHelpMessageToUser(const int type,
                                      const QString statusMsg,
                                      const QString text,
                                      const QString info,
                                      QMessageBox::StandardButtons buttons,
                                      QMessageBox::StandardButton defBtn,
                                      const QString btn1,
                                      const QString btn2
                                      ) {
    int response=0;
    QMessageBox msgBox;
    msgBox.setMinimumWidth(400);
    QPushButton *pbtn1, *pbtn2;

    switch (type) {
    case USER_MSG_INFO:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setWindowTitle("Information");
        msgBox.setText(text);
        if (!info.isNull()) msgBox.setInformativeText(info);
        msgBox.setIcon(QMessageBox::Information);
        if (buttons==QMessageBox::NoButton) {
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
        }
        else {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }
        msgBox.setDefaultButton(defBtn);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setWindowTitle("Error");
        msgBox.setText(text);
        if (!info.isNull()) msgBox.setInformativeText(info);
        //msgBox.setTextFormat(Qt::RichText);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_NETWORK:
        statusMessage(  tr("Nothing to do! Load or create a social network first")  );
        msgBox.setWindowTitle("Error");
        msgBox.setText(
                    tr("No network! \n"
                       "Load social network data or create a new social network first. \n")
                    );
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_EDGES:
        statusMessage(  tr("Nothing to do! Load social network data or create edges first")  );
        msgBox.setWindowTitle("Error");
        msgBox.setText(
                    tr("No edges! \n"
                       "Load social network data or create some edges first. \n")
                    );
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION:
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setWindowTitle("Question");
        msgBox.setText( text );
        if (!info.isNull()) msgBox.setInformativeText(info);
        if (buttons==QMessageBox::NoButton) {
            msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
        }
        else {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }

        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION_CUSTOM: // a custom question with just two buttons
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setWindowTitle("Question");
        msgBox.setText( text );
        if (!info.isNull()) msgBox.setInformativeText(info);
        pbtn1 = msgBox.addButton(btn1, QMessageBox::ActionRole);
        pbtn2 = msgBox.addButton(btn2, QMessageBox::ActionRole);
        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();
        if (msgBox.clickedButton() == pbtn1 ) {
            response=1;
        }
        else if (msgBox.clickedButton() == pbtn2 ) {
            response=2;
        }
        break;
    default: //just for sanity
        if (!statusMsg.isNull()) statusMessage(  statusMsg  );
        msgBox.setText( text );
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();
        break;
    }
    return response;

}







/**
 * @brief Called when user selects something in the Network Auto Create
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxNetworkAutoCreateSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected net auto create, index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1: // famous data-sets
        slotNetworkDataSetSelect();
        break;
    case 2: // scale-free
        slotNetworkRandomScaleFreeDialog();
        break;
    case 3: // sw
        slotNetworkRandomSmallWorldDialog();
        break;
    case 4: // erdos
        slotNetworkRandomErdosRenyiDialog();
        break;
    case 5: // lattice
        slotNetworkRandomLatticeDialog();
        break;
    case 6: // d-regular
        slotNetworkRandomRegularDialog();
        break;
    case 7: // ring lattice
        slotNetworkRandomRingLattice();
        break;
    case 8: // web crawler
        slotNetworkWebCrawlerDialog();
        break;

    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();
}


/**
 * @brief Called when user selects something in the Subgraph from Selected
 * Nodes selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditNodeSubgraphSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected subgraph creation, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotEditNodeSelectedToClique();
        break;
    case 2:
        slotEditNodeSelectedToStar();
        break;
    case 3:
        slotEditNodeSelectedToCycle();
        break;
    case 4:
        slotEditNodeSelectedToLine();
        break;
    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();
}





/**
 * @brief Called when user selects something in the Edge Transform
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditEdgeTransformSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected edge transform, index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotEditEdgeSymmetrizeAll();
        break;
    case 2:
        slotEditEdgeSymmetrizeStrongTies();
        break;
    case 3:
        slotEditEdgeSymmetrizeCocitation();
        break;
    case 4:
        slotEditEdgeDichotomizationDialog();
        break;

    };
}




/**
 * @brief Called when user selects something in the Matrices
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisMatricesSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected matrix analysis, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotNetworkViewSociomatrix();
        break;
    case 2:
        slotNetworkViewSociomatrixPlotText();
        break;
    case 3:
        slotAnalyzeMatrixAdjacencyInverse();
        break;
    case 4:
        slotAnalyzeMatrixAdjacencyTranspose();
        break;
    case 5:
        slotAnalyzeMatrixAdjacencyCocitation();
        break;
    case 6:
        slotAnalyzeMatrixDegree();
        break;
    case 7:
        slotAnalyzeMatrixLaplacian();
        break;
    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();

}




/**
 * @brief Called when user selects something in the Cohesion
 * selectbox of the toolbox to compute basic graph theoretic / network properties
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisCohesionSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected cohesion analysis, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeReciprocity();
        break;
    case 2:
        slotAnalyzeSymmetryCheck();
        break;
    case 3:
        slotAnalyzeDistance();
        break;
    case 4:
        slotAnalyzeDistanceAverage();
        break;
    case 5:
        slotAnalyzeMatrixDistances();
        break;
    case 6:
        slotAnalyzeMatrixGeodesics();
        break;
    case 7:
        slotAnalyzeEccentricity();
        break;
    case 8:
        slotAnalyzeDiameter();
        break;
    case 9:
        slotAnalyzeConnectedness();
        break;
    case 10:
        slotAnalyzeWalksLength();
        break;
    case 11:
        slotAnalyzeWalksTotal();
        break;
    case 12:
        slotAnalyzeReachabilityMatrix();
        break;
    case 13:
        slotAnalyzeClusteringCoefficient();
        break;
    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();
}






/**
 * @brief Called when the user selects something in the Communities selectbox
 * of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisCommunitiesSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected community analysis, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeCommunitiesCliqueCensus();
        break;
    case 2:
        slotAnalyzeCommunitiesTriadCensus();
        break;
    };
    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();

}





/**
 * @brief Called when the user selects something in the Structural Equivalence
 * selectbox of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisStrEquivalenceSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected struct. equivalence analysis, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeStrEquivalencePearsonDialog();
        break;
    case 2:
        slotAnalyzeStrEquivalenceSimilarityMeasureDialog();
        break;
    case 3:
        slotAnalyzeStrEquivalenceDissimilaritiesDialog();
        break;
    case 4:
        slotAnalyzeStrEquivalenceClusteringHierarchicalDialog();
        break;
    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();
}





/**
 * @brief Called when user selects something in the Prominence selectbox
 *  of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisProminenceSelectChanged(const int &selectedIndex) {
    qDebug()<< "selected prominence analysis, text index: " << selectedIndex;
    switch(selectedIndex){
    case 0:
        break;
    case 1:
        slotAnalyzeCentralityDegree();
        break;
    case 2:
        slotAnalyzeCentralityCloseness();
        break;
    case 3:
        slotAnalyzeCentralityClosenessIR();
        break;
    case 4:
        slotAnalyzeCentralityBetweenness();
        break;
    case 5:
        slotAnalyzeCentralityStress();
        break;
    case 6:
        slotAnalyzeCentralityEccentricity();
        break;
    case 7:
        slotAnalyzeCentralityPower();
        break;
    case 8:
        slotAnalyzeCentralityInformation();
        break;
    case 9:
        slotAnalyzeCentralityEigenvector();
        break;
    case 10:
        slotAnalyzePrestigeDegree();
        break;
    case 11:
        slotAnalyzePrestigePageRank();
        break;
    case 12:
        slotAnalyzePrestigeProximity();
        break;
    };

    qDebug()<< "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when the user selects a Prominence index in the Layout selectbox
 *  of the Control Panel.
 */
void MainWindow::toolBoxLayoutByIndexApplyBtnPressed(){
    qDebug()<<"User request to apply prominence-based layout...";
    int selectedIndex = toolBoxLayoutByIndexSelect->currentIndex();
    QString selectedIndexText = toolBoxLayoutByIndexSelect->currentText();
    int selectedLayoutType = toolBoxLayoutByIndexTypeSelect->currentIndex();
    qDebug()<<"elected index is "
           << selectedIndexText << " : " << selectedIndex
           << " selected layout type is " << selectedLayoutType;
    switch(selectedIndex) {
    case 0:
        break;
    case 1:
        if (selectedLayoutType==0)
            slotLayoutRadialRandom();
        else if (selectedLayoutType==1)
            slotLayoutRandom();
        break;
    default:
        if (selectedLayoutType==0)  { // radial
            slotLayoutRadialByProminenceIndex(selectedIndexText);
        }
        else if (selectedLayoutType==1)  { // on levels
            slotLayoutLevelByProminenceIndex(selectedIndexText);
        }
        else if (selectedLayoutType==2) { //  node size
            slotLayoutNodeSizeByProminenceIndex(selectedIndexText);
            // re-init other options for node sizes...
        }
        else if (selectedLayoutType==3){  // node color
            slotLayoutNodeColorByProminenceIndex(selectedIndexText);
        }
        break;
    };
}



/**
 * @brief Called when the user selects a model in the Layout by Force Directed
 * selectbox of left panel.
 */
void MainWindow::toolBoxLayoutForceDirectedApplyBtnPressed(){
    qDebug()<<"User selected to apply a FDP layout...";
    int selectedModel = toolBoxLayoutForceDirectedSelect->currentIndex();
    QString selectedModelText = toolBoxLayoutForceDirectedSelect->currentText();
    qDebug() << " selected index is " << selectedModelText << " : "
             << selectedModel;

    switch(selectedModel) {
    case 0:
        break;
    case 1:
        slotLayoutGuides(false);
        slotLayoutKamadaKawai();
        break;
    case 2:
        slotLayoutGuides(false);
        slotLayoutFruchterman();
        break;
    case 3:
        slotLayoutGuides(false);
        slotLayoutSpringEmbedder();
        break;
    default:
        toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
        break;
    };
}





/**
 * @brief Starts a new network (closing the current one).
 */
void MainWindow::slotNetworkNew() {
    slotNetworkClose();
}



/**
 * @brief Returns the last path used by user to open/save something
 */
QString MainWindow::getLastPath() {
    if ( appSettings["lastUsedDirPath"] == "socnetv-initial-none") {
        appSettings["lastUsedDirPath"] = appSettings["dataDir"];
    }
    qDebug()<< "Last path used: " << appSettings["lastUsedDirPath"] ;
    return appSettings["lastUsedDirPath"] ;
}


/**
 * @brief Sets the last path used by user to open/save a network and adds the file
 * to recent files...
  * @param filePath
 */
void MainWindow::setLastPath(const QString &filePath) {
    qDebug()<< "Setting last path and adding to recent files:" << filePath;
    QString currentPath = QFileInfo(filePath).dir().absolutePath();
    QDir::setCurrent(currentPath);
    appSettings["lastUsedDirPath"] = currentPath;

    if (    !QFileInfo(filePath).completeSuffix().toLower().contains( "bmp" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "jpg" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "png" ) &&
            !QFileInfo(filePath).completeSuffix().toLower().contains( "pdf" )
            ) {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while(recentFiles.size() > MaxRecentFiles )
            recentFiles.removeLast();
    }
    slotNetworkFileRecentUpdateActions();
    saveSettings();

}



/**
 * @brief Chooses a network file to load
 *
 * If m_fileName is empty, opens a file selection dialog
 * Then calls slotNetworkFilePreview()
 * Called on application loading from command line with filename parameter
 * Called from slotNetworkImport* methods
 * Called from slotNetworkFileLoadRecent
 *
 * @param m_fileName
 * @param fileFormat
 * @param checkSelectFileType
 */
void MainWindow::slotNetworkFileChoose(QString m_fileName,
                                       int fileFormat,
                                       const bool &checkSelectFileType) {
    qDebug() << " m_fileName: " << m_fileName
             << " fileFormat " << fileFormat
             << " checkSelectFileType " << checkSelectFileType;

    previous_fileName=fileName;
    QString fileType_filter;

    /*
     * CASE 1: No filename provided. This happens when:
     * - User clicked Open Network File or
     * - User clicked Import Network
     *
     * Prepare known filetypes and
     * Open a file selection dialog for the user
     *
     */
    if (m_fileName.isNull() || m_fileName.isEmpty() ) {

        fileType=fileFormat;

        // prepare supported filetype extensions
        switch (fileType){
        case FileType::GRAPHML:
            fileType_filter = tr("GraphML (*.graphml *.xml);;All (*)");
            break;
        case FileType::PAJEK:
            fileType_filter = tr("Pajek (*.net *.paj *.pajek);;All (*)");
            break;
        case FileType::ADJACENCY:
            fileType_filter = tr("Adjacency (*.csv *.sm *.adj *.txt);;All (*)");
            break;
        case FileType::GRAPHVIZ:
            fileType_filter = tr("GraphViz (*.dot);;All (*)");
            break;
        case FileType::UCINET:
            fileType_filter = tr("UCINET (*.dl *.dat);;All (*)");
            break;
        case FileType::GML:
            fileType_filter = tr("GML (*.gml);;All (*)");
            break;

        case FileType::EDGELIST_WEIGHTED:
            fileType_filter = tr("Weighted Edge List (*.txt *.list *.edgelist *.lst *.wlst);;All (*)");
            break;
        case FileType::EDGELIST_SIMPLE:
            fileType_filter = tr("Simple Edge List (*.txt *.list *.edgelist *.lst);;All (*)");
            break;
        case FileType::TWOMODE:
            fileType_filter = tr("Two-Mode Sociomatrix (*.2sm *.aff);;All (*)");
            break;
        default:	//All
            fileType_filter = tr("GraphML (*.graphml *.xml);;"
                                 "GML (*.gml *.xml);;"
                                 "Pajek (*.net *.pajek *.paj);;"
                                 "UCINET (*.dl *.dat);;"
                                 "Adjacency (*.csv *.adj *.sm *.txt);;"
                                 "GraphViz (*.dot);;"
                                 "Weighted Edge List (*.txt *.edgelist *.list *.lst *.wlst);;"
                                 "Simple Edge List (*.txt *.edgelist *.list *.lst);;"
                                 "Two-Mode Sociomatrix (*.2sm *.aff);;"
                                 "All (*)");
            break;

        }
        //prepare the filedialog
        QFileDialog *fileDialog = new QFileDialog(this);
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setNameFilter(fileType_filter);
        fileDialog->setViewMode(QFileDialog::Detail);
        fileDialog->setDirectory(getLastPath());

        //connect its signals to our slots
        connect ( fileDialog, &QFileDialog::filterSelected,
                  this, &MainWindow::slotNetworkFileDialogFilterSelected);
        connect ( fileDialog, &QFileDialog::fileSelected,
                  this, &MainWindow::slotNetworkFileDialogFileSelected);
        connect ( fileDialog, &QFileDialog::rejected ,
                  this, &MainWindow::slotNetworkFileDialogRejected);

        //open the filedialog
        statusMessage( tr("Choose a network file..."));
        if (fileDialog->exec()) {
            m_fileName = (fileDialog->selectedFiles()).at(0);
            qDebug() << "m_fileName " << m_fileName;

        }
        else {
            //display some error
            statusMessage( tr("Nothing to do..."));
        }
        return;

    }


    /*
     * CASE 2: Filename provided. This happens when:
     * - Application starts from command line with filename parameter or
     * - User selects a Recent File or
     * - User selects a file in a previous slotNetworkFileChoose call
     *
     * If checkSelectFileType==TRUE (that is on app start or Recent File),
     * it tries to understand fileType by file extension. If file has unknown
     * file extension or an ambiguous file extension used by many different file
     * formats, then it asks the user to provide the fileType. Then it loads the
     * file
     *
     * If checkSelectFileType==FALSE, then it loads the file with given fileType.
     *
     */
    if (checkSelectFileType || fileFormat==FileType::UNRECOGNIZED) {

        // This happens only on application startup or on loading a recent file.
        if ( ! m_fileName.endsWith(".graphml",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".net",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".paj",Qt::CaseInsensitive )  &&
             ! m_fileName.endsWith(".pajek",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".dl",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".dat",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".gml",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".wlst",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".wlist",Qt::CaseInsensitive )&&
             ! m_fileName.endsWith(".dot",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".2sm",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".sm",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".csv",Qt::CaseInsensitive ) &&
             ! m_fileName.endsWith(".aff",Qt::CaseInsensitive ))
        {
            //ambigious file type. Open an input dialog for the user to choose
            // what kind of network file this is.

            tempFileNameNoPath=m_fileName.split ("/");
            QStringList fileTypes;
            fileTypes << tr("GraphML")
                      << tr("GML")
                      << tr("Pajek")
                      << tr("UCINET")
                      << tr("Adjacency")
                      << tr("GraphViz")
                      << tr("Edge List (weighted)")
                      << tr("Edge List (simple, non-weighted)")
                      << tr("Two-mode sociomatrix") ;

            bool ok;
            QString userFileType= QInputDialog::getItem(
                        this,
                        tr("Selected file has ambiguous file extension!"),
                        tr("You selected: %1 \n"

                           "The name of this file has either an unknown extension \n"
                           "or an extension used by different network file formats.\n\n"

                           "SocNetV supports the following social network file "
                           "formats. \nIn parentheses the expected extension. \n"
                           "- GraphML (.graphml or .xml)\n"
                           "- GML (.gml or .xml)\n"
                           "- Pajek (.paj or .pajek or .net)\n"
                           "- UCINET (.dl .dat) \n"
                           "- GraphViz (.dot)\n"
                           "- Adjacency Matrix (.csv, .txt, .sm or .adj)\n"
                           "- Simple Edge List (.list or .lst)\n"
                           "- Weighted Edge List (.wlist or .wlst)\n"
                           "- Two-Mode / affiliation (.2sm or .aff) \n\n"

                           "If you are sure the file is of a supported format, please \n"
                           "select the right format from the list below.").
                        arg(tempFileNameNoPath.last()),
                        fileTypes, 0, false, &ok);
            if (ok && !userFileType.isEmpty()) {
                if (userFileType == "GraphML") {
                    fileFormat=FileType::GRAPHML;
                }
                else if (userFileType == "GraphML") {
                    fileFormat=FileType::PAJEK;
                }
                else if (userFileType == "GML") {
                    fileFormat=FileType::GML;
                }
                else if (userFileType == "UCINET") {
                    fileFormat=FileType::UCINET;
                }
                else if (userFileType == "Adjacency") {
                    fileFormat=FileType::ADJACENCY;
                }
                else if (userFileType == "GraphViz") {
                    fileFormat=FileType::GRAPHVIZ;
                }
                else if (userFileType == "Edge List (weighted)") {
                    fileFormat=FileType::EDGELIST_WEIGHTED;
                }
                else if (userFileType == "Edge List (simple, non-weighted)") {
                    fileFormat=FileType::EDGELIST_SIMPLE;
                }
                else if (userFileType == "Two-mode sociomatrix") {
                    fileFormat=FileType::TWOMODE;
                }

            }
            else {
                statusMessage( tr("Opening network file aborted."));
                //if a file was previously opened, get back to it.
                if (activeGraph->isLoaded())	{
                    fileName=previous_fileName;
                }
                return;
            }

        }

        else if (m_fileName.endsWith(".graphml",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".xml",Qt::CaseInsensitive ) ) {
            fileFormat=FileType::GRAPHML;
        }
        else if (m_fileName.endsWith(".net",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".paj",Qt::CaseInsensitive )  ||
                 m_fileName.endsWith(".pajek",Qt::CaseInsensitive ) ) {
            fileFormat=FileType::PAJEK;
        }
        else if (m_fileName.endsWith(".dl",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".dat",Qt::CaseInsensitive ) ) {
            fileFormat=FileType::UCINET;
        }
        else if (m_fileName.endsWith(".sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".csv",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".adj",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".txt",Qt::CaseInsensitive )) {
            fileFormat=FileType::ADJACENCY;
        }
        else if (m_fileName.endsWith(".dot",Qt::CaseInsensitive ) ) {
            fileFormat=FileType::GRAPHVIZ;
        }
        else if (m_fileName.endsWith(".gml",Qt::CaseInsensitive ) ) {
            fileFormat=FileType::GML;
        }
        else if (m_fileName.endsWith(".list",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".lst",Qt::CaseInsensitive )  ) {
            fileFormat=FileType::EDGELIST_SIMPLE;
        }
        else if (m_fileName.endsWith(".wlist",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".wlst",Qt::CaseInsensitive )  ) {
            fileFormat=FileType::EDGELIST_WEIGHTED;
        }
        else if (m_fileName.endsWith(".2sm",Qt::CaseInsensitive ) ||
                 m_fileName.endsWith(".aff",Qt::CaseInsensitive )  ) {
            fileFormat=FileType::TWOMODE;
        }
        else
            fileFormat=FileType::UNRECOGNIZED;
    }


    qDebug()<<"Calling slotNetworkFilePreview"
           << "with m_fileName" << m_fileName
           << "and fileFormat " << fileFormat;

    slotNetworkFilePreview(m_fileName, fileFormat );


}



/**
 * @brief Displays a status message when the user aborts the file dialog
 */
void MainWindow::slotNetworkFileDialogRejected() {
    qDebug() << "Dialog rejected. If a file was previously opened, get back to it.";
    statusMessage( tr("Opening aborted"));
}


/**
 * @brief Called when user the selects a file filter (i.e. GraphML) in the fileDialog
 * @param filter
 */
void MainWindow::slotNetworkFileDialogFilterSelected(const QString &filter) {
    qDebug() << "User selected network file filter" << filter;
    if (filter.startsWith("GraphML",Qt::CaseInsensitive ) ) {
        fileType=FileType::GRAPHML;
        qDebug() << "fileType FileType::GRAPHML";
    }
    else if (filter.contains("PAJEK",Qt::CaseInsensitive ) ) {
        fileType=FileType::PAJEK;
        qDebug() << "fileType FileType::PAJEK";
    }
    else if (filter.contains("DL",Qt::CaseInsensitive ) ||
             filter.contains("UCINET",Qt::CaseInsensitive ) ) {
        fileType=FileType::UCINET;
        qDebug() << "fileType FileType::UCINET";
    }
    else if (filter.contains("Adjacency",Qt::CaseInsensitive ) ) {
        fileType=FileType::ADJACENCY;
        qDebug() << "fileType FileType::ADJACENCY";
    }
    else if (filter.contains("GraphViz",Qt::CaseInsensitive ) ) {
        fileType=FileType::GRAPHVIZ;
        qDebug() << "fileType FileType::GRAPHVIZ";
    }
    else if (filter.contains("GML",Qt::CaseInsensitive ) ) {
        fileType=FileType::GML;
        qDebug() << "fileType FileType::GML";
    }
    else if (filter.contains("Simple Edge List",Qt::CaseInsensitive ) ) {
        fileType=FileType::EDGELIST_SIMPLE;
        qDebug() << "fileType FileType::EDGELIST_SIMPLE";
    }
    else if (filter.contains("Weighted Edge List",Qt::CaseInsensitive ) ) {
        fileType=FileType::EDGELIST_WEIGHTED;
        qDebug() << "fileType FileType::EDGELIST_WEIGHTED";
    }
    else if (filter.contains("Two-Mode",Qt::CaseInsensitive )  ) {
        fileType=FileType::TWOMODE;
        qDebug() << "fileType FileType::TWOMODE";
    }
    else {
        fileType=FileType::UNRECOGNIZED;
        qDebug() << "fileType FileType::UNRECOGNIZED";
    }


}


/**
 * @brief Called when user selects a file in the fileDialog
 * Calls slotNetworkFileChoose() again.
 * @param fileName
 *
 */
void MainWindow::slotNetworkFileDialogFileSelected(const QString &fileName) {
    qDebug() << "User selected filename:" << fileName
             << "calling slotNetworkFileChoose() with fileType" << fileType;
    slotNetworkFileChoose( fileName,
                           fileType,
                           (  (fileType==FileType::UNRECOGNIZED) ? true : false )
                           );
}



/**
 * @brief Saves the network to a file
 *
 * First, it checks if a fileName is currently set
 * If not, calls slotNetworkSaveAs (which prompts for a fileName before returning here)
 * If a fileName is set, it checks if fileFormat is supported and saves the network.
 * If not supported, or the file is new, just tries to save in GraphML
 * For other exporting options the user is informed to use the export menu.
 *
 * @param fileFormat
 */
void MainWindow::slotNetworkSave(const int &fileFormat) {
    statusMessage( tr("Saving file..."));

    if (activeNodes() == 0) {
        statusMessage(  QString(tr("Nothing to save. There are no vertices.")) );
    }
    if (activeGraph->isSaved()) {
        statusMessage(  QString(tr("Graph already saved.")) );
    }
    if ( fileName.isEmpty() ) {
        slotNetworkSaveAs();
        return;
    }

    QFileInfo fileInfo (fileName);

    fileNameNoPath = fileInfo.fileName();

    bool saveZeroWeightEdges = appSettings["saveZeroWeightEdges"] == "true" ? true:false;

    bool saveEdgeWeights = true;

    // if the specified format is one of the supported ones, just save it.
    if ( activeGraph->isFileFormatExportSupported( fileFormat ) )
    {
        activeGraph->saveToFile(fileName, fileFormat, saveEdgeWeights, saveZeroWeightEdges );
    }
    // else if it is GraphML or new file not saved yet, just save it.
    else if (activeGraph->getFileFormat()==FileType::GRAPHML ||
             ( activeGraph->isSaved() && !activeGraph->isLoaded() )
             )
    {
        activeGraph->saveToFile(fileName, FileType::GRAPHML, saveEdgeWeights, saveZeroWeightEdges);
    }
    // else check whether Graph thinks this is supported and save it
    else if ( activeGraph->isFileFormatExportSupported(
                  activeGraph->getFileFormat()
                  ) )
    {
        activeGraph->saveToFile(fileName, activeGraph->getFileFormat(), saveEdgeWeights, saveZeroWeightEdges );
    }
    // In any other case, save in GraphML.
    // First, inform the user that we will save in that format.
    else
    {
        switch(
               slotHelpMessageToUser (USER_MSG_QUESTION,
                                      tr("Save to GraphML?"),
                                      tr("Default File Format: GraphML "),
                                      tr("This network will be saved in GraphML format "
                                         "which is the default file format of SocNetV. \n\n"
                                         "Is this OK? \n\n"
                                         "If not, press Cancel, then go to Network > Export menu "
                                         "to see other supported formats to export your data to.")
                                      )
               )
        {
        case QMessageBox::Yes:
            fileName = QFileInfo(fileName).absolutePath() + "/"  + QFileInfo(fileName).baseName();
            fileName.append(".graphml");
            fileNameNoPath = QFileInfo (fileName).fileName();
            setLastPath(fileName); // store this path
            activeGraph->saveToFile(fileName, FileType::GRAPHML, saveEdgeWeights, saveZeroWeightEdges);
            break;
        case QMessageBox::Cancel:
        case QMessageBox::No:
            statusMessage( tr("Save aborted...") );
            break;
        }
    }

}




/**
 * @brief Prompts the user to save the network in a new file.
 * Always uses the GraphML format and extension.
 */
void MainWindow::slotNetworkSaveAs() {
    qDebug() << "User wants to save the file as a new name...";
    statusMessage( tr("Enter or select a filename to save the network..."));

    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Save Network to GraphML File Named..."),
                getLastPath(), tr("GraphML (*.graphml *.xml);;All (*)") );

    if (!fn.isEmpty())  {

        if  ( QFileInfo(fn).suffix().isEmpty() ){
            fn.append(".graphml");
            slotHelpMessageToUser (
                        USER_MSG_INFO,
                        tr("Appending .graphml extension."),
                        tr("Missing file extension. \n"
                           "Appended the standard .graphml extension to the given filename."),
                        tr("Final Filename: ") + QFileInfo(fn).fileName()
                        );
        }
        else if ( !QFileInfo(fn).suffix().contains("graphml",  Qt::CaseInsensitive) &&
                  !QFileInfo(fn).suffix().contains("xml",  Qt::CaseInsensitive)  ) {
            fn = QFileInfo(fn).absolutePath() + "/"  + QFileInfo(fn).baseName();
            fn.append(".graphml");
            slotHelpMessageToUser (
                        USER_MSG_INFO,
                        tr("Using .graphml extension."),
                        tr("Wrong file extension. \n"
                           "Appended the standard .graphml extension to the given filename."),
                        tr("Final Filename: ") + QFileInfo(fn).fileName()
                        );

        }
        fileName=fn;
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
        setLastPath(fileName); // store this path
        slotNetworkSave(FileType::GRAPHML);
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }
}



/**
 * @brief Updates the 'save' status of the network
 *
 * Updates Save icon and window title (if saved)
 *  status > 0 means network has been saved
 *  status = 0 means network has changed and needs saving
 *  status < 0 means network has changed but there was an error saving it.
 *
 * @param status
  */
void MainWindow::slotNetworkSavedStatus (const int &status) {

    if (status < 0) {
        statusMessage( tr("Error! Could not save this file: %1").arg (fileNameNoPath));
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);

    }
    else if (status == 0) {
        // Network needs saving
        // UX: Maybe change it to a more prominent color for the user to see?
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);
    }
    else {
        // Network is saved.
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
        networkSaveAct->setEnabled(false);
        setWindowTitle( fileNameNoPath );
        statusMessage( tr("Network saved under filename: %1").arg (fileNameNoPath));
    }

}



/**
 * @brief Closes the current network, saving it if needed.
 */
bool MainWindow::slotNetworkClose() {

    qDebug()<<"Request to close current network file. Check if it is saved...";

    statusMessage( tr("Closing network file..."));

    if (!activeGraph->isSaved()) {
        switch (
                slotHelpMessageToUser (
                    USER_MSG_QUESTION,
                    tr("Closing Network..."),
                    tr("Network has not been saved. \n"
                       "Do you want to save before closing it?")
                    )
                )
        {
        case QMessageBox::Yes: slotNetworkSave(); break;
        case QMessageBox::No: break;
        case QMessageBox::Cancel: return false; break;
        }
    }
    qDebug()<<"Closing network file. Calling initApp ...";
    initApp();
    qDebug()<<"Network file closed...";
    statusMessage( tr("Ready."));
    return true;
}



/**
 * @brief Sends the active network to the printer
 */
void MainWindow::slotNetworkPrint() {
    statusMessage( tr("Printing..."));
    QPrintDialog dialog(printer, this);
    if ( dialog.exec() == QDialog::Accepted )   {
        QPainter painter(printer);
        graphicsWidget->render(&painter);
    };
    statusMessage( tr("Ready."));
}





/**
 * @brief Imports a network from a GraphML formatted file
 */
void MainWindow::slotNetworkImportGraphML(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::GRAPHML, m_checkSelectFileType);
}



/**
 * @brief Imports a network from a GML formatted file
 */
void MainWindow::slotNetworkImportGML(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::GML, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a Pajek-like formatted file
 */
void MainWindow::slotNetworkImportPajek(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::PAJEK, m_checkSelectFileType);
}




/**
 * @brief Imports a network from a Adjacency matrix formatted file
 */
void MainWindow::slotNetworkImportAdjacency(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::ADJACENCY, m_checkSelectFileType);
}




/**
 * @brief Imports a network from a Dot (GraphViz) formatted file
 */
void MainWindow::slotNetworkImportGraphviz(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString() ,FileType::GRAPHVIZ, m_checkSelectFileType);
}





/**
 * @brief Imports a network from a UCINET formatted file
 */
void MainWindow::slotNetworkImportUcinet(){
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::UCINET, m_checkSelectFileType);
}



/**
 * @brief Imports a network from a simple List or weighted List formatted file
 */
void MainWindow::slotNetworkImportEdgeList(){

    qDebug() << "Importing an edge list network file..." ;

    bool m_checkSelectFileType = false;

    switch(
           slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM,
                                 tr("Select type..."),
                                 tr("Select type of edge list format"),
                                 tr("SocNetV can parse two kinds of edgelist formats: \n\n"
                                    "A. Edge lists with edge weights, "
                                    "where each line has exactly 3 columns: "
                                    "source  target  weight, i.e.:\n"
                                    "1 2 1 \n"
                                    "2 3 1 \n"
                                    "3 4 2 \n"
                                    "4 5 1 \n\n"
                                    "B. Simple edge lists without weights, where each line "
                                    "has two or more columns in the form: source, target1, target2, ... , i.e.:\n"
                                    "1 2 3 4 5 6\n"
                                    "2 3 4 \n"
                                    "3 5 8 7\n\n"
                                    "Please select the appropriate type of edge list format of "
                                    "the file you want to load:"),
                                 QMessageBox::NoButton, QMessageBox::NoButton,
                                 tr("Weighted"), tr("Simple non-weighted")

                                 )
           )
    {
    case 1:
        qDebug() << "Weighted list selected! " ;
        slotNetworkFileChoose( QString(), FileType::EDGELIST_WEIGHTED, m_checkSelectFileType);
        break;
    case 2:
        qDebug() << "Simple list selected! " ;
        slotNetworkFileChoose( QString(), FileType::EDGELIST_SIMPLE, m_checkSelectFileType);
        break;
    }
}



/**
 * @brief Imports a network from a two mode sociomatrix formatted file
 */
void MainWindow::slotNetworkImportTwoModeSM(){
    qDebug() << "Importing a two mode sociomatrix network file..." ;
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose( QString(), FileType::TWOMODE, m_checkSelectFileType);
}



/**
 * @brief Setup a list of all text codecs supported by OS
 */
void MainWindow::initNetworkAvailableTextCodecs() {
    qDebug() << "Checking which text codecs are supported and storing them to a list" ;
    QMap<QString, QTextCodec *> codecMap;
    QRegularExpression iso8859RegExp("ISO[- ]8859-([0-9]+).*");
    QRegularExpressionMatch match;

    foreach (int mib, QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

//    // FOR FUTURE REFERENCE (IF QTextCodec Class GETS REMOVED FROM QT6 QT5 CORE COMPAT MODULE)
        // Verify that Codec/Encoding is supported by QStringConverter,
        // Otherwise skip it.
//        std::optional<QStringConverter::Encoding> test_support = QStringConverter::encodingForName(codec->name());
//        if ( ! test_support.has_value()) {
//            continue;
//        }

        QString sortKey = codec->name().toUpper();
        match = iso8859RegExp.match(sortKey);

        int rank;

        if (sortKey.startsWith("UTF-8")) {
            rank = 1;
        } else if (sortKey.startsWith("UTF-16")) {
            rank = 2;
        } else if ( match.hasMatch()) {
            if (match.captured(1).size() == 1)
                rank = 3;
            else
                rank = 4;
        } else {
            rank = 5;
        }
        sortKey.prepend(QChar('0' + rank));

        codecMap.insert(sortKey, codec);
    }
    codecs = codecMap.values();
}



/**
 * @brief  Opens the preview dialog with the selected file contents
 *
 * The aim is to let the user see the file and possibly select the appropriate text codec.
 *
 * @param m_fileName
 * @param fileFormat
 * @return
 */
bool MainWindow::slotNetworkFilePreview(const QString &m_fileName,
                                        const int &fileFormat ){
    qDebug() << "Previewing file: "<< m_fileName;
    if (m_fileName.isEmpty()) {
        statusMessage(tr("No file selected."));
        return false;
     }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        QApplication::restoreOverrideCursor();
        slotHelpMessageToUserError(
                    tr("Cannot read file %1:\n%2")
                    .arg(m_fileName)
                    .arg(file.errorString())
                    );
        return false;
    }
    // Read data and pass them to the dialog
    QByteArray data = file.readAll();
    m_dialogPreviewFile->setEncodedData(data, m_fileName, fileFormat);
    // Restore the cursor
    QApplication::restoreOverrideCursor();
    // Show the dialog
    m_dialogPreviewFile->exec();
    return true;
}




/**
 * @brief Loads a selected file entry from the "Recent Files" menu
 */
void MainWindow::slotNetworkFileLoadRecent() {

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        qDebug() << "Loading recent file: " << action->data().toString()  ;
        slotNetworkFileChoose(action->data().toString() );
    }
}




/**
 * @brief Loads the given network file
 *
 * Inits the app, then calls the loadFile method of Graph.
 *
 * @param m_fileName
 * @param codecName
 * @param fileFormat
 */
void MainWindow::slotNetworkFileLoad(const QString &fileNameToLoad,
                                     const QString &codecName,
                                     const int &fileFormat )
{
    qDebug() << "Request to to load the file:"<< fileNameToLoad
             << "codecName" << codecName
             << "fileFormat" << fileFormat;

    initApp();

    userSelectedCodecName = codecName; // var for future use in a Settings dialog
    QString delimiter=QString();
    int sm_two_mode = 0;
    int sm_has_labels = 0;

    if ( fileFormat == FileType::TWOMODE ) {
        switch(
               slotHelpMessageToUser (
                   USER_MSG_QUESTION_CUSTOM,
                   tr("Select mode"),
                   tr("Two-mode sociomatrix"),
                   tr("If this file is in two-mode sociomatrix format, "
                      "please specify which mode to open \n\n"
                      "1st mode: rows are nodes \n"
                      "2nd mode: columns are nodes"),
                   QMessageBox::NoButton,
                   QMessageBox::Ok,
                   tr("1st Mode"),tr("2nd mode")

                   )
               ) {
        case 1:
            sm_two_mode = 1;
            break;
        case 2:
            sm_two_mode = 2;
            break;
        }
    }
    else if ( fileFormat == FileType::ADJACENCY ) {
        // Ask if there are labels defined on the first line of the ADJACENCY file
        switch(
               slotHelpMessageToUser (
                   USER_MSG_QUESTION_CUSTOM,
                   tr("Opt for labels"),
                   tr("Node labels?"),
                   tr("This file contains an adjacency matrix (sociomatrix). "
                      "Please specify whether there are node labels defined "
                      "on the first (comment) line. \n"),
                   QMessageBox::NoButton,
                   QMessageBox::Ok,
                   tr("Yes"),tr("No")

                   )
               ) {
        case 1:
            sm_has_labels = 1;
            break;
        case 2:
            sm_has_labels = 0;
            break;
        }

    }


    // Ask for data delimiter
    if ( fileFormat == FileType::ADJACENCY ||
         fileFormat == FileType::EDGELIST_SIMPLE ||
         fileFormat == FileType::EDGELIST_WEIGHTED ) {
        bool ok;
        delimiter =
                QInputDialog::getText(
                    this, tr("Column delimiter in file "),
                    tr("SocNetV supports edge list and adjacency "
                       "files with arbitrary column delimiters. \n"
                       "The default delimiter is one or more spaces.\n\n"
                       "If the column delimiter in this file is "
                       "other than simple space or TAB, \n"
                       "please enter it below.\n\n"
                       "For instance, if the delimiter is a "
                       "comma or pipe enter \",\" or \"|\" respectively.\n\n"
                       "Leave empty to use space or TAB as delimiter."),
                    QLineEdit::Normal,
                    QString(""), &ok);
        if (!ok || delimiter.isEmpty() || delimiter.isNull() ) {
            delimiter=" ";
        }
        qDebug()<<"selected delimiter" << delimiter;
    }

    // Change the cursor to wait cursor
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    qDebug() << "Calling graph to do the file load loading...";

    activeGraph->loadFile (
                fileNameToLoad,
                codecName,
                fileFormat,
                delimiter,
                sm_two_mode,
                sm_has_labels
                );

}




/**
 * @brief Informs the user (and the MW) about the type of the network loaded
 *
 * Called from Parser/Graph when a network file is loaded to
 * display the appropiate message.
 *
 * @param type
 * @param fName
 * @param netName
 * @param totalNodes
 * @param totalEdges
 * @param elapsedTime
 * @param message
 */
void MainWindow::slotNetworkFileLoaded (const int &type,
                                        const QString &fName,
                                        const QString &netName,
                                        const int &totalNodes,
                                        const int &totalEdges,
                                        const qreal &density,
                                        const qint64 &elapsedTime,
                                        const QString &message)
{

    // Restore the cursor override
    QApplication::restoreOverrideCursor();

    if (type <= 0 || fName.isEmpty() ) {
        qDebug()<< "ERROR LOADING FILE. FILE UNRECOGNIZED. Message from Parser: "
                << message
                << "Calling initApp()";

        statusMessage( tr("Error loading requested file. Aborted."));

        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error loading file"),
                              tr("Error loading network file"),
                              tr("Sorry, the selected file is not in a supported format or encoding, "
                                 "or contains formatting errors. \n\n"
                                 "The error message was: \n\n"
                                 "%1"
                                 "\n\n"
                                 "What now? Review the message above to see if it helps you to fix the data file. "
                                 "Try a different codec in the preview window "
                                 "or if the file is of a legacy format (i.e. Pajek, UCINET, GraphViz, etc), "
                                 "please use the options in the Import sub menu. \n").arg(message)
                              );

        initApp();

        return;
    }

    // A file has been loaded successfully.
    // Update our MW UI and save file path in settings

    qDebug()<< "Got signal that a file was loaded:"
            << " filename" << fName
            << " type " << type
            << " totalNodes" << totalNodes
            << " totalEdges" << totalEdges;

    fileName=fName;
    previous_fileName=fileName;
    QFileInfo fileInfo (fileName);
    fileNameNoPath = fileInfo.fileName();

    Q_ASSERT_X( !fileNameNoPath.isEmpty(),  "not empty filename ", "empty filename " );

    setWindowTitle(fileNameNoPath);
    setLastPath(fileName); // store this path and file

    QString fileFormatHuman;

    switch( type ) 	{
    case FileType::NOT_SAVED:
        break;
    case FileType::GRAPHML:
        fileFormatHuman = "GraphML";
        break;
    case FileType::PAJEK:
        fileFormatHuman = "Pajek";
        break;
    case FileType::ADJACENCY:
        fileFormatHuman = "Adjacency";
        break;
    case FileType::GRAPHVIZ:
        fileFormatHuman = "GraphViz";
        break;
    case FileType::UCINET:
        fileFormatHuman = "UCINET";
        break;
    case FileType::GML:
        fileFormatHuman = "GML";
        break;
    case FileType::EDGELIST_WEIGHTED:
        fileFormatHuman = "Weighted list";
        break;
    case FileType::EDGELIST_SIMPLE:
        fileFormatHuman = "Simple list";
        break;
    case FileType::TWOMODE:
        fileFormatHuman = "Two-mode affiliation";
        break;
    default: // Every non-expected case
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error"),
                              tr("Error"),
                              tr("Unrecognized format. Please specify the file-format using the Import Menu.")
                              );

        break;
    }

    // Update LCDs
    rightPanelNodesLCD->setText (QString::number(totalNodes));
    rightPanelEdgesLCD->setText(QString::number(totalEdges));
    rightPanelDensityLCD->setText(QString::number(density));

    statusMessage( tr("%1 formatted network, named '%2', loaded. Nodes: %3, Edges: %4, Density: %5. Elapsed time: %6 ms")
                   .arg(fileFormatHuman)
                   .arg( netName)
                   .arg( totalNodes )
                   .arg(totalEdges)
                   .arg(density)
                   .arg(elapsedTime) );

    networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
    networkSaveAct->setEnabled(false);

    QApplication::restoreOverrideCursor();
}



/**
 * @brief Toggles the interactive/selection mouse drag mode
 * @param checked
 */
void MainWindow::slotEditDragModeSelection(bool checked){
    qDebug() << "User changed drag mode, checked" << checked;

    editMouseModeScrollAct->setChecked(false);

    if (editMouseModeInteractiveAct->isChecked()) {

        graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);
        graphicsWidget->setInteractive(true);
    }
    else {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
        graphicsWidget->setInteractive(false);
    }



}



/**
 * @brief Toggles the non-interactive scrollhand drag mode.
 * @param checked
 */
void MainWindow::slotEditDragModeScroll(bool checked){

    qDebug() << "User changed scroll mode, checked" << checked;

    editMouseModeInteractiveAct->setChecked(false);
    graphicsWidget->setInteractive(false);

    if ( editMouseModeScrollAct->isChecked() ) {

        graphicsWidget->setDragMode(QGraphicsView::ScrollHandDrag);

    }
    else {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
    }

}



/**
 * @brief Clears the relations combo.
 */
void MainWindow::slotEditRelationsClear(){
    qDebug() << "Clearing relations combo...";
    editRelationChangeCombo->clear();
}


/**
 * @brief Prompts the user to enter the name of a new relation
 *
 * On success, emits signal to Graph to change to the new relation.
 */
void MainWindow::slotEditRelationAddPrompt() {

    bool ok;
    QString newRelationName;
    int relationsCounter=activeGraph->relations();

    qDebug() << "Prompting the user for the new relation name to be added to the relations combo...";

    //
    // Prompt the user for the new relation name
    //

    // Check if this is the first time, in order to show a more comprehensive message
    if (relationsCounter==1 && activeNodes()==0 ) {
        newRelationName = QInputDialog::getText(
                    this,
                    tr("Add new relation"),
                    tr("Enter a name for a new relation between the actors.\n"
                       "A relation is a collection of ties of a "
                       "specific kind between the network actors.\n"
                       "For instance, enter \"friendship\" if the "
                       "edges of this relation refer to the set of \n"
                       "friendships between pairs of actors."),
                    QLineEdit::Normal, QString(), &ok );
    }
    else {
        newRelationName = QInputDialog::getText(
                    this, tr("Add new relation"),
                    tr("Enter a name for the new relation (or press Cancel):"),
                    QLineEdit::Normal,QString(), &ok );
    }

    //
    // Check which button was pressed
    //
    if ( ok ) {

        // user pressed OK

        // Check if new relation name
        if (!newRelationName.isEmpty()){

            // a relation name entered

            // Check if it is already used by another relation.
            if ( editRelationChangeCombo->findText(newRelationName) > -1 )  {
                slotHelpMessageToUser(USER_MSG_CRITICAL,
                                      tr("Error. Relation name is used!"),
                                      tr("The relation name is already used."),
                                      tr("Please use another relation name that is not already used.")
                                      );
                return;

            }

            // Emit signal to Graph to add the relation and change to it
            bool changeRelation = true;
            emit signalRelationAddAndChange(newRelationName, changeRelation);
        }
        else {
            // no name entered
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. No relation name entered!"),
                                  tr("You did not type a name for this new relation")
                                  );
            return;
        }
    }
    else {
        // user pressed Cancel
        statusMessage( QString(tr("New relation cancelled.")) );
        return;
    }

    statusMessage( QString(tr("New relation named %1, added."))
                   .arg( newRelationName ) );
}



/**
 * @brief Adds a new relation to the relations combo
 *
 * Called from Graph when the network file parser or another Graph method
 * demands a new relation to be added to the UI combo.
 *
 * @param newRelationName
 */
void MainWindow::slotEditRelationAdd(const QString &newRelationName){

    qDebug() << "Adding new relation to relations combo:"
             << newRelationName;

    if (!newRelationName.isNull()) {

        editRelationChangeCombo->addItem(newRelationName);

        // Enable prev/next widgets, if they are disabled.
        if (!editRelationPreviousAct->isEnabled() && editRelationChangeCombo->count() > 1) {
            editRelationPreviousAct->setEnabled(true);
            editRelationNextAct->setEnabled(true);
        }

        statusMessage( QString(tr("Added a new relation named: %1."))
                       .arg( newRelationName ) );

    }

}




/**
 * @brief Changes the editRelations combo box index to relIndex
 *
 * If relIndex==RAND_MAX the index is set to the last relation index
 *
 * @param relIndex
 */
void MainWindow::slotEditRelationChange(const int &relIndex) {
    if ( relIndex == RAND_MAX){
        qDebug() << "relIndex==RANDMAX. Changing relation combo to last relation...";
        editRelationChangeCombo->setCurrentIndex(
                    ( editRelationChangeCombo->count()-1 )
                    );
    }
    else {
        qDebug() << "Changing relation combo to index" << relIndex;
        editRelationChangeCombo->setCurrentIndex(relIndex);
    }

}




/**
 * @brief Prompts the user to enter a new name for the current relation
 */
void MainWindow::slotEditRelationRename() {

    bool ok=false;

    qDebug()<<"Request to rename current relation:"
            << editRelationChangeCombo->currentText()
            << "Prompting for new name...";

    //
    // Get new name from user
    //
    QString newName = QInputDialog::getText(
                this,
                tr("Rename current relation"),
                tr("Enter a new name for this relation."),
                QLineEdit::Normal, QString(), &ok );

    //
    // Check entered name
    //
    if ( newName.isEmpty() || !ok ){
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Not a valid name."),
                              tr("Error"),
                              tr("You did not enter a valid name for this relation.")
                              );
        return;
    }

    //
    // Change name in combo - this will trigger the signal to activeGraph
    //
    editRelationChangeCombo->setCurrentText(newName);

}




/**
 * @brief Opens the Export to Image Dialog
 */
void MainWindow::slotNetworkExportImageDialog()
{
    qDebug() << "Opening Image export dialog...";

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage( tr("Opening Image export dialog. "));

    m_dialogExportImage = new DialogExportImage(this);

    connect( m_dialogExportImage, &DialogExportImage::userChoices,
             this, &MainWindow::slotNetworkExportImage);

    m_dialogExportImage->exec();
}



/**
 * @brief Exports the network to an image file
 *
 * @param filename
 * @param format
 * @param quality
 * @param compression
 */
void MainWindow::slotNetworkExportImage( const QString &filename,
                                         const QByteArray &format,
                                         const int &quality,
                                         const int &compression
                                         ) {

    qDebug() << "Exporting network to image file" << filename;

    if (filename.isEmpty())  {
        statusMessage( tr("No filename. Exporting to Image aborted.") );
        return;
    }
    // store this path
    setLastPath(filename);

    // Get network name from the filename
    tempFileNameNoPath=filename.split ("/");
    QString name = tempFileNameNoPath.last();
    name.truncate(name.lastIndexOf("."));

    //
    //  Grab network from canvas
    //
    qDebug() << "Grabbing network from the canvas";

    qreal ratio = 1;
    qreal w = graphicsWidget->width() * ratio;
    qreal h = graphicsWidget->height() * ratio;

    QImage picture = QImage(w, h, QImage::Format_ARGB32_Premultiplied);

    qDebug() << "Creating painter...";
    QPainter p;

    qDebug() << "Begin painter on picture...";
    p.begin(&picture);

    qDebug() << "render scene on painter...";
    graphicsWidget->render(&p);

    //
    // Add name and optionally log
    //
    qDebug() << "Adding name (and logo)..";
    p.setFont(QFont ("Helvetica", 10, QFont::Normal, false));
    if (appSettings["printLogo"]=="true") {
        QImage logo(":/images/socnetv-logo.png");
        p.drawImage(5,5, logo);
        p.drawText(7,47,name);
    }
    else {
        p.drawText(5,15,name);
    }

    qDebug() << "End painter on picture...";
    p.end();

    QString author = "SocNetV v" + VERSION;

    qDebug() << "slotNetworkExportImage() - saving image to file:"
             << filename
             << "format" << format
             << "quality:" << quality
             << "compression:" << compression
             << "Author:" << author;

    //
    // Write image to a file
    //
    QImageWriter imgWriter;
    imgWriter.setFormat(format);
    imgWriter.setQuality(quality);
    imgWriter.setCompression(compression);
    imgWriter.setFileName(filename);
    imgWriter.setText("Author", author);
    imgWriter.setText("", "Created by " + author);
    imgWriter.setOptimizedWrite(true);
    imgWriter.setProgressiveScanWrite(true);
    if ( imgWriter.write(picture) ) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Network exported to image file."),
                              tr("Network exported to image file."),
                              tr("Image filename: %1").arg(tempFileNameNoPath.last())
                              );
    }
    else {
        slotHelpMessageToUser(
                    USER_MSG_CRITICAL,
                    tr("Error exporting to image file!"),
                    tr("Error while exporting network to image file:"),
                    imgWriter.errorString()
                    );

    }

}



/**
 * @brief Opens the Export to PDF Dialog
 */
void MainWindow::slotNetworkExportPDFDialog()
{
    qDebug() << "MW::slotNetworkExportPDFDialog()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage( tr("Opening PDF export dialog. "));

    m_dialogExportPDF = new DialogExportPDF(this);

    connect( m_dialogExportPDF, &DialogExportPDF::userChoices,
             this, &MainWindow::slotNetworkExportPDF);

    m_dialogExportPDF->exec();
}




/**
 * @brief Exports the visible part of the network to a PDF Document
 *
 * @param pdfName
 * @param orientation
 * @param dpi
 * @param printerMode
 * @param pageSize
 */
void MainWindow::slotNetworkExportPDF(QString &pdfName,
                                      const QPageLayout::Orientation &orientation,
                                      const int &dpi,
                                      const QPrinter::PrinterMode printerMode=QPrinter::ScreenResolution,
                                      const QPageSize &pageSize = QPageSize(QPageSize::A4)
                                      ){
    qDebug()<< "MW::slotNetworkExportPDF()";

//    Q_UNUSED(dpi);

    if (pdfName.isEmpty())  {
        statusMessage( tr("No filename. Exporting to PDF aborted."));
        return;
    }
    else {

        setLastPath(pdfName); // store this path
        tempFileNameNoPath=pdfName.split ("/");
        QString name = tempFileNameNoPath.last();
        name.truncate(name.lastIndexOf("."));


        printerPDF = new QPrinter(printerMode);
        printerPDF->setOutputFormat(QPrinter::PdfFormat);
        printerPDF->setOutputFileName(pdfName);
        printerPDF->setPageOrientation(orientation);
        printerPDF->setPageSize(pageSize);
        printerPDF->setFontEmbeddingEnabled(true);
         printerPDF->setResolution(dpi);
        QPainter p;
        p.begin(printerPDF);
        graphicsWidget->render(&p, QRect(0, 0, printerPDF->width(), printerPDF->height()),
                                graphicsWidget->viewport()->rect());
        p.setFont(QFont ("Helvetica", 8, QFont::Normal, false));
        if (appSettings["printLogo"]=="true") {
            QImage logo(":/images/socnetv-logo.png");
            p.drawImage(5,5, logo);
            p.drawText(7,47,name);
        }
        else {
            p.drawText(5,15,name);
        }

        qDebug() << "End painter on QPrinter...";
        p.end();
        delete printerPDF;
    }
    qDebug()<< "Exporting PDF to "<< pdfName;
    tempFileNameNoPath=pdfName.split ("/");
    setLastPath(pdfName);
    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Network exported to PDF file."),
                          tr("Network exported to PDF file."),
                          tr("PDF filename: %1").arg(tempFileNameNoPath.last())
                          );

}




/**
 * @brief Exports the network to a Pajek-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportPajek()
{
    qDebug() << "MW::slotNetworkExportPajek";

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Pajek (*.paj *.net *.pajek);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Missing file extension. I will use .paj instead."),
                                  tr("Missing file extension. I will use the .paj extension."),
                                  tr("Appending an extension .paj to the given filename...")
                                  );
            fn.append(".paj");
        }
        fileName=fn;
        setLastPath(fileName);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }

    activeGraph->saveToFile(fileName, FileType::PAJEK);
}



/**
 * @brief Exports the network to an adjacency matrix-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportSM(){
    qDebug("MW: slotNetworkExportSM()");
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage( tr("Exporting active network under new filename..."));
    QString fn =  QFileDialog::getSaveFileName(
                this,
                tr("Export Network to File Named..."),
                getLastPath(), tr("Adjacency (*.csv *.txt *.adj *.sm *.net);;All (*)") );
    if (!fn.isEmpty())  {
        if  ( QFileInfo(fn).suffix().isEmpty() ){
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Missing file extension. I will use .csv instead."),
                                  tr("Missing file extension. I will use the .csv  extension."),
                                  tr("Appending an extension .csv  to the given filename...")
                                  );

            fn.append(".csv");
        }
        fileName=fn;
        setLastPath(fileName);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else  {
        statusMessage( tr("Saving aborted"));
        return;
    }


    bool saveEdgeWeights=false;
    if (activeGraph->isWeighted() )  {
        switch (
                slotHelpMessageToUser(USER_MSG_QUESTION,
                                      tr("Weighted graph. Social network with valued/weighted edges"),
                                      tr("Social network with valued/weighted edges"),
                                      tr("This social network includes valued/weighted edges "
                                         "(the depicted graph is weighted). "
                                         "Do you want to save the edge weights in the adjacency file?\n"
                                         "Select Yes if you want to save edge values "
                                         "in the resulting file. \n"
                                         "Select No, if you don't want edge values "
                                         "to be saved. In the later case, all non-zero values will be truncated to 1.")
                                      )

                )
        {
        case QMessageBox::Yes:
            saveEdgeWeights = true;
            break;
        case QMessageBox::No:
            saveEdgeWeights = false;
            break;
        case QMessageBox::Cancel:
            statusMessage( tr("Save aborted...") );
            return;
            break;
        }

    }

    activeGraph->saveToFile(fileName, FileType::ADJACENCY, saveEdgeWeights ) ;

}





/**
 * @brief TODO Exports the network to a DL-formatted file
 * @return
 */
bool MainWindow::slotNetworkExportDL(){
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return false;
    }

    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export UCINET", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;

}


/**
    TODO: Exports the network to a GW-formatted file
*/ 
bool MainWindow::slotNetworkExportGW(){
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return false;
    }

    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export GW", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;
}




/**
    TODO: Exports the network to a list-formatted file
*/
bool MainWindow::slotNetworkExportList(){
    if (fileName.isEmpty()) {
        statusMessage( tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(
                    this, "Export List", getLastPath(), 0);
        if (!fn.isEmpty())  {
            fileName=fn;
            setLastPath(fileName);
        }
        else  {
            statusMessage( tr("Saving aborted"));
            return false;
        }
    }

    return true;
}






/**
 * @brief Displays the file of the loaded network.
 *
 * If the network has been modified, it prompts the user
 * to save the network, then view its file.
 */
void MainWindow::slotNetworkFileView(){


    qDebug() << "Request to display current network file. Filename:" << fileName.toLatin1()
             << "isLoaded:" << activeGraph->isLoaded()
             << "isSaved:" << activeGraph->isSaved()
             << "graph filename:" << activeGraph->getFileName();

    if ( activeGraph->isLoaded() && activeGraph->isSaved()  ) {
        //network unmodified, read loaded file again.
        QFile f( fileName );
        if ( !f.open( QIODevice::ReadOnly | QIODevice::Text) ) {
            qDebug ("Error in open!");
            return;
        }
        TextEditor *ed = new TextEditor(fileName,this,false);
        QFileInfo fileInfo (fileName);
        fileNameNoPath = fileInfo.fileName();
        ed->setWindowTitle( fileNameNoPath );
        ed->show();
        m_textEditors << ed;
        statusMessage(  tr("Displaying network data file %1" ).arg(fileNameNoPath));
    }

    else if (!activeGraph->isSaved() ) {

        if ( !activeGraph->isLoaded() ) {
            // new network, not saved yet
            int response = slotHelpMessageToUser(
                        USER_MSG_QUESTION,
                        tr("New network not saved yet. You might want to save it first."),
                        tr("This new network you created has not been saved yet."),
                        tr("Do you want to open a file dialog to save your work "
                           "(then I will display the file)?"),
                        QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes
                        );
            if (  response == QMessageBox::Yes ) {
                slotNetworkSaveAs();
            }
            else { return; }
        }
        else {
            // loaded network, but modified
            int response = slotHelpMessageToUser(
                        USER_MSG_QUESTION,
                        tr("Current network has been modified. Save to the original file?"),
                        tr("Current social network has been modified since last save."),
                        tr("Do you want to save it to the original file?"),
                        QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes
                        );
            if ( response ==  QMessageBox::Yes ){
                slotNetworkSave();
            }else if (response ==QMessageBox::No ) {
                slotNetworkSaveAs();
            }
            else { // user pressed Cancel
                return;
            }

        }
        slotNetworkFileView();
    }
    else	{
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
    }
}




/**
 * @brief Opens the embedded text editor
 */
void MainWindow::slotNetworkTextEditor(){
    qDebug() << "slotNetworkTextEditor() : ";

    TextEditor *ed = new TextEditor("", this,false);
    ed->setWindowTitle(tr("New Network File"));
    ed->show();
    m_textEditors << ed;
    statusMessage(  tr("Enter your network data here" ) );
}





/**
 * @brief Displays the adjacency matrix of the network.
 *
 *  It uses a different method for writing the matrix to a file.
 *  While slotNetworkExportSM uses << operator of Matrix class
 *  (via adjacencyMatrix of Graph class), this is using directly the
 *  writeMatrixAdjacency method of Graph class
 */
void MainWindow::slotNetworkViewSociomatrix(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-"+dateTime+".html";

    qDebug() << "MW::slotNetworkViewSociomatrix() - dataDir"
              << appSettings["dataDir"]
              << "fn" <<fn;

    statusMessage ( tr ("Creating and writing adjacency matrix") );

    activeGraph->writeMatrixAdjacency(fn) ;
    //AVOID THIS, no preserving of node numbers when nodes are deleted.
    // activeGraph->writeMatrix(fn,MATRIX_ADJACENCY) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {

        qDebug() << "MW::slotNetworkViewSociomatrix() - "
                     "calling QDesktopServices::openUrl for"
                  << QUrl::fromLocalFile(fn) ;

        QDesktopServices::openUrl(  QUrl::fromLocalFile(fn)  );
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }
    statusMessage(tr("Adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Displays a text-only plot of the network adjacency matrix
 */
void MainWindow::slotNetworkViewSociomatrixPlotText(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int N=activeNodes();

    statusMessage(tr("Creating plot of adjacency matrix of %1 nodes.").arg(N ));

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-plot-"+dateTime+".html";

    bool simpler = false;
    if ( N > 999 ) {
        qreal MB = (N * N * 10)/(1024*1024);
        switch ( slotHelpMessageToUser(
                     USER_MSG_QUESTION,tr("Very large network to plot!"),
                     tr("Warning: Really large network"),
                     tr("To plot a %1 x %1 matrix arranged in HTML table, "
                        "I will need time to write a very large .html file , circa %2 MB in size. "
                        "Instead, I can create a simpler / smaller HTML file without table. "
                        "Press Yes to continue with simpler version, "
                        "Press No to create large file with HTML table.").arg(N).arg( MB ) ) ) {
        case QMessageBox::Yes:
            simpler = true;
            break;
        case QMessageBox::No:
            simpler = false;
            break;
        default:
            return;
            break;
        }
    }


    activeGraph->writeMatrixAdjacencyPlot(fn, simpler);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Visual form of adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Displays the dataset selection dialog
 */
void MainWindow::slotNetworkDataSetSelect(){
    qDebug()<< "MW::slotNetworkDataSetSelect()";

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    m_datasetSelectDialog = new DialogDataSetSelect(this);
    connect( m_datasetSelectDialog, SIGNAL( userChoices(QString) ),
             this, SLOT( slotNetworkDataSetRecreate(QString) ) );


    m_datasetSelectDialog->exec();
}



/**
 * @brief Recreates famous and widely used data sets in network analysis studies
 *
 * @param m_fileName
 *
 */
void MainWindow::slotNetworkDataSetRecreate (const QString m_fileName) {

    int fileFormat=0;

    qDebug()<< "MW::slotNetworkDataSetRecreate() datadir+fileName: "
            << appSettings["dataDir"]+m_fileName;

    activeGraph->writeDataSetToFile(appSettings["dataDir"], m_fileName);

    if (m_fileName.endsWith(".graphml")) {
        fileFormat=FileType::GRAPHML;
    }
    else if (m_fileName.endsWith(".pajek") || m_fileName.endsWith(".paj") ||
             m_fileName.endsWith(".net")) {
        fileFormat=FileType::PAJEK;
    }
    else if (m_fileName.endsWith(".sm") || m_fileName.endsWith(".adj") || m_fileName.endsWith(".csv")) {
        fileFormat=FileType::ADJACENCY;
    }
    else if (m_fileName.endsWith(".dot")) {
        fileFormat=FileType::GRAPHVIZ;
    }
    else if (m_fileName.endsWith(".dl")) {
        fileFormat=FileType::UCINET;
    }
    else if (m_fileName.endsWith(".gml")) {
        fileFormat=FileType::GML;
    }
    else if (m_fileName.endsWith(".wlst")) {
        fileFormat=FileType::EDGELIST_WEIGHTED;
    }
    else if (m_fileName.endsWith(".lst")) {
        fileFormat=FileType::EDGELIST_SIMPLE;
    }
    else if (m_fileName.endsWith(".2sm")) {
        fileFormat=FileType::TWOMODE;
    }

    slotNetworkFileLoad(appSettings["dataDir"]+m_fileName, "UTF-8", fileFormat);
}


/**
 * @brief Shows a dialog to create an Erdos-Renyi random network
 */
void MainWindow::slotNetworkRandomErdosRenyiDialog(){

    qDebug() << "Showing the dialog to create a random Erdos-Renyi network ";

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage( tr("Generate a random Erdos-Renyi network. "));

    m_randErdosRenyiDialog = new DialogRandErdosRenyi(
                this, appSettings["randomErdosEdgeProbability"].toFloat(0));

    connect( m_randErdosRenyiDialog, &DialogRandErdosRenyi::userChoices,
             this, &MainWindow::slotNetworkRandomErdosRenyi );

    m_randErdosRenyiDialog->exec();

}




/**
 * @brief Creates an Erdos-Renyi random symmetric network
 *
 * @param newNodes
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomErdosRenyi( const int newNodes,
                                              const QString model,
                                              const int edges,
                                              const qreal eprob,
                                              const QString mode,
                                              const bool diag)
{
    qDebug() << "Request to create an Erdos-Renyi random network...";

    statusMessage( tr("Creating new Erdos-Renyi random network. Please wait... ")  );

    appSettings["randomErdosEdgeProbability"] = QString::number(eprob);

    activeGraph->randomNetErdosCreate ( newNodes,
                                        model,
                                        edges,
                                        eprob,
                                        mode,
                                        diag);

    setWindowTitle("Untitled Erdos-Renyi random network");

    double threshold = log(newNodes)/newNodes;

    if ( (eprob ) > threshold )
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Erdős–Rényi random network created."),
                    tr("Random network created. \n"
                       "A new random network has been created according to the Erdős–Rényi model."),
                    tr("On average, edges should be %1. This graph is almost surely connected because: \n"
                        "probability > ln(n) that is: %2 < %3")
                    .arg(QString::number(eprob * newNodes*(newNodes-1)))
                    .arg(QString::number(eprob))
                    .arg(QString::number(threshold))
                    );

    else
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Erdős–Rényi random network created."),
                    tr("Random network created. \n"
                       "A new random network has been created according to the Erdős–Rényi model."),
                    tr("On average, edges should be %1. This graph is almost surely not connected because: \n"
                        "probability < ln(n) that is: %2 < %3")
                    .arg(QString::number(eprob * newNodes*(newNodes-1)))
                    .arg(QString::number(eprob))
                    .arg(QString::number(threshold))
                    );

}



/**
 * @brief Shows a dialog to create a scale-free random network
 */
void MainWindow::slotNetworkRandomScaleFreeDialog() {

     qDebug() << "Showing the dialog to create a random scale-free network ";

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage( tr("Generate a random Scale-Free network. "));
    m_randScaleFreeDialog = new DialogRandScaleFree(this);

    connect( m_randScaleFreeDialog, &DialogRandScaleFree::userChoices,
             this, &MainWindow::slotNetworkRandomScaleFree);

    m_randScaleFreeDialog->exec();

}


/**
 * @brief Creates a scale-free random network
 * @param nodes
 * @param power
 * @param initialNodes
 * @param edgesPerStep
 * @param zeroAppeal
 * @param mode
 */
void MainWindow::slotNetworkRandomScaleFree ( const int &newNodes,
                                              const int &power,
                                              const int &initialNodes,
                                              const int &edgesPerStep,
                                              const qreal &zeroAppeal,
                                              const QString &mode)
{
    qDebug() << "Request to create a new scale-free random network...";

    activeGraph->randomNetScaleFreeCreate( newNodes,
                                           power,
                                           initialNodes,
                                           edgesPerStep,
                                           zeroAppeal,
                                           mode);


    setWindowTitle("Untitled scale-free network");

    slotHelpMessageToUser (
                USER_MSG_INFO,
                tr("Scale-free random network created."),
                tr("Random network created. \n"
                   "A new scale-free random network with %1 nodes has been created according to the Barabási–Albert model.").arg(newNodes),
                tr("A scale-free network is a network whose degree distribution follows a power law.")

                );

}



/**
 * @brief Shows a dialog to create a small-world random network
 */
void MainWindow::slotNetworkRandomSmallWorldDialog()
{
    qDebug() << "Showing the dialog to create a random small-world network ";

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage( tr("Generate a random Small-World network. "));
    m_randSmallWorldDialog = new DialogRandSmallWorld(this);

    connect( m_randSmallWorldDialog, &DialogRandSmallWorld::userChoices,
             this, &MainWindow::slotNetworkRandomSmallWorld);

    m_randSmallWorldDialog->exec();

}


/**
 * @brief Creates a small-world random network
 *
 * @param newNodes
 * @param degree
 * @param beta
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomSmallWorld(const int &newNodes,
                                             const int &degree,
                                             const qreal &beta,
                                             const QString &mode,
                                             const bool &diag)
{
    Q_UNUSED(diag);

    qDebug() << "Request to create a new small-world random network...";

    activeGraph->randomNetSmallWorldCreate(newNodes, degree, beta, mode);

    setWindowTitle("Untitled small-world network");

    slotHelpMessageToUser (
                USER_MSG_INFO,
                tr("Small-World random network created."),
                tr("Random network created. \n"
                   "A new random network with %1 nodes has been created according to the Watts & Strogatz model.").arg(newNodes),
                tr("A small-world network has short average path lengths and high clustering coefficient.")
                );

}




/**
 * @brief Shows a dialog to create a d-regular random network
 */
void MainWindow::slotNetworkRandomRegularDialog()
{
    qDebug() << "Showing the dialog to create a random d-regular network ";

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage( tr("Generate a d-regular random network. "));
    m_randRegularDialog = new DialogRandRegular(this);

    connect( m_randRegularDialog, &DialogRandRegular::userChoices,
             this, &MainWindow::slotNetworkRandomRegular);

    m_randRegularDialog->exec();

}



/**
 * @brief Creates a pseudo-random d-regular network where every node has the same degree
 *
 * @param newNodes
 * @param degree
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomRegular(const int &newNodes, const int &degree,
                                          const QString &mode, const bool &diag){


    initApp();

    activeGraph->randomNetRegularCreate (newNodes,degree, mode, diag);

    setWindowTitle("Untitled d-regular network");


    slotHelpMessageToUser (
                USER_MSG_INFO,
                tr("d-regular network created."),
                tr("Random network created. \n"
                   "A new d-regular random network with %1 nodes has been created.").arg(newNodes),
                tr("Each node has the same number <em>%1</em> of neighbours, aka the same degree d.")
                .arg(degree)
                );

}





void MainWindow::slotNetworkRandomGaussian(){

}


/**
 * @brief Creates a ring lattice network
 *
 * A ring lattice is a network where each node has degree d:
 * - d/2 edges to the "right"
 * - d/2 edges to the "left"
 */
void MainWindow::slotNetworkRandomRingLattice(){

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    bool ok;
    statusMessage( "You have selected to create a ring lattice network. ");
    int newNodes=( QInputDialog::getInt(
                       this,
                       tr("Create ring lattice"),
                       tr("This will create a ring lattice network, "
                          "where each node has degree d:\n d/2 edges to the right "
                          "and d/2 to the left.\n"
                          "Please enter the number of nodes you want:"),
                       100, 4, maxRandomlyCreatedNodes, 1, &ok ) ) ;
    if (!ok) {
        statusMessage( "You did not enter an integer. Aborting.");
        return;
    }

    int degree = QInputDialog::getInt(
                this,
                tr("Create ring lattice..."),
                tr("Now, enter an even number d. \n"
                   "This is the total number of edges each new node will have:"),
                2, 2, newNodes-1, 2, &ok);

    if ( (degree % 2) == 1 ) {

        slotHelpMessageToUser (
                    USER_MSG_CRITICAL,
                    tr("Error. Cannot create such network."),
                    tr("Error. Cannot create such network!\n\n"
                       "The degree %1 is not an even number.").arg(degree),
                    tr("A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side. \n"
                    "Please try again entering an even number as degree.")

                    );
        return;
    }


    activeGraph->randomNetRingLatticeCreate(newNodes, degree, true );

    setWindowTitle("Untitled ring-lattice network");

    slotHelpMessageToUser (
                USER_MSG_INFO,
                tr("Ring lattice random network created."),
                tr("Random network created. \n"
                   "A new ring-lattice random network with %1 nodes has been created.").arg(newNodes),
                tr("A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side.")

                );

}



/**
 * @brief Shows a dialog to create a "random" lattice network.
 */
void MainWindow::slotNetworkRandomLatticeDialog()
{
    qDebug() << "Showing the Random Lattice Dialog...";
    statusMessage( tr("Generate a lattice network. "));
    m_randLatticeDialog = new DialogRandLattice(this);

    connect( m_randLatticeDialog, &DialogRandLattice::userChoices,
             this, &MainWindow::slotNetworkRandomLattice);

    m_randLatticeDialog->exec();

}



/**
 * @brief Creates a 'random' lattice network, i.e. a connected network where every node
 * has the same degree and is connected with its neighborhood
 *
 * A lattice is a network whose drawing forms a regular tiling
 * Lattices are also known as meshes or grids.
 *
 * @param newNodes
 * @param length
 * @param dimension
 * @param nei
 * @param mode
 * @param circular
 */
void MainWindow::slotNetworkRandomLattice(const int &newNodes,
                                          const int &length,
                                          const int &dimension,
                                          const int &nei,
                                          const QString &mode,
                                          const bool &circular){

    qDebug() << "Request to create a new lattice random network...";

    initApp();

    activeGraph->randomNetLatticeCreate (newNodes, length, dimension, nei, mode, circular);

    setWindowTitle("Untitled lattice network");

    slotHelpMessageToUser (
                USER_MSG_INFO,
                tr("Lattice random network created."),
                tr("Random network created. \n"
                   "A new lattice random network with %1 nodes has been created.").arg(newNodes),
                tr("A lattice is a network whose drawing forms a regular tiling. "
                    "Lattices are also known as meshes or grids.")
                );

}





/**
 * @brief Shows the web crawler dialog
 */
void MainWindow::slotNetworkWebCrawlerDialog() {

    // Close the current network
    if ( !this->slotNetworkClose() ) {
        // User cancelled. Do not proceed.
        return;
    }

    qDebug() << "Opening web crawler dialog...";

    m_WebCrawlerDialog = new DialogWebCrawler(this);

    connect (m_WebCrawlerDialog, &DialogWebCrawler::userChoices,
             this, &MainWindow::slotNetworkWebCrawler);

    m_WebCrawlerDialog->exec() ;
}



/**
 * @brief Starts the web crawler with the user options
 *
 * @param startUrl
 * @param urlPatternsIncluded
 * @param urlPatternsExcluded
 * @param linkClasses
 * @param maxNodes
 * @param maxLinksPerPage
 * @param intLinks
 * @param childLinks
 * @param parentLinks
 * @param selfLinks
 * @param extLinks
 * @param extLinksCrawl
 * @param socialLinks
 * @param delayedRequests
 */
void MainWindow::slotNetworkWebCrawler (const QUrl &startUrl,
                                        const QStringList &urlPatternsIncluded,
                                        const QStringList &urlPatternsExcluded,
                                        const QStringList &linkClasses,
                                        const int &maxNodes,
                                        const int &maxLinksPerPage,
                                        const bool &intLinks,
                                        const bool &childLinks,
                                        const bool &parentLinks,
                                        const bool &selfLinks,
                                        const bool &extLinks,
                                        const bool &extLinksCrawl,
                                        const bool &socialLinks,
                                        const bool &delayedRequests
                                        ) {

    // Check ssl
    if ( !QSslSocket::supportsSsl() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL,tr("No SSL support."),
                              tr("I cannot verify that your computer Operating System has OpenSSL support. \n\n"
                                "OpenSSL is an  Open Source software library for the Transport Layer Security (TLS) protocol (aka SSL), for applications that secure communications over computer networks. It is widely used by Internet servers, including the majority of HTTPS websites. \n\n"
                                "Without OpenSSL libraries installed in your computer, I cannot crawl webpages/URLs using https:// \n\n"
                                 "So, please download and install OpenSSL in your OS and try again."),
                              tr("Hint: Go to Help > System Information to see which OpenSSL version you need to install.")
                              );
        return;
    }


    // Start the web crawler
    qDebug() << "Calling Graph::startWebCrawler() to start the crawler process.";
    activeGraph->startWebCrawler(
                startUrl,
                urlPatternsIncluded,
                urlPatternsExcluded,
                linkClasses,
                maxNodes,
                maxLinksPerPage,
                intLinks,
                childLinks,
                parentLinks,
                selfLinks,
                extLinks,
                extLinksCrawl,
                socialLinks,
                delayedRequests) ;

}




/**
 * @brief Makes a network request to the given url
 *
 * Creates the QNetworkReply object to handle the reply.
 *
 * @param url
 * @param requestType
 */
void MainWindow::slotNetworkManagerRequest(const QUrl &url, const NetworkRequestType &requestType) {

    qDebug() << "New network request for url:" << url.toString() << "requestType:"<< requestType;

    // Create a network request object
    QNetworkRequest request;

    // Set request url
    request.setUrl(url);

    // Set request headers
    request.setRawHeader(
                "User-Agent",
                "SocNetV harmless spider - see https://socnetv.org");

    // Create a network reply object through which we will make the call and handle the reply content
    qDebug() << "Creating a network reply object and making the call...";
    QNetworkReply *reply = networkManager->get(request) ;

    // Connect signals and slots
    switch (requestType) {
    case NetworkRequestType::Crawler:
        // Wire the reply to the activeGraph, which in turn will pass it to the web crawler
         connect(reply, &QNetworkReply::finished, activeGraph, &Graph::slotHandleCrawlerRequestReply);
        break;
    case NetworkRequestType::CheckUpdate:
         connect(reply, &QNetworkReply::finished, this, &MainWindow::slotHelpCheckUpdateParse);
    default:
        break;
    }


    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(reply, &QNetworkReply::errorOccurred,
             this, &MainWindow::slotNetworkManagerReplyError);
    #endif


}



/**
 * @brief Shows a message box to the user when a NetworkReply encounters errors.
 *
 * The message box contains info about the error code.
 *
 * @param code
 */
void MainWindow::slotNetworkManagerReplyError(const QNetworkReply::NetworkError &code) {

    // Get network reply from the sender
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    // Get reply error string
    QString replyErrorMsg =  reply->errorString();

    // Will store the Qt description of the error
    QString errorMsg;

    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    switch (code) {
    case QNetworkReply::NoError:
        errorMsg="No error message!";
        break;
    case QNetworkReply::ConnectionRefusedError:
        errorMsg="the remote server refused the connection (the server is not accepting requests)";
        break;
    case QNetworkReply::RemoteHostClosedError:
        errorMsg="the remote server closed the connection prematurely, before the entire reply was received and processed";
        break;
    case QNetworkReply::HostNotFoundError:
        errorMsg="the remote host name was not found (invalid hostname)";
        break;
    case QNetworkReply::TimeoutError:
        errorMsg="the connection to the remote server timed out";
        break;
    case QNetworkReply::OperationCanceledError:
        errorMsg="the operation was canceled via calls to abort() or close() before it was finished.";
        break;
    case QNetworkReply::SslHandshakeFailedError:
        errorMsg="the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted.";
        break;
    case QNetworkReply::TemporaryNetworkFailureError:
        errorMsg="the connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established.";
        break;
    case QNetworkReply::NetworkSessionFailedError:
        errorMsg="the connection was broken due to disconnection from the network or failure to start the network.";
        break;
    case QNetworkReply::BackgroundRequestNotAllowedError:
        errorMsg="the background request is not currently allowed due to platform policy.";
        break;
    case QNetworkReply::TooManyRedirectsError:
        errorMsg="while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set by QNetworkRequest::setMaxRedirectsAllowed(). (This value was introduced in 5.6.)";
        break;
    case QNetworkReply::InsecureRedirectError:
        errorMsg="while following redirects, the network access API detected a redirect from a encrypted protocol (https) to an unencrypted one (http). (This value was introduced in 5.6.)";
        break;
    case QNetworkReply::ProxyConnectionRefusedError:
        errorMsg="the connection to the proxy server was refused (the proxy server is not accepting requests)";
        break;
    case QNetworkReply::ProxyConnectionClosedError:
        errorMsg="the proxy server closed the connection prematurely, before the entire reply was received and processed";
        break;
    case QNetworkReply::ProxyNotFoundError:
        errorMsg="the proxy host name was not found (invalid proxy hostname)";
        break;
    case QNetworkReply::ProxyTimeoutError:
        errorMsg="the connection to the proxy timed out or the proxy did not reply in time to the request sent";
        break;
    case QNetworkReply::ProxyAuthenticationRequiredError:
        errorMsg="the proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)";
        break;
    case QNetworkReply::ContentAccessDenied:
        errorMsg="the access to the remote content was denied (similar to HTTP error 403)";
        break;
    case QNetworkReply::ContentOperationNotPermittedError:
        errorMsg="the operation requested on the remote content is not permitted";
        break;
    case QNetworkReply::ContentNotFoundError:
        errorMsg="the remote content was not found at the server (similar to HTTP error 404)";
        break;
    case QNetworkReply::AuthenticationRequiredError:
        errorMsg="the remote server requires authentication to serve the content but the credentials provided were not accepted (if any)";
        break;
    case QNetworkReply::ContentReSendError:
        errorMsg="the request needed to be sent again, but this failed for example because the upload data could not be read a second time.";
        break;
    case QNetworkReply::ContentConflictError:
        errorMsg="the request could not be completed due to a conflict with the current state of the resource.";
        break;
    case QNetworkReply::ContentGoneError:
        errorMsg="the requested resource is no longer available at the server.";
        break;
    case QNetworkReply::InternalServerError:
        errorMsg="the server encountered an unexpected condition which prevented it from fulfilling the request.";
        break;
    case QNetworkReply::OperationNotImplementedError:
        errorMsg="the server does not support the functionality required to fulfill the request.";
        break;
    case QNetworkReply::ServiceUnavailableError:
        errorMsg="the server is unable to handle the request at this time.";
        break;
    case QNetworkReply::ProtocolUnknownError:
        errorMsg="the Network Access API cannot honor the request because the protocol is not known";
        break;
    case QNetworkReply::ProtocolInvalidOperationError:
        errorMsg="the requested operation is invalid for this protocol";
        break;
    case QNetworkReply::UnknownNetworkError:
        errorMsg="an unknown network-related error was detected";
        break;
    case QNetworkReply::UnknownProxyError:
        errorMsg="an unknown proxy-related error was detected";
        break;
    case QNetworkReply::UnknownContentError:
        errorMsg="an unknown error related to the remote content was detected";
        break;
    case QNetworkReply::ProtocolFailure:
        errorMsg="a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)";
        break;
    case QNetworkReply::UnknownServerError:
        errorMsg="an unknown error related to the server response was detected";
        break;
    }
    #endif

    slotHelpMessageToUserError("Network Error!  \n\n"
                               "Request to: '" + reply->request().url().toString() + "' encountered this error: \n\n" +
                               replyErrorMsg + "\n\n" +
                               "Error description: \n\n" + errorMsg +
                               "\n\nPlease, try again. " );


}


/**
 * @brief Shows a message box to the user when the Network Manager encounters any SSL error.
 *
 * @param reply
 * @param errors
 */
void MainWindow::slotNetworkManagerSslErrors(QNetworkReply *reply, const QList<QSslError> &errors) {

    QString sslErrorString;

    // Read errors and get the error decriptions.
    foreach(QSslError error, errors) {
        sslErrorString = error.errorString();
    }

    // Show the user a message box
    slotHelpMessageToUserError("SSL Error! \n\n"
                               "Request to: '" + reply->request().url().toString() + "' encountered this SSL error: \n\n"
                               + sslErrorString +
                               "\n\n Please, try again later. ");

}




/**
 * @brief Refreshes LCD values and toggles the networkSave icon, when the network has been modified.
 *
 * @param directed
 * @param vertices
 * @param edges
 * @param density
 * @param needsSaving
 */
void MainWindow::slotNetworkChanged(const bool &directed,
                                    const int &vertices,
                                    const int &edges,
                                    const qreal &density, const bool &needsSaving){

    qDebug()<<"Got signal that network changed. Updating mainwindow UI (LCDs, save icon, etc). Params: "
           << "directed" << directed
           << "vertices" << vertices
           << "edges" << edges
           << "density"<< density
           << "needsSaving" << needsSaving;


    if ( needsSaving ) {
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);
    }
    else {
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
        networkSaveAct->setEnabled(false);
    }

    rightPanelNodesLCD->setText (QString::number(vertices));
    if ( !directed ) {

        rightPanelEdgesLCD->setStatusTip(tr("Shows the total number of undirected edges in the network."));
        rightPanelEdgesLCD->setToolTip(tr("The total number of undirected edges in the network."));
        rightPanelNetworkTypeLCD->setStatusTip(tr("Undirected data mode. Toggle the menu option Edit->Edges->Undirected Edges to change it"));
        rightPanelNetworkTypeLCD->setToolTip(tr("The loaded network, if any, is undirected and \n"
                                                "any edge you add between nodes will be undirected.\n"
                                                "If you want to work with directed edges and/or \n"
                                                "transform the loaded network (if any) to directed \n"
                                                "disable the option Edit->Edges->Undirected \n"
                                                "or press CTRL+E+U"));
        rightPanelNetworkTypeLCD->setWhatsThis(tr("The loaded network, if any, is undirected and \n"
                                                  "any edge you add between nodes will be undirected.\n"
                                                  "If you want to work with directed edges and/or \n"
                                                  "transform the loaded network (if any) to directed \n"
                                                  "disable the option Edit->Edges->Undirected"));


        if (toolBoxEditEdgeModeSelect->currentIndex()==0) {
            toolBoxEditEdgeModeSelect->setCurrentIndex(1);
        }
        rightPanelNetworkTypeLCD->setText ("Undirected");

        rightPanelEdgesLabel->setText(tr("Edges:"));

        rightPanelSelectedEdgesLabel->setText( tr("Edges:"));
        editEdgeUndirectedAllAct->setChecked(true);
    }
    else {
        rightPanelEdgesLCD->setStatusTip(tr("Shows the total number of directed edges in the network."));
        rightPanelEdgesLCD->setToolTip(tr("The total number of directed edges in the network."));
        rightPanelNetworkTypeLCD->setStatusTip(tr("Directed data mode. Toggle the menu option Edit->Edges->Undirected Edges to change it"));
        rightPanelNetworkTypeLCD->setToolTip(tr("The loaded network, if any, is directed and \n"
                                                "any link you add between nodes will be a directed arc.\n"
                                                "If you want to work with undirected edges and/or \n"
                                                "transform the loaded network (if any) to undirected \n"
                                                "enable the option Edit->Edges->Undirected"));
        rightPanelNetworkTypeLCD->setWhatsThis(tr("The loaded network, if any, is directed and \n"
                                                  "any link you add between nodes will be a directed arc.\n"
                                                  "If you want to work with undirected edges and/or \n"
                                                  "transform the loaded network (if any) to undirected \n"
                                                  "enable the option Edit->Edges->Undirected"));

        rightPanelNetworkTypeLCD->setText ("Directed");
        if (toolBoxEditEdgeModeSelect->currentIndex()==1) {
            toolBoxEditEdgeModeSelect->setCurrentIndex(0);
        }
        rightPanelEdgesLabel->setText(tr("Arcs:"));

        rightPanelSelectedEdgesLabel->setText( tr("Arcs:")  );
        editEdgeUndirectedAllAct->setChecked(false);
    }
    rightPanelEdgesLCD->setText(QString::number(edges));
    rightPanelDensityLCD->setText(QString::number(density, 'f', 3));

    qDebug()<<"Finished updating mainwindow.";
}





/**
 * @brief Popups a context menu with options when the user right-clicks on the canvas
 *
 * @param mPos
 */
void MainWindow::slotEditOpenContextMenu( const QPointF &mPos) {
    Q_UNUSED(mPos);
    QMenu *contextMenu = new QMenu(" Menu",this);
    Q_CHECK_PTR( contextMenu );  //displays "out of memory" if needed

    int nodesSelected = activeGraph->getSelectedVerticesCount();

    contextMenu->addAction( "## Selected nodes: "
                              + QString::number(  nodesSelected ) + " ##  ");

    contextMenu->addSeparator();

    if (nodesSelected > 0) {
        contextMenu->addAction(editNodePropertiesAct );
        contextMenu->addSeparator();
        contextMenu->addAction(editNodeRemoveAct );
        if (nodesSelected > 1 ){
            editNodeRemoveAct->setText(tr("Remove ")
                                       + QString::number(nodesSelected)
                                       + tr(" nodes"));
            contextMenu->addSeparator();
            contextMenu->addAction(editNodeSelectedToCliqueAct);
            contextMenu->addAction(editNodeSelectedToStarAct);
            contextMenu->addAction(editNodeSelectedToCycleAct);
            contextMenu->addAction(editNodeSelectedToLineAct);

        }
        else {
            editNodeRemoveAct->setText(tr("Remove ")
                                       + QString::number(nodesSelected)
                                       + tr(" node"));
        }
        contextMenu->addSeparator();
    }

    contextMenu->addAction( editNodeAddAct );
    contextMenu->addSeparator();
    contextMenu->addAction( editEdgeAddAct );
    contextMenu->addSeparator();

    QMenu *options=new QMenu("Options", this);
    contextMenu->addMenu(options );

    options->addAction (openSettingsAct  );
    options->addSeparator();
    options->addAction (editNodeSizeAllAct );
    options->addAction (editNodeShapeAll  );
    options->addAction (editNodeColorAll );
    options->addAction (optionsNodeNumbersVisibilityAct);
    options->addAction (optionsNodeLabelsVisibilityAct);
    options->addSeparator();
    options->addAction (editEdgeColorAllAct  );
    options->addSeparator();
    options->addAction (changeBackColorAct  );
    options->addAction (backgroundImageAct  );

    //QCursor::pos() is good only for menus not related with node coordinates
    contextMenu->exec(QCursor::pos() );
    delete  contextMenu;
}






/**
 * @brief Selects all nodes
 */
void MainWindow::slotEditNodeSelectAll(){
    qDebug() << "Request to select all nodes...";
    graphicsWidget->selectAll();
    statusMessage( tr("Selected nodes: %1")
                   .arg( activeGraph->getSelectedVerticesCount()  ) );

}


/**
 * @brief Selects no nodes.
 */
void MainWindow::slotEditNodeSelectNone(){
    qDebug() << "Clearing node selection...";
    graphicsWidget->selectNone();
    statusMessage( QString(tr("Selection cleared") ) );
}



/**
 * @brief Automatically runs, when the user moves a node on the canvas, to
 * update new vertex coordinates in Graph, and show a status message.
 *
 * Called from GraphicsWidget
 *
 * @param nodeNumber
 * @param x
 * @param y
 */
void MainWindow::slotEditNodePosition(const int &nodeNumber,
                                      const int &x, const int &y){
    qDebug("Updating position for node %i - x: %i, y: %i", nodeNumber, x, y);
    activeGraph->vertexPosSet(nodeNumber, x, y);
}


/**
 * @brief Adds a new random node
 *
 * Called when the "Add Node" btn is clicked
 */
void MainWindow::slotEditNodeAdd() {
    qDebug() << "Request to add a new random node...";
    activeGraph->vertexCreateAtPosRandom(true);
    statusMessage( tr("New random positioned node (numbered %1) added.")
                   .arg(activeGraph->vertexNumberMax())  );
}



/**
 * @brief Opens the Find Node dialog
 */
void MainWindow::slotEditNodeFindDialog(){
    qDebug() << "Showing find node dialog...";
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    //@TODO - prominenceIndexList should be either
    // the list of all computes indices
    // or the last computed indice
    // or empty if the user has not computed any index yet.
    m_nodeFindDialog = new DialogNodeFind(this, prominenceIndexList) ;

    connect( m_nodeFindDialog, &DialogNodeFind::userChoices,
             this, &MainWindow::slotEditNodeFind);

    m_nodeFindDialog->exec();

    statusMessage( tr("Node find dialog opened. Enter your choices. ") );

    return;


}


/**
 * @brief Finds one or more nodes, according to their number, label or centrality score.
 *
 * @param list
 * @param searchType
 * @param indexStr
 */
void MainWindow::slotEditNodeFind(const QStringList &nodeList,
                                  const QString &searchType,
                                  const QString &indexStr)
{

    qDebug() << "Request to find nodes:" << nodeList
             << "search type:"<< searchType
             << "indexStr"<<indexStr;

    int indexType = 0;

    if (searchType == "numbers"){
        activeGraph->vertexFindByNumber(nodeList);
    }
    else if (searchType == "labels"){
        activeGraph->vertexFindByLabel(nodeList);
    }
    else if (searchType == "score"){

        indexType = activeGraph->getProminenceIndexByName(indexStr);

        activeGraph->vertexFindByIndexScore(indexType,
                                            nodeList,
                                            optionsEdgeWeightConsiderAct->isChecked(),
                                            inverseWeights,
                                            editFilterNodesIsolatesAct->isChecked() );

    }

    return;
}





/**
 * @brief Handles requests to delete a node and the attached objects (edges, etc).
 *
 * If the user has clicked on a node, it deletes it
 * Else it asks for a nodeNumber to remove.
 */
void MainWindow::slotEditNodeRemove() {
    qDebug() << "Request to remove a node...";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->relations() > 1){
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. Cannot remove node!"),
                              tr("Error. Cannot remove this node!"),
                              tr("This a network with more than 1 relations. If you remove "
                                 "a node from the active relation, and then ask me to go "
                                 "to the previous or the next relation, then I would crash "
                                 "because I would try to display edges from a deleted node."
                                 "You cannot remove nodes in multirelational networks.")
                              );
        return;
    }

    // if there are already multiple nodes selected, erase them
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if ( nodesSelected > 0) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "multiple nodes selected to be removed";
        foreach (int nodeNumber, activeGraph->getSelectedVertices() ) {
            activeGraph->vertexRemove(nodeNumber);
        }
        editNodeRemoveAct->setText(tr("Remove Node"));
        statusMessage( tr("Removed %1 nodes.").arg(nodesSelected) );
        QApplication::restoreOverrideCursor();
    }

    else {
        int nodeNumber=-1, min=-1, max=-1;
        bool ok=false;
        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        if (min==-1 || max==-1 ) {
            qDebug("ERROR in finding min max nodeNumbers. Abort");
            return;
        }
        else  {
            nodeNumber =  QInputDialog::getInt(
                        this,
                        tr("Remove node"),
                        tr("Choose a node to remove between ("
                           + QString::number(min).toLatin1()+"..."+
                           QString::number(max).toLatin1()+"):"),min, 1, max, 1, &ok);
            if (!ok) {
                statusMessage( "Remove node operation cancelled." );
                return;
            }
        }
        qDebug ("removing vertex with number %i from Graph", nodeNumber);
        activeGraph->vertexRemove(nodeNumber);
        qDebug("Completed. Node %i removed completely.",nodeNumber);
        statusMessage( tr("Node removed completely.") );
    }
}


/**
 * @brief Opens the Node Properties dialog for the selected nodes.
 * If no nodes are selected, prompts the user for a node number.
 */
void MainWindow::slotEditNodePropertiesDialog()
{
    qDebug() << "Request to open the node properties dialog...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int min = -1, max = -1, size = appSettings["initNodeSize"].toInt(nullptr, 10);
    int nodeNumber = 0;
    int selectedNodesCount = activeGraph->getSelectedVerticesCount();
    QColor color = QColor(appSettings["initNodeColor"]);
    QString shape = appSettings["initNodeShape"];
    QString iconPath = QString();
    QString label = "";
    bool ok = false;
    QHash<QString, QString> customAttributes = QHash<QString, QString>();

    if (selectedNodesCount == 0)
    {

        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        qDebug() << "no node selected"
                 << "min node number " << min
                 << "max node number " << max
                 << "opening inputdialog";

        if (min == -1 || max == -1)
        {
            qDebug("ERROR in finding min max nodeNumbers. Abort");
            return;
        }

        nodeNumber = QInputDialog::getInt(
            this,
            "Node Properties",
            tr("Choose a node between (" + QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "):"), min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Node properties cancelled.");
            return;
        }
        label = activeGraph->vertexLabel(nodeNumber);
        color = activeGraph->vertexColor(nodeNumber);
        shape = activeGraph->vertexShape(nodeNumber);
        size = activeGraph->vertexSize(nodeNumber);
        iconPath = activeGraph->vertexShapeIconPath(nodeNumber);
        customAttributes=activeGraph->vertexCustomAttributes(nodeNumber);
    }
    else
    {
        qDebug() << "selectedNodesCount" << selectedNodesCount;

        foreach (const int &nodeNumber, activeGraph->getSelectedVertices())
        {
            qDebug() << "reading properties of selected node"<< nodeNumber;

            if (selectedNodesCount > 1)
            {
                color = activeGraph->vertexColor(nodeNumber);
                shape = activeGraph->vertexShape(nodeNumber);
                iconPath = activeGraph->vertexShapeIconPath(nodeNumber);
                size = activeGraph->vertexSize(nodeNumber);
            }
            else
            {
                label = activeGraph->vertexLabel(nodeNumber);
                color = activeGraph->vertexColor(nodeNumber);
                shape = activeGraph->vertexShape(nodeNumber);
                iconPath = activeGraph->vertexShapeIconPath(nodeNumber);
                size = activeGraph->vertexSize(nodeNumber);
                customAttributes=activeGraph->vertexCustomAttributes(nodeNumber);
            }
        }
    }

    // @todo Add a function to group multiple nodes' properties changes together
    // This function should allow setting properties like color, size, and shape for multiple nodes at once.

    qDebug() << "opening DialogNodeEdit."
             << "label" << label
             << "size" << size
             << "color" << color
             << "shape" << shape
             << "iconPath" << iconPath
             << "customAttributes" << customAttributes;

    std::unique_ptr<DialogNodeEdit> m_nodeEditDialog = std::make_unique<DialogNodeEdit>(this,
                                                                                        nodeShapeList,
                                                                                        iconPathList,
                                                                                        label,
                                                                                        size,
                                                                                        color,
                                                                                        shape,
                                                                                        iconPath,
                                                                                        customAttributes);
                                                                                       
    connect(m_nodeEditDialog.get(), &DialogNodeEdit::userChoices,
            this, &MainWindow::slotEditNodeProperties);

    m_nodeEditDialog->exec();
}


/**
 * @brief Applies the selected properties to one or multiple nodes in the graph.
 *
 * This slot updates the properties of the selected nodes or a single node, as
 * specified by the user in DialogNodeEdit. It updates the label, size, color, shape, and custom
 * attributes of the nodes.
 *
 * @param label The new label for the node(s).
 * @param size The new size for the node(s).
 * @param color The new color for the node(s).
 * @param shape The new shape for the node(s).
 * @param iconPath The path to the icon for the node(s).
 * @param customAttributes A hash of custom attributes to set for the node(s).
 */
void MainWindow::slotEditNodeProperties(const QString &label,
                                        const int &size,
                                        const QColor &color,
                                        const QString &shape,
                                        const QString &iconPath,
                                        const QHash<QString, QString> &customAttributes) {

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    qDebug()<< "Request to update node properties - new properties: "
            << " label " << label
            << " size " << size
            << " color " << color
            << " shape " << shape
            << " vertexClicked " <<activeGraph->vertexClicked()
            << " selectedNodesCount " << selectedNodesCount
            << "customAttributes" << customAttributes;

    if ( selectedNodesCount == 0 && activeGraph->vertexClicked() != 0) {
        // no node selected but user entered a node number in a dialog
        if ( label !="" && appSettings["initNodeLabelsVisibility"] != "true")
            slotOptionsNodeLabelsVisibility(true);
        activeGraph->vertexLabelSet( activeGraph->vertexClicked(), label );
        activeGraph->vertexColorSet( activeGraph->vertexClicked(), color.name());
        activeGraph->vertexSizeSet( activeGraph->vertexClicked(), size);
        activeGraph->vertexShapeSet( activeGraph->vertexClicked(), shape, iconPath );
        activeGraph->vertexCustomAttributesSet( activeGraph->vertexClicked(), customAttributes);

        statusMessage( tr("Updated the properties of node %1. ").arg(activeGraph->vertexClicked()));

    }
    else {
        //some nodes are selected
        int nodeNumber = 0;
        foreach (nodeNumber, activeGraph->getSelectedVertices() ) {
            qDebug()<< "node " << nodeNumber;
            if ( !label.isEmpty() ) {
                if ( selectedNodesCount > 1 )
                {
                    activeGraph->vertexLabelSet(
                                nodeNumber,
                                label + QString::number(nodeNumber)
                                );
                }
                else {
                    activeGraph->vertexLabelSet( nodeNumber, label );
                }
                // turn on labels visibility if they are hidden
                if ( appSettings["initNodeLabelsVisibility"] != "true") {
                    slotOptionsNodeLabelsVisibility(true);
                }
            }
            activeGraph->vertexColorSet( nodeNumber, color.name());
            activeGraph->vertexSizeSet(nodeNumber,size);
            activeGraph->vertexShapeSet( nodeNumber, shape, iconPath);
            activeGraph->vertexCustomAttributesSet( nodeNumber, customAttributes);
        }
        statusMessage( tr("Updated the properties of %1 nodes. ").arg(selectedNodesCount));
    }

}



/**
 * @brief Creates a complete subgraph (clique) from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToClique () {
    qDebug() << "MW::slotEditNodeSelectedToClique()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if ( selectedNodesCount < 3 ) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Error. Not enough nodes selected."),
                              tr("Cannot create new clique because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first.")
                              );
        return;
    }

    activeGraph->verticesCreateSubgraph(QList<int> (), SUBGRAPH_CLIQUE);

    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Clique created."),
                          tr("A new clique has been created from ") + QString::number(selectedNodesCount)
                          + tr(" nodes")
                          );

}



/**
 * @brief Creates a star subgraph from selected nodes.
 * User must choose a central node.
 */
void MainWindow::slotEditNodeSelectedToStar() {
    qDebug() << "MW::slotEditNodeSelectedToStar()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if ( selectedNodesCount < 3 ) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new star subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first.")
                              );
        return;
    }

    int center;
    bool ok=false;

    int min = activeGraph->getSelectedVerticesMin();
    int max = activeGraph->getSelectedVerticesMax();
    center=QInputDialog::getInt(
                this,
                "Create star subgraph",
                tr("To create a star subgraph from selected nodes, \n"
                   "enter the number of the central actor ("
                   +QString::number(min).toLatin1()+"..."
                   +QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
    if (!ok) {
        statusMessage( "Create star subgraph cancelled." );
        return;
    }

    activeGraph->verticesCreateSubgraph(QList<int> (), SUBGRAPH_STAR,center);

    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Star subgraph created."),
                          tr("A new star subgraph has been created with ") +
                          QString::number( selectedNodesCount )
                          + tr(" nodes.")
                          );

}



/**
 * @brief Creates a cycle subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToCycle() {
    qDebug() << "MW::slotEditNodeSelectedToCycle()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if ( selectedNodesCount < 3 ) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new cycle subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first.")
                              );
        return;
    }

    activeGraph->verticesCreateSubgraph(QList<int> (),SUBGRAPH_CYCLE);

    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Cycle subgraph created."),
                          tr("A new cycle subgraph has been created with ")
                          + QString::number( selectedNodesCount )
                          + tr(" select nodes.")
                          );

}



/**
 * @brief Creates a line subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToLine() {
    qDebug() << "MW::slotEditNodeSelectedToLine()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if ( selectedNodesCount < 3 ) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new line subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first.")
                              );
        return;
    }

    activeGraph->verticesCreateSubgraph(QList<int> (),SUBGRAPH_LINE);

    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Line subgraph created."),
                          tr("A new line subgraph has been created with ")
                          + QString::number( selectedNodesCount )
                          + tr(" selected nodes.")
                          );

}



/**
 * @brief Changes the color of all nodes to parameter color
 *
 * If the color is invalid, opens a QColorDialog to
 * select a new node color for all nodes.
 *
 * @param color
 */
void MainWindow::slotEditNodeColorAll(QColor color){
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeColor"] ),
                this,
                "Change the color of all nodes" );
    }
    if (color.isValid()) {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        qDebug() << "MW::slotEditNodeColorAll() : "
                 << appSettings["initNodeColor"];
        activeGraph->vertexColorSet(0, appSettings["initNodeColor"]);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Change all nodes' color. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }
}




/**
 * @brief Changes the size of nodes to newSize.
 *
 * If newSize = 0 asks the user a new size for all nodes
 * If normalized = true, changes node sizes according to their amount
 *
 * @param newSize
 * @param normalized
 */
void MainWindow::slotEditNodeSizeAll(int newSize, const bool &normalized) {
    Q_UNUSED(normalized);
    qDebug() << "MW: slotEditNodeSizeAll() - "
              << " newSize " << newSize ;
    if ( newSize == 0 && !normalized ) {
        bool ok=true;
        newSize = QInputDialog::getInt(
                    this,
                    "Change node size",
                    tr("Select new size for all nodes:"),
                    appSettings["initNodeSize"].toInt(0, 10), 1, 100, 1, &ok );

        if (!ok) {
            statusMessage( "Change node size operation cancelled." );
            return;
        }
    }

    appSettings["initNodeSize"]= QString::number(newSize);

    activeGraph->vertexSizeSet(0, newSize);

    statusMessage(tr("Ready"));
    return;
}






/**
 * @brief Change the shape of a node or all nodes.
 * If shape == null, prompts the user a list of available node shapes to select.
 * Then changes the shape of all nodes/vertices accordingly.
 * If vertex is non-zero, changes the shape of that node only.
 * Called when user clicks on Edit->Node > Change all nodes shapes
 * Called from DialogSettings when the user has selected a new default node shape
  * @param shape
 * @param vertex
 */
void MainWindow::slotEditNodeShape(const int &vertex, QString shape,
                                   QString nodeIconPath) {
    qDebug() << "MW::slotEditNodeShape() - vertex " << vertex
             << "(0 means all)"
             <<"new shape" << shape
            << "nodeIconPath"<<nodeIconPath;

    if ( shape.isNull() ) {

        bool ok=false;

        int curShapeIndex = nodeShapeList.indexOf(appSettings["initNodeShape"]);

        if ( curShapeIndex == -1 ) {
            curShapeIndex=1;
        }
        shape = QInputDialog::getItem(this,
                                      "Node shape",
                                      "Select a shape for all nodes: ",
                                      nodeShapeList, curShapeIndex, true, &ok);
        if ( !ok ) {
            //user pressed Cancel
            statusMessage(tr("Change node shapes aborted."));
            return;
        }
        if (shape=="custom") {
            nodeIconPath = QFileDialog::getOpenFileName(
                        this, tr("Select an icon"), getLastPath(),
                        tr("Images (*.png *.jpg *.jpeg *.svg);;All (*.*)")
                        );
            if (nodeIconPath.isNull() ) {
                //user pressed Cancel
                statusMessage(tr("Change node shapes aborted."));
                return;
            }
        }
        else {
            nodeIconPath = iconPathList [ nodeShapeList.indexOf(shape) ];
        }
    }

    if (vertex == 0) { //change all nodes shapes
        activeGraph->vertexShapeSet(-1, shape, nodeIconPath);
        appSettings["initNodeShape"] = shape;
        appSettings["initNodeIconPath"] = nodeIconPath;
        statusMessage(tr("All shapes have been changed."));
    }
    else { //only one
        activeGraph->vertexShapeSet( vertex, shape, nodeIconPath);
        statusMessage(tr("Node shape has been changed."));
    }
}




/**
 * @brief Changes the size of one or all node numbers.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node number font size
 * if v1=0, it changes all node numbers
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeNumberSize(int v1, int newSize, const bool prompt) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeNumberSize - newSize " << newSize;
    if (prompt) {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node numbers size to: (1-16)"),
                                       appSettings["initNodeNumberSize"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change font size: Aborted.") );
            return;
        }
    }
    if (v1) { //change one node number only
        activeGraph->vertexNumberSizeSet(v1, newSize);
    }
    else { //change all
        appSettings["initNodeNumberSize"] = QString::number(newSize);
        activeGraph->vertexNumberSizeSet(0, newSize);
    }
    statusMessage( tr("Changed node numbers size.") );
}




/**
 * @brief Changes the text color of all node numbers
 * Called from Edit menu option and Settings dialog.
 * If color is invalid, asks the user to enter a new node number color
 * @param color
 */
void MainWindow::slotEditNodeNumbersColor(const int &v1, QColor color){
    qDebug() << "MW:slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeNumberColor"] ),
                this,
                "Change the color of all node numbers" );
    }

    if (color.isValid()) {

        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        if (v1) {
            activeGraph->vertexNumberColorSet(v1, color.name());

        }
        else {
            appSettings["initNodeNumberColor"] = color.name();
            activeGraph->vertexNumberColorSet(0, color.name());
        }

        QApplication::restoreOverrideCursor();
        statusMessage( tr("Node number color changed. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }

}


/**
 * @brief Changes the distance of one or all node numbers from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node number distance
 * if v1=0, it changes all node number distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeNumberDistance(int v1, int newDistance) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeNumberDistance - newSize " << newDistance;
    if (!newDistance) {
        newDistance = QInputDialog::getInt(
                    this, "Change node number distance",
                    tr("Change all node numbers distance from their nodes to: (1-16)"),
                    appSettings["initNodeNumberDistance"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change node number distance aborted.") );
            return;
        }
    }
    if (v1) { //change one node number distance only
        activeGraph->vertexNumberDistanceSet(v1, newDistance);
    }
    else { //change all
        appSettings["initNodeNumberDistance"] = QString::number(newDistance);
        activeGraph->vertexNumberDistanceSet(0, newDistance);
    }
    statusMessage( tr("Changed node number distance.") );
}



/**
 * @brief Changes the size of one or all node Labels.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node Label font size
 * if v1=0, it changes all node Labels
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeLabelSize(const int v1, int newSize) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeLabelSize - newSize " << newSize;
    if (!newSize) {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node labels text size to: (1-16)"),
                                       appSettings["initNodeLabelSize"].toInt(0,10), 1, 32, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change font size: Aborted.") );
            return;
        }
    }
    if (v1) { //change one node Label only
        activeGraph->vertexLabelSizeSet(v1, newSize);
    }
    else { //change all
        appSettings["initNodeLabelSize"] = QString::number(newSize);
        activeGraph->vertexLabelSizeSet(0, newSize);
    }
    statusMessage( tr("Changed node label size.") );
}







/**
 * @brief Changes the color of all node labels.
 * Asks the user to enter a new node label color
 */
void MainWindow::slotEditNodeLabelsColor(QColor color){
    qDebug() << "MW::slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initNodeLabelColor"] ),
                this,
                "Change the color of all node labels" );
    }
    if (color.isValid()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        activeGraph->vertexLabelColorSet(0, color.name());
        appSettings["initNodeLabelColor"] = color.name();
        optionsNodeLabelsVisibilityAct->setChecked(true);
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Label colors changed. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }
}




/**
 * @brief MainWindow::slotEditNodeLabelDistance
 * Changes the distance of one or all node label from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node label distance
 * if v1=0, it changes all node label distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeLabelDistance(int v1, int newDistance) {
    bool ok=false;
    qDebug() << "MW::slotEditNodeLabelDistance - newSize " << newDistance;
    if (!newDistance) {
        newDistance = QInputDialog::getInt(
                    this, "Change node label distance",
                    tr("Change all node labels distance from their nodes to: (1-16)"),
                    appSettings["initNodeLabelDistance"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change node label distance aborted.") );
            return;
        }
    }
    if (v1) { //change one node label distance only
        activeGraph->vertexLabelDistanceSet(v1, newDistance);
    }
    else { //change all
        appSettings["initNodeLabelDistance"] = QString::number(newDistance);
        activeGraph->vertexLabelDistanceAllSet(newDistance);
    }
    statusMessage( tr("Changed node label distance.") );
}



/**
 * @brief MainWindow::slotEditNodeOpenContextMenu
 * Called from GW when the user has right-clicked on a node
 * Opens a node context menu with some options when the user right-clicks on a node
 */
void MainWindow::slotEditNodeOpenContextMenu() {

    qDebug("MW: slotEditNodeOpenContextMenu() for node %i at %i, %i",
           activeGraph->vertexClicked(), QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number( activeGraph->vertexClicked() ), this);
    Q_CHECK_PTR( nodeContextMenu );  //displays "out of memory" if needed
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if ( nodesSelected == 1) {
        nodeContextMenu->addAction(
                    tr("## NODE ") + QString::number(activeGraph->vertexClicked()) + " ##  "
                    );
    }
    else {
        nodeContextMenu->addAction(
                    tr("## NODE ") + QString::number(activeGraph->vertexClicked())
                    + " ##  " + tr(" (selected nodes: ")
                    + QString::number ( nodesSelected ) + ")");
    }

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editNodePropertiesAct );

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editEdgeAddAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editNodeRemoveAct );


    nodeContextMenu->addSeparator();


    //QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu->exec(QCursor::pos() );
    delete  nodeContextMenu;

}



/**
 * @brief Updates the UI (LCDs and Actions) after a change in the user-selected nodes/edges
 *
 * @param nodes
 * @param edges
 */
void MainWindow::slotEditSelectionChanged(const int &selNodes, const int &selEdges) {
    qDebug()<< "Updating UI for new selection";
    rightPanelSelectedNodesLCD->setText(QString::number(selNodes));
    rightPanelSelectedEdgesLCD->setText(QString::number(selEdges));

    if (selNodes > 1){
        editNodeRemoveAct->setText(tr("Remove ")
                                   + QString::number(selNodes)
                                   + tr(" nodes"));
        editNodeSelectedToCliqueAct->setEnabled(true);
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from ")
                                             + QString::number(selNodes)
                                             + tr(" selected nodes"));
        editNodeSelectedToStarAct->setEnabled(true);
        editNodeSelectedToStarAct->setText(tr("Create a star from ")
                                           + QString::number(selNodes)
                                           + tr(" selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(true);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from ")
                                            + QString::number(selNodes)
                                            + tr(" selected nodes"));
        editNodeSelectedToLineAct->setEnabled(true);
        editNodeSelectedToLineAct->setText(tr("Create a line from ")
                                           + QString::number(selNodes)
                                           + tr(" selected nodes"));
    }
    else {
        editNodeRemoveAct->setText(tr("Remove Node"));
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from selected nodes"));
        editNodeSelectedToCliqueAct->setEnabled(false);
        editNodeSelectedToStarAct->setText(tr("Create a star from selected nodes"));
        editNodeSelectedToStarAct->setEnabled(false);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(false);
        editNodeSelectedToLineAct->setText(tr("Create a line from selected nodes"));
        editNodeSelectedToLineAct->setEnabled(false);

    }

    //
    // NOTE:
    // DO NOT display a message on the status bar on high frequently called functions like this
    //

}



/**
 * @brief Displays information about the given node on the statusbar.
 *
 * Usually called by Graph, after the user clicks on a node.
 *
 * @param number
 * @param p
 * @param label
 * @param inDegree
 * @param outDegree
 */
void MainWindow::slotEditNodeInfoStatusBar (const int &number,
                                            const QPointF &p,
                                            const QString &label,
                                            const int &inDegree,
                                            const int &outDegree) {

    qDebug()<<"Updating node info in status bar...";
    rightPanelClickedNodeLCD->setText (QString::number(number));
    rightPanelClickedNodeInDegreeLCD->setText ( QString::number (inDegree) ) ;
    rightPanelClickedNodeOutDegreeLCD->setText ( QString::number (outDegree) ) ;

    if (number!=0)  {

        statusMessage(  QString(tr("Position (%1, %2):  Node %3, label %4 - "
                                   "In-Degree: %5, Out-Degree: %6"))
                        .arg( ceil( p.x() ) )
                        .arg( ceil( p.y() )).arg( number )
                        .arg( ( label == "") ? "unset" : label )
                        .arg(inDegree).arg(outDegree) );
    }
    else {
        statusMessage( tr("Position (%1,%2): Double-click to create a new node." )
                       .arg(p.x())
                       .arg(p.y())  );
    }
}




/**
 * @brief Displays information about the clicked edge on the statusbar
 *
 * Called by Graph when the user clicks on an edge or when we need to init the LCDs (i.e. clearing the graph).
 *
 * @param edge
 * @param openMenu
 */
void MainWindow::slotEditEdgeClicked (const MyEdge &edge,
                                      const bool &openMenu) {

    int v1 = edge.source;
    int v2 = edge.target;
    qreal weight = edge.weight;
    qreal reverseWeight = edge.rWeight;
    int type = edge.type;

//    qDebug()<<"clicked edge"
//           << v1
//           << "->"
//           << v2
//           << "=" << weight
//           << "type" << type
//           << "openMenu"<<openMenu;


    if (v1 ==0 || v2 == 0) {
        rightPanelClickedEdgeNameLCD->setText("-");
        rightPanelClickedEdgeWeightLCD->setText("-");
        rightPanelClickedEdgeReciprocalWeightLCD->setText("");

        return;
    }

    QString edgeName;

    if ( type == EdgeType::Undirected ) {
        statusMessage(  QString
                        (tr("Undirected edge %1 <--> %2 of weight %3 has been selected. "
                            "Click anywhere else to unselect it."))
                        .arg( v1 ).arg( v2 )
                        .arg( weight )
                        );
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1)+QString(" -- ")+QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setText("");
        rightPanelClickedEdgeReciprocalWeightLCD->setText("");
        if (openMenu) {
            edgeName=QString("EDGE: ") + QString::number(v1)+QString(" -- ")+QString::number(v2);
        }
    }
    else if (type == EdgeType::Reciprocated){
        statusMessage(  QString
                        (tr("Reciprocated edge %1 <--> %2 has been selected. "
                            "Weight %1 --> %2 = %3, "
                            "Weight %2 --> %1 = %4. "
                            "Click anywhere else to unselect it."))
                        .arg( v1 ).arg( v2 )
                        .arg( weight ).arg(reverseWeight)
                        );
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1)+QString(" <-->")+QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setText("Recipr.:");
        rightPanelClickedEdgeReciprocalWeightLCD->setText(QString::number(reverseWeight));
        if (openMenu) {
            edgeName=QString("RECIPROCATED EDGE: ") + QString::number(v1)+QString(" <-->")+QString::number(v2);
        }

    }
    else{
        statusMessage(  QString(tr("Directed edge %1 --> %2 of weight %3 has been selected. "
                                   "Click again to unselect it."))
                        .arg( v1 ).arg( v2 )
                        .arg( weight )
                        );
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1)+QString(" -->")+QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setText("");
        rightPanelClickedEdgeReciprocalWeightLCD->setText("");

        if (openMenu) {
            edgeName=QString("DIRECTED EDGE: ") + QString::number(v1)+QString(" -->")+QString::number(v2);
        }

    }

    if (openMenu) {
        slotEditEdgeOpenContextMenu(edgeName);
    }
}




/**
* @brief Popups a context menu with edge-related options
 * Called when the user right-clicks on an edge
* @param str
*/
void MainWindow::slotEditEdgeOpenContextMenu(const QString &str) {
    qDebug()<< "MW: slotEditEdgeOpenContextMenu() for" << str
            << "at"<< QCursor::pos().x() << "," << QCursor::pos().y();
    //make the menu
    QMenu *edgeContextMenu = new QMenu(str, this);
    edgeContextMenu->addAction( str );
    edgeContextMenu->addSeparator();
    edgeContextMenu->addAction( editEdgeRemoveAct );
    edgeContextMenu->addAction( editEdgeWeightAct );
    edgeContextMenu->addAction( editEdgeLabelAct );
    edgeContextMenu->addAction( editEdgeColorAct );
    edgeContextMenu->exec(QCursor::pos() );
    delete  edgeContextMenu;
}



/**
 * @brief Opens dialogs for the user to specify the source and the target node of a new edge
 *
 * Called when user clicks on the MW button/menu item "Add edge"
 *
 */
void MainWindow::slotEditEdgeAdd(){
    qDebug()<<"Request to add a new edge through UI. Opening source/target node dialogs...";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    qreal weight=1; 	//weight of this new edge should be one...
    bool ok=false;
    int min=activeGraph->vertexNumberMin();
    int max=activeGraph->vertexNumberMax();

    if (min==max) return;		//if there is only one node->no edge

    if ( ! activeGraph->vertexClicked() ) {
        sourceNode=QInputDialog::getInt(
                    this,
                    "Create new edge, Step 1",
                    tr("This will draw a new edge between two nodes. \n"
                       "Enter source node ("
                       +QString::number(min).toLatin1()+"..."
                       +QString::number(max).toLatin1()+"):"), min, 1, max , 1, &ok ) ;
        if (!ok) {
            statusMessage( "Add edge operation cancelled." );
            return;
        }
    }
    else
        sourceNode=activeGraph->vertexClicked();

    qDebug()<<"sourceNode:" << sourceNode;

    if ( activeGraph->vertexExists(sourceNode) ==-1 ) {
        qDebug()<< "Cannot find sourceNode"<<sourceNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?")
                              );
        return;
    }

    targetNode=QInputDialog::getInt
            (this, "Create new edge, Step 2",
             tr( "Source node:" ) + QString::number( sourceNode )
             + tr(" \nNow enter a target node [")
             + QString::number(min).toLatin1()
             + "..."
             + QString::number(max).toLatin1()+"]:",min, min, max , 1, &ok)     ;
    if (!ok) {
        statusMessage( "Add edge target operation cancelled." );
        return;
    }
    if ( activeGraph->vertexExists(targetNode) ==-1 ) {
        qDebug()<< "Cannot find targetNode"<<targetNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?")
                              );
        return;
    }

    weight=QInputDialog::getDouble(
                this, "Create new edge, Step 3",
                tr("Source and target nodes accepted. \n"
                   "Please, enter the weight of new edge: "),1.0, -100.0, 100.0, 1, &ok);
    if (!ok) {
        statusMessage( "Add edge operation cancelled." );
        return;
    }
    //Check if this edge already exists...
    if (activeGraph->edgeExists(sourceNode, targetNode)!=0 ) {
        qDebug("edge exists. Aborting");
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That edge already exists!"),
                              tr("Error. That edge already exists!"),
                              tr("Are you sure you entered the correct node numbers?")
                              );
        return;
    }

    slotEditEdgeCreate(sourceNode, targetNode, weight);

}



/**
 * @brief Handles UI requests to create new edges, when the user clicks the menu item or doubles-clicks two nodes
 *
 * @param source
 * @param target
 * @param weight
 */
void MainWindow::slotEditEdgeCreate (const int &source, const int &target, const qreal &weight) {
    qDebug()<< "User requested to create a new edge"
            << source << "->" << target << "weight" << weight
            << "Setting user settings and calling Graph to to do the job...";

    bool bezier = false;
    bool result = activeGraph->edgeCreate(
                source, target, weight,
                appSettings["initEdgeColor"] ,
            ( editEdgeUndirectedAllAct->isChecked() ) ? 2:0,
            ( editEdgeUndirectedAllAct->isChecked() ) ? false :
                                                        ( (appSettings["initEdgeArrows"] == "true") ? true: false)
            , bezier);

    if (result) {
        statusMessage(tr("New edge %1 -> %2 created, weight %3.").arg(source).arg(target).arg(weight));
    }
}



/**
 * @brief Removes a clicked edge. Otherwise asks the user to specify one edge.
 */
void MainWindow::slotEditEdgeRemove(){

    qDebug() << "Removing selected edges...";

    if ( !activeNodes() || activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int min=0, max=0, sourceNode=-1, targetNode=-1;
    bool ok=false;
    bool removeOpposite = false;

    int selectedEdgeCount = activeGraph->getSelectedEdgesCount();

    qDebug() << "Selected edges:" << selectedEdgeCount;

    if ( ! selectedEdgeCount ) {

        min=activeGraph->vertexNumberMin();
        max=activeGraph->vertexNumberMax();

        qDebug() << "MW::slotEditEdgeRemove() - No edge selected. "
                    "Prompting user to select...";

        sourceNode=QInputDialog::getInt(
                    this,tr("Remove edge"),
                    tr("Source node:  (")+QString::number(min)+
                    "..."+QString::number(max)+"):", min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(
                    this,
                    tr("Remove edge"),
                    tr("Target node:  (")+QString::number(min)+"..."+
                    QString::number(max)+"):",min, 1, max , 1, &ok )   ;
        if (!ok) {
            statusMessage( "Remove edge operation cancelled." );
            return;
        }
        if ( activeGraph->edgeExists(sourceNode, targetNode, false)!=0 ) {
            removeOpposite=false;
            if ( activeGraph->isUndirected() ) {
                removeOpposite=true;
            }
        }
        else {
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?")
                                  );
            return;
        }

    }
    else {

        if ( selectedEdgeCount > 1) {

            qDebug() << "MW::slotEditEdgeRemove() - Multiple edges selected. "
                        "Calling Graph to remove all of them...";

            activeGraph->edgeRemoveSelectedAll();
            return;
        }

        qDebug() << "MW::slotEditEdgeRemove() - One edge selected: "
                 << activeGraph->edgeClicked().source
                 << "->"
                 << activeGraph->edgeClicked().target;

        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated) {

            QStringList items;

            QString arcA = QString::number( activeGraph->edgeClicked().source) + " -->"
                    +QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number( activeGraph->edgeClicked().target)+ " -->"
                    +QString::number(activeGraph->edgeClicked().source);

            items << arcA
                  << arcB
                  << "Both";

            ok = false;

            QString selectedArc = QInputDialog::getItem(
                        this, tr("Select edge"),
                        tr("This is a reciprocated edge. "
                           "Select direction to remove:"), items, 0, false, &ok);

            if ( selectedArc == arcA ) {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if ( selectedArc == arcB ) {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else {  // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                removeOpposite=true;
            }

        }
        else {
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }


    }

    activeGraph->edgeRemove(sourceNode, targetNode, removeOpposite);

    qDebug()<< "MW::slotEditEdgeRemove() -"
            << "View items now:"<< graphicsWidget->items().size()
            << "Scene items now:"<< graphicsWidget->scene()->items().size();

}









/**
 * @brief Changes the label of an edge.
 */
void MainWindow::slotEditEdgeLabel(){
    qDebug() << "MW::slotEditEdgeLabel()";
    if ( !activeEdges() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;

    int min=activeGraph->vertexNumberMin();
    int max=activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target )
    {	//no edge clicked. Ask user to define an edge.
        sourceNode=QInputDialog::getInt(this,
                                        "Change edge label",
                                        tr("Select edge source node:  ("+
                                           QString::number(min).toLatin1()+
                                           "..."+QString::number(max).toLatin1()+
                                           "):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge label operation cancelled." );
            return;
        }
        targetNode=QInputDialog::getInt(this,
                                        "Change edge label...",
                                        tr("Select edge target node:  ("+
                                           QString::number(min).toLatin1()+"..." +
                                           QString::number(max).toLatin1()+"):"),
                                        min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge label operation cancelled." );
            return;
        }

        if ( ! activeGraph->edgeExists (sourceNode, targetNode ) )  {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?")
                                  );

            return;
        }

    }
    else
    {	//edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }

    QString label = QInputDialog::getText( this, tr("Change edge label"),
                                           tr("Enter label: ") );

    if ( !label.isEmpty()) {
        qDebug() << "MW::slotEditEdgeLabel() - " << sourceNode << "->"
                 << targetNode << " new label " << label;
        activeGraph->edgeLabelSet( sourceNode, targetNode, label);
        slotOptionsEdgeLabelsVisibility(true);
        statusMessage( tr("Changed edge label. ")  );
    }
    else {
        statusMessage( tr("Change edge label aborted. ") );
    }
}






/**
 * @brief Changes the color of all edges weighted below threshold to parameter color
 *
 * If color is not valid, it opens a QColorDialog
 * If threshold == RAND_MAX it changes the color of all edges.
 *
 * @param color = QColor()
 * @param threshold = RAND_MAX
 */
void MainWindow::slotEditEdgeColorAll(QColor color, const int threshold){
    qDebug() << "Changing the color of all matching edges to color: " << color.name() << " threshold " << threshold;
    if (!color.isValid()) {
        QString text;
        if (threshold < RAND_MAX) {
            text = "Change the color of edges weighted < "
                    + QString::number(threshold) ;
        }
        else
            text = "Change the color of all edges" ;
        color = QColorDialog::getColor( appSettings["initEdgeColor"], this,
                text);
    }
    if (color.isValid()) {
        qDebug() << "new edge color: " << color.name() << " threshold " << threshold;
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        if (threshold < 0 ) {
            appSettings["initEdgeColorNegative"]=color.name();
        }
        else if (threshold == 0 ) {
            appSettings["initEdgeColorZero"]=color.name();
        }
        else {
            appSettings["initEdgeColor"]=color.name();
        }
        activeGraph->edgeColorAllSet(color.name(), threshold );
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Changed edges color. ")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("edges color change aborted. ") );
    }
}




/**
 * @brief Changes the color of the clicked edge.
 * If no edge is clicked, then it asks the user to specify one.
 */
void MainWindow::slotEditEdgeColor(){
    qDebug() << "MW::slotEditEdgeColor()";
    if (  !activeEdges() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode=-1, targetNode=-1;
    bool ok=false;

    int min=activeGraph->vertexNumberMin();
    int max=activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target)
    {	//no edge clicked. Ask user to define an edge.
        sourceNode=QInputDialog::getInt(this,
                                        "Change edge color",
                                        tr("Select edge source node:  ("+
                                           QString::number(min).toLatin1()+
                                           "..."+QString::number(max).toLatin1()+
                                           "):"), min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge color operation cancelled." );
            return;
        }
        targetNode=QInputDialog::getInt(this,
                                        "Change edge color...",
                                        tr("Select edge target node:  ("+
                                           QString::number(min).toLatin1()+"..." +
                                           QString::number(max).toLatin1()+"):"),
                                        min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge color operation cancelled." );
            return;
        }

        if ( ! activeGraph->edgeExists(sourceNode, targetNode ) )  {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?")
                                  );

            return;
        }

    }
    else
    {	//edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }
    QString curColor = activeGraph->edgeColor(sourceNode, targetNode);
    if (!QColor(curColor).isValid()) {
        curColor=appSettings["initEdgeColor"];
    }
    QColor color = QColorDialog::getColor(
                curColor, this, tr("Select new color....") );

    if ( color.isValid()) {
        QString newColor=color.name();
        qDebug() << "MW::slotEditEdgeColor() - " << sourceNode << "->"
                 << targetNode << " newColor "
                 << newColor;
        activeGraph->edgeColorSet( sourceNode, targetNode, newColor);
        statusMessage( tr("Edge color changed.")  );
    }
    else {
        statusMessage( tr("Change edge color aborted. ") );
    }

}




/**
 * @brief Changes the weight of the clicked edge.
 * If no edge is clicked, asks the user to specify an Edge.
 */
void MainWindow::slotEditEdgeWeight(){
    if (  !activeEdges()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    qDebug("MW::slotEditEdgeWeight()");
    int  sourceNode=-1, targetNode=-1;
    qreal newWeight=1.0;
    int min=activeGraph->vertexNumberMin();
    int max=activeGraph->vertexNumberMax();
    bool changeBothEdges=false;
    bool ok=false;

    // Check if an edge has been clicked/selected.
    if ( activeGraph->edgeClicked().source==0 || activeGraph->edgeClicked().target==0 ) {
        // No edge clicked/selected. Show dialog to select the edge by source/target nodes.
        sourceNode=QInputDialog::getInt(
                    this,
                    "Edge weight",
                    tr("Select edge source node:  ("+
                       QString::number(min).toLatin1()+"..."+
                       QString::number(max).toLatin1()+"):"),
                    min, 1, max , 1, &ok)   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        targetNode=QInputDialog::getInt(
                    this,
                    "Edge weight",
                    tr("Select edge target node:  ("+
                       QString::number(min).toLatin1()+"..."+
                       QString::number(max).toLatin1()+"):"),
                    min, 1, max , 1, &ok  )   ;
        if (!ok) {
            statusMessage( "Change edge weight operation cancelled." );
            return;
        }

        qDebug("source %i target %i",sourceNode, targetNode);
    }
    else {
        // An edge is clicked/selected.

        qDebug() << "MW: slotEditEdgeWeight() - an Edge has already been clicked";

        // Check if clicked edge is reciprocated
        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated) {
            // Clicked edge is reciprocated.
            // We need the user to let us know if she wants to change a single edge or both
            QStringList items;
            QString arcA = QString::number(activeGraph->edgeClicked().source)+ " -->"+QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number(activeGraph->edgeClicked().target)+ " -->"+QString::number(activeGraph->edgeClicked().source);
            items << arcA
                  << arcB
                  << "Both";
            ok = false;
            QString selectedArc = QInputDialog::getItem(this, tr("Select edge"),
                                                        tr("This is a reciprocated edge. "
                                                           "Select direction:"), items, 0, false, &ok);
            if ( selectedArc == arcA ) {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if ( selectedArc == arcB ) {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else {  // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                changeBothEdges=true;
            }

        }
        else {
            // Clicked edge is not reciprocated. We are good to go.
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }


        qDebug() << "MW: slotEditEdgeWeight() from "
                 << sourceNode << " to " << targetNode;

    }

    qreal oldWeight= 0;

    QString dialogTitle="Edge " + QString::number(sourceNode) + "->" + QString::number(targetNode);

    bool undirected = activeGraph->isUndirected();

    // Get the new edge weight -- only if the edge exists.
    if ( ( oldWeight= activeGraph->edgeWeight(sourceNode, targetNode)) != 0 ) {

        // Fix the dialog title.
        if (changeBothEdges || undirected ){
            dialogTitle="Edge " + QString::number(sourceNode) + "<->" + QString::number(targetNode);
        }

        // Prompt the user for the new edge weight
        newWeight = (qreal) QInputDialog::getDouble(
                    this,
                    dialogTitle,
                    tr("New edge weight: "),
                    oldWeight, -RAND_MAX, RAND_MAX, 2, &ok ) ;

        if (ok) {
            activeGraph->edgeWeightSet(sourceNode, targetNode, newWeight,
                                       undirected|| changeBothEdges
                                       );
        }
        else {
            statusMessage(  QString(tr("Change edge weight cancelled."))  );
            return;
        }
    }

}



/**
 * @brief Symmetrizes the ties between every two connected nodes.
 * If there is an arc from Node A to Node B,
 * then a new arc from Node B to Node is created of the same weight.
 * Thus, all arcs become reciprocal and the network becomes symmetric
 * with a symmetric adjacency matrix
 */
void MainWindow::slotEditEdgeSymmetrizeAll(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug() << "Request to symmetrize all edges...";
    activeGraph->setSymmetric();
    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("All ties have been symmetrized."),
                          tr("All ties between nodes have been symmetrized."),
                          tr("The network is now symmetric. ")
                          );
}


/**
 * @brief Adds a new cocitation symmetric relation to the network
 *
 * In the new relation, there are ties only between pairs of nodes who were cocited by others.
 */
void MainWindow::slotEditEdgeSymmetrizeCocitation(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug() << "Request to add a new symmetric relation using cocited nodes...";
    activeGraph->relationAddCocitation();

    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("New cocitation relation added. Ready"),
                          tr("New cocitation relation has been added to the network."),
                          tr("In the new relation, there are ties only between pairs of nodes who were cocited by others.")
                         );

}




/**
 * @brief Opens up the edge dichotomization dialog
  */
void MainWindow::slotEditEdgeDichotomizationDialog(){

    // @TODO: Check if the network is already binary and abord?

    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug() << "MW: slotEditEdgeDichotomizationDialog() - "
                "spawning edgeDichotomizationDialog";

    m_edgeDichotomizationDialog = new DialogEdgeDichotomization(this) ;

    connect( m_edgeDichotomizationDialog, &DialogEdgeDichotomization::userChoices,
             this, &MainWindow::slotEditEdgeDichotomization);

    m_edgeDichotomizationDialog->exec();

}



/**
 * @brief Calls Graph::graphDichotomization() to create a new binary relation
 * in a valued network using edge dichotomization according to threshold value.
  */
void MainWindow::slotEditEdgeDichotomization(const qreal threshold){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug("MW: slotEditEdgeDichotomization() calling graphDichotomization()");
    activeGraph->graphDichotomization(threshold);
    slotHelpMessageToUser(USER_MSG_INFO,tr("New binary relation added."),
                          tr("New dichotomized relation created"),
                          tr("A new relation called \"%1\" has been added to the network, "
                             "using the given dichotomization threshold. ").
                          arg("Binary"));


    statusMessage( tr("Edge dichotomization finished. ") );

}


/**
 * @brief MainWindow::slotEditEdgeSymmetrizeStrongTies
 */
void MainWindow::slotEditEdgeSymmetrizeStrongTies(){
    if ( activeEdges() ==0 )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qDebug()<< "MW::slotEditEdgeSymmetrizeStrongTies() - calling addRelationSymmetricStrongTies()";
    int oldRelationsCounter=activeGraph->relations();
    int answer=0;
    if (oldRelationsCounter>0) {
        switch (
                answer=slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM, tr("Select"),
                                             tr("Symmetrize social network by examining strong ties"),
                                             tr("This network has multiple relations. "
                                                "Symmetrize by examining reciprocated ties across all relations or just the current relation?"),
                                             QMessageBox::NoButton, QMessageBox::NoButton,
                                             tr("all relations"), tr("current relation")
                                             )
                ){
        case 1:
            activeGraph->addRelationSymmetricStrongTies(true);
            break;
        case 2:
            activeGraph->addRelationSymmetricStrongTies(false);
            break;
        }


    }
    else {
        activeGraph->addRelationSymmetricStrongTies(false);
    }
    slotHelpMessageToUser(USER_MSG_INFO,tr("New symmetric relation created from strong ties"),
                          tr("New relation created from strong ties"),
                          tr("A new relation \"%1\" has been added to the network. "
                             "by counting reciprocated ties only. "
                             "This relation is binary and symmetric. ").arg("Strong Ties"));

}

/**
 * @brief Transforms all directed arcs to undirected edges.
 * The result is a undirected and symmetric network
 */
void MainWindow::slotEditEdgeUndirectedAll(const bool &toggle){
    qDebug()<<"MW: slotEditEdgeUndirectedAll() - calling Graph::graphUndirectedSet()";
    if (toggle) {
        activeGraph->setUndirected(true);
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() !=0 ) {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready") ) ;

        }
        else {
            statusMessage( tr("Undirected data mode. "
                              "Any edge you add will be undirected. Ready")) ;
        }
    }
    else {
        activeGraph->setDirected(true);
        optionsEdgeArrowsAct->trigger();
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() !=0 ) {
            statusMessage ( tr("Directed data mode. "
                               "All existing undirected edges transformed to "
                               "directed. Ready")) ;

        }
        else {
            statusMessage ( tr("Directed data mode. "
                               "Any new edge you add will be directed. Ready")) ;
        }
    }

}



/**
 * @brief Toggles between directed (mode=0) and undirected edges (mode=1)
 *
 * @param mode
 */
void MainWindow::slotEditEdgeMode(const int &mode){
    if (mode==1) {
        qDebug()<<"Changing edge mode to undirected. Informing Graph...";
        activeGraph->setUndirected(true);
        qDebug()<<"Setting optionsEdgeArrowsAct to false";
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() !=0 ) {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready") ) ;

        }
        else {
            statusMessage( tr("Undirected data mode. "
                              "Any edge you add will be undirected. Ready")) ;
        }
    }
    else {
        qDebug()<<"Changing edge mode to directed. Informing Graph...";
        activeGraph->setDirected(true);
        qDebug()<<"Triggering optionsEdgeArrowsAct checkbox";
        optionsEdgeArrowsAct->trigger();
        qDebug()<<"Setting optionsEdgeArrowsAct to true";
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() !=0 ) {
            statusMessage ( tr("Directed data mode. "
                               "All existing undirected edges transformed to "
                               "directed.")) ;

        }
        else {
            statusMessage ( tr("Directed data mode. "
                               "Any new edge you add will be directed.")) ;
        }
    }

}







/**
 * @brief Shows a dialog where the user can specify criteria to filter nodes
 *
 */
void MainWindow::slotFilterNodesDialogByCentrality() {

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    // Create a new node filtering dialog
    m_DialogNodeFilterByCentrality = new DialogFilterNodesByCentrality(this);

    // Connect dialog signal to the graph
    connect( m_DialogNodeFilterByCentrality, &DialogFilterNodesByCentrality::userChoices,
             activeGraph, &Graph::edgeFilterByWeight);

    // Show the dialog
    m_DialogNodeFilterByCentrality->exec() ;
}



/**
 * @brief Toggles the status of all isolated vertices
 *
 * @param checked
 */
void MainWindow::slotEditFilterNodesIsolates(bool checked){

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    activeGraph->vertexIsolatedAllToggle( ! editFilterNodesIsolatesAct->isChecked() );
    if ( checked ){
        statusMessage(  tr("Isolated nodes disabled.")  );
    }
    else {
        statusMessage(  tr("Isolated nodes enabled.")  );
    }

}



/**
 * @brief Shows a dialog where the user can specify how to filter edges by their weight
 *
 * All edges weighted more (or less) than the specified weight will be disabled.
 */
void MainWindow::slotEditFilterEdgesByWeightDialog() {

    // Note: We do not check if there are active edges, because the user might have disabled all edges previously.

    // Create a new edge filtering dialog
    m_DialogEdgeFilterByWeight = new DialogFilterEdgesByWeight(this);

    // Connect dialog signal to the graph
    connect( m_DialogEdgeFilterByWeight, &DialogFilterEdgesByWeight::userChoices,
             activeGraph, &Graph::edgeFilterByWeight);

    // Show the dialog
    m_DialogEdgeFilterByWeight->exec() ;
}



/**
 * @brief Toggles the status of all unilateral edges
 *
 * @param checked
 */
void MainWindow::slotEditFilterEdgesUnilateral(bool checked) {

    if ( !activeEdges() && editFilterEdgesUnilateralAct->isChecked() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    activeGraph->edgeFilterUnilateral( ! editFilterEdgesUnilateralAct->isChecked() );
    if ( checked ){
        statusMessage(  tr("Unilateral (weak) edges disabled.")  );
    }
    else {
        statusMessage(  tr("Unilateral (weak) edges enabled.")  );
    }

}



/**
*	Transforms all nodes to edges
    TODO slotEditTransformNodes2Edges
*/
void MainWindow::slotEditTransformNodes2Edges(){


}




/**
    TODO slotLayoutColorationStrongStructural
*/
void MainWindow::slotLayoutColorationStrongStructural() {
}


/**
    TODO slotLayoutColorationRegular
*/
void MainWindow::slotLayoutColorationRegular() {
}



/**
 * @brief Calls Graph::layoutRandom
 * to reposition all nodes on a random layout
 */
void MainWindow::slotLayoutRandom(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    graphicsWidget->clearGuides();

    activeGraph->layoutRandom();

    statusMessage( tr("Nodes in random positions.") );
}



/**
 * @brief Calls Graph::layoutRadialRandom
 * to reposition all nodes on a radial layout randomly
 */
void MainWindow::slotLayoutRadialRandom(){
    qDebug() << "MainWindow::slotLayoutRadialRandom()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }


    slotLayoutGuides(false);

    activeGraph->layoutRadialRandom(true);

    slotLayoutGuides(true);
    statusMessage( tr("Nodes in random concentric circles.") );
}





/**
 * @brief Calls Graph::layoutForceDirectedSpringEmbedder to embed the Eades
 * spring-gravitational model to the network.
 * Called from menu or toolbox checkbox
 */
void MainWindow::slotLayoutSpringEmbedder(){
    qDebug()<< "MW:slotLayoutSpringEmbedder";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph->layoutForceDirectedSpringEmbedder(500);
    statusMessage( tr("Spring-Gravitational (Eades) model embedded.") );
}





/**
 * @brief Calls Graph::layoutForceDirectedFruchtermanReingold to embed
 * the Fruchterman-Reingold model of repelling-attracting forces to the network.
 * Called from menu or toolbox
 */
void MainWindow::slotLayoutFruchterman(){
    qDebug("MW: slotLayoutFruchterman ()");
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph->layoutForceDirectedFruchtermanReingold(100);

    statusMessage( tr("Fruchterman & Reingold model embedded.") );
}





/**
 * @brief Layouts the network according to the Kamada-Kawai FDP model
 */
void MainWindow::slotLayoutKamadaKawai(){
    qDebug()<< "MW::slotLayoutKamadaKawai ()";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    activeGraph->layoutForceDirectedKamadaKawai(400);

    statusMessage( tr("Kamada & Kawai model embedded.") );
}




/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutRadialByProminenceIndex(){
    qDebug() << "Got request to apply a radial layout by prominence index. Checking what index is requested...";
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText=menuitem->text();

    slotLayoutRadialByProminenceIndex(menuItemText);

}



/**
 * @brief Applies a radial layout on the social network, where each node is placed on concentric circles according to their index score.
 *
*  More prominent nodes are closer to the centre of the screen.
*
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutRadialByProminenceIndex(QString prominenceIndexName=""){
    qDebug() << "Will apply a radial layout by prominence index: " << prominenceIndexName;

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    slotLayoutGuides(true);

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qDebug() << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);

    bool dropIsolates=false;

    if (indexType==IndexType::IC && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    activeGraph->layoutByProminenceIndex(
                indexType, 0,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Nodes in inner circles have higher %1 score. ").arg(prominenceIndexName ) );

}





/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutLevelByProminenceIndex(){
    qDebug() << "Got request to apply a level layout by prominence index. Checking what index is requested...";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();

    slotLayoutLevelByProminenceIndex(menuItemText);

}




/**
 * @brief Applies a level layout on the social network, where each node is placed on different top-down levels according to their index score.
 *
 *  More prominent nodes are closer to the the top of the screen
 *
 * @param prominenceIndexName
*/
void MainWindow::slotLayoutLevelByProminenceIndex(QString prominenceIndexName=""){
    qDebug() << "Will apply a level layout by prominence index: " << prominenceIndexName;

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    slotLayoutGuides(true);

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qDebug() << "indexType" << indexType ;

    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(1);

    bool dropIsolates=false;

    if (indexType ==IndexType::IC && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    activeGraph->layoutByProminenceIndex(
                indexType , 1,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Nodes in upper levels have higher %1 score. ").arg(prominenceIndexName ) );

}






/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeSizeByProminenceIndex(){
    qDebug() << "Got request to apply a color layout by prominence index. Checking what index is requested...";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();

    slotLayoutNodeSizeByProminenceIndex(menuItemText);

}


/**
 * @brief Applies a node size layout on the social network, where the size of each of node is analogous to their index score.
 *
 *  More prominent nodes are bigger.
 *
 * @param prominenceIndexName
*/
void MainWindow::slotLayoutNodeSizeByProminenceIndex(QString prominenceIndexName=""){
    qDebug() << "Will apply a node size layout by prominence index: " << prominenceIndexName;

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qDebug() << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType+1);

    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(2);

    bool dropIsolates=false;

    if (indexType==IndexType::IC && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    activeGraph->layoutByProminenceIndex(
                indexType, 2,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Bigger nodes have greater %1 score.").arg(prominenceIndexName ) );
}




/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeColorByProminenceIndex(){

    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem=(QAction *) sender();
    QString menuItemText = menuitem->text();

    slotLayoutNodeColorByProminenceIndex(menuItemText);

}


/**
 * @brief Applies a color layout on the social network. Changes the colors of all nodes according to their index score.
 *
 *  More prominent nodes have more warm colors
 *
 * RED=rgb(255,0,0) most prominent
 * BLUE=rgb(0,0,255) least prominent
 *
 * @param prominenceIndexName
*/
void MainWindow::slotLayoutNodeColorByProminenceIndex(QString prominenceIndexName=""){
    qDebug() << "Will apply a node color layout by prominence index: " << prominenceIndexName;
    if ( !activeNodes() )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qDebug() << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType+1);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(3);

    bool dropIsolates=false;

    if (indexType==8 && activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();


    activeGraph->layoutByProminenceIndex(
                indexType, 3,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage( tr("Nodes with warmer color have greater %1 score.").arg(prominenceIndexName));

}






/**
 * @brief Shows or hides (clears) layout guides
 *
 * @param toggle
 */
void MainWindow::slotLayoutGuides(const bool &toggle){
    qDebug()<< "MW:slotLayoutGuides()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    if (toggle){
        layoutGuidesAct->setChecked(true);
        statusMessage( tr("Layout Guides are displayed") );
    }
    else {
        layoutGuidesAct->setChecked(false);
        graphicsWidget->clearGuides();
        statusMessage( tr("Layout Guides removed") );
    }
}



/**
*	Returns the amount of enabled/active edges on the scene.
*/
int MainWindow::activeEdges(){
    qDebug() << "MW::activeEdges()";
    return activeGraph->edgesEnabled();
}





/**
*	Returns the number of active nodes on the scene.
*/
int MainWindow::activeNodes(){ 
    return activeGraph->vertices();
}






/**
*	Displays the arc and dyad reciprocity of the network
*/
void MainWindow::slotAnalyzeReciprocity(){

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-reciprocity-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeReciprocity(fn, optionsEdgeWeightConsiderAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Reciprocity report saved as: ") + QDir::toNativeSeparators(fn) );

}



/**
*	Displays a box informing the user about the symmetry or not of the adjacency matrix
*/

void MainWindow::slotAnalyzeSymmetryCheck(){
    if ( !activeNodes() )   {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric()) {
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Symmetric network."),
                    tr("The adjacency matrix is symmetric.")
                    );
    }
    else{
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Non symmetric network."),
                    tr("The adjacency matrix is not symmetric.")
                    );
    }


    statusMessage (QString(tr("Ready")) );

}



/**
 * @brief Writes the adjacency matrix inverse
 */
void MainWindow::slotAnalyzeMatrixAdjacencyInverse(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-inverse-"+dateTime+".html";

    statusMessage(tr ("Inverting adjacency matrix.") );

    //activeGraph->writeMatrixAdjacencyInvert(fn, QString("lu")) ;
    activeGraph->writeMatrix(fn,MATRIX_ADJACENCY_INVERSE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Inverse matrix saved as: ")+QDir::toNativeSeparators(fn));
}






/**
 * @brief Writes the transpose adjacency matrix
 */
void MainWindow::slotAnalyzeMatrixAdjacencyTranspose(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-transpose-"+dateTime+".html";

    statusMessage( tr ("Transposing adjacency matrix.") );

    activeGraph->writeMatrix(fn,MATRIX_ADJACENCY_TRANSPOSE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Transpose adjacency matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the cocitation matrix
 */
void MainWindow::slotAnalyzeMatrixAdjacencyCocitation(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-cocitation-"+dateTime+".html";

    statusMessage( tr ("Computing Cocitation matrix.") );

    activeGraph->writeMatrix(fn,MATRIX_COCITATION) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Cocitation matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the degree matrix of the graph
 */
void MainWindow::slotAnalyzeMatrixDegree(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-degree-"+dateTime+".html";

    statusMessage(tr ("Computing Degree matrix.") );

    //activeGraph->writeMatrixDegreeText(fn) ;
    activeGraph->writeMatrix(fn, MATRIX_DEGREE) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Degree matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Writes the Laplacian matrix of the graph
 */
void MainWindow::slotAnalyzeMatrixLaplacian(){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    qDebug() << "MW:slotAnalyzeMatrixLaplacian() - calling Graph::writeMatrix";

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-laplacian-"+dateTime+".html";

    statusMessage(tr ("Computing Laplacian matrix") );

    activeGraph->writeMatrix(fn, MATRIX_LAPLACIAN) ;

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Laplacian matrix saved as: ")+QDir::toNativeSeparators(fn));
}



/**
 * @brief If the network has weighted / valued edges, it asks the user
 * if the app should consider weights or not.
 */
void MainWindow::askAboutEdgeWeights(const bool userTriggered){

    qDebug() << "MW::askAboutEdgeWeights() - checking if graph weighted.";

    if (userTriggered) {
        if (!activeGraph->isWeighted()  ){
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Non-Weighted Network"),
                                  tr("You do not work on a weighted network at the moment. \n"
                                     "Therefore, I will not consider edge weights during "
                                     "computations. \n"
                                     "This option applies only when you load or create "
                                     "a weighted network "));
            optionsEdgeWeightConsiderAct->setChecked(false);
            return;
        }
    }
    else {
        if (!activeGraph->isWeighted()  ){
            optionsEdgeWeightConsiderAct->setChecked(false);
            return;
        }
    }
    qDebug() << "MW::askAboutEdgeWeights() - graph weighted - checking if we have asked user.";

    if (askedAboutWeights) {
        return;
    }

    qDebug() << "MW::askAboutEdgeWeights() - graph weighted - let's ask the user.";

    switch(
           slotHelpMessageToUser(USER_MSG_QUESTION,
                                 tr("Weighted Network"),
                                 tr("This is a weighted network. Consider edge weights?"),
                                 tr("The ties in this network have weights (non-unit values) assigned to them. "
                                    "Do you want me to take these edge weights into account (i.e. when computing distances) ?"),
                                 QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)

           )
    {
    case QMessageBox::Yes:
        optionsEdgeWeightConsiderAct->setChecked(true);
        break;
    case QMessageBox::No:
        optionsEdgeWeightConsiderAct->setChecked(false);
        break;
    default: // just for sanity
        optionsEdgeWeightConsiderAct->setChecked(false);
        return;
        break;
    }


    if (optionsEdgeWeightConsiderAct->isChecked()){
        switch(

               slotHelpMessageToUser(
                   USER_MSG_QUESTION, tr("Inverse edge weights during calculations? "),
                   tr("Inverse edge weights during calculations? "),
                   tr("If the edge weights denote cost or real distances (i.e. miles between cities), "
                      "press No, since the distance between two nodes should be the quickest "
                      "or cheaper one. \n\n"
                      "If the weights denote value or strength (i.e. votes or interaction), "
                      "press Yes to inverse the weights, since the distance between two "
                      "nodes should be the most valuable one."),
                   QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)

               )
        {
        case QMessageBox::Yes:
            inverseWeights=true;
            break;
        case QMessageBox::No:
            inverseWeights=false;
            break;
        default: // just for sanity
            inverseWeights=true;
            return;
            break;
        }
    }
    askedAboutWeights=true;
    return;
}



/**
 * @brief Handles requests to compute the graph/geodesic distance between two user-specified nodes
 *
 * The geodesic distance of two nodes is the length of the shortest path between them.
 *
 */
void MainWindow::slotAnalyzeDistance(){
    if ( !activeNodes() || !activeEdges()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool ok1=false, ok2=false;
    int  min=1, max=1, sourceNum=-1, targetNum=-1;

    min=activeGraph->vertexNumberMin();
    max=activeGraph->vertexNumberMax();


    sourceNum=QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select source node (%1..%2):")
            .arg(QString::number(min)).arg(QString::number(max)),
        min, min, max, 1, &ok1
        )   ;

    if (!ok1) {
        statusMessage( "Distance calculation operation cancelled." );
        return;
    }

    targetNum=QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select target node (%1..%2):")
            .arg(QString::number(min),QString::number(max)),
        min, min, max, 1, &ok2
       );

    if (!ok2) {
        statusMessage( tr("Distance calculation operation cancelled.") );
        return;
    }

    qDebug() << "Computing geodesic distance:" << sourceNum  << "->" <<  targetNum;

    if (activeGraph->isSymmetric() && sourceNum>targetNum) {
        qSwap(sourceNum,targetNum);
    }

    askAboutEdgeWeights();

    int distanceGeodesic = activeGraph->graphDistanceGeodesic(
        sourceNum, targetNum,
        optionsEdgeWeightConsiderAct->isChecked(),
        inverseWeights);

    if ( distanceGeodesic > 0 && distanceGeodesic < RAND_MAX) {

        qDebug() << "geodesic distance" << sourceNum  << "->" <<  targetNum << "=" << distanceGeodesic;

        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(distanceGeodesic),
                    tr("Geodesic Distance: %1").arg(distanceGeodesic),
                    tr("Nodes %1 and %2 are connected through at least one path. The length of the shortest path is %3.")
                        .arg(sourceNum).arg(targetNum).arg(distanceGeodesic)
                    );
    }
    else {
        qDebug() << "geodesic distance" << sourceNum  << "->" <<  targetNum << "is infinite.";
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Nodes %1 and %2 are not connected. "
                       "In this case, their geodesic distance is considered to be infinite.")
                    .arg(sourceNum).arg(targetNum)
                    );
    }

}




/**
 * @brief Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
 */
void MainWindow::slotAnalyzeMatrixDistances(){
    qDebug() << "Request to compute the matrix of geodesic distances. Please wait...";
    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesic-distances-"+dateTime+".html";

    askAboutEdgeWeights();

    statusMessage( tr("Computing geodesic distances. Please wait...") );

    activeGraph->writeMatrix(fn,MATRIX_DISTANCES,
                             optionsEdgeWeightConsiderAct->isChecked(),
                             inverseWeights,
                             editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Geodesic Distances matrix saved as: ")+QDir::toNativeSeparators(fn));
}




/**
 * @brief Invokes calculation of the geodedics matrix (the number of shortest paths
 * between each pair of nodes in the loaded network), then displays it.
 */
void MainWindow::slotAnalyzeMatrixGeodesics(){
    qDebug() << "Request to compute the matrix of geodesics. Please wait...";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesics-"+dateTime+".html";

    askAboutEdgeWeights();

    statusMessage(  tr("Computing geodesics (number of shortest paths) for each pair. Please wait...") );

    activeGraph->writeMatrix(fn,MATRIX_GEODESICS,
                             optionsEdgeWeightConsiderAct->isChecked(),
                             inverseWeights,
                             editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Geodesics Matrix saved as: ") + QDir::toNativeSeparators(fn));
}




/**
 * @brief Displays the network diameter (largest geodesic)
 */
void MainWindow::slotAnalyzeDiameter() {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    statusMessage( tr("Computing graph diameter. Please wait...") );

    int netDiameter=activeGraph->graphDiameter(
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights);

    if ( activeGraph->isWeighted() ) {
        if (optionsEdgeWeightConsiderAct->isChecked()) {
            slotHelpMessageToUser (
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                            "D = %1").arg(netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                            "(maximum shortest path length) between any two nodes.\n\n"
                             "Note, since this is a weighted network, "
                            "the diameter can be greater than N.")
                        );

        }
        else {
            slotHelpMessageToUser (
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                            "D = %1").arg(netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                            "(maximum shortest path length) between any two nodes.\n\n"
                            "Note, edge weights were disregarded during the computation. "
                            "This is the diameter of the corresponding network without weights.")
                        );
        }
    }
    else
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Network diameter computed."),
                    tr("Network diameter computed. \n\n"
                        "D = %1").arg(netDiameter),
                    tr("The diameter of a network is the maximum geodesic distance "
                        "(maximum shortest path length) between any two nodes.\n\n"
                        "Note, since this is a non-weighted network, the diameter is always smaller than N-1.")
                    );


}





/**
 * @brief Displays the average shortest path length (average graph distance)
 */
void MainWindow::slotAnalyzeDistanceAverage() {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    statusMessage(  tr("Computing Average Graph Distance. Please wait...") );

    qreal averGraphDistance=activeGraph->graphDistanceGeodesicAverage(
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() );

    bool isConnected = activeGraph->isConnected();

    if ( isConnected ) {
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Average graph distance computed."),
                    tr("Average graph distance computed. \n\n"
                        "d = %1").arg(averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                        "for all possible pairs of nodes.\n\n"
                        "The average distance in this connected network "
                        "is the sum of pair-wise distances divided by N * (N - 1).")
                    );
    }
    else {
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("Average distance computed."),
                    tr("Average distance computed. \n\n"
                        "d = %1").arg(averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                        "for all possible pairs of nodes.\n\n"
                        "The average distance in this disconnected network "
                        "is the sum of pair-wise distances divided by the number of existing geodesics.")
                    );
    }
}


/**
*	Writes Eccentricity indices into a file, then displays it.
*/
void MainWindow::slotAnalyzeEccentricity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeEccentricity(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eccentricities saved as: ") + QDir::toNativeSeparators(fn) );
}






/**
 * @brief Reports the network connectedness
 */
void MainWindow::slotAnalyzeConnectedness(){
    qDebug() << "MW::slotAnalyzeConnectedness()" ;

    int N = activeGraph->vertices();

    if (!N) {
        // null network with empty graph is connected
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("This empty network is considered connected!"),
                    tr("Empty network is considered connected!"),
                    tr("A null network (empty graph) is considered connected.")
                    );

    }
    else if (N==1){
        // 1-actor network with singleton graph is connected
        slotHelpMessageToUser (
                    USER_MSG_INFO,
                    tr("This 1-actor network is considered connected!"),
                    tr("This 1-actor network is considered connected!"),
                    tr("A 1-actor network (singleton graph) is always considered connected.")
                    );
    }
    else {
        bool isConnected=activeGraph->isConnected();

        qDebug() << "MW::slotAnalyzeConnectedness result " << isConnected;

        if(isConnected){
            if (activeGraph->isDirected()){
                slotHelpMessageToUser (
                            USER_MSG_INFO,
                            tr("This directed network is strongly connected."),
                            tr("This directed network is strongly connected."),
                            tr("A 1-actor network (singleton graph) is considered connected.")
                            );

            }
            else {
                slotHelpMessageToUser (
                            USER_MSG_INFO,
                            tr("This undirected network is connected."),
                            tr("This undirected network is connected."),
                            tr("This network has an undirected graph which is connected.")
                            );

            }
        }
        else {
            if (activeGraph->isDirected()){
                slotHelpMessageToUser (
                            USER_MSG_INFO,
                            tr("This directed network is disconnected."),
                            tr("This directed network is disconnected."),
                            tr("There are pairs of nodes that are not connected with any directed path.")
                            );

            }
            else {
                slotHelpMessageToUser (
                            USER_MSG_INFO,
                            tr("This undirected network is not connected."),
                            tr("This undirected network is not connected."),
                            tr("There are pairs of nodes that are not connected with any path.")
                            );
            }
        }

    }

}


/**
*	Calls Graph:: writeWalksOfLengthMatrixPlainText() to calculate and print
*   the number of walks of a given length , between each pair of nodes.
*/
void MainWindow::slotAnalyzeWalksLength(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok=false;

    int length = QInputDialog::getInt(
                this, "Number of walks",
                tr("Select desired length of walk: (2 to %1)").arg(activeNodes()-1),
                2, 2, activeNodes()-1, 1, &ok );
    if (!ok) {
        statusMessage( "Cancelled." );
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-length-"+QString::number(length)+"-"+dateTime+".html";


    activeGraph->writeMatrixWalks(fn, length);


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Walks of length %1 matrix saved as: ").arg(length) + QDir::toNativeSeparators(fn) );
}



/**
 * @brief Calls Graph:: writeWalksTotalMatrixPlainText() to calculate and print
*  the total number of walks of any length , between each pair of nodes.
 */
void MainWindow::slotAnalyzeWalksTotal(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 50) {
        switch( QMessageBox::critical(
                    this,
                    "Slow function warning",
                    tr("Please note that this function is VERY SLOW on large networks (n>50), "
                       "since it will calculate all powers of the sociomatrix up to n-1 "
                       "in order to find out all possible walks. \n\n"
                       "If you need to make a simple reachability test, "
                       "we advise to use the Reachability Matrix function instead. \n\n"
                       "Are you sure you want to continue?"),
                    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-total-"+dateTime+".html";

    statusMessage(  tr("Computing total walks matrix. Please wait...") );

    activeGraph->writeMatrixWalks(fn);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage("Total walks matrix saved as: " + QDir::toNativeSeparators(fn));

}



/**
*	Calls Graph:: writeReachabilityMatrixPlainText() to calculate and print
*   the Reachability Matrix of the network.
*/
void MainWindow::slotAnalyzeReachabilityMatrix(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-reachability-"+dateTime+".html";

    statusMessage(  tr("Computing reachability matrix. Please wait...") );

    activeGraph->writeMatrix(fn, MATRIX_REACHABILITY );

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Reachability matrix saved as: ") + QDir::toNativeSeparators(fn) );
}








/**
 * @brief Calls Graph::writeClusteringCoefficient() to write Clustering Coefficients
 * into a file, and displays it.
 */
void MainWindow::slotAnalyzeClusteringCoefficient (){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficient-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph->writeClusteringCoefficient(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Clustering Coefficients saved as: ") + QDir::toNativeSeparators(fn));
}








/**
 * @brief Calls Graph:: writeCliqueCensus() to write the Clique Census
*  into a file, then displays it.
 */
void MainWindow::slotAnalyzeCommunitiesCliqueCensus(){

    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() == 1 ) {
        slotHelpMessageToUserError("Only one node is present, therefore 1 clique");
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clique-census-"+dateTime+".html";

    bool considerWeights=true;

    if (! activeGraph->writeCliqueCensus(fn, considerWeights) ) {
        return;
    }

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Clique Census saved as: ") + QDir::toNativeSeparators(fn));
}




/**
*	Calls Graph to compute and write a triad census into a file, then displays it.
*/
void MainWindow::slotAnalyzeCommunitiesTriadCensus() {

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-triad-census-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph->writeTriadCensus(fn, considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Triad Census saved as: ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Displays the DialogSimilarityMatches dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityMeasureDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()";

    if ( !activeNodes()  )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    m_dialogSimilarityMatches = new DialogSimilarityMatches(this);

    connect( m_dialogSimilarityMatches, &DialogSimilarityMatches::userChoices,
             this, &MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure );

    m_dialogSimilarityMatches->exec();

}




/**
 * @brief Calls Graph::writeMatrixSimilarityMatching() to write a
 * similarity matrix according to given measure into a file, and displays it.
 *
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure(const QString &matrix,
                                                              const QString &varLocation,
                                                              const QString &measure,
                                                              const bool &diagonal) {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString metric;
    if (measure.contains("Simple",Qt::CaseInsensitive))
        metric = "simple-matching" ;
    else if (measure.contains("Jaccard",Qt::CaseInsensitive))
        metric ="jaccard" ;
    else if (measure.contains("None",Qt::CaseInsensitive))
        metric = "none";
    else if (measure.contains("Hamming",Qt::CaseInsensitive))
        metric ="hamming";
    else if (measure.contains("Cosine",Qt::CaseInsensitive))
        metric ="cosine";
    else if (measure.contains("Euclidean",Qt::CaseInsensitive))
        metric ="euclidean";
    else if (measure.contains("Manhattan",Qt::CaseInsensitive))
        metric ="manhattan";
    else if (measure.contains("Pearson ",Qt::CaseInsensitive))
        metric = "pearson";
    else if (measure.contains("Chebyshev",Qt::CaseInsensitive))
        metric = "chebyshev";


    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-similarity-"+metric+"-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph->writeMatrixSimilarityMatching( fn,
                                                measure,
                                                matrix,
                                                varLocation,
                                                diagonal,
                                                considerWeights);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Similarity matrix saved as: ") + QDir::toNativeSeparators(fn));
}




/**
 * @brief Displays the DialogDissimilarities dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceDissimilaritiesDialog()";

    m_dialogdissimilarities = new DialogDissimilarities(this);

    connect( m_dialogdissimilarities, &DialogDissimilarities::userChoices,
             this, &MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile );

    m_dialogdissimilarities->exec();

}





/**
 * @brief Invokes calculation of pair-wise tie profile dissimilarities of the
 * network, then displays it.
 * @param metric
 * @param varLocation
 * @param diagonal
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile(const QString &metric,
                                                                    const QString &varLocation,
                                                                    const bool &diagonal){
    qDebug() << "MW::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile()";
    if ( !activeNodes()    )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString metricStr;
    if (metric.contains("Simple",Qt::CaseInsensitive))
        metricStr = "simple-matching" ;
    else if (metric.contains("Jaccard",Qt::CaseInsensitive))
        metricStr ="jaccard" ;
    else if (metric.contains("None",Qt::CaseInsensitive))
        metricStr = "none";
    else if (metric.contains("Hamming",Qt::CaseInsensitive))
        metricStr ="hamming";
    else if (metric.contains("Cosine",Qt::CaseInsensitive))
        metricStr ="cosine";
    else if (metric.contains("Euclidean",Qt::CaseInsensitive))
        metricStr ="euclidean";
    else if (metric.contains("Manhattan",Qt::CaseInsensitive))
        metricStr ="manhattan";
    else if (metric.contains("Pearson ",Qt::CaseInsensitive))
        metricStr = "pearson";
    else if (metric.contains("Chebyshev",Qt::CaseInsensitive))
        metricStr = "chebyshev";

    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-dissimilarities-"+metricStr+"-"+dateTime+".html";


    askAboutEdgeWeights();

    activeGraph->writeMatrixDissimilarities(fn, metric, varLocation,diagonal,
                                            optionsEdgeWeightConsiderAct->isChecked());

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Tie profile dissimilarities matrix saved as: ")+QDir::toNativeSeparators(fn));
}



/**
 * @brief Calls the m_dialogSimilarityPearson to display the Pearson statistics dialog
 */
void MainWindow::slotAnalyzeStrEquivalencePearsonDialog(){
    qDebug()<< "MW::slotAnalyzeStrEquivalencePearsonDialog()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    m_dialogSimilarityPearson = new DialogSimilarityPearson(this);

    connect( m_dialogSimilarityPearson, &DialogSimilarityPearson::userChoices,
             this, &MainWindow::slotAnalyzeStrEquivalencePearson );

    m_dialogSimilarityPearson->exec();
}



/**
 * @brief Calls Graph::writeMatrixSimilarityPearson() to write Pearson
 * Correlation Coefficients into a file, and displays it.
 *
 */
void MainWindow::slotAnalyzeStrEquivalencePearson(const QString &matrix,
                                                  const QString &varLocation,
                                                  const bool &diagonal) {
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-pearson-coefficients-"+dateTime+".html";

    bool considerWeights=true;

    activeGraph->writeMatrixSimilarityPearson( fn, considerWeights, matrix, varLocation, diagonal);

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Pearson correlation coefficients matrix saved as: ") + QDir::toNativeSeparators(fn));
}



/**
 * @brief Displays the slotAnalyzeStrEquivalenceClusteringHierarchicalDialog dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog() {
    qDebug()<< "MW::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString preselectMatrix = "Adjacency";

    if (!activeGraph->isWeighted()) {
        preselectMatrix = "Distances";
    }
    m_dialogClusteringHierarchical = new DialogClusteringHierarchical(this, preselectMatrix);

    connect( m_dialogClusteringHierarchical, &DialogClusteringHierarchical::userChoices,
             this, &MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchical );

    m_dialogClusteringHierarchical->exec();

}



/**
 * @brief Called from DialogClusteringHierarchical with user choices. Calls
 * Graph::writeClusteringHierarchical() to compute and write HCA and displays the report.
 * @param matrix
 * @param similarityMeasure
 * @param linkageCriterion
 * @param diagonal
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchical(const QString &matrix,
                                                                 const QString &varLocation,
                                                                 const QString &metric,
                                                                 const QString &method,
                                                                 const bool &diagonal,
                                                                 const bool &diagram){

    qDebug()<< "MW::slotAnalyzeStrEquivalenceClusteringHierarchical()";


    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-hierarchical-clustering-"+dateTime+".html";

    bool considerWeights=true;
    bool inverseWeights=false;
    bool dropIsolates=true;

    if (! activeGraph->writeClusteringHierarchical(fn,
                                                   varLocation,
                                                   matrix,
                                                   metric,
                                                   method,
                                                   diagonal,
                                                   diagram,
                                                   considerWeights,
                                                   inverseWeights,
                                                   dropIsolates) ){

        return;
    }


    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Hierarchical Cluster Analysis saved as: ") + QDir::toNativeSeparators(fn));

}




/**
*	Writes Out-Degree Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityDegree(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights(false);

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree-"+dateTime+".html";


    activeGraph->writeCentralityDegree(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                editFilterNodesIsolatesAct->isChecked() );

    statusMessage(tr("Opening Out-Degree Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Out-Degree Centralities report saved as: ") + QDir::toNativeSeparators(fn));
}




/**
*	Writes Closeness Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityCloseness(){
    qDebug() << "MW::slotAnalyzeCentralityCloseness()";
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool dropIsolates=false;
    askAboutEdgeWeights();

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-"+dateTime+".html";


    activeGraph->writeCentralityCloseness(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked() || dropIsolates);

    statusMessage(tr("Opening Closeness Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Closeness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityClosenessIR
*	Writes Centrality Closeness (based on Influence Range) indices into a file,
*   then displays it.
 */
void MainWindow::slotAnalyzeCentralityClosenessIR(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-influence-range-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeCentralityClosenessInfluenceRange(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Influence Range Closeness Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Influence Range Closeness Centralities report saved as: ")+QDir::toNativeSeparators(fn));
}




/**
*	Writes Betweenness Centralities into a file, then displays it.
*/
void MainWindow::slotAnalyzeCentralityBetweenness(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-betweenness-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeCentralityBetweenness(
                fn, optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Betweenness Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Betweenness Centralities report saved as: ")+QDir::toNativeSeparators(fn));
}





/**
*	Writes Degree Prestige indices (In-Degree Centralities) into a file, then displays it.
*/
void MainWindow::slotAnalyzePrestigeDegree(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric()) {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("This network is not directed (undirected graph). "
                                 "The Degree Prestige index counts inbound edges, "
                                 "therefore it is meaningful on directed networks. "
                                 "For undirected networks, such as this one, Degree Prestige is the same as Degree Centrality.")
                              );

    }

    askAboutEdgeWeights(false);

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-degree-"+dateTime+".html";

    activeGraph->writePrestigeDegree(fn,
                                     optionsEdgeWeightConsiderAct->isChecked(),
                                     editFilterNodesIsolatesAct->isChecked() );

    statusMessage(tr("Opening Degree Prestige (in-degree) report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Degree Prestige (in-degree) report saved as: ") + QDir::toNativeSeparators(fn));
}



/**
*	Writes PageRank Prestige indices into a file, then displays it.
*/
void MainWindow::slotAnalyzePrestigePageRank(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-pagerank-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writePrestigePageRank(fn, editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening PageRank Prestige report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("PageRank Prestige report saved as: ")+ QDir::toNativeSeparators(fn));
}



/**
 * @brief MainWindow::slotAnalyzePrestigeProximity
 * Writes Proximity Prestige indices into a file, then displays them.
 */
void MainWindow::slotAnalyzePrestigeProximity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-proximity-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writePrestigeProximity(fn, true, false ,
                                        editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Proximity Prestige report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Proximity Prestige report saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityInformation
 * Writes Informational Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityInformation(){

    qDebug() << "MW::slotAnalyzeCentralityInformation()";

    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 200) {
        switch(
               QMessageBox::critical(
                   this, "Slow function warning",
                   tr("Please note that this function is <b>SLOW</b> on large "
                      "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                      "Aii=1+weighted_degree_ni <br>"
                      "Aij=1 if (i,j)=0 <br>"
                      "Aij=1-wij if (i,j)=wij <br>"
                      "Next, it will compute the inverse matrix C of A. "
                      "The computation of the inverse matrix is a CPU intensive function "
                      "although it uses LU decomposition. <br>"
                      "How slow is this? For instance, to compute IC scores of 600 nodes "
                      "on a modern i7 4790K CPU you will need to wait for 2 minutes at least. <br>"
                      "Are you sure you want to continue?"), QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) ) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-information-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeCentralityInformation(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights);

    statusMessage(tr("Opening Information Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Information Centralities report saved as: ")+ QDir::toNativeSeparators(fn));
}






/**
 * @brief Writes Eigenvector Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityEigenvector(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eigenvector-"+dateTime+".html";

    askAboutEdgeWeights();

    bool dropIsolates = false;

    activeGraph->writeCentralityEigenvector(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                dropIsolates);

    statusMessage(tr("Opening Eigenvector Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eigenvector Centralities report saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityStress
 * Writes Stress Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityStress(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-stress-"+dateTime+".html";

    askAboutEdgeWeights();


    activeGraph->writeCentralityStress(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Stress Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Stress Centralities report saved as: ")+ QDir::toNativeSeparators(fn));
}





/**
 * @brief MainWindow::slotAnalyzeCentralityPower
 * Writes Gil-Schmidt Power Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityPower(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-power-Gil-Schmidt-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeCentralityPower(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Gil-Schmidt Power Centralities report..."));
    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Gil-Schmidt Power Centralities report saved as: ")+ QDir::toNativeSeparators(fn));
}




/**
 * @brief MainWindow::slotAnalyzeCentralityEccentricity
 * Writes Eccentricity Centralities into a file, then displays it.
 */
void MainWindow::slotAnalyzeCentralityEccentricity(){
    if ( !activeNodes()   )  {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString dateTime=QDateTime::currentDateTime().toString ( QString ("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eccentricity-"+dateTime+".html";

    askAboutEdgeWeights();

    activeGraph->writeCentralityEccentricity(
                fn,
                optionsEdgeWeightConsiderAct->isChecked(),
                inverseWeights,
                editFilterNodesIsolatesAct->isChecked());

    statusMessage(tr("Opening Closeness Centralities report..."));

    if ( appSettings["viewReportsInSystemBrowser"] == "true" ) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }
    else {
        TextEditor *ed = new TextEditor(fn,this,true);
        ed->show();
        m_textEditors << ed;
    }

    statusMessage(tr("Eccentricity Centralities report saved as: ")+ QDir::toNativeSeparators(fn));
}



/**
 * @brief Updates the prominence distribution miniChart
 * Called from Graph after computing the prominence index distribution.
 * @param series
 * @param axisX
 * @param min
 * @param max
 */
void MainWindow::slotAnalyzeProminenceDistributionChartUpdate(QAbstractSeries *series,
                                                              QAbstractAxis *axisX,
                                                              const qreal &min,
                                                              const qreal &max,
                                                              QAbstractAxis *axisY,
                                                              const qreal &minF,
                                                              const qreal &maxF) {

    Q_UNUSED(minF);

    qDebug() << "Updating the prominence distribution miniChart";

    if (series == Q_NULLPTR) {
        qDebug() << "series is null! Resetting to trivial";
        miniChart->resetToTrivial();
        return;
    }


    // Set the style of the lines and bars
    switch (series->type()) {
    case QAbstractSeries::SeriesTypeBar	:
        qDebug() << "this an BarSeries";
        break;
    case QAbstractSeries::SeriesTypeArea :
        qDebug() << "this an AreaSeries";

        break;
    default:
        break;
    }


    // Clear miniChart from old series.
    miniChart->removeAllSeries();

    // Remove all axes
    miniChart->removeAllAxes();

    // Add series to miniChart
    miniChart->addSeries(series);

    // Set Chart title and remove legend
    miniChart->setTitle(series->name() + QString(" distribution"), QFont("Times",8));

    miniChart->toggleLegend(false);


    QString chartHelpMsg = tr("Distribution of %1 values:\n"
                              "Min value: %2 \n"
                              "Max value: %3 \n"
                              "Please note that, due to the small size of this widget, \n"
                              "if you display a distribution in Bar Chart where there are \n"
                              "more than 10 values, the widget will not show all bars. \n"
                              "In this case, use Line or Area Chart (from Settings). \n"
                              "In any case, the large chart in the HTML report \n"
                              "is better than this widget..."
                           )
            .arg(series->name() )
            .arg(min, 0,'g',appSettings["initReportsRealNumberPrecision"].toInt(0, 10))
            .arg(max, 0,'g',appSettings["initReportsRealNumberPrecision"].toInt(0, 10));

    miniChart->setToolTip( chartHelpMsg );

    miniChart->setWhatsThis( chartHelpMsg );

    // if true, then bar chart appears with default X axis (1,2,3 ...)
    bool useDefaultAxes = false;

    if ( ! useDefaultAxes ) {
        if ( axisX != Q_NULLPTR ) {
            qDebug() << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisX not null. Setting it to miniChart";
            miniChart->setAxisX(axisX, series);

            miniChart->setAxisXMin(0);
            miniChart->setAxisXLabelFont();
            miniChart->setAxisXLinePen();
            miniChart->setAxisXGridLinePen();
            miniChart->setAxisXLabelsAngle(-90);
        }
        if ( axisY != Q_NULLPTR ){
            qDebug() << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisY not null. Setting it to miniChart";
            miniChart->setAxisY(axisY, series);
            miniChart->setAxisYMin(0);
            miniChart->setAxisYLabelFont();
            miniChart->setAxisYLinePen();
            miniChart->setAxisYGridLinePen();
        }
    }


     if ( ( axisX == Q_NULLPTR && axisY == Q_NULLPTR ) || useDefaultAxes ){

         qDebug() << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                     "axisX and axisY null. Calling createDefaultAxes()";
         miniChart->createDefaultAxes();

         qDebug() << "MW::slotAnalyzeProminenceDistributionChartUpdate() - setting axis min";
         miniChart->setAxisYMin(0);
         miniChart->setAxisXMin(0);

         // Apply our theme to axes:
         miniChart->setAxesThemeDefault();
         miniChart->axes(Qt::Vertical).first()->setMax(maxF+1.0);
         miniChart->setAxisXLabelsAngle(-90);
         //    axisX->setShadesVisible(false);

     }








}





/**
 * @brief Creates a Qt Progress Dialog
 * if max = 0, then max becomes equal to active vertices*
 * @param max
 * @param msg
 */
void MainWindow::slotProgressBoxCreate(const int &max, const QString &msg){
    qDebug() << "MW::slotProgressBoxCreate" ;

    if (  appSettings["showProgressBar"] == "true"  ){
        int duration = (max==0) ? activeNodes(): max;
        QProgressDialog *progressBox = new QProgressDialog(msg,
                                                           "Cancel",
                                                           0,
                                                           duration,
                                                           this);
        polishProgressDialog(progressBox);

        progressBox->setWindowModality(Qt::WindowModal);
        progressBox->setWindowModality(Qt::ApplicationModal);

        connect ( activeGraph, &Graph::signalProgressBoxUpdate,
                  progressBox, &QProgressDialog::setValue );

        progressBox->setMinimumDuration(0);
        progressBox->setAutoClose(true);
        progressBox->setAutoReset(true);

        progressDialogs.push(progressBox);
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

}


/**
 * @brief Destroys the first in queue Progress dialog
 */
void MainWindow::slotProgressBoxDestroy(const int &max){
    qDebug() << "MainWindow::slotProgressBoxDestroy";
    QApplication::restoreOverrideCursor();
    if (  appSettings["showProgressBar"] == "true" && max > -1 ) {
        if (! progressDialogs.isEmpty()) {
            QProgressDialog *progressBox = progressDialogs.pop();
            progressBox->reset();
            progressBox->deleteLater();
            delete progressBox;
        }
    }
}





/**
 * @brief MainWindow::slotOptionsNodeNumbersVisibility
 * Turns on/off displaying the numbers of nodes (outside ones)
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsNodeNumbersVisibility()" << toggle;
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Nodes Numbers. Please wait...") );
    appSettings["initNodeNumbersVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setNodeNumberVisibility(toggle);
    optionsNodeNumbersVisibilityAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Node Numbers are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Node Numbers are visible again...") );
    }
    QApplication::restoreOverrideCursor();
    return;
}




/**
 * @brief MainWindow::slotOptionsNodeNumbersInside
 * Turns on/off displaying the nodenumbers inside the nodes.
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersInside(bool toggle){
    qDebug() << "MW::slotOptionsNodeNumbersInside()" << toggle;

    statusMessage( tr("Toggle Numbers inside nodes. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    // if node numbers are hidden, show them first.
    if ( toggle && appSettings["initNodeNumbersVisibility"] != "true" )
        slotOptionsNodeNumbersVisibility(true);

    appSettings["initNodeNumbersInside"] = (toggle) ? "true":"false";
    graphicsWidget->setNumbersInsideNodes(toggle);
    optionsNodeNumbersVisibilityAct->setChecked (toggle);

    if (toggle){
        statusMessage( tr("Numbers inside nodes...") );
    }
    else {
        statusMessage( tr("Numbers outside nodes...") );
    }
    QApplication::restoreOverrideCursor();
}





/**
 * @brief MainWindow::slotOptionsNodeLabelsVisibility
 * Turns on/off displaying labels
 * @param toggle
 */
void MainWindow::slotOptionsNodeLabelsVisibility(bool toggle){
    qDebug() << "MW::slotOptionsNodeLabelsVisibility()" << toggle;

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    statusMessage( tr("Toggle Nodes Labels. Please wait...") );

    appSettings["initNodeLabelsVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setNodeLabelsVisibility(toggle);
    optionsNodeLabelsVisibilityAct->setChecked ( toggle );

    if (!toggle) {
        statusMessage( tr("Node Labels are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Node Labels are visible again...") );
    }
    QApplication::restoreOverrideCursor();
}






/**
 * @brief MainWindow::slotOptionsEdgesVisibility
 * @param toggle
 */
void MainWindow::slotOptionsEdgesVisibility(bool toggle){
    if ( !activeEdges() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges. Please wait...") );
    appSettings["initEdgesVisibility"] = (toggle) ? "true": "false";
    graphicsWidget->setAllItemsVisibility(TypeEdge, toggle);
    if (!toggle) 	{
        statusMessage( tr("Edges are invisible now. Click again the same menu to display them.") );
    }
    else{
        statusMessage( tr("Edges visible again...") );
    }
    QApplication::restoreOverrideCursor();
}




/**
 * @brief Turns on/off the arrows of edges
 * @param toggle
 */
void MainWindow::slotOptionsEdgeArrowsVisibility(bool toggle){
    qDebug()<<"Request to toggle edges arrows to:" << toggle;

    statusMessage( tr("Toggling Edges' Arrows. Please wait...") );
    appSettings["initEdgeArrows"]= (toggle) ? "true":"false";

    graphicsWidget->setEdgeArrowsVisibility(toggle);
    if (toggle) {
        statusMessage( tr("Arrows in edges: on."));
    }
    else {
        statusMessage( tr("Arrows in edges: off."));
    }

}





/**
 * @brief Toggles edge weights during computations
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightsDuringComputation(bool toggle) {
    askedAboutWeights=false;
    askAboutEdgeWeights(toggle);
    activeGraph->setModStatus(activeGraph->ModStatus::EdgeCount);
}



/**
*  FIXME edges Bezier
*/
void MainWindow::slotOptionsEdgesBezier(bool toggle){
    if ( !activeNodes() ) {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage( tr("Toggle edges bezier. Please wait...") );
    // //	graphicsWidget->setBezier(toggle);
    if (!toggle) 	{
        // 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        // 		QList<QGraphicsItem *> list = scene->items();
        // 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++) {
        // 			if ( (*item)->type() ==TypeEdge ){
        // 				GraphicsEdge *edge = (GraphicsEdge*) (*item);
        // //				edge->toggleBezier(false);
        // 				(*item)->hide();(*item)->show();
        // 			}
        //
        // 		}
        // 		QApplication::restoreOverrideCursor();
        // 		return;
    }
    else{
        // 		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        // 		QList<QGraphicsItem *> list = scene->items();
        // 		for (QList<QGraphicsItem *>::iterator item=list.begin();item!=list.end(); item++){
        // 			if ( (*item)->type() ==TypeEdge ){
        // 				GraphicsEdge *edge = (GraphicsEdge*) (*item);
        // //				edge->toggleBezier(true);
        // 				(*item)->hide();(*item)->show();
        // 			}
        // 		}
        // 		QApplication::restoreOverrideCursor();
    }

}


/**
 * @brief MainWindow::slotOptionsEdgeThicknessPerWeight
 * @param toggle
 */
void MainWindow::slotOptionsEdgeThicknessPerWeight(bool toggle) {
    if (toggle) {

    }
    else {

    }
}






/**
 * @brief Changes the distance of edge arrows from nodes
 * Called from Edit menu option and DialogSettings
 * if offset=0, asks the user to enter a new offset
 * if v1=0 and v2=0, it changes all edges
 * @param v1
 * @param v2
 * @param offset
 */
void MainWindow::slotOptionsEdgeOffsetFromNode(const int &offset, const int &v1, const int &v2) {
    bool ok=false;
    qDebug() << "MW::slotOptionsEdgeOffsetFromNode - new offset " << offset;
    int newOffset=offset;

    if (!newOffset) {
        newOffset = QInputDialog::getInt(
                    this, "Change edge offset",
                    tr("Change all edges offset from their nodes to: (1-16)"),
                    appSettings["initNodeLabelDistance"].toInt(0,10), 1, 16, 1, &ok );
        if (!ok) {
            statusMessage( tr("Change edge offset aborted.") );
            return;
        }
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (v1 && v2) { //change one edge offset only
        graphicsWidget->setEdgeOffsetFromNode(v1,v2,newOffset);
    }
    else { //change all
        appSettings["initEdgeOffsetFromNode"] = QString::number(newOffset);
        graphicsWidget->setEdgeOffsetFromNode(v1,v2,newOffset);
    }

    QApplication::restoreOverrideCursor();

    statusMessage( tr("Changed edge offset from nodes.") );
}




/**
 * @brief
 * Turns on/off displaying edge weight numbers
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightNumbersVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsEdgeWeightNumbersVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges Weights. Please wait...") );
    appSettings["initEdgeWeightNumbersVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setEdgeWeightNumbersVisibility(toggle);
    activeGraph->edgeWeightNumbersVisibilitySet(toggle);
    optionsEdgeWeightNumbersAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Edge weights are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Edge weights are visible again...") );
    }
    QApplication::restoreOverrideCursor();

}







/**
 * @brief MainWindow::slotOptionsEdgeLabelsVisibility
 * Turns on/off displaying edge labels
 * @param toggle
 */
void MainWindow::slotOptionsEdgeLabelsVisibility(bool toggle) {
    qDebug() << "MW::slotOptionsEdgeLabelsVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle Edges Labels. Please wait...") );

    appSettings["initEdgeLabelsVisibility"] = (toggle) ? "true":"false";
    graphicsWidget->setEdgeLabelsVisibility(toggle);
    activeGraph->edgeLabelsVisibilitySet(toggle);
    optionsEdgeLabelsAct->setChecked ( toggle );
    if (!toggle) {
        statusMessage( tr("Edge labels are invisible now. "
                          "Click the same option again to display them.") );
    }
    else{
        statusMessage( tr("Edge labels are visible again...") );
    }
    QApplication::restoreOverrideCursor();

}


/**
 * @brief Turns on/off saving zero-edge edge weights (only for GraphML at the moment)
 * @param toggle
 */
void MainWindow::slotOptionsSaveZeroWeightEdges(bool toggle) {
    qDebug() << "MW::slotOptionsSaveZeroWeightEdges - Toggling saving zero weight edges";
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    statusMessage( tr("Toggle zero-weight edges saving. Please wait...") );

    appSettings["saveZeroWeightEdges"] = (toggle) ? "true":"false";

    if (toggle) {
        statusMessage( tr("Zero-weight edges will be saved to graphml files. ") );
    }
    else{
        statusMessage( tr("Zero-weight edges will NOT be saved to graphml files.") );
    }
    QApplication::restoreOverrideCursor();

}


/**
 * @brief Turns opengl on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasOpenGL(const bool &toggle) {
    statusMessage( tr("Toggle openGL. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    //Inform graphicsWidget about the change
    graphicsWidget->setOptionsOpenGL(toggle);

    if (!toggle) {
        appSettings["opengl"] = "false";
        statusMessage( tr("Using openGL off.") );
    }
    else {
        appSettings["opengl"] = "true";
        statusMessage( tr("Using OpenGL on.") );
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns antialiasing on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasing(bool toggle) {

    qDebug()<< "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage( tr("Toggle anti-aliasing. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    graphicsWidget->setOptionsAntialiasing(toggle);

    if (!toggle) {
        appSettings["antialiasing"] = "false";
        statusMessage( tr("Anti-aliasing off.") );
    }
    else {
        appSettings["antialiasing"] = "true";
        statusMessage( tr("Anti-aliasing on.") );
    }
    QApplication::restoreOverrideCursor();
}



/**
 * @brief Turns antialiasing auto-adjustment on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasingAutoAdjust(const bool &toggle) {

    qDebug()<< "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage( tr("Toggle anti-aliasing auto adjust. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    graphicsWidget->setOptionsNoAntialiasingAutoAdjust(toggle);

    if (!toggle) {
        appSettings["canvasAntialiasingAutoAdjustment"] = "false";
        statusMessage( tr("Antialiasing auto-adjustment off.") );
    }
    else {
        appSettings["canvasAntialiasingAutoAdjustment"] = "true";
        statusMessage( tr("Antialiasing auto-adjustment on.") );
    }

    QApplication::restoreOverrideCursor();
}



/**
 * @brief Turns smooth pixmap transformations on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSmoothPixmapTransform(const bool &toggle) {

    qDebug()<< "MW::slotOptionsCanvasSmoothPixmapTransform() " << toggle;

    statusMessage( tr("Toggle smooth pixmap transformations. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (!toggle) {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "false";
        statusMessage( tr("Smooth pixmap transformations off.") );
    }
    else {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "true";
        statusMessage( tr("Smooth pixmap transformations on.") );
    }

    QApplication::restoreOverrideCursor();
}




/**
 * @brief Turns saving painter state on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSavePainterState(const bool &toggle) {

    qDebug()<< "MW::slotOptionsCanvasSavePainterState() " << toggle;

    statusMessage( tr("Toggle saving painter state. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (!toggle) {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
        appSettings["canvasPainterStateSave"] = "false";
        statusMessage( tr("Saving painter state off.") );
    }
    else {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
        appSettings["canvasPainterStateSave"] = "true";
        statusMessage( tr("Saving painter state on.") );
    }

    QApplication::restoreOverrideCursor();
}




/**
 * @brief Turns caching of canvas background on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasCacheBackground(const bool &toggle) {

    qDebug()<< "MW::slotOptionsCanvasCacheBackground() " << toggle;

    statusMessage( tr("Toggle canvas background caching state. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (!toggle) {
        graphicsWidget->setCacheMode(QGraphicsView::CacheNone);
        appSettings["canvasCacheBackground"] = "false";
        statusMessage( tr("Canvas background caching  off.") );
    }
    else {
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        appSettings["canvasCacheBackground"] = "true";
        statusMessage( tr("Canvas background caching  on.") );
    }

    QApplication::restoreOverrideCursor();
}






/**
 * @brief Turns selected edge highlighting
 * @param toggle
 */
void MainWindow::slotOptionsCanvasEdgeHighlighting(const bool &toggle) {

    qDebug()<< "MW::slotOptionsCanvasEdgeHighlighting() " << toggle;

    statusMessage( tr("Toggle edge highlighting state. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (!toggle) {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "false";
        statusMessage( tr("Edge highlighting off.") );
    }
    else {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "true";
        statusMessage( tr("Edge highlighting on.") );
    }

    QApplication::restoreOverrideCursor();
}





/**
 * @brief Sets canvas update mode
 * @param toggle
 */
void MainWindow::slotOptionsCanvasUpdateMode(const QString &mode) {

    qDebug()<< "MW::slotOptionsCanvasUpdateMode() " << mode;

    statusMessage( tr("Setting canvas update mode. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if ( mode == "Full" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::FullViewportUpdate );
    }
    else if ( mode == "Minimal" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::MinimalViewportUpdate );
    }
    else if ( mode == "Smart" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::SmartViewportUpdate );
    }
    else if ( mode == "Bounding Rectangle" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::BoundingRectViewportUpdate );
    }
    else if ( mode == "None" ) {
        graphicsWidget->setViewportUpdateMode( QGraphicsView::NoViewportUpdate );
    }
    else { //
        graphicsWidget->setViewportUpdateMode( QGraphicsView::MinimalViewportUpdate );
    }

    appSettings["canvasUpdateMode"] = mode;

    statusMessage( tr("Canvas update mode: ") + mode );


    QApplication::restoreOverrideCursor();
}





/**
 * @brief Changes the indexing method of the graphics scene.
 *
 * Called from Settings dialog.
 *
 * @param method
 */
void MainWindow::slotOptionsCanvasIndexMethod(const QString &method) {

    qDebug()<< "Changing graphics scene index method to:" << method;

    statusMessage( tr("Setting canvas index method. Please wait...") );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if ( method == "BspTreeIndex" ) {  // Qt default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    }
    else if ( method == "NoIndex" ) {  // for animated scenes
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    }
    else { // default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }

    appSettings["canvasIndexMethod"] = method;

    statusMessage( tr("Canvas index method: ") + method );

    QApplication::restoreOverrideCursor();
}







/**
 * @brief MainWindow::slotOptionsEmbedLogoExporting
 *
 * @param toggle
 */
void MainWindow::slotOptionsEmbedLogoExporting(bool toggle){
    if (!toggle) {
        statusMessage( tr("SocNetV logo print off.") );
        appSettings["printLogo"] = "false";
    }
    else {
        appSettings["printLogo"] = "true";
        statusMessage( tr("SocNetV logo print on.") );
    }
}

/**
 * @brief Turns progress dialogs on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsProgressDialogVisibility(bool toggle) {
    statusMessage( tr("Toggle progressbar..."));
    if (!toggle)  {
        appSettings["showProgressBar"] = "false";
        statusMessage( tr("Progress bars off.") );
    }
    else   {
        appSettings["showProgressBar"] = "true";
        statusMessage( tr("Progress bars on.") );
    }
}



/**
 * @brief
 * Turns debugging messages on or off
 * @param toggle
 */
void MainWindow::slotOptionsDebugMessages(bool toggle){
    if (!toggle)   {
        qDebug()<<"Disabling debugging messages";
        appSettings["printDebug"] = "false";
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                             "socnetv.debug=false");
        statusMessage( tr("Debug messages off.") );
    }
    else  {
        appSettings["printDebug"] = "true";
        QLoggingCategory::setFilterRules("default.debug=true\n"
                                             "socnetv.debug=true");
        qDebug()<<"Enabled debugging messages";
        statusMessage( tr("Debug messages on.") );
    }
}




/**
 * @brief
 * Called from Options menu and Settings dialog
 * @param color QColor
 */
void MainWindow::slotOptionsBackgroundColor (QColor color){

    if (!color.isValid()) {
        color = QColorDialog::getColor( QColor ( appSettings["initBackgroundColor"] ),
                this,
                "Change the background color" );
    }
    if (color.isValid()) {
        appSettings["initBackgroundColor"] = color.name();
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        graphicsWidget->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"]))
                );
        QApplication::restoreOverrideCursor();
        statusMessage( tr("Background changed.")  );
    }
    else {
        // user pressed Cancel
        statusMessage( tr("Invalid color. ") );
    }

}


/**
 * @brief
 * Toggles displaying a custom image in the background
 * If toggle = true, presents a dialog to select an image file
 * Called from app menu option
 * @param toggle
 */
void MainWindow::slotOptionsBackgroundImageSelect(bool toggle) {
    statusMessage( tr("Toggle BackgroundImage..."));
    QString m_fileName ;
    if (toggle == false)   {
        statusMessage( tr("BackgroundImage off.") );
        graphicsWidget->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"] ) )
                );
    }
    else   {
        m_fileName = QFileDialog::getOpenFileName(
                    this, tr("Select one image"), getLastPath(),
                    tr("Images (*.png *.jpg *.jpeg);;All (*.*)")
                    );
        if (m_fileName.isNull() )
            appSettings["initBackgroundImage"] = "";
        appSettings["initBackgroundImage"] = m_fileName;
        slotOptionsBackgroundImage();
    }
}



/**
 * @brief
 * Enables/disables displaying a user-defined custom image in the background
 * Called from Settings Dialog and
 */
void MainWindow::slotOptionsBackgroundImage() {
    statusMessage( tr("Toggle BackgroundImage..."));
    if (appSettings["initBackgroundImage"].isEmpty())   {
        statusMessage( tr("BackgroundImage off.") );
        graphicsWidget->setBackgroundBrush(
                    QBrush(QColor (appSettings["initBackgroundColor"] ) )
                );
    }
    else   {
        setLastPath(appSettings["initBackgroundImage"]);
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage( tr("BackgroundImage on.") );
    }


}



/**
 * @brief Toggles full screen mode (F11)
 * @param toggle
 */
void MainWindow::slotOptionsWindowFullScreen(bool toggle) {
    if (toggle== false)   {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage( tr("Full screen mode off. Press F11 again to enter full screen.") );
    } else {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage( tr("Full screen mode on. Press F11 again to exit full screen.") );
    }
}

/**
 * @brief Turns Toolbar on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsWindowToolbarVisibility(bool toggle) {
    statusMessage( tr("Toggle toolbar..."));
    if (toggle== false)   {
        toolBar->hide();
        appSettings["showToolBar"] = "false";
        statusMessage( tr("Toolbar off.") );
    }
    else  {
        toolBar->show();
        appSettings["showToolBar"] = "true";
        statusMessage( tr("Toolbar on.") );
    }
}




/**
 * @brief Turns window statusbar on or off
 * @param toggle
 */
void MainWindow::slotOptionsWindowStatusbarVisibility(bool toggle) {
    statusMessage( tr("Toggle statusbar..."));

    if (toggle == false)   {
        statusBar()->hide();
        appSettings["showStatusBar"] = "false";
        statusMessage( tr("Status bar off.") );
    }
    else   {
        statusBar()->show();
        appSettings["showStatusBar"] = "true";
        statusMessage( tr("Status bar on.") );
    }

}


/**
 * @brief Toggles left panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowLeftPanelVisibility(bool toggle) {
    statusMessage( tr("Toggle left panel..."));

    if (toggle == false)   {
        leftPanel->hide();
        appSettings["showLeftPanel"] = "false";
        statusMessage( tr("Left Panel off.") );
    }
    else   {
        leftPanel->show();
        appSettings["showLeftPanel"] = "true";
        statusMessage( tr("Left Panel on.") );
    }

}


/**
 * @brief Toggles right panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowRightPanelVisibility(bool toggle) {
    statusMessage( tr("Toggle left panel..."));

    if (toggle == false)   {
        rightPanel->hide();
        appSettings["showRightPanel"] = "false";
        statusMessage( tr("Right Panel off.") );
    }
    else   {
        rightPanel->show();
        appSettings["showRightPanel"] = "true";
        statusMessage( tr("Right Panel on.") );
    }

}




/**
 * @brief Toggles the use of our own Qt StyleSheet
 *
 * The .qss file is defined in project resources
 *
 * @param checked
 */
void MainWindow::slotOptionsCustomStylesheet(const bool checked = true ){
    if ( checked ) {
        slotStyleSheetByName(":/qss/default.qss");
        appSettings["useCustomStyleSheet"] = "true";
    }
    else {
        slotStyleSheetByName("");
        appSettings["useCustomStyleSheet"] = "false";
    }
}



/**
 * @brief Loads a custom Qt StyleSheet (.qss file)
 *
 * If sheetFileName is empty, the app uses platform-specific Qt style
 *
 * @param sheetFileName
 */
void MainWindow::slotStyleSheetByName(const QString &sheetFileName) {

    qDebug() << "Opening stylesheet file: "<< sheetFileName;

    QString styleSheet = "";

    if ( !sheetFileName.isEmpty() ) {

        QFile file(sheetFileName);

        if (!file.open(QFile::ReadOnly)) {
            qDebug () << "Could not open (for reading) file:" << sheetFileName;
            slotHelpMessageToUserError(
                        tr("Cannot read stylesheet file %1:\n%2")
                        .arg(sheetFileName).arg(file.errorString())
                        );
            return;
        }
        styleSheet = QString::fromLatin1(file.readAll());
    }
    qApp->setStyleSheet(styleSheet);
}


/**
*  Displays a random tip
*/
void MainWindow::slotHelpTips() {
    int randomTip=rand() % (tips.size()); //Pick a tip.
    QMessageBox::about( this, tr("Tip Of The Day"), tips[randomTip]);
}



/**
    Creates our tips.
*/
void MainWindow::slotHelpCreateTips(){
    tips+=tr("To create a new node: \n"
             "- double-click somewhere on the canvas \n"
             "- or press the keyboard shortcut CTRL+. (dot)\n"
             "- or press the Add Node button on the left panel");
    tips+=tr("SocNetV can work with either undirected or directed data. "
             "When you start SocNetV for the first time, the application uses "
             "the 'directed data' mode; every edge you create is directed. "
             "To enter the 'undirected data' mode, press CTRL+E+U or enable the "
             "menu option Edit->Edges->Undirected Edges ");
    tips+=tr("If your screen is small, and the canvas appears even smaller "
             "hide the Control and/or Statistics panel. Then the canvas "
             "will expand to the whole application window. "
             "Open the Settings/Preferences dialog->Window options and "
             "disable the two panels.");
    tips+=tr("A scale-free network is a network whose degree distribution follows a power law. "
             "SocNetV generates random scale-free networks according to the "
             "Barabási–Albert (BA) model using a preferential attachment mechanism.");
    tips+=tr("To delete a node permanently: \n"
             "- right-click on it and select Remove Node \n"
             "- or press CTRL+ALT+. and enter its number\n"
             "- or press the Remove Node button on the Control Panel");
    tips+=tr("To rotate the network: \n"
             " - drag the bottom slider to left or right \n"
             " - or click the buttons on the corners of the bottom slider\n"
             " - or press CTRL and the left or right arrow.");
    tips+=tr("To create a new edge between nodes A and B: \n"
             "- double-click on node A, then double-click on node B.\n"
             "- or middle-click on node A, and again on node B.\n"
             "- or right-click on the node, then select Add Edge from the popup.\n"
             "- or press the keyboard shortcut CTRL+/ \n"
             "- or press the Add Edge button on the Control Panel");
    tips+=tr("Add a label to an edge by right-clicking on it "
             "and selecting Change Label.");
    tips+=tr("You can change the background color of the canvas. "
             "Do it from the menu Options > View or "
             "permanently save this setting in Settings/Preferences.");
    tips+=tr("Default node colors, shapes and sizes can be changed. "
             "Open the Settings/Preferences dialog and use the "
             "options on the Node tab.");
    tips+=tr("The Statistics Panel shows network-level information (i.e. density) "
             "as well as info about any node you clicked on (inDegrees, "
             "outDegrees, clustering).");
    tips+=tr("You can move any node by left-clicking and dragging it with your mouse. "
             "If you want you can move multiple nodes at once. Left-click on empty space "
             "on the canvas and drag to create a rectangle selection around them. "
             "Then left-click on one of the selected nodes and drag it.");
    tips+=tr("To save the node positions in a network, you need to save your data "
             "in a format which supports node positions, suchs as GraphML or Pajek.");
    tips+=tr("Embed visualization models on the network from the options in "
             "the Layout menu or the select boxes on the left Control Panel. ");
    tips+=tr("To change the label of a node right-click on it, and click "
             "Selected Node Properties from the popup menu.");
    tips+=tr("All basic operations of SocNetV are available from the left Control panel "
             "or by right-clicking on a Node or an Edge or on canvas empty space.");
    tips+=tr("Node info (number, position, degree, etc) is displayed on the Status bar, "
             "when you left-click on it.");
    tips+=tr("Edge information is displayed on the Status bar, when you left-click on it.");
    tips+=tr("Save your work often, especially when working with large data sets. "
             "SocNetV alogorithms are faster when working with saved data. ");

    tips+=tr("You can change the precision of real numbers in reports.  "
             "Go to Settings > General and change it under Reports > Real number precision. ");

    tips+=tr("The Closeness Centrality (CC) of a node v, is the inverse sum of "
             "the shortest distances between v and every other node. CC is "
             "interpreted as the ability to access information through the "
             "\'grapevine\' of network members. Nodes with high closeness "
             "centrality are those who can reach many other nodes in few steps. "
             "This index can be calculated in both graphs and digraphs. "
             "It can also be calculated in weighted graphs although the weight of "
             "each edge (v,u) in E is always considered to be 1. ");

    tips+=tr("The Information Centrality (IC) index counts all paths between "
             "nodes weighted by strength of tie and distance. "
             "This centrality  measure developed by Stephenson and Zelen (1989) "
             "focuses on how information might flow through many different paths. "
             "This index should be calculated only for undirected graphs. "
             "Note: To compute this index, SocNetV drops all isolated nodes.");

}





/**
 * @brief
 * Opens the system web browser to load the online Manual
 */
void MainWindow::slotHelp(){
    statusMessage( tr("Opening the SocNetV Manual in your default web browser....") );
    QDesktopServices::openUrl(QUrl("https://socnetv.org/docs/index.html"));
}




/**
 * @brief On user demand, makes a network request to SocNetV website to
 * download the latest version text file.
 */
void MainWindow::slotHelpCheckUpdateDialog() {

    QString url = "https://socnetv.org/latestversion.txt";

    qDebug() << "Will make a 'check for updates' request to url:" << url;

    slotNetworkManagerRequest(QUrl(url), NetworkRequestType::CheckUpdate);

}




/**
 * @brief Parses the reply from the network request we do in slotHelpCheckUpdateDialog
 * @param reply
 */
void MainWindow::slotHelpCheckUpdateParse() {

    qDebug() << "MW::slotHelpCheckUpdateParse()";

    // Get network reply from the sender
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    QByteArray ba = reply->readAll();       // read reply

    reply->deleteLater();                   // schedule reply to be deleted

    QString replyContent(ba);

    // Simplify the reply
    replyContent = replyContent.simplified();

    // Check reply if empty
    if (replyContent.isEmpty()) {
        slotHelpMessageToUserError("Empty response from https://socnetv.org. "
                                   "Could not get the latest version number. Please try again later...");
        return;
    }

    // Store the remote version string
    QString remoteVersion = replyContent;

    bool ok1=false;

    // Remove . from reply content and convert it to integer
    int remoteVersionInt = (replyContent.remove(".")).toInt(&ok1, 10);

    qDebug() << "MW::slotHelpCheckUpdateParse() - remoteVersionInt:" << remoteVersionInt;

    // Test if it has been converted
    if (!ok1) {
        slotHelpMessageToUserError("Could not understand the version number I got from https://socnetv.org. "
                                   "Please, try again later...");
        return;

    }

    // Get local version - we need this to compare the remote against it.
    QString localVersionStr = VERSION;

    // Check if we are on a beta/dev
    if (localVersionStr.contains("beta")) {
        localVersionStr.remove("beta");
        localVersionStr.remove("-");
    }
    else if (localVersionStr.contains("rc")) {
        localVersionStr.remove("rc");
        localVersionStr.remove("-");
    }
    else if (localVersionStr.contains("dev")) {
        localVersionStr.remove("dev");
        localVersionStr.remove("-");
    }

    // Remove . from local version and convert it to integer
    int localVersionInt = (localVersionStr.remove(".")).toInt(&ok1, 10);

    qDebug() << "MW::slotHelpCheckUpdateParse() - localVersionInt:" << localVersionInt;

    // Test if it has been converted
    if (!ok1) {
        slotHelpMessageToUserError("Error in current version string. "
                                   "Please, contact our developer team.");
        return;
    }


    // Compare integer versions
    if( remoteVersionInt > localVersionInt ) {

        switch( slotHelpMessageToUser(
                    USER_MSG_QUESTION,
                    tr("Newer SocNetV version available!"),
                    tr("<p>Your version: ")+ VERSION+ "</p><p>" +
                    tr("<p>Remote version: <b>")+replyContent + "</b></p>",
                    tr("<p><b>There is a newer SocNetV version available! </b></p>"
                       "<p>Do you want to download the latest version now? </p> "
                       "<p>Press Yes, and I will open your default web browser for you "
                       "to download the latest SocNetV package...</p>"),
                    QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes
                    ) )

        {
        case QMessageBox::Yes:
            statusMessage( tr("Opening SocNetV website in your default web browser....") );
            QDesktopServices::openUrl(QUrl
                                      ("https://socnetv.org/downloads"
                                       "?utm_source=application&utm_medium=banner&utm_campaign=socnetv"+ VERSION
                                       ));
            break;
        case QMessageBox::No:
            break;
        case QMessageBox::Cancel:
            //userCancelled = true;
            break;
        case QMessageBox::NoButton:
        default: // just for sanity
            break;
        }
    }
    else {
        slotHelpMessageToUserInfo(
                    tr("<p>Your version: ")+ VERSION+ "</p>" +
                    tr("<p>Remote version: ")+remoteVersion + "</p>" +
                    tr("<p>You are running the latest and greatest version of SocNetV. <br />"
                       "Nothing to do!</p>")
                    );
    }


}




/**
 * @brief Shows a dialog with system information for bug reporting purposes
 */
void MainWindow::slotHelpSystemInfo() {
    qDebug() << "MW: slotHelpSystemInfo()";

    m_systemInfoDialog = new DialogSystemInfo(this);

    m_systemInfoDialog->exec() ;
}




/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout(){
    int randomCookie=rand()%fortuneCookie.size();

    QMessageBox::about(
                this, tr("About SocNetV"),
                tr("<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)") +
                tr("<p><b>Version</b>: ") + VERSION + "</p>" +

                tr("<p>Website: <a href=\"https://socnetv.org\">https://socnetv.org</a></p>")+

                tr("<p>(C) 2005-2023 by Dimitris V. Kalamaras</p>")+
                tr("<p><a href=\"https://socnetv.org/contact\">Have questions? Contact us!</a></p>")+

                tr("<p><b>Fortune cookie: </b><br> \"")  + fortuneCookie[randomCookie]  + "\"" +

                tr("<p><b>License:</b><p>") +

                tr("<p>This program is free software; you can redistribute it "
                   "and/or modify it under the terms of the GNU General "
                   "Public License as published by the Free Software Foundation; "
                   "either version 3 of the License, or (at your option) "
                   "any later version.</p>") +

                tr("<p>This program is distributed in the hope that it "
                   "will be useful, but WITHOUT ANY WARRANTY; "
                   "without even the implied warranty of MERCHANTABILITY "
                   "or FITNESS FOR A PARTICULAR PURPOSE. "
                   "See the GNU General Public License for more details.</p>") +

                tr("<p>You should have received a copy of the GNU "
                   "General Public License along with this program; "
                   "If not, see http://www.gnu.org/licenses/</p>"));
}



/**
    Creates the fortune cookies displayed on the above message.
*/
void MainWindow::createFortuneCookies(){
    fortuneCookie+="sic itur ad astra / sic transit gloria mundi ? <br /> "
                   "--Unknown";
    fortuneCookie+="The truth is not my business. I am a statistician... I don’t like words like \"correct\" and \"truth\". "
                   "Statistics is about measuring against convention. <br /> "
                   "Walter Radermacher, Eurostat director, interview to NY Times, 2012.";
    fortuneCookie+="Losers of yesterday, the winners of tomorrow... <br /> "
                   "--B.Brecht";
    fortuneCookie+="I've seen things you people wouldn't believe. Attack ships on fire off the shoulder of Orion. "
                   "I watched C-beams glitter in the dark near the Tannhauser gate. "
                   "All those moments will be lost in time... like tears in rain... Time to die.<br />"
                   "Replicant Roy Batty, Blade Runner (1982)";
    fortuneCookie+="Patriotism is the virtue of the wicked... <br /> "
                   "--O. Wilde";
    fortuneCookie+="No tengo nunca mas, no tengo siempre. En la arena <br />"
                   "la victoria dejo sus piers perdidos.<br />"
                   "Soy un pobre hombre dispuesto a amar a sus semejantes.<br />"
                   "No se quien eres. Te amo. No doy, no vendo espinas. <br /> "
                   "--Pablo Neruda"  ;
    fortuneCookie+="Man must not check reason by tradition, but contrawise, "
                   "must check tradition by reason.<br> --Leo Tolstoy";
    fortuneCookie+="Only after the last tree has been cut down, <br>"
                   "only after the last river has been poisoned,<br> "
                   "only after the last fish has been caught,<br>"
                   "only then will you realize that money cannot be eaten. <br> "
                   "--The Cree People";
    fortuneCookie+="Stat rosa pristina nomine, nomina nuda tenemus <br >"
                   " --Unknown";
    fortuneCookie+="Jupiter and Saturn, Oberon, Miranda <br />"
                   "And Titania, Neptune, Titan. <br />"
                   "Stars can frighten. <br /> Syd Barrett";

}




/**
    Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt(){
    QMessageBox::aboutQt(this, "About Qt - SocNetV");
}



