/**
 * @file graph_clustering_coefficients.cpp
 * @brief Implements clustering coefficient calculations for the Graph class.
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
#include <QDebug>

/**
 * @brief Returns the local clustering coefficient (CLUCOF) of vertex v1.
 *
 * For undirected (symmetric) graphs, uses the Watts–Strogatz formula:
 *
 *   C_i = 2 * |{e_jk : v_j, v_k ∈ N_i, e_jk ∈ E}| / ( k_i * (k_i - 1) )
 *
 * where N_i is the set of direct neighbours of v_i and k_i = |N_i|.
 * Because the graph is symmetric every undirected edge is counted once
 * (the reverse-edge dedup guard in the inner loop is preserved).
 *
 * For directed (asymmetric) graphs, uses the generalisation described in
 * the SocNetV manual (equivalent to Watts–Strogatz extended to digraphs):
 *
 *   C_i = |{e_jk : v_j, v_k ∈ N_i, e_jk ∈ E}| / ( k_i * (k_i - 1) )
 *
 * where N_i is the UNION of in-neighbours and out-neighbours of v_i
 * (excluding v_i itself), and each directed edge e_jk is counted
 * independently – i.e. e_jk and e_kj are distinct.
 *
 * Bug fix (issue #58): the previous implementation built N_i from
 * reciprocalEdgesHash() (only mutual ties), which gave C_i = 0 for nodes
 * whose in- and out-neighbourhood were not identical.  N_i must be the
 * full combined neighbourhood for directed networks.
 *
 * @param v1  The vertex number whose local CLUCOF is requested.
 * @return    The local clustering coefficient in [0, 1], or 0 for isolates
 *            and vertices with fewer than 2 neighbours.
 */
