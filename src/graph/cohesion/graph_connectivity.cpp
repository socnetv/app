/**
 * @file graph_connectivity.cpp
 * @brief Implements local and global vertex connectivity (Menger's theorem via max-flow) for the Graph class.
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
#include "graphvertex.h"
#include <QDebug>
#include <QHash>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QVector>
#include <algorithm>
#include <climits>
#include <limits>

namespace {

// ----------------------------------------------------------------------------
// Adjacency helpers, shared by every function below. "respectDirection" is the same
// weak/strong choice Graph::graphStronglyConnectedComponents() and the GUI's Connectedness
// prompt use: true respects the graph's actual edge directions (directed reachability, matching
// graphDistanceGeodesic()); false treats every edge as bidirectional (matching
// graphWeaklyConnectedComponents()'s "weak" convention). Undirected graphs only ever call these
// with respectDirection == false in practice (there's nothing to ask), which naturally collapses
// to plain undirected adjacency since every edge already exists in both directions.
// ----------------------------------------------------------------------------

QSet<int> outNeighborsOf(Graph &g, int v, int currentRelation)
{
    QSet<int> result;
    const int vi = g.vertexIndexByNumber(v);
    if (vi < 0)
        return result;
    const GraphVertex *gv = g.vertexAtIndex(vi);
    for (auto eit = gv->outEdges().cbegin(); eit != gv->outEdges().cend(); ++eit) {
        if (eit.value().first != currentRelation) continue;
        if (!eit.value().second.second) continue;
        result.insert(eit.key());
    }
    return result;
}

QSet<int> inNeighborsOf(Graph &g, int v, int currentRelation)
{
    QSet<int> result;
    const int vi = g.vertexIndexByNumber(v);
    if (vi < 0)
        return result;
    const GraphVertex *gv = g.vertexAtIndex(vi);
    for (auto eit = gv->inEdges().cbegin(); eit != gv->inEdges().cend(); ++eit) {
        if (eit.value().first != currentRelation) continue;
        if (!eit.value().second.second) continue;
        result.insert(eit.key());
    }
    return result;
}

// Union of out- and in-neighbors when respectDirection is false, so a reciprocated or
// one-directional pair still counts as exactly one neighbor - this is the "underlying simple
// undirected graph" adjacency that weak-mode connectivity is defined over.
QSet<int> neighborsOf(Graph &g, int v, int currentRelation, bool respectDirection)
{
    QSet<int> result = outNeighborsOf(g, v, currentRelation);
    if (!respectDirection)
        result.unite(inNeighborsOf(g, v, currentRelation));
    return result;
}

bool adjacent(Graph &g, int u, int v, int currentRelation, bool respectDirection)
{
    return outNeighborsOf(g, u, currentRelation).contains(v)
        || (!respectDirection && inNeighborsOf(g, u, currentRelation).contains(v));
}

QList<int> enabledVertexNumbers(Graph &g)
{
    QList<int> verts;
    for (auto it = g.verticesBegin(); it != g.verticesEnd(); ++it) {
        if ((*it)->isEnabled())
            verts.append((*it)->number());
    }
    return verts;
}

// ----------------------------------------------------------------------------
// Vertex-split max-flow, implementing local vertex connectivity via Menger's theorem.
//
// Menger's theorem: for non-adjacent s,t, the maximum number of internally vertex-disjoint
// s-to-t paths equals the minimum number of vertices (other than s,t) whose removal disconnects
// them - exactly the local vertex connectivity kappa(s,t) we want. "Internally vertex-disjoint"
// paths turn into a max-flow problem via vertex splitting: every vertex v becomes two flow-graph
// nodes v_in and v_out joined by an edge of capacity 1 (routing a unit of flow through v now
// costs exactly one unit of the thing we're counting: vertex removals), and every real graph edge
// u->v becomes an edge u_out->v_in of effectively infinite capacity (so only vertex splits, never
// edges, constrain the flow). s and t get an infinite-capacity split instead of capacity 1, since
// they can never be part of their own separating set. The max-flow value from s_out to t_in is
// then exactly kappa(s,t).
//
// Max-flow is computed via Edmonds-Karp (repeated BFS shortest augmenting path). This is not the
// asymptotically fastest max-flow algorithm in general (Dinic's or push-relabel do better on
// dense graphs with large capacities), but it's the right fit here specifically: every capacity
// in this flow graph is either 1 (vertex splits) or effectively infinite (real edges and s/t's
// splits), and s,t are non-adjacent by construction (checked by the caller), so every s-to-t path
// must cross at least one capacity-1 split edge - meaning every augmenting path found has
// bottleneck exactly 1. The number of BFS rounds therefore equals the final flow value directly,
// which is bounded by min(|out-neighbors of s|, |in-neighbors of t|) <= n-2. At SocNetV's network
// sizes this is a handful to a few hundred BFS passes at most, each O(V+E) - trivially fast, and
// far simpler to get right than the algorithms that exist to shave complexity that doesn't matter
// at this scale.
struct FlowEdge
{
    int to;
    qint64 cap;
};

void addFlowEdge(QVector<FlowEdge> &edges, QVector<QVector<int>> &adjList, int from, int to, qint64 cap)
{
    adjList[from].append(edges.size());
    edges.append(FlowEdge{to, cap});
    adjList[to].append(edges.size());
    edges.append(FlowEdge{from, 0}); // paired reverse residual edge, capacity 0 until flow uses the forward one
}

// One Edmonds-Karp round: BFS for a shortest s->t path over edges with remaining capacity, then
// push flow equal to the path's bottleneck capacity. Because edges[i] and edges[i^1] are always
// the forward/reverse pair for the same underlying connection (addFlowEdge() always appends them
// together), pushing flow on edges[i] and crediting it back on edges[i^1] is all the residual-
// capacity bookkeeping max-flow needs - no separate "flow used" structure required. Returns the
// amount of flow pushed, or 0 once no augmenting path remains (max-flow reached).
qint64 augmentOnce(int numFlowNodes, const QVector<QVector<int>> &adjList, QVector<FlowEdge> &edges, int s, int t)
{
    QVector<int> viaEdge(numFlowNodes, -1);
    QVector<bool> visited(numFlowNodes, false);
    QQueue<int> queue;
    queue.enqueue(s);
    visited[s] = true;

    while (!queue.isEmpty() && !visited[t]) {
        const int u = queue.dequeue();
        for (int eid : adjList[u]) {
            const FlowEdge &e = edges[eid];
            if (!visited[e.to] && e.cap > 0) {
                visited[e.to] = true;
                viaEdge[e.to] = eid;
                queue.enqueue(e.to);
            }
        }
    }

    if (!visited[t])
        return 0;

    qint64 bottleneck = std::numeric_limits<qint64>::max();
    for (int v = t; v != s; ) {
        const int eid = viaEdge[v];
        bottleneck = std::min(bottleneck, edges[eid].cap);
        v = edges[eid ^ 1].to;
    }
    for (int v = t; v != s; ) {
        const int eid = viaEdge[v];
        edges[eid].cap -= bottleneck;
        edges[eid ^ 1].cap += bottleneck;
        v = edges[eid ^ 1].to;
    }
    return bottleneck;
}

// Builds the vertex-split flow graph over `verts` and returns the max-flow from source to
// target, i.e. kappa(source, target). Caller guarantees source != target and they are not
// adjacent (Menger's theorem doesn't apply otherwise - see Graph::graphNodeConnectivity()).
int localVertexConnectivityFlow(Graph &g, const QList<int> &verts, int source, int target,
                                 int currentRelation, bool respectDirection)
{
    const int n = verts.size();
    QHash<int, int> nodeIndex;
    nodeIndex.reserve(n);
    for (int i = 0; i < n; ++i)
        nodeIndex[verts[i]] = i;

    constexpr qint64 INF_CAP = std::numeric_limits<qint64>::max() / 4;
    const int numFlowNodes = 2 * n; // vertex i -> in-node 2*i, out-node 2*i+1
    QVector<QVector<int>> adjList(numFlowNodes);
    QVector<FlowEdge> edges;
    edges.reserve(4 * n);

    for (int i = 0; i < n; ++i) {
        const int v = verts[i];
        const qint64 splitCap = (v == source || v == target) ? INF_CAP : 1;
        addFlowEdge(edges, adjList, 2 * i, 2 * i + 1, splitCap);
    }

    for (int i = 0; i < n; ++i) {
        const int u = verts[i];
        const QSet<int> nbrs = outNeighborsOf(g, u, currentRelation);
        QSet<int> allNbrs = nbrs;
        if (!respectDirection)
            allNbrs.unite(inNeighborsOf(g, u, currentRelation));
        for (int w : allNbrs) {
            const auto jt = nodeIndex.constFind(w);
            if (jt == nodeIndex.constEnd())
                continue; // neighbor outside the enabled-vertex set - not part of this computation
            addFlowEdge(edges, adjList, 2 * i + 1, 2 * jt.value(), INF_CAP);
        }
    }

    const int sOut = 2 * nodeIndex.value(source) + 1;
    const int tIn = 2 * nodeIndex.value(target);

    qint64 totalFlow = 0;
    qint64 pushed;
    while ((pushed = augmentOnce(numFlowNodes, adjList, edges, sOut, tIn)) > 0)
        totalFlow += pushed;

    return static_cast<int>(totalFlow);
}

} // namespace

/**
 * @brief Local vertex connectivity kappa(source, target): the minimum number of nodes, other
 * than source and target themselves, whose removal disconnects target from source.
 *
 * Computed via Menger's theorem / vertex-split max-flow - see the detailed algorithm comment on
 * localVertexConnectivityFlow() above.
 *
 * source and target being directly adjacent is reported as NodeConnectivityStatus::Adjacent, not
 * as a number: Menger's theorem requires non-adjacency (an edge is a "path" no vertex removal can
 * ever break), so there is no finite, meaningful cut size to report - reporting some sentinel
 * number here would silently misrepresent "cannot be separated by removing other nodes" as an
 * ordinary connectivity value.
 *
 * @param source
 * @param target
 * @param respectDirection true for the "strong" reading (directed reachability, source must
 *        reach target via directed paths); false for "weak" (every edge treated as bidirectional).
 *        Only meaningful to vary on a directed graph - see the GUI's Connectedness/Node
 *        Connectivity prompts, which only ask when isDirected() is true.
 * @return NodeConnectivityResult - see graph.h. Ok.value == 0 is a normal, valid answer (target
 *         is simply unreachable from source under the chosen direction mode).
 */
