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
 * @brief Opens the Settings dialog
 */
void MainWindow::slotOpenSettingsDialog()
{

    // build dialog

    m_settingsDialog = new DialogSettings(appSettings, nodeShapeList, iconPathList, this);

    connect(m_settingsDialog, &DialogSettings::saveSettings,
            this, &MainWindow::saveSettings);

    connect(m_settingsDialog, &DialogSettings::setReportsDataDir,
            activeGraph, &Graph::setReportsDataDir);

    connect(m_settingsDialog, &DialogSettings::setReportsRealNumberPrecision,
            activeGraph, &Graph::setReportsRealNumberPrecision);

    connect(m_settingsDialog, &DialogSettings::setReportsLabelLength,
            activeGraph, &Graph::setReportsLabelLength);

    connect(m_settingsDialog, &DialogSettings::setReportsChartType,
            activeGraph, &Graph::setReportsChartType);

    connect(m_settingsDialog, &DialogSettings::setReportsOutputFormat,
            activeGraph, &Graph::setReportsOutputFormat);

    connect(m_settingsDialog, &DialogSettings::setDebugMsgs,
            this, &MainWindow::slotOptionsDebugMessages);

    connect(m_settingsDialog, &DialogSettings::setProgressDialog,
            this, &MainWindow::slotOptionsProgressDialogVisibility);

    connect(m_settingsDialog, &DialogSettings::setPrintLogo,
            this, &MainWindow::slotOptionsEmbedLogoExporting);

    connect(m_settingsDialog, &DialogSettings::setCustomStylesheet,
            this, &MainWindow::slotOptionsCustomStylesheet);

    connect(m_settingsDialog, &DialogSettings::setToolBar,
            this, &MainWindow::slotOptionsWindowToolbarVisibility);

    connect(m_settingsDialog, &DialogSettings::setStatusBar,
            this, &MainWindow::slotOptionsWindowStatusbarVisibility);

    connect(m_settingsDialog, &DialogSettings::setLeftPanel,
            this, &MainWindow::slotOptionsWindowLeftPanelVisibility);

    connect(m_settingsDialog, &DialogSettings::setRightPanel,
            this, &MainWindow::slotOptionsWindowRightPanelVisibility);

    connect(m_settingsDialog, &DialogSettings::setCanvasBgColor,
            this, &MainWindow::slotOptionsBackgroundColor);

    connect(m_settingsDialog, &DialogSettings::setCanvasBgImage,
            this, &MainWindow::slotOptionsBackgroundImage);

    connect(m_settingsDialog, &DialogSettings::setCanvasOpenGL,
            this, &MainWindow::slotOptionsCanvasOpenGL);

    connect(m_settingsDialog, &DialogSettings::setCanvasAntialiasing,
            this, &MainWindow::slotOptionsCanvasAntialiasing);

    connect(m_settingsDialog, &DialogSettings::setCanvasAntialiasingAutoAdjust,
            this, &MainWindow::slotOptionsCanvasAntialiasingAutoAdjust);

    connect(m_settingsDialog, &DialogSettings::setCanvasSmoothPixmapTransform,
            this, &MainWindow::slotOptionsCanvasSmoothPixmapTransform);

    connect(m_settingsDialog, &DialogSettings::setCanvasSavePainterState,
            this, &MainWindow::slotOptionsCanvasSavePainterState);

    connect(m_settingsDialog, &DialogSettings::setCanvasCacheBackground,
            this, &MainWindow::slotOptionsCanvasCacheBackground);

    connect(m_settingsDialog, &DialogSettings::setCanvasEdgeHighlighting,
            this, &MainWindow::slotOptionsCanvasEdgeHighlighting);

    connect(m_settingsDialog, &DialogSettings::setCanvasUpdateMode,
            this, &MainWindow::slotOptionsCanvasUpdateMode);

    connect(m_settingsDialog, &DialogSettings::setCanvasIndexMethod,
            this, &MainWindow::slotOptionsCanvasIndexMethod);

    connect(m_settingsDialog, SIGNAL(setNodeColor(QColor)),
            this, SLOT(slotEditNodeColorAll(QColor)));

    connect(m_settingsDialog, &DialogSettings::setNodeShape,
            this, &MainWindow::slotEditNodeShape);

    connect(m_settingsDialog, &DialogSettings::setNodeSize,
            this, &MainWindow::slotEditNodeSizeAll);

    connect(m_settingsDialog, &DialogSettings::setNodeNumbersVisibility,
            this, &MainWindow::slotOptionsNodeNumbersVisibility);

    connect(m_settingsDialog, &DialogSettings::setNodeNumbersInside,
            this, &MainWindow::slotOptionsNodeNumbersInside);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberColor,
            this, &MainWindow::slotEditNodeNumbersColor);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberSize,
            this, &MainWindow::slotEditNodeNumberSize);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberDistance,
            this, &MainWindow::slotEditNodeNumberDistance);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelsVisibility,
            this, &MainWindow::slotOptionsNodeLabelsVisibility);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelSize,
            this, &MainWindow::slotEditNodeLabelSize);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelColor,
            this, &MainWindow::slotEditNodeLabelsColor);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelDistance,
            this, &MainWindow::slotEditNodeLabelDistance);

    connect(m_settingsDialog, &DialogSettings::setEdgesVisibility,
            this, &MainWindow::slotOptionsEdgesVisibility);

    connect(m_settingsDialog, &DialogSettings::setEdgesBezier,
            this, &MainWindow::slotOptionsEdgesBezier);

    connect(m_settingsDialog, &DialogSettings::setEdgeArrowsVisibility,
            this, &MainWindow::slotOptionsEdgeArrowsVisibility);
    connect(m_settingsDialog, &DialogSettings::setEdgeArrowSize,
            this, &MainWindow::slotOptionsEdgeArrowSize);

    connect(m_settingsDialog, &DialogSettings::setEdgeOffsetFromNode,
            this, &MainWindow::slotOptionsEdgeOffsetFromNode);

    connect(m_settingsDialog, &DialogSettings::setEdgeColor,
            this, &MainWindow::slotEditEdgeColorAll);

    connect(m_settingsDialog, &DialogSettings::setEdgeWeightNumbersVisibility,
            this, &MainWindow::slotOptionsEdgeWeightNumbersVisibility);

    connect(m_settingsDialog, &DialogSettings::setEdgeLabelsVisibility,
            this, &MainWindow::slotOptionsEdgeLabelsVisibility);

    connect(m_settingsDialog, &DialogSettings::setSaveZeroWeightEdges,
            this, &MainWindow::slotOptionsSaveZeroWeightEdges);

    connect(m_settingsDialog, &DialogSettings::setShowZeroWeightEdges, // #30
            this, &MainWindow::slotOptionsShowZeroWeightEdges);

    // show settings dialog
    m_settingsDialog->exec();
}










/**
 * @brief Loads a plain-text interactive script and starts executing it. See #261.
 *
 * One command per line: 'delay X' (wait X seconds) or 'new' (File > New). Commands run one at a
 * time via processNextInteractiveCommand(), each dispatched through the real Qt event loop so the
 * script behaves like an actual sequence of user actions rather than a tight synchronous loop -
 * and each only advances to the next line once its own dispatched work has genuinely finished,
 * not just been queued (see the dispatch-pattern note on processNextInteractiveCommand() itself).
 *
 * @param scriptPath
 */
void MainWindow::runInteractiveScript(const QString &scriptPath)
{
    QFile file(scriptPath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "Cannot read interactive script:" << scriptPath << file.errorString();
        return;
    }
    QTextStream in(&file);
    m_interactiveScriptLines = in.readAll().split('\n');
    m_interactiveScriptIndex = 0;
    qCDebug(lcMainWindow) << "Loaded interactive script:" << scriptPath
             << "-" << m_interactiveScriptLines.size() << "line(s)";
    QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
}

/**
 * @brief Executes one line of the interactive script, then schedules the next. See #261.
 *
 * Three dispatch shapes recur throughout this function. Whichever one a command uses, the rule is
 * always the same: only advance to the next command (call `processNextInteractiveCommand()`, or
 * queue it via `QTimer::singleShot`/`QMetaObject::invokeMethod`) once this command's own work has
 * genuinely finished - never merely queued or triggered. Getting this wrong is a real,
 * reproducible bug, not a style preference: it lets the next script command race this one's
 * still-in-flight work, found via an out-of-bounds crash where an unwrapped 'quit' right after
 * 'erdos' tore down Graph state while 'erdos' was still creating nodes.
 *
 * - **No dispatch** (`new`, `render`, `bulk-node-size`, `bulk-edge-color`): the work is already a
 *   direct, blocking call on this (GUI) thread, no cross-thread queuing involved - genuinely done
 *   by the time the call returns, so advancing immediately afterward is correct as-is.
 * - **Single-step dispatch** (`relation`, `erdos`, `erdos-m`, `save`, `add-node`, `add-edge`,
 *   `add-relation`, `click-node`, `move`): the work is handed over via
 *   `QMetaObject::invokeMethod(activeGraph, lambda, Qt::QueuedConnection)` - "queue this lambda to
 *   run on activeGraph's thread, whenever that thread is free." `invokeMethod()` only *queues* the
 *   job and returns immediately - it does not wait for the lambda to finish. `BENCH` logging AND
 *   the call advancing to the next command (via a nested
 *   `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)` back to the GUI thread) must
 *   therefore both happen *inside* that same lambda, at the point the work is genuinely done -
 *   never outside/after the `invokeMethod()` call itself.
 * - **Two-step dispatch** (`filter_ego`, `filter_isolates`, `symmetrize_strongties`,
 *   `symmetrize_cocitation`, `unilateral`, `distances`, `distances_bench`,
 *   `report-centrality-degree`, `report-centrality-closeness`, `report-centrality-closeness-ir`,
 *   `report-centrality-betweenness`, `report-centrality-stress`, `report-centrality-eccentricity`,
 *   `report-centrality-power`, `report-centrality-information`, `report-centrality-eigenvector`,
 *   `report-prestige-degree`, `report-prestige-proximity`, `report-prestige-pagerank`): used when
 *   the work can
 *   take many seconds and should show a progress dialog, via the `runGraphOperationAsync()`
 *   helper. It takes two separate lambdas - one that performs the (possibly slow) computation, one
 *   that runs only after that computation has fully completed, to report on it and advance the
 *   script. Because both lambdas need to see the same timer/result values, and a plain local
 *   variable would not survive between two separate lambdas, those values are wrapped in
 *   `std::shared_ptr` instead - each lambda holds its own copy of the pointer, all pointing at the
 *   same shared value.
 *
 * Every command logs one `"BENCH <command> ... elapsed_ms=<N>"` line via `qInfo()` (not
 * `qDebug()`/`qCDebug()`) on completion, so this keeps printing even when debug output is quiet by
 * default.
 */
