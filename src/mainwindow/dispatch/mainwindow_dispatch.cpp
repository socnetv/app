/**
 * @file mainwindow_dispatch.cpp
 * @brief Implements MainWindow async-dispatch/busy-guard helpers (WS15): status messages, progress dialog polish, busy state, runGraphOperationAsync, and the generator confirmation-size prompt.
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