qreal Graph::clusteringCoefficientLocal(const int &v1)
{
    if (!isModified() && (m_graph[vpos[v1]]->hasCLC()))
    {
        qreal clucof = m_graph[vpos[v1]]->CLC();
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "Not modified. Returning cached clucof =" << clucof;
        return clucof;
    }
    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "Graph changed or clucof not yet calculated.";

    const bool isSymmetric = this->isSymmetric();

    qreal clucof = 0, denom = 0, nom = 0;
    int u1 = 0, u2 = 0, k = 0;
    H_StrToBool neighborhoodEdges; // tracks directed edges found among N_i
    neighborhoodEdges.clear();

    // ------------------------------------------------------------------
    // Build the neighbourhood N_i.
    //
    // Undirected graph: N_i = direct neighbours (same as before, via
    //   reciprocalEdgesHash which equals outEdgesEnabledHash on a symmetric
    //   graph).
    //
    // Directed graph: N_i = union of out-neighbours and in-neighbours.
    //   We use a QSet<int> to deduplicate nodes that are both.
    // ------------------------------------------------------------------

    QSet<int> neighborhood;

    if (isSymmetric)
    {
        // For undirected graphs the existing reciprocalEdgesHash() path is
        // correct – every neighbour appears in both directions.
        QHash<int, qreal> reciprocal = m_graph[vpos[v1]]->reciprocalEdgesHash();
        for (auto it = reciprocal.cbegin(); it != reciprocal.cend(); ++it)
        {
            if (it.key() != v1)
                neighborhood.insert(it.key());
        }
    }
    else
    {
        // Directed graph: collect out-neighbours.
        QHash<int, qreal> outN = m_graph[vpos[v1]]->outEdgesEnabledHash();
        for (auto it = outN.cbegin(); it != outN.cend(); ++it)
        {
            if (it.key() != v1)
                neighborhood.insert(it.key());
        }

        // Collect in-neighbours (heap-allocated – take ownership).
        QHash<int, qreal> *inN = m_graph[vpos[v1]]->inEdgesEnabledHash();
        for (auto it = inN->cbegin(); it != inN->cend(); ++it)
        {
            if (it.key() != v1)
                neighborhood.insert(it.key());
        }
        delete inN;
        inN = nullptr;
    }

    k = neighborhood.size();

    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "neighbourhood N_i size k =" << k
             << "members:" << neighborhood;

    if (k < 2)
    {
        // A node with 0 or 1 neighbour cannot form any triangle.
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "k < 2, returning 0.";
        m_graph[vpos[v1]]->setCLC(0);
        return 0;
    }

    // ------------------------------------------------------------------
    // Count edges that exist among the members of N_i.
    //
    // For undirected graphs we deduplicate {u1,u2} / {u2,u1} pairs so
    // that each undirected edge is counted once, matching the formula
    //   nom = |{e_jk}|  (unordered pairs).
    //
    // For directed graphs every ordered pair (u1→u2) is a distinct edge,
    // so we only skip exact duplicates of the same ordered string, which
    // the QHash insert naturally handles.
    // ------------------------------------------------------------------

    for (int u1_node : neighborhood)
    {
        for (int u2_node : neighborhood)
        {
            if (u1_node == u2_node)
                continue;

            u1 = u1_node;
            u2 = u2_node;

            if (edgeExists(u1, u2) != 0)
            {
                QString edge = QString::number(u1) + "->" + QString::number(u2);
                QString revedge = QString::number(u2) + "->" + QString::number(u1);

                if (isSymmetric)
                {
                    // Count each undirected edge once.
                    if (!neighborhoodEdges.contains(edge) &&
                        !neighborhoodEdges.contains(revedge))
                    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Undirected edge added:" << edge;
                    }
                }
                else
                {
                    // Count each directed edge independently.
                    if (!neighborhoodEdges.contains(edge))
                    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Directed edge added:" << edge;
                    }
                }
            }
        }
    }

    nom = neighborhoodEdges.size();
    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "edges in neighbourhood =" << nom;

    if (nom == 0)
    {
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "No edges in neighbourhood, returning 0.";
        m_graph[vpos[v1]]->setCLC(0);
        return 0;
    }

    // ------------------------------------------------------------------
    // Denominator: maximum possible edges among k neighbours.
    //
    // Undirected:  k*(k-1)/2   (unordered pairs)
    // Directed:    k*(k-1)     (ordered pairs; both u→v and v→u are valid)
    // ------------------------------------------------------------------

    if (isSymmetric)
    {
        denom = k * (k - 1.0) / 2.0;
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "Undirected graph. Max neighbourhood edges =" << denom;
    }
    else
    {
        denom = k * (k - 1.0);
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "Directed graph. Max neighbourhood edges =" << denom;
    }

    clucof = nom / denom;
    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "CLUCOF =" << clucof;

    m_graph[vpos[v1]]->setCLC(clucof);
    return clucof;
}

/**
 * @brief Computes local clustering coefficients and returns
 * the network average Clustering Coefficient
 * @param updateProgress
 * @return
 */
qreal Graph::clusteringCoefficient(const bool updateProgress)
{
    qDebug() << "Graph::clusteringCoefficient()";
    averageCLC = 0;
    varianceCLC = 0;
    maxCLC = 0;
    minCLC = 1;
    qreal temp = 0;
    qreal x = 0;
    qreal N = vertices();
    int progressCounter = 0;
    VList::const_iterator vertex;

    QString pMsg = tr("Computing Clustering Coefficient. \n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex)
    {

        if (updateProgress)
        {
            progressUpdate(++progressCounter);
        }

        temp = clusteringCoefficientLocal((*vertex)->number());

        if (temp > maxCLC)
        {
            maxCLC = temp;
            maxNodeCLC = (*vertex)->number();
        }
        if (temp < minCLC)
        {
            minNodeCLC = (*vertex)->number();
            minCLC = temp;
        }
        averageCLC += temp;
    }

    averageCLC = averageCLC / N;

    qDebug() << "Graph::clusteringCoefficient() network average " << averageCLC;

    for (vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex)
    {
        x = ((*vertex)->CLC() - averageCLC);
        x *= x;
        varianceCLC += x;
    }

    varianceCLC /= N;

    if (updateProgress)
    {
        progressFinish();
    }

    return averageCLC;
}
