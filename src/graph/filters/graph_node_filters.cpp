/**
 * @file graph_node_filters.cpp
 * @brief Implements vertex filtering functions for the Graph class.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "graph.h"
#include "filter_condition.h"
#include "filter_spec.h"
#include "graph_query.h"
#include <QDebug>

/**
 * @brief Filters vertices by their score on the given centrality or prestige index.
 *
 * Pushes a GraphVisibilitySnapshot onto m_visibilityHistory before making
 * any changes, so vertexFilterRestoreAll() can undo this filter.
 *
 * Performs two passes over the graph:
 *
 * - Pass 1: Enables or disables each vertex depending on whether its score
 *   satisfies the threshold condition. Emits setVertexVisibility for every
 *   vertex whose state changes.
 *
 * - Pass 2: Updates all edges. An edge is made visible only when both its
 *   source and target vertices are enabled after pass 1. Emits
 *   signalSetEdgeVisibility for every out-edge of every vertex.
 *
 * The two-pass design is required so that edge visibility can be determined
 * correctly: during pass 2 all vertices are already in their final enabled
 * state, so both endpoints of any edge can be queried reliably.
 *
 * Returns early with a status message if the requested index has not been
 * computed yet — the user should run the corresponding analysis first.
 *
 * @param threshold        Score threshold to compare against.
 * @param overThreshold    If true, disable vertices with score >= threshold;
 *                         if false, disable vertices with score <= threshold.
 * @param centralityIndex  The centrality or prestige index to use, as defined
 *                         by the IndexType enum in global.h.
 *
 * @see isCentralityIndexComputed()
 * @see vertexFilterRestoreAll()
 * @see IndexType
 */
