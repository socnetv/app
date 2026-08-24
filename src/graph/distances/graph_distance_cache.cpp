/**
 * @file graph_distance_cache.cpp
 * @brief Implements distance matrix creation, geodesic aggregation, SSSP helpers, and caching logic for the Graph class.
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
#include "engine/distance_engine.h"
#include <QDebug>

//
// Distance matrices / wrapper hub
//

/**
 * @brief Computes the geodesic distances between all vertices:
 * In the process, it also computes many other centrality/prestige metrics:
 * * The so-called sigma matrix, where the (i,j) element is the number of shortest paths
 *   from vertex i to vertex j, called sigma(i,j).
 * * The Diameter of the graph, m_graphDiameter, which is the length of the longest
 *   shortest path between every (i,j)
 * * The Eccentricity of every node i which is the length of the longest shortest
 *   path from i to every other node j
 * * The InfluenceRange and InfluenceDomain of each node.
 * * The centralities for every u in V (if centralities=true):
 *   - Betweenness: BC(u) = Sum ( sigma(i,j,u)/sigma(i,j) ) for every s,t in V
 *     Plain language: how often u sits "in between" on the shortest routes connecting
 *     other pairs of actors - a broker/gatekeeper measure. High betweenness means removing
 *     u would disrupt many people's shortest path to each other.
 *   - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
 *     Plain language: like betweenness, but simply counts how many shortest paths pass
 *     through u, without dividing by how many alternative shortest paths existed for that
 *     pair - so it also rewards actors on many paths even when those paths weren't a pair's
 *     *only* shortest route.
 *   - Eccentricity: EC(u) =  1/maxDistance(u,t)  for some t in V
 *     Plain language: a worst-case reachability measure - how far away is u's single most
 *     distant counterpart? High eccentricity centrality means even u's "hardest to reach"
 *     other actor is nearby.
 *   - Closeness: CC(u) =  1 / Sum( d(u,t) )  for every  t in V
 *     Plain language: how close u is, on average, to everyone else - a low total distance
 *     to others gives a high closeness score. Only meaningful on a fully connected graph,
 *     since an unreachable actor has undefined distance (see IRCC, centralityClosenessIR(),
 *     for the disconnected-graph-friendly variant).
 *   - Power (Gil-Schmidt): PC(s) = [1/(N-1)] * Sum_i( nthOrder[i] / i ), where nthOrder[i] is
 *     the number of nodes at distance i from s (computed in DistanceEngine::compute()).
 *     Plain language: a generalized degree measure that gives (shrinking) credit for nodes
 *     several steps away too, not just direct neighbors - similar in spirit to eigenvector
 *     centrality, but computed directly from how many nodes sit at each distance rather than
 *     via eigen-decomposition. Not to be confused with Bonacich's differently-named "Power
 *     Centrality" measure.
 * @param centralities
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */

void Graph::graphDistancesGeodesic(const bool &computeCentralities,
                                   const bool &considerWeights,
                                   const bool &inverseWeights,
                                   const bool &dropIsolates)
{
    DistanceEngine engine(*this);
    engine.compute(computeCentralities,
                   considerWeights,
                   inverseWeights,
                   dropIsolates);
}

//
// DISCONNECTED PAIRS CACHE
// During SSSP, we may find pairs of vertices that are not connected.
// We store these in a hash for quick lookup, so that if we encounter the same pair again,
// we can immediately return "not connected" without recalculating.
//
void Graph::notConnectedPairsClear()
{
    m_vertexPairsNotConnected.clear();
}

void Graph::notConnectedPairsInsert(int from, int to)
{
    m_vertexPairsNotConnected.insert(from, to);
}

int Graph::notConnectedPairsSize() const
{
    return m_vertexPairsNotConnected.size();
}

//
// DISTANCE CENTRALITY CACHE FLAGS
//
void Graph::resetDistanceCentralityCacheFlags()
{
    calculatedDistances = false;
    calculatedCentralities = false;
    m_graphWeaklyConnectedComponents = 0;
    m_graphStronglyConnectedComponents = 0;
    m_vertexComponentId.clear();
}

void Graph::setSymmetricCached(bool v) { m_graphIsSymmetric = v; }
bool Graph::symmetricCached() const { return m_graphIsSymmetric; }

void Graph::setConnectedCached(bool v) { m_graphIsConnected = v; }

int Graph::graphWeaklyConnectedComponentsCached() const { return m_graphWeaklyConnectedComponents; }
int Graph::graphStronglyConnectedComponentsCached() const { return m_graphStronglyConnectedComponents; }
void Graph::setDiameterCached(int v) { m_graphDiameter = v; }

void Graph::resetDistanceAggregates()
{
    m_graphDiameter = 0;
    m_graphAverageDistance = 0;
    m_graphSumDistance = 0;
    m_graphGeodesicsCount = 0;
}

void Graph::addToDistanceSum(qreal delta) { m_graphSumDistance += delta; }
void Graph::incGeodesicsCount() { ++m_graphGeodesicsCount; }
void Graph::addGeodesicsCount(int n) { m_graphGeodesicsCount += n; }
void Graph::setAverageDistanceCached(qreal v) { m_graphAverageDistance = v; }
