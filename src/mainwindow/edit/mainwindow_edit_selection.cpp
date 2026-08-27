/**
 * @file mainwindow_edit_selection.cpp
 * @brief Implements MainWindow canvas selection/drag-mode handling, the generic context menu, subgraph extraction, and the Data Table view.
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
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "widgets/graphtablewidget.h"
#include "forms/dialognodeedit.h"
#include "forms/dialogedgeedit.h"
#include "forms/dialogbulkedit.h"
#include "forms/dialognodefind.h"
#include "forms/dialogedgedichotomization.h"
#include "forms/dialogfilteredgesbyweight.h"
#include "forms/dialogfilternodesbycentrality.h"
#include "forms/dialogfilterbyattribute.h"
#include "forms/dialogquerybuilder.h"

#include <QtWidgets>

/**
 * @brief Toggles the interactive/selection mouse drag mode
 * @param checked
 */
void MainWindow::slotEditDragModeSelection(bool checked)
{
    qCDebug(lcMainWindow) << "User changed drag mode, checked" << checked;

    editMouseModeScrollAct->setChecked(false);

    if (editMouseModeInteractiveAct->isChecked())
    {

        graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);
        graphicsWidget->setInteractive(true);
    }
    else
    {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
        graphicsWidget->setInteractive(false);
    }
}

/**
 * @brief Toggles the non-interactive scrollhand drag mode.
 * @param checked
 */
void MainWindow::slotEditDragModeScroll(bool checked)
{

    qCDebug(lcMainWindow) << "User changed scroll mode, checked" << checked;

    editMouseModeInteractiveAct->setChecked(false);
    graphicsWidget->setInteractive(false);

    if (editMouseModeScrollAct->isChecked())
    {

        graphicsWidget->setDragMode(QGraphicsView::ScrollHandDrag);
    }
    else
    {
        graphicsWidget->setDragMode(QGraphicsView::NoDrag);
    }
}

/**
 * @brief Shows a context menu when the user right-clicks on an empty area of the canvas
 *        (i.e. not directly on a node or edge).
 *
 * The menu is selection-aware:
 * - When exactly one node is selected and no edges: offers Node Properties.
 * - When multiple nodes are selected: offers Remove + structural transforms (clique, star, …).
 * - When any nodes or edges are selected (≥ 1 total): offers "Edit Selection in Data Table",
 *   "Set property on selected nodes…", and/or "Set property on selected edges…".
 * - Always offers Add Node, Add Edge, and the Options sub-menu.
 *
 * @param mPos  The canvas position where the right-click occurred (unused — menu is shown
 *              at cursor position via QCursor::pos()).
 */
