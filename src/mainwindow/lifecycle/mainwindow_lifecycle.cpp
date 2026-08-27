/**
 * @file mainwindow_lifecycle.cpp
 * @brief Implements MainWindow's window lifecycle handlers: close event, resize event, and
 *        thread teardown.
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
#include "texteditor.h"

#include <QCloseEvent>
#include <QResizeEvent>

/**
 * @brief Called when the application closes. Asks to write any unsaved network data.
 * @param ce
 */
void MainWindow::closeEvent(QCloseEvent *ce)
{

    //
    // Show a status message
    //
    qCDebug(lcMainWindow) << "Received close event. Show a status message to user...";
    statusMessage(tr("Closing SocNetV. Bye!"));

    //
    // Check if the graph has been saved
    //
    bool userCancelled = false;
    qCDebug(lcMainWindow) << "Checking if current graph is saved...";
    if (m_interactiveScriptQuitting)
    {
        // 'quit' interactive-script command: no one is present to answer the save-changes
        // prompt below, so skip it entirely - same "bypass modal dialogs that would block a
        // scripted run" precedent already used by 'erdos'/'add-node'/etc.
        ce->accept();
        qCDebug(lcMainWindow) << "Interactive script quit - skipping save-changes prompt.";
    }
    else if (activeGraph->isSaved())
    {
        ce->accept();
        qCDebug(lcMainWindow) << "Graph is already saved. Nothing to do.";
    }
    else
    {
        qCDebug(lcMainWindow) << "Graph NOT saved. Asking the user what to do.";
        switch (slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Save changes"),
            tr("Modified network has not been saved!"),
            tr("Do you want to save the changes to the network file?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel))
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
    if (userCancelled)
    {
        qCDebug(lcMainWindow) << "User canceled (while saving graph). Returning without closing the app.";
        return;
    }

    //
    // Terminate running threads
    //
    qCDebug(lcMainWindow) << "I will terminate any running threads...";
    terminateThreads("closeEvent()");

    //
    // Delete other objects and pointers
    //
    qCDebug(lcMainWindow) << "Deleting other objects and pointers...";

    qCDebug(lcMainWindow) << "Deleting printer";
    delete printer;
    qCDebug(lcMainWindow) << "Deleting printerPDF";
    delete printerPDF;
    // graphicsWidget is NOT explicitly deleted here (WS7 MW0 finding, confirmed via lldb/ASan):
    // Qt's own window-activation bookkeeping (QApplicationPrivate::setActiveWindow, run later
    // during this same close sequence, after closeEvent() returns) still holds a live reference
    // to it at this point and dereferences it - deleting it here left that reference dangling and
    // crashed. `this` (MainWindow) is never explicitly destructed either (see main.cpp - heap
    // allocated, no matching delete, process just exits), so graphicsWidget - a `this`-parented
    // QObject - was never going to be cleanly destructed via ~MainWindow() anyway; the OS reclaims
    // it at process exit the same way it already reclaims MainWindow itself.
    qCDebug(lcMainWindow) << "Deleting activeGraph";
    delete activeGraph;
    // scene is likewise NOT explicitly deleted here (same WS7 MW0 finding): graphicsWidget (a
    // QGraphicsView, still alive per above) holds a live, non-owning reference to it
    // (QGraphicsView(sc, m_parent) does not reparent/take ownership - graphicsWidget.cpp), and
    // dereferences it while handling a viewport event during this same close sequence. scene has
    // no Qt parent (`new QGraphicsScene()`), so the app is nominally responsible for deleting it -
    // but since the process exits immediately once this close sequence finishes, leaving it for
    // the OS to reclaim at exit is no different in practice from deleting it here, and avoids the
    // dangling-reference crash.

    //    delete miniChart;

    qCDebug(lcMainWindow) << "Clearing and deleting text editors...";
    foreach (TextEditor *ed, m_textEditors)
    {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();

    qCDebug(lcMainWindow) << " Checking if networkManager thread is running...";
    if (networkManager->thread()->isRunning())
    {
        qCDebug(lcMainWindow) << "networkManager thread running"
                 << "Calling deleteLater();";
        networkManager->deleteLater();
    }

    // Not explicitly deleted: both are constructed with `this` as their QObject parent
    // (mainwindow.cpp), so normal Qt parent-child teardown already destroys them when
    // MainWindow itself is destroyed. An explicit delete() here crashed - the action is still
    // attached to a live QToolBar at this point in closeEvent, and ~QAction() reaching into
    // QToolBar::actionEvent()/removeAction() to detach itself segfaulted.

    qCDebug(lcMainWindow) << "Clearing codecs...";
    codecs.clear();

    qCDebug(lcMainWindow) << "Finished. Bye!";
}

/**
 * @brief Terminates any remaining threads.
 *
 * @param reason
 */
void MainWindow::terminateThreads(const QString &reason)
{
    qCDebug(lcMainWindow) << "Terminating threads (those started from MW). Reason:" << reason
             << " Checking if graphThread is running...";
    if (graphThread.isRunning())
    {
        qCDebug(lcMainWindow) << "graphThread running."
                 << "Calling graphThread.quit();";
        graphThread.quit();
        qCDebug(lcMainWindow) << "deleting activeGraph and pointer";
        delete activeGraph;
        activeGraph = 0; // see why here: https://goo.gl/tQxpGA
    }
}

/**
 * @brief Called whenever the app window is resized.
 */
void MainWindow::resizeEvent(QResizeEvent *e)
{

    Q_UNUSED(e);
    //    int w0=e->oldSize().width();
    //    int h0=e->oldSize().height();
    //    int w=width();
    //    int h=height();


    //    statusMessage(
    //                 tr("Window resized to (%1, %2)px.")
    //                .arg(w).arg(h)
    //                );
}