Graph::NodeConnectivityResult Graph::graphNodeConnectivity(int source, int target, bool respectDirection)
{
    NodeConnectivityResult result;

    if (source == target || vertexIndexByNumber(source) < 0 || vertexIndexByNumber(target) < 0) {
        result.status = NodeConnectivityStatus::Invalid;
        return result;
    }

    const int currentRelation = relationCurrent();

    if (adjacent(*this, source, target, currentRelation, respectDirection)) {
        result.status = NodeConnectivityStatus::Adjacent;
        return result;
    }

    const QList<int> verts = enabledVertexNumbers(*this);
    result.status = NodeConnectivityStatus::Ok;
    result.value = localVertexConnectivityFlow(*this, verts, source, target, currentRelation, respectDirection);
    return result;
}

/**
 * @brief Global vertex connectivity kappa(G): the minimum, over every non-adjacent pair of
 * vertices, of their local vertex connectivity (graphNodeConnectivity()) - the network's
 * worst-case robustness to node removal, i.e. the fewest nodes that would need to be removed to
 * disconnect the network at its weakest point.
 *
 * Naive pairwise-minimum algorithm: iterate all non-adjacent pairs, tracking the minimum local
 * connectivity seen so far, starting that minimum at the cheap degree bound below and pruning
 * with an early exit once it can't go any lower. This is O(n^2) local-connectivity computations
 * in the worst case (O(n) for directed "strong" mode, since kappa(s,t) can differ from kappa(t,s)
 * so ordered pairs must be tested) - deliberately the simple approach rather than the smarter
 * O(n) algorithm (Even 1975, which fixes one vertex and reuses far fewer max-flow computations):
 * SocNetV's networks and this feature's usage pattern (an occasional, user-triggered analysis,
 * not a hot path) don't call for that extra complexity yet, and the two pruning steps below
 * already cut the common cases down a lot in practice.
 *
 * Two pruning steps, both used here:
 * - Fast path: if the graph is already disconnected (per graphWeaklyConnectedComponents() /
 *   graphStronglyConnectedComponents(), whichever matches respectDirection), kappa(G) = 0
 *   immediately, with no max-flow computation at all. This is purely an optimization, not a
 *   correctness requirement: the pair loop below would reach the same answer on its own the
 *   moment it tests the unreachable pair, since local connectivity between an unreachable pair is
 *   0 by construction (max-flow finds no augmenting path) - but checking the already-cached
 *   component count first avoids running any flow computation for the common case of an obviously
 *   fragmented network.
 * - Degree bound (Whitney's inequality): kappa(G) <= delta(G), the minimum vertex degree (its
 *   directed analogue: kappa(D) <= min over v of min(indeg(v), outdeg(v))). Seeding the running
 *   minimum with this bound and only ever lowering it means a complete graph - where no
 *   non-adjacent pair exists at all to test - needs no special case: the pair loop simply never
 *   executes, leaving the initial degree bound as the final answer, which is exactly correct
 *   (kappa(K_n) = n-1, the minimum degree of K_n).
 *
 * @param respectDirection true for strong connectivity (ordered pairs, directed reachability);
 *        false for weak (unordered pairs, every edge treated as bidirectional). Only meaningful
 *        to vary on a directed graph.
 * @return kappa(G).
 *
 * @note Precondition: at least 2 enabled vertices. Callers (the GUI's Graph Connectivity action)
 *       special-case 0/1-vertex networks the same way Connectedness already does, so this is
 *       never invoked otherwise.
 */
