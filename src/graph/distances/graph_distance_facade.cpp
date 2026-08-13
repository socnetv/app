/**
 * @file graph_distance_facade.cpp
 * @brief Implements façade-level distance and connectivity accessors of the Graph class, delegating computations to DistanceEngine and exposing cached aggregates.
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
#include <QFile>
#include <QList>
#include <QMap>
#include <QQueue>
#include <QTextStream>
#include <algorithm> // std::min
#include <cstdlib>  // RAND_MAX

// PUBLIC DISTANCE API FACADE

/**
 * @brief Returns the geodesic distance (length of shortest path)
 * from vertex v1 to vertex v2
 * @param v1
 * @param v2
 * @param considerWeights
 * @param inverseWeights
 * @return
 */
int Graph::graphDistanceGeodesic(const int &v1, const int &v2,
                                 const bool &considerWeights,
                                 const bool &inverseWeights)
{
    qCDebug(lcDistances) << "Graph::graphDistanceGeodesic()";
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);
    return apspDistance(v1, v2);
}

/**
 * @brief Returns the already-computed geodesic distance from vertex v1 to vertex v2, for the
 * current relation, without triggering a recompute (unlike graphDistanceGeodesic() above).
 * RAND_MAX if either vertex is unknown or nothing has been computed yet for this relation.
 */
qreal Graph::apspDistance(const int &v1, const int &v2)
{
    const int i = vertexIndexByNumber(v1);
    const int j = vertexIndexByNumber(v2);
    if (i < 0 || j < 0)
        return RAND_MAX;
    auto it = m_apspDist.find(relationCurrent());
    if (it == m_apspDist.end())
        return RAND_MAX;
    return it->item(i, j);
}

/**
 * @brief Returns the already-computed number of shortest paths from vertex v1 to vertex v2,
 * for the current relation, without triggering a recompute. 0 if either vertex is unknown or
 * nothing has been computed yet for this relation.
 */
int Graph::apspShortestPaths(const int &v1, const int &v2)
{
    const int i = vertexIndexByNumber(v1);
    const int j = vertexIndexByNumber(v2);
    if (i < 0 || j < 0)
        return 0;
    auto it = m_apspSigma.find(relationCurrent());
    if (it == m_apspSigma.end())
        return 0;
    return static_cast<int>(it->item(i, j));
}

/**
 * @brief Returns a histogram of geodesic distances across all ordered vertex pairs.
 *
 * Ensures the full APSP result is available (uses cache if graph is unchanged),
 * then iterates all enabled vertex pairs and groups them by their geodesic distance
 * into a QMap<int, int> where key = distance and value = number of ordered pairs
 * at that distance.  Unreachable pairs (distance == RAND_MAX) are excluded.
 *
 * This method is called ONLY on explicit user request — it is never triggered as a
 * side-effect of centrality computation or other internal analysis paths.
 *
 * @param considerWeights  Pass through to graphDistancesGeodesic; selects BFS vs Dijkstra.
 * @param inverseWeights   Pass through to graphDistancesGeodesic; inverts edge weights.
 * @return QMap<int, int>  Sorted map: distance bucket → count of ordered pairs.
 */
QMap<int, int> Graph::graphGeodesicDistanceDistribution(const bool &considerWeights,
                                                        const bool &inverseWeights)
{
    qCDebug(lcDistances) << "Graph::graphGeodesicDistanceDistribution()";

    // Ensure the APSP result is available.  graphDistancesGeodesic() is a no-op
    // when calculatedDistances is true (i.e. graph structure is unchanged).
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);

    QMap<int, int> distribution;

    VList::const_iterator it1, it2;
    for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1) {
        if (!(*it1)->isEnabled()) continue;
        const int src = (*it1)->number();
        for (it2 = m_graph.cbegin(); it2 != m_graph.cend(); ++it2) {
            if (!(*it2)->isEnabled()) continue;
            const int tgt = (*it2)->number();
            if (src == tgt) continue;  // skip self-pairs

            const qreal d = apspDistance(src, tgt);

            // Exclude unreachable pairs (RAND_MAX is the sentinel used by DistanceEngine).
            if (d <= 0 || d >= static_cast<qreal>(RAND_MAX)) continue;

            // Round to nearest integer: distances from BFS are exact integers stored
            // as qreal; Dijkstra distances are real-valued but are still grouped by
            // integer bucket for the histogram.
            distribution[qRound(d)]++;
        }
    }

    qCDebug(lcDistances) << "Graph::graphGeodesicDistanceDistribution() - distribution:" << distribution;
    return distribution;
}

