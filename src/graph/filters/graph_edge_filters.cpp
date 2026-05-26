/**
 * @file graph_edge_filters.cpp
 * @brief Implements edge filtering operations for the Graph class
 *        (weight threshold filters, unilateral filters, relation-based filters).
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
#include "filter_condition.h"
#include "filter_spec.h"


/**
 * @brief Hides edges whose weight does not satisfy the threshold condition.
 *
 * Non-destructive: pushes a GraphVisibilitySnapshot onto m_visibilityHistory
 * before making any changes, so the filter can be undone via vertexFilterRemoveAt().
 *
 * @param m_threshold   The weight threshold.
 * @param overThreshold If true, keep edges with weight >= threshold (hide the rest);
 *                      if false, keep edges with weight <= threshold.
 */
void Graph::edgeFilterByWeight(const qreal m_threshold, const bool overThreshold)
{
    QString words;

    if (overThreshold)
    {
        qDebug() << "filtering edges with weight over or equal" << m_threshold;
        words = "equal or over";
    }
    else
    {
        qDebug() << "Filtering edges with weight below or equal" << m_threshold;
        words = "equal or under";
    }

    // Snapshot BEFORE making changes so the filter can be undone via vertexFilterRemoveAt().
    GraphVisibilitySnapshot snapshot;
    snapshot.active                       = true;
    snapshot.spec.type                    = FilterSpec::Type::EdgeWeight;
    snapshot.spec.edgeWeightThreshold     = static_cast<float>(m_threshold);
    snapshot.spec.edgeWeightOverThreshold = overThreshold;

    VList::const_iterator si;
    for (si = m_graph.cbegin(); si != m_graph.cend(); ++si)
    {
        const int vnum = (*si)->number();
        snapshot.nodeVisible.insert(vnum, (*si)->isEnabled());

        H_edges::const_iterator ei = (*si)->m_outEdges.constBegin();
        while (ei != (*si)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
                snapshot.arcVisible.insert(QPair<int,int>(vnum, ei.key()),
                                           ei.value().second.second);
            ++ei;
        }
    }
    m_visibilityHistory.push(snapshot);

    VList::const_iterator it;

    int source, target = 0;
    qreal weight = 0, reverseEdgeWeight = 0;
    bool preserveReverseEdge = false;
    H_edges::iterator ed;

    // Loop over all vertices
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        source = (*it)->number();

        // Loop over all out edges of source
        for (ed = (*it)->m_outEdges.begin(); ed != (*it)->m_outEdges.end(); ++ed)
        {

            // Init preserve reserve edge status to false
            preserveReverseEdge = false;

            if (ed.value().first != m_curRelation)
            {
                // This edge does not belong to this relation
                continue;
            }

            target = ed.key();
            weight = ed.value().second.first;

            // Check the filtering type: over or under
            if (overThreshold)
            {
                // We should enable only edges with weight >= threshold
                if (weight < m_threshold)
                {
                    // this outedge must be disabled - check reverse edge
                    reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                    if (reverseEdgeWeight != 0 && reverseEdgeWeight >= m_threshold)
                    {
                        // reverse edge exists and doesn't match. It must be preserved.
                        preserveReverseEdge = true;
                    }
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false));
                    // Disable the inedge of the target vertex too (needed for inDegree)
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, false, preserveReverseEdge, weight, reverseEdgeWeight);
                }
                else
                {
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true));
                    // Enable the inedge of the target vertex too (needed for inDegree)
                    this->edgeInboundStatusSet(target, source, true);
                    emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverseEdge);
                }
            }
            else
            {
                // We should enable edges with weight <= the threshold
                if (weight > m_threshold)
                {
                    // this outedge must be disabled - check reverse edge
                    reverseEdgeWeight = (*it)->hasEdgeFrom(target);
                    if (reverseEdgeWeight != 0 && reverseEdgeWeight <= m_threshold)
                    {
                        // reverse edge exists and doesn't match. It must be preserved.
                        preserveReverseEdge = true;
                    }
                    // Disable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false));
                    // Disable the inedge of the target vertex too (needed for inDegree)
                    this->edgeInboundStatusSet(target, source, false);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, false, preserveReverseEdge, weight, reverseEdgeWeight);
                }
                else
                {
                    // Enable the edge
                    ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true));
                    // Enable the inedge of the target vertex too (needed for inDegree)
                    this->edgeInboundStatusSet(target, source, true);

                    emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverseEdge);
                }
            }
        }
    }
    // Update graph mod status
    setModStatus(ModStatus::EdgeCount);
    // Emit a status message
    progressStatus(tr("Edges with weight %1 %2 have been filtered.").arg(words).arg(m_threshold));
}

/**
 * @brief Re-enables all edges in the current relation.
 *
 * Restores full edge visibility after a weight filter has been applied.
 * Non-destructive: only visibility is changed, no data is modified.
 */
