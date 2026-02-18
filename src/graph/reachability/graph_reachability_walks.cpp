/**
 * @file graph_reachability_walks.cpp
 * @brief Implements reachability analysis and walk-based algorithms
 *        for the Graph class.
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
 * @brief Returns true if vertices v1 and v2 are reachable.
 *
 * @param v1
 * @param v2
 * @return bool
 */
bool Graph::graphReachable(const int &v1, const int &v2)
{
    qDebug() << "Graph::reachable()";
    graphDistancesGeodesic(false);
    return (m_graph[vpos[v1]]->distance(v2) != RAND_MAX) ? true : false;
}

/**
 * @brief Creates the reachability matrix XRM
 */
void Graph::createMatrixReachability()
{
    qDebug() << "Creating the Reachability Matrix...";

    graphDistancesGeodesic(false);

    VList::const_iterator it, jt;

    int N = vertices(false, false, true);

    int progressCounter = 0;
    int source = 0, target = 0;
    int i = 0, j = 0;
    int reachVal = 0;

    XRM.resize(N, N);

    QString pMsg = tr("Creating reachability matrix. \nPlease wait ");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    qDebug() << "Writing Reachability matrix...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if (!(*it)->isEnabled())
        {
            qDebug() << "source vertex" << source << "disabled. SKIP";
            continue;
        }

        qDebug() << "source vertex" << source << "i" << i;

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            target = (*jt)->number();

            if (!(*jt)->isEnabled())
            {
                qDebug() << "target vertex" << target << "disabled. SKIP";
                continue;
            }

            qDebug() << "target vertex" << target << "j" << j;

            reachVal = ((*it)->distance(target) != RAND_MAX) ? 1 : 0;
            qDebug() << "Setting XRM (" << i << "," << j << ") =" << reachVal;
            XRM.setItem(i, j, reachVal);

            j++;
        }
        j = 0;
        i++;
    }

    emit signalProgressBoxKill();
}

/**
 * @brief Calculates and returns the number of walks of a given length between v1 and v2
 * @param v1
 * @param v2
 * @param length
 * @return
 */
int Graph::walksBetween(int v1, int v2, int length)
{
    const bool updateProgress = false;
    const bool considerWeights = false;   // counting walks, not weight-products
    const bool inverseWeights = false;
    const bool dropIsolates = false;
    const bool symmetrize = false;
    graphWalksMatrixCreate(vertices(), length, updateProgress,
                           considerWeights, inverseWeights,
                           dropIsolates, symmetrize);
    return XM.item(v1 - 1, v2 - 1);
}

/**
 * @brief Computes either the "Walks of given length" or the "Total Walks" matrix.
 * If length>0, it computes the Walks of given length matrix, XM=AM^l
 * where each element (i,j) denotes the number of walks of length l between vertex i and j.
 * If length=0, it computes the Total Walks matrix, XSM=Sum{AM^n} where each (i,j)
 * denotes the total number of walks of any length between vertices i and j.
 * NOTE: In the latter case, this function is VERY SLOW on large networks (n>50),
 * since it will calculate all powers of the sociomatrix up to n-1 in order to find out all
 * possible walks.
 * @param N - dimension of the sociomatrix (number of vertices). Default is 0, in which case it will be calculated as the number of vertices in the graph.
 * @param length - the length of walks to be calculated. Default is 0, in which case all walks of any length will be calculated.
 * @param updateProgress
 */
// src/graph/reachability/graph_reachability_walks.cpp