void MainWindow::processNextInteractiveCommand()
{
    if (m_interactiveScriptIndex >= m_interactiveScriptLines.size())
    {
        qCDebug(lcMainWindow) << "Interactive script finished.";
        return;
    }
    const QString line = m_interactiveScriptLines.at(m_interactiveScriptIndex).trimmed();
    ++m_interactiveScriptIndex;

    if (line.isEmpty() || line.startsWith('#'))
    {
        processNextInteractiveCommand();
        return;
    }

    qCDebug(lcMainWindow) << "Interactive script command:" << line;

    if (line == "new")
    {
        QElapsedTimer timer;
        timer.start();
        slotNetworkNew();
        qInfo() << "BENCH new N=" << activeNodes() << "E=" << activeEdges()
                << "elapsed_ms=" << timer.elapsed();
        QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
    }
    else if (line.startsWith("delay "))
    {
        bool ok = false;
        const double seconds = line.mid(6).trimmed().toDouble(&ok);
        if (!ok || seconds < 0)
        {
            qWarning() << "Malformed 'delay' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        QTimer::singleShot(qRound(seconds * 1000), this, [this, timer]() {
            // elapsed_ms should read ~= the requested delay - a cheap sanity check that
            // scripted delays aren't drifting under load.
            qInfo() << "BENCH delay elapsed_ms=" << timer->elapsed();
            processNextInteractiveCommand();
        });
    }
    else if (line.startsWith("relation "))
    {
        // Dispatched the same way editRelationChangeCombo's activated(int) does - a queued
        // cross-thread call to Graph::relationSet(int), not a direct synchronous call.
        bool ok = false;
        const int relNum = line.mid(9).trimmed().toInt(&ok);
        if (!ok || relNum < 0)
        {
            qWarning() << "Malformed 'relation' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, relNum]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->relationSet(relNum);
            qInfo() << "BENCH relation N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread, not
            // immediately after invokeMethod() returns - otherwise the next script command can
            // race ahead while this one is still running (confirmed via a reproducible crash
            // for erdos/erdos-m, the same "single-step dispatch" pattern as this command).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line.startsWith("erdos "))
    {
        // erdos <N> <p> <directed|undirected> - creates a deterministic-shape (though not
        // deterministic-content, since it's still randomized) large Erdos-Renyi G(n,p) network
        // for reliable stress-testing, without depending on an external large dataset file
        // being present. Bypasses slotNetworkRandomErdosRenyi()'s modal "network created" info
        // dialog, which would otherwise block a scripted run with no one to click it.
        const QStringList parts = line.mid(6).trimmed().split(' ', Qt::SkipEmptyParts);
        bool nOk = false, pOk = false;
        const int n = parts.value(0).toInt(&nOk);
        const qreal p = parts.value(1).toDouble(&pOk);
        const QString directedness = parts.value(2, "directed");
        if (!nOk || !pOk || n <= 0 || p < 0.0 || p > 1.0
            || (directedness != "directed" && directedness != "undirected"))
        {
            qWarning() << "Malformed 'erdos' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        const qint64 expectedEdges = static_cast<qint64>(p * n * (n - 1));
        if (!confirmGenerationSize(expectedEdges, tr("Erdős–Rényi network"), true))
        {
            processNextInteractiveCommand();
            return;
        }
        const QString mode = (directedness == "undirected") ? "graph" : "digraph";
        QMetaObject::invokeMethod(activeGraph, [this, n, p, mode]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->randomNetErdosCreate(n, "G(n,p)", 0, p, mode, false);
            qInfo() << "BENCH erdos N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread, not
            // immediately after invokeMethod() returns - otherwise the next script command
            // (e.g. 'quit', or another 'erdos'/'erdos-m') can race ahead while this one is
            // still running. Confirmed via a reproducible out-of-bounds QList crash: a script
            // with no delay between 'erdos' and 'quit' tore down Graph state mid-loop.
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line.startsWith("erdos-m "))
    {
        // erdos-m <N> <M> <directed|undirected> - same as 'erdos' but G(n,M): exactly M edges
        // placed at random, instead of each edge existing independently with probability p.
        // Node and edge *count* are then exactly reproducible run to run (which edges land
        // where is still randomized) - unlike G(n,p), where edge count only concentrates
        // around its expected value. Used by the WS6.6 render-perf fixture, where a stable
        // N/E shape matters for comparing timing thresholds across runs.
        const QStringList parts = line.mid(8).trimmed().split(' ', Qt::SkipEmptyParts);
        bool nOk = false, mOk = false;
        const int n = parts.value(0).toInt(&nOk);
        const int m = parts.value(1).toInt(&mOk);
        const QString directedness = parts.value(2, "directed");
        if (!nOk || !mOk || n <= 0 || m < 0
            || (directedness != "directed" && directedness != "undirected"))
        {
            qWarning() << "Malformed 'erdos-m' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        if (!confirmGenerationSize(m, tr("Erdős–Rényi network"), true))
        {
            processNextInteractiveCommand();
            return;
        }
        const QString mode = (directedness == "undirected") ? "graph" : "digraph";
        QMetaObject::invokeMethod(activeGraph, [this, n, m, mode]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->randomNetErdosCreate(n, "G(n,M)", m, 0, mode, false);
            qInfo() << "BENCH erdos-m N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line == "unilateral")
    {
        // Direct Graph::edgeFilterUnilateral() call via runGraphOperationAsync, matching
        // slotEditFilterEdgesUnilateral()'s own dispatch - same convention as
        // 'filter_isolates'/'symmetrize_strongties' below. Previously triggered the real
        // QAction instead, then advanced immediately without waiting for the (already async,
        // since WS15 P3) slot to actually finish - the same race class confirmed on 'erdos'.
        const bool toggleTo = !editFilterEdgesUnilateralAct->isChecked();
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        runGraphOperationAsync(
            [this, toggleTo]() { activeGraph->edgeFilterUnilateral(toggleTo); },
            tr("Filtering unilateral edges (script)..."),
            [this, timer]() {
                qInfo() << "BENCH unilateral N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line.startsWith("save "))
    {
        // save <path> - always saves as GraphML, bypassing slotNetworkSave()'s fileName-based
        // format detection and "Save As" dialog fallback (which would block a scripted run).
        const QString path = line.mid(5).trimmed();
        if (path.isEmpty())
        {
            qWarning() << "Malformed 'save' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, path]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->saveToFile(path, FileType::GRAPHML, true, true);
            qInfo() << "BENCH save N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line == "add-node")
    {
        QMetaObject::invokeMethod(activeGraph, [this]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->vertexCreateAtPosRandom(false);
            qInfo() << "BENCH add-node N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line.startsWith("add-edge "))
    {
        const QStringList parts = line.mid(9).trimmed().split(' ', Qt::SkipEmptyParts);
        bool sOk = false, tOk = false;
        const int source = parts.value(0).toInt(&sOk);
        const int target = parts.value(1).toInt(&tOk);
        const qreal weight = parts.value(2, "1").toDouble();
        if (!sOk || !tOk)
        {
            qWarning() << "Malformed 'add-edge' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, source, target, weight]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->edgeCreate(source, target, weight, "black", EdgeType::Directed,
                                    true, false, QString(), true);
            qInfo() << "BENCH add-edge N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line.startsWith("add-relation "))
    {
        const QString name = line.mid(13).trimmed();
        if (name.isEmpty())
        {
            qWarning() << "Malformed 'add-relation' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, name]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->relationAdd(name, true);
            qInfo() << "BENCH add-relation N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line.startsWith("click-node "))
    {
        // click-node <id> - sets Graph::vertexClicked() without going through GraphicsWidget's
        // real mouse-press/selection-changed chain (which also gates filterNodesByEgoNetworkAct's
        // enabled state - irrelevant here since the commands below call Graph:: methods directly,
        // not via that QAction). Prerequisite for 'filter_ego'.
        bool ok = false;
        const int id = line.mid(11).trimmed().toInt(&ok);
        if (!ok)
        {
            qWarning() << "Malformed 'click-node' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, id]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->vertexClickedSet(id, QPointF());
            qInfo() << "BENCH click-node id=" << id << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise). Also
            // makes 'filter_ego' below's own FIFO-ordering workaround belt-and-braces rather
            // than load-bearing, since vertexClickedSet() is now guaranteed complete before the
            // next command starts.
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line == "filter_ego")
    {
        // WS15 P3 Group C test aid: mirrors slotFilterNodesByEgoNetwork()'s real
        // vertexFilterByEgoNetwork() call and runGraphOperationAsync dispatch, skipping only the
        // GUI-only filter-chip/filter-bar bookkeeping (same philosophy as distances_bench skipping
        // the disk write) - added specifically to reproduce and verify the fix for the reported
        // multi-minute freeze on a large network (2000+ nodes) with only the OS beachball as
        // feedback. Needs 'click-node <id>' first.
        //
        // vertexClicked() is read *inside* the graphThread-dispatched lambda below, not here on
        // the GUI thread before dispatch (unlike the real slotFilterNodesByEgoNetwork(), which
        // reads it on the GUI thread - safe there since a real user's node-click and menu-click
        // are two separate actions with ample time between them for click-node's own queued
        // vertexClickedSet() call to complete). Back-to-back scripted commands have no such gap:
        // reading it here raced with click-node's still-pending queued call and read 0. Reading
        // it inside this lambda instead makes it FIFO-ordered with click-node's own queued call,
        // since both run on graphThread's single event queue.
        auto v1 = std::make_shared<int>(0);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        runGraphOperationAsync(
            [this, v1]() {
                *v1 = activeGraph->vertexClicked();
                activeGraph->vertexFilterByEgoNetwork(*v1);
            },
            tr("Filtering ego network (script)..."),
            [this, v1, timer]() {
                qInfo() << "BENCH filter_ego v1=" << *v1
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line.startsWith("filter_isolates "))
    {
        // filter_isolates <on|off> - direct Graph::vertexIsolatedAllToggle() call via
        // runGraphOperationAsync, same dispatch as the real editFilterNodesIsolatesAct-driven
        // slotEditFilterNodesIsolates(), skipping only the QAction/status-message side effects.
        const QString arg = line.mid(16).trimmed();
        if (arg != "on" && arg != "off")
        {
            qWarning() << "Malformed 'filter_isolates' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        const bool disableIsolates = (arg == "on");
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        runGraphOperationAsync(
            [this, disableIsolates]() { activeGraph->vertexIsolatedAllToggle(disableIsolates); },
            tr("Filtering isolate nodes (script)..."),
            [this, disableIsolates, timer]() {
                qInfo() << "BENCH filter_isolates disable=" << disableIsolates
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line.startsWith("symmetrize_strongties "))
    {
        // symmetrize_strongties <all|current> - direct Graph::addRelationSymmetricStrongTies()
        // call via runGraphOperationAsync. Only safe to script on a single-relation network - the
        // real slotEditEdgeSymmetrizeStrongTies() shows a modal chooser dialog when multiple
        // relations exist, which would block an unattended script (same reason 'erdos'/'save'
        // bypass their own real slots' modal dialogs).
        const QString arg = line.mid(22).trimmed();
        if (arg != "all" && arg != "current")
        {
            qWarning() << "Malformed 'symmetrize_strongties' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        const bool allRelations = (arg == "all");
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        runGraphOperationAsync(
            [this, allRelations]() { activeGraph->addRelationSymmetricStrongTies(allRelations); },
            tr("Symmetrizing strong ties (script)..."),
            [this, allRelations, timer]() {
                qInfo() << "BENCH symmetrize_strongties all=" << allRelations
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "symmetrize_cocitation")
    {
        // Direct Graph::relationAddCocitation() call via runGraphOperationAsync - no modal
        // dialog in the real slot for this one, so no bypass needed.
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();
        runGraphOperationAsync(
            [this]() { activeGraph->relationAddCocitation(); },
            tr("Computing cocitation relation (script)..."),
            [this, timer]() {
                qInfo() << "BENCH symmetrize_cocitation N=" << activeNodes()
                        << "E=" << activeEdges() << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "distances" || line.startsWith("distances "))
    {
        // distances [weights] [inverse] [dropisolates] [csv] - mirrors the real Cohesion >
        // Distances Matrix menu action (slotAnalyzeMatrixDistances()) exactly: same computation
        // (writeMatrix() -> graphMatrixDistanceGeodesicCreate()), same runGraphOperationAsync()
        // dispatch, same output file - just without opening a TextEditor afterward (no one
        // present to view it in a script). Also skips askAboutEdgeWeights()'s modal prompt
        // entirely - the tokens below already answer what it would ask. WS16 (#113): the 'csv'
        // token selects ReportFormat::Csv explicitly, rather than reading the persisted Settings
        // preference - a script has no Settings dialog to reflect, and an explicit token matches
        // the shape of every other boolean token here.
        const QStringList tokens = line.mid(9).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesic-distances-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeMatrix(fn, MATRIX_DISTANCES,
                                                    considerWeights, inverseWeights, dropIsolates,
                                                    "Rows", false, reportFormat);
            },
            tr("Computing geodesic distances. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH distances weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "distances_bench" || line.startsWith("distances_bench "))
    {
        // distances_bench [weights] [inverse] [dropisolates] [centralities] - benchmarking-only
        // variant of 'distances' above: same dispatch mechanism (runGraphOperationAsync) and
        // same underlying computation, but skips the disk write entirely, for isolating pure
        // computation cost. 'centralities' has no real-menu equivalent (the GUI computes each
        // centrality index via ~9 separate menu actions, not one combined action), so it lives
        // here rather than on 'distances'.
        const QStringList tokens = line.mid(15).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const bool computeCentralities = tokens.contains("centralities");

        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, computeCentralities, considerWeights, inverseWeights, dropIsolates]() {
                activeGraph->graphDistancesGeodesic(computeCentralities, considerWeights,
                                                    inverseWeights, dropIsolates);
            },
            tr("Computing geodesic distances (benchmark, no disk write). Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, computeCentralities, timer]() {
                qInfo() << "BENCH distances_bench weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "centralities=" << computeCentralities
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-degree" || line.startsWith("report-centrality-degree "))
    {
        // report-centrality-degree [weights] [dropisolates] [csv] - WS16 (#113) baseline: mirrors
        // the real Analyze > Centrality > Degree menu action (slotAnalyzeCentralityDegree())
        // exactly, same as 'distances' mirrors slotAnalyzeMatrixDistances() above. This is the
        // first centrality/prestige report ever exercised headlessly - none of the other 11
        // writeCentrality*/writePrestige* functions have a script command yet. Also skips
        // askAboutEdgeWeights()'s modal prompt entirely - the tokens below already answer what it
        // would ask. The 'csv' token selects ReportFormat::Csv explicitly (Step 2), matching
        // 'distances'' handling.
        const QStringList tokens = line.mid(24).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityDegree(fn, considerWeights, dropIsolates, reportFormat);
            },
            tr("Computing Degree Centralities. Please wait..."),
            [this, considerWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-degree weights=" << considerWeights
                        << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-closeness" || line.startsWith("report-centrality-closeness "))
    {
        // report-centrality-closeness [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityCloseness() exactly, same shape as report-centrality-degree.
        const QStringList tokens = line.mid(27).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityCloseness(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Closeness Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-closeness weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-closeness-ir" || line.startsWith("report-centrality-closeness-ir "))
    {
        // report-centrality-closeness-ir [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityClosenessIR() exactly.
        const QStringList tokens = line.mid(30).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-influence-range-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityClosenessInfluenceRange(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Influence Range Closeness Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-closeness-ir weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-betweenness" || line.startsWith("report-centrality-betweenness "))
    {
        // report-centrality-betweenness [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityBetweenness() exactly.
        const QStringList tokens = line.mid(29).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-betweenness-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityBetweenness(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Betweenness Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-betweenness weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-stress" || line.startsWith("report-centrality-stress "))
    {
        // report-centrality-stress [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityStress() exactly.
        const QStringList tokens = line.mid(24).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-stress-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityStress(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Stress Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-stress weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-eccentricity" || line.startsWith("report-centrality-eccentricity "))
    {
        // report-centrality-eccentricity [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityEccentricity() exactly.
        const QStringList tokens = line.mid(30).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eccentricity-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityEccentricity(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Eccentricity Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-eccentricity weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-power" || line.startsWith("report-centrality-power "))
    {
        // report-centrality-power [weights] [inverse] [dropisolates] [csv] - WS16 Step 2:
        // mirrors slotAnalyzeCentralityPower() exactly.
        const QStringList tokens = line.mid(23).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-power-Gil-Schmidt-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityPower(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Power Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-centrality-power weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-information" || line.startsWith("report-centrality-information "))
    {
        // report-centrality-information [weights] [inverse] [csv] - WS16 Step 2: mirrors
        // slotAnalyzeCentralityInformation() exactly (no dropIsolates - Information Centrality
        // doesn't take one; it always excludes isolates internally).
        const QStringList tokens = line.mid(29).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-information-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, reportFormat, success]() {
                *success = activeGraph->writeCentralityInformation(
                    fn, considerWeights, inverseWeights, reportFormat);
            },
            tr("Computing Information Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, success, timer]() {
                qInfo() << "BENCH report-centrality-information weights=" << considerWeights
                        << "inverse=" << inverseWeights
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-centrality-eigenvector" || line.startsWith("report-centrality-eigenvector "))
    {
        // report-centrality-eigenvector [weights] [inverse] [csv] - WS16 Step 2: mirrors
        // slotAnalyzeCentralityEigenvector() exactly (dropIsolates is fixed false there, so no
        // token for it here either).
        const QStringList tokens = line.mid(29).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = false;
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eigenvector-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeCentralityEigenvector(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Eigenvector Centralities. Please wait..."),
            [this, considerWeights, inverseWeights, success, timer]() {
                qInfo() << "BENCH report-centrality-eigenvector weights=" << considerWeights
                        << "inverse=" << inverseWeights
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-prestige-degree" || line.startsWith("report-prestige-degree "))
    {
        // report-prestige-degree [weights] [dropisolates] [csv] - WS16 Step 2: mirrors
        // slotAnalyzePrestigeDegree() exactly.
        const QStringList tokens = line.mid(22).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-prestige-degree-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writePrestigeDegree(fn, considerWeights, dropIsolates, reportFormat);
            },
            tr("Computing Degree Prestige. Please wait..."),
            [this, considerWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-prestige-degree weights=" << considerWeights
                        << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-prestige-proximity" || line.startsWith("report-prestige-proximity "))
    {
        // report-prestige-proximity [dropisolates] [csv] - WS16 Step 2: mirrors
        // slotAnalyzePrestigeProximity() exactly (considerWeights/inverseWeights are fixed
        // true/false there, so no tokens for them here either).
        const QStringList tokens = line.mid(25).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-prestige-proximity-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writePrestigeProximity(fn, true, false, dropIsolates, reportFormat);
            },
            tr("Computing Proximity Prestige. Please wait..."),
            [this, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-prestige-proximity dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-prestige-pagerank" || line.startsWith("report-prestige-pagerank "))
    {
        // report-prestige-pagerank [dropisolates] [csv] - WS16 Step 2: mirrors
        // slotAnalyzePrestigePageRank() exactly.
        const QStringList tokens = line.mid(24).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-prestige-pagerank-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writePrestigePageRank(fn, dropIsolates, reportFormat);
            },
            tr("Computing PageRank Prestige. Please wait..."),
            [this, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-prestige-pagerank dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-reciprocity" || line.startsWith("report-reciprocity "))
    {
        // report-reciprocity [weights] [csv] - WS16 Step 3: mirrors slotAnalyzeReciprocity()
        // exactly.
        const QStringList tokens = line.mid(18).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-reciprocity-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, reportFormat, success]() {
                *success = activeGraph->writeReciprocity(fn, considerWeights, reportFormat);
            },
            tr("Computing Reciprocity. Please wait..."),
            [this, considerWeights, success, timer]() {
                qInfo() << "BENCH report-reciprocity weights=" << considerWeights
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-eccentricity" || line.startsWith("report-eccentricity "))
    {
        // report-eccentricity [weights] [inverse] [dropisolates] [csv] - WS16 Step 3: mirrors
        // slotAnalyzeEccentricity() exactly.
        const QStringList tokens = line.mid(19).trimmed().split(' ', Qt::SkipEmptyParts);
        const bool considerWeights = tokens.contains("weights");
        const bool inverseWeights = tokens.contains("inverse");
        const bool dropIsolates = tokens.contains("dropisolates");
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, considerWeights, inverseWeights, dropIsolates, reportFormat, success]() {
                *success = activeGraph->writeEccentricity(
                    fn, considerWeights, inverseWeights, dropIsolates, reportFormat);
            },
            tr("Computing Eccentricity. Please wait..."),
            [this, considerWeights, inverseWeights, dropIsolates, success, timer]() {
                qInfo() << "BENCH report-eccentricity weights=" << considerWeights
                        << "inverse=" << inverseWeights << "dropisolates=" << dropIsolates
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-clustering-coefficient" || line.startsWith("report-clustering-coefficient "))
    {
        // report-clustering-coefficient [csv] - WS16 Step 3: mirrors
        // slotAnalyzeClusteringCoefficient() exactly (considerWeights is fixed true there, so
        // no token for it here either).
        const QStringList tokens = line.mid(29).trimmed().split(' ', Qt::SkipEmptyParts);
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficient-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, reportFormat, success]() {
                *success = activeGraph->writeClusteringCoefficient(fn, true, reportFormat);
            },
            tr("Computing Clustering Coefficients. Please wait..."),
            [this, success, timer]() {
                qInfo() << "BENCH report-clustering-coefficient"
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "report-triad-census" || line.startsWith("report-triad-census "))
    {
        // report-triad-census [csv] - WS16 Step 3: mirrors slotAnalyzeCommunitiesTriadCensus()
        // exactly (considerWeights is fixed true there, so no token for it here either).
        const QStringList tokens = line.mid(19).trimmed().split(' ', Qt::SkipEmptyParts);
        const int reportFormat = tokens.contains("csv") ? ReportFormat::Csv : ReportFormat::Html;

        const QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
        const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
        const QString fn = appSettings["dataDir"] + "socnetv-report-triad-census-" + dateTime + ext;
        auto success = std::make_shared<bool>(false);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        runGraphOperationAsync(
            [this, fn, reportFormat, success]() {
                *success = activeGraph->writeTriadCensus(fn, true, reportFormat);
            },
            tr("Computing Triad Census. Please wait..."),
            [this, success, timer]() {
                qInfo() << "BENCH report-triad-census"
                        << "success=" << *success
                        << "N=" << activeNodes() << "E=" << activeEdges()
                        << "elapsed_ms=" << timer->elapsed();
                QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
            });
    }
    else if (line == "render")
    {
        // WS10/WS6.6 render-perf benchmarking aid: forces a synchronous repaint (unlike
        // update(), which only schedules one), so this measures the actual paint cost of
        // the current canvas state rather than whether a repaint got scheduled at all.
        QElapsedTimer timer;
        timer.start();
        graphicsWidget->viewport()->repaint();
        qInfo() << "BENCH render N=" << activeNodes() << "E=" << activeEdges()
                << "elapsed_ms=" << timer.elapsed();
        QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
    }
    else if (line.startsWith("bulk-node-size "))
    {
        // bulk-node-size <N> - calls slotEditNodeSizeAll() directly with a nonzero size so
        // its modal QInputDialog (used when newSize==0) never opens, which would otherwise
        // block a scripted run with no one to click it.
        bool ok = false;
        const int newSize = line.mid(15).trimmed().toInt(&ok);
        if (!ok || newSize <= 0)
        {
            qWarning() << "Malformed 'bulk-node-size' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QElapsedTimer timer;
        timer.start();
        slotEditNodeSizeAll(newSize);
        qInfo() << "BENCH bulk-node-size N=" << activeNodes() << "E=" << activeEdges()
                << "elapsed_ms=" << timer.elapsed();
        QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
    }
    else if (line.startsWith("bulk-edge-color "))
    {
        // bulk-edge-color <name> - calls slotEditEdgeColorAll() directly with a valid QColor
        // so its modal QColorDialog (used when color is invalid) never opens.
        const QString name = line.mid(16).trimmed();
        const QColor color(name);
        if (!color.isValid())
        {
            qWarning() << "Malformed 'bulk-edge-color' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QElapsedTimer timer;
        timer.start();
        slotEditEdgeColorAll(color);
        qInfo() << "BENCH bulk-edge-color N=" << activeNodes() << "E=" << activeEdges()
                << "elapsed_ms=" << timer.elapsed();
        QTimer::singleShot(0, this, &MainWindow::processNextInteractiveCommand);
    }
    else if (line.startsWith("move "))
    {
        // move <node> <x> <y> - simulates a single-node drag (and its cascading edge-geometry
        // updates) by setting an absolute canvas position, same backlog item named in
        // roadmap_ws12_cli_scripting_mode.md.
        const QStringList parts = line.mid(5).trimmed().split(' ', Qt::SkipEmptyParts);
        bool nodeOk = false, xOk = false, yOk = false;
        const int node = parts.value(0).toInt(&nodeOk);
        const int x = parts.value(1).toInt(&xOk);
        const int y = parts.value(2).toInt(&yOk);
        if (!nodeOk || !xOk || !yOk)
        {
            qWarning() << "Malformed 'move' command, skipping:" << line;
            processNextInteractiveCommand();
            return;
        }
        QMetaObject::invokeMethod(activeGraph, [this, node, x, y]() {
            QElapsedTimer timer;
            timer.start();
            activeGraph->vertexPosSet(node, x, y);
            qInfo() << "BENCH move N=" << activeNodes() << "E=" << activeEdges()
                    << "elapsed_ms=" << timer.elapsed();
            // Advance only after this queued lambda actually finishes on graphThread - see the
            // matching comment on 'erdos' above for why (a reproducible crash otherwise).
            QMetaObject::invokeMethod(this, &MainWindow::processNextInteractiveCommand, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
    else if (line == "quit")
    {
        // Ends the app cleanly so a scripted run doesn't need to be killed externally. Goes
        // through the real close() / closeEvent() path (not a raw QCoreApplication::quit(),
        // which still routes through closeEvent() during Qt's shutdown) with
        // m_interactiveScriptQuitting set, so the "save changes?" modal - which would otherwise
        // block forever with no one present to answer it - is skipped.
        QElapsedTimer timer;
        timer.start();
        qCDebug(lcMainWindow) << "Interactive script requested quit.";
        // Log BENCH *before* close(), not after: close() can trigger teardown of activeGraph
        // as part of application shutdown, so reading activeNodes()/activeEdges() afterward
        // would touch already-torn-down state.
        qInfo() << "BENCH quit N=" << activeNodes() << "E=" << activeEdges()
                << "elapsed_ms=" << timer.elapsed();
        m_interactiveScriptQuitting = true;
        close();
    }
    else
    {
        qWarning() << "Unknown interactive script command, skipping:" << line;
        processNextInteractiveCommand();
    }
}



/**
 * @brief  Shows a message in the status bar, with the given duration
 *
 * Called by Graph::statusMessage to display some message to the user
 *
 * @param message
 */
void MainWindow::statusMessage(const QString message)
{
    statusBar()->showMessage(message, appSettings["initStatusBarDuration"].toInt(0));
    statusBar()->repaint();
}

/**
 * @brief Helper function to display a popup with useful info
 * @param text
 */
void MainWindow::slotHelpMessageToUserInfo(const QString text)
{
    slotHelpMessageToUser(USER_MSG_INFO, tr("Useful information"), text);
}

/**
 * @brief Helper function to display a popup with an error message
 * @param text
 */
void MainWindow::slotHelpMessageToUserError(const QString text)
{
    slotHelpMessageToUser(USER_MSG_CRITICAL, tr("Error"), text);
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
                                      const QString btn2,
                                      const QString btn3)
{
    int response = 0;
    QMessageBox msgBox;
    msgBox.setMinimumWidth(400);
    QPushButton *pbtn1, *pbtn2;

    switch (type)
    {
    case USER_MSG_INFO:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Information");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        msgBox.setIcon(QMessageBox::Information);
        if (buttons == QMessageBox::NoButton)
        {
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
        }
        else
        {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }
        msgBox.setDefaultButton(defBtn);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Error");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        // msgBox.setTextFormat(Qt::RichText);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_NETWORK:
        statusMessage(tr("Nothing to do! Load or create a social network first"));
        msgBox.setWindowTitle("Error");
        msgBox.setText(
            tr("No network! \n"
               "Load social network data or create a new social network first. \n"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_EDGES:
        statusMessage(tr("Nothing to do! Load social network data or create edges first"));
        msgBox.setWindowTitle("Error");
        msgBox.setText(
            tr("No edges! \n"
               "Load social network data or create some edges first. \n"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Question");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        if (buttons == QMessageBox::NoButton)
        {
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
        }
        else
        {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }

        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION_CUSTOM: // a custom question with just two/three buttons
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Question");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        pbtn1 = msgBox.addButton(btn1, QMessageBox::ActionRole);
        pbtn2 = msgBox.addButton(btn2, QMessageBox::ActionRole);
        if (!btn3.isNull() && !btn3.isEmpty())
        {
            QPushButton *pbtn3 = msgBox.addButton(btn3, QMessageBox::ActionRole);
            msgBox.setIcon(QMessageBox::Question);
            response = msgBox.exec();
            if (msgBox.clickedButton() == pbtn1)
                response = 1;
            else if (msgBox.clickedButton() == pbtn2)
                response = 2;
            else if (msgBox.clickedButton() == pbtn3)
                response = 3;
        }
        else
        {
            msgBox.setIcon(QMessageBox::Question);
            response = msgBox.exec();
            if (msgBox.clickedButton() == pbtn1)
                response = 1;
            else if (msgBox.clickedButton() == pbtn2)
                response = 2;
        }
        break;
    default: // just for sanity
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setText(text);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();
        break;
    }
    return response;
}


/**
 * @brief Called when user selects something in the Subgraph from Selected
 * Nodes selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditNodeSubgraphSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected subgraph creation, text index: " << selectedIndex;
    switch (selectedIndex)
    {
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

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when user selects something in the Edge Transform
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditEdgeTransformSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected edge transform, index: " << selectedIndex;
    switch (selectedIndex)
    {
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
 * @brief Called when user selects a filter action in the Filter
 * selectbox of the toolbox Network group
 * @param selectedIndex
 */
void MainWindow::toolBoxFilterSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected filter action, index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotFilterNodesByEgoNetwork();
        break;
    case 2:
        slotFilterNodesBySelection();
        break;
    case 3:
        slotFilterNodesDialogByCentrality();
        break;
    case 4:
        slotFilterNodesByAttribute();
        break;
    case 5:
        slotEditFilterEdgesByWeightDialog();
        break;
    case 6:
        slotFilterNodesRestoreAll();
        break;
    case 7:
        slotEditFilterEdgesReset();
        break;
    };
    // Reset to placeholder after dispatching
    toolBoxFilterSelect->blockSignals(true);
    toolBoxFilterSelect->setCurrentIndex(0);
    toolBoxFilterSelect->blockSignals(false);
}

/**
 * @brief Called when user selects something in the Matrices
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisMatricesSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected matrix analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
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

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when user selects something in the Cohesion
 * selectbox of the toolbox to compute basic graph theoretic / network properties
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisCohesionSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected cohesion analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
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
        slotAnalyzeGeodesicDistribution();
        break;
    case 6:
        slotAnalyzeMatrixDistances();
        break;
    case 7:
        slotAnalyzeMatrixGeodesics();
        break;
    case 8:
        slotAnalyzeEccentricity();
        break;
    case 9:
        slotAnalyzeDiameter();
        break;
    case 10:
        slotAnalyzeConnectedness();
        break;
    case 11:
        slotAnalyzeWalksLength();
        break;
    case 12:
        slotAnalyzeWalksTotal();
        break;
    case 13:
        slotAnalyzeReachabilityMatrix();
        break;
    case 14:
        slotAnalyzeClusteringCoefficient();
        break;
    case 15:
        slotAnalyzeNodeConnectivity();
        break;
    case 16:
        slotAnalyzeConnectivity();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when the user selects something in the Communities selectbox
 * of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisCommunitiesSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected community analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotAnalyzeCommunitiesCliqueCensus();
        break;
    case 2:
        slotAnalyzeCommunitiesTriadCensus();
        break;
    };
    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when the user selects something in the Structural Equivalence
 * selectbox of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisStrEquivalenceSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected struct. equivalence analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
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

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when user selects something in the Prominence selectbox
 *  of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisProminenceSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected prominence analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
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

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when the user selects a Prominence index in the Layout selectbox
 *  of the Control Panel.
 */
void MainWindow::toolBoxLayoutByIndexApplyBtnPressed()
{
    qCDebug(lcMainWindow) << "User request to apply prominence-based layout...";
    int selectedIndex = toolBoxLayoutByIndexSelect->currentIndex();
    QString selectedIndexText = toolBoxLayoutByIndexSelect->currentText();
    int selectedLayoutType = toolBoxLayoutByIndexTypeSelect->currentIndex();
    qCDebug(lcMainWindow) << "elected index is "
             << selectedIndexText << " : " << selectedIndex
             << " selected layout type is " << selectedLayoutType;
    switch (selectedIndex)
    {
    case 0: // None
        break;
    case 1: // Random
        if (selectedLayoutType == 1)
            slotLayoutRadialRandom();
        else if (selectedLayoutType == 2)
            slotLayoutRandom();
        break;
    default:
        if (selectedLayoutType == 0) // None type — do nothing
            break;
        else if (selectedLayoutType == 1) // Radial
            slotLayoutRadialByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 2) // On Levels
            slotLayoutLevelByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 3) // Node Size
            slotLayoutNodeSizeByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 4) // Node Color
            slotLayoutNodeColorByProminenceIndex(selectedIndexText);
        break;
    };
}

/**
 * @brief Called when the user selects a model in the Layout by Force Directed
 * selectbox of left panel.
 */
void MainWindow::toolBoxLayoutForceDirectedApplyBtnPressed()
{
    qCDebug(lcMainWindow) << "User selected to apply a FDP layout...";
    int selectedModel = toolBoxLayoutForceDirectedSelect->currentIndex();
    QString selectedModelText = toolBoxLayoutForceDirectedSelect->currentText();
    qCDebug(lcMainWindow) << " selected index is " << selectedModelText << " : "
             << selectedModel;

    switch (selectedModel)
    {
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
        toolBoxLayoutForceDirectedSelect->blockSignals(true);
        toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
        toolBoxLayoutForceDirectedSelect->blockSignals(false);
        break;
    };

    // FD model controls node positions — reset prominence Type if it was
    // Radial (1) or On Levels (2), but leave Node Size/Color (3/4) intact.
    if (selectedModel > 0)
    {
        int currentType = toolBoxLayoutByIndexTypeSelect->currentIndex();
        if (currentType == 1 || currentType == 2)
        {
            toolBoxLayoutByIndexTypeSelect->blockSignals(true);
            toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);
            toolBoxLayoutByIndexTypeSelect->blockSignals(false);
        }
    }
}


























/**
 * @brief Toggles the interactive/selection mouse drag mode
 * @param checked
 */
void MainWindow::slotEditDragModeSelection(bool checked)
{
    qCDebug(lcMainWindow) << "User changed drag mode, checked" << checked;

    editMouseModeScrollAct->setChecked(false);

    if (editMouseModeInteractiveAct->isChecked())
    {

        graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);
        graphicsWidget->setInteractive(true);
    }
    else
    {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
        graphicsWidget->setInteractive(false);
    }
}

/**
 * @brief Toggles the non-interactive scrollhand drag mode.
 * @param checked
 */
void MainWindow::slotEditDragModeScroll(bool checked)
{

    qCDebug(lcMainWindow) << "User changed scroll mode, checked" << checked;

    editMouseModeInteractiveAct->setChecked(false);
    graphicsWidget->setInteractive(false);

    if (editMouseModeScrollAct->isChecked())
    {

        graphicsWidget->setDragMode(QGraphicsView::ScrollHandDrag);
    }
    else
    {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
    }
}

/**
 * @brief Clears the relations combo.
 */
void MainWindow::slotEditRelationsClear()
{
    qCDebug(lcMainWindow) << "Clearing relations combo...";
    editRelationChangeCombo->clear();
}

/**
 * @brief Prompts the user to enter the name of a new relation
 *
 * On success, emits signal to Graph to change to the new relation.
 */
void MainWindow::slotEditRelationAddPrompt()
{

    bool ok;
    QString newRelationName;
    int relationsCounter = activeGraph->relations();

    qCDebug(lcMainWindow) << "Prompting the user for the new relation name to be added to the relations combo...";

    //
    // Prompt the user for the new relation name
    //

    // Check if this is the first time, in order to show a more comprehensive message
    if (relationsCounter == 1 && activeNodes() == 0)
    {
        newRelationName = QInputDialog::getText(
            this,
            tr("Add new relation"),
            tr("Enter a name for a new relation between the actors.\n"
               "A relation is a collection of ties of a "
               "specific kind between the network actors.\n"
               "For instance, enter \"friendship\" if the "
               "edges of this relation refer to the set of \n"
               "friendships between pairs of actors."),
            QLineEdit::Normal, QString(), &ok);
    }
    else
    {
        newRelationName = QInputDialog::getText(
            this, tr("Add new relation"),
            tr("Enter a name for the new relation (or press Cancel):"),
            QLineEdit::Normal, QString(), &ok);
    }

    //
    // Check which button was pressed
    //
    if (ok)
    {

        // user pressed OK

        // Check if new relation name
        if (!newRelationName.isEmpty())
        {

            // a relation name entered

            // Check if it is already used by another relation.
            if (editRelationChangeCombo->findText(newRelationName) > -1)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL,
                                      tr("Error. Relation name is used!"),
                                      tr("The relation name is already used."),
                                      tr("Please use another relation name that is not already used."));
                return;
            }

            // Emit signal to Graph to add the relation and change to it
            bool changeRelation = true;
            emit signalRelationAddAndChange(newRelationName, changeRelation);
        }
        else
        {
            // no name entered
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. No relation name entered!"),
                                  tr("You did not type a name for this new relation"));
            return;
        }
    }
    else
    {
        // user pressed Cancel
        statusMessage(QString(tr("New relation cancelled.")));
        return;
    }

    statusMessage(QString(tr("New relation named %1, added."))
                      .arg(newRelationName));
}

/**
 * @brief Adds a new relation to the relations combo
 *
 * Called from Graph when the network file parser or another Graph method
 * demands a new relation to be added to the UI combo.
 *
 * @param newRelationName
 */
void MainWindow::slotEditRelationAdd(const QString &newRelationName)
{

    qCDebug(lcMainWindow) << "Adding new relation to relations combo:"
             << newRelationName;

    if (!newRelationName.isNull())
    {

        editRelationChangeCombo->addItem(newRelationName);

        // Enable prev/next widgets, if they are disabled.
        if (!editRelationPreviousAct->isEnabled() && editRelationChangeCombo->count() > 1)
        {
            editRelationPreviousAct->setEnabled(true);
            editRelationNextAct->setEnabled(true);
        }

        statusMessage(QString(tr("Added a new relation named: %1."))
                          .arg(newRelationName));
    }
}

/**
 * @brief Changes the editRelations combo box index to relIndex
 *
 * If relIndex==RAND_MAX the index is set to the last relation index
 *
 * @param relIndex
 */
void MainWindow::slotEditRelationChange(const int &relIndex)
{
    if (relIndex == RAND_MAX)
    {
        qCDebug(lcMainWindow) << "relIndex==RANDMAX. Changing relation combo to last relation...";
        editRelationChangeCombo->setCurrentIndex(
            (editRelationChangeCombo->count() - 1));
    }
    else
    {
        qCDebug(lcMainWindow) << "Changing relation combo to index" << relIndex;
        editRelationChangeCombo->setCurrentIndex(relIndex);
    }
}

/**
 * @brief Prompts the user to enter a new name for the current relation
 */
void MainWindow::slotEditRelationRename()
{

    bool ok = false;

    qCDebug(lcMainWindow) << "Request to rename current relation:"
             << editRelationChangeCombo->currentText()
             << "Prompting for new name...";

    //
    // Get new name from user
    //
    QString newName = QInputDialog::getText(
        this,
        tr("Rename current relation"),
        tr("Enter a new name for this relation."),
        QLineEdit::Normal, QString(), &ok);

    //
    // Check entered name
    //
    if (newName.isEmpty() || !ok)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Not a valid name."),
                              tr("Error"),
                              tr("You did not enter a valid name for this relation."));
        return;
    }

    //
    // Change name in combo - this will trigger the signal to activeGraph
    //
    editRelationChangeCombo->setCurrentText(newName);
}







































/**
 * @brief Shows a context menu when the user right-clicks on an empty area of the canvas
 *        (i.e. not directly on a node or edge).
 *
 * The menu is selection-aware:
 * - When exactly one node is selected and no edges: offers Node Properties.
 * - When multiple nodes are selected: offers Remove + structural transforms (clique, star, …).
 * - When any nodes or edges are selected (≥ 1 total): offers "Edit Selection in Data Table",
 *   "Set property on selected nodes…", and/or "Set property on selected edges…".
 * - Always offers Add Node, Add Edge, and the Options sub-menu.
 *
 * @param mPos  The canvas position where the right-click occurred (unused — menu is shown
 *              at cursor position via QCursor::pos()).
 */
void MainWindow::slotEditOpenContextMenu(const QPointF &mPos)
{
    Q_UNUSED(mPos);
    QMenu *contextMenu = new QMenu(" Menu", this);
    Q_CHECK_PTR(contextMenu); // displays "out of memory" if needed

    int nodesSelected = activeGraph->getSelectedVerticesCount();
    int edgesSelected = activeGraph->getSelectedEdgesCount();
    int totalSelected = nodesSelected + edgesSelected;

    contextMenu->addAction("## Selected nodes: " + QString::number(nodesSelected) + "  edges: " + QString::number(edgesSelected) + " ##  ");

    contextMenu->addSeparator();

    if (nodesSelected > 0)
    {
        if (nodesSelected == 1 && edgesSelected == 0)
        {
            // Single node: offer per-node properties dialog
            contextMenu->addAction(editNodePropertiesAct);
        }
        contextMenu->addSeparator();
        contextMenu->addAction(editNodeRemoveAct);
        if (nodesSelected > 1)
        {
            editNodeRemoveAct->setText(tr("Remove ") + QString::number(nodesSelected) + tr(" nodes"));
            contextMenu->addSeparator();
            contextMenu->addAction(editNodeSelectedToCliqueAct);
            contextMenu->addAction(editNodeSelectedToStarAct);
            contextMenu->addAction(editNodeSelectedToCycleAct);
            contextMenu->addAction(editNodeSelectedToLineAct);
        }
        else
        {
            editNodeRemoveAct->setText(tr("Remove ") + QString::number(nodesSelected) + tr(" node"));
        }
        contextMenu->addSeparator();
    }

    // Table / bulk actions — shown whenever at least one item is selected
    if (totalSelected >= 1)
    {
        contextMenu->addAction(editNodeEditSelectionInTableAct);
        if (nodesSelected > 0)
            contextMenu->addAction(editNodeSetPropertyForSelectionAct);
        if (edgesSelected > 0)
            contextMenu->addAction(editEdgeSetPropertyForSelectionAct);
        contextMenu->addSeparator();
    }

    contextMenu->addAction(editNodeAddAct);
    contextMenu->addSeparator();
    contextMenu->addAction(editEdgeAddAct);
    contextMenu->addSeparator();

    QMenu *options = new QMenu("Options", this);
    contextMenu->addMenu(options);

    options->addAction(openSettingsAct);
    options->addSeparator();
    options->addAction(editNodeSizeAllAct);
    options->addAction(editNodeShapeAll);
    options->addAction(editNodeColorAll);
    options->addAction(optionsNodeNumbersVisibilityAct);
    options->addAction(optionsNodeLabelsVisibilityAct);
    options->addSeparator();
    options->addAction(editEdgeColorAllAct);
    options->addSeparator();
    options->addAction(changeBackColorAct);
    options->addAction(backgroundImageAct);

    // QCursor::pos() is good only for menus not related with node coordinates
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

/**
 * @brief Selects all nodes
 */
void MainWindow::slotEditNodeSelectAll()
{
    qCDebug(lcMainWindow) << "Request to select all nodes...";
    graphicsWidget->selectAll();
    statusMessage(tr("Selected nodes: %1")
                      .arg(activeGraph->getSelectedVerticesCount()));
}

/**
 * @brief Selects no nodes.
 */
void MainWindow::slotEditNodeSelectNone()
{
    qCDebug(lcMainWindow) << "Clearing node selection...";
    graphicsWidget->selectNone();
    statusMessage(QString(tr("Selection cleared")));
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
                                      const int &x, const int &y)
{
    qCDebug(lcMainWindow, "Updating position for node %i - x: %i, y: %i", nodeNumber, x, y);
    activeGraph->vertexPosSet(nodeNumber, x, y);
}

/**
 * @brief Adds a new random node
 *
 * Called when the "Add Node" btn is clicked
 */
void MainWindow::slotEditNodeAdd()
{
    qCDebug(lcMainWindow) << "Request to add a new random node...";
    activeGraph->vertexCreateAtPosRandom(true);
    statusMessage(tr("New random positioned node (numbered %1) added.")
                      .arg(activeGraph->vertexNumberMax()));
}

/**
 * @brief Opens the Find Node dialog
 */
void MainWindow::slotEditNodeFindDialog()
{
    qCDebug(lcMainWindow) << "Showing find node dialog...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    //@TODO - prominenceIndexList should be either
    // the list of all computes indices
    // or the last computed indice
    // or empty if the user has not computed any index yet.
    m_nodeFindDialog = new DialogNodeFind(this, prominenceIndexList);

    connect(m_nodeFindDialog, &DialogNodeFind::userChoices,
            this, &MainWindow::slotEditNodeFind);

    m_nodeFindDialog->exec();

    statusMessage(tr("Node find dialog opened. Enter your choices. "));

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

    qCDebug(lcMainWindow) << "Request to find nodes:" << nodeList
             << "search type:" << searchType
             << "indexStr" << indexStr;

    int indexType = 0;

    if (searchType == "numbers")
    {
        activeGraph->vertexFindByNumber(nodeList);
    }
    else if (searchType == "labels")
    {
        activeGraph->vertexFindByLabel(nodeList);
    }
    else if (searchType == "score")
    {

        indexType = activeGraph->getProminenceIndexByName(indexStr);

        const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
        const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
        const bool inverseWeightsFinal = inverseWeights;

        runGraphOperationAsync(
            [this, indexType, nodeList, considerWeights, inverseWeightsFinal, dropIsolates]() {
                activeGraph->vertexFindByIndexScore(indexType,
                                                    nodeList,
                                                    considerWeights,
                                                    inverseWeightsFinal,
                                                    dropIsolates);
            },
            tr("Finding nodes by index score. Please wait..."));
    }

    return;
}

/**
 * @brief Handles requests to delete a node and the attached objects (edges, etc).
 *
 * If the user has clicked on a node, it deletes it
 * Else it asks for a nodeNumber to remove.
 */
void MainWindow::slotEditNodeRemove()
{
    qCDebug(lcMainWindow) << "Request to remove a node...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->relations() > 1)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. Cannot remove node!"),
                              tr("Error. Cannot remove this node!"),
                              tr("This a network with more than 1 relations. If you remove "
                                 "a node from the active relation, and then ask me to go "
                                 "to the previous or the next relation, then I would crash "
                                 "because I would try to display edges from a deleted node."
                                 "You cannot remove nodes in multirelational networks."));
        return;
    }

    // if there are already multiple nodes selected, erase them
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if (nodesSelected > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        qCDebug(lcMainWindow) << "multiple nodes selected to be removed";
        foreach (int nodeNumber, activeGraph->getSelectedVertices())
        {
            activeGraph->vertexRemove(nodeNumber);
        }
        editNodeRemoveAct->setText(tr("Remove Node"));
        statusMessage(tr("Removed %1 nodes.").arg(nodesSelected));
        QApplication::restoreOverrideCursor();
    }

    else
    {
        int nodeNumber = -1, min = -1, max = -1;
        bool ok = false;
        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        if (min == -1 || max == -1)
        {
            qCDebug(lcMainWindow, "ERROR in finding min max nodeNumbers. Abort");
            return;
        }
        else
        {
            nodeNumber = QInputDialog::getInt(
                this,
                tr("Remove node"),
                tr("Choose a node to remove between (" + QString::number(min).toLatin1() + "..." +
                   QString::number(max).toLatin1() + "):"),
                min, 1, max, 1, &ok);
            if (!ok)
            {
                statusMessage("Remove node operation cancelled.");
                return;
            }
        }
        qCDebug(lcMainWindow, "removing vertex with number %i from Graph", nodeNumber);
        activeGraph->vertexRemove(nodeNumber);
        qCDebug(lcMainWindow, "Completed. Node %i removed completely.", nodeNumber);
        statusMessage(tr("Node removed completely."));
    }
}

/**
 * @brief Opens the Node Properties dialog for the selected nodes.
 * If no nodes are selected, prompts the user for a node number.
 */
void MainWindow::slotEditNodePropertiesDialog()
{
    qCDebug(lcMainWindow) << "Request to open the node properties dialog...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount == 0)
    {
        statusMessage(tr("Select a node first to edit its properties."));
        return;
    }

    if (selectedNodesCount > 1)
    {
        slotEditNodeSetPropertyForSelection();
        return;
    }

    // Exactly one node selected — open the full properties dialog
    const int nodeNumber = activeGraph->getSelectedVertices().constFirst();
    const QString label = activeGraph->vertexLabel(nodeNumber);
    const QColor color = activeGraph->vertexColor(nodeNumber);
    const QString shape = activeGraph->vertexShape(nodeNumber);
    const int size = activeGraph->vertexSize(nodeNumber);
    const QString iconPath = activeGraph->vertexShapeIconPath(nodeNumber);
    const QHash<QString, QString> customAttributes = activeGraph->vertexCustomAttributes(nodeNumber);

    qCDebug(lcMainWindow) << "opening DialogNodeEdit for node" << nodeNumber;

    std::unique_ptr<DialogNodeEdit> m_nodeEditDialog = std::make_unique<DialogNodeEdit>(
        this, nodeShapeList, iconPathList, label, size, color, shape, iconPath, customAttributes);

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
                                        const QHash<QString, QString> &customAttributes)
{

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    qCDebug(lcMainWindow) << "Request to update node properties - new properties: "
             << " label " << label
             << " size " << size
             << " color " << color
             << " shape " << shape
             << " vertexClicked " << activeGraph->vertexClicked()
             << " selectedNodesCount " << selectedNodesCount
             << "customAttributes" << customAttributes;

    if (selectedNodesCount == 0 && activeGraph->vertexClicked() != 0)
    {
        // no node selected but user entered a node number in a dialog
        if (label != "" && appSettings["initNodeLabelsVisibility"] != "true")
            slotOptionsNodeLabelsVisibility(true);
        activeGraph->vertexLabelSet(activeGraph->vertexClicked(), label);
        activeGraph->vertexColorSet(activeGraph->vertexClicked(), color.name());
        activeGraph->vertexSizeSet(activeGraph->vertexClicked(), size);
        activeGraph->vertexShapeSet(activeGraph->vertexClicked(), shape, iconPath);
        activeGraph->vertexCustomAttributesSet(activeGraph->vertexClicked(), customAttributes);

        statusMessage(tr("Updated the properties of node %1. ").arg(activeGraph->vertexClicked()));
    }
    else
    {
        // some nodes are selected
        int nodeNumber = 0;
        foreach (nodeNumber, activeGraph->getSelectedVertices())
        {
            qCDebug(lcMainWindow) << "node " << nodeNumber;
            if (!label.isEmpty())
            {
                if (selectedNodesCount > 1)
                {
                    activeGraph->vertexLabelSet(
                        nodeNumber,
                        label + QString::number(nodeNumber));
                }
                else
                {
                    activeGraph->vertexLabelSet(nodeNumber, label);
                }
                // turn on labels visibility if they are hidden
                if (appSettings["initNodeLabelsVisibility"] != "true")
                {
                    slotOptionsNodeLabelsVisibility(true);
                }
            }
            activeGraph->vertexColorSet(nodeNumber, color.name());
            activeGraph->vertexSizeSet(nodeNumber, size);
            activeGraph->vertexShapeSet(nodeNumber, shape, iconPath);
            activeGraph->vertexCustomAttributesSet(nodeNumber, customAttributes);
        }
        statusMessage(tr("Updated the properties of %1 nodes. ").arg(selectedNodesCount));
    }
}

/**
 * @brief Syncs the Data Table selection to match the canvas selection.
 *
 * Connected to GraphicsWidget::userSelectedItems. Always updates the table's
 * selection model (even when the dock is hidden) so that resolveNodeTargets /
 * resolveEdgeTargets return correct rows when the user later opens the dock
 * and clicks "Set property..." or "Remove attribute...".
 *
 * When only nodes are selected, switches the visible tab to Nodes; when only
 * edges are selected, switches to Edges.
 */
void MainWindow::slotCacheSelection(const QList<int> &nodes,
                                    const QList<SelectedEdge> &edges)
{
    if (!m_tableWidget)
        return;

    // Auto-switch tab when dock is visible:
    // only go to Edges tab when the selection is entirely edges (no nodes).
    if (m_tableDock && m_tableDock->isVisible())
    {
        if (!edges.isEmpty() && nodes.isEmpty())
            m_tableWidget->showEdgesTab();
        else
            m_tableWidget->showNodesTab();
    }

    m_tableWidget->syncNodeSelection(nodes);
    m_tableWidget->syncEdgeSelection(edges);
}

/**
 * @brief Raises the Data Table dock and pre-selects rows matching the current canvas selection.
 */
void MainWindow::slotEditNodeEditSelectionInTable()
{
    if (!m_tableDock || !m_tableWidget)
        return;

    m_tableDock->show();
    m_tableDock->raise();

    const QList<int> nodes = activeGraph->getSelectedVertices();
    const QList<SelectedEdge> edges = activeGraph->getSelectedEdges();

    // Only switch to Edges tab when the selection is entirely edges (no nodes)
    if (!edges.isEmpty() && nodes.isEmpty())
        m_tableWidget->showEdgesTab();
    else
        m_tableWidget->showNodesTab();

    m_tableWidget->syncNodeSelection(nodes);
    m_tableWidget->syncEdgeSelection(edges);
}

/**
 * @brief Opens DialogBulkEdit for the currently selected nodes (canvas shortcut).
 */
void MainWindow::slotEditNodeSetPropertyForSelection()
{
    const QList<int> selectedNodes = activeGraph->getSelectedVertices();
    const int count = selectedNodes.size();
    if (count == 0)
    {
        statusMessage(tr("No nodes selected."));
        return;
    }

    // Collect unique custom attribute keys across the selection
    QSet<QString> keySet;
    for (const int v : selectedNodes)
        for (const QString &k : activeGraph->vertexCustomAttributes(v).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    auto dlg = std::make_unique<DialogBulkEdit>(
        DialogBulkEdit::Scope::Nodes, existingKeys,
        nodeShapeList, iconPathList, count, false, this);

    connect(dlg.get(), &DialogBulkEdit::userChoices,
            this, [this, selectedNodes](const QString &property, const QString &value)
            {
        // Resolved once here (doesn't depend on which vertex) rather than per-iteration inside
        // the dispatched lambda below, which also keeps that lambda from reading
        // nodeShapeList/iconPathList (MainWindow-owned) on graphThread at all.
        QString shapeIconPath;
        if (property == QLatin1String("Shape")) {
            const int idx = nodeShapeList.indexOf(value);
            shapeIconPath = (idx >= 0 && idx < iconPathList.size()) ? iconPathList[idx] : QString();
        }
        runGraphOperationAsync(
            [this, selectedNodes, property, value, shapeIconPath]() {
                for (const int v : selectedNodes) {
                    if (property == QLatin1String("Label")) {
                        activeGraph->vertexLabelSet(v, value);
                    } else if (property == QLatin1String("Size")) {
                        activeGraph->vertexSizeSet(v, value.toInt());
                    } else if (property == QLatin1String("Color")) {
                        activeGraph->vertexColorSet(v, value);
                    } else if (property == QLatin1String("Shape")) {
                        activeGraph->vertexShapeSet(v, value, shapeIconPath);
                    } else {
                        activeGraph->vertexCustomAttributeSet(v, property, value);
                    }
                }
            },
            tr("Applying bulk edit..."),
            [this, selectedNodes, property]() {
                statusMessage(tr("Set '%1' on %2 node(s).").arg(property).arg(selectedNodes.size()));
                if (m_tableDock && m_tableDock->isVisible())
                    m_tableWidget->refresh(activeGraph);
            }); });

    dlg->exec();
}

/**
 * @brief Opens DialogBulkEdit for the currently selected edges (canvas shortcut).
 */
void MainWindow::slotEditEdgeSetPropertyForSelection()
{
    const QList<SelectedEdge> selectedEdges = activeGraph->getSelectedEdges();
    const int count = selectedEdges.size();
    if (count == 0)
    {
        statusMessage(tr("No edges selected."));
        return;
    }

    // Collect unique custom attribute keys across the selection
    QSet<QString> keySet;
    for (const SelectedEdge &e : selectedEdges)
        for (const QString &k : activeGraph->edgeCustomAttributes(e.first, e.second).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    auto dlg = std::make_unique<DialogBulkEdit>(
        DialogBulkEdit::Scope::Edges, existingKeys,
        QStringList(), QStringList(), count, false, this);

    connect(dlg.get(), &DialogBulkEdit::userChoices,
            this, [this, selectedEdges](const QString &property, const QString &value)
            {
        runGraphOperationAsync(
            [this, selectedEdges, property, value]() {
                for (const SelectedEdge &e : selectedEdges) {
                    if (property == QLatin1String("Label")) {
                        activeGraph->edgeLabelSet(e.first, e.second, value);
                    } else if (property == QLatin1String("Weight")) {
                        activeGraph->edgeWeightSet(e.first, e.second, value.toDouble());
                    } else if (property == QLatin1String("Color")) {
                        activeGraph->edgeColorSet(e.first, e.second, value);
                    } else {
                        activeGraph->edgeCustomAttributesSet(
                            e.first, e.second, {{property, value}});
                    }
                }
            },
            tr("Applying bulk edit..."),
            [this, selectedEdges, property]() {
                statusMessage(tr("Set '%1' on %2 edge(s).").arg(property).arg(selectedEdges.size()));
                if (m_tableDock && m_tableDock->isVisible())
                    m_tableWidget->refresh(activeGraph);
            }); });

    dlg->exec();
}

/**
 * @brief Creates a complete subgraph (clique) from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToClique()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToClique()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Error. Not enough nodes selected."),
                              tr("Cannot create new clique because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_CLIQUE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Clique created."),
                                  tr("A new clique has been created from ") + QString::number(selectedNodesCount) + tr(" nodes"));
        });
}

/**
 * @brief Creates a star subgraph from selected nodes.
 * User must choose a central node.
 */
void MainWindow::slotEditNodeSelectedToStar()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToStar()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new star subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    int center;
    bool ok = false;

    int min = activeGraph->getSelectedVerticesMin();
    int max = activeGraph->getSelectedVerticesMax();
    center = QInputDialog::getInt(
        this,
        "Create star subgraph",
        tr("To create a star subgraph from selected nodes, \n"
           "enter the number of the central actor (" +
           QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "):"),
        min, 1, max, 1, &ok);
    if (!ok)
    {
        statusMessage("Create star subgraph cancelled.");
        return;
    }

    runGraphOperationAsync(
        [this, center]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_STAR, center);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Star subgraph created."),
                                  tr("A new star subgraph has been created with ") +
                                      QString::number(selectedNodesCount) + tr(" nodes."));
        });
}

/**
 * @brief Creates a cycle subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToCycle()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToCycle()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new cycle subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_CYCLE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Cycle subgraph created."),
                                  tr("A new cycle subgraph has been created with ") + QString::number(selectedNodesCount) + tr(" select nodes."));
        });
}

/**
 * @brief Creates a line subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToLine()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToLine()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new line subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_LINE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Line subgraph created."),
                                  tr("A new line subgraph has been created with ") + QString::number(selectedNodesCount) + tr(" selected nodes."));
        });
}

/**
 * @brief Changes the color of all nodes to parameter color
 *
 * If the color is invalid, opens a QColorDialog to
 * select a new node color for all nodes.
 *
 * @param color
 */
void MainWindow::slotEditNodeColorAll(QColor color)
{
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeColor"]),
                                       this,
                                       "Change the color of all nodes");
    }
    if (color.isValid())
    {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        qCDebug(lcMainWindow) << "MW::slotEditNodeColorAll() : "
                 << appSettings["initNodeColor"];
        activeGraph->vertexColorSet(0, appSettings["initNodeColor"]);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Change all nodes' color. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
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
void MainWindow::slotEditNodeSizeAll(int newSize, const bool &normalized)
{
    Q_UNUSED(normalized);
    qCDebug(lcMainWindow) << "MW: slotEditNodeSizeAll() - "
             << " newSize " << newSize;
    if (newSize == 0 && !normalized)
    {
        bool ok = true;
        newSize = QInputDialog::getInt(
            this,
            "Change node size",
            tr("Select new size for all nodes:"),
            appSettings["initNodeSize"].toInt(0, 10), 1, 100, 1, &ok);

        if (!ok)
        {
            statusMessage("Change node size operation cancelled.");
            return;
        }
    }

    appSettings["initNodeSize"] = QString::number(newSize);

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
                                   QString nodeIconPath)
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeShape() - vertex " << vertex
             << "(0 means all)"
             << "new shape" << shape
             << "nodeIconPath" << nodeIconPath;

    if (shape.isNull())
    {

        bool ok = false;

        int curShapeIndex = nodeShapeList.indexOf(appSettings["initNodeShape"]);

        if (curShapeIndex == -1)
        {
            curShapeIndex = 1;
        }
        shape = QInputDialog::getItem(this,
                                      "Node shape",
                                      "Select a shape for all nodes: ",
                                      nodeShapeList, curShapeIndex, true, &ok);
        if (!ok)
        {
            // user pressed Cancel
            statusMessage(tr("Change node shapes aborted."));
            return;
        }
        if (shape == "custom")
        {
            nodeIconPath = QFileDialog::getOpenFileName(
                this, tr("Select an icon"), getLastPath(),
                tr("Images (*.png *.jpg *.jpeg *.svg);;All (*.*)"));
            if (nodeIconPath.isNull())
            {
                // user pressed Cancel
                statusMessage(tr("Change node shapes aborted."));
                return;
            }
        }
        else
        {
            nodeIconPath = iconPathList[nodeShapeList.indexOf(shape)];
        }
    }

    if (vertex == 0)
    { // change all nodes shapes
        activeGraph->vertexShapeSet(-1, shape, nodeIconPath);
        appSettings["initNodeShape"] = shape;
        appSettings["initNodeIconPath"] = nodeIconPath;
        statusMessage(tr("All shapes have been changed."));
    }
    else
    { // only one
        activeGraph->vertexShapeSet(vertex, shape, nodeIconPath);
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
void MainWindow::slotEditNodeNumberSize(int v1, int newSize, const bool prompt)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumberSize - newSize " << newSize;
    if (prompt)
    {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node numbers size to: (1-16)"),
                                       appSettings["initNodeNumberSize"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change font size: Aborted."));
            return;
        }
    }
    if (v1)
    { // change one node number only
        activeGraph->vertexNumberSizeSet(v1, newSize);
    }
    else
    { // change all
        appSettings["initNodeNumberSize"] = QString::number(newSize);
        activeGraph->vertexNumberSizeSet(0, newSize);
    }
    statusMessage(tr("Changed node numbers size."));
}

/**
 * @brief Changes the text color of all node numbers
 * Called from Edit menu option and Settings dialog.
 * If color is invalid, asks the user to enter a new node number color
 * @param color
 */
void MainWindow::slotEditNodeNumbersColor(const int &v1, QColor color)
{
    qCDebug(lcMainWindow) << "MW:slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeNumberColor"]),
                                       this,
                                       "Change the color of all node numbers");
    }

    if (color.isValid())
    {

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (v1)
        {
            activeGraph->vertexNumberColorSet(v1, color.name());
        }
        else
        {
            appSettings["initNodeNumberColor"] = color.name();
            activeGraph->vertexNumberColorSet(0, color.name());
        }

        QApplication::restoreOverrideCursor();
        statusMessage(tr("Node number color changed. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
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
void MainWindow::slotEditNodeNumberDistance(int v1, int newDistance)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumberDistance - newSize " << newDistance;
    if (!newDistance)
    {
        newDistance = QInputDialog::getInt(
            this, "Change node number distance",
            tr("Change all node numbers distance from their nodes to: (1-16)"),
            appSettings["initNodeNumberDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change node number distance aborted."));
            return;
        }
    }
    if (v1)
    { // change one node number distance only
        activeGraph->vertexNumberDistanceSet(v1, newDistance);
    }
    else
    { // change all
        appSettings["initNodeNumberDistance"] = QString::number(newDistance);
        activeGraph->vertexNumberDistanceSet(0, newDistance);
    }
    statusMessage(tr("Changed node number distance."));
}

/**
 * @brief Changes the size of one or all node Labels.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node Label font size
 * if v1=0, it changes all node Labels
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeLabelSize(const int v1, int newSize)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeLabelSize - newSize " << newSize;
    if (!newSize)
    {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node labels text size to: (1-16)"),
                                       appSettings["initNodeLabelSize"].toInt(0, 10), 1, 32, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change font size: Aborted."));
            return;
        }
    }
    if (v1)
    { // change one node Label only
        activeGraph->vertexLabelSizeSet(v1, newSize);
    }
    else
    { // change all
        appSettings["initNodeLabelSize"] = QString::number(newSize);
        activeGraph->vertexLabelSizeSet(0, newSize);
    }
    statusMessage(tr("Changed node label size."));
}

/**
 * @brief Changes the color of all node labels.
 * Asks the user to enter a new node label color
 */
void MainWindow::slotEditNodeLabelsColor(QColor color)
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeLabelColor"]),
                                       this,
                                       "Change the color of all node labels");
    }
    if (color.isValid())
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        activeGraph->vertexLabelColorSet(0, color.name());
        appSettings["initNodeLabelColor"] = color.name();
        optionsNodeLabelsVisibilityAct->setChecked(true);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Label colors changed. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
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
void MainWindow::slotEditNodeLabelDistance(int v1, int newDistance)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeLabelDistance - newSize " << newDistance;
    if (!newDistance)
    {
        newDistance = QInputDialog::getInt(
            this, "Change node label distance",
            tr("Change all node labels distance from their nodes to: (1-16)"),
            appSettings["initNodeLabelDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change node label distance aborted."));
            return;
        }
    }
    if (v1)
    { // change one node label distance only
        activeGraph->vertexLabelDistanceSet(v1, newDistance);
    }
    else
    { // change all
        appSettings["initNodeLabelDistance"] = QString::number(newDistance);
        activeGraph->vertexLabelDistanceAllSet(newDistance);
    }
    statusMessage(tr("Changed node label distance."));
}

/**
 * @brief Shows a context menu when the user right-clicks directly on a node.
 *
 * The menu is always selection-aware — it reflects the total number of currently
 * selected nodes, not just the node that was right-clicked:
 * - Single node selected: offers Node Properties (full per-node dialog) and
 *   "Edit Selection in Data Table" to navigate to that row in the table.
 * - Multiple nodes selected: offers "Edit Selection in Data Table",
 *   "Set property on selected nodes…", and structural transforms instead of
 *   per-node properties (which would be ambiguous for mixed selections).
 * - Always offers Add Edge, Remove, filter actions, and Ego Radial Layout.
 */
void MainWindow::slotEditNodeOpenContextMenu()
{

    qCDebug(lcMainWindow, "MW: slotEditNodeOpenContextMenu() for node %i at %i, %i",
           activeGraph->vertexClicked(), QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number(activeGraph->vertexClicked()), this);
    Q_CHECK_PTR(nodeContextMenu); // displays "out of memory" if needed
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if (nodesSelected == 1)
    {
        nodeContextMenu->addAction(
            tr("## NODE ") + QString::number(activeGraph->vertexClicked()) + " ##  ");
    }
    else
    {
        nodeContextMenu->addAction(
            tr("## NODE ") + QString::number(activeGraph->vertexClicked()) + " ##  " + tr(" (selected nodes: ") + QString::number(nodesSelected) + ")");
    }

    nodeContextMenu->addSeparator();

    if (nodesSelected == 1)
    {
        // Single node: full per-node properties dialog is appropriate
        nodeContextMenu->addAction(editNodePropertiesAct);
    }
    else
    {
        // Multiple nodes: bulk-set shortcut instead of the ambiguous single-node dialog
        nodeContextMenu->addAction(editNodeSetPropertyForSelectionAct);
    }
    // Always offer "Edit in Data Table" so the user can inspect or bulk-edit
    // the selected row(s) regardless of how many nodes are selected
    nodeContextMenu->addAction(editNodeEditSelectionInTableAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editEdgeAddAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editNodeRemoveAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(filterNodesBySelectionAct);
    nodeContextMenu->addAction(filterNodesByEgoNetworkAct);
    nodeContextMenu->addAction(filterNodesRestoreAllAct);

    nodeContextMenu->addSeparator();
    nodeContextMenu->addAction(tr("Ego Radial layout"), this, &MainWindow::slotLayoutEgoRadial);
    nodeContextMenu->addSeparator();

    // QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu->exec(QCursor::pos());
    delete nodeContextMenu;
}

/**
 * @brief Updates the UI (LCDs and Actions) after a change in the user-selected nodes/edges
 *
 * @param nodes
 * @param edges
 */
void MainWindow::slotEditSelectionChanged(const int &selNodes, const int &selEdges)
{
    qCDebug(lcMainWindow) << "Updating UI for new selection";
    rightPanelSelectedNodesLCD->setText(QString::number(selNodes));
    rightPanelSelectedEdgesLCD->setText(QString::number(selEdges));

    if (selNodes > 1)
    {
        editNodeRemoveAct->setText(tr("Remove ") + QString::number(selNodes) + tr(" nodes"));
        editNodeSelectedToCliqueAct->setEnabled(true);
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToStarAct->setEnabled(true);
        editNodeSelectedToStarAct->setText(tr("Create a star from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(true);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToLineAct->setEnabled(true);
        editNodeSelectedToLineAct->setText(tr("Create a line from ") + QString::number(selNodes) + tr(" selected nodes"));
    }
    else
    {
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

    // Enable selection focus whenever at least 1 node is selected
    filterNodesBySelectionAct->setEnabled(selNodes >= 1);
    // Enable ego network focus only when exactly 1 node is selected
    filterNodesByEgoNetworkAct->setEnabled(selNodes == 1);
    // Enable subgraph-from-selection whenever at least 1 node is selected
    editSubgraphExtractFromSelectionAct->setEnabled(selNodes >= 1);

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
void MainWindow::slotEditNodeInfoStatusBar(const int &number,
                                           const QPointF &p,
                                           const QString &label,
                                           const int &inDegree,
                                           const int &outDegree)
{

    qCDebug(lcMainWindow) << "Updating node info in status bar...";
    rightPanelClickedNodeLCD->setText(QString::number(number));
    const bool nodeClicked = (number != 0);
    rightPanelClickedNodeInDegreeLabel->setVisible(nodeClicked);
    rightPanelClickedNodeInDegreeLCD->setVisible(nodeClicked);
    rightPanelClickedNodeOutDegreeLabel->setVisible(nodeClicked);
    rightPanelClickedNodeOutDegreeLCD->setVisible(nodeClicked);
    if (nodeClicked)
    {
        rightPanelClickedNodeInDegreeLCD->setText(QString::number(inDegree));
        rightPanelClickedNodeOutDegreeLCD->setText(QString::number(outDegree));
    }

    if (number != 0)
    {

        statusMessage(QString(tr("Position (%1, %2):  Node %3, label %4 - "
                                 "In-Degree: %5, Out-Degree: %6"))
                          .arg(ceil(p.x()))
                          .arg(ceil(p.y()))
                          .arg(number)
                          .arg((label == "") ? "unset" : label)
                          .arg(inDegree)
                          .arg(outDegree));
    }
    else
    {
        statusMessage(tr("Position (%1,%2): Double-click to create a new node.")
                          .arg(p.x())
                          .arg(p.y()));
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
void MainWindow::slotEditEdgeClicked(const MyEdge &edge,
                                     const bool &openMenu)
{

    int v1 = edge.source;
    int v2 = edge.target;
    qreal weight = edge.weight;
    qreal reverseWeight = edge.rWeight;
    int type = edge.type;


    if (v1 == 0 || v2 == 0)
    {
        rightPanelClickedEdgeNameLCD->setText("-");
        rightPanelClickedEdgeWeightLabel->setVisible(false);
        rightPanelClickedEdgeWeightLCD->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);
        return;
    }

    rightPanelClickedEdgeWeightLabel->setVisible(true);
    rightPanelClickedEdgeWeightLCD->setVisible(true);

    QString edgeName;

    if (type == EdgeType::Undirected)
    {
        statusMessage(QString(tr("Undirected edge %1 <--> %2 of weight %3 has been selected. "
                                 "Click anywhere else to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" -- ") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);
        if (openMenu)
        {
            edgeName = QString("EDGE: ") + QString::number(v1) + QString(" -- ") + QString::number(v2);
        }
    }
    else if (type == EdgeType::Reciprocated)
    {
        statusMessage(QString(tr("Reciprocated edge %1 <--> %2 has been selected. "
                                 "Weight %1 --> %2 = %3, "
                                 "Weight %2 --> %1 = %4. "
                                 "Click anywhere else to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight)
                          .arg(reverseWeight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" <-->") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setText("Recipr.:");
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(true);
        rightPanelClickedEdgeReciprocalWeightLCD->setText(QString::number(reverseWeight));
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(true);
        if (openMenu)
        {
            edgeName = QString("RECIPROCATED EDGE: ") + QString::number(v1) + QString(" <-->") + QString::number(v2);
        }
    }
    else
    {
        statusMessage(QString(tr("Directed edge %1 --> %2 of weight %3 has been selected. "
                                 "Click again to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" -->") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);

        if (openMenu)
        {
            edgeName = QString("DIRECTED EDGE: ") + QString::number(v1) + QString(" -->") + QString::number(v2);
        }
    }

    if (openMenu)
    {
        slotEditEdgeOpenContextMenu(edgeName);
    }
}

/**
 * @brief Shows a context menu when the user right-clicks directly on an edge.
 *
 * The menu is selection-aware — it reflects the total number of currently
 * selected edges:
 * - Single edge selected: offers Edge Properties (per-edge dialog for label,
 *   weight, color, custom attributes) plus Remove, Weight, Label, Color actions.
 * - Multiple edges selected: offers "Edit Selection in Data Table" and
 *   "Set property on selected edges…" at the top, followed by per-edge actions.
 *
 * @param str  A human-readable identifier for the edge shown as the menu title
 *             (e.g. "3 --> 7").
 */
void MainWindow::slotEditEdgeOpenContextMenu(const QString &str)
{
    qCDebug(lcMainWindow) << "MW: slotEditEdgeOpenContextMenu() for" << str
             << "at" << QCursor::pos().x() << "," << QCursor::pos().y();

    const int edgesSelected = activeGraph->getSelectedEdgesCount();

    QMenu *edgeContextMenu = new QMenu(str, this);
    edgeContextMenu->addAction(str);
    edgeContextMenu->addSeparator();

    if (edgesSelected > 1)
    {
        // Multiple edges selected: offer bulk actions first
        edgeContextMenu->addAction(editNodeEditSelectionInTableAct);
        edgeContextMenu->addAction(editEdgeSetPropertyForSelectionAct);
        edgeContextMenu->addSeparator();
    }
    else
    {
        // Single edge: per-edge properties
        edgeContextMenu->addAction(editEdgePropertiesAct);
        edgeContextMenu->addSeparator();
    }

    edgeContextMenu->addAction(editEdgeRemoveAct);
    edgeContextMenu->addAction(editEdgeWeightAct);
    edgeContextMenu->addAction(editEdgeLabelAct);
    edgeContextMenu->addAction(editEdgeColorAct);
    edgeContextMenu->exec(QCursor::pos());
    delete edgeContextMenu;
}

/**
 * @brief Opens dialogs for the user to specify the source and the target node of a new edge
 *
 * Called when user clicks on the MW button/menu item "Add edge"
 *
 */
void MainWindow::slotEditEdgeAdd()
{
    qCDebug(lcMainWindow) << "Request to add a new edge through UI. Opening source/target node dialogs...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    qreal weight = 1; // weight of this new edge should be one...
    bool ok = false;
    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (min == max)
        return; // if there is only one node->no edge

    if (!activeGraph->vertexClicked())
    {
        sourceNode = QInputDialog::getInt(
            this,
            "Create new edge, Step 1",
            tr("This will draw a new edge between two nodes. \n"
               "Enter source node (" +
               QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Add edge operation cancelled.");
            return;
        }
    }
    else
        sourceNode = activeGraph->vertexClicked();

    qCDebug(lcMainWindow) << "sourceNode:" << sourceNode;

    if (!activeGraph->vertexExists(sourceNode))
    {
        qCDebug(lcMainWindow) << "Cannot find sourceNode" << sourceNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?"));
        return;
    }

    targetNode = QInputDialog::getInt(this, "Create new edge, Step 2",
                                      tr("Source node:") + QString::number(sourceNode) + tr(" \nNow enter a target node [") + QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "]:", min, min, max, 1, &ok);
    if (!ok)
    {
        statusMessage("Add edge target operation cancelled.");
        return;
    }
    if (!activeGraph->vertexExists(targetNode))
    {
        qCDebug(lcMainWindow) << "Cannot find targetNode" << targetNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?"));
        return;
    }

    weight = QInputDialog::getDouble(
        this, "Create new edge, Step 3",
        tr("Source and target nodes accepted. \n"
           "Please, enter the weight of new edge: "),
        1.0, -100.0, 100.0, 1, &ok);
    if (!ok)
    {
        statusMessage("Add edge operation cancelled.");
        return;
    }
    // Check if this edge already exists...
    if (activeGraph->edgeExists(sourceNode, targetNode) != 0)
    {
        qCDebug(lcMainWindow, "edge exists. Aborting");
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That edge already exists!"),
                              tr("Error. That edge already exists!"),
                              tr("Are you sure you entered the correct node numbers?"));
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
void MainWindow::slotEditEdgeCreate(const int &source, const int &target, const qreal &weight)
{
    qCDebug(lcMainWindow) << "User requested to create a new edge"
             << source << "->" << target << "weight" << weight
             << "Setting user settings and calling Graph to to do the job...";

    bool bezier = false;
    bool result = activeGraph->edgeCreate(
        source, target, weight,
        appSettings["initEdgeColor"],
        (editEdgeUndirectedAllAct->isChecked()) ? 2 : 0,
        (editEdgeUndirectedAllAct->isChecked()) ? false : ((appSettings["initEdgeArrows"] == "true") ? true : false), bezier);

    if (result)
    {
        statusMessage(tr("New edge %1 -> %2 created, weight %3.").arg(source).arg(target).arg(weight));
    }
}

/**
 * @brief Opens the Edge Properties dialog for the currently clicked edge.
 */
void MainWindow::slotEditEdgePropertiesDialog()
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditEdgePropertiesDialog()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QList<SelectedEdge> selectedEdges = activeGraph->getSelectedEdges();
    const int selectedEdgesCount = selectedEdges.size();

    if (selectedEdgesCount == 0)
    {
        statusMessage(tr("Select an edge first to edit its properties."));
        return;
    }

    if (selectedEdgesCount > 1)
    {
        slotEditEdgeSetPropertyForSelection();
        return;
    }

    // Exactly one edge selected — open the full properties dialog
    int v1 = selectedEdges.constFirst().first;
    int v2 = selectedEdges.constFirst().second;

    // A reciprocated edge is one GraphicsEdge item, but two independent directed arcs in the
    // model (each with its own weight/label/color) - ask which direction to edit rather than
    // silently always editing whichever direction was created first. Same pattern as the
    // direction-choice dialog in slotEditEdgeRemove(). See #251, #264.
    activeGraph->edgeClickedSet(v1, v2);
    if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
    {
        QStringList items;
        QString arcA = QString::number(v1) + " -->" + QString::number(v2);
        QString arcB = QString::number(v2) + " -->" + QString::number(v1);
        items << arcA << arcB;

        bool ok = false;
        QString selectedArc = QInputDialog::getItem(
            this, tr("Select edge"),
            tr("This is a reciprocated edge. "
               "Select direction to edit properties for:"),
            items, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Edge properties operation cancelled."));
            return;
        }
        if (selectedArc == arcB)
        {
            const int tmp = v1;
            v1 = v2;
            v2 = tmp;
        }
    }

    const QString label = activeGraph->edgeLabel(v1, v2);
    const double weight = static_cast<double>(activeGraph->edgeWeight(v1, v2));
    const QColor color = QColor(activeGraph->edgeColor(v1, v2));
    const QHash<QString, QString> attrs = activeGraph->edgeCustomAttributes(v1, v2);

    DialogEdgeEdit *dialog = new DialogEdgeEdit(this, v1, v2, label, weight, color, attrs);
    connect(dialog, &DialogEdgeEdit::userChoices,
            this, [this, v1, v2](const QString &label, const double &weight, const QColor &color, const QHash<QString, QString> &customAttributes)
            { slotEditEdgeProperties(v1, v2, label, weight, color, customAttributes); });
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

/**
 * @brief Applies updated edge properties received from DialogEdgeEdit.
 */
void MainWindow::slotEditEdgeProperties(const int &v1,
                                        const int &v2,
                                        const QString &label,
                                        const double &weight,
                                        const QColor &color,
                                        const QHash<QString, QString> &customAttributes)
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditEdgeProperties()";
    if (v1 == 0 || v2 == 0)
        return;

    activeGraph->edgeLabelSet(v1, v2, label);
    activeGraph->edgeWeightSet(v1, v2, static_cast<qreal>(weight));
    activeGraph->edgeColorSet(v1, v2, color.name());
    activeGraph->edgeCustomAttributesSet(v1, v2, customAttributes);

    statusMessage(tr("Updated properties of edge %1 \u2192 %2.").arg(v1).arg(v2));
}

/**
 * @brief Removes a clicked edge. Otherwise asks the user to specify one edge.
 */
void MainWindow::slotEditEdgeRemove()
{

    qCDebug(lcMainWindow) << "Removing selected edges...";

    if (!activeNodes() || activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int min = 0, max = 0, sourceNode = -1, targetNode = -1;
    bool ok = false;
    bool removeOpposite = false;

    int selectedEdgeCount = activeGraph->getSelectedEdgesCount();

    qCDebug(lcMainWindow) << "Selected edges:" << selectedEdgeCount;

    if (!selectedEdgeCount)
    {

        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - No edge selected. "
                    "Prompting user to select...";

        sourceNode = QInputDialog::getInt(
            this, tr("Remove edge"),
            tr("Source node:  (") + QString::number(min) +
                "..." + QString::number(max) + "):",
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Remove edge operation cancelled.");
            return;
        }

        targetNode = QInputDialog::getInt(
            this,
            tr("Remove edge"),
            tr("Target node:  (") + QString::number(min) + "..." +
                QString::number(max) + "):",
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Remove edge operation cancelled.");
            return;
        }
        if (activeGraph->edgeExists(sourceNode, targetNode, false) != 0)
        {
            removeOpposite = false;
            if (activeGraph->isUndirected())
            {
                removeOpposite = true;
            }
        }
        else
        {
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));
            return;
        }
    }
    else
    {

        if (selectedEdgeCount > 1)
        {

            qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - Multiple edges selected. "
                        "Calling Graph to remove all of them...";

            activeGraph->edgeRemoveSelectedAll();
            return;
        }

        // Exactly one edge is selected, but it may have been selected via rubber-band rather
        // than an individual click - in that case edgeClicked() below still holds stale (or
        // default 0,0) state from whatever was last individually clicked, not the actual
        // selected edge. Sync it explicitly so the reciprocated-edge dialog below shows the
        // right node numbers regardless of how the edge was selected.
        {
            const SelectedEdge selected = activeGraph->getSelectedEdges().first();
            activeGraph->edgeClickedSet(selected.first, selected.second);
        }

        qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - One edge selected: "
                 << activeGraph->edgeClicked().source
                 << "->"
                 << activeGraph->edgeClicked().target;

        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
        {

            QStringList items;

            QString arcA = QString::number(activeGraph->edgeClicked().source) + " -->" + QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number(activeGraph->edgeClicked().target) + " -->" + QString::number(activeGraph->edgeClicked().source);

            items << arcA
                  << arcB
                  << "Both";

            ok = false;

            QString selectedArc = QInputDialog::getItem(
                this, tr("Select edge"),
                tr("This is a reciprocated edge. "
                   "Select direction to remove:"),
                items, 0, false, &ok);

            if (selectedArc == arcA)
            {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if (selectedArc == arcB)
            {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else
            { // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                removeOpposite = true;
            }
        }
        else
        {
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }
    }

    activeGraph->edgeRemove(sourceNode, targetNode, removeOpposite);

    qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() -"
             << "View items now:" << graphicsWidget->items().size()
             << "Scene items now:" << graphicsWidget->scene()->items().size();
}

/**
 * @brief Changes the label of an edge.
 */
void MainWindow::slotEditEdgeLabel()
{
    qCDebug(lcMainWindow) << "MW::slotEditEdgeLabel()";
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    bool ok = false;

    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target)
    { // no edge clicked. Ask user to define an edge.
        sourceNode = QInputDialog::getInt(this,
                                          "Change edge label",
                                          tr("Select edge source node:  (" +
                                             QString::number(min).toLatin1() +
                                             "..." + QString::number(max).toLatin1() +
                                             "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge label operation cancelled.");
            return;
        }
        targetNode = QInputDialog::getInt(this,
                                          "Change edge label...",
                                          tr("Select edge target node:  (" +
                                             QString::number(min).toLatin1() + "..." +
                                             QString::number(max).toLatin1() + "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge label operation cancelled.");
            return;
        }

        if (!activeGraph->edgeExists(sourceNode, targetNode))
        {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));

            return;
        }
    }
    else
    { // edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }

    QString label = QInputDialog::getText(this, tr("Change edge label"),
                                          tr("Enter label: "));

    if (!label.isEmpty())
    {
        qCDebug(lcMainWindow) << "MW::slotEditEdgeLabel() - " << sourceNode << "->"
                 << targetNode << " new label " << label;
        activeGraph->edgeLabelSet(sourceNode, targetNode, label);
        slotOptionsEdgeLabelsVisibility(true);
        statusMessage(tr("Changed edge label. "));
    }
    else
    {
        statusMessage(tr("Change edge label aborted. "));
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
void MainWindow::slotEditEdgeColorAll(QColor color, const int threshold)
{
    qCDebug(lcMainWindow) << "Changing the color of all matching edges to color: " << color.name() << " threshold " << threshold;
    if (!color.isValid())
    {
        QString text;
        if (threshold < RAND_MAX)
        {
            text = "Change the color of edges weighted < " + QString::number(threshold);
        }
        else
            text = "Change the color of all edges";
        color = QColorDialog::getColor(appSettings["initEdgeColor"], this,
                                       text);
    }
    if (color.isValid())
    {
        qCDebug(lcMainWindow) << "new edge color: " << color.name() << " threshold " << threshold;
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (threshold < 0)
        {
            appSettings["initEdgeColorNegative"] = color.name();
        }
        else if (threshold == 0)
        {
            appSettings["initEdgeColorZero"] = color.name();
        }
        else
        {
            appSettings["initEdgeColor"] = color.name();
        }
        activeGraph->edgeColorAllSet(color.name(), threshold);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Changed edges color. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("edges color change aborted. "));
    }
}

/**
 * @brief Changes the color of the clicked edge.
 * If no edge is clicked, then it asks the user to specify one.
 */
void MainWindow::slotEditEdgeColor()
{
    qCDebug(lcMainWindow) << "MW::slotEditEdgeColor()";
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    bool ok = false;

    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target)
    { // no edge clicked. Ask user to define an edge.
        sourceNode = QInputDialog::getInt(this,
                                          "Change edge color",
                                          tr("Select edge source node:  (" +
                                             QString::number(min).toLatin1() +
                                             "..." + QString::number(max).toLatin1() +
                                             "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge color operation cancelled.");
            return;
        }
        targetNode = QInputDialog::getInt(this,
                                          "Change edge color...",
                                          tr("Select edge target node:  (" +
                                             QString::number(min).toLatin1() + "..." +
                                             QString::number(max).toLatin1() + "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge color operation cancelled.");
            return;
        }

        if (!activeGraph->edgeExists(sourceNode, targetNode))
        {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));

            return;
        }
    }
    else
    { // edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }
    QString curColor = activeGraph->edgeColor(sourceNode, targetNode);
    if (!QColor(curColor).isValid())
    {
        curColor = appSettings["initEdgeColor"];
    }
    QColor color = QColorDialog::getColor(
        curColor, this, tr("Select new color...."));

    if (color.isValid())
    {
        QString newColor = color.name();
        qCDebug(lcMainWindow) << "MW::slotEditEdgeColor() - " << sourceNode << "->"
                 << targetNode << " newColor "
                 << newColor;
        activeGraph->edgeColorSet(sourceNode, targetNode, newColor);
        statusMessage(tr("Edge color changed."));
    }
    else
    {
        statusMessage(tr("Change edge color aborted. "));
    }
}

/**
 * @brief Changes the weight of the clicked edge.
 * If no edge is clicked, asks the user to specify an Edge.
 */
void MainWindow::slotEditEdgeWeight()
{
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    qCDebug(lcMainWindow, "MW::slotEditEdgeWeight()");
    int sourceNode = -1, targetNode = -1;
    qreal newWeight = 1.0;
    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();
    bool changeBothEdges = false;
    bool ok = false;

    // Check if an edge has been clicked/selected.
    if (activeGraph->edgeClicked().source == 0 || activeGraph->edgeClicked().target == 0)
    {
        // No edge clicked/selected. Show dialog to select the edge by source/target nodes.
        sourceNode = QInputDialog::getInt(
            this,
            "Edge weight",
            tr("Select edge source node:  (" +
               QString::number(min).toLatin1() + "..." +
               QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge weight operation cancelled.");
            return;
        }

        targetNode = QInputDialog::getInt(
            this,
            "Edge weight",
            tr("Select edge target node:  (" +
               QString::number(min).toLatin1() + "..." +
               QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge weight operation cancelled.");
            return;
        }

        qCDebug(lcMainWindow, "source %i target %i", sourceNode, targetNode);
    }
    else
    {
        // An edge is clicked/selected.

        qCDebug(lcMainWindow) << "MW: slotEditEdgeWeight() - an Edge has already been clicked";

        // Check if clicked edge is reciprocated
        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
        {
            // Clicked edge is reciprocated.
            // We need the user to let us know if she wants to change a single edge or both
            QStringList items;
            QString arcA = QString::number(activeGraph->edgeClicked().source) + " -->" + QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number(activeGraph->edgeClicked().target) + " -->" + QString::number(activeGraph->edgeClicked().source);
            items << arcA
                  << arcB
                  << "Both";
            ok = false;
            QString selectedArc = QInputDialog::getItem(this, tr("Select edge"),
                                                        tr("This is a reciprocated edge. "
                                                           "Select direction:"),
                                                        items, 0, false, &ok);
            if (selectedArc == arcA)
            {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if (selectedArc == arcB)
            {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else
            { // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                changeBothEdges = true;
            }
        }
        else
        {
            // Clicked edge is not reciprocated. We are good to go.
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }

        qCDebug(lcMainWindow) << "MW: slotEditEdgeWeight() from "
                 << sourceNode << " to " << targetNode;
    }

    qreal oldWeight = 0;

    QString dialogTitle = "Edge " + QString::number(sourceNode) + "->" + QString::number(targetNode);

    bool undirected = activeGraph->isUndirected();

    // Get the new edge weight -- only if the edge exists.
    if ((oldWeight = activeGraph->edgeWeight(sourceNode, targetNode)) != 0)
    {

        // Fix the dialog title.
        if (changeBothEdges || undirected)
        {
            dialogTitle = "Edge " + QString::number(sourceNode) + "<->" + QString::number(targetNode);
        }

        // Prompt the user for the new edge weight
        newWeight = (qreal)QInputDialog::getDouble(
            this,
            dialogTitle,
            tr("New edge weight: "),
            oldWeight, -RAND_MAX, RAND_MAX, 2, &ok);

        if (ok)
        {
            activeGraph->edgeWeightSet(sourceNode, targetNode, newWeight,
                                       undirected || changeBothEdges);
        }
        else
        {
            statusMessage(QString(tr("Change edge weight cancelled.")));
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
void MainWindow::slotEditEdgeSymmetrizeAll()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "Request to symmetrize all edges...";
    activeGraph->setSymmetric();
    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("All ties have been symmetrized."),
                          tr("All ties between nodes have been symmetrized."),
                          tr("The network is now symmetric. "));
}

/**
 * @brief Adds a new cocitation symmetric relation to the network
 *
 * In the new relation, there are ties only between pairs of nodes who were cocited by others.
 */
void MainWindow::slotEditEdgeSymmetrizeCocitation()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "Request to add a new symmetric relation using cocited nodes...";
    runGraphOperationAsync(
        [this]() { activeGraph->relationAddCocitation(); },
        tr("Computing cocitation relation..."),
        [this]() {
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("New cocitation relation added. Ready"),
                                  tr("New cocitation relation has been added to the network."),
                                  tr("In the new relation, there are ties only between pairs of nodes who were cocited by others."));
        });
}

/**
 * @brief Opens up the edge dichotomization dialog
 */
void MainWindow::slotEditEdgeDichotomizationDialog()
{

    // @TODO: Check if the network is already binary and abord?

    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "MW: slotEditEdgeDichotomizationDialog() - "
                "spawning edgeDichotomizationDialog";

    m_edgeDichotomizationDialog = new DialogEdgeDichotomization(this);

    connect(m_edgeDichotomizationDialog, &DialogEdgeDichotomization::userChoices,
            this, &MainWindow::slotEditEdgeDichotomization);

    m_edgeDichotomizationDialog->exec();
}

/**
 * @brief Calls Graph::graphDichotomization() to create a new binary relation
 * in a valued network using edge dichotomization according to threshold value.
 */
void MainWindow::slotEditEdgeDichotomization(const qreal threshold)
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow, "MW: slotEditEdgeDichotomization() calling graphDichotomization()");
    activeGraph->graphDichotomization(threshold);
    slotHelpMessageToUser(USER_MSG_INFO, tr("New binary relation added."),
                          tr("New dichotomized relation created"),
                          tr("A new relation called \"%1\" has been added to the network, "
                             "using the given dichotomization threshold. ")
                              .arg("Binary"));

    statusMessage(tr("Edge dichotomization finished. "));
}

/**
 * @brief MainWindow::slotEditEdgeSymmetrizeStrongTies
 */
void MainWindow::slotEditEdgeSymmetrizeStrongTies()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "MW::slotEditEdgeSymmetrizeStrongTies() - calling addRelationSymmetricStrongTies()";
    int oldRelationsCounter = activeGraph->relations();
    // callIt stays false (silent return, no info dialog) if the multi-relation dialog below is
    // dismissed without picking 1 or 2 - matches the original switch's lack of a default case.
    bool callIt = true;
    bool allRelations = false;
    if (oldRelationsCounter > 0)
    {
        switch (
            slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM, tr("Select"),
                                  tr("Symmetrize social network by examining strong ties"),
                                  tr("This network has multiple relations. "
                                     "Symmetrize by examining reciprocated ties across all relations or just the current relation?"),
                                  QMessageBox::NoButton, QMessageBox::NoButton,
                                  tr("all relations"), tr("current relation")))
        {
        case 1:
            allRelations = true;
            break;
        case 2:
            allRelations = false;
            break;
        default:
            callIt = false;
            break;
        }
    }

    if (!callIt)
        return;

    runGraphOperationAsync(
        [this, allRelations]() { activeGraph->addRelationSymmetricStrongTies(allRelations); },
        tr("Symmetrizing strong ties..."),
        [this]() {
            slotHelpMessageToUser(USER_MSG_INFO, tr("New symmetric relation created from strong ties"),
                                  tr("New relation created from strong ties"),
                                  tr("A new relation \"%1\" has been added to the network. "
                                     "by counting reciprocated ties only. "
                                     "This relation is binary and symmetric. ")
                                      .arg("Strong Ties"));
        });
}

/**
 * @brief Transforms all directed arcs to undirected edges.
 * The result is a undirected and symmetric network
 */
void MainWindow::slotEditEdgeUndirectedAll(const bool &toggle)
{
    qCDebug(lcMainWindow) << "MW: slotEditEdgeUndirectedAll() - calling Graph::graphUndirectedSet()";
    if (toggle)
    {
        activeGraph->setUndirected(true);
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready"));
        }
        else
        {
            statusMessage(tr("Undirected data mode. "
                             "Any edge you add will be undirected. Ready"));
        }
    }
    else
    {
        activeGraph->setDirected(true);
        optionsEdgeArrowsAct->trigger();
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Directed data mode. "
                             "All existing undirected edges transformed to "
                             "directed. Ready"));
        }
        else
        {
            statusMessage(tr("Directed data mode. "
                             "Any new edge you add will be directed. Ready"));
        }
    }
}

/**
 * @brief Toggles between directed (mode=0) and undirected edges (mode=1)
 *
 * @param mode
 */
void MainWindow::slotEditEdgeMode(const int &mode)
{
    if (mode == 1)
    {
        qCDebug(lcMainWindow) << "Changing edge mode to undirected. Informing Graph...";
        activeGraph->setUndirected(true);
        qCDebug(lcMainWindow) << "Setting optionsEdgeArrowsAct to false";
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready"));
        }
        else
        {
            statusMessage(tr("Undirected data mode. "
                             "Any edge you add will be undirected. Ready"));
        }
    }
    else
    {
        qCDebug(lcMainWindow) << "Changing edge mode to directed. Informing Graph...";
        activeGraph->setDirected(true);
        qCDebug(lcMainWindow) << "Triggering optionsEdgeArrowsAct checkbox";
        optionsEdgeArrowsAct->trigger();
        qCDebug(lcMainWindow) << "Setting optionsEdgeArrowsAct to true";
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Directed data mode. "
                             "All existing undirected edges transformed to "
                             "directed."));
        }
        else
        {
            statusMessage(tr("Directed data mode. "
                             "Any new edge you add will be directed."));
        }
    }
}

/**
 * @brief Updates the edge-mode combo and arrows action to reflect the directed
 *        state of the newly selected relation, without calling setDirected/setUndirected.
 *
 * Connected to Graph::signalGraphDirectedChanged, which is emitted by relationSet()
 * whenever the user switches to a different relation.
 *
 * @param directed true if the new relation is directed
 */
void MainWindow::slotEditGraphDirectedChanged(const bool &directed)
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditGraphDirectedChanged - directed:" << directed;
    // Block the combo's signal so we don't re-enter slotEditEdgeMode and
    // accidentally call setDirected/setUndirected (which mutate edges).
    toolBoxEditEdgeModeSelect->blockSignals(true);
    toolBoxEditEdgeModeSelect->setCurrentIndex(directed ? 0 : 1);
    toolBoxEditEdgeModeSelect->blockSignals(false);
    optionsEdgeArrowsAct->setChecked(directed);
}

/**
 * @brief Shows a dialog where the user can specify criteria to filter nodes
 *
 */
void MainWindow::slotFilterNodesDialogByCentrality()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    // NOTE: Filter state is snapshot-based. vertexFilterRestoreAll() undoes
    // the last filter; filterNodesRestoreAllAct is enabled after apply.
    //
    //  TODO Deferred (future issues):
    //  - Auto-compute selected index if not yet computed (#216).
    //  - Persist last-used index and threshold (#216).
    //  - Add "Compute missing indices" shortcut in dialog (#216).

    QVector<bool> computedMask;

    const int count = prominenceIndexList.size();
    computedMask.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const IndexType t = static_cast<IndexType>(i + 1);
        computedMask.append(activeGraph->isCentralityIndexComputed(t));
    }

    DialogFilterNodesByCentrality dlg(prominenceIndexList,
                                      computedMask,
                                      this);

    // Not connected straight to activeGraph anymore: that resolved to a queued cross-thread
    // connection (dialog on GUI thread, Graph on graphThread) with no busy-guard and no progress
    // feedback - the same unprotected-async-dispatch shape WS15 P2 closed for computations,
    // just for filtering. Routed through a local lambda + runGraphOperationAsync instead.
    connect(&dlg,
            &DialogFilterNodesByCentrality::userChoices,
            this, [this](const float threshold, const bool overThreshold, const IndexType centralityIndex)
            {
        runGraphOperationAsync(
            [this, threshold, overThreshold, centralityIndex]() {
                activeGraph->vertexFilterByCentrality(threshold, overThreshold, centralityIndex);
            },
            tr("Filtering by centrality..."),
            [this]() {
                filterNodesRestoreAllAct->setEnabled(true);
                m_filterChips.append({tr("Nodes: centrality filter"), FilterCondition::Scope::Nodes});
                m_filterBar->addChip(tr("Nodes: centrality filter"), FilterCondition::Scope::Nodes);
            }); });

    dlg.exec();
}

/**
 * @brief Focuses the view on the currently selected nodes and edges between them.
 *
 * Hides all nodes not in the current selection. Only edges whose both
 * endpoints are selected remain visible. Implements #210.
 */
void MainWindow::slotFilterNodesBySelection()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const QList<int> selection = activeGraph->getSelectedVertices();
    if (selection.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No nodes selected"),
                              tr("Please select at least one node first."));
        return;
    }
    runGraphOperationAsync(
        [this, selection]() { activeGraph->vertexFilterBySelection(selection); },
        tr("Filtering by selection..."),
        [this]() {
            filterNodesRestoreAllAct->setEnabled(true);
            m_filterChips.append({tr("Nodes: selection"), FilterCondition::Scope::Nodes});
            m_filterBar->addChip(tr("Nodes: selection"), FilterCondition::Scope::Nodes);
        });
}

/**
 * @brief Triggers ego network focus mode on the currently selected node.
 */
void MainWindow::slotFilterNodesByEgoNetwork()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int v1 = activeGraph->vertexClicked();
    if (v1 == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No node selected"),
                              tr("Please click on a node first."));
        return;
    }
    runGraphOperationAsync(
        [this, v1]() { activeGraph->vertexFilterByEgoNetwork(v1); },
        tr("Filtering ego network..."),
        [this]() {
            filterNodesRestoreAllAct->setEnabled(true);
            m_filterChips.append({tr("Nodes: ego network"), FilterCondition::Scope::Nodes});
            m_filterBar->addChip(tr("Nodes: ego network"), FilterCondition::Scope::Nodes);
        });
}

/**
 * @brief Opens the Filter By Attribute dialog and applies the chosen condition.
 *
 * Handles both node and edge attribute filtering depending on the scope
 * selected in the dialog.
 */
void MainWindow::slotFilterNodesByAttribute()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QStringList nodeKeys = activeGraph->graphHasVertexCustomAttributes();
    const QStringList edgeKeys = activeGraph->graphHasEdgeCustomAttributes();

    if (nodeKeys.isEmpty() && edgeKeys.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No custom attributes"),
                              tr("This network has no custom node or edge attributes. "
                                 "Add attributes via the Data Table (Ctrl+D) using "
                                 "\"Add attribute…\", or via the Node / Edge Properties dialogs."));
        return;
    }

    DialogFilterByAttribute dlg(nodeKeys, edgeKeys, this);

    connect(&dlg, &DialogFilterByAttribute::userChoices,
            this, [this](const FilterCondition &cond)
            {
        runGraphOperationAsync(
            [this, cond]() {
                if (cond.scope == FilterCondition::Scope::Edges) {
                    activeGraph->edgeFilterByAttribute(cond);
                } else if (cond.scope == FilterCondition::Scope::Both) {
                    activeGraph->vertexFilterByAttribute(cond);
                    activeGraph->edgeFilterByAttribute(cond);
                } else {
                    activeGraph->vertexFilterByAttribute(cond);
                }
            },
            tr("Filtering by attribute..."),
            [this, cond]() {
                if (cond.scope == FilterCondition::Scope::Edges) {
                    m_filterChips.append({cond.label(), FilterCondition::Scope::Edges});
                    m_filterBar->addChip(cond.label(), FilterCondition::Scope::Edges);
                } else if (cond.scope == FilterCondition::Scope::Both) {
                    // Split into two chips so each scope can be closed independently.
                    FilterCondition nc = cond; nc.scope = FilterCondition::Scope::Nodes;
                    FilterCondition ec = cond; ec.scope = FilterCondition::Scope::Edges;
                    m_filterChips.append({nc.label(), FilterCondition::Scope::Nodes});
                    m_filterChips.append({ec.label(), FilterCondition::Scope::Edges});
                    m_filterBar->addChip(nc.label(), FilterCondition::Scope::Nodes);
                    m_filterBar->addChip(ec.label(), FilterCondition::Scope::Edges);
                } else {
                    m_filterChips.append({cond.label(), FilterCondition::Scope::Nodes});
                    m_filterBar->addChip(cond.label(), FilterCondition::Scope::Nodes);
                }
                if (cond.scope != FilterCondition::Scope::Edges)
                    filterNodesRestoreAllAct->setEnabled(true);
                if (cond.scope == FilterCondition::Scope::Edges ||
                    cond.scope == FilterCondition::Scope::Both)
                    editFilterEdgesRestoreAllAct->setEnabled(true);
            }); });

    dlg.exec();

    if (dlg.wasQueryBuilderRequested())
        slotFilterByQueryBuilder();
}

/**
 * @brief Opens the Query Builder dialog for multi-condition AND filtering.
 *
 * Scope (Nodes / Edges) is selected inside the dialog.
 * Applies one compound filter that counts as a single chip and one snapshot.
 */
void MainWindow::slotFilterByQueryBuilder()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QStringList nodeKeys = activeGraph->graphHasVertexCustomAttributes();
    const QStringList edgeKeys = activeGraph->graphHasEdgeCustomAttributes();

    if (nodeKeys.isEmpty() && edgeKeys.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No custom attributes"),
                              tr("This network has no custom node or edge attributes. "
                                 "Add attributes via the Data Table (Ctrl+D)."));
        return;
    }

    DialogQueryBuilder *dlg = new DialogQueryBuilder(nodeKeys, edgeKeys, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &DialogQueryBuilder::userChoices,
            this, [this](const GraphQuery &query)
            {
        if (query.conditions.isEmpty()) return;

        const FilterCondition::Scope scope = query.conditions.first().scope;
        const int n = query.conditions.size();

        runGraphOperationAsync(
            [this, query, scope]() {
                if (scope == FilterCondition::Scope::Edges) {
                    activeGraph->edgeFilterByQuery(query);
                } else {
                    activeGraph->vertexFilterByQuery(query);
                }
            },
            tr("Applying query filter..."),
            [this, scope, n]() {
                if (scope == FilterCondition::Scope::Edges) {
                    const QString label = tr("Edges: query (%1 condition(s))").arg(n);
                    m_filterChips.append({label, FilterCondition::Scope::Edges});
                    m_filterBar->addChip(label, FilterCondition::Scope::Edges);
                    editFilterEdgesRestoreAllAct->setEnabled(true);
                } else {
                    const QString label = tr("Nodes: query (%1 condition(s))").arg(n);
                    m_filterChips.append({label, FilterCondition::Scope::Nodes});
                    m_filterBar->addChip(label, FilterCondition::Scope::Nodes);
                    filterNodesRestoreAllAct->setEnabled(true);
                }
            }); });

    dlg->exec();
}

/**
 * @brief Restores all nodes hidden by the last filter.
 */
void MainWindow::slotFilterNodesRestoreAll()
{
    // Find the last non-edge chip and remove it via the unified stack mechanism.
    int lastNodeIdx = -1;
    for (int i = m_filterChips.size() - 1; i >= 0; --i)
    {
        if (m_filterChips[i].second != FilterCondition::Scope::Edges)
        {
            lastNodeIdx = i;
            break;
        }
    }
    if (lastNodeIdx < 0)
        return;

    runGraphOperationAsync(
        [this, lastNodeIdx]() { activeGraph->vertexFilterRemoveAt(lastNodeIdx); },
        tr("Restoring filtered nodes..."),
        [this, lastNodeIdx]() {
            m_filterChips.removeAt(lastNodeIdx);
            m_filterBar->clearAllChips();
            for (const auto &chip : std::as_const(m_filterChips))
                m_filterBar->addChip(chip.first, chip.second);

            bool hasNodeFilters = false;
            for (const auto &chip : std::as_const(m_filterChips))
                if (chip.second != FilterCondition::Scope::Edges)
                {
                    hasNodeFilters = true;
                    break;
                }
            filterNodesRestoreAllAct->setEnabled(hasNodeFilters);
        });
}

/**
 * @brief Toggles the status of all isolated vertices
 *
 * @param checked
 */
void MainWindow::slotEditFilterNodesIsolates(bool checked)
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const bool toggleTo = !editFilterNodesIsolatesAct->isChecked();
    runGraphOperationAsync(
        [this, toggleTo]() { activeGraph->vertexIsolatedAllToggle(toggleTo); },
        tr("Toggling isolated nodes..."),
        [this, checked]() {
            if (checked)
            {
                statusMessage(tr("Isolated nodes disabled."));
            }
            else
            {
                statusMessage(tr("Isolated nodes enabled."));
            }
        });
}

/**
 * @brief Shows a dialog where the user can specify how to filter edges by their weight
 *
 * All edges weighted more (or less) than the specified weight will be disabled.
 */
void MainWindow::slotEditFilterEdgesByWeightDialog()
{

    // Note: We do not check if there are active edges, because the user might have disabled all edges previously.

    // Create a new edge filtering dialog
    m_DialogEdgeFilterByWeight = new DialogFilterEdgesByWeight(this);

    // Was two separate connections to userChoices: one straight to activeGraph (a queued
    // cross-thread dispatch with no busy-guard/progress feedback - the same shape WS15 P2 closed
    // for computations), and a second, entirely independent one doing the GUI chip bookkeeping
    // immediately, with no actual dependency on the filter having completed. Merged into one
    // handler via runGraphOperationAsync, so the chip only appears once the filter is done.
    connect(m_DialogEdgeFilterByWeight, &DialogFilterEdgesByWeight::userChoices,
            this, [this](const qreal threshold, const bool overThreshold)
            {
        runGraphOperationAsync(
            [this, threshold, overThreshold]() {
                activeGraph->edgeFilterByWeight(threshold, overThreshold);
            },
            tr("Filtering edges by weight..."),
            [this]() {
                editFilterEdgesRestoreAllAct->setEnabled(true);
                m_filterChips.append({tr("Edges: weight filter"), FilterCondition::Scope::Edges});
                m_filterBar->addChip(tr("Edges: weight filter"), FilterCondition::Scope::Edges);
            }); });

    // Show the dialog
    m_DialogEdgeFilterByWeight->exec();
}

/**
 * @brief Restores all edges hidden by the weight filter.
 */
void MainWindow::slotEditFilterEdgesReset()
{
    // Collect edge-scope chip indices first (highest index first, so each
    // vertexFilterRemoveAt below - and the matching m_filterChips removal in
    // onComplete - operates on the correct stack position without the two
    // lists needing to be mutated in lockstep inside the same loop anymore.
    QList<int> edgeChipIndices;
    for (int i = m_filterChips.size() - 1; i >= 0; --i)
    {
        if (m_filterChips[i].second == FilterCondition::Scope::Edges)
            edgeChipIndices.append(i);
    }
    runGraphOperationAsync(
        [this, edgeChipIndices]() {
            for (int i : edgeChipIndices)
                activeGraph->vertexFilterRemoveAt(i);
        },
        tr("Resetting edge filters..."),
        [this, edgeChipIndices]() {
            for (int i : edgeChipIndices)
                m_filterChips.removeAt(i);
            m_filterBar->clearAllChips();
            for (const auto &chip : std::as_const(m_filterChips))
                m_filterBar->addChip(chip.first, chip.second);
            editFilterEdgesRestoreAllAct->setEnabled(false);
        });
}

/**
 * @brief Toggles the status of all unilateral edges
 *
 * @param checked
 */
void MainWindow::slotEditFilterEdgesUnilateral(bool checked)
{

    if (!activeEdges() && editFilterEdgesUnilateralAct->isChecked())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    const bool toggleTo = !editFilterEdgesUnilateralAct->isChecked();
    runGraphOperationAsync(
        [this, toggleTo]() { activeGraph->edgeFilterUnilateral(toggleTo); },
        tr("Filtering unilateral edges..."),
        [this, checked]() {
            if (checked)
            {
                statusMessage(tr("Unilateral (weak) edges disabled."));
            }
            else
            {
                statusMessage(tr("Unilateral (weak) edges enabled."));
            }
        });
}

/**
*	Transforms all nodes to edges
    TODO slotEditTransformNodes2Edges
*/
/**
 * @brief Extracts the currently visible nodes and their inter-edges into an
 *        independent Graph object and saves it to a user-chosen file.
 */
void MainWindow::slotEditSubgraphExtract()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;
    const QString defaultName = tr("Subgraph of %1").arg(activeGraph->getName());
    const QString subgraphName = QInputDialog::getText(
        this,
        tr("Name the subgraph"),
        tr("Subgraph name:"),
        QLineEdit::Normal,
        defaultName,
        &ok);
    if (!ok || subgraphName.trimmed().isEmpty())
        return;

    // sub is written on graphThread (inside the dispatched lambda) and read back here in
    // onComplete (GUI thread) - a std::shared_ptr box makes that value visible across the
    // thread hop, same pattern the writeCentralityX() slots use for their success flag.
    const QString name = subgraphName.trimmed();
    auto sub = std::make_shared<Graph *>(nullptr);
    runGraphOperationAsync(
        [this, name, sub]() { *sub = activeGraph->subgraphExtract(name); },
        tr("Extracting subgraph..."),
        [this, name, sub]() {
            if (!*sub)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
                return;
            }
            saveSubgraphToFile(*sub, name);
        });
}

/**
 * @brief Extracts currently selected nodes and their inter-edges into an
 *        independent Graph object and saves it to a user-chosen file.
 */
void MainWindow::slotEditSubgraphExtractFromSelection()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;
    const QString defaultName = tr("Subgraph of %1").arg(activeGraph->getName());
    const QString subgraphName = QInputDialog::getText(
        this,
        tr("Name the subgraph"),
        tr("Subgraph name:"),
        QLineEdit::Normal,
        defaultName,
        &ok);
    if (!ok || subgraphName.trimmed().isEmpty())
        return;

    const QString name = subgraphName.trimmed();
    auto sub = std::make_shared<Graph *>(nullptr);
    runGraphOperationAsync(
        [this, name, sub]() { *sub = activeGraph->subgraphExtractFromSelection(name); },
        tr("Extracting subgraph..."),
        [this, name, sub]() {
            if (!*sub)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
                return;
            }
            saveSubgraphToFile(*sub, name);
        });
}

/**
 * @brief Shows a format-selection save dialog for @p sub, writes the file, then
 *        deletes the graph object.
 *
 * Offered formats: GraphML (full fidelity), Pajek, Adjacency.
 * Warns the user when the chosen format cannot preserve custom attributes or
 * when only the active relation will be written.
 *
 * Takes ownership of @p sub — the pointer is invalid after this call.
 */
void MainWindow::saveSubgraphToFile(Graph *sub, const QString &subgraphName)
{
    // Build filesystem-safe default base name.
    QString sanitized = subgraphName;
    sanitized.replace(QRegularExpression("[/\\\\:*?\"<>|\\s]+"), "_");

    // Default to GraphML (preserves all attributes and relations).
    const QString defaultPath =
        QFileInfo(getLastPath()).dir().filePath(sanitized + ".graphml");

    const QString filterGraphML = tr("GraphML (*.graphml *.xml)");
    const QString filterPajek = tr("Pajek (*.net *.paj)");
    const QString filterAdjacency = tr("Adjacency (*.csv *.sm *.adj)");
    const QString filterDot = tr("GraphViz DOT (*.dot)");
    const QString filterDL = tr("UCINET DL (*.dl *.dat)");
    const QString filterEdgeListW = tr("Weighted Edge List (*.wlst)");
    const QString filterEdgeListS = tr("Simple Edge List (*.lst)");

    const QString allFormats =
        filterGraphML + ";;" +
        filterPajek + ";;" +
        filterAdjacency + ";;" +
        filterDot + ";;" +
        filterDL + ";;" +
        filterEdgeListW + ";;" +
        filterEdgeListS + ";;All (*)";

    QString selectedFilter;
    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Save Subgraph As…"),
        defaultPath,
        allFormats,
        &selectedFilter);

    if (fn.isEmpty())
    {
        statusMessage(tr("Saving aborted."));
        delete sub;
        return;
    }

    // Map selected filter → FileType and canonical extension.
    int fileType = FileType::GRAPHML;
    QString defaultExt = ".graphml";

    if (selectedFilter.startsWith("Pajek"))
    {
        fileType = FileType::PAJEK;
        defaultExt = ".net";
    }
    else if (selectedFilter.startsWith("Adjacency"))
    {
        fileType = FileType::ADJACENCY;
        defaultExt = ".csv";
    }
    else if (selectedFilter.startsWith("GraphViz"))
    {
        fileType = FileType::GRAPHVIZ;
        defaultExt = ".dot";
    }
    else if (selectedFilter.startsWith("UCINET"))
    {
        fileType = FileType::UCINET;
        defaultExt = ".dl";
    }
    else if (selectedFilter.startsWith("Weighted Edge"))
    {
        fileType = FileType::EDGELIST_WEIGHTED;
        defaultExt = ".wlst";
    }
    else if (selectedFilter.startsWith("Simple Edge"))
    {
        fileType = FileType::EDGELIST_SIMPLE;
        defaultExt = ".lst";
    }
    // else: GraphML or "All (*)" → keep GRAPHML

    if (QFileInfo(fn).suffix().isEmpty())
        fn.append(defaultExt);

    setLastPath(fn);

    // --- Fidelity warnings -------------------------------------------------
    const bool hasCustomAttrs =
        !sub->graphHasVertexCustomAttributes().isEmpty() ||
        !sub->graphHasEdgeCustomAttributes().isEmpty();
    const bool multiRelation = sub->relations() > 1;

    // GraphML and DOT both preserve custom attributes; all other formats lose them.
    const bool loseCustomAttrs =
        fileType != FileType::GRAPHML &&
        fileType != FileType::GRAPHVIZ &&
        hasCustomAttrs;

    if (loseCustomAttrs)
    {
        const int answer = slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Custom attributes will not be saved"),
            tr("Format limitation: custom node/edge attributes"),
            tr("The chosen format does not support custom node or edge attributes. "
               "Those attributes will be lost in the exported file.\n\n"
               "Use GraphML to preserve all attributes.\n\n"
               "Continue with the chosen format?"));
        if (answer != QMessageBox::Yes)
        {
            statusMessage(tr("Saving aborted."));
            delete sub;
            return;
        }
    }

    // Formats that export only the active relation (DL and Pajek handle multi-relation natively).
    const bool activeRelOnly =
        fileType == FileType::ADJACENCY ||
        fileType == FileType::GRAPHVIZ ||
        fileType == FileType::EDGELIST_WEIGHTED ||
        fileType == FileType::EDGELIST_SIMPLE;

    if (activeRelOnly && multiRelation)
    {
        QString fmtName;
        switch (fileType)
        {
        case FileType::ADJACENCY:
            fmtName = tr("Adjacency");
            break;
        case FileType::GRAPHVIZ:
            fmtName = tr("GraphViz DOT");
            break;
        case FileType::EDGELIST_WEIGHTED:
            fmtName = tr("Weighted Edge List");
            break;
        case FileType::EDGELIST_SIMPLE:
            fmtName = tr("Simple Edge List");
            break;
        default:
            break;
        }
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Only active relation will be exported"),
            tr("Multi-relation graph — %1 format").arg(fmtName),
            tr("The %1 format supports a single relation. "
               "Only the currently active relation \"%2\" will be written.")
                .arg(fmtName)
                .arg(sub->relationCurrentName()));
    }

    // --- Format-specific options -------------------------------------------
    bool saveEdgeWeights = true;
    if (fileType == FileType::ADJACENCY && sub->isWeighted())
    {
        const int answer = slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Save edge weights?"),
            tr("Weighted graph"),
            tr("This graph has weighted edges. "
               "Include edge weights in the adjacency file?\n\n"
               "Select Yes to save weights, No to write 0/1 values only."));
        saveEdgeWeights = (answer == QMessageBox::Yes);
    }

    // --- Write -------------------------------------------------------------
    sub->saveToFile(fn, fileType, saveEdgeWeights);

    const int n = sub->vertices();
    const int e = sub->edgesEnabled();
    delete sub;

    statusMessage(tr("Subgraph saved: %1 nodes, %2 edges → %3")
                      .arg(n)
                      .arg(e)
                      .arg(QFileInfo(fn).fileName()));
}

void MainWindow::slotEditTransformNodes2Edges()
{
}

/**
    TODO slotLayoutColorationStrongStructural
*/
void MainWindow::slotLayoutColorationStrongStructural()
{
}

/**
    TODO slotLayoutColorationRegular
*/
void MainWindow::slotLayoutColorationRegular()
{
}

/**
 * @brief Calls Graph::layoutRandom
 * to reposition all nodes on a random layout
 */
void MainWindow::slotLayoutRandom()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    graphicsWidget->clearGuides();

    runGraphOperationAsync(
        [this]() {
            activeGraph->layoutRandom();
        },
        tr("Placing nodes in random positions. Please wait..."),
        tr("Nodes in random positions."));
}

/**
 * @brief Calls Graph::layoutRadialRandom
 * to reposition all nodes on a radial layout randomly
 */
void MainWindow::slotLayoutRadialRandom()
{
    qCDebug(lcMainWindow) << "MainWindow::slotLayoutRadialRandom()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    slotLayoutGuides(false);

    runGraphOperationAsync(
        [this]() {
            activeGraph->layoutRadialRandom(true);
        },
        tr("Placing nodes in random concentric circles. Please wait..."),
        [this]() {
            slotLayoutGuides(true);
            statusMessage(tr("Nodes in random concentric circles."));
        });
}

/**
 * @brief Calls Graph::layoutEgoRadial.
 * Resolves the ego vertex from the current selection (exactly one selected)
 * or from the last-clicked vertex, whichever is available.
 */
void MainWindow::slotLayoutEgoRadial()
{
    qCDebug(lcMainWindow) << "MainWindow::slotLayoutEgoRadial()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int egoVertex = 0;
    if (activeGraph->getSelectedVerticesCount() == 1)
        egoVertex = activeGraph->getSelectedVertices().first();
    else if (activeGraph->vertexClicked() != 0)
        egoVertex = activeGraph->vertexClicked();
    else
    {
        statusMessage(tr("Please select or click one node first."));
        return;
    }
    runGraphOperationAsync(
        [this, egoVertex]() {
            activeGraph->layoutEgoRadial(egoVertex);
        },
        tr("Computing ego radial layout. Please wait..."),
        [this, egoVertex]() {
            statusMessage(tr("Ego radial layout centered on vertex %1.").arg(egoVertex));
        });
}

/**
 * @brief Embed the Eades spring-gravitational model to the network.
 * Called from menu or toolbox checkbox.
 */
void MainWindow::slotLayoutSpringEmbedder()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutSpringEmbedder";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    // Eades (1984) designed this algorithm for small graphs (N < 30).
    // For larger graphs the system frequently gets stuck in local minima
    // and the O(N²×I) complexity makes it slow. Warn the user.
    if (activeNodes() > 30)
    {
        QMessageBox::warning(
            this,
            tr("Eades Spring Embedder — Size Warning"),
            tr("The Eades Spring Embedder was designed for graphs with fewer than 30 nodes.\n\n"
               "This network has %1 nodes. The layout may get stuck in local minima "
               "and the result may not be aesthetically pleasing.\n\n"
               "Consider using the Fruchterman-Reingold or Kamada-Kawai models instead, "
               "which handle larger graphs better.")
                .arg(activeNodes()));
    }
    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedSpringEmbedder(300); },
        tr("Computing Spring-Gravitational (Eades) layout. Please wait..."),
        tr("Spring-Gravitational (Eades) model embedded."));
}

/**
 * @brief Calls Graph::layoutForceDirectedFruchtermanReingold to embed
 * the Fruchterman-Reingold model of repelling-attracting forces to the network.
 * Called from menu or toolbox
 */
void MainWindow::slotLayoutFruchterman()
{
    qCDebug(lcMainWindow, "MW: slotLayoutFruchterman ()");
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedFruchtermanReingold(100); },
        tr("Computing Fruchterman & Reingold layout. Please wait..."),
        tr("Fruchterman & Reingold model embedded."));
}

/**
 * @brief Layouts the network according to the Kamada-Kawai FDP model
 */
void MainWindow::slotLayoutKamadaKawai()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutKamadaKawai ()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedKamadaKawai(400); },
        tr("Computing Kamada & Kawai layout. Please wait..."),
        tr("Kamada & Kawai model embedded."));
}

/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutRadialByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a radial layout by prominence index. Checking what index is requested...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
    QString menuItemText = menuitem->text();

    slotLayoutRadialByProminenceIndex(menuItemText);
}

/**
 * @brief Applies a radial layout on the social network, where each node is placed on concentric circles according to their index score.
 *
 *  More prominent nodes are closer to the centre of the screen.
 *
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutRadialByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a radial layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    slotLayoutGuides(true);

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(1); // Radial
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    toolBoxLayoutForceDirectedSelect->blockSignals(true);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0); // None — FD position overridden
    toolBoxLayoutForceDirectedSelect->blockSignals(false);

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 0,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 radial layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes in inner circles have higher %1 score. ").arg(prominenceIndexName));
}

/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutLevelByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a level layout by prominence index. Checking what index is requested...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
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
void MainWindow::slotLayoutLevelByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a level layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    slotLayoutGuides(true);

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(2); // On Levels
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    toolBoxLayoutForceDirectedSelect->blockSignals(true);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0); // None — FD position overridden
    toolBoxLayoutForceDirectedSelect->blockSignals(false);

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 1,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 level layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes in upper levels have higher %1 score. ").arg(prominenceIndexName));
}

/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeSizeByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a color layout by prominence index. Checking what index is requested...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
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
void MainWindow::slotLayoutNodeSizeByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a node size layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(3); // Node Size
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    // Node size coexists with FD positional layout — don't reset FD combobox

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 2,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 node size layout. Please wait...").arg(prominenceIndexName),
        tr("Bigger nodes have greater %1 score.").arg(prominenceIndexName));
}

/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeColorByProminenceIndex()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
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
void MainWindow::slotLayoutNodeColorByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a node color layout by prominence index: " << prominenceIndexName;
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(4); // Node Color
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    // Node color coexists with FD positional layout — don't reset FD combobox

    bool dropIsolates = false;

    if (indexType == 8 && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 3,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 node color layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes with warmer color have greater %1 score.").arg(prominenceIndexName));
}

/**
 * @brief Colors nodes by their weakly connected component.
 *
 * Runs a BFS to identify weakly connected components and assigns a distinct
 * color to each one. If the network is already fully connected (1 component),
 * reports that and leaves colors unchanged.
 */
void MainWindow::slotLayoutNodeColorByComponent()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutNodeColorByComponent()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int components = activeGraph->graphWeaklyConnectedComponents();

    if (components <= 1)
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Network is fully connected — only one component."),
            tr("Network is fully connected."),
            tr("All nodes belong to a single connected component. No coloring applied."));
        return;
    }

    // Visually distinct palette — cycles if there are more components than entries
    static const QStringList palette = {
        "#e74c3c", "#3498db", "#2ecc71", "#f39c12", "#9b59b6",
        "#1abc9c", "#e67e22", "#34495e", "#e91e63", "#00bcd4",
        "#8bc34a", "#ff5722", "#673ab7", "#009688", "#ffc107"};

    const QHash<int, int> &compMap = activeGraph->vertexComponentId();
    for (auto it = activeGraph->verticesBegin(); it != activeGraph->verticesEnd(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        const int compId = compMap.value((*it)->number(), 1);
        activeGraph->vertexColorSet((*it)->number(),
                                    palette.at((compId - 1) % palette.size()));
    }

    statusMessage(tr("Nodes colored by component: %1 components found.").arg(components));
}

/**
 * @brief Shows or hides (clears) layout guides
 *
 * @param toggle
 */
void MainWindow::slotLayoutGuides(const bool &toggle)
{
    qCDebug(lcMainWindow) << "MW:slotLayoutGuides()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    if (toggle)
    {
        layoutGuidesAct->setChecked(true);
        statusMessage(tr("Layout Guides are displayed"));
    }
    else
    {
        layoutGuidesAct->setChecked(false);
        graphicsWidget->clearGuides();
        statusMessage(tr("Layout Guides removed"));
    }
}

/**
 *	Returns the amount of enabled/active edges on the scene.
 */
int MainWindow::activeEdges()
{
    qCDebug(lcMainWindow) << "MW::activeEdges()";
    return activeGraph->edgesEnabled();
}

/**
 *	Returns the number of active nodes on the scene.
 */
int MainWindow::activeNodes()
{
    return activeGraph->vertices();
}

/**
 *	Displays the arc and dyad reciprocity of the network
 */
/**
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeReciprocity()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-reciprocity-" + dateTime + ext;

    askAboutEdgeWeights();

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeReciprocity(fn, considerWeights, reportFormat);
        },
        tr("Computing Reciprocity. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Reciprocity report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Displays a box informing the user about the symmetry or not of the adjacency matrix
 */

void MainWindow::slotAnalyzeSymmetryCheck()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric())
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Symmetric network."),
            tr("The adjacency matrix is symmetric."));
    }
    else
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Non symmetric network."),
            tr("The adjacency matrix is not symmetric."));
    }

    statusMessage(QString(tr("Ready")));
}

/**
 * @brief Writes the adjacency matrix inverse
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyInverse()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-inverse-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_ADJACENCY_INVERSE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Inverting adjacency matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Inverse matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the transpose adjacency matrix
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyTranspose()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-transpose-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_ADJACENCY_TRANSPOSE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Transposing adjacency matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Transpose adjacency matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the cocitation matrix
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyCocitation()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-cocitation-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_COCITATION, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Cocitation matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Cocitation matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the degree matrix of the graph
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-degree-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_DEGREE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Degree matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Degree matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the Laplacian matrix of the graph
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixLaplacian()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    qCDebug(lcMainWindow) << "MW:slotAnalyzeMatrixLaplacian()";

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-laplacian-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_LAPLACIAN, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Laplacian matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Laplacian matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief If the network has weighted / valued edges, it asks the user
 * if the app should consider weights or not.
 */
void MainWindow::askAboutEdgeWeights(const bool userTriggered)
{

    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - checking if graph weighted.";

    if (userTriggered)
    {
        if (!activeGraph->isWeighted())
        {
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
    else
    {
        if (!activeGraph->isWeighted())
        {
            optionsEdgeWeightConsiderAct->setChecked(false);
            return;
        }
    }
    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - graph weighted - checking if we have asked user.";

    if (askedAboutWeights)
    {
        return;
    }

    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - graph weighted - let's ask the user.";

    switch (
        slotHelpMessageToUser(USER_MSG_QUESTION,
                              tr("Weighted Network"),
                              tr("This is a weighted network. Consider edge weights?"),
                              tr("The ties in this network have weights (non-unit values) assigned to them. "
                                 "Do you want me to take these edge weights into account (i.e. when computing distances) ?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)

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

    if (optionsEdgeWeightConsiderAct->isChecked())
    {
        switch (

            slotHelpMessageToUser(
                USER_MSG_QUESTION, tr("Inverse edge weights during calculations? "),
                tr("Inverse edge weights during calculations? "),
                tr("If the edge weights denote cost or real distances (i.e. miles between cities), "
                   "press No, since the distance between two nodes should be the quickest "
                   "or cheaper one. \n\n"
                   "If the weights denote value or strength (i.e. votes or interaction), "
                   "press Yes to inverse the weights, since the distance between two "
                   "nodes should be the most valuable one."),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)

        )
        {
        case QMessageBox::Yes:
            inverseWeights = true;
            break;
        case QMessageBox::No:
            inverseWeights = false;
            break;
        default: // just for sanity
            inverseWeights = true;
            return;
            break;
        }
    }
    askedAboutWeights = true;
    return;
}

/**
 * @brief Handles requests to compute the graph/geodesic distance between two user-specified nodes
 *
 * The geodesic distance of two nodes is the length of the shortest path between them.
 *
 */
void MainWindow::slotAnalyzeDistance()
{
    if (!activeNodes() || !activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool ok1 = false, ok2 = false;
    int min = 1, max = 1, sourceNum = -1, targetNum = -1;

    min = activeGraph->vertexNumberMin();
    max = activeGraph->vertexNumberMax();

    sourceNum = QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select source node (%1..%2):")
            .arg(QString::number(min))
            .arg(QString::number(max)),
        min, min, max, 1, &ok1);

    if (!ok1)
    {
        statusMessage("Distance calculation operation cancelled.");
        return;
    }

    targetNum = QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select target node (%1..%2):")
            .arg(QString::number(min), QString::number(max)),
        min, min, max, 1, &ok2);

    if (!ok2)
    {
        statusMessage(tr("Distance calculation operation cancelled."));
        return;
    }

    qCDebug(lcMainWindow) << "Computing geodesic distance:" << sourceNum << "->" << targetNum;

    if (activeGraph->isSymmetric() && sourceNum > targetNum)
    {
        qSwap(sourceNum, targetNum);
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto distanceGeodesic = std::make_shared<int>(0);

    runGraphOperationAsync(
        [this, sourceNum, targetNum, considerWeights, inverseWeightsFinal, distanceGeodesic]() {
            *distanceGeodesic = activeGraph->graphDistanceGeodesic(
                sourceNum, targetNum, considerWeights, inverseWeightsFinal);
        },
        tr("Computing geodesic distance. Please wait..."),
        [this, sourceNum, targetNum, considerWeights, inverseWeightsFinal, distanceGeodesic]() {
            if (*distanceGeodesic > 0 && *distanceGeodesic < RAND_MAX)
            {
                qCDebug(lcMainWindow) << "geodesic distance" << sourceNum << "->" << targetNum << "=" << *distanceGeodesic;

                // Reconstruct the actual shortest path so the user sees the intermediate nodes.
                // Cheap regardless of network size (single-source BFS/Dijkstra, not the full
                // APSP the call above just triggered) - safe to run synchronously here.
                const QList<int> path = activeGraph->graphGeodesicShortestPath(
                    sourceNum, targetNum, considerWeights, inverseWeightsFinal);

                // Format the path as "v1 → v2 → … → vN" using node labels where available.
                QString pathStr;
                if (path.size() >= 2) {
                    for (int i = 0; i < path.size(); ++i) {
                        if (i > 0) pathStr += " \xE2\x86\x92 ";   // → (UTF-8 right arrow)
                        const QString lbl = activeGraph->vertexLabel(path[i]).trimmed();
                        pathStr += lbl.isEmpty() ? QString::number(path[i]) : lbl;
                    }
                }

                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(*distanceGeodesic),
                    tr("Geodesic Distance: %1").arg(*distanceGeodesic),
                    tr("Nodes %1 and %2 are connected. The shortest path has length %3.\n\n"
                       "Shortest path:\n%4")
                        .arg(sourceNum)
                        .arg(targetNum)
                        .arg(*distanceGeodesic)
                        .arg(pathStr.isEmpty() ? tr("(path unavailable)") : pathStr));

                if (path.size() >= 2)
                    graphicsWidget->selectPath(path);
            }
            else
            {
                qCDebug(lcMainWindow) << "geodesic distance" << sourceNum << "->" << targetNum << "is infinite.";
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Nodes %1 and %2 are not connected. "
                       "In this case, their geodesic distance is considered to be infinite.")
                        .arg(sourceNum)
                        .arg(targetNum));
            }
        });
}

/**
 * @brief Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixDistances()
{
    qCDebug(lcMainWindow) << "Request to compute the matrix of geodesic distances. Please wait...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesic-distances-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_DISTANCES,
                                                considerWeights, inverseWeightsFinal, dropIsolates,
                                                "Rows", false, reportFormat);
        },
        tr("Computing geodesic distances. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Geodesic Distances matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Invokes calculation of the geodedics matrix (the number of shortest paths
 * between each pair of nodes in the loaded network), then displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixGeodesics()
{
    qCDebug(lcMainWindow) << "Request to compute the matrix of geodesics. Please wait...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesics-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_GEODESICS,
                                                considerWeights, inverseWeightsFinal, dropIsolates,
                                                "Rows", false, reportFormat);
        },
        tr("Computing geodesics (number of shortest paths) for each pair. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Geodesics Matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the network diameter (largest geodesic)
 */
void MainWindow::slotAnalyzeDiameter()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto netDiameter = std::make_shared<int>(0);
    auto isWeighted = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, considerWeights, inverseWeightsFinal, netDiameter, isWeighted]() {
            *netDiameter = activeGraph->graphDiameter(considerWeights, inverseWeightsFinal);
            *isWeighted = activeGraph->isWeighted();
        },
        tr("Computing graph diameter. Please wait..."),
        [this, considerWeights, netDiameter, isWeighted]() {
            if (*isWeighted)
            {
                if (considerWeights)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                           "D = %1")
                            .arg(*netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                           "(maximum shortest path length) between any two nodes.\n\n"
                           "Note, since this is a weighted network, "
                           "the diameter can be greater than N."));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                           "D = %1")
                            .arg(*netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                           "(maximum shortest path length) between any two nodes.\n\n"
                           "Note, edge weights were disregarded during the computation. "
                           "This is the diameter of the corresponding network without weights."));
                }
            }
            else
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Network diameter computed."),
                    tr("Network diameter computed. \n\n"
                       "D = %1")
                        .arg(*netDiameter),
                    tr("The diameter of a network is the maximum geodesic distance "
                       "(maximum shortest path length) between any two nodes.\n\n"
                       "Note, since this is a non-weighted network, the diameter is always smaller than N-1."));
        });
}