void Graph::vertexFilterByCentrality(const float threshold,
                                     const bool overThreshold,
                                     const IndexType centralityIndex)
{
    qDebug() << "Graph::vertexFilterByCentrality()"
             << "index:" << static_cast<int>(centralityIndex)
             << "threshold:" << threshold
             << "overThreshold:" << overThreshold;

    if (!isCentralityIndexComputed(centralityIndex))
    {
        qDebug() << "Graph::vertexFilterByCentrality() - "
                    "index"
                 << static_cast<int>(centralityIndex)
                 << "not yet computed. Aborting.";
        progressStatus(
            tr("Please compute the selected centrality/prestige index first, "
               "then apply the filter."));
        return;
    }

    // ------------------------------------------------------------------
    // Snapshot current visibility state BEFORE making any changes.
    // Allows vertexFilterRestoreAll() to undo this filter.
    // ------------------------------------------------------------------
    GraphVisibilitySnapshot snapshot;
    snapshot.active = true;

    VList::const_iterator si;
    for (si = m_graph.cbegin(); si != m_graph.cend(); ++si)
    {
        const int vnum = (*si)->number();
        snapshot.nodeVisible.insert(vnum, (*si)->isEnabled());

        H_edges::const_iterator ei = (*si)->m_outEdges.constBegin();
        while (ei != (*si)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
            {
                snapshot.arcVisible.insert(
                    QPair<int, int>(vnum, ei.key()),
                    ei.value().second.second);
            }
            ++ei;
        }
    }

    snapshot.spec.type                      = FilterSpec::Type::Centrality;
    snapshot.spec.centralityThreshold      = threshold;
    snapshot.spec.centralityOverThreshold  = overThreshold;
    snapshot.spec.centralityIndex          = centralityIndex;
    m_visibilityHistory.push(snapshot);

    const QString condition = overThreshold ? QStringLiteral(">=") : QStringLiteral("<=");
    const qreal thresh = static_cast<qreal>(threshold);

    // ---------------------------------------------------------------
    // PASS 1: Enable / disable vertices only, no edge changes yet.
    // After this pass every vertex is in its correct enabled state,
    // so the edge pass can safely query both endpoints.
    // ---------------------------------------------------------------
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        qreal score = 0.0;
        switch (centralityIndex)
        {
        case IndexType::DC:
            score = (*it)->DC();
            break;
        case IndexType::CC:
            score = (*it)->CC();
            break;
        case IndexType::IRCC:
            score = (*it)->IRCC();
            break;
        case IndexType::BC:
            score = (*it)->BC();
            break;
        case IndexType::SC:
            score = (*it)->SC();
            break;
        case IndexType::EC:
            score = (*it)->EC();
            break;
        case IndexType::PC:
            score = (*it)->PC();
            break;
        case IndexType::IC:
            score = (*it)->IC();
            break;
        case IndexType::EVC:
            score = (*it)->EVC();
            break;
        case IndexType::DP:
            score = (*it)->DP();
            break;
        case IndexType::PRP:
            score = (*it)->PRP();
            break;
        case IndexType::PP:
            score = (*it)->PP();
            break;
        default:
            score = (*it)->DC();
            break;
        }

        const bool shouldDisable = overThreshold ? (score >= thresh) : (score <= thresh);
        const bool currentlyEnabled = (*it)->isEnabled();

        qDebug() << "PASS 1 - vertex" << (*it)->number()
                 << "score=" << score
                 << condition << threshold
                 << "shouldDisable:" << shouldDisable
                 << "currentlyEnabled:" << currentlyEnabled;

        if (currentlyEnabled == !shouldDisable)
            continue; // already in correct state

        (*it)->setEnabled(!shouldDisable);
        setModStatus(ModStatus::VertexCount);
        emit setVertexVisibility((*it)->number(), !shouldDisable);
    }

    // PASS 2 — Update edge visibility.
    // Mirrors edgeFilterByWeight's proven re-enable pattern exactly.
    // An edge is visible only when both endpoints are enabled.
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        const int v = (*it)->number();

        H_edges::iterator ed;
        for (ed = (*it)->m_outEdges.begin(); ed != (*it)->m_outEdges.end(); ++ed)
        {
            if (ed.value().first != m_curRelation)
                continue;

            const int target = ed.key();
            const qreal weight = ed.value().second.first;
            const qreal reverseEdgeWeight = (*it)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseEdgeWeight != 0);

            const bool sourceEnabled = (*it)->isEnabled();
            const bool targetEnabled = vertexAtIndex(vertexIndexByNumber(target))->isEnabled();
            const bool edgeVisible = sourceEnabled && targetEnabled;

            qDebug() << "PASS 2 - edge" << v << "->" << target
                     << "sourceEnabled:" << sourceEnabled
                     << "targetEnabled:" << targetEnabled
                     << "edgeVisible:" << edgeVisible;

            // Update out-edge storage
            ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeVisible));
            // Update in-edge bookkeeping on target
            edgeInboundStatusSet(target, v, edgeVisible);

            if (edgeVisible)
            {
                // Re-enable: use the same short form as edgeFilterByWeight
                emit signalSetEdgeVisibility(m_curRelation, v, target,
                                             true, preserveReverse);
            }
            else
            {
                // Disable: pass weights so GW can decide on arc rendering
                emit signalSetEdgeVisibility(m_curRelation, v, target,
                                             false, preserveReverse,
                                             weight, reverseEdgeWeight);
            }
        }
    }

    progressStatus(
        tr("Filter applied: vertices with score %1 %2 are now hidden.")
            .arg(condition, QString::number(threshold)));
}

/**
 * @brief Saves current visibility state and shows only the ego network
 *        of vertex v1 at the given depth.
 *
 * Non-destructive: pushes a GraphVisibilitySnapshot onto m_visibilityHistory
 * before making any changes. Call vertexFilterRestoreAll() to undo.
 *
 * Only out-edges in the current relation are used to determine neighbors.
 * Works correctly for both directed and undirected graphs.
 *
 * @param v1     The ego vertex (center of the neighborhood).
 * @param depth  Neighborhood depth (currently only depth=1 supported).
 */