/**
 * @brief Writes a geodesic distance distribution report to an HTML file.
 *
 * Computes the distribution via graphGeodesicDistanceDistribution() (cache-aware),
 * then writes a sortable HTML table (distance | pair count | % | cumulative %)
 * to @p fileName.
 *
 * @param fileName        Full path of the output HTML file.
 * @param considerWeights Passed through to graphGeodesicDistanceDistribution().
 * @param inverseWeights  Passed through to graphGeodesicDistanceDistribution().
 * @return true on success; false if the file could not be opened.
 */
bool Graph::writeGeodesicDistribution(const QString &fileName,
                                      const bool &considerWeights,
                                      const bool &inverseWeights)
{
    qCDebug(lcDistances) << "Graph::writeGeodesicDistribution() ->" << fileName;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCDebug(lcDistances) << "Graph::writeGeodesicDistribution() - cannot open file";
        return false;
    }
    QTextStream out(&file);

    const QMap<int, int> dist = graphGeodesicDistanceDistribution(considerWeights, inverseWeights);

    // Compute the total number of connected ordered pairs for percentage columns.
    int totalPairs = 0;
    for (int cnt : dist) totalPairs += cnt;

    out << htmlHead;
    out << "<h1>" << tr("GEODESIC DISTANCE DISTRIBUTION") << "</h1>";

    out << "<p>"
        << "<span class=\"info\">" << tr("Network name: ") << "</span>"
        << getName()
        << "<br />"
        << "<span class=\"info\">" << tr("Actors: ") << "</span>"
        << vertices()
        << "</p>";

    if (dist.isEmpty()) {
        out << "<p>" << tr("No connected pairs found.") << "</p>";
        out << htmlEnd;
        file.close();
        return true;
    }

    out << "<p class=\"description\">"
        << tr("Number of ordered pairs of connected nodes separated by each "
              "geodesic distance <em>d</em>.  "
              "Unreachable pairs are excluded.  "
              "Diameter of this network: <strong>%1</strong>.")
               .arg(graphDiameterCached())
        << "</p>";

    out << "<table class=\"stripes sortable\">"
        << "<thead><tr>"
        << "<th>" << tr("Distance (d)") << "</th>"
        << "<th>" << tr("Pairs") << "</th>"
        << "<th>" << tr("% of connected pairs") << "</th>"
        << "<th>" << tr("Cumulative %") << "</th>"
        << "</tr></thead><tbody>";

    int cumulative = 0;
    for (auto it = dist.cbegin(); it != dist.cend(); ++it) {
        const int d     = it.key();
        const int count = it.value();
        cumulative += count;
        const double pct  = 100.0 * count      / totalPairs;
        const double cpct = 100.0 * cumulative / totalPairs;
        out << "<tr>"
            << "<td>" << d << "</td>"
            << "<td>" << count << "</td>"
            << QString("<td>%1%</td>").arg(pct,  0, 'f', 2)
            << QString("<td>%1%</td>").arg(cpct, 0, 'f', 2)
            << "</tr>";
    }

    out << "</tbody></table>";
    out << htmlEnd;
    file.close();
    return true;
}

/**
 * @brief Reconstructs one shortest path from vertex v1 to vertex v2.
 *
 * Runs a single-source BFS (unweighted) or Dijkstra (weighted) from v1,
 * keeping a predecessor array, then traces the path back from v2.
 * This is an on-demand, interactive call — it does NOT touch the APSP cache
 * and is safe to call at any time regardless of calculatedDistances.
 *
 * @param v1               Source vertex number.
 * @param v2               Target vertex number.
 * @param considerWeights  If true, uses Dijkstra with edge weights; otherwise BFS.
 * @param inverseWeights   If true, uses 1/weight as the edge cost (for closeness-style paths).
 * @return QList<int>      Ordered list of vertex numbers on the path, inclusive of v1 and v2.
 *                         Returns an empty list when v1 == v2 or no path exists.
 */
