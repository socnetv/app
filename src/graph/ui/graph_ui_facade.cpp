/**
 * @file graph_ui_facade.cpp
 * @brief Implements Façade wrapper methods called by the UI

 * @author Dimitris B. Kalamaras
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

#include "graph.h"
#include <QThread>
#include <QCoreApplication>
#include <QMetaObject>

QThread *Graph::getThread() const
{
    return thread();
}

void Graph::moveToThreadFacade(QThread *t)
{
    moveToThread(t);
}

/**
 * @brief Runs @p fn on the application's main/GUI thread, regardless of the calling thread.
 *
 * Exists because `Graph`'s own thread affinity is graphThread (see moveToThreadFacade()), but
 * some UI façade code (currently: the prominence-distribution chart builders in
 * graph_ui_prominence_distribution.cpp) constructs real Qt GUI objects — QtCharts series/axes,
 * QChartView::grab() for PNG export — which is only ever safe on the main/GUI thread, per Qt's
 * single-threaded-GUI rule. Those objects would otherwise inherit whichever thread happens to be
 * calling in, which is graphThread whenever this is reached from algorithm-slice code (layouts,
 * report generation, etc.) that now genuinely executes there (see #254).
 *
 * Uses QCoreApplication::instance() rather than the qApp macro (which force-casts to
 * QApplication*) so this stays correct in the headless CLI, where only a QCoreApplication is
 * constructed - never a QApplication. Qt::AutoConnection (the default, used explicitly here for
 * clarity) resolves to a direct, synchronous call when already on the target thread - which is
 * always true for the CLI, since it never creates a second thread - so this has no behavioural
 * or performance effect there. In GUI mode, called from graphThread, it resolves to queued
 * delivery on the next GUI event loop iteration.
 *
 * @param fn The work to run on the main/GUI thread.
 */
void Graph::runOnGuiThread(std::function<void()> fn)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), fn, Qt::AutoConnection);
}

// INTERNAL-FACADE HELPERS
/**
 * @brief Emits a status message to be shown in the UI status bar.
 * @param msg The message to be shown.
 */
void Graph::progressStatus(const QString &msg)
{
    emit statusMessage(msg);
}
/**
 * @brief Resets the cancellation status
 */
void Graph::resetProgressCanceled()
{
    qCDebug(lcGraphUI) << "Graph::resetProgressCanceled() - resetting flag";
    m_progressCanceled = false;
}
/**
 * @brief Returns true if the user has requested cancellation via the progress dialog.
 */
bool Graph::progressCanceled() const
{
    return m_progressCanceled;
}

/**
 * @brief Slot called by MainWindow when the user clicks Cancel in the progress dialog.
 */
void Graph::slotCancelComputation()
{
    qCDebug(lcGraphUI) << "Graph::slotCancelComputation() - setting flag from thread:" << QThread::currentThreadId();
    m_progressCanceled = true;
}
