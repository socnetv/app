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

/**
 * @brief Returns true if the graph is weighted (valued),
 * i.e. if any e in |E| has value not 0 or 1
 *  Complexity: O(n^2)
 * @return
 */
bool Graph::isWeighted()
{

    if (calculatedGraphWeighted)
    {
        qDebug() << "graph not modified. Returning isWeighted: "
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
    qDebug() << "graph is weighted:" << m_graphIsWeighted;

    return m_graphIsWeighted;
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
 * @return bool
 */
bool Graph::isSymmetric()
{
    qDebug() << "Graph::isSymmetric() ";

    if (calculatedGraphSymmetry)
    {
        qDebug() << "Graph::isSymmetric() - graph not modified and "
                    "already calculated symmetry. Returning previous result: "
                 << m_graphIsSymmetric;
        return m_graphIsSymmetric;
    }
    m_graphIsSymmetric = true;
    int v2 = 0, v1 = 0;
    qreal weight = 0;

    QHash<int, qreal> enabledOutEdges;

    QHash<int, qreal>::const_iterator hit;
    VList::const_iterator lit;

    for (lit = m_graph.cbegin(); lit != m_graph.cend(); ++lit)
    {
        v1 = (*lit)->number();

        if (!(*lit)->isEnabled())
            continue;

        enabledOutEdges = (*lit)->outEdgesEnabledHash();

        hit = enabledOutEdges.cbegin();

        while (hit != enabledOutEdges.cend())
        {

            v2 = hit.key();
            weight = hit.value();

            if (edgeExists(v2, v1) != weight)
            {

                m_graphIsSymmetric = false;
                //                qDebug() <<"Graph::isSymmetric() - "
                //                         << " graph not symmetric because "
                //                         << v1 << "->" << v2 << " weight " << weight
                //                         << " differs from " << v2 << "->" << v1 ;

                break;
            }
            ++hit;
        }
    }
    // delete enabledOutEdges;
    qDebug() << "Graph: isSymmetric() - Finished. Result:" << m_graphIsSymmetric;
    calculatedGraphSymmetry = true;
    return m_graphIsSymmetric;
}

/**
 * @brief Transforms the graph to symmetric (all edges reciprocal)
 */
void Graph::setSymmetric()
{
    qDebug() << "Tranforming graph to symmetric...";
    VList::const_iterator it;
    int v2 = 0, v1 = 0, weight;
    qreal invertWeight = 0;
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        //        qDebug() << "iterate over edges of v1 " << v1;
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            v2 = it1.key();
            weight = it1.value();
            //            qDebug() << "v1" << v1 << "outLinked to" << v2 << ", weight:" << weight;
            invertWeight = edgeExists(v2, v1);
            if (invertWeight == 0)
            {
                //                qDebug() << "v1" << v1 << "is NOT inLinked from v2" <<  v2  ;
                edgeCreate(v2, v1, weight, initEdgeColor, false, true, false,
                           QString(), false);
            }
            else
            {
                //                qDebug() << "v1" << v1 << "is inLinked from v2" <<  v2  ;
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
    qDebug() << "Graph::setDirected - Setting graph directed to:" << toggle;
    if (!toggle)
    {
        setUndirected(true, signalMW);
        return;
    }
    if (toggle == isDirected())
    {
        qDebug() << "Graph::setDirected - Same as now, nothing to do.";
        return;
    }
    m_graphIsDirected = true;
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
    qDebug() << "Graph::setUndirected - Toggling graph undirected to" << toggle;
    qDebug() << "Graph::setUndirected - m_graphIsSymmetric:" << m_graphIsSymmetric
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
        qDebug() << "Graph::setUndirected - Same as now, nothing to do.";
        return;
    }
    // NOTE: We set m_graphIsDirected = false BEFORE the loop so that
    // edgeTypeSet() and other callers see the correct state immediately.
    m_graphIsDirected = false;

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
            qDebug() << "Graph::setUndirected - Iterating over edges of v1 " << v1;
            enabledOutEdges = (*it)->outEdgesEnabledHash();
            it1 = enabledOutEdges.cbegin();
            while (it1 != enabledOutEdges.cend())
            {
                v2 = it1.key();
                weight = it1.value();
                qDebug() << "edge" << "v1" << v1 << "->" << v2 << " = " << "weight" << weight;
                edgeTypeSet(v1, v2, weight, EdgeType::Undirected);
                ++it1;
            }
        }
    }
    else
    {
        qDebug() << "Graph::setUndirected -Graph already has symmetric arcs (m_graphIsSymmetric=true); skipping reverse-arc addition.";
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
    qDebug() << "Graph::isDirected m_graphIsDirected" << m_graphIsDirected;
    return m_graphIsDirected;
}

/**
 * @brief Returns true if graph is undirected
 *
 * @return bool
 */
bool Graph::isUndirected()
{
    //    qDebug() << "isUndirected: " << !m_graphIsDirected;
    return !m_graphIsDirected;
}
