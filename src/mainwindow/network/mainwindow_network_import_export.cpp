/**
 * @file mainwindow_network_import_export.cpp
 * @brief Implements MainWindow Network menu: import (GraphML, GML, Pajek, Adjacency, GraphViz, UCINET, EdgeList, two-mode) and export (image, PDF, Pajek, SM, DOT, DL, GW, list, CSV, JSON) actions.
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
 * @brief Imports a network from a GraphML formatted file
 */
void MainWindow::slotNetworkImportGraphML()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::GRAPHML, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a GML formatted file
 */
void MainWindow::slotNetworkImportGML()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::GML, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a Pajek-like formatted file
 */
void MainWindow::slotNetworkImportPajek()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::PAJEK, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a Adjacency matrix formatted file
 */
void MainWindow::slotNetworkImportAdjacency()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::ADJACENCY, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a Dot (GraphViz) formatted file
 */
void MainWindow::slotNetworkImportGraphviz()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::GRAPHVIZ, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a UCINET formatted file
 */
void MainWindow::slotNetworkImportUcinet()
{
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::UCINET, m_checkSelectFileType);
}

/**
 * @brief Imports a network from a simple List or weighted List formatted file
 */
void MainWindow::slotNetworkImportEdgeList()
{

    qCDebug(lcMainWindow) << "Importing an edge list network file...";

    bool m_checkSelectFileType = false;

    switch (
        slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM,
                              tr("Select type..."),
                              tr("Select type of edge list format"),
                              tr("SocNetV can parse two kinds of edgelist formats: \n\n"
                                 "A. Edge lists with edge weights, "
                                 "where each line has exactly 3 columns: "
                                 "source  target  weight, i.e.:\n"
                                 "1 2 1 \n"
                                 "2 3 1 \n"
                                 "3 4 2 \n"
                                 "4 5 1 \n\n"
                                 "B. Simple edge lists without weights, where each line "
                                 "has two or more columns in the form: source, target1, target2, ... , i.e.:\n"
                                 "1 2 3 4 5 6\n"
                                 "2 3 4 \n"
                                 "3 5 8 7\n\n"
                                 "Please select the appropriate type of edge list format of "
                                 "the file you want to load:"),
                              QMessageBox::NoButton, QMessageBox::NoButton,
                              tr("Weighted"), tr("Simple non-weighted")

                                  ))
    {
    case 1:
        qCDebug(lcMainWindow) << "Weighted list selected! ";
        slotNetworkFileChoose(QString(), FileType::EDGELIST_WEIGHTED, m_checkSelectFileType);
        break;
    case 2:
        qCDebug(lcMainWindow) << "Simple list selected! ";
        slotNetworkFileChoose(QString(), FileType::EDGELIST_SIMPLE, m_checkSelectFileType);
        break;
    }
}

/**
 * @brief Imports a network from a two mode sociomatrix formatted file
 */
void MainWindow::slotNetworkImportTwoModeSM()
{
    qCDebug(lcMainWindow) << "Importing a two mode sociomatrix network file...";
    bool m_checkSelectFileType = false;
    slotNetworkFileChoose(QString(), FileType::TWOMODE, m_checkSelectFileType);
}

/**
 * @brief Opens the Export to Image Dialog
 */
void MainWindow::slotNetworkExportImageDialog()
{
    qCDebug(lcMainWindow) << "Opening Image export dialog...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage(tr("Opening Image export dialog. "));

    m_dialogExportImage = new DialogExportImage(this);

    connect(m_dialogExportImage, &DialogExportImage::userChoices,
            this, &MainWindow::slotNetworkExportImage);

    m_dialogExportImage->exec();
}

/**
 * @brief Exports the network to an image file
 *
 * @param filename
 * @param format
 * @param quality
 * @param compression
 */
