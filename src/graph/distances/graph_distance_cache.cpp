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
 * @brief Creates the matrix SIGMA of shortest paths (geodesics) between vertices
 * Each SIGMA(i,j) is the number of shortest paths (geodesics) from i and j
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphMatrixShortestPathsCreate(const bool &considerWeights,
                                           const bool &inverseWeights,
                                           const bool &dropIsolates)
{
    qDebug() << "Graph::graphMatrixShortestPathsCreate()";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    VList::const_iterator it, jt;

    int N = vertices(dropIsolates, false, true);

    int progressCounter = 0;
    int source = 0, target = 0;
    int i = 0, j = 0;

    qDebug() << "Graph::graphMatrixShortestPathsCreate() - Resizing matrix to hold "
             << N << " vertices";

    SIGMA.resize(N, N);

    QString pMsg = tr("Creating shortest paths matrix. \nPlease wait ");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    qDebug() << "Graph::graphMatrixShortestPathsCreate() - Writing shortest paths matrix...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if ((*it)->isIsolated() && dropIsolates)
        {
            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "isolated. SKIP";

            continue;
        }

        if (!(*it)->isEnabled())
        {
            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "disabled. SKIP";
            continue;
        }

        qDebug() << "Graph::graphMatrixShortestPathsCreate() - source" << source
                 << "i" << i;

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            target = (*jt)->number();

            if ((*jt)->isIsolated() && dropIsolates)
            {
                qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "isolated. SKIP";
                continue;
            }

            if (!(*jt)->isEnabled())
            {
                qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "Graph::graphMatrixShortestPathsCreate() - "
                     << "target" << target << "j" << j;

            qDebug() << "Graph::graphMatrixShortestPathsCreate() -  setting SIGMA ("
                     << i << "," << j << ") =" << (*it)->shortestPaths(target);
            SIGMA.setItem(i, j, (*it)->shortestPaths(target));
            j++;
        }
        j = 0;
        i++;
    }

    emit signalProgressBoxKill();
}

/**
 * @brief Creates the matrix DM of geodesic distances between vertices
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphMatrixDistanceGeodesicCreate(const bool &considerWeights,
                                              const bool &inverseWeights,
                                              const bool &dropIsolates)
{
    qDebug() << "Graph::graphMatrixDistanceGeodesicCreate()";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    VList::const_iterator it, jt;

    int N = vertices(dropIsolates, false, true);

    int progressCounter = 0;
    int source = 0, target = 0;
    int i = 0, j = 0;

    qDebug() << "Graph::graphMatrixDistanceGeodesicCreate() - "
                "Resizing distance matrix to hold "
             << N << " vertices";

    DM.resize(N, N);

    QString pMsg = tr("Creating geodesic distances matrix. \nPlease wait ");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - Writing distances matrix...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if ((*it)->isIsolated() && dropIsolates)
        {
            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << source << "isolated. SKIP";

            continue;
        }

        if (!(*it)->isEnabled())
        {
            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << source << "disabled. SKIP";
            continue;
        }

        qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - source"
                 << source << "i" << i;

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            target = (*jt)->number();

            if ((*jt)->isIsolated() && dropIsolates)
            {
                qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                         << target << "isolated. SKIP";
                continue;
            }

            if (!(*jt)->isEnabled())
            {
                qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                         << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() - "
                     << "target" << target << "j" << j;

            qDebug() << "Graph: graphMatrixDistanceGeodesicCreate() -  setting DM ("
                     << i << "," << j << ") =" << (*it)->distance(target);
            DM.setItem(i, j, (*it)->distance(target));

            j++;
        }
        j = 0;
        i++;
    }

    emit signalProgressBoxKill();
}

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
 *   - Stress: SC(u) = Sum ( sigma(i,j) ) for every s,t in V
 *   - Eccentricity: EC(u) =  1/maxDistance(u,t)  for some t in V
 *   - Closeness: CC(u) =  1 / Sum( d(u,t) )  for every  t in V
 *   - Power:
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
// SSSP helpers
//
void Graph::ssspStackClear()
{
    while (!Stack.empty())
    {
        Stack.pop();
    }
}

bool Graph::ssspStackEmpty() const
{
    return Stack.empty();
}

int Graph::ssspStackTop() const
{
    return Stack.top();
}

void Graph::ssspStackPop()
{
    Stack.pop();
}

int Graph::ssspStackSize() const
{
    return static_cast<int>(Stack.size());
}

void Graph::ssspStackPush(int v) { 
    Stack.push(v); 
}


void Graph::ssspNthOrderClear()
{
    sizeOfNthOrderNeighborhood.clear();
}

H_f_i::const_iterator Graph::ssspNthOrderBegin() const
{
    return sizeOfNthOrderNeighborhood.constBegin();
}

H_f_i::const_iterator Graph::ssspNthOrderEnd() const
{
    return sizeOfNthOrderNeighborhood.constEnd();
}
int Graph::ssspNthOrderValue(qreal dist) const
{
    return sizeOfNthOrderNeighborhood.value(dist, 0);
}
void Graph::ssspNthOrderIncrement(int dist)
{
    sizeOfNthOrderNeighborhood.insert(
        dist,
        sizeOfNthOrderNeighborhood.value(dist, 0) + 1);
}
void Graph::ssspNthOrderIncrement(qreal dist)
{
    sizeOfNthOrderNeighborhood.insert(
        dist,
        sizeOfNthOrderNeighborhood.value(dist, 0) + 1);
}
void Graph::ssspComponentReset(int value)
{
    sizeOfComponent = value;
}

void Graph::ssspComponentAdd(int delta)
{
    sizeOfComponent += delta;
}

int Graph::ssspComponentSize() const
{
    return sizeOfComponent;
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
}

void Graph::setSymmetricCached(bool v) { m_graphIsSymmetric = v; }
bool Graph::symmetricCached() const { return m_graphIsSymmetric; }

void Graph::setConnectedCached(bool v) { m_graphIsConnected = v; }
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
void Graph::setAverageDistanceCached(qreal v) { m_graphAverageDistance = v; }