void Graph::vertexFilterByEgoNetwork(const int v1, const int depth)
{
    Q_UNUSED(depth); // reserved for future k-hop support

    qDebug() << "Graph::vertexFilterByEgoNetwork() - ego:" << v1;

    if (!vertexExists(v1))
    {
        qDebug() << "Graph::vertexFilterByEgoNetwork() - vertex" << v1 << "not found. Aborting.";
        return;
    }

    // ------------------------------------------------------------------
    // Build the visible set: ego + its 1-hop neighbors via enabled
    // out-edges in the current relation.
    // ------------------------------------------------------------------
    QSet<int> visibleSet;
    visibleSet.insert(v1);

    const GraphVertex *ego = vertexAtIndex(vpos[v1]);
    H_edges::const_iterator it = ego->m_outEdges.constBegin();
    while (it != ego->m_outEdges.constEnd())
    {
        if (it.value().first == m_curRelation && it.value().second.second == true)
        {
            visibleSet.insert(it.key());
        }
        ++it;
    }

    qDebug() << "Graph::vertexFilterByEgoNetwork() - visible set:" << visibleSet;

    // ------------------------------------------------------------------
    // Snapshot current visibility state BEFORE making any changes.
    // ------------------------------------------------------------------
    GraphVisibilitySnapshot snapshot;
    snapshot.active = true;

    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        snapshot.nodeVisible.insert(vnum, (*vi)->isEnabled());

        H_edges::const_iterator ei = (*vi)->m_outEdges.constBegin();
        while (ei != (*vi)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
            {
                snapshot.arcVisible.insert(
                    QPair<int, int>(vnum, ei.key()),
                    ei.value().second.second);
            }
            ++ei;
        }
    }

    snapshot.spec.type      = FilterSpec::Type::Ego;
    snapshot.spec.egoVertex = v1;
    snapshot.spec.egoDepth  = depth;
    m_visibilityHistory.push(snapshot);

    // ------------------------------------------------------------------
    // PASS 1: Set vertex visibility.
    // ------------------------------------------------------------------
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        const bool shouldBeVisible = visibleSet.contains(vnum);
        if ((*vi)->isEnabled() != shouldBeVisible)
        {
            (*vi)->setEnabled(shouldBeVisible);
            setModStatus(ModStatus::VertexCount);
            emit setVertexVisibility(vnum, shouldBeVisible);
        }
    }

    // ------------------------------------------------------------------
    // PASS 2: Set edge visibility.
    // An edge is visible only if both endpoints are in the visible set.
    // ------------------------------------------------------------------
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int source = (*vi)->number();

        H_edges::iterator ei = (*vi)->m_outEdges.begin();
        while (ei != (*vi)->m_outEdges.end())
        {
            if (ei.value().first != m_curRelation)
            {
                ++ei;
                continue;
            }
            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);
            const bool edgeShouldBeVisible =
                visibleSet.contains(source) && visibleSet.contains(target);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeShouldBeVisible));
            edgeInboundStatusSet(target, source, edgeShouldBeVisible);

            if (edgeShouldBeVisible)
            {
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             true, preserveReverse);
            }
            else
            {
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse,
                                             weight, reverseWeight);
            }
            ++ei;
        }
    }

    progressStatus(tr("Showing ego network of node %1 (%2 neighbors).")
                       .arg(v1)
                       .arg(visibleSet.size() - 1));
}

/**
 * @brief Saves current visibility state and shows only the selected vertices
 *        and the edges between them.
 *
 * Non-destructive: pushes a GraphVisibilitySnapshot onto m_visibilityHistory
 * before making any changes. Call vertexFilterRestoreAll() to undo.
 *
 * Implements the "Focus on Selection" mode (#210): all nodes not in
 * @p selectedVertices are hidden, and only edges whose both endpoints are
 * in the selection remain visible.
 *
 * @param selectedVertices  List of vertex numbers that should remain visible.
 */
