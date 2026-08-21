/**
 * @file mainwindow_interactive_script.cpp
 * @brief Implements MainWindow::runInteractiveScript() and processNextInteractiveCommand() - the WS12 CLI scripting mode command dispatcher.
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

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "texteditor.h"
#include "forms/dialogsettings.h"

#include <QtWidgets>

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
