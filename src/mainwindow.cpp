/**
 * @file mainwindow.cpp
 * @brief Implements the MainWindow class, which serves as the primary interface for the SocNetV application.
 * @details This file contains the logic for the main application window, including menus, toolbars, and user interactions for network visualization and analysis.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
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
#include <QSignalBlocker>
#include <QElapsedTimer>
#include <memory>

#include <QtSvg> // for SVG icons
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
#include <QUrl>

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
#include "forms/dialogedgeedit.h"
#include "forms/dialogbulkedit.h"
#include "forms/dialogfilternodesbycentrality.h"
#include "forms/dialogfilterbyattribute.h"
#include "forms/dialogquerybuilder.h"
#include "widgets/filterbarwidget.h"
#include "widgets/graphtablewidget.h"
#include "graph/io/table_export.h"
#include <QDockWidget>
#include "forms/dialogfilteredgesbyweight.h"
#include "forms/dialogedgedichotomization.h"
#include "forms/dialogsimilaritypearson.h"
#include "forms/dialogsimilaritymatches.h"
#include "forms/dialogclusteringhierarchical.h"
#include "forms/dialogdissimilarities.h"
#include "forms/dialogsettings.h"
#include "forms/dialogsysteminfo.h"

Q_LOGGING_CATEGORY(lcMainWindow, "socnetv.mainwindow")

/**
 * @brief Constructs the MainWindow (MW) object
 *
 * @param m_fileName
 * @param forceProgress
 * @param maximized
 * @param fullscreen
 * @param debugLevel
 */
MainWindow::MainWindow(const QString &m_fileName, const bool &forceProgress, const bool &maximized, const bool &fullscreen, const int &debugLevel,
                       const QString &encodingOverride, const QString &interactiveScriptPath)
{
    m_encodingOverride = encodingOverride;

    qCDebug(lcMainWindow) << "=========== MainWindow (MW) constructor starting on thread:" << thread();

    //
    // Setup debug messages/level
    //
    switch (debugLevel)
    {
    case 0:
    default:
        // Debugging disabled by command line parameter (case 0), or no -d flag at all
        // (default -- WS14 L1: a no-flag launch is release-build-quiet by design, same
        // as an explicit -d 0, not just "whatever was last saved to settings.conf"; see
        // the matching fix in initSettings() below for why that second part matters).
        // Bare message, no decoration -- NOT an empty pattern. A truly empty pattern has
        // no %{message} token, so Qt's formatter reduces every message to an empty string
        // regardless of severity or category, silently swallowing qInfo()/qWarning()/
        // qCritical() output too, not just the qDebug() spam this is meant to suppress
        // (found while wiring up WS14's own qInfo()-based benchmark instrumentation,
        // which went dark under the old "" pattern for exactly this reason).
        qSetMessagePattern("%{message}");
        // Disable debugging messages with filter rule
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                         "socnetv.*.debug=false");
        break;
    case 1:
        // Debugging set to minimum by command line parameter
        qSetMessagePattern("[%{type}] (%{file}:%{line}) %{function} - %{message}");
        break;
    case 2:
        // Debugging set to maximum by command line parameter
        qSetMessagePattern("[%{type} %{category}] %{time yyyyMMdd h:mm:ss.zzz t} %{threadid} (%{file}:%{line}) %{function} - %{message}");
        break;
    }

    //
    // Setup window icon
    //
    setWindowIcon(QIcon(":/images/socnetv_logo_white_bg_128px.svg"));

    //
    // Initialize/load app settings and store them to memory
    //
    appSettings = initSettings(debugLevel, forceProgress);

    //
    // Initialize minimum app window  size
    //

    // Get host screen width and height
    int primaryScreenWidth = QApplication::primaryScreen()->availableSize().width();
    int primaryScreenHeight = QApplication::primaryScreen()->availableSize().height();

    // Set a default min width and height
    int windowMinWidth = 1024;
    int windowMinHeight = 750;

    // For large screens, use more generous min height and width.
    if (primaryScreenWidth >= 1920)
    {
        windowMinWidth = 1440;
    }
    else if (primaryScreenWidth >= 1280)
    {
        windowMinWidth = 1024;
    }
    if (primaryScreenHeight >= 1440)
    {
        windowMinHeight = 960;
    }
    else if (primaryScreenHeight >= 1024)
    {
        windowMinHeight = 800;
    }

    qCDebug(lcMainWindow) << "primaryScreen: " << primaryScreenWidth << "x" << primaryScreenHeight
             << "Set Minimum MW size to:" << windowMinWidth << "x" << windowMinHeight;

    // Set MW minimum size, before creating the graphics widget
    setMinimumSize(windowMinWidth, windowMinHeight);

    //
    // Initialize devices
    //
    qCDebug(lcMainWindow) << "Initialize devices...";

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
    qCDebug(lcMainWindow) << "Setup canvas, graph, widgets (actions, menus, panels, signal/slots) and init app...";
    initView(); // Init our network view

    initGraph(); // Init the graph model

    initActions(); // Register and construct menu Actions

    initMenuBar(); // Construct the menu

    initToolBar(); // Build the toolbar

    initPanels(); // Build the toolbox

    initWindowLayout(); // Init the application window, set layout etc

    initSignalSlots(); // Connect signals and slots between app components

    if (maximized)
    {
        qCDebug(lcMainWindow) << "maximizing window as per user request.";
        showMaximized();
    }
    if (fullscreen)
    {
        showFullScreen();
    }

    initApp(); // Load and initialise default app parameters

    graphicsWidget->setFocus();

    //
    // Load user-provided network file, if any
    //
    qCDebug(lcMainWindow) << "Checking if user provided file on startup...";
    if (!m_fileName.isEmpty())
    {
        qCDebug(lcMainWindow) << "Loading user provided file" << m_fileName;
        slotNetworkFileChoose(m_fileName);
    }

    QString welcomeMsg = tr("Welcome to %1, version %2").arg(qApp->applicationName(), VERSION);

    statusMessage(welcomeMsg);

    if (!interactiveScriptPath.isEmpty())
    {
        runInteractiveScript(interactiveScriptPath);
    }

    qCDebug(lcMainWindow) << "@@@@ MW Constructor finished, on thread:" << thread();
}

/**
 * @brief Deletes variables on MW closing
 */
MainWindow::~MainWindow()
{

    qCDebug(lcMainWindow) << "Destructor for MW running...";

    // Init app to clear all maps etc.
    initApp();

    // Terminate any threads running
    terminateThreads("~MainWindow()");

    // Delete devices
    delete printer;
    delete printerPDF;

    delete scene;
    delete graphicsWidget;

    foreach (TextEditor *ed, m_textEditors)
    {
        ed->close();
        delete ed;
    }

    m_textEditors.clear();

    codecs.clear();

    qCDebug(lcMainWindow) << "Destruct function finished - bye!";
}

































































































































































/**
*	Transforms all nodes to edges
    TODO slotEditTransformNodes2Edges
*/
























/**
 *	Displays the arc and dyad reciprocity of the network
 */

/**
 *	Displays a box informing the user about the symmetry or not of the adjacency matrix
 */



































/**
 *	Writes Betweenness Centralities into a file, then displays it.
 */





























