/**
 * @brief Displays the average shortest path length (average graph distance)
 */
void MainWindow::slotAnalyzeDistanceAverage()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto averGraphDistance = std::make_shared<qreal>(0);
    auto isConnected = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, considerWeights, inverseWeightsFinal, dropIsolates, averGraphDistance, isConnected]() {
            *averGraphDistance = activeGraph->graphDistanceGeodesicAverage(
                considerWeights, inverseWeightsFinal, dropIsolates);
            // Cheap here: graphDistanceGeodesicAverage() just triggered the full APSP with
            // these exact params, so isConnected()'s cache hit is immediate - not a second
            // expensive computation.
            *isConnected = activeGraph->isConnected();
        },
        tr("Computing Average Graph Distance. Please wait..."),
        [this, averGraphDistance, isConnected]() {
            if (*isConnected)
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Average graph distance computed."),
                    tr("Average graph distance computed. \n\n"
                       "d = %1")
                        .arg(*averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                       "for all possible pairs of nodes.\n\n"
                       "The average distance in this connected network "
                       "is the sum of pair-wise distances divided by N * (N - 1)."));
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Average distance computed."),
                    tr("Average distance computed. \n\n"
                       "d = %1")
                        .arg(*averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                       "for all possible pairs of nodes.\n\n"
                       "The average distance in this disconnected network "
                       "is the sum of pair-wise distances divided by the number of existing geodesics."));
            }
        });
}