void Graph::vertexFilterBySelection(const QList<int> &selectedVertices)
{
    qDebug() << "Graph::vertexFilterBySelection() - selection:" << selectedVertices;

    if (selectedVertices.isEmpty())
    {
        qDebug() << "Graph::vertexFilterBySelection() - empty selection, aborting.";
        progressStatus(tr("No nodes selected. Please select at least one node first."));
        return;
    }

    const QSet<int> visibleSet(selectedVertices.cbegin(), selectedVertices.cend());

    // ------------------------------------------------------------------
    // Snapshot current visibility state BEFORE making any changes.
    // ------------------------------------------------------------------
    GraphVisibilitySnapshot snapshot;
    snapshot.spec.type      = FilterSpec::Type::Selection;
    snapshot.spec.selection = selectedVertices;
    snapshot.active = true;

    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        snapshot.nodeVisible.insert(vnum, (*vi)->isEnabled());

        H_edges::const_iterator ei = (*vi)->m_outEdges.constBegin();
        while (ei != (*vi)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
            {
                snapshot.arcVisible.insert(
                    QPair<int, int>(vnum, ei.key()),
                    ei.value().second.second);
            }
            ++ei;
        }
    }

    m_visibilityHistory.push(snapshot);

    // ------------------------------------------------------------------
    // PASS 1: Set vertex visibility.
    // ------------------------------------------------------------------
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        const bool shouldBeVisible = visibleSet.contains(vnum);
        if ((*vi)->isEnabled() != shouldBeVisible)
        {
            (*vi)->setEnabled(shouldBeVisible);
            setModStatus(ModStatus::VertexCount);
            emit setVertexVisibility(vnum, shouldBeVisible);
        }
    }

    // ------------------------------------------------------------------
    // PASS 2: Set edge visibility.
    // An edge is visible only if both endpoints are in the selection.
    // ------------------------------------------------------------------
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int source = (*vi)->number();

        H_edges::iterator ei = (*vi)->m_outEdges.begin();
        while (ei != (*vi)->m_outEdges.end())
        {
            if (ei.value().first != m_curRelation)
            {
                ++ei;
                continue;
            }
            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);
            const bool edgeShouldBeVisible =
                visibleSet.contains(source) && visibleSet.contains(target);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeShouldBeVisible));
            edgeInboundStatusSet(target, source, edgeShouldBeVisible);

            if (edgeShouldBeVisible)
            {
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             true, preserveReverse);
            }
            else
            {
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse,
                                             weight, reverseWeight);
            }
            ++ei;
        }
    }

    progressStatus(tr("Showing %1 selected node(s) and edges between them.")
                       .arg(visibleSet.size()));
}

// ---------------------------------------------------------------------------
// Node attribute filter
// ---------------------------------------------------------------------------

/**
 * @brief Shows only vertices whose custom attribute satisfies @p cond;
 *        all other vertices are hidden.
 *
 * Non-destructive: pushes a GraphVisibilitySnapshot onto m_visibilityHistory
 * before making any changes. Call vertexFilterRestoreAll() to undo.
 *
 * Returns early with a status message if the resulting visible set is empty.
 *
 * @param cond  The filter condition (key, operator, value).
 */
