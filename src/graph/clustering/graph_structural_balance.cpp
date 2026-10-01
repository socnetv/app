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
#include <QQueue>

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

/**
 * @brief Tests structural balance's strong theorem (Cartwright-Harary #304): an undirected
 * signed graph is fully balanced (every cycle positive, not just every closed triad - see
 * graphStructuralBalance()'s per-triad classification above for the weaker, local question) if
 * and only if its vertices can be partitioned into exactly two factions such that every positive
 * edge stays inside a faction and every negative edge crosses between factions.
 *
 * Genuinely distinct from graphStructuralBalance(): a graph can have zero closed triads at all
 * (e.g. a 4-cycle with signs +,+,+,- where only 4 of 6 possible pairs are tied) and still fail
 * this test, because full balance is a property of every cycle in the graph, not just 3-vertex
 * ones - graphStructuralBalance() would report "nothing to classify" on such a graph while this
 * function correctly finds it not clusterizable.
 *
 * Algorithm: BFS/2-coloring, the same shape as the classical "is this graph bipartite" check -
 * pick an unvisited vertex, assign it faction 0; for each of its edges, a positive edge forces
 * the neighbor onto the same faction, a negative edge forces the opposite faction; if a neighbor
 * is already assigned and the forced faction contradicts it, the graph is not clusterizable.
 * Repeats from any remaining unvisited vertex to cover disconnected components (each one
 * independently 2-colorable; isolated vertices trivially land on faction 0). This reduces the
 * problem to the classical graph-bipartiteness question, decidable in P (not NP-hard - contrast
 * the frustration index below); BFS/2-coloring is the standard exact algorithm for it and is
 * asymptotically optimal, O(V+E) - every edge must be examined at least once to know whether it
 * violates the coloring, and every vertex at least once to place isolates, so no exact algorithm
 * can do better in the worst case. (Other signed-graph balance algorithms exist - e.g. an
 * eigenvalue-based score - but those answer a different, continuous question and are typically
 * superlinear, not a faster way to answer this yes/no one.)
 *
 * Deliberately does NOT compute the frustration index (the minimum number of ties that would
 * need to change to make an unbalanced graph balanced) - that is a separate, NP-hard question
 * needing an LP/ILP solver, not a graph walk. See the WS18 roadmap doc.
 *
 * Same undirected/signed guards as graphStructuralBalance() - see its own doc comment for why.
 *
 * @return false if the graph is directed, has no negative-weight edge, or is not clusterizable
 * into two factions; true if it is (in which case each GraphVertex::faction() holds its 0/1
 * assignment - same per-vertex-field convention as signedDegreePos() etc., not a parallel list
 * on Graph)
 */
bool Graph::graphClusterizability()
{
    qCDebug(lcClustering) << "Graph::graphClusterizability()";

    if (isDirected())
    {
        qCDebug(lcClustering) << "Graph::graphClusterizability() - graph is directed, refusing.";
        calculatedClusterizability = false;
        return false;
    }

    if (!hasNegativeWeight())
    {
        qCDebug(lcClustering) << "Graph::graphClusterizability() - graph has no negative-weight "
                                  "edge, refusing.";
        calculatedClusterizability = false;
        return false;
    }

    const int N = m_graph.size();
    const int currentRelation = relationCurrent();

    QHash<int, int> faction; // vertex number -> 0/1, only for visited vertices
    faction.reserve(N);

    bool clusterizable = true;

    for (auto startIt = m_graph.cbegin(); startIt != m_graph.cend() && clusterizable; ++startIt)
    {
        const int startNum = (*startIt)->number();
        if (faction.contains(startNum))
            continue; // already visited in an earlier component's BFS

        faction[startNum] = 0;
        QQueue<int> queue;
        queue.enqueue(startNum);

        while (!queue.isEmpty() && clusterizable)
        {
            const int uNum = queue.dequeue();
            GraphVertex *uVert = vertexPtr(uNum);
            const int uFaction = faction.value(uNum);

            // Undirected graph (guarded above) stores each tie as symmetric out-edges on both
            // endpoints (see Graph::edgeCreate()/addOutEdge() for EdgeType::Undirected), so
            // iterating uVert's own out-edges alone - not every other vertex in the graph -
            // already finds every neighbor: true O(degree(u)) per dequeue, not O(N). Same single-
            // pass pattern as graphDistancesGeodesic()'s BFS (graph_distance_facade.cpp): read
            // relation/weight/enabled straight off the iterator instead of a second hasEdgeTo()
            // lookup per neighbor.
            for (auto eit = uVert->outEdges().cbegin(); eit != uVert->outEdges().cend() && clusterizable; ++eit)
            {
                if (eit.value().first != currentRelation)
                    continue; // wrong relation
                if (!eit.value().second.second)
                    continue; // edge disabled

                const int vNum = eit.key();
                const qreal w = eit.value().second.first;
                if (w == 0)
                    continue;

                const int forcedFaction = (w > 0) ? uFaction : (1 - uFaction);

                if (faction.contains(vNum))
                {
                    if (faction.value(vNum) != forcedFaction)
                    {
                        clusterizable = false;
                        break;
                    }
                }
                else
                {
                    faction[vNum] = forcedFaction;
                    queue.enqueue(vNum);
                }
            }
        }
    }

    calculatedClusterizability = clusterizable;

    if (clusterizable)
    {
        for (auto it = m_graph.cbegin(); it != m_graph.cend(); ++it)
            (*it)->setFaction(faction.value((*it)->number(), 0));
    }

    return clusterizable;
}
