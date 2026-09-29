/**
 * @file graph_structural_balance.cpp
 * @brief Implements Heider/Cartwright-Harary structural balance classification of closed triads
 * for undirected signed graphs.
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
#include <QtConcurrent/QtConcurrent>

/**
 * @brief Classifies every closed triad (all 3 dyads present) of an undirected signed graph as
 * balanced or unbalanced by the product-of-signs rule (Heider/Cartwright-Harary): positive
 * product of the 3 edge signs is balanced (0 or 2 negative edges), negative product is
 * unbalanced (1 or 3 negative edges). Triples with fewer than 3 dyads present ("open") are
 * counted separately and excluded from the balance ratio, matching the classical definition,
 * which is only stated for complete (closed) triads - see e.g. R's signnet::balance_score()
 * ("triangles" method) and count_signed_triangles(), which count complete triangles only.
 *
 * Undirected only: Cartwright-Harary balance theory is classically defined for undirected
 * signed graphs. Directed-graph semantics are a separate, deferred design question (#303).
 *
 * Signed only: also refuses on a graph with no negative-weight edge at all. Every edge would
 * implicitly be positive, so every closed triad would trivially classify as "+++"/balanced -
 * a mathematically correct but misleading 100%-balanced result for a caller with no
 * signed-network intent, not an actual finding about the network's structure. Uses
 * hasNegativeWeight() (cached O(n^2)/O(1)), same guard style as P1's negative-weight refusal.
 *
 * Complexity: O(n^3), same shape as graphTriadCensus() - three nested loops each bounded by N,
 * parallelized the same way (QtConcurrent::blockingMap over the outer vertex loop, one
 * QAtomicInteger<int> counter per classification bucket to avoid a data race across worker
 * threads).
 *
 * @return false if the graph is directed or has no negative-weight edge (refuses cleanly, does
 * not compute), true otherwise
 */
bool Graph::graphStructuralBalance()
{
    qCDebug(lcClustering) << "Graph::graphStructuralBalance()";

    if (isDirected())
    {
        qCDebug(lcClustering) << "Graph::graphStructuralBalance() - graph is directed, refusing. "
                                  "Structural balance is defined for undirected signed graphs only.";
        calculatedStructuralBalance = false;
        return false;
    }

    if (!hasNegativeWeight())
    {
        qCDebug(lcClustering) << "Graph::graphStructuralBalance() - graph has no negative-weight "
                                  "edge, refusing. Structural balance requires a signed network.";
        calculatedStructuralBalance = false;
        return false;
    }

    QString pMsg = tr("Computing Structural Balance. \nPlease wait...");
    progressStatus(pMsg);

    if (progressCanceled())
    {
        calculatedStructuralBalance = false;
        return false;
    }

    const int N = m_graph.size();

    // Bucket order: 0="+++" 1="++-" 2="+--" 3="---" (sign counts along each closed triad's 3
    // dyads, canonicalized by negative-edge count so traversal order of the 3 vertices doesn't
    // matter - a triad with exactly one negative edge is always bucket 1 regardless of which of
    // the 3 dyads that edge is). Bucket 4 = open (fewer than 3 dyads present).
    QVector<QAtomicInteger<int>> balanceCounts(5);

    QList<int> positions;
    positions.reserve(N);
    for (int i = 0; i < N; ++i)
        positions.append(i);

    QtConcurrent::blockingMap(positions, [&](int i) {
        GraphVertex *v1vert = m_graph.at(i);

        for (int j = i + 1; j < N; ++j)
        {
            GraphVertex *v2vert = m_graph.at(j);
            const int ver2 = v2vert->number();

            const qreal w12 = v1vert->hasEdgeTo(ver2);

            for (int k = j + 1; k < N; ++k)
            {
                GraphVertex *v3vert = m_graph.at(k);
                const int ver3 = v3vert->number();

                const qreal w13 = v1vert->hasEdgeTo(ver3);
                const qreal w23 = v2vert->hasEdgeTo(ver3);

                const int present = (w12 != 0 ? 1 : 0) + (w13 != 0 ? 1 : 0) + (w23 != 0 ? 1 : 0);

                if (present < 3)
                {
                    balanceCounts[4].fetchAndAddOrdered(1); // open
                    continue;
                }

                const int negCount = (w12 < 0 ? 1 : 0) + (w13 < 0 ? 1 : 0) + (w23 < 0 ? 1 : 0);
                balanceCounts[negCount].fetchAndAddOrdered(1);
            } // end 3rd loop
        } // end 2nd loop
    });

    structuralBalanceCounts.clear();
    for (int i = 0; i < 5; ++i)
        structuralBalanceCounts.append(balanceCounts[i].loadAcquire());

    calculatedStructuralBalance = true;

    return true;
}