/**
 * @brief Computes and displays the distribution of geodesic distances across
 * all ordered pairs of connected nodes as an HTML report.
 *
 * Triggered only by explicit user action (Analyze → Cohesion → Geodesic Distance
 * Distribution).  Uses the APSP cache when available; otherwise runs the full
 * APSP computation first.  Writes the result to an HTML file and opens it in the
 * system browser or the built-in TextEditor according to app settings.
 */
void MainWindow::slotAnalyzeGeodesicDistribution()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeGeodesicDistribution()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"]
                 + "socnetv-report-geodesic-distribution-" + dateTime + ".html";

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, success]() {
            *success = activeGraph->writeGeodesicDistribution(fn, considerWeights, inverseWeightsFinal);
        },
        tr("Computing geodesic distance distribution. Please wait..."),
        [this, fn, success]() {
            if (!*success)
            {
                statusMessage(tr("Error: could not write geodesic distribution report."));
                return;
            }
            if (appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Geodesic distance distribution saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Eccentricity indices into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeEccentricity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeEccentricity(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eccentricity. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Eccentricities saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Reports the network connectedness
 */
void MainWindow::slotAnalyzeConnectedness()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeConnectedness()";

    int N = activeGraph->vertices();

    if (!N)
    {
        // null network with empty graph is connected
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("This empty network is considered connected!"),
            tr("Empty network is considered connected!"),
            tr("A null network (empty graph) is considered connected."));
    }
    else if (N == 1)
    {
        // 1-actor network with singleton graph is connected
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("This 1-actor network is considered connected!"),
            tr("This 1-actor network is considered connected!"),
            tr("A 1-actor network (singleton graph) is always considered connected."));
    }
    else
    {
        // Directed graphs have two distinct notions of connectivity (weak: ignores edge
        // direction; strong: requires all-pairs directed reachability) - ask which one to
        // check rather than silently picking one. Undirected graphs have no such ambiguity:
        // the two notions coincide, so there's nothing to ask.
        bool useStrong = false;
        if (activeGraph->isDirected())
        {
            bool ok = false;
            const QStringList connectivityTypes = {
                tr("Weak (ignores edge direction)"),
                tr("Strong (respects edge direction)")
            };
            const QString choice = QInputDialog::getItem(
                this,
                tr("Connectedness"),
                tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
                   "Weak: treats every edge as bidirectional. Answers \"how many disconnected "
                   "islands are there?\"\n\n"
                   "Strong: respects edge direction. Every node must be able to reach, and be "
                   "reached from, every other node."),
                connectivityTypes, 0, false, &ok);

            if (!ok)
            {
                statusMessage(tr("Connectedness check cancelled."));
                return;
            }
            useStrong = (connectivityTypes.indexOf(choice) == 1);
        }

        const int components = useStrong
            ? activeGraph->graphStronglyConnectedComponents()
            : activeGraph->graphWeaklyConnectedComponents();
        const bool isConnected = (components == 1);

        qCDebug(lcMainWindow) << "MW::slotAnalyzeConnectedness result connected:" << isConnected
                 << "components:" << components << "strong:" << useStrong;

        const QString compStr = QString::number(components);
        if (isConnected)
        {
            if (activeGraph->isDirected())
            {
                if (useStrong)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is strongly connected (1 component)."),
                        tr("This directed network is strongly connected."),
                        tr("Every node can reach, and be reached from, every other node "
                           "via directed paths."));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is weakly connected (1 component)."),
                        tr("This directed network is weakly connected."),
                        tr("All nodes belong to a single weakly connected component. "
                           "Weak connectivity ignores edge direction - run Connectedness again "
                           "and choose Strong to check directed reachability."));
                }
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("This undirected network is connected (1 component)."),
                    tr("This undirected network is connected."),
                    tr("All nodes belong to a single connected component. "
                       "There is a path between every pair of nodes."));
            }
        }
        else
        {
            if (activeGraph->isDirected())
            {
                if (useStrong)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is not strongly connected (%1 components).").arg(compStr),
                        tr("This directed network is not strongly connected."),
                        tr("There are %1 strongly connected components. "
                           "Some node pairs cannot reach each other via directed paths.")
                            .arg(compStr));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is disconnected (%1 components).").arg(compStr),
                        tr("This directed network is disconnected."),
                        tr("There are %1 disconnected components. "
                           "Some node pairs are unreachable from each other, even ignoring "
                           "edge direction. "
                           "Use Layout > Node Color by Connected Component "
                           "to visualize the components.")
                            .arg(compStr));
                }
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("This undirected network is disconnected (%1 components).").arg(compStr),
                    tr("This undirected network is disconnected."),
                    tr("There are %1 disconnected components. "
                       "Some node pairs have no path between them. "
                       "Use Layout > Node Color by Connected Component "
                       "to visualize the components.")
                        .arg(compStr));
            }
        }
    }
}