void MainWindow::slotEditOpenContextMenu(const QPointF &mPos)
{
    Q_UNUSED(mPos);
    QMenu *contextMenu = new QMenu(" Menu", this);
    Q_CHECK_PTR(contextMenu); // displays "out of memory" if needed

    int nodesSelected = activeGraph->getSelectedVerticesCount();
    int edgesSelected = activeGraph->getSelectedEdgesCount();
    int totalSelected = nodesSelected + edgesSelected;

    contextMenu->addAction("## Selected nodes: " + QString::number(nodesSelected) + "  edges: " + QString::number(edgesSelected) + " ##  ");

    contextMenu->addSeparator();

    if (nodesSelected > 0)
    {
        if (nodesSelected == 1 && edgesSelected == 0)
        {
            // Single node: offer per-node properties dialog
            contextMenu->addAction(editNodePropertiesAct);
        }
        contextMenu->addSeparator();
        contextMenu->addAction(editNodeRemoveAct);
        if (nodesSelected > 1)
        {
            editNodeRemoveAct->setText(tr("Remove ") + QString::number(nodesSelected) + tr(" nodes"));
            contextMenu->addSeparator();
            contextMenu->addAction(editNodeSelectedToCliqueAct);
            contextMenu->addAction(editNodeSelectedToStarAct);
            contextMenu->addAction(editNodeSelectedToCycleAct);
            contextMenu->addAction(editNodeSelectedToLineAct);
        }
        else
        {
            editNodeRemoveAct->setText(tr("Remove ") + QString::number(nodesSelected) + tr(" node"));
        }
        contextMenu->addSeparator();
    }

    // Table / bulk actions — shown whenever at least one item is selected
    if (totalSelected >= 1)
    {
        contextMenu->addAction(editNodeEditSelectionInTableAct);
        if (nodesSelected > 0)
            contextMenu->addAction(editNodeSetPropertyForSelectionAct);
        if (edgesSelected > 0)
            contextMenu->addAction(editEdgeSetPropertyForSelectionAct);
        contextMenu->addSeparator();
    }

    contextMenu->addAction(editNodeAddAct);
    contextMenu->addSeparator();
    contextMenu->addAction(editEdgeAddAct);
    contextMenu->addSeparator();

    QMenu *options = new QMenu("Options", this);
    contextMenu->addMenu(options);

    options->addAction(openSettingsAct);
    options->addSeparator();
    options->addAction(editNodeSizeAllAct);
    options->addAction(editNodeShapeAll);
    options->addAction(editNodeColorAll);
    options->addAction(optionsNodeNumbersVisibilityAct);
    options->addAction(optionsNodeLabelsVisibilityAct);
    options->addSeparator();
    options->addAction(editEdgeColorAllAct);
    options->addSeparator();
    options->addAction(changeBackColorAct);
    options->addAction(backgroundImageAct);

    // QCursor::pos() is good only for menus not related with node coordinates
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

/**
 * @brief Syncs the Data Table selection to match the canvas selection.
 *
 * Connected to GraphicsWidget::userSelectedItems. Always updates the table's
 * selection model (even when the dock is hidden) so that resolveNodeTargets /
 * resolveEdgeTargets return correct rows when the user later opens the dock
 * and clicks "Set property..." or "Remove attribute...".
 *
 * When only nodes are selected, switches the visible tab to Nodes; when only
 * edges are selected, switches to Edges.
 */
void MainWindow::slotCacheSelection(const QList<int> &nodes,
                                    const QList<SelectedEdge> &edges)
{
    if (!m_tableWidget)
        return;

    // Auto-switch tab when dock is visible:
    // only go to Edges tab when the selection is entirely edges (no nodes).
    if (m_tableDock && m_tableDock->isVisible())
    {
        if (!edges.isEmpty() && nodes.isEmpty())
            m_tableWidget->showEdgesTab();
        else
            m_tableWidget->showNodesTab();
    }

    m_tableWidget->syncNodeSelection(nodes);
    m_tableWidget->syncEdgeSelection(edges);
}

/**
 * @brief Updates the UI (LCDs and Actions) after a change in the user-selected nodes/edges
 *
 * @param nodes
 * @param edges
 */
void MainWindow::slotEditSelectionChanged(const int &selNodes, const int &selEdges)
{
    qCDebug(lcMainWindow) << "Updating UI for new selection";
    rightPanelSelectedNodesLCD->setText(QString::number(selNodes));
    rightPanelSelectedEdgesLCD->setText(QString::number(selEdges));

    if (selNodes > 1)
    {
        editNodeRemoveAct->setText(tr("Remove ") + QString::number(selNodes) + tr(" nodes"));
        editNodeSelectedToCliqueAct->setEnabled(true);
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToStarAct->setEnabled(true);
        editNodeSelectedToStarAct->setText(tr("Create a star from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(true);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from ") + QString::number(selNodes) + tr(" selected nodes"));
        editNodeSelectedToLineAct->setEnabled(true);
        editNodeSelectedToLineAct->setText(tr("Create a line from ") + QString::number(selNodes) + tr(" selected nodes"));
    }
    else
    {
        editNodeRemoveAct->setText(tr("Remove Node"));
        editNodeSelectedToCliqueAct->setText(tr("Create a clique from selected nodes"));
        editNodeSelectedToCliqueAct->setEnabled(false);
        editNodeSelectedToStarAct->setText(tr("Create a star from selected nodes"));
        editNodeSelectedToStarAct->setEnabled(false);
        editNodeSelectedToCycleAct->setText(tr("Create a cycle from selected nodes"));
        editNodeSelectedToCycleAct->setEnabled(false);
        editNodeSelectedToLineAct->setText(tr("Create a line from selected nodes"));
        editNodeSelectedToLineAct->setEnabled(false);
    }

    // Enable selection focus whenever at least 1 node is selected
    filterNodesBySelectionAct->setEnabled(selNodes >= 1);
    // Enable ego network focus only when exactly 1 node is selected
    filterNodesByEgoNetworkAct->setEnabled(selNodes == 1);
    // Enable subgraph-from-selection whenever at least 1 node is selected
    editSubgraphExtractFromSelectionAct->setEnabled(selNodes >= 1);

    //
    // NOTE:
    // DO NOT display a message on the status bar on high frequently called functions like this
    //
}

/**
 * @brief Extracts the currently visible nodes and their inter-edges into an
 *        independent Graph object and saves it to a user-chosen file.
 */
void MainWindow::slotEditSubgraphExtract()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;
    const QString defaultName = tr("Subgraph of %1").arg(activeGraph->getName());
    const QString subgraphName = QInputDialog::getText(
        this,
        tr("Name the subgraph"),
        tr("Subgraph name:"),
        QLineEdit::Normal,
        defaultName,
        &ok);
    if (!ok || subgraphName.trimmed().isEmpty())
        return;

    // sub is written on graphThread (inside the dispatched lambda) and read back here in
    // onComplete (GUI thread) - a std::shared_ptr box makes that value visible across the
    // thread hop, same pattern the writeCentralityX() slots use for their success flag.
    const QString name = subgraphName.trimmed();
    auto sub = std::make_shared<Graph *>(nullptr);
    runGraphOperationAsync(
        [this, name, sub]() { *sub = activeGraph->subgraphExtract(name); },
        tr("Extracting subgraph..."),
        [this, name, sub]() {
            if (!*sub)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
                return;
            }
            saveSubgraphToFile(*sub, name);
        });
}

/**
 * @brief Extracts currently selected nodes and their inter-edges into an
 *        independent Graph object and saves it to a user-chosen file.
 */
void MainWindow::slotEditSubgraphExtractFromSelection()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;
    const QString defaultName = tr("Subgraph of %1").arg(activeGraph->getName());
    const QString subgraphName = QInputDialog::getText(
        this,
        tr("Name the subgraph"),
        tr("Subgraph name:"),
        QLineEdit::Normal,
        defaultName,
        &ok);
    if (!ok || subgraphName.trimmed().isEmpty())
        return;

    const QString name = subgraphName.trimmed();
    auto sub = std::make_shared<Graph *>(nullptr);
    runGraphOperationAsync(
        [this, name, sub]() { *sub = activeGraph->subgraphExtractFromSelection(name); },
        tr("Extracting subgraph..."),
        [this, name, sub]() {
            if (!*sub)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
                return;
            }
            saveSubgraphToFile(*sub, name);
        });
}

/**
 * @brief Shows or hides the Data Table dock panel (#225).
 *
 * When shown, the node and edge models are immediately refreshed from the
 * current graph state.
 *
 * @param checked  true → show panel; false → hide panel.
 */
void MainWindow::slotViewDataTable(bool checked)
{
    if (checked)
    {
        m_tableDock->show();
        m_tableWidget->refresh(activeGraph);
    }
    else
    {
        m_tableDock->hide();
    }
}