int Graph::graphConnectivity(bool respectDirection)
{
    const int currentRelation = relationCurrent();
    const QList<int> verts = enabledVertexNumbers(*this);
    const int n = verts.size();

    const bool graphIsConnected = respectDirection
        ? (graphStronglyConnectedComponents() == 1)
        : (graphWeaklyConnectedComponents() == 1);
    if (!graphIsConnected) {
        qCDebug(lcCohesion) << "Graph::graphConnectivity() - graph disconnected, kappa(G)=0";
        return 0;
    }

    int best = INT_MAX;
    for (int v : verts) {
        int deg;
        if (respectDirection)
            deg = std::min(outNeighborsOf(*this, v, currentRelation).size(),
                            inNeighborsOf(*this, v, currentRelation).size());
        else
            deg = neighborsOf(*this, v, currentRelation, false).size();
        best = std::min(best, deg);
    }
    qCDebug(lcCohesion) << "Graph::graphConnectivity() - degree bound:" << best;

    for (int i = 0; i < n && best > 0; ++i) {
        const int jStart = respectDirection ? 0 : i + 1;
        for (int j = jStart; j < n && best > 0; ++j) {
            if (i == j) continue;
            const int u = verts[i];
            const int w = verts[j];
            if (adjacent(*this, u, w, currentRelation, respectDirection))
                continue;

            const int k = localVertexConnectivityFlow(*this, verts, u, w, currentRelation, respectDirection);
            best = std::min(best, k);
        }
    }

    qCDebug(lcCohesion) << "Graph::graphConnectivity() - kappa(G):" << best;
    return best;
}