/**
 * @brief Handles requests to compute the local vertex connectivity between two user-specified
 * nodes: the minimum number of other nodes that must be removed to disconnect them.
 */
void MainWindow::slotAnalyzeNodeConnectivity()
{
    if (!activeNodes() || !activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok1 = false, ok2 = false;
    const int min = activeGraph->vertexNumberMin();
    const int max = activeGraph->vertexNumberMax();

    const int sourceNum = QInputDialog::getInt(
        this,
        tr("Node Connectivity"),
        tr("Select source node (%1..%2):")
            .arg(QString::number(min))
            .arg(QString::number(max)),
        min, min, max, 1, &ok1);

    if (!ok1)
    {
        statusMessage(tr("Node connectivity calculation cancelled."));
        return;
    }

    const int targetNum = QInputDialog::getInt(
        this,
        tr("Node Connectivity"),
        tr("Select target node (%1..%2):")
            .arg(QString::number(min), QString::number(max)),
        min, min, max, 1, &ok2);

    if (!ok2)
    {
        statusMessage(tr("Node connectivity calculation cancelled."));
        return;
    }

    bool respectDirection = false;
    if (activeGraph->isDirected())
    {
        bool ok = false;
        const QStringList connectivityTypes = {
            tr("Weak (ignores edge direction)"),
            tr("Strong (respects edge direction)")
        };
        const QString choice = QInputDialog::getItem(
            this,
            tr("Node Connectivity"),
            tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
               "Weak: treats every edge as bidirectional.\n\n"
               "Strong: respects edge direction - counts nodes needed to block directed paths "
               "from the source to the target."),
            connectivityTypes, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Node connectivity calculation cancelled."));
            return;
        }
        respectDirection = (connectivityTypes.indexOf(choice) == 1);
    }

    qCDebug(lcMainWindow) << "Computing node connectivity:" << sourceNum << "->" << targetNum
                          << "respectDirection:" << respectDirection;

    auto result = std::make_shared<Graph::NodeConnectivityResult>();

    runGraphOperationAsync(
        [this, sourceNum, targetNum, respectDirection, result]() {
            *result = activeGraph->graphNodeConnectivity(sourceNum, targetNum, respectDirection);
        },
        tr("Computing node connectivity. Please wait..."),
        [this, sourceNum, targetNum, result]() {
            switch (result->status)
            {
            case Graph::NodeConnectivityStatus::Adjacent:
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Nodes %1 and %2 are directly connected.").arg(sourceNum).arg(targetNum),
                    tr("Nodes %1 and %2 are directly connected.").arg(sourceNum).arg(targetNum),
                    tr("Node connectivity is not defined for directly connected nodes: "
                       "the edge between them means no removal of other nodes can ever "
                       "separate them."));
                break;
            case Graph::NodeConnectivityStatus::Ok:
                if (result->value > 0)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Node Connectivity: %1").arg(result->value),
                        tr("Node Connectivity: %1").arg(result->value),
                        tr("At least %1 other node(s) must be removed to disconnect "
                           "%2 from %3.")
                            .arg(result->value)
                            .arg(targetNum)
                            .arg(sourceNum));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Node Connectivity: %1").arg(QString("\xE2\x88\x9E")),
                        tr("Nodes %1 and %2 are already disconnected.").arg(sourceNum).arg(targetNum),
                        tr("%1 cannot reach %2 at all, so no node removal is needed.")
                            .arg(sourceNum)
                            .arg(targetNum));
                }
                break;
            case Graph::NodeConnectivityStatus::Invalid:
            default:
                slotHelpMessageToUser(
                    USER_MSG_CRITICAL,
                    tr("Invalid node connectivity request."),
                    tr("Invalid node connectivity request."),
                    tr("Source and target must be two distinct, existing nodes."));
                break;
            }
        });
}