void Graph::vertexFilterByAttribute(const FilterCondition &cond)
{
    qDebug() << "Graph::vertexFilterByAttribute() key:" << cond.key
             << "op:" << static_cast<int>(cond.op) << "value:" << cond.value;

    // Build visible set: vertices that satisfy the condition.
    QSet<int> visibleSet;
    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const QHash<QString,QString> attrs = (*vi)->customAttributes();
        if (attrs.contains(cond.key) && cond.matches(attrs.value(cond.key)))
            visibleSet.insert((*vi)->number());
    }

    if (visibleSet.isEmpty())
    {
        progressStatus(tr("No nodes found matching: %1.").arg(cond.label()));
        return;
    }

    // Snapshot current visibility state BEFORE making any changes.
    GraphVisibilitySnapshot snapshot;
    snapshot.active          = true;
    snapshot.spec.type       = FilterSpec::Type::Attribute;
    snapshot.spec.condition  = cond;

    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        snapshot.nodeVisible.insert(vnum, (*vi)->isEnabled());

        H_edges::const_iterator ei = (*vi)->m_outEdges.constBegin();
        while (ei != (*vi)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
            {
                snapshot.arcVisible.insert(
                    QPair<int,int>(vnum, ei.key()),
                    ei.value().second.second);
            }
            ++ei;
        }
    }

    m_visibilityHistory.push(snapshot);

    // PASS 1: Set vertex visibility.
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        const bool shouldBeVisible = visibleSet.contains(vnum);
        if ((*vi)->isEnabled() != shouldBeVisible)
        {
            (*vi)->setEnabled(shouldBeVisible);
            setModStatus(ModStatus::VertexCount);
            emit setVertexVisibility(vnum, shouldBeVisible);
        }
    }

    // PASS 2: Set edge visibility (both endpoints must be visible).
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int source = (*vi)->number();

        H_edges::iterator ei = (*vi)->m_outEdges.begin();
        while (ei != (*vi)->m_outEdges.end())
        {
            if (ei.value().first != m_curRelation)
            {
                ++ei;
                continue;
            }
            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);
            const bool edgeShouldBeVisible =
                visibleSet.contains(source) && visibleSet.contains(target);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeShouldBeVisible));
            edgeInboundStatusSet(target, source, edgeShouldBeVisible);

            if (edgeShouldBeVisible)
                emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverse);
            else
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse, weight, reverseWeight);
            ++ei;
        }
    }

    progressStatus(tr("Showing %1 node(s) matching: %2.")
                       .arg(visibleSet.size()).arg(cond.label()));
}

/**
 * @brief Restores vertex and edge visibility from the top snapshot on the
 *        history stack.
 *
 * If the stack is empty (no filter is active), this is a no-op.
 * Pops the snapshot after restoring, so repeated calls walk back through
 * the filter history one step at a time.
 */
void Graph::vertexFilterRestoreAll()
{
    qDebug() << "Graph::vertexFilterRestoreAll()";

    if (m_visibilityHistory.isEmpty())
    {
        qDebug() << "Graph::vertexFilterRestoreAll() - history stack empty, nothing to restore.";
        progressStatus(tr("No active filter to restore."));
        return;
    }

    applyVisibilitySnapshot(m_visibilityHistory.pop());
    progressStatus(tr("Graph visibility restored."));
}

/**
 * @brief Restores vertex and edge visibility to the state recorded in @p snap.
 *
 * Private helper shared by vertexFilterRestoreAll() and vertexFilterRemoveAt().
 * Does not touch m_visibilityHistory — callers manage the stack.
 */
void Graph::applyVisibilitySnapshot(const GraphVisibilitySnapshot &snap)
{
    // PASS 1: Restore vertex visibility.
    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        const bool wasVisible = snap.nodeVisible.value(vnum, true);
        if ((*vi)->isEnabled() != wasVisible)
        {
            (*vi)->setEnabled(wasVisible);
            setModStatus(ModStatus::VertexCount);
            emit setVertexVisibility(vnum, wasVisible);
        }
    }

    // PASS 2: Restore edge visibility.
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int source = (*vi)->number();

        H_edges::iterator ei = (*vi)->m_outEdges.begin();
        while (ei != (*vi)->m_outEdges.end())
        {
            if (ei.value().first != m_curRelation)
            {
                ++ei;
                continue;
            }
            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);
            const bool wasVisible =
                snap.arcVisible.value(QPair<int, int>(source, target), true);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, wasVisible));
            edgeInboundStatusSet(target, source, wasVisible);

            if (wasVisible)
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             true, preserveReverse);
            else
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse,
                                             weight, reverseWeight);
            ++ei;
        }
    }
}

/**
 * @brief Removes the filter at @p stackIndex (0 = oldest) and replays the rest.
 *
 * Drains m_visibilityHistory to a list, restores to the pre-first-filter base
 * state (the oldest snapshot), then re-applies every spec except the one at
 * @p stackIndex.  Each replay call pushes a fresh snapshot, leaving the stack
 * correct for subsequent single-step restores.
 *
 * Centrality replay requires the relevant index to still be computed; if it is
 * not, vertexFilterByCentrality() returns early and that step is skipped.
 */
