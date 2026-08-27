/**
 * @file mainwindow_edit_shared.cpp
 * @brief Implements small MainWindow Edit-menu-adjacent helpers: saving a subgraph selection to file, and active node/edge counts.
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
