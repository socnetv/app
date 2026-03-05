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

QThread *Graph::getThread() const
{
    return thread();
}

void Graph::moveToThreadFacade(QThread *t)
{
    moveToThread(t);
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
void Graph::resetProgressCanceled() {
    m_progressCanceled = false;
}
/**
 * @brief Emits a signal to create a progress box in the UI with the given maximum value and message.
 * @param max The maximum value for the progress box.
 * @param msg The message to be shown in the progress box.
 */
void Graph::progressCreate(int max, const QString &msg)
{
    resetProgressCanceled();
    emit signalProgressBoxCreate(max, msg);
}
/**
 * @brief Emits a signal to update the progress box in the UI with the given value.
 * @param value The current value to update the progress box with.
 */
void Graph::progressUpdate(int value)
{
    emit signalProgressBoxUpdate(value);
}
/**
 * @brief Emits a signal to kill the progress box in the UI, indicating that the operation is complete.
 */
void Graph::progressFinish()
{
    emit signalProgressBoxKill();
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
    qDebug() << "Graph::slotCancelComputation() - user canceled computation";
    m_progressCanceled = true;
}