void MainWindow::slotNetworkExportImage(const QString &filename,
                                        const QByteArray &format,
                                        const int &quality,
                                        const int &compression)
{

    qCDebug(lcMainWindow) << "Exporting network to image file" << filename;

    if (filename.isEmpty())
    {
        statusMessage(tr("No filename. Exporting to Image aborted."));
        return;
    }
    // store this path
    setLastPath(filename);

    // Get network name from the filename
    tempFileNameNoPath = filename.split("/");
    QString name = tempFileNameNoPath.last();
    name.truncate(name.lastIndexOf("."));

    //
    //  Grab network from canvas
    //
    qCDebug(lcMainWindow) << "Grabbing network from the canvas";

    qreal ratio = 1;
    qreal w = graphicsWidget->width() * ratio;
    qreal h = graphicsWidget->height() * ratio;

    QImage picture = QImage(w, h, QImage::Format_ARGB32_Premultiplied);

    qCDebug(lcMainWindow) << "Creating painter...";
    QPainter p;

    qCDebug(lcMainWindow) << "Begin painter on picture...";
    p.begin(&picture);

    qCDebug(lcMainWindow) << "render scene on painter...";
    graphicsWidget->render(&p);

    //
    // Add name and optionally log
    //
    qCDebug(lcMainWindow) << "Adding name (and logo)..";
    p.setFont(QFont("Helvetica", 10, QFont::Normal, false));
    if (appSettings["printLogo"] == "true")
    {
        QImage logo(":/images/socnetv-logo.png");
        p.drawImage(5, 5, logo);
        p.drawText(7, 47, name);
    }
    else
    {
        p.drawText(5, 15, name);
    }

    qCDebug(lcMainWindow) << "End painter on picture...";
    p.end();

    QString author = "SocNetV v" + VERSION;

    qCDebug(lcMainWindow) << "slotNetworkExportImage() - saving image to file:"
             << filename
             << "format" << format
             << "quality:" << quality
             << "compression:" << compression
             << "Author:" << author;

    //
    // Write image to a file
    //
    QImageWriter imgWriter;
    imgWriter.setFormat(format);
    imgWriter.setQuality(quality);
    imgWriter.setCompression(compression);
    imgWriter.setFileName(filename);
    imgWriter.setText("Author", author);
    imgWriter.setText("", "Created by " + author);
    imgWriter.setOptimizedWrite(true);
    imgWriter.setProgressiveScanWrite(true);
    if (imgWriter.write(picture))
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Network exported to image file."),
                              tr("Network exported to image file."),
                              tr("Image filename: %1").arg(tempFileNameNoPath.last()));
    }
    else
    {
        slotHelpMessageToUser(
            USER_MSG_CRITICAL,
            tr("Error exporting to image file!"),
            tr("Error while exporting network to image file:"),
            imgWriter.errorString());
    }
}

/**
 * @brief Opens the Export to PDF Dialog
 */
void MainWindow::slotNetworkExportPDFDialog()
{
    qCDebug(lcMainWindow) << "MW::slotNetworkExportPDFDialog()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage(tr("Opening PDF export dialog. "));

    m_dialogExportPDF = new DialogExportPDF(this);

    connect(m_dialogExportPDF, &DialogExportPDF::userChoices,
            this, &MainWindow::slotNetworkExportPDF);

    m_dialogExportPDF->exec();
}

/**
 * @brief Exports the visible part of the network to a PDF Document
 *
 * @param pdfName
 * @param orientation
 * @param dpi
 * @param printerMode
 * @param pageSize
 */