QList<int> Graph::graphGeodesicShortestPath(const int &v1, const int &v2,
                                             const bool &considerWeights,
                                             const bool &inverseWeights)
{
    qCDebug(lcDistances) << "Graph::graphGeodesicShortestPath()" << v1 << "->" << v2;

    if (v1 == v2 || !vpos.contains(v1) || !vpos.contains(v2))
        return QList<int>();

    const int N = m_graph.size();
    const int currentRelation = relationCurrent();

    // pred[i] = vertex NUMBER of the predecessor of m_graph[i] on the shortest
    // path from v1.  Initialised to -1 (no predecessor known yet).
    QVector<int> pred(N, -1);

    // dist[i] = best distance from v1 to m_graph[i] found so far.
    // RAND_MAX is the sentinel meaning "not yet reached".
    QVector<qreal> dist(N, static_cast<qreal>(RAND_MAX));

    const int srcIdx = vpos[v1];
    dist[srcIdx] = 0;

    if (!considerWeights) {
        // -----------------------------------------------------------------------
        // Unweighted BFS.  Each edge has cost 1, so the first time a vertex is
        // reached it is via the shortest path — no relaxation needed.
        // Time: O(V + E).
        // -----------------------------------------------------------------------
        QQueue<int> queue;     // stores vertex positions (indices into m_graph)
        queue.enqueue(srcIdx);

        while (!queue.isEmpty()) {
            const int u = queue.dequeue();

            // Iterate over all out-edges of vertex u in the current relation.
            for (auto eit = m_graph[u]->outEdges().cbegin();
                 eit != m_graph[u]->outEdges().cend(); ++eit) {

                if (eit.value().first != currentRelation) continue;  // wrong relation
                if (!eit.value().second.second) continue;            // edge disabled

                const int wNum = eit.key();                          // target vertex number
                if (!vpos.contains(wNum)) continue;
                const int w = vpos[wNum];

                // In BFS, the first visit is always via the shortest path.
                if (dist[w] == static_cast<qreal>(RAND_MAX)) {
                    dist[w] = dist[u] + 1;
                    pred[w] = m_graph[u]->number();                  // record predecessor
                    queue.enqueue(w);
                }
            }
        }
    } else {
        // -----------------------------------------------------------------------
        // Weighted Dijkstra.  Edge weights are used as costs (or their inverses
        // when inverseWeights is true).  Uses a QMap<qreal, QList<int>> as a
        // simple priority queue ordered by distance.
        // Time: O((V + E) log V) due to QMap insertion/lookup.
        // -----------------------------------------------------------------------
        QMap<qreal, QList<int>> pq;  // distance → list of vertex positions at that distance
        pq[0.0].append(srcIdx);

        while (!pq.isEmpty()) {
            // Extract the entry with the smallest distance.
            auto it = pq.begin();
            const qreal d = it.key();
            const int u = it.value().takeFirst();
            if (it.value().isEmpty()) pq.erase(it);

            // Skip stale entries (a shorter path to u was already settled).
            if (d > dist[u]) continue;

            // Relax all out-edges of u.
            for (auto eit = m_graph[u]->outEdges().cbegin();
                 eit != m_graph[u]->outEdges().cend(); ++eit) {

                if (eit.value().first != currentRelation) continue;
                if (!eit.value().second.second) continue;

                const int wNum = eit.key();
                if (!vpos.contains(wNum)) continue;
                const int w = vpos[wNum];

                qreal edgeCost = eit.value().second.first;      // raw edge weight
                if (inverseWeights && edgeCost != 0.0)
                    edgeCost = 1.0 / edgeCost;                  // invert for closeness-style cost

                const qreal newDist = dist[u] + edgeCost;
                if (newDist < dist[w]) {
                    dist[w] = newDist;
                    pred[w] = m_graph[u]->number();              // record predecessor
                    pq[newDist].append(w);                       // enqueue with new distance
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Path reconstruction: walk the predecessor chain from v2 back to v1.
    // -----------------------------------------------------------------------
    const int tgtIdx = vpos[v2];
    if (dist[tgtIdx] == static_cast<qreal>(RAND_MAX))
        return QList<int>();   // v2 is unreachable from v1

    QList<int> path;
    int cur = v2;
    while (cur != v1) {
        path.prepend(cur);
        const int curPred = pred[vpos[cur]];
        if (curPred == -1) return QList<int>();  // corrupted predecessor — should not happen
        cur = curPred;
    }
    path.prepend(v1);  // prepend source last so path reads v1 → … → v2

    qCDebug(lcDistances) << "Graph::graphGeodesicShortestPath() - path:" << path;
    return path;
}

/**
 * @brief Returns the diameter of the graph, aka the largest geodesic distance
 * between any two vertices
 * @param considerWeights
 * @param inverseWeights
 * @return
 */
int Graph::graphDiameter(const bool considerWeights,
                         const bool inverseWeights)
{
    qCDebug(lcDistances) << "Graph::graphDiameter()";
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);
    return m_graphDiameter;
}

/**
 * @brief Returns the average distance of the graph
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 * @return
 */
qreal Graph::graphDistanceGeodesicAverage(const bool considerWeights,
                                          const bool inverseWeights,
                                          const bool dropIsolates)
{

    qCDebug(lcDistances) << "Graph::graphDistanceGeodesicAverage() - Computing distances...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    qCDebug(lcDistances) << "Graph::graphDistanceGeodesicAverage() - "
             << "average distance:"
             << m_graphAverageDistance;

    return m_graphAverageDistance;
}

/**
 * @brief Returns the number of geodesics (shortest-paths) in the graph.
 *
 * @return int
 */
int Graph::getGeodesicsCount()
{
    qCDebug(lcDistances) << "Graph::getGeodesicsCount()";

    graphDistancesGeodesic(false, false, false, false);

    qCDebug(lcDistances) << "Graph::getGeodesicsCount() - geodesics:" << m_graphGeodesicsCount;
    return m_graphGeodesicsCount;
}

/**
 * @brief Checks if the graph is connected, in the sense of a topological space,
 * i.e., there is a path from any vertex to any other vertex in the graph.
 * @return bool
 */
bool Graph::isConnected()
{

    qCDebug(lcDistances) << "Graph::isConnected() ";

    if (calculatedDistances)
    {
        qCDebug(lcDistances) << "Graph::isConnected() - graph unmodified. Returning:"
                 << m_graphIsConnected;
        return m_graphIsConnected;
    }

    graphDistancesGeodesic(false, false, false, false);

    return m_graphIsConnected;
}

// DISTANCE CACHE GETTERS - these return cached values without recalculating anything.
// They are used by the UI and reporting engines to get distance metrics without triggering recalculations.

/**
 * @brief Returns the average geodesic distance of the graph, without recalculating it.
 * @return qreal
 */
qreal Graph::graphDistanceGeodesicAverageCached() const
{
    return m_graphAverageDistance;
}
/**
 * @brief Returns the number of geodesics (shortest paths) in the graph, without recalculating it.
 * @return int
 */
int Graph::graphDiameterCached() const
{
    return m_graphDiameter;
}

/**
 * @brief Returns true if the graph is connected, without recalculating it.
 * @return bool
 */
bool Graph::isConnectedCached() const
{
    return m_graphIsConnected;
}

/**
 * @brief Counts weakly connected components using BFS.
 *
 * Weak connectivity treats every edge as undirected regardless of graph type:
 *   - Undirected: equivalent to ordinary connected components.
 *   - Directed: two nodes are in the same component when there is an undirected
 *     path between them (ignoring arrow direction). Answers the practical
 *     "how many disconnected islands?" question consistently for both types.
 *     Does NOT imply strong connectivity (all-pairs directed reachability);
 *     use isConnected() / graphDistancesGeodesic() for that.
 *
 * m_graphIsConnected is NOT touched here; that flag belongs to the SSSP engine.
 * Component IDs (1-based) are cached in m_vertexComponentId keyed by vertex
 * number, ready for the colorize-by-component layout action.
 *
 * @return Number of weakly connected components.
 */
int Graph::graphWeaklyConnectedComponents()
{
    if (m_graphWeaklyConnectedComponents > 0) {
        qCDebug(lcDistances) << "Graph::graphWeaklyConnectedComponents() - cached:" << m_graphWeaklyConnectedComponents;
        return m_graphWeaklyConnectedComponents;
    }

    qCDebug(lcDistances) << "Graph::graphWeaklyConnectedComponents() - computing";

    const int currentRelation = relationCurrent();
    QHash<int, bool> visited;
    int componentId = 0;

    for (auto it = verticesBegin(); it != verticesEnd(); ++it) {
        const int v = (*it)->number();
        if (!(*it)->isEnabled() || visited.contains(v))
            continue;

        ++componentId;
        QQueue<int> queue;
        queue.enqueue(v);
        visited[v] = true;

        while (!queue.isEmpty()) {
            const int u = queue.dequeue();
            const int ui = vertexIndexByNumber(u);
            if (ui < 0) continue;
            const GraphVertex *uv = vertexAtIndex(ui);

            // Follow out-edges
            for (auto eit = uv->outEdges().cbegin(); eit != uv->outEdges().cend(); ++eit) {
                if (eit.value().first != currentRelation) continue;
                if (!eit.value().second.second) continue;
                const int w = eit.key();
                if (!visited.contains(w)) {
                    visited[w] = true;
                    queue.enqueue(w);
                }
            }

            // Follow in-edges (makes traversal undirected → weak connectivity)
            for (auto eit = uv->inEdges().cbegin(); eit != uv->inEdges().cend(); ++eit) {
                if (eit.value().first != currentRelation) continue;
                if (!eit.value().second.second) continue;
                const int w = eit.key();
                if (!visited.contains(w)) {
                    visited[w] = true;
                    queue.enqueue(w);
                }
            }

            m_vertexComponentId[u] = componentId;
        }
    }

    m_graphWeaklyConnectedComponents = componentId;
    qCDebug(lcDistances) << "Graph::graphWeaklyConnectedComponents() -" << componentId << "component(s)";
    return m_graphWeaklyConnectedComponents;
}

/**
 * @brief Counts strongly connected components using Tarjan's algorithm.
 *
 * Strong connectivity respects edge direction: two nodes are in the same strongly connected
 * component (SCC) only if each is reachable from the other via directed edges. This is a strictly
 * finer partition than weak connectivity (graphWeaklyConnectedComponents(), which treats every
 * edge as undirected) - a graph can be a single weak component while having many strong
 * components, e.g. a directed path a->b->c is one weak component but three strong components,
 * since c cannot reach a. For an undirected graph the two notions coincide (every edge is
 * effectively reciprocal), so this still returns the right answer, just via a slightly more
 * roundabout route than graphWeaklyConnectedComponents().
 *
 * Algorithm (Tarjan 1972): a single DFS assigns each vertex a discovery "index" (the order it was
 * first visited) and a "lowlink" - the smallest index reachable from that vertex by following zero
 * or more tree edges and then at most one edge back into an ancestor still on the DFS stack.
 * Vertices are pushed onto an explicit stack as they're discovered and popped once a whole SCC is
 * found. A vertex is the *root* of an SCC exactly when its lowlink equals its own index - meaning
 * nothing below it on the DFS stack can reach back above it - and popping the stack down to and
 * including that root yields exactly the members of one SCC. This is O(V+E), a single DFS pass,
 * with no graph transpose needed - unlike Kosaraju's algorithm, which gets the same complexity but
 * needs two passes over a transposed graph. Tarjan's is the better fit here since SocNetV already
 * has cheap out-edge iteration and no existing transpose-graph structure to reuse.
 *
 * Implemented as an explicit-stack simulation of the textbook recursive DFS, not recursion itself:
 * SocNetV networks can have a directed path of a few thousand nodes (a plausible worst case for a
 * citation or hierarchy network), which would recurse that deep and risk overflowing the real call
 * stack. Each simulated stack frame remembers which out-edge it was in the middle of examining
 * (Frame::cursor), which is exactly the resumption point a real call stack would give for free
 * after a simulated recursive call "returns" (its frame is popped).
 *
 * m_vertexComponentId (weak-component IDs, used by the colorize-by-component layout action) is
 * NOT touched here; strong components are not currently exposed per-vertex, only as a count.
 *
 * @return Number of strongly connected components.
 */
int Graph::graphStronglyConnectedComponents()
{
    if (m_graphStronglyConnectedComponents > 0) {
        qCDebug(lcDistances) << "Graph::graphStronglyConnectedComponents() - cached:" << m_graphStronglyConnectedComponents;
        return m_graphStronglyConnectedComponents;
    }

    qCDebug(lcDistances) << "Graph::graphStronglyConnectedComponents() - computing";

    const int currentRelation = relationCurrent();

    struct Frame {
        int v = 0;
        QList<int> neighbors;
        int cursor = 0;
    };

    auto outNeighborsOf = [this, currentRelation](int v) {
        QList<int> neighbors;
        const int vi = vertexIndexByNumber(v);
        const GraphVertex *gv = vertexAtIndex(vi);
        for (auto eit = gv->outEdges().cbegin(); eit != gv->outEdges().cend(); ++eit) {
            if (eit.value().first != currentRelation) continue;
            if (!eit.value().second.second) continue;
            neighbors.append(eit.key());
        }
        return neighbors;
    };

    QHash<int, int> index, lowlink;
    QHash<int, bool> onStack;
    QList<int> tarjanStack;
    int nextIndex = 0;
    int componentCount = 0;

    for (auto it = verticesBegin(); it != verticesEnd(); ++it) {
        const int start = (*it)->number();
        if (!(*it)->isEnabled() || index.contains(start))
            continue;

        QList<Frame> callStack;
        {
            Frame f;
            f.v = start;
            f.neighbors = outNeighborsOf(start);
            index[start] = nextIndex;
            lowlink[start] = nextIndex;
            ++nextIndex;
            tarjanStack.append(start);
            onStack[start] = true;
            callStack.append(f);
        }

        while (!callStack.isEmpty()) {
            const int top = callStack.size() - 1;

            if (callStack[top].cursor < callStack[top].neighbors.size()) {
                const int w = callStack[top].neighbors[callStack[top].cursor];
                ++callStack[top].cursor;

                if (!index.contains(w)) {
                    // Tree edge: simulate a recursive call by pushing a new frame. Note we don't
                    // touch callStack[top] again after this append - QList::append() may
                    // reallocate the backing array, which would invalidate any reference taken
                    // before the call, so every access below is by fresh index, never by
                    // reference held across a mutation.
                    Frame nf;
                    nf.v = w;
                    nf.neighbors = outNeighborsOf(w);
                    index[w] = nextIndex;
                    lowlink[w] = nextIndex;
                    ++nextIndex;
                    tarjanStack.append(w);
                    onStack[w] = true;
                    callStack.append(nf);
                } else if (onStack.value(w, false)) {
                    // Back/cross edge into a vertex still on the stack: it's part of the same
                    // SCC-in-progress, so it can lower this vertex's lowlink.
                    lowlink[callStack[top].v] = std::min(lowlink[callStack[top].v], index[w]);
                }
            } else {
                // All of this vertex's out-edges are examined - "return" from the simulated call.
                const int v = callStack[top].v;
                if (lowlink[v] == index[v]) {
                    // v is an SCC root: everything above it on the Tarjan stack, plus v itself,
                    // is exactly one strongly connected component.
                    ++componentCount;
                    int w;
                    do {
                        w = tarjanStack.takeLast();
                        onStack[w] = false;
                    } while (w != v);
                }
                callStack.removeLast();
                if (!callStack.isEmpty()) {
                    const int parentTop = callStack.size() - 1;
                    lowlink[callStack[parentTop].v] = std::min(lowlink[callStack[parentTop].v], lowlink[v]);
                }
            }
        }
    }

    m_graphStronglyConnectedComponents = componentCount;
    qCDebug(lcDistances) << "Graph::graphStronglyConnectedComponents() -" << componentCount << "component(s)";
    return m_graphStronglyConnectedComponents;
}

/**
 * @brief Returns the sum of all finite geodesic distances accumulated by DistanceEngine,
 * without recalculating anything.
 */
qreal Graph::graphSumDistanceCached() const
{
    return m_graphSumDistance;
}

/**
 * @brief Returns the number of geodesics (shortest paths) accumulated by DistanceEngine,
 * without recalculating anything.
 */
qreal Graph::graphGeodesicsCountCached() const
{
    return m_graphGeodesicsCount;
}
