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
    qCDebug(lcReachability) << "Graph::reachable()";
    graphDistancesGeodesic(false);
    return (apspDistance(v1, v2) != RAND_MAX) ? true : false;
}

// XRM (createMatrixReachability) and XM/XSM (graphWalksMatrixCreate) construction
// moved to matrices/graph_matrix_reachability.cpp (WS5 A4).

/**
 * @brief Calculates and returns the number of walks of a given length between v1 and v2
 * @param v1
 * @param v2
 * @param length
 * @return
 */
int Graph::walksBetween(int v1, int v2, int length)
{
    const bool dropIsolates = false;
    const bool considerWeights = false; // counting walks, not weight-products
    const bool inverseWeights = false;
    const bool symmetrize = false;
    graphWalksMatrixCreate(vertices(), length,
                           dropIsolates, considerWeights,
                           inverseWeights, symmetrize);
    return XM.item(v1 - 1, v2 - 1);
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