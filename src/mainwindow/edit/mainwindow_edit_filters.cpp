/**
 * @file mainwindow_edit_filters.cpp
 * @brief Implements MainWindow node/edge filtering (isolates, weight, ego network, attribute, query builder) and the nodes-to-edges transform.
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
#include "widgets/filterbarwidget.h"

#include <QtWidgets>

/**
 * @brief Called when user selects a filter action in the Filter
 * selectbox of the toolbox Network group
 * @param selectedIndex
 */
void MainWindow::toolBoxFilterSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected filter action, index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotFilterNodesByEgoNetwork();
        break;
    case 2:
        slotFilterNodesBySelection();
        break;
    case 3:
        slotFilterNodesDialogByCentrality();
        break;
    case 4:
        slotFilterNodesByAttribute();
        break;
    case 5:
        slotEditFilterEdgesByWeightDialog();
        break;
    case 6:
        slotFilterNodesRestoreAll();
        break;
    case 7:
        slotEditFilterEdgesReset();
        break;
    };
    // Reset to placeholder after dispatching
    toolBoxFilterSelect->blockSignals(true);
    toolBoxFilterSelect->setCurrentIndex(0);
    toolBoxFilterSelect->blockSignals(false);
}

/**
 * @brief Shows a dialog where the user can specify criteria to filter nodes
 *
 */
void MainWindow::slotFilterNodesDialogByCentrality()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    // NOTE: Filter state is snapshot-based. vertexFilterRestoreAll() undoes
    // the last filter; filterNodesRestoreAllAct is enabled after apply.
    //
    //  TODO Deferred (future issues):
    //  - Auto-compute selected index if not yet computed (#216).
    //  - Persist last-used index and threshold (#216).
    //  - Add "Compute missing indices" shortcut in dialog (#216).

    QVector<bool> computedMask;

    const int count = prominenceIndexList.size();
    computedMask.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const IndexType t = static_cast<IndexType>(i + 1);
        computedMask.append(activeGraph->isCentralityIndexComputed(t));
    }

    DialogFilterNodesByCentrality dlg(prominenceIndexList,
                                      computedMask,
                                      this);

    // Not connected straight to activeGraph anymore: that resolved to a queued cross-thread
    // connection (dialog on GUI thread, Graph on graphThread) with no busy-guard and no progress
    // feedback - the same unprotected-async-dispatch shape WS15 P2 closed for computations,
    // just for filtering. Routed through a local lambda + runGraphOperationAsync instead.
    connect(&dlg,
            &DialogFilterNodesByCentrality::userChoices,
            this, [this](const float threshold, const bool overThreshold, const IndexType centralityIndex)
            {
        runGraphOperationAsync(
            [this, threshold, overThreshold, centralityIndex]() {
                activeGraph->vertexFilterByCentrality(threshold, overThreshold, centralityIndex);
            },
            tr("Filtering by centrality..."),
            [this]() {
                filterNodesRestoreAllAct->setEnabled(true);
                m_filterChips.append({tr("Nodes: centrality filter"), FilterCondition::Scope::Nodes});
                m_filterBar->addChip(tr("Nodes: centrality filter"), FilterCondition::Scope::Nodes);
            }); });

    dlg.exec();
}

/**
 * @brief Focuses the view on the currently selected nodes and edges between them.
 *
 * Hides all nodes not in the current selection. Only edges whose both
 * endpoints are selected remain visible. Implements #210.
 */
void MainWindow::slotFilterNodesBySelection()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const QList<int> selection = activeGraph->getSelectedVertices();
    if (selection.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No nodes selected"),
                              tr("Please select at least one node first."));
        return;
    }
    runGraphOperationAsync(
        [this, selection]() { activeGraph->vertexFilterBySelection(selection); },
        tr("Filtering by selection..."),
        [this]() {
            filterNodesRestoreAllAct->setEnabled(true);
            m_filterChips.append({tr("Nodes: selection"), FilterCondition::Scope::Nodes});
            m_filterBar->addChip(tr("Nodes: selection"), FilterCondition::Scope::Nodes);
        });
}

/**
 * @brief Triggers ego network focus mode on the currently selected node.
 */
void MainWindow::slotFilterNodesByEgoNetwork()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int v1 = activeGraph->vertexClicked();
    if (v1 == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No node selected"),
                              tr("Please click on a node first."));
        return;
    }
    runGraphOperationAsync(
        [this, v1]() { activeGraph->vertexFilterByEgoNetwork(v1); },
        tr("Filtering ego network..."),
        [this]() {
            filterNodesRestoreAllAct->setEnabled(true);
            m_filterChips.append({tr("Nodes: ego network"), FilterCondition::Scope::Nodes});
            m_filterBar->addChip(tr("Nodes: ego network"), FilterCondition::Scope::Nodes);
        });
}

/**
 * @brief Opens the Filter By Attribute dialog and applies the chosen condition.
 *
 * Handles both node and edge attribute filtering depending on the scope
 * selected in the dialog.
 */
