/**
 * @file graph_matrix_adjacency.cpp
 * @brief Implements Graph methods for building adjacency-based matrices (adjacency and inverse adjacency) from the current network state.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
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
#include <QtConcurrent/QtConcurrent>

/**
 * @brief  Creates an adjacency matrix AM
 *  where AM(i,j)=1 if i is connected to j
 *  and AM(i,j)=0 if i not connected to j
 *
 * Parallelization (WS15 P4): unlike the other three matrix-fill candidates, this one has no
 * APSP dependency - it's a direct O(N²) edgeExists() scan (halved via the upper-triangle
 * trick: each outer vertex i fills both AM(i,j) and AM(j,i) for j>=i in one pass). Maps via
 * QtConcurrent::blockingMap over vertex positions using compactedMatrixIndex(), same as the
 * other three. Each worker thread writes row i (cells j=i..N-1) *and* column i (cells
 * j=0..i-1, via the paired AM(j,i) writes) - not confined to a single row like the other
 * candidates, but still race-free: the upper-triangle partition (i<=j) guarantees no two
 * different outer vertices ever write the same (row,col) cell. edgeExists() itself is
 * read-only/thread-safe (fixed under WS15 P4's first implementation, see graph_edges.cpp).
 * Single progressCanceled() check before the parallel step, not per-vertex as before.
 *
 * @param dropIsolates
 * @param considerWeights
 * @param inverseWeights
 * @param symmetrize
 */
void Graph::createMatrixAdjacency(const bool dropIsolates,
                                  const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool symmetrize)
{
    qCDebug(lcGraphMatrices) << "Graph::createMatrixAdjacency() "
             << "dropIsolates" << dropIsolates
             << "considerWeights" << considerWeights
             << "inverseWeights" << inverseWeights
             << "symmetrize" << symmetrize;
    int N = vertices(dropIsolates, false, true);

    qCDebug(lcGraphMatrices) << "Graph::createMatrixAdjacency() -resizing AM to" << N;
    AM.resize(N, N);

    QString pMsg = tr("Creating Adjacency Matrix. \nPlease wait...");
    progressStatus(pMsg);

    if (progressCanceled())
    {
        return;
    }

    const QVector<int> rowOf = compactedMatrixIndex(dropIsolates);
    const int total = m_graph.size();

    QList<int> positions;
    positions.reserve(total);
    for (int p = 0; p < total; ++p)
        positions.append(p);

    QtConcurrent::blockingMap(positions, [&](int p1) {
        const int i = rowOf[p1];
        if (i < 0)
            return;

        GraphVertex *v1 = m_graph.at(p1);
        qreal m_weight = RAND_MAX;

        for (int p2 = p1; p2 < total; ++p2)
        {
            const int j = rowOf[p2];
            if (j < 0)
                continue;

            GraphVertex *v2 = m_graph.at(p2);

            if ((m_weight = edgeExists(v1->number(), v2->number())) != 0)
            {
                if (!considerWeights)
                    AM.setItem(i, j, 1);
                else if (inverseWeights)
                    AM.setItem(i, j, 1.0 / m_weight);
                else
                    AM.setItem(i, j, m_weight);
            }
            else
            {
                AM.setItem(i, j, 0);
            }

            if (i != j)
            {
                if ((m_weight = edgeExists(v2->number(), v1->number())) != 0)
                {
                    if (!considerWeights)
                        AM.setItem(j, i, 1);
                    else if (inverseWeights)
                        AM.setItem(j, i, 1.0 / m_weight);
                    else
                        AM.setItem(j, i, m_weight);

                    if (symmetrize && (AM.item(i, j) != AM.item(j, i)))
                        AM.setItem(i, j, AM.item(j, i));
                }
                else
                {
                    AM.setItem(j, i, 0);
                    if (symmetrize && (AM.item(i, j) != AM.item(j, i)))
                        AM.setItem(j, i, AM.item(i, j));
                }
            }
        }
    });

    calculatedAdjacencyMatrix = true;
}

/**
 * @brief Computes the inverse of the current adjacency matrix
 * @param method
 * @return
 */
bool Graph::createMatrixAdjacencyInverse(const QString &method)
{
    qCDebug(lcGraphMatrices) << "Graph::createMatrixAdjacencyInverse() ";

    bool considerWeights = false;
    bool dropIsolates = true; // always drop isolates else AM will be singular

    int N = vertices(dropIsolates, false, true);

    createMatrixAdjacency(dropIsolates, considerWeights);
    if (progressCanceled())
    {
        return false;
    }
    invAM.resize(N, N);

    if (method == "gauss")
    {
        // Unreachable in the current codebase - every caller passes "lu" - kept for
        // cross-checking only. inverseByGaussJordanElimination() has no singularity signal
        // of its own, so this keeps the after-the-fact "any nonzero entry" scan Fix #269
        // replaced on the "lu" path below.
        invAM.inverseByGaussJordanElimination(AM);

        int i = 0, j = 0;
        bool isSingular = true;
        VList::const_iterator it, it1;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled() || (*it)->isIsolated())
                continue;
            j = 0;
            for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
            {
                if (!(*it1)->isEnabled() || (*it1)->isIsolated())
                    continue;
                if (invAM.item(i, j) != 0)
                    isSingular = false;
                j++;
            }
            i++;
        }
        return !isSingular;
    }

    // Fix #269: inverse() now reports genuine singularity via a real relative pivot-magnitude
    // check in ludcmp(), instead of the weak "any nonzero entry in the result" heuristic above.
    const bool invertible = invAM.inverse(AM, [this] { return progressCanceled(); });
    if (progressCanceled())
    {
        return false;
    }

    return invertible;
}
