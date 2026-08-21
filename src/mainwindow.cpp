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
*	Transforms all nodes to edges
    TODO slotEditTransformNodes2Edges
*/


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
 *	Displays a box informing the user about the symmetry or not of the adjacency matrix
 */



































/**
 *	Writes Betweenness Centralities into a file, then displays it.
 */










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