/**
 * @brief Handles requests to compute the network's overall vertex connectivity: the minimum,
 * over every pair of nodes, of their local vertex connectivity.
 */
void MainWindow::slotAnalyzeConnectivity()
{
    const int N = activeGraph->vertices();
    if (N < 2)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool respectDirection = false;
    if (activeGraph->isDirected())
    {
        bool ok = false;
        const QStringList connectivityTypes = {
            tr("Weak (ignores edge direction)"),
            tr("Strong (respects edge direction)")
        };
        const QString choice = QInputDialog::getItem(
            this,
            tr("Graph Connectivity"),
            tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
               "Weak: treats every edge as bidirectional.\n\n"
               "Strong: respects edge direction."),
            connectivityTypes, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Graph connectivity calculation cancelled."));
            return;
        }
        respectDirection = (connectivityTypes.indexOf(choice) == 1);
    }

    qCDebug(lcMainWindow) << "Computing graph connectivity, respectDirection:" << respectDirection;

    auto kappa = std::make_shared<int>(0);

    runGraphOperationAsync(
        [this, respectDirection, kappa]() {
            *kappa = activeGraph->graphConnectivity(respectDirection);
        },
        tr("Computing graph connectivity. Please wait..."),
        [this, kappa]() {
            if (*kappa > 0)
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Graph Connectivity: %1").arg(*kappa),
                    tr("Graph Connectivity: %1").arg(*kappa),
                    tr("At least %1 node(s) must be removed to disconnect some pair "
                       "of nodes in this network - its worst-case robustness to "
                       "node removal.")
                        .arg(*kappa));
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Graph Connectivity: 0"),
                    tr("This network is already disconnected."),
                    tr("Some pair of nodes is already unreachable, so no node removal "
                       "is needed to disconnect the network further."));
            }
        });
}

/**
 *	Calculate and print the number of walks of a given length , between each pair of nodes.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeWalksLength()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;

    int length = QInputDialog::getInt(
        this, "Number of walks",
        tr("Select desired length of walk: (2 to %1)").arg(activeNodes() - 1),
        2, 2, activeNodes() - 1, 1, &ok);
    if (!ok)
    {
        statusMessage("Cancelled.");
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-length-" + QString::number(length) + "-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);
    runGraphOperationAsync(
        [this, fn, length, reportFormat, success]() {
            *success = activeGraph->writeMatrixWalks(fn, length, false, reportFormat);
        },
        tr("Computing walks of length %1. Please wait...").arg(length),
        [this, fn, length, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Walks of length %1 matrix saved as: ").arg(length) + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calculate and print the total number of walks of any length, between each pair of nodes.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeWalksTotal()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 50)
    {
        switch (QMessageBox::critical(
            this,
            "Slow function warning",
            tr("Please note that this function is VERY SLOW on large networks (n>50), "
               "since it will calculate all powers of the sociomatrix up to n-1 "
               "in order to find out all possible walks. \n\n"
               "If you need to make a simple reachability test, "
               "we advise to use the Reachability Matrix function instead. \n\n"
               "Are you sure you want to continue?"),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-total-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);
    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrixWalks(fn, 0, false, reportFormat);
        },
        tr("Computing total walks matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage("Total walks matrix saved as: " + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calls Graph::writeMatrix(fn, MATRIX_REACHABILITY) to calculate and print
 *   the Reachability Matrix of the network.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeReachabilityMatrix()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-reachability-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_REACHABILITY, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing reachability matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Reachability matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calls Graph::writeClusteringCoefficient() to write Clustering Coefficients
 * into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeClusteringCoefficient()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficient-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeClusteringCoefficient(fn, considerWeights, reportFormat);
        },
        tr("Computing Clustering Coefficients. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Clustering Coefficients saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calls Graph:: writeCliqueCensus() to write the Clique Census
 *  into a file, then displays it.
 */
