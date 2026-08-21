/**
 * @file mainwindow_network_view.cpp
 * @brief Implements MainWindow Network menu: file preview in system browser, internal text editor, sociomatrix view, and the Network toolbox auto-create combo box.
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
#include "webcrawler.h"
#include "forms/dialogpreviewfile.h"
#include "forms/dialogdatasetselect.h"
#include "forms/dialogranderdosrenyi.h"
#include "forms/dialograndsmallworld.h"
#include "forms/dialograndscalefree.h"
#include "forms/dialograndregular.h"
#include "forms/dialograndlattice.h"
#include "forms/dialogwebcrawler.h"
#include "forms/dialogexportimage.h"
#include "forms/dialogexportpdf.h"

#include <QtWidgets>
#include <QTextCodec>
#include <QNetworkReply>
#include <QNetworkAccessManager>

/**
 * @brief Called when user selects something in the Network Auto Create
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxNetworkAutoCreateSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected net auto create, index: " << selectedIndex;
    switch (selectedIndex)
    {
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

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Displays the file of the loaded network.
 *
 * If the network has been modified, it prompts the user
 * to save the network, then view its file.
 */
void MainWindow::slotNetworkFileView()
{

    qCDebug(lcMainWindow) << "Request to display current network file. Filename:" << fileName.toLatin1()
             << "isLoaded:" << activeGraph->isLoaded()
             << "isSaved:" << activeGraph->isSaved()
             << "graph filename:" << activeGraph->getFileName();

    if (activeGraph->isLoaded() && activeGraph->isSaved())
    {
        // network unmodified, read loaded file again.
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qCDebug(lcMainWindow, "Error in open!");
            return;
        }
        TextEditor *ed = new TextEditor(fileName, this, false);
        QFileInfo fileInfo(fileName);
        fileNameNoPath = fileInfo.fileName();
        ed->setWindowTitle(fileNameNoPath);
        ed->show();
        m_textEditors << ed;
        statusMessage(tr("Displaying network data file %1").arg(fileNameNoPath));
    }

    else if (!activeGraph->isSaved())
    {

        if (!activeGraph->isLoaded())
        {
            // new network, not saved yet
            int response = slotHelpMessageToUser(
                USER_MSG_QUESTION,
                tr("New network not saved yet. You might want to save it first."),
                tr("This new network you created has not been saved yet."),
                tr("Do you want to open a file dialog to save your work "
                   "(then I will display the file)?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (response == QMessageBox::Yes)
            {
                slotNetworkSaveAs();
            }
            else
            {
                return;
            }
        }
        else
        {
            // loaded network, but modified
            int response = slotHelpMessageToUser(
                USER_MSG_QUESTION,
                tr("Current network has been modified. Save to the original file?"),
                tr("Current social network has been modified since last save."),
                tr("Do you want to save it to the original file?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (response == QMessageBox::Yes)
            {
                slotNetworkSave();
            }
            else if (response == QMessageBox::No)
            {
                slotNetworkSaveAs();
            }
            else
            { // user pressed Cancel
                return;
            }
        }
        slotNetworkFileView();
    }
    else
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
    }
}

/**
 * @brief Opens the embedded text editor
 */
void MainWindow::slotNetworkTextEditor()
{
    qCDebug(lcMainWindow) << "slotNetworkTextEditor() : ";

    TextEditor *ed = new TextEditor("", this, false);
    ed->setWindowTitle(tr("New Network File"));
    ed->show();
    m_textEditors << ed;
    statusMessage(tr("Enter your network data here"));
}

/**
 * @brief Displays the adjacency matrix of the network.
 *
 *  It uses a different method for writing the matrix to a file.
 *  While slotNetworkExportSM uses << operator of Matrix class
 *  (via adjacencyMatrix of Graph class), this is using directly the
 *  writeMatrixAdjacency method of Graph class.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 *  CSV output always opens via the system's default handler (QDesktopServices::openUrl),
 *  regardless of the "view reports in system browser" setting - the internal TextEditor's
 *  plain-text mode isn't a useful CSV view.
 */
void MainWindow::slotNetworkViewSociomatrix()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-" + dateTime + ext;

    qCDebug(lcMainWindow) << "MW::slotNetworkViewSociomatrix() - dataDir"
             << appSettings["dataDir"]
             << "fn" << fn;

    // AVOID activeGraph->writeMatrix(fn,MATRIX_ADJACENCY), no preserving of node numbers
    // when nodes are deleted.
    auto success = std::make_shared<bool>(false);
    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrixAdjacency(fn, true, reportFormat);
        },
        tr("Creating and writing adjacency matrix"),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {

                qCDebug(lcMainWindow) << "MW::slotNetworkViewSociomatrix() - "
                            "calling QDesktopServices::openUrl for"
                         << QUrl::fromLocalFile(fn);

                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays a text-only plot of the network adjacency matrix
 */
void MainWindow::slotNetworkViewSociomatrixPlotText()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int N = activeNodes();

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-plot-" + dateTime + ".html";

    bool simpler = false;
    if (N > 999)
    {
        qreal MB = (N * N * 10) / (1024 * 1024);
        switch (slotHelpMessageToUser(
            USER_MSG_QUESTION, tr("Very large network to plot!"),
            tr("Warning: Really large network"),
            tr("To plot a %1 x %1 matrix arranged in HTML table, "
               "I will need time to write a very large .html file , circa %2 MB in size. "
               "Instead, I can create a simpler / smaller HTML file without table. "
               "Press Yes to continue with simpler version, "
               "Press No to create large file with HTML table.")
                .arg(N)
                .arg(MB)))
        {
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

    runGraphOperationAsync(
        [this, fn, simpler]() {
            activeGraph->writeMatrixAdjacencyPlot(fn, simpler);
        },
        tr("Creating plot of adjacency matrix of %1 nodes.").arg(N),
        [this, fn]() {
            if (activeGraph->progressCanceled())
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Visual form of adjacency matrix saved as ") + QDir::toNativeSeparators(fn));
        });
}