void MainWindow::slotNetworkExportPDF(QString &pdfName,
                                      const QPageLayout::Orientation &orientation,
                                      const int &dpi,
                                      const QPrinter::PrinterMode printerMode = QPrinter::ScreenResolution,
                                      const QPageSize &pageSize = QPageSize(QPageSize::A4))
{
    qCDebug(lcMainWindow) << "MW::slotNetworkExportPDF()";

    //    Q_UNUSED(dpi);

    if (pdfName.isEmpty())
    {
        statusMessage(tr("No filename. Exporting to PDF aborted."));
        return;
    }
    else
    {

        setLastPath(pdfName); // store this path
        tempFileNameNoPath = pdfName.split("/");
        QString name = tempFileNameNoPath.last();
        name.truncate(name.lastIndexOf("."));

        printerPDF = new QPrinter(printerMode);
        printerPDF->setOutputFormat(QPrinter::PdfFormat);
        printerPDF->setOutputFileName(pdfName);
        printerPDF->setPageOrientation(orientation);
        printerPDF->setPageSize(pageSize);
        printerPDF->setFontEmbeddingEnabled(true);
        printerPDF->setResolution(dpi);
        QPainter p;
        p.begin(printerPDF);
        graphicsWidget->render(&p, QRect(0, 0, printerPDF->width(), printerPDF->height()),
                               graphicsWidget->viewport()->rect());
        p.setFont(QFont("Helvetica", 8, QFont::Normal, false));
        if (appSettings["printLogo"] == "true")
        {
            QImage logo(":/images/socnetv-logo.png");
            p.drawImage(5, 5, logo);
            p.drawText(7, 47, name);
        }
        else
        {
            p.drawText(5, 15, name);
        }

        qCDebug(lcMainWindow) << "End painter on QPrinter...";
        p.end();
        delete printerPDF;
        // Reset to nullptr (WS7 MW0 finding, ASan-confirmed): printerPDF is a MainWindow member,
        // and closeEvent() also does `delete printerPDF;` on app quit. Without nulling it here,
        // that was a use-after-free/double-free on any session that exported a PDF at least once
        // before quitting - pre-existing, unrelated to the printer's own use in slotNetworkPrint()
        // (which never reassigns/deletes it).
        printerPDF = nullptr;
    }
    qCDebug(lcMainWindow) << "Exporting PDF to " << pdfName;
    tempFileNameNoPath = pdfName.split("/");
    setLastPath(pdfName);
    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("Network exported to PDF file."),
                          tr("Network exported to PDF file."),
                          tr("PDF filename: %1").arg(tempFileNameNoPath.last()));
}

/**
 * @brief Exports the network to a Pajek-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportPajek()
{
    qCDebug(lcMainWindow) << "MW::slotNetworkExportPajek";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage(tr("Exporting active network under new filename..."));
    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Export Network to File Named..."),
        getLastPath(), tr("Pajek (*.paj *.net *.pajek);;All (*)"));
    if (!fn.isEmpty())
    {
        if (QFileInfo(fn).suffix().isEmpty())
        {
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Missing file extension. I will use .paj instead."),
                                  tr("Missing file extension. I will use the .paj extension."),
                                  tr("Appending an extension .paj to the given filename..."));
            fn.append(".paj");
        }
        fileName = fn;
        setLastPath(fileName);
        QFileInfo fileInfo(fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else
    {
        statusMessage(tr("Saving aborted"));
        return;
    }

    activeGraph->saveToFile(fileName, FileType::PAJEK);
}

/**
 * @brief Exports the network to an adjacency matrix-formatted file
 * Calls the relevant Graph method.
 */
