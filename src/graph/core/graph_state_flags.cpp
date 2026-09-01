/**
 * @file graph_state_flags.cpp
 * @brief Implements graph-level state flags and toggles for the Graph class
 *        (weighted/symmetric/directed/undirected).
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
#include <QAtomicInteger>
#include <QtConcurrent/QtConcurrent>

/**
 * @brief Returns true if the **current relation** has at least one edge
 *        with weight other than 0 or 1 (i.e. the relation is valued/weighted).
 *
 * The result is cached via @c calculatedGraphWeighted and invalidated on
 * every relation switch (@see relationSet()). Consequently, for multi-relation
 * graphs this method only reflects the active relation — switching to a
 * non-weighted relation after a weighted one will return false.
 *
 * When you need to know whether *any* relation is weighted, use
 * @see isAnyRelationWeighted() instead.
 *
 * Complexity: O(n²) on cache miss, O(1) on hit.
 */
bool Graph::isWeighted()
{

    if (calculatedGraphWeighted)
    {
        qCDebug(lcGraphCore) << "graph not modified. Returning isWeighted: "
                 << m_graphIsWeighted;
        return m_graphIsWeighted;
    }

    // Reset before scan: only set true if a non-unit weight is found.
    // Without this, a stale true from a previous relation's edges would persist.
    m_graphIsWeighted = false;

    qreal m_weight = 0;
    VList::const_iterator it, it1;

    QString pMsg = tr("Checking if the graph edges are valued. \nPlease wait...");
    progressStatus(pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {
            m_weight = edgeExists((*it1)->number(), (*it)->number());
            if (m_weight != 1 && m_weight != 0)
            {
                setWeighted(true);
                break;
            }
        }
        if (m_graphIsWeighted)
        {
            break;
        }
    }
    calculatedGraphWeighted = true;
    qCDebug(lcGraphCore) << "graph is weighted:" << m_graphIsWeighted;

    return m_graphIsWeighted;
}

/**
 * @brief Returns true if any relation in the graph has at least one edge
 *        with weight other than 0 or 1.
 *
 * Unlike isWeighted(), which only scans the current relation, this method
 * iterates every relation. It is used to report overall graph weightedness
 * independently of which relation is currently active.
 */
bool Graph::isAnyRelationWeighted()
{
    const int savedRel = m_curRelation;
    const int numRels = relations();
    bool anyWeighted = false;

    for (int r = 0; r < numRels && !anyWeighted; ++r) {
        relationSet(r, false);
        anyWeighted = isWeighted();
    }

    if (m_curRelation != savedRel)
        relationSet(savedRel, false);

    return anyWeighted;
}

/**
 * @brief Sets the graph to be weighted ( valued edges ).
 * @param toggle
 */
void Graph::setWeighted(const bool &toggle)
{
    m_graphIsWeighted = toggle;
}

/**
 * @brief Returns TRUE if the adjacency matrix of the current relation is symmetric
 *
 * Parallelization (WS15 P4): each vertex's out-edges are checked independently against
 * their reverse arc via edgeExists() (read-only, thread-safe since WS15 P4's edgeExists()
 * fix) - same per-vertex shape as centralityDegree(). Can't short-circuit on first
 * asymmetry found under QtConcurrent::blockingMap (no shared control flow across worker
 * threads), so instead every vertex is always checked and the result is OR-reduced via
 * QAtomicInteger - strictly more work than the old early-exit in the asymmetric case, but
 * never wrong, and no slower than before in the common (symmetric) case where every vertex
 * had to be checked anyway.
 *
 * @return bool
 */
bool Graph::isSymmetric()
{
    qCDebug(lcGraphCore) << "Graph::isSymmetric() ";

    if (calculatedGraphSymmetry)
    {
        qCDebug(lcGraphCore) << "Graph::isSymmetric() - graph not modified and "
                    "already calculated symmetry. Returning previous result: "
                 << m_graphIsSymmetric;
        return m_graphIsSymmetric;
    }

    // storeRelease/loadAcquire (not relaxed load/store): makes explicit at the call site that
    // this flag crosses the worker-thread -> main-thread boundary, so every read a worker did
    // before its storeRelease is visible once the main thread's loadAcquire below sees it.
    // blockingMap() already joins every worker before returning, so this is technically
    // redundant with that barrier - kept for self-documentation and to match the same pattern
    // used in centralityDegree()'s asymmetric flag.
    QAtomicInteger<bool> asymmetric{false};

    QtConcurrent::blockingMap(m_graph, [&](GraphVertex *v) {
        if (!v->isEnabled())
            return;

        const int v1 = v->number();
        const QHash<int, qreal> enabledOutEdges = v->outEdgesEnabledHash();

        for (auto hit = enabledOutEdges.cbegin(); hit != enabledOutEdges.cend(); ++hit)
        {
            const int v2 = hit.key();
            const qreal weight = hit.value();

            if (edgeExists(v2, v1) != weight)
            {
                asymmetric.storeRelease(true);
                break;
            }
        }
    });

    m_graphIsSymmetric = !asymmetric.loadAcquire();
    qCDebug(lcGraphCore) << "Graph: isSymmetric() - Finished. Result:" << m_graphIsSymmetric;
    calculatedGraphSymmetry = true;
    return m_graphIsSymmetric;
}