void MainWindow::slotFilterNodesByAttribute()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QStringList nodeKeys = activeGraph->graphHasVertexCustomAttributes();
    const QStringList edgeKeys = activeGraph->graphHasEdgeCustomAttributes();

    if (nodeKeys.isEmpty() && edgeKeys.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No custom attributes"),
                              tr("This network has no custom node or edge attributes. "
                                 "Add attributes via the Data Table (Ctrl+D) using "
                                 "\"Add attribute…\", or via the Node / Edge Properties dialogs."));
        return;
    }

    DialogFilterByAttribute dlg(nodeKeys, edgeKeys, this);

    connect(&dlg, &DialogFilterByAttribute::userChoices,
            this, [this](const FilterCondition &cond)
            {
        runGraphOperationAsync(
            [this, cond]() {
                if (cond.scope == FilterCondition::Scope::Edges) {
                    activeGraph->edgeFilterByAttribute(cond);
                } else if (cond.scope == FilterCondition::Scope::Both) {
                    activeGraph->vertexFilterByAttribute(cond);
                    activeGraph->edgeFilterByAttribute(cond);
                } else {
                    activeGraph->vertexFilterByAttribute(cond);
                }
            },
            tr("Filtering by attribute..."),
            [this, cond]() {
                if (cond.scope == FilterCondition::Scope::Edges) {
                    m_filterChips.append({cond.label(), FilterCondition::Scope::Edges});
                    m_filterBar->addChip(cond.label(), FilterCondition::Scope::Edges);
                } else if (cond.scope == FilterCondition::Scope::Both) {
                    // Split into two chips so each scope can be closed independently.
                    FilterCondition nc = cond; nc.scope = FilterCondition::Scope::Nodes;
                    FilterCondition ec = cond; ec.scope = FilterCondition::Scope::Edges;
                    m_filterChips.append({nc.label(), FilterCondition::Scope::Nodes});
                    m_filterChips.append({ec.label(), FilterCondition::Scope::Edges});
                    m_filterBar->addChip(nc.label(), FilterCondition::Scope::Nodes);
                    m_filterBar->addChip(ec.label(), FilterCondition::Scope::Edges);
                } else {
                    m_filterChips.append({cond.label(), FilterCondition::Scope::Nodes});
                    m_filterBar->addChip(cond.label(), FilterCondition::Scope::Nodes);
                }
                if (cond.scope != FilterCondition::Scope::Edges)
                    filterNodesRestoreAllAct->setEnabled(true);
                if (cond.scope == FilterCondition::Scope::Edges ||
                    cond.scope == FilterCondition::Scope::Both)
                    editFilterEdgesRestoreAllAct->setEnabled(true);
            }); });

    dlg.exec();

    if (dlg.wasQueryBuilderRequested())
        slotFilterByQueryBuilder();
}

/**
 * @brief Opens the Query Builder dialog for multi-condition AND filtering.
 *
 * Scope (Nodes / Edges) is selected inside the dialog.
 * Applies one compound filter that counts as a single chip and one snapshot.
 */
void MainWindow::slotFilterByQueryBuilder()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QStringList nodeKeys = activeGraph->graphHasVertexCustomAttributes();
    const QStringList edgeKeys = activeGraph->graphHasEdgeCustomAttributes();

    if (nodeKeys.isEmpty() && edgeKeys.isEmpty())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("No custom attributes"),
                              tr("This network has no custom node or edge attributes. "
                                 "Add attributes via the Data Table (Ctrl+D)."));
        return;
    }

    DialogQueryBuilder *dlg = new DialogQueryBuilder(nodeKeys, edgeKeys, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &DialogQueryBuilder::userChoices,
            this, [this](const GraphQuery &query)
            {
        if (query.conditions.isEmpty()) return;

        const FilterCondition::Scope scope = query.conditions.first().scope;
        const int n = query.conditions.size();

        runGraphOperationAsync(
            [this, query, scope]() {
                if (scope == FilterCondition::Scope::Edges) {
                    activeGraph->edgeFilterByQuery(query);
                } else {
                    activeGraph->vertexFilterByQuery(query);
                }
            },
            tr("Applying query filter..."),
            [this, scope, n]() {
                if (scope == FilterCondition::Scope::Edges) {
                    const QString label = tr("Edges: query (%1 condition(s))").arg(n);
                    m_filterChips.append({label, FilterCondition::Scope::Edges});
                    m_filterBar->addChip(label, FilterCondition::Scope::Edges);
                    editFilterEdgesRestoreAllAct->setEnabled(true);
                } else {
                    const QString label = tr("Nodes: query (%1 condition(s))").arg(n);
                    m_filterChips.append({label, FilterCondition::Scope::Nodes});
                    m_filterBar->addChip(label, FilterCondition::Scope::Nodes);
                    filterNodesRestoreAllAct->setEnabled(true);
                }
            }); });

    dlg->exec();
}

/**
 * @brief Restores all nodes hidden by the last filter.
 */
