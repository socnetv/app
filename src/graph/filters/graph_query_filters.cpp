/**
 * @file graph_query_filters.cpp
 * @brief Implements compound AND-query filters for nodes and edges (#221 Phase 1).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "graph.h"
#include "graph_query.h"
#include "filter_spec.h"
#include <QDebug>


/**
 * @brief Shows only vertices whose custom attributes satisfy ALL conditions in @p query.
 *
 * AND logic: a vertex must pass every condition to remain visible.
 * Non-destructive: pushes a GraphVisibilitySnapshot with FilterSpec::Type::Query
 * before making any changes. Undo via vertexFilterRemoveAt() or vertexFilterRestoreAll().
 *
 * Follows the same two-pass pattern as vertexFilterByAttribute() so edge visibility
 * is resolved after all vertex states are final.
 *
 * @param query  Compound query; all conditions should have Scope::Nodes.
 */
void Graph::vertexFilterByQuery(const GraphQuery &query)
{
    if (query.conditions.isEmpty())
        return;

    qDebug() << "Graph::vertexFilterByQuery()" << query.conditions.size() << "condition(s)";

    // Build visible set: vertices satisfying ALL conditions.
    QSet<int> visibleSet;
    VList::const_iterator vi;
    for (vi = m_graph.cbegin(); vi != m_graph.cend(); ++vi)
    {
        const QHash<QString,QString> attrs = (*vi)->customAttributes();
        bool allMatch = true;
        for (const auto &cond : std::as_const(query.conditions))
        {
            if (!attrs.contains(cond.key) || !cond.matches(attrs.value(cond.key)))
            {
                allMatch = false;
                break;
            }
        }
        if (allMatch)
            visibleSet.insert((*vi)->number());
    }

    if (visibleSet.isEmpty())
    {
        progressStatus(tr("No nodes match all %1 condition(s).").arg(query.conditions.size()));
        return;
    }

    // Snapshot BEFORE making changes.
    GraphVisibilitySnapshot snapshot;
    snapshot.active               = true;
    snapshot.spec.type            = FilterSpec::Type::Query;
    snapshot.spec.queryConditions = query.conditions;

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
            if (ei.value().first != m_curRelation) { ++ei; continue; }

            const int target = ei.key();
            const qreal weight = ei.value().second.first;
            const qreal reverseWeight = (*vi)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseWeight != 0);
            const bool edgeVisible = visibleSet.contains(source) && visibleSet.contains(target);

            ei.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeVisible));
            edgeInboundStatusSet(target, source, edgeVisible);

            if (edgeVisible)
                emit signalSetEdgeVisibility(m_curRelation, source, target, true, preserveReverse);
            else
                emit signalSetEdgeVisibility(m_curRelation, source, target,
                                             false, preserveReverse, weight, reverseWeight);
            ++ei;
        }
    }

    progressStatus(tr("Query: showing %1 node(s) matching all %2 condition(s).")
                       .arg(visibleSet.size()).arg(query.conditions.size()));
}


/**
 * @brief Shows only edges whose custom attributes satisfy ALL conditions in @p query.
 *
 * AND logic: an edge must pass every condition to remain visible.
 * Non-destructive: pushes a GraphVisibilitySnapshot with FilterSpec::Type::EdgeQuery.
 * Undo via vertexFilterRemoveAt() or vertexFilterRestoreAll().
 *
 * @param query  Compound query; all conditions should have Scope::Edges.
 */
void Graph::edgeFilterByQuery(const GraphQuery &query)
{
    if (query.conditions.isEmpty())
        return;

    qDebug() << "Graph::edgeFilterByQuery()" << query.conditions.size() << "condition(s)";

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
                bool allMatch = true;
                for (const auto &cond : std::as_const(query.conditions))
                {
                    if (!attrs.contains(cond.key) || !cond.matches(attrs.value(cond.key)))
                    {
                        allMatch = false;
                        break;
                    }
                }
                if (allMatch)
                    ++matchCount;
            }
            ++ei;
        }
    }

    if (matchCount == 0)
    {
        progressStatus(tr("No edges match all %1 condition(s).").arg(query.conditions.size()));
        return;
    }

    // Snapshot BEFORE making changes.
    GraphVisibilitySnapshot snapshot;
    snapshot.active               = true;
    snapshot.spec.type            = FilterSpec::Type::EdgeQuery;
    snapshot.spec.queryConditions = query.conditions;

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

    // Apply: hide edges not satisfying ALL conditions.
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
            bool condMet = true;
            for (const auto &cond : std::as_const(query.conditions))
            {
                if (!attrs.contains(cond.key) || !cond.matches(attrs.value(cond.key)))
                {
                    condMet = false;
                    break;
                }
            }

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

    progressStatus(tr("Query: showing %1 edge(s) matching all %2 condition(s).")
                       .arg(matchCount).arg(query.conditions.size()));
}
