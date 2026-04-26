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
// Internal helper
// ---------------------------------------------------------------------------

/**
 * @brief Returns true if @p attrValue satisfies the condition described by
 *        @p cond.
 *
 * Numeric operators (>, <, ≥, ≤) attempt a double conversion of both sides
 * and fall back to lexicographic comparison when either side is not numeric.
 * Eq / Neq always use exact string comparison.
 * Contains uses case-insensitive substring search.
 */
static bool matchesCondition(const QString &attrValue, const FilterCondition &cond)
{
    switch (cond.op) {
    case FilterCondition::Op::Eq:
        return attrValue == cond.value;
    case FilterCondition::Op::Neq:
        return attrValue != cond.value;
    case FilterCondition::Op::Contains:
        return attrValue.contains(cond.value, Qt::CaseInsensitive);
    default:
        break;
    }

    // Numeric branch
    bool okA = false, okB = false;
    const double a = attrValue.toDouble(&okA);
    const double b = cond.value.toDouble(&okB);

    if (okA && okB) {
        switch (cond.op) {
        case FilterCondition::Op::Gt:  return a >  b;
        case FilterCondition::Op::Lt:  return a <  b;
        case FilterCondition::Op::Gte: return a >= b;
        case FilterCondition::Op::Lte: return a <= b;
        default: break;
        }
    }

    // Lexicographic fallback
    switch (cond.op) {
    case FilterCondition::Op::Gt:  return attrValue >  cond.value;
    case FilterCondition::Op::Lt:  return attrValue <  cond.value;
    case FilterCondition::Op::Gte: return attrValue >= cond.value;
    case FilterCondition::Op::Lte: return attrValue <= cond.value;
    default: break;
    }
    return false;
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
        if (attrs.contains(cond.key) && matchesCondition(attrs.value(cond.key), cond))
            visibleSet.insert((*vi)->number());
    }

    if (visibleSet.isEmpty())
    {
        progressStatus(tr("No nodes found matching: %1.").arg(cond.label()));
        return;
    }

    // Snapshot current visibility state BEFORE making any changes.
    GraphVisibilitySnapshot snapshot;
    snapshot.active = true;

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

    const GraphVisibilitySnapshot snapshot = m_visibilityHistory.pop();

    // ------------------------------------------------------------------
    // PASS 1: Restore vertex visibility.
    // ------------------------------------------------------------------
    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        const bool wasVisible = snapshot.nodeVisible.value(vnum, true);
        if ((*vi)->isEnabled() != wasVisible)
        {
            (*vi)->setEnabled(wasVisible);
            setModStatus(ModStatus::VertexCount);
            emit setVertexVisibility(vnum, wasVisible);
        }
    }

    // ------------------------------------------------------------------
    // PASS 2: Restore edge visibility.
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
            const bool wasVisible =
                snapshot.arcVisible.value(QPair<int, int>(source, target), true);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, wasVisible));
            edgeInboundStatusSet(target, source, wasVisible);

            if (wasVisible)
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

    progressStatus(tr("Graph visibility restored."));
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