void MainWindow::slotFilterNodesRestoreAll()
{
    // Find the last non-edge chip and remove it via the unified stack mechanism.
    int lastNodeIdx = -1;
    for (int i = m_filterChips.size() - 1; i >= 0; --i)
    {
        if (m_filterChips[i].second != FilterCondition::Scope::Edges)
        {
            lastNodeIdx = i;
            break;
        }
    }
    if (lastNodeIdx < 0)
        return;

    runGraphOperationAsync(
        [this, lastNodeIdx]() { activeGraph->vertexFilterRemoveAt(lastNodeIdx); },
        tr("Restoring filtered nodes..."),
        [this, lastNodeIdx]() {
            m_filterChips.removeAt(lastNodeIdx);
            m_filterBar->clearAllChips();
            for (const auto &chip : std::as_const(m_filterChips))
                m_filterBar->addChip(chip.first, chip.second);

            bool hasNodeFilters = false;
            for (const auto &chip : std::as_const(m_filterChips))
                if (chip.second != FilterCondition::Scope::Edges)
                {
                    hasNodeFilters = true;
                    break;
                }
            filterNodesRestoreAllAct->setEnabled(hasNodeFilters);
        });
}

/**
 * @brief Toggles the status of all isolated vertices
 *
 * @param checked
 */
void MainWindow::slotEditFilterNodesIsolates(bool checked)
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const bool toggleTo = !editFilterNodesIsolatesAct->isChecked();
    runGraphOperationAsync(
        [this, toggleTo]() { activeGraph->vertexIsolatedAllToggle(toggleTo); },
        tr("Toggling isolated nodes..."),
        [this, checked]() {
            if (checked)
            {
                statusMessage(tr("Isolated nodes disabled."));
            }
            else
            {
                statusMessage(tr("Isolated nodes enabled."));
            }
        });
}

/**
 * @brief Shows a dialog where the user can specify how to filter edges by their weight
 *
 * All edges weighted more (or less) than the specified weight will be disabled.
 */
void MainWindow::slotEditFilterEdgesByWeightDialog()
{

    // Note: We do not check if there are active edges, because the user might have disabled all edges previously.

    // Create a new edge filtering dialog
    m_DialogEdgeFilterByWeight = new DialogFilterEdgesByWeight(this);

    // Was two separate connections to userChoices: one straight to activeGraph (a queued
    // cross-thread dispatch with no busy-guard/progress feedback - the same shape WS15 P2 closed
    // for computations), and a second, entirely independent one doing the GUI chip bookkeeping
    // immediately, with no actual dependency on the filter having completed. Merged into one
    // handler via runGraphOperationAsync, so the chip only appears once the filter is done.
    connect(m_DialogEdgeFilterByWeight, &DialogFilterEdgesByWeight::userChoices,
            this, [this](const qreal threshold, const bool overThreshold)
            {
        runGraphOperationAsync(
            [this, threshold, overThreshold]() {
                activeGraph->edgeFilterByWeight(threshold, overThreshold);
            },
            tr("Filtering edges by weight..."),
            [this]() {
                editFilterEdgesRestoreAllAct->setEnabled(true);
                m_filterChips.append({tr("Edges: weight filter"), FilterCondition::Scope::Edges});
                m_filterBar->addChip(tr("Edges: weight filter"), FilterCondition::Scope::Edges);
            }); });

    // Show the dialog
    m_DialogEdgeFilterByWeight->exec();
}

/**
 * @brief Restores all edges hidden by the weight filter.
 */
void MainWindow::slotEditFilterEdgesReset()
{
    // Collect edge-scope chip indices first (highest index first, so each
    // vertexFilterRemoveAt below - and the matching m_filterChips removal in
    // onComplete - operates on the correct stack position without the two
    // lists needing to be mutated in lockstep inside the same loop anymore.
    QList<int> edgeChipIndices;
    for (int i = m_filterChips.size() - 1; i >= 0; --i)
    {
        if (m_filterChips[i].second == FilterCondition::Scope::Edges)
            edgeChipIndices.append(i);
    }
    runGraphOperationAsync(
        [this, edgeChipIndices]() {
            for (int i : edgeChipIndices)
                activeGraph->vertexFilterRemoveAt(i);
        },
        tr("Resetting edge filters..."),
        [this, edgeChipIndices]() {
            for (int i : edgeChipIndices)
                m_filterChips.removeAt(i);
            m_filterBar->clearAllChips();
            for (const auto &chip : std::as_const(m_filterChips))
                m_filterBar->addChip(chip.first, chip.second);
            editFilterEdgesRestoreAllAct->setEnabled(false);
        });
}

/**
 * @brief Toggles the status of all unilateral edges
 *
 * @param checked
 */
void MainWindow::slotEditFilterEdgesUnilateral(bool checked)
{

    if (!activeEdges() && editFilterEdgesUnilateralAct->isChecked())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    const bool toggleTo = !editFilterEdgesUnilateralAct->isChecked();
    runGraphOperationAsync(
        [this, toggleTo]() { activeGraph->edgeFilterUnilateral(toggleTo); },
        tr("Filtering unilateral edges..."),
        [this, checked]() {
            if (checked)
            {
                statusMessage(tr("Unilateral (weak) edges disabled."));
            }
            else
            {
                statusMessage(tr("Unilateral (weak) edges enabled."));
            }
        });
}

void MainWindow::slotEditTransformNodes2Edges()
{
}
