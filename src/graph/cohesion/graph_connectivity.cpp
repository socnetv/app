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
#include <QPair>
#include <QQueue>
#include <QSet>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
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

// ----------------------------------------------------------------------------
// Shared cancel-checkable, parallel pair-testing loop, used by both
// Graph::graphConnectivity() (Esfahanian-Hakimi) and Graph::graphConnectivityNaive() (the
// full O(n^2) sweep). Both algorithms boil down to "run localVertexConnectivityFlow() over a
// list of non-adjacent pairs, keep the smallest result" - only the *list of pairs* differs
// between them (Esfahanian-Hakimi tests a much smaller, cleverly-chosen list; the naive
// algorithm tests literally every non-adjacent pair). Factoring this out means the
// parallelization/cancellation/early-exit machinery only needs to be written and verified once.
//
// Pairs are processed in fixed-size batches via QtConcurrent::blockingMap - each worker
// computes one pair's independent, read-only localVertexConnectivityFlow() call and writes
// into a batch-local results vector (no shared accumulator during the parallel step itself,
// unlike a plain int written from multiple threads at once, which would race). Each batch's
// results are reduced into the running minimum sequentially right after blockingMap returns,
// then progressCanceled() is checked between batches - a real, responsive check, since this
// loop is not itself further parallelized beyond one batch at a time: checking between batches
// actually interrupts the sweep within roughly one batch's duration, unlike an all-or-nothing
// single parallel step. The early exit once the running minimum hits 0 is preserved at batch
// granularity (stop dispatching further batches, not mid-batch) rather than per-pair.
//
// Returns true if the sweep completed (best now holds the true minimum over every pair in
// `pairs`, combined with whatever `best` already held on entry), or false if canceled partway
// (best holds the minimum found among pairs tested so far - a valid upper bound, not
// necessarily exact).
bool testPairsForMinimum(Graph &g, const QList<QPair<int, int>> &pairs, const QList<int> &verts,
                          int currentRelation, bool respectDirection, int &best)
{
    // Batch size: large enough that blockingMap's per-batch dispatch overhead is negligible
    // next to the actual max-flow work, small enough that Cancel responds within a few
    // batches' worth of time rather than needing to wait for the whole (potentially huge)
    // pair list.
    constexpr int kBatchSize = 200;

    const int totalPairs = static_cast<int>(pairs.size());
    for (int batchStart = 0; batchStart < totalPairs && best > 0; batchStart += kBatchSize) {
        const int batchEnd = std::min(batchStart + kBatchSize, totalPairs);
        const int batchSize = batchEnd - batchStart;

        QVector<int> batchResults(batchSize);

        QList<int> batchIndices;
        batchIndices.reserve(batchSize);
        for (int k = 0; k < batchSize; ++k)
            batchIndices.append(k);

        QtConcurrent::blockingMap(batchIndices, [&](int k) {
            const QPair<int, int> &pair = pairs[batchStart + k];
            batchResults[k] = localVertexConnectivityFlow(
                g, verts, pair.first, pair.second, currentRelation, respectDirection);
        });

        for (int result : batchResults)
            best = std::min(best, result);

        if (g.progressCanceled())
            return false;
    }

    return true;
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
 * @brief Global vertex connectivity kappa(G), computed via Esfahanian & Hakimi's (1984)
 * reduced-pair-set algorithm - O(n + delta^2) max-flow calls instead of the O(n^2) a full
 * pairwise sweep needs (see graphConnectivityNaive() below for that sweep, kept as a
 * comparative/correctness cross-check, not for production use).
 *
 * kappa(G) is the minimum, over every non-adjacent pair of vertices, of their local vertex
 * connectivity (graphNodeConnectivity()) - the network's worst-case robustness to node removal,
 * i.e. the fewest nodes that would need to be removed to disconnect the network at its weakest
 * point.
 *
 * ---- The algorithm, in plain terms ----
 *
 * Testing every single non-adjacent pair (the naive approach) is wasteful: Esfahanian & Hakimi
 * proved that checking just TWO carefully-chosen groups of pairs is provably enough to find the
 * true global minimum, no matter how large the network is:
 *
 *   1. Pick any vertex v of minimum degree (delta(G)) - the same vertex the cheap degree-bound
 *      trick already identifies "for free". Removing all of v's neighbors always disconnects v
 *      from the rest of the graph, so kappa(G) can never exceed delta(G) - this is exactly
 *      Whitney's inequality, already used elsewhere in this file as a cheap upper bound.
 *   2. k1 = the smallest kappa(v, w) found by testing v against every OTHER vertex w it isn't
 *      already adjacent to. This alone would already catch the true kappa(G) in the common case
 *      where v happens to be on the "weak side" of the network's worst cut.
 *   3. k2 = the smallest kappa(u, w) found by testing every pair of v's OWN neighbors against
 *      each other (only the non-adjacent pairs among them - same rule as everywhere else in this
 *      file). This is the clever part: it turns out that IF the worst cut in the whole network
 *      does not involve v directly (so step 2 could miss it), that cut must still separate at
 *      least two of v's own neighbors from each other - so testing pairs among v's neighbors is
 *      guaranteed to catch it instead. (Why: a vertex cut smaller than v's own degree can't
 *      possibly contain every one of v's neighbors - there are too few "removal slots" for that
 *      many neighbors - so at least two neighbors must survive on opposite sides of the split.)
 *   4. kappa(G) = min(k1, k2). Since we don't know in advance which of the two cases above
 *      actually applies to this specific network, both k1 and k2 are always computed - one of
 *      them is guaranteed to find the true answer, and taking the smaller of the two is safe
 *      regardless of which one it turns out to be.
 *
 * ---- Why this is correct (the proof, in brief) ----
 *
 * Let S be a true minimum vertex cutset of G, |S| = kappa(G) - the actual smallest set of
 * vertices whose removal disconnects the network at its single weakest point. Exactly one of two
 * cases must hold for our chosen vertex v:
 *
 *   - Case A: v is NOT in S. Then S still disconnects some other pair, and since v itself
 *     survives the cut, v ends up on one side of the resulting split. That means removing S also
 *     disconnects v from whichever vertex w landed on the other side - so kappa(v, w) <= |S| =
 *     kappa(G). Since k1 tests v against literally every other vertex, it is guaranteed to find
 *     this w (or something at least as good), so k1 <= kappa(G).
 *   - Case B: v IS in S (v belongs to every minimum cutset of G). Removing S (which includes v)
 *     splits the rest of the graph into at least two pieces. v had delta(G) neighbors before
 *     removal; since |S| = kappa(G) <= delta(G) (Whitney's inequality) and v itself already
 *     accounts for one member of S, there are strictly fewer than delta(G) OTHER vertices left in
 *     S to "use up" on v's neighbors - too few removal slots to contain every one of v's
 *     delta(G) neighbors. So at least two of v's neighbors, call them u and w, must survive on
 *     opposite sides of the split, meaning S \ {v} (or a subset of it) disconnects u from w:
 *     kappa(u, w) <= |S| - 1 < kappa(G), or at worst kappa(u,w) <= kappa(G). Since k2 tests every
 *     pair of v's neighbors against each other, it is guaranteed to find this (u, w) pair, so
 *     k2 <= kappa(G).
 *
 * Either way, min(k1, k2) <= kappa(G). And since k1/k2 are themselves local connectivity values
 * between real, non-adjacent vertex pairs in G, neither can ever be smaller than the true global
 * minimum: min(k1, k2) >= kappa(G) by definition of kappa(G) as that minimum. Both inequalities
 * together give min(k1, k2) = kappa(G) exactly - not an approximation or a heuristic bound.
 *
 * Step 2 is O(n) max-flow calls (one per other vertex); step 3 is O(delta^2) max-flow calls
 * (every pair among v's delta neighbors) - both far smaller than the O(n^2) the naive sweep
 * needs, especially once n grows into the thousands while delta (typical for sparse, realistic
 * social networks) stays in the tens at most. See
 * `docs/roadmaps/roadmap_ws11_algorithm_additions.md` and the manual's Graph Connectivity
 * section for a plainer-language walkthrough of the same idea.
 *
 * ---- Implementation notes ----
 *
 * Reuses the same degree-bound loop that already existed for the cheap upper-bound trick, just
 * additionally remembering *which* vertex achieved the minimum degree (not only the minimum
 * value) so it can be used as `v` above. Both step 2's and step 3's pair lists are handed to the
 * same testPairsForMinimum() helper graphConnectivityNaive() also uses - same batched
 * parallelization (QtConcurrent::blockingMap, kBatchSize-sized chunks) and the same
 * between-batches progressCanceled() check inherited from #278's fix, applied here to a vastly
 * smaller pair list. The disconnected-graph fast path (kappa(G)=0, no max-flow at all) is kept
 * unchanged from the naive version.
 *
 * @param respectDirection true for strong connectivity (ordered pairs, directed reachability);
 *        false for weak (unordered pairs, every edge treated as bidirectional). Only meaningful
 *        to vary on a directed graph.
 * @return GraphConnectivityResult - Ok with the true kappa(G), or Canceled with the best (lowest)
 *         value found among pairs tested before cancellation (a valid upper bound on kappa(G),
 *         not necessarily exact - step 2 or step 3 may not have finished yet).
 *
 * @note Precondition: at least 2 enabled vertices. Callers (the GUI's Graph Connectivity action)
 *       special-case 0/1-vertex networks the same way Connectedness already does, so this is
 *       never invoked otherwise.
 */
Graph::GraphConnectivityResult Graph::graphConnectivity(bool respectDirection)
{
    const int currentRelation = relationCurrent();
    const QList<int> verts = enabledVertexNumbers(*this);

    const bool graphIsConnected = respectDirection
        ? (graphStronglyConnectedComponents() == 1)
        : (graphWeaklyConnectedComponents() == 1);
    if (!graphIsConnected) {
        qCDebug(lcCohesion) << "Graph::graphConnectivity() - graph disconnected, kappa(G)=0";
        return {GraphConnectivityStatus::Ok, 0};
    }

    // Degree-bound pass: same trick as graphConnectivityNaive(), but also remembers which
    // vertex achieved the minimum - that vertex becomes `v` in the algorithm's own terms above.
    int best = INT_MAX;
    int v = verts.isEmpty() ? -1 : verts.first();
    for (int candidate : verts) {
        int deg;
        if (respectDirection)
            deg = std::min(outNeighborsOf(*this, candidate, currentRelation).size(),
                            inNeighborsOf(*this, candidate, currentRelation).size());
        else
            deg = neighborsOf(*this, candidate, currentRelation, false).size();
        if (deg < best) {
            best = deg;
            v = candidate;
        }
    }
    qCDebug(lcCohesion) << "Graph::graphConnectivity() - degree bound:" << best
             << "achieved by vertex" << v;

    // Step 2 (k1): v against every other non-adjacent vertex.
    QList<QPair<int, int>> vPairs;
    for (int w : verts) {
        if (w == v) continue;
        if (adjacent(*this, v, w, currentRelation, respectDirection))
            continue;
        vPairs.append({v, w});
    }
    qCDebug(lcCohesion) << "Graph::graphConnectivity() - step 2 (k1) pairs:" << vPairs.size();

    if (!testPairsForMinimum(*this, vPairs, verts, currentRelation, respectDirection, best)) {
        qCDebug(lcCohesion) << "Graph::graphConnectivity() - canceled during step 2, best so far:" << best;
        return {GraphConnectivityStatus::Canceled, best};
    }

    if (best == 0) {
        qCDebug(lcCohesion) << "Graph::graphConnectivity() - kappa(G):" << best << "(found in step 2)";
        return {GraphConnectivityStatus::Ok, best};
    }

    // Step 3 (k2): every non-adjacent pair among v's own neighbors (not v's neighbors against
    // the rest of the graph - just against each other).
    const QSet<int> vNeighbors = neighborsOf(*this, v, currentRelation, respectDirection);
    const QList<int> vNeighborsList(vNeighbors.cbegin(), vNeighbors.cend());

    QList<QPair<int, int>> neighborPairs;
    for (int i = 0; i < vNeighborsList.size(); ++i) {
        const int jStart = respectDirection ? 0 : i + 1;
        for (int j = jStart; j < vNeighborsList.size(); ++j) {
            if (i == j) continue;
            const int u = vNeighborsList[i];
            const int w = vNeighborsList[j];
            if (adjacent(*this, u, w, currentRelation, respectDirection))
                continue;
            neighborPairs.append({u, w});
        }
    }
    qCDebug(lcCohesion) << "Graph::graphConnectivity() - step 3 (k2) pairs among"
             << vNeighborsList.size() << "neighbors of v:" << neighborPairs.size();

    if (!testPairsForMinimum(*this, neighborPairs, verts, currentRelation, respectDirection, best)) {
        qCDebug(lcCohesion) << "Graph::graphConnectivity() - canceled during step 3, best so far:" << best;
        return {GraphConnectivityStatus::Canceled, best};
    }

    qCDebug(lcCohesion) << "Graph::graphConnectivity() - kappa(G):" << best;
    return {GraphConnectivityStatus::Ok, best};
}

/**
 * @brief Global vertex connectivity kappa(G), computed via the naive full-pairwise-minimum
 * sweep: iterate every non-adjacent pair of vertices, tracking the minimum local connectivity
 * seen so far. O(n^2) local-connectivity computations in the worst case (O(n) for directed
 * "strong" mode's ordered pairs, since kappa(s,t) can differ from kappa(t,s)).
 *
 * Kept deliberately alongside the faster graphConnectivity() (Esfahanian & Hakimi, 1984, see its
 * own doc comment) as a comparative/correctness cross-check: this function's "test literally
 * everything" approach is trivially, structurally correct by construction, which makes it a
 * trustworthy ground truth for verifying the faster algorithm's cleverer (and therefore more
 * failure-prone) reduced pair selection produces the same answer. Not used by any production
 * code path (the GUI's Graph Connectivity menu action and the CLI's vertex_connectivity kernel
 * both call graphConnectivity() instead) - this is intentionally the slow, obviously-correct
 * reference implementation, kept for possible future cross-checking rather than deleted.
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
 * Parallelization + cancellation (#278, WS11): confirmed hanging 30+ minutes uncancellable on a
 * real N=2000 sparse network, with zero progressCanceled() checks anywhere in this file. Every
 * non-adjacent pair is enumerated up front into a flat list, then handed to the same
 * testPairsForMinimum() helper graphConnectivity() (Esfahanian-Hakimi) also uses - see that
 * helper's own comment for the batching/parallelization/cancellation mechanics.
 *
 * @param respectDirection true for strong connectivity (ordered pairs, directed reachability);
 *        false for weak (unordered pairs, every edge treated as bidirectional). Only meaningful
 *        to vary on a directed graph.
 * @return GraphConnectivityResult - Ok with the true kappa(G), or Canceled with the best (lowest)
 *         value found among pairs tested before cancellation (a valid upper bound on kappa(G),
 *         not necessarily exact).
 *
 * @note Precondition: at least 2 enabled vertices. Callers (the GUI's Graph Connectivity action)
 *       special-case 0/1-vertex networks the same way Connectedness already does, so this is
 *       never invoked otherwise.
 */
Graph::GraphConnectivityResult Graph::graphConnectivityNaive(bool respectDirection)
{
    const int currentRelation = relationCurrent();
    const QList<int> verts = enabledVertexNumbers(*this);
    const int n = verts.size();

    const bool graphIsConnected = respectDirection
        ? (graphStronglyConnectedComponents() == 1)
        : (graphWeaklyConnectedComponents() == 1);
    if (!graphIsConnected) {
        qCDebug(lcCohesion) << "Graph::graphConnectivityNaive() - graph disconnected, kappa(G)=0";
        return {GraphConnectivityStatus::Ok, 0};
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
    qCDebug(lcCohesion) << "Graph::graphConnectivityNaive() - degree bound:" << best;

    // Enumerate every non-adjacent pair up front (cheap - adjacency lookups only, no max-flow
    // yet), so the expensive part below can be dispatched in fixed-size, cancel-checkable batches.
    QList<QPair<int, int>> pairs;
    for (int i = 0; i < n; ++i) {
        const int jStart = respectDirection ? 0 : i + 1;
        for (int j = jStart; j < n; ++j) {
            if (i == j) continue;
            const int u = verts[i];
            const int w = verts[j];
            if (adjacent(*this, u, w, currentRelation, respectDirection))
                continue;
            pairs.append({u, w});
        }
    }
    qCDebug(lcCohesion) << "Graph::graphConnectivityNaive() - non-adjacent pairs to test:" << pairs.size();

    if (!testPairsForMinimum(*this, pairs, verts, currentRelation, respectDirection, best)) {
        qCDebug(lcCohesion) << "Graph::graphConnectivityNaive() - canceled, best so far:" << best;
        return {GraphConnectivityStatus::Canceled, best};
    }

    qCDebug(lcCohesion) << "Graph::graphConnectivityNaive() - kappa(G):" << best;
    return {GraphConnectivityStatus::Ok, best};
}