void MainWindow::slotAnalyzeCommunitiesCliqueCensus()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() == 1)
    {
        slotHelpMessageToUserError("Only one node is present, therefore 1 clique");
        return;
    }

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clique-census-" + dateTime + ".html";

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, success]() {
            *success = activeGraph->writeCliqueCensus(fn, considerWeights);
        },
        tr("Computing Clique Census. Please wait..."),
        [this, fn, success]() {
            if (!*success)
            {
                return;
            }
            if (appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Clique Census saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calls Graph to compute and write a triad census into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCommunitiesTriadCensus()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-triad-census-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeTriadCensus(fn, considerWeights, reportFormat);
        },
        tr("Computing Triad Census. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Triad Census saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the DialogSimilarityMatches dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    m_dialogSimilarityMatches = new DialogSimilarityMatches(this);

    connect(m_dialogSimilarityMatches, &DialogSimilarityMatches::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure);

    m_dialogSimilarityMatches->exec();
}

/**
 * @brief Calls Graph::writeMatrixSimilarityMatching() to write a
 * similarity matrix according to given measure into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure(const QString &matrix,
                                                              const QString &varLocation,
                                                              const QString &measure,
                                                              const bool &diagonal)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString metric;
    if (measure.contains("Simple", Qt::CaseInsensitive))
        metric = "simple-matching";
    else if (measure.contains("Jaccard", Qt::CaseInsensitive))
        metric = "jaccard";
    else if (measure.contains("None", Qt::CaseInsensitive))
        metric = "none";
    else if (measure.contains("Hamming", Qt::CaseInsensitive))
        metric = "hamming";
    else if (measure.contains("Cosine", Qt::CaseInsensitive))
        metric = "cosine";
    else if (measure.contains("Euclidean", Qt::CaseInsensitive))
        metric = "euclidean";
    else if (measure.contains("Manhattan", Qt::CaseInsensitive))
        metric = "manhattan";
    else if (measure.contains("Pearson ", Qt::CaseInsensitive))
        metric = "pearson";
    else if (measure.contains("Chebyshev", Qt::CaseInsensitive))
        metric = "chebyshev";

    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-similarity-" + metric + "-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, measure, matrix, varLocation, diagonal, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeMatrixSimilarityMatching(
                fn, measure, matrix, varLocation, diagonal, considerWeights, reportFormat);
        },
        tr("Computing Similarity Matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Similarity matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the DialogDissimilarities dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceDissimilaritiesDialog()";

    m_dialogdissimilarities = new DialogDissimilarities(this);

    connect(m_dialogdissimilarities, &DialogDissimilarities::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile);

    m_dialogdissimilarities->exec();
}

/**
 * @brief Invokes calculation of pair-wise tie profile dissimilarities of the
 * network, then displays it.
 * @param metric
 * @param varLocation
 * @param diagonal
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile(const QString &metric,
                                                                    const QString &varLocation,
                                                                    const bool &diagonal)
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString metricStr;
    if (metric.contains("Simple", Qt::CaseInsensitive))
        metricStr = "simple-matching";
    else if (metric.contains("Jaccard", Qt::CaseInsensitive))
        metricStr = "jaccard";
    else if (metric.contains("None", Qt::CaseInsensitive))
        metricStr = "none";
    else if (metric.contains("Hamming", Qt::CaseInsensitive))
        metricStr = "hamming";
    else if (metric.contains("Cosine", Qt::CaseInsensitive))
        metricStr = "cosine";
    else if (metric.contains("Euclidean", Qt::CaseInsensitive))
        metricStr = "euclidean";
    else if (metric.contains("Manhattan", Qt::CaseInsensitive))
        metricStr = "manhattan";
    else if (metric.contains("Pearson ", Qt::CaseInsensitive))
        metricStr = "pearson";
    else if (metric.contains("Chebyshev", Qt::CaseInsensitive))
        metricStr = "chebyshev";

    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-dissimilarities-" + metricStr + "-" + dateTime + ext;

    askAboutEdgeWeights();

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, metric, varLocation, diagonal, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeMatrixDissimilarities(fn, metric, varLocation, diagonal,
                                                                considerWeights, reportFormat);
        },
        tr("Computing Tie Profile Dissimilarities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Tie profile dissimilarities matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calls the m_dialogSimilarityPearson to display the Pearson statistics dialog
 */
void MainWindow::slotAnalyzeStrEquivalencePearsonDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalencePearsonDialog()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    m_dialogSimilarityPearson = new DialogSimilarityPearson(this);

    connect(m_dialogSimilarityPearson, &DialogSimilarityPearson::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalencePearson);

    m_dialogSimilarityPearson->exec();
}

/**
 * @brief Calls Graph::writeMatrixSimilarityPearson() to write Pearson
 * Correlation Coefficients into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalencePearson(const QString &matrix,
                                                  const QString &varLocation,
                                                  const bool &diagonal)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-pearson-coefficients-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, matrix, varLocation, diagonal, reportFormat, success]() {
            *success = activeGraph->writeMatrixSimilarityPearson(
                fn, considerWeights, matrix, varLocation, diagonal, reportFormat);
        },
        tr("Computing Pearson Correlation Coefficients. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Pearson correlation coefficients matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the slotAnalyzeStrEquivalenceClusteringHierarchicalDialog dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString preselectMatrix = "Adjacency";

    if (!activeGraph->isWeighted())
    {
        preselectMatrix = "Distances";
    }
    m_dialogClusteringHierarchical = new DialogClusteringHierarchical(this, preselectMatrix);

    connect(m_dialogClusteringHierarchical, &DialogClusteringHierarchical::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchical);

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
                                                                 const bool &diagram)
{

    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceClusteringHierarchical()";

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-hierarchical-clustering-" + dateTime + ".html";

    bool considerWeights = activeGraph->isWeighted();
    bool inverseWeights = false;
    bool dropIsolates = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, varLocation, matrix, metric, method, diagonal, diagram,
         considerWeights, inverseWeights, dropIsolates, success]() {
            *success = activeGraph->writeClusteringHierarchical(fn,
                                                                 varLocation,
                                                                 matrix,
                                                                 metric,
                                                                 method,
                                                                 diagonal,
                                                                 diagram,
                                                                 considerWeights,
                                                                 inverseWeights,
                                                                 dropIsolates);
        },
        tr("Computing Hierarchical Cluster Analysis. Please wait..."),
        [this, fn, success]() {
            if (!*success)
            {
                return;
            }
            if (appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Hierarchical Cluster Analysis saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Out-Degree Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights(false);

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree-" + dateTime + ext;

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityDegree(fn, considerWeights, dropIsolates, reportFormat);
        },
        tr("Computing Degree Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Out-Degree Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Out-Degree Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Closeness Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityCloseness()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeCentralityCloseness()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    askAboutEdgeWeights();

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-" + dateTime + ext;

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityCloseness(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Closeness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Closeness Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Closeness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityClosenessIR
 *	Writes Centrality Closeness (based on Influence Range) indices into a file,
 *   then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityClosenessIR()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-influence-range-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityClosenessInfluenceRange(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Influence Range Closeness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Influence Range Closeness Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Influence Range Closeness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Betweenness Centralities into a file, then displays it.
 */
/**
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityBetweenness()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-betweenness-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityBetweenness(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Betweenness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Betweenness Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Betweenness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Degree Prestige indices (In-Degree Centralities) into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigeDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric())
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("This network is not directed (undirected graph). "
                                 "The Degree Prestige index counts inbound edges, "
                                 "therefore it is meaningful on directed networks. "
                                 "For undirected networks, such as this one, Degree Prestige is the same as Degree Centrality."));
    }

    askAboutEdgeWeights(false);

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-degree-" + dateTime + ext;

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigeDegree(fn, considerWeights, dropIsolates, reportFormat);
        },
        tr("Computing Degree Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Degree Prestige (in-degree) report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Degree Prestige (in-degree) report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes PageRank Prestige indices into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigePageRank()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-pagerank-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigePageRank(fn, dropIsolates, reportFormat);
        },
        tr("Computing PageRank Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening PageRank Prestige report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("PageRank Prestige report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzePrestigeProximity
 * Writes Proximity Prestige indices into a file, then displays them.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigeProximity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-proximity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigeProximity(fn, true, false, dropIsolates, reportFormat);
        },
        tr("Computing Proximity Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Proximity Prestige report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Proximity Prestige report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityInformation
 * Writes Informational Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityInformation()
{

    qCDebug(lcMainWindow) << "MW::slotAnalyzeCentralityInformation()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    // WS14: recalibrated 2026-07-31 after L4 removed matrix.cpp's qDebug() formatting tax, which
    // (per real measurement) had been the dominant cost at the old n>200 threshold -- the previous
    // "2 minutes for 600 nodes on an i7 4790K" claim is now off by roughly two orders of magnitude
    // (600 nodes measures ~1.2s post-fix). The genuine O(n^3) matrix-inversion cost this warns
    // about is real and unaffected by that fix, though, and does become slow again at large n --
    // measured (MacBook Pro M5, 24GB RAM): 1000 nodes ~3.1s, 2000 nodes ~15.8s, growing faster than
    // linearly beyond that. Threshold raised accordingly; wording updated to current numbers, to lead
    // with the more important caveat now that speed alone rarely justifies interrupting the user:
    // this computation is not yet cancellable once started (WS5 roadmap, issue I1).
    if (activeNodes() > 2000)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function can be <b>SLOW</b> on very large "
                   "networks (n>2000), and cannot currently be canceled once started. <br><br>"
                   "It computes the (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "then computes the inverse matrix C of A via LU decomposition -- an O(n&sup3;) "
                   "operation. <br><br>"
                   "How slow is this? On a modern machine, a 2,000-node network takes about 15 "
                   "seconds; since the cost grows roughly cubically with network size, much larger "
                   "networks (tens of thousands of nodes) can take several minutes or more. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
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

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-information-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, reportFormat, success]() {
            *success = activeGraph->writeCentralityInformation(
                fn, considerWeights, inverseWeightsFinal, reportFormat);
        },
        tr("Computing Information Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Information Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Information Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes Eigenvector Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityEigenvector()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eigenvector-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    const bool dropIsolates = false;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityEigenvector(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eigenvector Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Eigenvector Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Eigenvector Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityStress
 * Writes Stress Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityStress()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-stress-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityStress(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Stress Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Stress Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Stress Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityPower
 * Writes Gil-Schmidt Power Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityPower()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-power-Gil-Schmidt-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityPower(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Power Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Gil-Schmidt Power Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Gil-Schmidt Power Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityEccentricity
 * Writes Eccentricity Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityEccentricity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eccentricity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityEccentricity(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eccentricity Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Closeness Centralities report..."));
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Eccentricity Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
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
                                                              const qreal &maxF)
{

    Q_UNUSED(minF);

    qCDebug(lcMainWindow) << "Updating the prominence distribution miniChart";

    if (series == Q_NULLPTR)
    {
        qCDebug(lcMainWindow) << "series is null! Resetting to trivial";
        miniChart->resetToTrivial();
        return;
    }

    // Set the style of the lines and bars
    switch (series->type())
    {
    case QAbstractSeries::SeriesTypeBar:
        qCDebug(lcMainWindow) << "this an BarSeries";
        break;
    case QAbstractSeries::SeriesTypeArea:
        qCDebug(lcMainWindow) << "this an AreaSeries";

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
    miniChart->setTitle(series->name() + QString(" distribution"), QFont("Times", 8));

    miniChart->toggleLegend(false);

    QString chartHelpMsg = tr("Distribution of %1 values:\n"
                              "Min value: %2 \n"
                              "Max value: %3 \n"
                              "Please note that, due to the small size of this widget, \n"
                              "if you display a distribution in Bar Chart where there are \n"
                              "more than 10 values, the widget will not show all bars. \n"
                              "In this case, use Line or Area Chart (from Settings). \n"
                              "In any case, the large chart in the HTML report \n"
                              "is better than this widget...")
                               .arg(series->name())
                               .arg(min, 0, 'g', appSettings["initReportsRealNumberPrecision"].toInt(0, 10))
                               .arg(max, 0, 'g', appSettings["initReportsRealNumberPrecision"].toInt(0, 10));

    miniChart->setToolTip(chartHelpMsg);

    miniChart->setWhatsThis(chartHelpMsg);

    // if true, then bar chart appears with default X axis (1,2,3 ...)
    bool useDefaultAxes = false;

    if (!useDefaultAxes)
    {
        if (axisX != Q_NULLPTR)
        {
            qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisX not null. Setting it to miniChart";
            miniChart->setAxisX(axisX, series);

            miniChart->setAxisXMin(0);
            miniChart->setAxisXLabelFont();
            miniChart->setAxisXLinePen();
            miniChart->setAxisXGridLinePen();
            miniChart->setAxisXLabelsAngle(-90);
        }
        if (axisY != Q_NULLPTR)
        {
            qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                        "axisY not null. Setting it to miniChart";
            miniChart->setAxisY(axisY, series);
            miniChart->setAxisYMin(0);
            miniChart->setAxisYLabelFont();
            miniChart->setAxisYLinePen();
            miniChart->setAxisYGridLinePen();
        }
    }

    if ((axisX == Q_NULLPTR && axisY == Q_NULLPTR) || useDefaultAxes)
    {

        qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - "
                    "axisX and axisY null. Calling createDefaultAxes()";
        miniChart->createDefaultAxes();

        qCDebug(lcMainWindow) << "MW::slotAnalyzeProminenceDistributionChartUpdate() - setting axis min";
        miniChart->setAxisYMin(0);
        miniChart->setAxisXMin(0);

        // Apply our theme to axes:
        miniChart->setAxesThemeDefault();
        miniChart->axes(Qt::Vertical).first()->setMax(maxF + 1.0);
        miniChart->setAxisXLabelsAngle(-90);
        //    axisX->setShadesVisible(false);
    }
}

/**
 * @brief Fixes known bugs in QProgressDialog class.
   i.e. Workaround for macOS-only Qt bug: QTBUG-65750, QTBUG-70357.
   QProgressDialog too small and too narrow to fit the text of its label
 * @param dialog
 */