void MainWindow::slotNetworkExportSM()
{
    qCDebug(lcMainWindow, "MW: slotNetworkExportSM()");
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage(tr("Exporting active network under new filename..."));
    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Export Network to File Named..."),
        getLastPath(), tr("Adjacency (*.csv *.txt *.adj *.sm *.net);;All (*)"));
    if (!fn.isEmpty())
    {
        if (QFileInfo(fn).suffix().isEmpty())
        {
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Missing file extension. I will use .csv instead."),
                                  tr("Missing file extension. I will use the .csv  extension."),
                                  tr("Appending an extension .csv  to the given filename..."));

            fn.append(".csv");
        }
        fileName = fn;
        setLastPath(fileName);
        QFileInfo fileInfo(fileName);
        fileNameNoPath = fileInfo.fileName();
    }
    else
    {
        statusMessage(tr("Saving aborted"));
        return;
    }

    bool saveEdgeWeights = false;
    if (activeGraph->isWeighted())
    {
        switch (
            slotHelpMessageToUser(USER_MSG_QUESTION,
                                  tr("Weighted graph. Social network with valued/weighted edges"),
                                  tr("Social network with valued/weighted edges"),
                                  tr("This social network includes valued/weighted edges "
                                     "(the depicted graph is weighted). "
                                     "Do you want to save the edge weights in the adjacency file?\n"
                                     "Select Yes if you want to save edge values "
                                     "in the resulting file. \n"
                                     "Select No, if you don't want edge values "
                                     "to be saved. In the later case, all non-zero values will be truncated to 1."))

        )
        {
        case QMessageBox::Yes:
            saveEdgeWeights = true;
            break;
        case QMessageBox::No:
            saveEdgeWeights = false;
            break;
        case QMessageBox::Cancel:
            statusMessage(tr("Save aborted..."));
            return;
            break;
        }
    }

    activeGraph->saveToFile(fileName, FileType::ADJACENCY, saveEdgeWeights);
}

/**
 * @brief Exports the active relation to a GraphViz DOT (.dot) file.
 *
 * Warns the user if the graph has multiple relations (only the active one is
 * written) or if it carries custom attributes (those are preserved in DOT).
 */
void MainWindow::slotNetworkExportDot()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    statusMessage(tr("Exporting active relation to GraphViz DOT file…"));

    if (activeGraph->relations() > 1)
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Only active relation will be exported"),
            tr("Multi-relation graph — GraphViz DOT format"),
            tr("GraphViz DOT supports a single graph. "
               "Only the currently active relation \"%1\" will be written.")
                .arg(activeGraph->relationCurrentName()));
    }

    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Export Network to GraphViz DOT File…"),
        getLastPath(),
        tr("GraphViz DOT (*.dot);;All (*)"));

    if (fn.isEmpty())
    {
        statusMessage(tr("Export aborted."));
        return;
    }
    if (QFileInfo(fn).suffix().isEmpty())
        fn.append(".dot");

    setLastPath(fn);
    activeGraph->saveToFile(fn, FileType::GRAPHVIZ);
}

/**
 * @brief Exports the current graph to a UCINET DL-formatted file.
 *
 * Uses FULLMATRIX format.  Multi-relation graphs are fully supported:
 * all relations are written as consecutive matrices in the DATA section.
 */
bool MainWindow::slotNetworkExportDL()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return false;
    }

    statusMessage(tr("Exporting network to UCINET DL file…"));

    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Export Network to UCINET DL File…"),
        getLastPath(),
        tr("UCINET DL (*.dl *.dat);;All (*)"));

    if (fn.isEmpty())
    {
        statusMessage(tr("Export aborted."));
        return false;
    }
    if (QFileInfo(fn).suffix().isEmpty())
        fn.append(".dl");

    setLastPath(fn);
    activeGraph->saveToFile(fn, FileType::UCINET);
    return true;
}

/**
    TODO: Exports the network to a GW-formatted file
*/
bool MainWindow::slotNetworkExportGW()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return false;
    }

    if (fileName.isEmpty())
    {
        statusMessage(tr("Saving network under new filename..."));
        QString fn = QFileDialog::getSaveFileName(
            this, "Export GW", getLastPath(), 0);
        if (!fn.isEmpty())
        {
            fileName = fn;
            setLastPath(fileName);
        }
        else
        {
            statusMessage(tr("Saving aborted"));
            return false;
        }
    }

    return true;
}

/**
 * @brief Exports the active relation to a simple or weighted edge list file.
 *
 * Asks the user whether to include edge weights.  Warns when multiple
 * relations are present (only the active relation is written).  Node labels
 * are used as identifiers; spaces are replaced with underscores.
 */