/**
 * @brief Transforms the graph to symmetric (all edges reciprocal)
 */
void Graph::setSymmetric()
{
    qCDebug(lcGraphCore) << "Tranforming graph to symmetric...";
    VList::const_iterator it;
    int v2 = 0, v1 = 0, weight;
    qreal invertWeight = 0;
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            v2 = it1.key();
            weight = it1.value();
            invertWeight = edgeExists(v2, v1);
            if (invertWeight == 0)
            {
                edgeCreate(v2, v1, weight, initEdgeColor, false, true, false,
                           QString(), false);
            }
            else
            {
                if (weight != invertWeight)
                    edgeWeightSet(v2, v1, weight);
            }

            ++it1;
        }
    }
    // delete enabledOutEdges;

    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Toggles the graph directed or undirected
 *
 * @param toggle
 * @param signalMW
 */
void Graph::setDirected(const bool &toggle, const bool &signalMW)
{
    qCDebug(lcGraphCore) << "Graph::setDirected - Setting graph directed to:" << toggle;
    if (!toggle)
    {
        setUndirected(true, signalMW);
        return;
    }
    if (toggle == isDirected())
    {
        qCDebug(lcGraphCore) << "Graph::setDirected - Same as now, nothing to do.";
        return;
    }
    m_graphIsDirected = true;
    if (m_curRelation >= 0 && m_curRelation < m_relationsDirected.size())
        m_relationsDirected[m_curRelation] = true;
    setModStatus(ModStatus::EdgeCount, signalMW);
}

/**
 * @brief Makes the graph undirected or directed.
 *
 * @param toggle
 * @param signalMW
 */
void Graph::setUndirected(const bool &toggle, const bool &signalMW)
{
    qCDebug(lcGraphCore) << "Graph::setUndirected - Toggling graph undirected to" << toggle;
    qCDebug(lcGraphCore) << "Graph::setUndirected - m_graphIsSymmetric:" << m_graphIsSymmetric
             << "m_graphIsDirected:" << m_graphIsDirected
             << "m_totalEdges:" << m_totalEdges
             << "calculatedEdges:" << calculatedEdges;

    if (!toggle)
    {
        setDirected(true);
        return;
    }
    if (toggle == isUndirected())
    {
        qCDebug(lcGraphCore) << "Graph::setUndirected - Same as now, nothing to do.";
        return;
    }
    // NOTE: We set m_graphIsDirected = false BEFORE the loop so that
    // edgeTypeSet() and other callers see the correct state immediately.
    m_graphIsDirected = false;
    if (m_curRelation >= 0 && m_curRelation < m_relationsDirected.size())
        m_relationsDirected[m_curRelation] = false;

    // Only add reverse arcs if the graph was previously directed.
    // If the graph was already loaded with symmetric (undirected) arcs
    // (e.g. from a DOT 'graph' file), the reverse arcs already exist
    // and this loop would double them (see issue #187).
    if (!m_graphIsSymmetric)
    {
        VList::const_iterator it;
        int v2 = 0, v1 = 0;
        qreal weight;
        QHash<int, qreal> enabledOutEdges;
        QHash<int, qreal>::const_iterator it1;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            v1 = (*it)->number();
            qCDebug(lcGraphCore) << "Graph::setUndirected - Iterating over edges of v1 " << v1;
            enabledOutEdges = (*it)->outEdgesEnabledHash();
            it1 = enabledOutEdges.cbegin();
            while (it1 != enabledOutEdges.cend())
            {
                v2 = it1.key();
                weight = it1.value();
                qCDebug(lcGraphCore) << "edge" << "v1" << v1 << "->" << v2 << " = " << "weight" << weight;
                edgeTypeSet(v1, v2, weight, EdgeType::Undirected);
                ++it1;
            }
        }
    }
    else
    {
        qCDebug(lcGraphCore) << "Graph::setUndirected -Graph already has symmetric arcs (m_graphIsSymmetric=true); skipping reverse-arc addition.";
    }

    m_graphIsSymmetric = true;
    setModStatus(ModStatus::EdgeCount, signalMW);
}

/**
 * @brief Returns true if graph is directed
 *
 * @return bool
 */
bool Graph::isDirected()
{
    qCDebug(lcGraphCore) << "Graph::isDirected m_graphIsDirected" << m_graphIsDirected;
    return m_graphIsDirected;
}

/**
 * @brief Returns true if graph is undirected
 *
 * @return bool
 */
bool Graph::isUndirected()
{
    return !m_graphIsDirected;
}