void MainWindow::polishProgressDialog(QProgressDialog *dialog)
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
 * @brief Enables/disables the menu bar, toolbar, and canvas while a graphThread operation runs.
 *
 * Fixes a live, reproducible crash (WS15 P2, roadmap_ws15_cancellation_progress_unification.md):
 * graph-mutating actions (e.g. "New") run directly on the GUI thread with no dispatch to
 * graphThread and no guard, so one could run concurrently with an in-flight graphThread
 * computation - a genuine use-after-free race on Graph's own data. The Qt::ApplicationModal
 * progress dialog was assumed to already prevent this but doesn't reliably (root cause not fully
 * understood, see the roadmap); this is an unconditional second layer that doesn't depend on
 * understanding that gap. graphicsWidget is included because add-node/add-edge via mouse
 * (mousePressEvent/mouseDoubleClickEvent) bypasses the menu/toolbar entirely.
 *
 * menuBar()/toolBar's own setEnabled(false) only disables those container widgets - it blocks
 * clicks but leaves each QAction's own isEnabled() (and therefore its keyboard shortcut, e.g.
 * Ctrl+N) untouched, since actions aren't Qt children of the menus/toolbars they're added to.
 * Root cause found live 2026-08-08: clicking Cancel on the busy dialog doesn't stop the
 * still-running graphThread computation, but Ctrl+N straight afterwards could still fire and
 * race it - a real, reproducible crash on both macOS and Linux. Fixed here by also disabling
 * every QAction reachable from the menu bar/toolbar (recursively through submenus) while busy,
 * and restoring only the ones this function itself disabled - not blanket re-enabling everything,
 * since plenty of actions are legitimately disabled elsewhere for unrelated reasons (e.g. "no
 * network loaded"). This alone closes the crash path even though the dialog itself still hides
 * on Cancel before the computation actually stops (a separate, lower-severity UX follow-up).
 *
 * @param busy true to disable (operation starting), false to re-enable (operation complete)
 */
void MainWindow::setAppBusy(bool busy)
{
    menuBar()->setEnabled(!busy);
    toolBar->setEnabled(!busy);
    graphicsWidget->setEnabled(!busy);
    // leftPanel hosts the toolbox comboboxes (edit/analysis/visualization selects, e.g.
    // toolBoxAnalysisStrEquivalenceSelect) - plain QComboBoxes, not QActions, so the
    // menuBar/toolBar QAction sweep below never reaches them. Without this, a toolbox
    // selection can re-trigger a runGraphOperationAsync operation while a prior one is
    // still running on graphThread, racing on shared Graph:: matrix members (AM/SCM/etc.)
    leftPanel->setEnabled(!busy);

    if (busy)
    {
        std::function<void(const QList<QAction *> &)> disableActions =
            [this, &disableActions](const QList<QAction *> &actions) {
                for (QAction *action : actions)
                {
                    if (action->isEnabled())
                    {
                        action->setEnabled(false);
                        m_actionsDisabledForBusy << action;
                    }
                    if (action->menu())
                    {
                        disableActions(action->menu()->actions());
                    }
                }
            };
        disableActions(menuBar()->actions());
        disableActions(toolBar->actions());
    }
    else
    {
        for (QAction *action : m_actionsDisabledForBusy)
        {
            action->setEnabled(true);
        }
        m_actionsDisabledForBusy.clear();
    }
}

/**
 * @brief Runs a slow Graph operation on graphThread without blocking the GUI thread (fix #254).
 *
 * `activeGraph` already lives on graphThread (see MainWindow::initGraph()) - this dispatches
 * `operation` there via a queued invocation instead of introducing any new thread or using
 * QtConcurrent::run(). That matters: DistanceEngine::compute() already parallelises internally
 * via QtConcurrent::blockingMap over the *global* thread pool, so wrapping the outer call in
 * QtConcurrent::run() as well would submit a second task to that same pool and risk contending
 * with blockingMap's own worker tasks. Routing through graphThread sidesteps that entirely -
 * blockingMap's behaviour is completely unaffected; only the identity of the thread that calls
 * and blocks on it changes (graphThread instead of the GUI thread).
 *
 * Shows an indeterminate progress dialog (no setValue() calls - cheap regardless of network
 * size, unlike the granular showProgressBar mechanism) for the duration. Because it's
 * ApplicationModal, it also prevents the user from mutating the graph from the GUI thread while
 * `operation` runs on graphThread - required, since DistanceEngine and friends mutate Graph's
 * member state directly and are not internally synchronized against concurrent access.
 *
 * The dialog's Cancel button is wired to the same Graph::slotCancelComputation() the existing
 * granular progress dialog uses. Note this carries over an existing limitation, not a new one:
 * cancellation can't interrupt mid-blockingMap (see the comment in
 * DistanceEngine::runAllSources()) - it can only take effect at the checkpoints initRun()/
 * finalize() already have.
 *
 * @param operation   The Graph call to run, e.g. [this, args...](){ activeGraph->foo(args...); }
 *                    — capture only plain values read from UI widgets *before* calling this
 *                    method, never QAction/QWidget pointers to be dereferenced inside the
 *                    lambda body, since that body executes on graphThread, not the GUI thread.
 * @param waitMessage Status bar message and progress dialog text shown immediately.
 * @param doneMessage Status bar message shown on completion (optional). For anything beyond a
 *                    plain status message on completion (e.g. opening a generated report file,
 *                    which must wait for `operation` to actually finish and only happen if it
 *                    succeeded), use the std::function<void()> overload below instead.
 */
void MainWindow::runGraphOperationAsync(std::function<void()> operation,
                                        const QString &waitMessage,
                                        const QString &doneMessage)
{
    runGraphOperationAsync(operation, waitMessage, [this, doneMessage]() {
        if (!doneMessage.isEmpty())
            statusMessage(doneMessage);
    });
}

/**
 * @brief Overload of runGraphOperationAsync() for completions that need more than a status
 * message - e.g. opening a report file that `operation` just wrote, which must only happen
 * after `operation` actually finishes (and, if `operation` reports success/failure via a
 * captured flag, only on success). See the primary overload above for the full explanation of
 * why this dispatches via graphThread rather than QtConcurrent::run().
 *
 * @param operation   Same contract as the primary overload - GUI state must be captured as
 *                    plain values *before* the call, not read from inside the lambda.
 * @param waitMessage Status bar message and progress dialog text shown immediately.
 * @param onComplete  Runs on the GUI thread after `operation` finishes. If `operation` needs to
 *                    report success/failure, capture a shared flag (e.g. `std::shared_ptr<bool>`)
 *                    in both lambdas - see the writeCentralityX() call sites for the pattern.
 */
void MainWindow::runGraphOperationAsync(std::function<void()> operation,
                                        const QString &waitMessage,
                                        std::function<void()> onComplete)
{
    // Must happen before the busy dialog exists (let alone is shown), not just before dispatch:
    // this is the one guaranteed reset point every wrapped operation goes through. Some chains
    // also reset internally (DistanceEngine's progress sink; randomNetErdosCreate()'s own reset,
    // kept for its --interactive-script benchmark path which bypasses this wrapper entirely) but
    // most don't - without this central call, a single earlier cancel would silently no-op every
    // subsequent wrapped operation's cancelCheck() forever after. See WS15 P3 Phase 2
    // (roadmap_ws15_cancellation_progress_unification.md).
    activeGraph->resetProgressCanceled();

    statusMessage(waitMessage);

    QProgressDialog *busyDialog = new QProgressDialog(waitMessage, tr("Cancel"), 0, 0, this);
    busyDialog->setWindowModality(Qt::ApplicationModal);
    busyDialog->setMinimumDuration(0);
    busyDialog->setAutoClose(false);
    busyDialog->setAutoReset(false);
    polishProgressDialog(busyDialog);
    // Qt::DirectConnection: runs slotCancelComputation() synchronously on this (GUI) thread at
    // click-time, instead of queueing onto graphThread's event loop - which is exactly what's
    // blocked for the whole duration of the computation this button is meant to interrupt. See
    // WS15 P1 (roadmap_ws15_cancellation_progress_unification.md).
    connect(busyDialog, &QProgressDialog::canceled,
            activeGraph, &Graph::slotCancelComputation, Qt::DirectConnection);
    // The Cancel button is wired by Qt itself to QProgressDialog::cancel(), which
    // unconditionally hides the dialog before emitting canceled() - setAutoClose(false)/
    // setAutoReset(false) above don't gate this path at all, only setValue() reaching
    // maximum() does. Without this, the dialog disappears immediately even though the
    // computation may keep running for a while (coarse cancelCheck granularity in some
    // operations), letting the user believe it already stopped (WS15 Finding 8). Re-show it,
    // relabeled and disabled, until the operation's own completion continuation below calls
    // reset()/deleteLater() for real.
    connect(busyDialog, &QProgressDialog::canceled, busyDialog, [busyDialog]() {
        busyDialog->setLabelText(tr("Canceling - please wait for the operation to stop..."));
        busyDialog->setEnabled(false);
        busyDialog->show();
    }, Qt::DirectConnection);
    busyDialog->show();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    setAppBusy(true);

    QMetaObject::invokeMethod(activeGraph, [this, operation, onComplete, busyDialog]() {
        operation();
        QMetaObject::invokeMethod(this, [this, onComplete, busyDialog]() {
            QApplication::restoreOverrideCursor();
            setAppBusy(false);
            // reset(), not close(): QProgressDialog::close() triggers its internal cancel()
            // path and emits canceled() -> Graph::slotCancelComputation() -> m_progressCanceled
            // stays true until the next resetProgressCanceled() call, silently no-op'ing every
            // operation after the first.
            //
            // reset() alone does NOT hide the dialog here: per QProgressDialog's docs, reset()
            // only hides it when autoClose is true, and setAutoClose(false) is set above (see
            // the cancel-button re-show workaround, WS15 Finding 8). Without an explicit hide(),
            // the dialog stayed mapped on screen - invisible-in-Qt's-model but still on-screen -
            // until deleteLater()'s deferred deletion eventually ran. For onComplete callbacks
            // that open their own modal QMessageBox synchronously (e.g. Node/Graph Connectivity's
            // result dialog), that deferred deletion doesn't happen until the new modal's nested
            // event loop is dismissed, so the progress dialog visibly lingered behind it the
            // whole time.
            busyDialog->reset();
            busyDialog->hide();
            busyDialog->deleteLater();
            if (onComplete)
                onComplete();
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

/**
 * @brief Warns before network generation above a safety edge-count threshold.
 *
 * Generating tens of millions of edges can exhaust memory and crash the app before any
 * progress dialog even appears - checked against the *expected* edge count up front, so
 * nothing partially-built needs to be unwound if the user declines.
 *
 * @param expectedEdges  Estimated edge count the generator would produce.
 * @param generatorLabel Human-readable generator name, used in the warning message.
 * @param scripted       True when called from --interactive-script: there's no one to answer
 *                        a modal prompt, so this refuses outright and logs instead, matching
 *                        the malformed-command pattern used elsewhere in the dispatcher.
 * @return true if generation should proceed (under the limit, or the user chose to proceed
 *         anyway), false if it was refused/declined.
 */
bool MainWindow::confirmGenerationSize(qint64 expectedEdges, const QString &generatorLabel,
                                       bool scripted)
{
    static constexpr qint64 kMaxSafeGeneratedEdges = 2000000;

    if (expectedEdges <= kMaxSafeGeneratedEdges)
    {
        return true;
    }

    if (scripted)
    {
        qWarning() << "Refusing" << generatorLabel << "- expected edges" << expectedEdges
                   << "exceeds the safety limit of" << kMaxSafeGeneratedEdges;
        return false;
    }

    return slotHelpMessageToUser(
               USER_MSG_QUESTION,
               tr("Large network"),
               tr("This %1 would create approximately %2 edges, "
                  "which exceeds the safety limit of %3.")
                   .arg(generatorLabel)
                   .arg(expectedEdges)
                   .arg(kMaxSafeGeneratedEdges),
               tr("Generating a network this large has been observed to exhaust available "
                  "memory and crash the application. Proceed anyway?"),
               QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
           == QMessageBox::Yes;
}

/**
 * @brief Toggles visibility of node numbers.
 * Persists the choice in appSettings["initNodeNumbersVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeNumbersVisibility()" << toggle;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Nodes Numbers. Please wait..."));
    appSettings["initNodeNumbersVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setNodeNumberVisibility(toggle);
    optionsNodeNumbersVisibilityAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Node Numbers are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Node Numbers are visible again..."));
    }
    QApplication::restoreOverrideCursor();
    return;
}

/**
 * @brief Toggles whether node numbers are drawn inside or outside nodes.
 * Shows node numbers first if they are currently hidden.
 * Persists the choice in appSettings["initNodeNumbersInside"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersInside(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeNumbersInside()" << toggle;

    statusMessage(tr("Toggle Numbers inside nodes. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    // if node numbers are hidden, show them first.
    if (toggle && appSettings["initNodeNumbersVisibility"] != "true")
        slotOptionsNodeNumbersVisibility(true);

    appSettings["initNodeNumbersInside"] = (toggle) ? "true" : "false";
    graphicsWidget->setNumbersInsideNodes(toggle);
    optionsNodeNumbersVisibilityAct->setChecked(toggle);

    if (toggle)
    {
        statusMessage(tr("Numbers inside nodes..."));
    }
    else
    {
        statusMessage(tr("Numbers outside nodes..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of node labels.
 * Persists the choice in appSettings["initNodeLabelsVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeLabelsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeLabelsVisibility()" << toggle;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    statusMessage(tr("Toggle Nodes Labels. Please wait..."));

    appSettings["initNodeLabelsVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setNodeLabelsVisibility(toggle);
    optionsNodeLabelsVisibilityAct->setChecked(toggle);

    if (!toggle)
    {
        statusMessage(tr("Node Labels are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Node Labels are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of all edges.
 * Persists the choice in appSettings["initEdgesVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgesVisibility(bool toggle)
{
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges. Please wait..."));
    appSettings["initEdgesVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setAllItemsVisibility(TypeEdge, toggle);
    if (!toggle)
    {
        statusMessage(tr("Edges are invisible now. Click again the same menu to display them."));
    }
    else
    {
        statusMessage(tr("Edges visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of directional arrows on edges.
 * Persists the choice in appSettings["initEdgeArrows"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeArrowsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "Request to toggle edges arrows to:" << toggle;

    statusMessage(tr("Toggling Edges' Arrows. Please wait..."));
    appSettings["initEdgeArrows"] = (toggle) ? "true" : "false";

    graphicsWidget->setEdgeArrowsVisibility(toggle);
    if (toggle)
    {
        statusMessage(tr("Arrows in edges: on."));
    }
    else
    {
        statusMessage(tr("Arrows in edges: off."));
    }
}

/**
 * @brief Applies a new arrow size to all edges and persists the setting.
 * @param size  Arrow size in pixels (2–20).
 */
void MainWindow::slotOptionsEdgeArrowSize(const int &size)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeArrowSize - new size" << size;
    appSettings["initEdgeArrowSize"] = QString::number(size);
    graphicsWidget->setEdgeArrowSize(size);
    statusMessage(tr("Changed edge arrow size to %1.").arg(size));
}

/**
 * @brief Toggles edge weights during computations
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightsDuringComputation(bool toggle)
{
    askedAboutWeights = false;
    askAboutEdgeWeights(toggle);
    activeGraph->setModStatus(activeGraph->ModStatus::EdgeCount);
}

/**
 * @brief Toggles drawing edges as Bezier curves or straight lines.
 * Persists the choice in appSettings["initEdgeShape"] ("bezier" or "line").
 * @param toggle
 */
void MainWindow::slotOptionsEdgesBezier(bool toggle)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage(tr("Toggle edges bezier. Please wait..."));
    appSettings["initEdgeShape"] = toggle ? "bezier" : "line";
    graphicsWidget->setEdgesBezier(toggle);
    statusMessage(tr("Edges drawn as %1.").arg(toggle ? tr("Bezier curves") : tr("straight lines")));
}

/**
 * @brief MainWindow::slotOptionsEdgeThicknessPerWeight
 * @param toggle
 */
void MainWindow::slotOptionsEdgeThicknessPerWeight(bool toggle)
{
    if (toggle)
    {
    }
    else
    {
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
void MainWindow::slotOptionsEdgeOffsetFromNode(const int &offset, const int &v1, const int &v2)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeOffsetFromNode - new offset " << offset;
    int newOffset = offset;

    if (!newOffset)
    {
        newOffset = QInputDialog::getInt(
            this, "Change edge offset",
            tr("Change all edges offset from their nodes to: (1-16)"),
            appSettings["initNodeLabelDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change edge offset aborted."));
            return;
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (v1 && v2)
    { // change one edge offset only
        graphicsWidget->setEdgeOffsetFromNode(v1, v2, newOffset);
    }
    else
    { // change all
        appSettings["initEdgeOffsetFromNode"] = QString::number(newOffset);
        graphicsWidget->setEdgeOffsetFromNode(v1, v2, newOffset);
    }

    QApplication::restoreOverrideCursor();

    statusMessage(tr("Changed edge offset from nodes."));
}

/**
 * @brief Toggles visibility of edge weight numbers.
 * Persists the choice in appSettings["initEdgeWeightNumbersVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightNumbersVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeWeightNumbersVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges Weights. Please wait..."));
    appSettings["initEdgeWeightNumbersVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setEdgeWeightNumbersVisibility(toggle);
    activeGraph->edgeWeightNumbersVisibilitySet(toggle);
    optionsEdgeWeightNumbersAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Edge weights are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Edge weights are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of edge labels.
 * Persists the choice in appSettings["initEdgeLabelsVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeLabelsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeLabelsVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges Labels. Please wait..."));

    appSettings["initEdgeLabelsVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setEdgeLabelsVisibility(toggle);
    activeGraph->edgeLabelsVisibilitySet(toggle);
    optionsEdgeLabelsAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Edge labels are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Edge labels are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns on/off saving zero-edge edge weights (only for GraphML at the moment)
 * @param toggle
 */
void MainWindow::slotOptionsSaveZeroWeightEdges(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsSaveZeroWeightEdges - Toggling saving zero weight edges";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle zero-weight edges saving. Please wait..."));

    appSettings["saveZeroWeightEdges"] = (toggle) ? "true" : "false";

    if (toggle)
    {
        statusMessage(tr("Zero-weight edges will be saved to graphml files. "));
    }
    else
    {
        statusMessage(tr("Zero-weight edges will NOT be saved to graphml files."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns on/off drawing of zero-weight edges on the canvas (#30)
 * @param toggle
 */
void MainWindow::slotOptionsShowZeroWeightEdges(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsShowZeroWeightEdges - toggle:" << toggle;
    appSettings["showZeroWeightEdges"] = (toggle) ? "true" : "false";
    activeGraph->showZeroWeightEdgesSet(toggle);
    statusMessage(toggle
                      ? tr("Zero-weight edges will be drawn on the canvas.")
                      : tr("Zero-weight edges will not be drawn on the canvas."));
}

/**
 * @brief Turns opengl on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasOpenGL(const bool &toggle)
{
    statusMessage(tr("Toggle openGL. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Inform graphicsWidget about the change
    graphicsWidget->setOptionsOpenGL(toggle);

    if (!toggle)
    {
        appSettings["opengl"] = "false";
        statusMessage(tr("Using openGL off."));
    }
    else
    {
        appSettings["opengl"] = "true";
        statusMessage(tr("Using OpenGL on."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns antialiasing on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasing(bool toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage(tr("Toggle anti-aliasing. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    graphicsWidget->setOptionsAntialiasing(toggle);

    if (!toggle)
    {
        appSettings["antialiasing"] = "false";
        statusMessage(tr("Anti-aliasing off."));
    }
    else
    {
        appSettings["antialiasing"] = "true";
        statusMessage(tr("Anti-aliasing on."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns antialiasing auto-adjustment on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasingAutoAdjust(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage(tr("Toggle anti-aliasing auto adjust. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    graphicsWidget->setOptionsNoAntialiasingAutoAdjust(toggle);

    if (!toggle)
    {
        appSettings["canvasAntialiasingAutoAdjustment"] = "false";
        statusMessage(tr("Antialiasing auto-adjustment off."));
    }
    else
    {
        appSettings["canvasAntialiasingAutoAdjustment"] = "true";
        statusMessage(tr("Antialiasing auto-adjustment on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns smooth pixmap transformations on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSmoothPixmapTransform(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasSmoothPixmapTransform() " << toggle;

    statusMessage(tr("Toggle smooth pixmap transformations. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "false";
        statusMessage(tr("Smooth pixmap transformations off."));
    }
    else
    {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "true";
        statusMessage(tr("Smooth pixmap transformations on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns saving painter state on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSavePainterState(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasSavePainterState() " << toggle;

    statusMessage(tr("Toggle saving painter state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
        appSettings["canvasPainterStateSave"] = "false";
        statusMessage(tr("Saving painter state off."));
    }
    else
    {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
        appSettings["canvasPainterStateSave"] = "true";
        statusMessage(tr("Saving painter state on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns caching of canvas background on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasCacheBackground(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasCacheBackground() " << toggle;

    statusMessage(tr("Toggle canvas background caching state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheNone);
        appSettings["canvasCacheBackground"] = "false";
        statusMessage(tr("Canvas background caching  off."));
    }
    else
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        appSettings["canvasCacheBackground"] = "true";
        statusMessage(tr("Canvas background caching  on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns selected edge highlighting
 * @param toggle
 */
void MainWindow::slotOptionsCanvasEdgeHighlighting(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasEdgeHighlighting() " << toggle;

    statusMessage(tr("Toggle edge highlighting state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "false";
        statusMessage(tr("Edge highlighting off."));
    }
    else
    {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "true";
        statusMessage(tr("Edge highlighting on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Sets canvas update mode
 * @param toggle
 */
void MainWindow::slotOptionsCanvasUpdateMode(const QString &mode)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasUpdateMode() " << mode;

    statusMessage(tr("Setting canvas update mode. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (mode == "Full")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
    else if (mode == "Minimal")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }
    else if (mode == "Smart")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    }
    else if (mode == "Bounding Rectangle")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    }
    else if (mode == "None")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }
    else
    { //
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }

    appSettings["canvasUpdateMode"] = mode;

    statusMessage(tr("Canvas update mode: ") + mode);

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Changes the indexing method of the graphics scene.
 *
 * Called from Settings dialog.
 *
 * @param method
 */
void MainWindow::slotOptionsCanvasIndexMethod(const QString &method)
{

    qCDebug(lcMainWindow) << "Changing graphics scene index method to:" << method;

    statusMessage(tr("Setting canvas index method. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (method == "BspTreeIndex")
    { // Qt default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }
    else if (method == "NoIndex")
    { // for animated scenes
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    }
    else
    { // default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }

    appSettings["canvasIndexMethod"] = method;

    statusMessage(tr("Canvas index method: ") + method);

    QApplication::restoreOverrideCursor();
}

/**
 * @brief MainWindow::slotOptionsEmbedLogoExporting
 *
 * @param toggle
 */
void MainWindow::slotOptionsEmbedLogoExporting(bool toggle)
{
    if (!toggle)
    {
        statusMessage(tr("SocNetV logo print off."));
        appSettings["printLogo"] = "false";
    }
    else
    {
        appSettings["printLogo"] = "true";
        statusMessage(tr("SocNetV logo print on."));
    }
}

/**
 * @brief Turns progress dialogs on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsProgressDialogVisibility(bool toggle)
{
    statusMessage(tr("Toggle progressbar..."));
    if (!toggle)
    {
        appSettings["showProgressBar"] = "false";
        statusMessage(tr("Progress bars off."));
    }
    else
    {
        appSettings["showProgressBar"] = "true";
        statusMessage(tr("Progress bars on."));
    }
}

/**
 * @brief
 * Turns debugging messages on or off
 * @param toggle
 */
void MainWindow::slotOptionsDebugMessages(bool toggle)
{
    if (!toggle)
    {
        qCDebug(lcMainWindow) << "Disabling debugging messages";
        appSettings["printDebug"] = "false";
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                         "socnetv.*.debug=false");
        statusMessage(tr("Debug messages off."));
    }
    else
    {
        appSettings["printDebug"] = "true";
        QLoggingCategory::setFilterRules("default.debug=true\n"
                                         "socnetv.*.debug=true");
        qCDebug(lcMainWindow) << "Enabled debugging messages";
        statusMessage(tr("Debug messages on."));
    }
}

/**
 * @brief
 * Called from Options menu and Settings dialog
 * @param color QColor
 */
void MainWindow::slotOptionsBackgroundColor(QColor color)
{

    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initBackgroundColor"]),
                                       this,
                                       "Change the background color");
    }
    if (color.isValid())
    {
        appSettings["initBackgroundColor"] = color.name();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Background changed."));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
    }
}

/**
 * @brief Toggles a custom background image on the canvas.
 * When enabled, opens a file dialog to pick the image.
 * Persists the path in appSettings["initBackgroundImage"].
 * @param toggle
 */
void MainWindow::slotOptionsBackgroundImageSelect(bool toggle)
{
    statusMessage(tr("Toggle BackgroundImage..."));
    QString m_fileName;
    if (toggle == false)
    {
        statusMessage(tr("BackgroundImage off."));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
    }
    else
    {
        m_fileName = QFileDialog::getOpenFileName(
            this, tr("Select one image"), getLastPath(),
            tr("Images (*.png *.jpg *.jpeg);;All (*.*)"));
        if (m_fileName.isNull())
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
void MainWindow::slotOptionsBackgroundImage()
{
    statusMessage(tr("Toggle BackgroundImage..."));
    if (appSettings["initBackgroundImage"].isEmpty())
    {
        statusMessage(tr("BackgroundImage off."));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
    }
    else
    {
        setLastPath(appSettings["initBackgroundImage"]);
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage(tr("BackgroundImage on."));
    }
}

/**
 * @brief Toggles full screen mode (F11)
 * @param toggle
 */
void MainWindow::slotOptionsWindowFullScreen(bool toggle)
{
    if (toggle == false)
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage(tr("Full screen mode off. Press F11 again to enter full screen."));
    }
    else
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage(tr("Full screen mode on. Press F11 again to exit full screen."));
    }
}

/**
 * @brief Turns Toolbar on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsWindowToolbarVisibility(bool toggle)
{
    statusMessage(tr("Toggle toolbar..."));
    if (toggle == false)
    {
        toolBar->hide();
        appSettings["showToolBar"] = "false";
        statusMessage(tr("Toolbar off."));
    }
    else
    {
        toolBar->show();
        appSettings["showToolBar"] = "true";
        statusMessage(tr("Toolbar on."));
    }
}

/**
 * @brief Turns window statusbar on or off
 * @param toggle
 */
void MainWindow::slotOptionsWindowStatusbarVisibility(bool toggle)
{
    statusMessage(tr("Toggle statusbar..."));

    if (toggle == false)
    {
        statusBar()->hide();
        appSettings["showStatusBar"] = "false";
        statusMessage(tr("Status bar off."));
    }
    else
    {
        statusBar()->show();
        appSettings["showStatusBar"] = "true";
        statusMessage(tr("Status bar on."));
    }
}

/**
 * @brief Toggles left panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowLeftPanelVisibility(bool toggle)
{
    statusMessage(tr("Toggle left panel..."));

    if (toggle == false)
    {
        m_leftScrollArea->hide();
        appSettings["showLeftPanel"] = "false";
        statusMessage(tr("Left Panel off."));
    }
    else
    {
        m_leftScrollArea->show();
        appSettings["showLeftPanel"] = "true";
        statusMessage(tr("Left Panel on."));
    }
}

/**
 * @brief Toggles right panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowRightPanelVisibility(bool toggle)
{
    statusMessage(tr("Toggle left panel..."));

    if (toggle == false)
    {
        rightPanel->hide();
        appSettings["showRightPanel"] = "false";
        statusMessage(tr("Right Panel off."));
    }
    else
    {
        rightPanel->show();
        appSettings["showRightPanel"] = "true";
        statusMessage(tr("Right Panel on."));
    }
}

/**
 * @brief Shows or hides the Data Table dock panel (#225).
 *
 * When shown, the node and edge models are immediately refreshed from the
 * current graph state.
 *
 * @param checked  true → show panel; false → hide panel.
 */
void MainWindow::slotViewDataTable(bool checked)
{
    if (checked)
    {
        m_tableDock->show();
        m_tableWidget->refresh(activeGraph);
    }
    else
    {
        m_tableDock->hide();
    }
}

/**
 * @brief Toggles the use of our own Qt StyleSheet
 *
 * The .qss file is defined in project resources
 *
 * @param checked
 */
void MainWindow::slotOptionsCustomStylesheet(const bool checked = true)
{
    if (checked)
    {
        slotStyleSheetByName(":/qss/default.qss");
        appSettings["useCustomStyleSheet"] = "true";
    }
    else
    {
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
void MainWindow::slotStyleSheetByName(const QString &sheetFileName)
{

    qCDebug(lcMainWindow) << "Opening stylesheet file: " << sheetFileName;

    QString styleSheet = "";

    if (!sheetFileName.isEmpty())
    {

        QFile file(sheetFileName);

        if (!file.open(QFile::ReadOnly))
        {
            qCDebug(lcMainWindow) << "Could not open (for reading) file:" << sheetFileName;
            slotHelpMessageToUserError(
                tr("Cannot read stylesheet file %1:\n%2")
                    .arg(sheetFileName)
                    .arg(file.errorString()));
            return;
        }
        styleSheet = QString::fromLatin1(file.readAll());
    }
    qApp->setStyleSheet(styleSheet);
}

/**
 *  Displays a random tip
 */
void MainWindow::slotHelpTips()
{
    int randomTip = rand() % (tips.size()); // Pick a tip.
    QMessageBox::about(this, tr("Tip Of The Day"), tips[randomTip]);
}

/**
    Creates our tips.
*/
void MainWindow::slotHelpCreateTips()
{
    tips += tr("To create a new node: \n"
               "- double-click somewhere on the canvas \n"
               "- or press the keyboard shortcut CTRL+. (dot)\n"
               "- or press the Add Node button on the left panel");
    tips += tr("SocNetV can work with either undirected or directed data. "
               "When you start SocNetV for the first time, the application uses "
               "the 'directed data' mode; every edge you create is directed. "
               "To enter the 'undirected data' mode, press CTRL+E+U or enable the "
               "menu option Edit->Edges->Undirected Edges ");
    tips += tr("If your screen is small, and the canvas appears even smaller "
               "hide the Control and/or Statistics panel. Then the canvas "
               "will expand to the whole application window. "
               "Open the Settings/Preferences dialog->Window options and "
               "disable the two panels.");
    tips += tr("A scale-free network is a network whose degree distribution follows a power law. "
               "SocNetV generates random scale-free networks according to the "
               "Barabási–Albert (BA) model using a preferential attachment mechanism.");
    tips += tr("To delete a node permanently: \n"
               "- right-click on it and select Remove Node \n"
               "- or press CTRL+ALT+. and enter its number\n"
               "- or press the Remove Node button on the Control Panel");
    tips += tr("To rotate the network: \n"
               " - drag the bottom slider to left or right \n"
               " - or click the buttons on the corners of the bottom slider\n"
               " - or press CTRL and the left or right arrow.");
    tips += tr("To create a new edge between nodes A and B: \n"
               "- double-click on node A, then double-click on node B.\n"
               "- or middle-click on node A, and again on node B.\n"
               "- or right-click on the node, then select Add Edge from the popup.\n"
               "- or press the keyboard shortcut CTRL+/ \n"
               "- or press the Add Edge button on the Control Panel");
    tips += tr("Add a label to an edge by right-clicking on it "
               "and selecting Change Label.");
    tips += tr("You can change the background color of the canvas. "
               "Do it from the menu Options > View or "
               "permanently save this setting in Settings/Preferences.");
    tips += tr("Default node colors, shapes and sizes can be changed. "
               "Open the Settings/Preferences dialog and use the "
               "options on the Node tab.");
    tips += tr("The Statistics Panel shows network-level information (i.e. density) "
               "as well as info about any node you clicked on (inDegrees, "
               "outDegrees, clustering).");
    tips += tr("You can move any node by left-clicking and dragging it with your mouse. "
               "If you want you can move multiple nodes at once. Left-click on empty space "
               "on the canvas and drag to create a rectangle selection around them. "
               "Then left-click on one of the selected nodes and drag it.");
    tips += tr("To save the node positions in a network, you need to save your data "
               "in a format which supports node positions, suchs as GraphML or Pajek.");
    tips += tr("Embed visualization models on the network from the options in "
               "the Layout menu or the select boxes on the left Control Panel. ");
    tips += tr("To change the label of a node right-click on it, and click "
               "Selected Node Properties from the popup menu.");
    tips += tr("All basic operations of SocNetV are available from the left Control panel "
               "or by right-clicking on a Node or an Edge or on canvas empty space.");
    tips += tr("Node info (number, position, degree, etc) is displayed on the Status bar, "
               "when you left-click on it.");
    tips += tr("Edge information is displayed on the Status bar, when you left-click on it.");
    tips += tr("Save your work often, especially when working with large data sets. "
               "SocNetV alogorithms are faster when working with saved data. ");

    tips += tr("You can change the precision of real numbers in reports.  "
               "Go to Settings > General and change it under Reports > Real number precision. ");

    tips += tr("The Closeness Centrality (CC) of a node v, is the inverse sum of "
               "the shortest distances between v and every other node. CC is "
               "interpreted as the ability to access information through the "
               "\'grapevine\' of network members. Nodes with high closeness "
               "centrality are those who can reach many other nodes in few steps. "
               "This index can be calculated in both graphs and digraphs. "
               "It can also be calculated in weighted graphs although the weight of "
               "each edge (v,u) in E is always considered to be 1. ");

    tips += tr("The Information Centrality (IC) index counts all paths between "
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
void MainWindow::slotHelp()
{
    statusMessage(tr("Opening the SocNetV Manual in your default web browser...."));
    QDesktopServices::openUrl(QUrl("https://socnetv.org/manual/?utm_source=application&utm_medium=banner&utm_campaign=socnetv" + VERSION));
}

/**
 * @brief On user demand, makes a network request to SocNetV website to
 * download the latest version text file.
 */
void MainWindow::slotHelpCheckUpdateDialog()
{

    QString url = "https://socnetv.org/latestversion.txt";

    qCDebug(lcMainWindow) << "Will make a 'check for updates' request to url:" << url;

    slotNetworkManagerRequest(QUrl(url), NetworkRequestType::CheckUpdate);
}

/**
 * @brief Compares two version strings component-by-component.
 *
 * Handles versions with 1, 2, or 3 components (e.g. "3.3", "3.3.1", "3.10").
 * Returns:
 *   -1 if a < b
 *    0 if a == b
 *   +1 if a > b
 */
static int compareVersions(const QString &a, const QString &b)
{
    const QStringList aParts = a.split('.');
    const QStringList bParts = b.split('.');
    const int len = qMax(aParts.size(), bParts.size());
    for (int i = 0; i < len; ++i)
    {
        const int av = (i < aParts.size()) ? aParts[i].toInt() : 0;
        const int bv = (i < bParts.size()) ? bParts[i].toInt() : 0;
        if (av < bv)
            return -1;
        if (av > bv)
            return +1;
    }
    return 0;
}

/**
 * @brief Parses the reply from the network request we do in slotHelpCheckUpdateDialog
 */
void MainWindow::slotHelpCheckUpdateParse()
{
    qCDebug(lcMainWindow) << "MW::slotHelpCheckUpdateParse()";

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QByteArray ba = reply->readAll();
    reply->deleteLater();

    QString replyContent = QString(ba).simplified();

    if (replyContent.isEmpty())
    {
        slotHelpMessageToUserError(
            "Empty response from https://socnetv.org. "
            "Could not get the latest version number. Please try again later...");
        return;
    }

    // Validate: remote version must look like digits and dots only
    static const QRegularExpression versionRx(R"(^\d+(\.\d+){0,2}$)");
    if (!versionRx.match(replyContent).hasMatch())
    {
        slotHelpMessageToUserError(
            "Could not understand the version number I got from https://socnetv.org. "
            "Please, try again later...");
        return;
    }

    const QString remoteVersion = replyContent;

    // Strip pre-release suffixes from local version (beta, rc, dev)
    QString localVersion = VERSION;
    static const QRegularExpression preReleaseSuffixRx(R"([-.]?(beta|rc|dev)\d*)", QRegularExpression::CaseInsensitiveOption);
    localVersion.remove(preReleaseSuffixRx);

    qCDebug(lcMainWindow) << "MW::slotHelpCheckUpdateParse() - localVersion:" << localVersion
             << "remoteVersion:" << remoteVersion;

    const int cmp = compareVersions(remoteVersion, localVersion);

    if (cmp > 0)
    {
        // Remote is newer
        switch (slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Newer SocNetV version available!"),
            tr("<p>Your version: ") + VERSION + "</p>" +
                tr("<p>Remote version: <b>") + remoteVersion + "</b></p>",
            tr("<p><b>There is a newer SocNetV version available!</b></p>"
               "<p>Do you want to download the latest version now?</p>"
               "<p>Press Yes, and I will open your default web browser for you "
               "to download the latest SocNetV package...</p>"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
        {
        case QMessageBox::Yes:
            statusMessage(tr("Opening SocNetV website in your default web browser...."));
            QDesktopServices::openUrl(QUrl(
                "https://socnetv.org/downloads"
                "?utm_source=application&utm_medium=banner&utm_campaign=socnetv" +
                VERSION));
            break;
        default:
            break;
        }
    }
    else
    {
        // Up to date (cmp == 0) or somehow ahead (cmp < 0, e.g. on a dev build)
        slotHelpMessageToUserInfo(
            tr("<p>Your version: ") + VERSION + "</p>" +
            tr("<p>Remote version: ") + remoteVersion + "</p>" +
            tr("<p>You are running the latest and greatest version of SocNetV.<br/>"
               "Nothing to do!</p>"));
    }
}

/**
 * @brief Shows a dialog with system information for bug reporting purposes
 */
void MainWindow::slotHelpSystemInfo()
{
    qCDebug(lcMainWindow) << "MW: slotHelpSystemInfo()";

    m_systemInfoDialog = new DialogSystemInfo(this);

    m_systemInfoDialog->exec();
}

/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout()
{
    int randomCookie = rand() % fortuneCookie.size();

    QMessageBox::about(
        this, tr("About SocNetV"),
        tr("<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)") +
            tr("<p><b>Version</b>: ") + VERSION + "</p>" +

            tr("<p>Website: <a href=\"https://socnetv.org\">https://socnetv.org</a></p>") +

            tr("<p>(C) 2005-2026 by Dimitris B. Kalamaras</p>") +
            tr("<p><a href=\"https://socnetv.org/contact\">Have questions? Contact us!</a></p>") +

            tr("<p><b>Fortune cookie: </b><br> \"") + fortuneCookie[randomCookie] + "\"" +

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
void MainWindow::createFortuneCookies()
{
    fortuneCookie += "sic itur ad astra / sic transit gloria mundi ? <br /> "
                     "--Unknown";
    fortuneCookie += "The truth is not my business. I am a statistician... I don’t like words like \"correct\" and \"truth\". "
                     "Statistics is about measuring against convention. <br /> "
                     "Walter Radermacher, Eurostat director, interview to NY Times, 2012.";
    fortuneCookie += "Losers of yesterday, the winners of tomorrow... <br /> "
                     "--B.Brecht";
    fortuneCookie += "I've seen things you people wouldn't believe. Attack ships on fire off the shoulder of Orion. "
                     "I watched C-beams glitter in the dark near the Tannhauser gate. "
                     "All those moments will be lost in time... like tears in rain... Time to die.<br />"
                     "Replicant Roy Batty, Blade Runner (1982)";
    fortuneCookie += "Patriotism is the virtue of the wicked... <br /> "
                     "--O. Wilde";
    fortuneCookie += "No tengo nunca mas, no tengo siempre. En la arena <br />"
                     "la victoria dejo sus piers perdidos.<br />"
                     "Soy un pobre hombre dispuesto a amar a sus semejantes.<br />"
                     "No se quien eres. Te amo. No doy, no vendo espinas. <br /> "
                     "--Pablo Neruda";
    fortuneCookie += "Man must not check reason by tradition, but contrawise, "
                     "must check tradition by reason.<br> --Leo Tolstoy";
    fortuneCookie += "Only after the last tree has been cut down, <br>"
                     "only after the last river has been poisoned,<br> "
                     "only after the last fish has been caught,<br>"
                     "only then will you realize that money cannot be eaten. <br> "
                     "--The Cree People";
    fortuneCookie += "Stat rosa pristina nomine, nomina nuda tenemus <br >"
                     " --Unknown";
    fortuneCookie += "Jupiter and Saturn, Oberon, Miranda <br />"
                     "And Titania, Neptune, Titan. <br />"
                     "Stars can frighten. <br /> Syd Barrett";
    fortuneCookie += "In theory, there is no difference between theory and practice. <br /> "
                     "In practice, there is. <br /> --Yogi Berra";
}

/**
    Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt()
{
    QMessageBox::aboutQt(this, "About Qt - SocNetV");
}
