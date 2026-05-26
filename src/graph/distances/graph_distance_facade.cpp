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
#include <QQueue>

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
    qDebug() << "Graph::graphDistanceGeodesic()";
    graphDistancesGeodesic(false, considerWeights, inverseWeights, false);
    return m_graph[vpos[v1]]->distance(v2);
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
    qDebug() << "Graph::graphDiameter()";
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

    qDebug() << "Graph::graphDistanceGeodesicAverage() - Computing distances...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    qDebug() << "Graph::graphDistanceGeodesicAverage() - "
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
    qDebug() << "Graph::getGeodesicsCount()";

    graphDistancesGeodesic(false, false, false, false);

    qDebug() << "Graph::getGeodesicsCount() - geodesics:" << m_graphGeodesicsCount;
    return m_graphGeodesicsCount;
}

/**
 * @brief Checks if the graph is connected, in the sense of a topological space,
 * i.e., there is a path from any vertex to any other vertex in the graph.
 * @return bool
 */
bool Graph::isConnected()
{

    qDebug() << "Graph::isConnected() ";

    if (calculatedDistances)
    {
        qDebug() << "Graph::isConnected() - graph unmodified. Returning:"
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
        qDebug() << "Graph::graphWeaklyConnectedComponents() - cached:" << m_graphWeaklyConnectedComponents;
        return m_graphWeaklyConnectedComponents;
    }

    qDebug() << "Graph::graphWeaklyConnectedComponents() - computing";

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
    qDebug() << "Graph::graphWeaklyConnectedComponents() -" << componentId << "component(s)";
    return m_graphWeaklyConnectedComponents;
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