void Graph::graphWalksMatrixCreate(const int &N,
                                   const int &length,
                                   const bool &updateProgress,
                                   const bool &dropIsolates,
                                   const bool &considerWeights,
                                   const bool &inverseWeights,
                                   const bool &symmetrize)
{
    // Build adjacency matrix with explicit policy (BUGFIX: do not force weights)
    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);

    if (length > 0)
    {
        qDebug() << "Graph::graphWalksMatrixCreate() - "
                    "Calculating sociomatrix power"
                 << length;

        QString pMsg = tr("Computing walks of length %1. \nPlease wait...").arg(length);
        emit statusMessage(pMsg);
        if (updateProgress)
            emit signalProgressBoxCreate(length, pMsg);

        XM = AM.pow(length, false);

        if (updateProgress)
            emit signalProgressBoxUpdate(length);
    }
    else
    {
        qDebug() << "Graph::graphWalksMatrixCreate() - "
                    "Calculating all sociomatrix powers up to"
                 << N - 1;

        XM = AM;   // product matrix
        XSM = AM;  // sum of product matrices

        QString pMsg = tr("Computing sociomatrix powers up to %1. \nPlease wait...").arg(N - 1);
        emit statusMessage(pMsg);
        if (updateProgress)
            emit signalProgressBoxCreate(N - 1, pMsg);

        for (int i = 2; i <= (N - 1); ++i)
        {
            emit statusMessage(tr("Computing all sociomatrix powers up to %1. "
                                  "Now computing A^%2. Please wait...")
                                   .arg(N - 1)
                                   .arg(i));

            XM *= AM;
            XSM += XM;

            if (updateProgress)
                emit signalProgressBoxUpdate(i);
        }

        if (updateProgress)
            emit signalProgressBoxUpdate(N - 1);
    }

    if (updateProgress)
        emit signalProgressBoxKill();
}


/**
 * @brief Returns the influence range of vertex v1, namely the set of nodes who are
 *  reachable from v1 (See Wasserman and Faust, pp.200-201, based on Lin, 1976).
 * The Influence Range of vertex v can also be defined as:
 * Ji = Sum [ D(v,j), iff D(v,j) != inf  ] for every j in V, where j!=v and D the distance matrix
 *  This function is for digraphs only
 * @param v1
 * @return
 */
QList<int> Graph::vertexinfluenceRange(int v1)
{

    qDebug() << "Graph::vertexinfluenceRange() - vertex:" << v1;

    graphDistancesGeodesic(false);

    VList::const_iterator jt;

    int N = vertices(false, false, true);

    int progressCounter = 0;
    int target = 0;

    influenceRanges.clear();
    influenceRanges.reserve(N);

    QString pMsg = tr("Creating Influence Range List. \nPlease wait ");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        target = (*jt)->number();

        if (!(*jt)->isEnabled())
        {
            qDebug() << "Graph::vertexinfluenceRange() - target:"
                     << target << "disabled. SKIP";
            continue;
        }

        if (graphDistanceGeodesic(v1, target) != RAND_MAX)
        {
            qDebug() << "Graph::vertexinfluenceRange() - v1 can reach:" << target;
            influenceRanges.insert(v1, target);
        }
    }

    emit signalProgressBoxKill();

    return influenceRanges.values(v1);
}

/**
 * @brief Returns the influence domain of vertex v1, namely the set of nodes who can
 *  reach v1
 * The Influence Domain Ii of vertex v can also be defined as:
 * Ii = Sum [ D(i,v), iff D(i,v) != inf  ] for every in V, where i!=v and D the distance matrix
 *  This function applies to digraphs only
 * @param v1
 * @return
 */
QList<int> Graph::vertexinfluenceDomain(int v1)
{
    qDebug() << "Graph::vertexinfluenceDomain() - vertex:" << v1;

    graphDistancesGeodesic(false);

    VList::const_iterator it;

    int N = vertices(false, false, true);

    int progressCounter = 0;
    int source = 0;

    influenceDomains.clear();
    influenceDomains.reserve(N);

    QString pMsg = tr("Creating Influence Domain List. \nPlease wait ");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        source = (*it)->number();

        if (!(*it)->isEnabled())
        {
            qDebug() << "Graph::vertexinfluenceDomain() - "
                     << source << "disabled. SKIP";
            continue;
        }

        if ((*it)->distance(v1) != RAND_MAX)
        {
            qDebug() << "Graph::vertexinfluenceDomain() - v1 reachable from:" << source;
            influenceDomains.insert(v1, source);
        }
    }

    emit signalProgressBoxKill();

    return influenceDomains.values(v1);
}


/**
    Returns the number of triples of vertex v1
    A triple Υ at a vertex v is a path of length two for which v is the center vertex.
*/
qreal Graph::numberOfTriples(int v1)
{
    qreal totalDegree = 0;
    if (isSymmetric())
    {
        totalDegree = vertexEdgesOutbound(v1);
        return totalDegree * (totalDegree - 1.0) / 2.0;
    }
    totalDegree = vertexEdgesOutbound(v1) + vertexEdgesInbound(v1); // FIXEM
    return totalDegree * (totalDegree - 1.0);
}