void Graph::vertexFilterRemoveAt(int stackIndex)
{
    const int histSize = m_visibilityHistory.size();
    if (stackIndex < 0 || stackIndex >= histSize)
        return;

    // Transfer stack to list in application order (index 0 = oldest).
    QList<GraphVisibilitySnapshot> history;
    history.reserve(histSize);
    while (!m_visibilityHistory.isEmpty())
        history.prepend(m_visibilityHistory.pop());

    // Restore to the pre-first-filter state (history[0] was captured before
    // any filter was applied, so applying it shows all nodes again).
    applyVisibilitySnapshot(history[0]);

    // Replay every spec except the removed one.
    for (int i = 0; i < histSize; ++i)
    {
        if (i == stackIndex)
            continue;
        vertexFilterReplaySpec(history[i].spec);
    }

    progressStatus(tr("Filter removed; %1 filter(s) still active.")
                       .arg(histSize - 1));
}

/**
 * @brief Re-applies one filter from its stored replay parameters.
 *
 * Used by vertexFilterRemoveAt() during stack reconstruction.
 */
void Graph::vertexFilterReplaySpec(const FilterSpec &spec)
{
    switch (spec.type)
    {
    case FilterSpec::Type::Attribute:
        vertexFilterByAttribute(spec.condition);
        break;
    case FilterSpec::Type::Ego:
        vertexFilterByEgoNetwork(spec.egoVertex, spec.egoDepth);
        break;
    case FilterSpec::Type::Selection:
        if (!spec.selection.isEmpty())
            vertexFilterBySelection(spec.selection);
        break;
    case FilterSpec::Type::Centrality:
        vertexFilterByCentrality(spec.centralityThreshold,
                                 spec.centralityOverThreshold,
                                 spec.centralityIndex);
        break;
    case FilterSpec::Type::EdgeAttribute:
        edgeFilterByAttribute(spec.condition);
        break;
    case FilterSpec::Type::EdgeWeight:
        edgeFilterByWeight(static_cast<qreal>(spec.edgeWeightThreshold),
                           spec.edgeWeightOverThreshold);
        break;
    case FilterSpec::Type::Query:
        if (!spec.queryConditions.isEmpty())
            vertexFilterByQuery(GraphQuery{spec.queryConditions});
        break;
    case FilterSpec::Type::EdgeQuery:
        if (!spec.queryConditions.isEmpty())
            edgeFilterByQuery(GraphQuery{spec.queryConditions});
        break;
    default:
        break;
    }
}

/**
 * @brief Returns the FilterSpec list in application order (oldest first).
 *
 * Available for consumers that need to inspect the active filter sequence
 * (e.g. Phase 1 DialogQueryBuilder prefill). MainWindow currently tracks
 * chip labels separately via m_nodeFilterChips.
 * QStack<T> inherits QVector<T>, so at(0) is the bottom (oldest) entry.
 */
QList<FilterSpec> Graph::filterSpecList() const
{
    QList<FilterSpec> result;
    result.reserve(m_visibilityHistory.size());
    for (int i = 0; i < m_visibilityHistory.size(); ++i)
        result.append(m_visibilityHistory.at(i).spec);
    return result;
}


/**
 * @brief Returns true if the visibility history stack is empty.
 *
 * Used by the UI to determine whether a "Restore All" action should
 * be enabled. The stack holds one entry per non-destructive filter
 * operation (e.g. ego network focus). Each call to vertexFilterRestoreAll()
 * pops one entry; when the stack is empty, there is nothing left to restore.
 *
 * @return true if no filter snapshots are pending, false otherwise.
 * @see vertexFilterByEgoNetwork()
 * @see vertexFilterRestoreAll()
 */
bool Graph::visibilityHistoryEmpty() const
{
    return m_visibilityHistory.isEmpty();
}