bool MainWindow::slotNetworkExportList()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return false;
    }

    if (activeGraph->relations() > 1)
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Only active relation will be exported"),
            tr("Multi-relation graph — Edge List format"),
            tr("Edge List format supports a single relation. "
               "Only the currently active relation \"%1\" will be written.")
                .arg(activeGraph->relationCurrentName()));
    }

    // Ask weighted or simple
    bool weighted = false;
    if (activeGraph->isWeighted())
    {
        switch (slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Include edge weights?"),
            tr("Weighted graph"),
            tr("This network has weighted edges. "
               "Do you want to include edge weights in the exported file?\n\n"
               "Yes → weighted edge list (source target weight)\n"
               "No  → simple edge list (source target)")))
        {
        case QMessageBox::Yes:
            weighted = true;
            break;
        case QMessageBox::No:
            weighted = false;
            break;
        case QMessageBox::Cancel:
            statusMessage(tr("Export aborted."));
            return false;
        }
    }

    statusMessage(tr("Exporting active relation to Edge List file…"));

    const QString filter = weighted
                               ? tr("Weighted Edge List (*.wlst *.csv *.txt);;All (*)")
                               : tr("Simple Edge List (*.lst *.csv *.txt);;All (*)");
    const QString defaultExt = weighted ? ".wlst" : ".lst";

    QString fn = QFileDialog::getSaveFileName(
        this, tr("Export Network to Edge List File…"), getLastPath(), filter);

    if (fn.isEmpty())
    {
        statusMessage(tr("Export aborted."));
        return false;
    }
    if (QFileInfo(fn).suffix().isEmpty())
        fn.append(defaultExt);

    setLastPath(fn);
    activeGraph->saveToFile(
        fn,
        weighted ? FileType::EDGELIST_WEIGHTED : FileType::EDGELIST_SIMPLE);
    return true;
}

/**
 * @brief Exports all node data (unfiltered) to a CSV file chosen by the user.
 */
void MainWindow::slotNetworkExportNodesCSV()
{
    m_tableWidget->refresh(activeGraph);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Nodes as CSV"), tr("nodes.csv"),
        tr("CSV files (*.csv)"));
    if (path.isEmpty())
        return;
    if (TableExport::toCSV(m_tableWidget->nodeModel(), path))
        statusMessage(tr("Nodes exported to %1").arg(path));
    else
        statusMessage(tr("Export failed: could not write to %1").arg(path));
}

/**
 * @brief Exports all edge data (unfiltered) to a CSV file chosen by the user.
 */
void MainWindow::slotNetworkExportEdgesCSV()
{
    m_tableWidget->refresh(activeGraph);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Edges as CSV"), tr("edges.csv"),
        tr("CSV files (*.csv)"));
    if (path.isEmpty())
        return;
    if (TableExport::toCSV(m_tableWidget->edgeModel(), path))
        statusMessage(tr("Edges exported to %1").arg(path));
    else
        statusMessage(tr("Export failed: could not write to %1").arg(path));
}

/**
 * @brief Exports all node data (unfiltered) to a JSON file chosen by the user.
 */
void MainWindow::slotNetworkExportNodesJSON()
{
    m_tableWidget->refresh(activeGraph);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Nodes as JSON"), tr("nodes.json"),
        tr("JSON files (*.json)"));
    if (path.isEmpty())
        return;
    if (TableExport::toJSON(m_tableWidget->nodeModel(), path))
        statusMessage(tr("Nodes exported to %1").arg(path));
    else
        statusMessage(tr("Export failed: could not write to %1").arg(path));
}

/**
 * @brief Exports all edge data (unfiltered) to a JSON file chosen by the user.
 */
void MainWindow::slotNetworkExportEdgesJSON()
{
    m_tableWidget->refresh(activeGraph);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Edges as JSON"), tr("edges.json"),
        tr("JSON files (*.json)"));
    if (path.isEmpty())
        return;
    if (TableExport::toJSON(m_tableWidget->edgeModel(), path))
        statusMessage(tr("Edges exported to %1").arg(path));
    else
        statusMessage(tr("Export failed: could not write to %1").arg(path));
}