void Graph::edgeFilterReset()
{
    qDebug() << "Graph::edgeFilterReset()";

    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        const int source = (*it)->number();

        H_edges::iterator ed;
        for (ed = (*it)->m_outEdges.begin(); ed != (*it)->m_outEdges.end(); ++ed)
        {
            if (ed.value().first != m_curRelation)
                continue;

            const qreal weight = ed.value().second.first;
            const qreal reverseWeight = (*it)->hasEdgeFrom(ed.key());
            const bool preserveReverse = (reverseWeight != 0);

            ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true));
            edgeInboundStatusSet(ed.key(), source, true);
            emit signalSetEdgeVisibility(m_curRelation, source, ed.key(),
                                         true, preserveReverse);
        }
    }

    setModStatus(ModStatus::EdgeCount);
    progressStatus(tr("All edges restored."));
}

/**
 * @brief Toggles (enables or disables) all edges of the given relation
 *
 * Calls the homonymous method of GraphVertex class.
 *
 * @param relation
 * @param status
 */
void Graph::edgeFilterByRelation(int relation, bool status)
{
    qDebug() << "toggling all edges in relation" << relation << "to status" << status;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
        {
            // Skip if the node is disabled.
            continue;
        }
        (*it)->setEnabledEdgesByRelation(relation, status);
    }
}

/**
 * @brief Enables or disables unilateral edges in current relationship.
 *
 * If toggle=true, all non-reciprocal edges are disabled, effectively making
 * the network symmetric.
 *
 * @param toggle
 */
void Graph::edgeFilterUnilateral(const bool &toggle)
{
    qDebug() << "Toggling unilateral edges:" << toggle;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        (*it)->setEnabledUnilateralEdges(toggle);
    }
    setModStatus(ModStatus::EdgeCount);
    progressStatus(tr("Unilateral edges have been temporarily disabled."));
}

/**
 * @brief Hides all edges whose custom attribute does not satisfy @p cond.
 *
 * Non-destructive: pushes a GraphVisibilitySnapshot onto m_visibilityHistory
 * before making any changes. Undo via vertexFilterRemoveAt() or vertexFilterRestoreAll().
 *
 * Returns early with a status message if no edge satisfies the condition.
 *
 * @param cond  The filter condition (key, operator, value).
 */
void Graph::edgeFilterByAttribute(const FilterCondition &cond)
{
    qDebug() << "Graph::edgeFilterByAttribute() key:" << cond.key
             << "op:" << static_cast<int>(cond.op) << "value:" << cond.value;

    // Count matching edges (for early-exit guard).
    int matchCount = 0;
    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        H_edges::const_iterator ei = (*vi)->m_outEdges.constBegin();
        while (ei != (*vi)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation && ei.value().second.second)
            {
                const QHash<QString,QString> attrs =
                    (*vi)->outEdgeCustomAttributes(ei.key());
                if (attrs.contains(cond.key) && cond.matches(attrs.value(cond.key)))
                    ++matchCount;
            }
            ++ei;
        }
    }

    if (matchCount == 0)
    {
        progressStatus(tr("No edges found matching: %1.").arg(cond.label()));
        return;
    }

    // Snapshot BEFORE making changes.
    GraphVisibilitySnapshot snapshot;
    snapshot.active         = true;
    snapshot.spec.type      = FilterSpec::Type::EdgeAttribute;
    snapshot.spec.condition = cond;

    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int vnum = (*vi)->number();
        snapshot.nodeVisible.insert(vnum, (*vi)->isEnabled());

        H_edges::const_iterator ei = (*vi)->m_outEdges.constBegin();
        while (ei != (*vi)->m_outEdges.constEnd())
        {
            if (ei.value().first == m_curRelation)
                snapshot.arcVisible.insert(QPair<int,int>(vnum, ei.key()),
                                           ei.value().second.second);
            ++ei;
        }
    }

    m_visibilityHistory.push(snapshot);

    // Apply: hide edges that do NOT match the condition.
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const int source = (*vi)->number();

        H_edges::iterator ei = (*vi)->m_outEdges.begin();
        while (ei != (*vi)->m_outEdges.end())
        {
            if (ei.value().first != m_curRelation) { ++ei; continue; }

            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);

            const QHash<QString,QString> attrs = (*vi)->outEdgeCustomAttributes(target);
            const bool condMet = attrs.contains(cond.key) && cond.matches(attrs.value(cond.key));

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, condMet));
            edgeInboundStatusSet(target, source, condMet);

            if (condMet)
                emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverse);
            else
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse, weight, reverseWeight);
            ++ei;
        }
    }

    progressStatus(tr("Showing %1 edge(s) matching: %2.")
                       .arg(matchCount).arg(cond.label()));
}
