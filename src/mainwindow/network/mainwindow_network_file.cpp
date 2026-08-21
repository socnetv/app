/**
 * @file mainwindow_network_file.cpp
 * @brief Implements MainWindow Network menu: new/open/save/close/print, recent files, file dialog plumbing, and load/save status reactions.
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
#include "widgets/graphtablewidget.h"
#include "graph/io/table_export.h"

#include <QtWidgets>
#include <QTextCodec>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QPrintDialog>

/**
 * @brief Updates the Recent Files QActions in the menu
 */
void MainWindow::slotNetworkFileRecentUpdateActions()
{

    int numRecentFiles = qMin(recentFiles.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1  %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(recentFiles[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    // separatorAct->setVisible(numRecentFiles > 0);
}

/**
 * @brief Starts a new network (closing the current one).
 */
void MainWindow::slotNetworkNew()
{
    slotNetworkClose();
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
                                       const bool &checkSelectFileType)
{
    qCDebug(lcMainWindow) << " m_fileName: " << m_fileName
             << " fileFormat " << fileFormat
             << " checkSelectFileType " << checkSelectFileType;

    previous_fileName = fileName;
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
    if (m_fileName.isNull() || m_fileName.isEmpty())
    {

        fileType = fileFormat;

        // prepare supported filetype extensions
        switch (fileType)
        {
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
        default: // All
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
        // prepare the filedialog
        QFileDialog *fileDialog = new QFileDialog(this);
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setNameFilter(fileType_filter);
        fileDialog->setViewMode(QFileDialog::Detail);
        fileDialog->setDirectory(getLastPath());

        // connect its signals to our slots
        connect(fileDialog, &QFileDialog::filterSelected,
                this, &MainWindow::slotNetworkFileDialogFilterSelected);
        connect(fileDialog, &QFileDialog::fileSelected,
                this, &MainWindow::slotNetworkFileDialogFileSelected);
        connect(fileDialog, &QFileDialog::rejected,
                this, &MainWindow::slotNetworkFileDialogRejected);

        // open the filedialog
        statusMessage(tr("Choose a network file..."));
        if (fileDialog->exec())
        {
            m_fileName = (fileDialog->selectedFiles()).at(0);
            qCDebug(lcMainWindow) << "m_fileName " << m_fileName;
        }
        else
        {
            // display some error
            statusMessage(tr("Nothing to do..."));
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
    if (checkSelectFileType || fileFormat == FileType::UNRECOGNIZED)
    {

        // This happens only on application startup or on loading a recent file.
        if (!m_fileName.endsWith(".graphml", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".net", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".paj", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".pajek", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".dl", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".dat", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".gml", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".wlst", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".wlist", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".dot", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".2sm", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".sm", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".csv", Qt::CaseInsensitive) &&
            !m_fileName.endsWith(".aff", Qt::CaseInsensitive))
        {
            // ambigious file type. Open an input dialog for the user to choose
            //  what kind of network file this is.

            tempFileNameNoPath = m_fileName.split("/");
            QStringList fileTypes;
            fileTypes << tr("GraphML")
                      << tr("GML")
                      << tr("Pajek")
                      << tr("UCINET")
                      << tr("Adjacency")
                      << tr("GraphViz")
                      << tr("Edge List (weighted)")
                      << tr("Edge List (simple, non-weighted)")
                      << tr("Two-mode sociomatrix");

            bool ok;
            QString userFileType = QInputDialog::getItem(
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
                   "select the right format from the list below.")
                    .arg(tempFileNameNoPath.last()),
                fileTypes, 0, false, &ok);
            if (ok && !userFileType.isEmpty())
            {
                if (userFileType == "GraphML")
                {
                    fileFormat = FileType::GRAPHML;
                }
                else if (userFileType == "GraphML")
                {
                    fileFormat = FileType::PAJEK;
                }
                else if (userFileType == "GML")
                {
                    fileFormat = FileType::GML;
                }
                else if (userFileType == "UCINET")
                {
                    fileFormat = FileType::UCINET;
                }
                else if (userFileType == "Adjacency")
                {
                    fileFormat = FileType::ADJACENCY;
                }
                else if (userFileType == "GraphViz")
                {
                    fileFormat = FileType::GRAPHVIZ;
                }
                else if (userFileType == "Edge List (weighted)")
                {
                    fileFormat = FileType::EDGELIST_WEIGHTED;
                }
                else if (userFileType == "Edge List (simple, non-weighted)")
                {
                    fileFormat = FileType::EDGELIST_SIMPLE;
                }
                else if (userFileType == "Two-mode sociomatrix")
                {
                    fileFormat = FileType::TWOMODE;
                }
            }
            else
            {
                statusMessage(tr("Opening network file aborted."));
                // if a file was previously opened, get back to it.
                if (activeGraph->isLoaded())
                {
                    fileName = previous_fileName;
                }
                return;
            }
        }

        else if (m_fileName.endsWith(".graphml", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".xml", Qt::CaseInsensitive))
        {
            fileFormat = FileType::GRAPHML;
        }
        else if (m_fileName.endsWith(".net", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".paj", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".pajek", Qt::CaseInsensitive))
        {
            fileFormat = FileType::PAJEK;
        }
        else if (m_fileName.endsWith(".dl", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".dat", Qt::CaseInsensitive))
        {
            fileFormat = FileType::UCINET;
        }
        else if (m_fileName.endsWith(".sm", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".csv", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".adj", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".txt", Qt::CaseInsensitive))
        {
            fileFormat = FileType::ADJACENCY;
        }
        else if (m_fileName.endsWith(".dot", Qt::CaseInsensitive))
        {
            fileFormat = FileType::GRAPHVIZ;
        }
        else if (m_fileName.endsWith(".gml", Qt::CaseInsensitive))
        {
            fileFormat = FileType::GML;
        }
        else if (m_fileName.endsWith(".list", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".lst", Qt::CaseInsensitive))
        {
            fileFormat = FileType::EDGELIST_SIMPLE;
        }
        else if (m_fileName.endsWith(".wlist", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".wlst", Qt::CaseInsensitive))
        {
            fileFormat = FileType::EDGELIST_WEIGHTED;
        }
        else if (m_fileName.endsWith(".2sm", Qt::CaseInsensitive) ||
                 m_fileName.endsWith(".aff", Qt::CaseInsensitive))
        {
            fileFormat = FileType::TWOMODE;
        }
        else
            fileFormat = FileType::UNRECOGNIZED;
    }

    qCDebug(lcMainWindow) << "Calling slotNetworkFilePreview"
             << "with m_fileName" << m_fileName
             << "and fileFormat " << fileFormat;

    slotNetworkFilePreview(m_fileName, fileFormat);
}

/**
 * @brief Displays a status message when the user aborts the file dialog
 */
void MainWindow::slotNetworkFileDialogRejected()
{
    qCDebug(lcMainWindow) << "Dialog rejected. If a file was previously opened, get back to it.";
    statusMessage(tr("Opening aborted"));
}

/**
 * @brief Called when user the selects a file filter (i.e. GraphML) in the fileDialog
 * @param filter
 */
void MainWindow::slotNetworkFileDialogFilterSelected(const QString &filter)
{
    qCDebug(lcMainWindow) << "User selected network file filter" << filter;
    if (filter.startsWith("GraphML", Qt::CaseInsensitive))
    {
        fileType = FileType::GRAPHML;
        qCDebug(lcMainWindow) << "fileType FileType::GRAPHML";
    }
    else if (filter.contains("PAJEK", Qt::CaseInsensitive))
    {
        fileType = FileType::PAJEK;
        qCDebug(lcMainWindow) << "fileType FileType::PAJEK";
    }
    else if (filter.contains("DL", Qt::CaseInsensitive) ||
             filter.contains("UCINET", Qt::CaseInsensitive))
    {
        fileType = FileType::UCINET;
        qCDebug(lcMainWindow) << "fileType FileType::UCINET";
    }
    else if (filter.contains("Adjacency", Qt::CaseInsensitive))
    {
        fileType = FileType::ADJACENCY;
        qCDebug(lcMainWindow) << "fileType FileType::ADJACENCY";
    }
    else if (filter.contains("GraphViz", Qt::CaseInsensitive))
    {
        fileType = FileType::GRAPHVIZ;
        qCDebug(lcMainWindow) << "fileType FileType::GRAPHVIZ";
    }
    else if (filter.contains("GML", Qt::CaseInsensitive))
    {
        fileType = FileType::GML;
        qCDebug(lcMainWindow) << "fileType FileType::GML";
    }
    else if (filter.contains("Simple Edge List", Qt::CaseInsensitive))
    {
        fileType = FileType::EDGELIST_SIMPLE;
        qCDebug(lcMainWindow) << "fileType FileType::EDGELIST_SIMPLE";
    }
    else if (filter.contains("Weighted Edge List", Qt::CaseInsensitive))
    {
        fileType = FileType::EDGELIST_WEIGHTED;
        qCDebug(lcMainWindow) << "fileType FileType::EDGELIST_WEIGHTED";
    }
    else if (filter.contains("Two-Mode", Qt::CaseInsensitive))
    {
        fileType = FileType::TWOMODE;
        qCDebug(lcMainWindow) << "fileType FileType::TWOMODE";
    }
    else
    {
        fileType = FileType::UNRECOGNIZED;
        qCDebug(lcMainWindow) << "fileType FileType::UNRECOGNIZED";
    }
}

/**
 * @brief Called when user selects a file in the fileDialog
 * Calls slotNetworkFileChoose() again.
 * @param fileName
 *
 */
void MainWindow::slotNetworkFileDialogFileSelected(const QString &fileName)
{
    qCDebug(lcMainWindow) << "User selected filename:" << fileName
             << "calling slotNetworkFileChoose() with fileType" << fileType;
    slotNetworkFileChoose(fileName,
                          fileType,
                          ((fileType == FileType::UNRECOGNIZED) ? true : false));
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
void MainWindow::slotNetworkSave(const int &fileFormat)
{
    statusMessage(tr("Saving file..."));

    if (activeNodes() == 0)
    {
        statusMessage(QString(tr("Nothing to save. There are no vertices.")));
    }
    if (activeGraph->isSaved())
    {
        statusMessage(QString(tr("Graph already saved.")));
    }
    if (fileName.isEmpty())
    {
        slotNetworkSaveAs();
        return;
    }

    QFileInfo fileInfo(fileName);

    fileNameNoPath = fileInfo.fileName();

    bool saveZeroWeightEdges = appSettings["saveZeroWeightEdges"] == "true" ? true : false;

    bool saveEdgeWeights = true;

    // if the specified format is one of the supported ones, just save it.
    if (activeGraph->isFileFormatExportSupported(fileFormat))
    {
        activeGraph->saveToFile(fileName, fileFormat, saveEdgeWeights, saveZeroWeightEdges);
    }
    // else if it is GraphML or new file not saved yet, just save it.
    else if (activeGraph->getFileFormat() == FileType::GRAPHML ||
             (activeGraph->isSaved() && !activeGraph->isLoaded()))
    {
        activeGraph->saveToFile(fileName, FileType::GRAPHML, saveEdgeWeights, saveZeroWeightEdges);
    }
    // else check whether Graph thinks this is supported and save it
    else if (activeGraph->isFileFormatExportSupported(
                 activeGraph->getFileFormat()))
    {
        activeGraph->saveToFile(fileName, activeGraph->getFileFormat(), saveEdgeWeights, saveZeroWeightEdges);
    }
    // In any other case, save in GraphML.
    // First, inform the user that we will save in that format.
    else
    {
        switch (
            slotHelpMessageToUser(USER_MSG_QUESTION,
                                  tr("Save to GraphML?"),
                                  tr("Default File Format: GraphML "),
                                  tr("This network will be saved in GraphML format "
                                     "which is the default file format of SocNetV. \n\n"
                                     "Is this OK? \n\n"
                                     "If not, press Cancel, then go to Network > Export menu "
                                     "to see other supported formats to export your data to.")))
        {
        case QMessageBox::Yes:
            fileName = QFileInfo(fileName).absolutePath() + "/" + QFileInfo(fileName).baseName();
            fileName.append(".graphml");
            fileNameNoPath = QFileInfo(fileName).fileName();
            setLastPath(fileName); // store this path
            activeGraph->saveToFile(fileName, FileType::GRAPHML, saveEdgeWeights, saveZeroWeightEdges);
            break;
        case QMessageBox::Cancel:
        case QMessageBox::No:
            statusMessage(tr("Save aborted..."));
            break;
        }
    }
}

/**
 * @brief Prompts the user to save the network in a new file.
 * Always uses the GraphML format and extension.
 */
void MainWindow::slotNetworkSaveAs()
{
    qCDebug(lcMainWindow) << "User wants to save the file as a new name...";
    statusMessage(tr("Enter or select a filename to save the network..."));

    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Save Network to GraphML File Named..."),
        getLastPath(), tr("GraphML (*.graphml *.xml);;All (*)"));

    if (!fn.isEmpty())
    {

        if (QFileInfo(fn).suffix().isEmpty())
        {
            fn.append(".graphml");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Appending .graphml extension."),
                tr("Missing file extension. \n"
                   "Appended the standard .graphml extension to the given filename."),
                tr("Final Filename: ") + QFileInfo(fn).fileName());
        }
        else if (!QFileInfo(fn).suffix().contains("graphml", Qt::CaseInsensitive) &&
                 !QFileInfo(fn).suffix().contains("xml", Qt::CaseInsensitive))
        {
            fn = QFileInfo(fn).absolutePath() + "/" + QFileInfo(fn).baseName();
            fn.append(".graphml");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Using .graphml extension."),
                tr("Wrong file extension. \n"
                   "Appended the standard .graphml extension to the given filename."),
                tr("Final Filename: ") + QFileInfo(fn).fileName());
        }
        fileName = fn;
        QFileInfo fileInfo(fileName);
        fileNameNoPath = fileInfo.fileName();
        setLastPath(fileName); // store this path
        slotNetworkSave(FileType::GRAPHML);
    }
    else
    {
        statusMessage(tr("Saving aborted"));
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
void MainWindow::slotNetworkSavedStatus(const int &status)
{

    if (status < 0)
    {
        statusMessage(tr("Error! Could not save this file: %1").arg(fileNameNoPath));
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);
    }
    else if (status == 0)
    {
        // Network needs saving
        // UX: Maybe change it to a more prominent color for the user to see?
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);
    }
    else
    {
        // Network is saved.
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
        networkSaveAct->setEnabled(false);
        setWindowTitle(fileNameNoPath);
        statusMessage(tr("Network saved under filename: %1").arg(fileNameNoPath));
    }
}

/**
 * @brief Closes the current network, saving it if needed.
 */
bool MainWindow::slotNetworkClose()
{

    qCDebug(lcMainWindow) << "Request to close current network file. Check if it is saved...";

    statusMessage(tr("Closing network file..."));

    // An interactive script has no one to click the save-confirmation dialog below - treat
    // unsaved changes as discarded so the script can run unattended. See #261.
    if (!activeGraph->isSaved() && !m_interactiveScriptLines.isEmpty())
    {
        qCDebug(lcMainWindow) << "Interactive script active - discarding unsaved changes without prompting.";
    }
    else if (!activeGraph->isSaved())
    {
        switch (
            slotHelpMessageToUser(
                USER_MSG_QUESTION,
                tr("Closing Network..."),
                tr("Network has not been saved. \n"
                   "Do you want to save before closing it?")))
        {
        case QMessageBox::Yes:
            slotNetworkSave();
            break;
        case QMessageBox::No:
            break;
        case QMessageBox::Cancel:
            return false;
            break;
        }
    }
    qCDebug(lcMainWindow) << "Closing network file. Calling initApp ...";
    initApp();
    qCDebug(lcMainWindow) << "Network file closed...";
    statusMessage(tr("Ready."));
    return true;
}

/**
 * @brief Sends the active network to the printer
 */
void MainWindow::slotNetworkPrint()
{
    statusMessage(tr("Printing..."));
    QPrintDialog dialog(printer, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QPainter painter(printer);
        graphicsWidget->render(&painter);
    };
    statusMessage(tr("Ready."));
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
                                        const int &fileFormat)
{
    qCDebug(lcMainWindow) << "Previewing file: " << m_fileName;
    if (m_fileName.isEmpty())
    {
        statusMessage(tr("No file selected."));
        return false;
    }
    if (!m_encodingOverride.isEmpty())
    {
        qCDebug(lcMainWindow) << "Encoding override set via --encoding, skipping preview dialog:" << m_encodingOverride;
        slotNetworkFileLoad(m_fileName, m_encodingOverride, fileFormat);
        return true;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly))
    {
        QApplication::restoreOverrideCursor();
        slotHelpMessageToUserError(
            tr("Cannot read file %1:\n%2")
                .arg(m_fileName)
                .arg(file.errorString()));
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
void MainWindow::slotNetworkFileLoadRecent()
{

    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        qCDebug(lcMainWindow) << "Loading recent file: " << action->data().toString();
        slotNetworkFileChoose(action->data().toString());
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
                                     const int &fileFormat)
{
    qCDebug(lcMainWindow) << "Request to to load the file:" << fileNameToLoad
             << "codecName" << codecName
             << "fileFormat" << fileFormat;

    initApp();

    userSelectedCodecName = codecName; // var for future use in a Settings dialog
    QString delimiter = QString();
    int sm_two_mode = 0;
    int sm_has_labels = 0;
    if (fileFormat == FileType::TWOMODE)
    {
        switch (
            slotHelpMessageToUser(
                USER_MSG_QUESTION_CUSTOM,
                tr("Two-Mode Sociomatrix — Select Import Mode"),
                tr("Two-mode sociomatrix import"),
                tr("This file contains a two-mode (bipartite) sociomatrix, "
                   "where rows represent one set of actors (e.g. persons) "
                   "and columns represent another set (e.g. events or groups).\n\n"
                   "How would you like to import it?\n\n"
                   "Bipartite graph: creates both sets of nodes and connects "
                   "each person to the events they attend. (Recommended)\n\n"
                   "Person network: creates only the person nodes and connects "
                   "two persons if they share at least one event.\n\n"
                   "Event network: creates only the event nodes and connects "
                   "two events if they share at least one person."),
                QMessageBox::NoButton,
                QMessageBox::Ok,
                tr("Bipartite graph"),
                tr("Person network"),
                tr("Event network")))
        {
        case 1:
            sm_two_mode = 1; // bipartite — default
            break;
        case 2:
            sm_two_mode = 2; // person (Mode-1) projection
            break;
        case 3:
            sm_two_mode = 3; // event (Mode-2) projection
            break;
        default:
            sm_two_mode = 1; // user closed the dialog — go bipartite
            break;
        }
    }
    else if (fileFormat == FileType::ADJACENCY)
    {
        // Ask if there are labels defined on the first line of the ADJACENCY file
        switch (
            slotHelpMessageToUser(
                USER_MSG_QUESTION_CUSTOM,
                tr("Opt for labels"),
                tr("Node labels?"),
                tr("This file contains an adjacency matrix (sociomatrix). "
                   "Please specify whether there are node labels defined "
                   "on the first (comment) line. \n"),
                QMessageBox::NoButton,
                QMessageBox::Ok,
                tr("Yes"), tr("No")

                    ))
        {
        case 1:
            sm_has_labels = 1;
            break;
        case 2:
            sm_has_labels = 0;
            break;
        }
    }

    // Ask for data delimiter
    if (fileFormat == FileType::ADJACENCY ||
        fileFormat == FileType::EDGELIST_SIMPLE ||
        fileFormat == FileType::EDGELIST_WEIGHTED)
    {
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
        if (!ok || delimiter.isEmpty() || delimiter.isNull())
        {
            delimiter = " ";
        }
        qCDebug(lcMainWindow) << "selected delimiter" << delimiter;
    }

    // Change the cursor to wait cursor
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    qCDebug(lcMainWindow) << "Calling graph to do the file load loading...";

    activeGraph->loadFile(
        fileNameToLoad,
        codecName,
        fileFormat,
        delimiter,
        sm_two_mode,
        sm_has_labels);
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
void MainWindow::slotNetworkFileLoaded(const int &type,
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

    if (type <= 0 || fName.isEmpty())
    {
        qCDebug(lcMainWindow) << "ERROR LOADING FILE. FILE UNRECOGNIZED. Message from Parser: "
                 << message
                 << "Calling initApp()";

        statusMessage(tr("Error loading requested file. Aborted."));

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
                                 "please use the options in the Import sub menu. \n")
                                  .arg(message));

        initApp();

        return;
    }

    // A file has been loaded successfully.
    // Update our MW UI and save file path in settings

    qCDebug(lcMainWindow) << "Got signal that a file was loaded:"
             << " filename" << fName
             << " type " << type
             << " totalNodes" << totalNodes
             << " totalEdges" << totalEdges;

    fileName = fName;
    previous_fileName = fileName;
    QFileInfo fileInfo(fileName);
    fileNameNoPath = fileInfo.fileName();

    Q_ASSERT_X(!fileNameNoPath.isEmpty(), "not empty filename ", "empty filename ");

    setWindowTitle(fileNameNoPath);
    setLastPath(fileName); // store this path and file

    QString fileFormatHuman;

    switch (type)
    {
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
                              tr("Unrecognized format. Please specify the file-format using the Import Menu."));

        break;
    }

    // Update LCDs
    rightPanelNodesLCD->setText(QString::number(totalNodes));
    rightPanelEdgesLCD->setText(QString::number(totalEdges));
    rightPanelDensityLCD->setText(QString::number(density));

    statusMessage(tr("%1 formatted network, named '%2', loaded. Nodes: %3, Edges: %4, Density: %5. Elapsed time: %6 ms")
                      .arg(fileFormatHuman)
                      .arg(netName)
                      .arg(totalNodes)
                      .arg(totalEdges)
                      .arg(density)
                      .arg(elapsedTime));

    networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
    networkSaveAct->setEnabled(false);

    // Refresh the data table panel if it is visible
    if (m_tableWidget && m_tableDock->isVisible())
        m_tableWidget->refresh(activeGraph);

    QApplication::restoreOverrideCursor();
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
                                    const qreal &density, const bool &needsSaving)
{

    qCDebug(lcMainWindow) << "Got signal that network changed. Updating mainwindow UI (LCDs, save icon, etc). Params: "
             << "directed" << directed
             << "vertices" << vertices
             << "edges" << edges
             << "density" << density
             << "needsSaving" << needsSaving;

    if (needsSaving)
    {
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px_notsaved.svg"));
        networkSaveAct->setEnabled(true);
    }
    else
    {
        networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
        networkSaveAct->setEnabled(false);
    }

    rightPanelNodesLCD->setText(QString::number(vertices));
    if (!directed)
    {

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

        if (toolBoxEditEdgeModeSelect->currentIndex() == 0)
        {
            // Block signals: this is a UI-state sync, not a user edit-mode change. Without this,
            // setCurrentIndex() re-enters slotEditEdgeMode(), which calls setDirected/setUndirected
            // (mutating the graph) and triggers optionsEdgeArrowsAct->trigger(), which forces a
            // synchronous full-canvas repaint via statusMessage(). See #260.
            toolBoxEditEdgeModeSelect->blockSignals(true);
            toolBoxEditEdgeModeSelect->setCurrentIndex(1);
            toolBoxEditEdgeModeSelect->blockSignals(false);
        }
        rightPanelNetworkTypeLCD->setText("Undirected");

        rightPanelEdgesLabel->setText(tr("Edges:"));

        rightPanelSelectedEdgesLabel->setText(tr("Edges:"));
        editEdgeUndirectedAllAct->setChecked(true);
    }
    else
    {
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

        rightPanelNetworkTypeLCD->setText("Directed");
        if (toolBoxEditEdgeModeSelect->currentIndex() == 1)
        {
            // See the matching comment in the "undirected" branch above.
            toolBoxEditEdgeModeSelect->blockSignals(true);
            toolBoxEditEdgeModeSelect->setCurrentIndex(0);
            toolBoxEditEdgeModeSelect->blockSignals(false);
        }
        rightPanelEdgesLabel->setText(tr("Arcs:"));

        rightPanelSelectedEdgesLabel->setText(tr("Arcs:"));
        editEdgeUndirectedAllAct->setChecked(false);
    }
    rightPanelEdgesLCD->setText(QString::number(edges));
    rightPanelDensityLCD->setText(QString::number(density, 'f', 3));

    qCDebug(lcMainWindow) << "Finished updating mainwindow.";
}
