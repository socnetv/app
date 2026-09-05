/**
 * @file graph_matrix_distances.cpp
 * @brief Implements construction of the SIGMA (shortest-paths count) and DM
 *        (geodesic distance) matrices for the Graph class, from cached APSP results.
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

#include <QAtomicInteger>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

/**
 * @brief Maps each vertex's position in m_graph to its compacted row/column index in an
 * N-by-N Matrix, or -1 if the vertex is excluded (disabled, or isolated with dropIsolates).
 *
 * Parallelization (WS15 P4): the matrix-fill loops below used to track their row/column
 * index with plain int i/j counters incremented sequentially as included vertices were
 * visited - safe single-threaded, but that sequential dependency is exactly what blocks
 * mapping the outer loop over worker threads (each one needs to know its own row index
 * without waiting for every earlier vertex to be visited first). Precomputing the mapping
 * once, sequentially, lets each worker thread look up its own (and any other vertex's)
 * compacted index independently.
 *
 * @param dropIsolates  If true, isolated vertices are also excluded (in addition to
 *        disabled ones) - matches each caller's own existing skip logic exactly.
 * @return QVector<int> sized m_graph.size(), indexed by position in m_graph.
 */
QVector<int> Graph::compactedMatrixIndex(const bool &dropIsolates) const
{
    QVector<int> index(m_graph.size(), -1);
    int next = 0;
    for (int pos = 0; pos < m_graph.size(); ++pos)
    {
        GraphVertex *v = m_graph.at(pos);
        if (!v->isEnabled() || (v->isIsolated() && dropIsolates))
            continue;
        index[pos] = next++;
    }
    return index;
}

/**
 * @brief Creates the matrix SIGMA of shortest paths (geodesics) between vertices
 * Each SIGMA(i,j) is the number of shortest paths (geodesics) from i and j
 *
 * Parallelization (WS15 P4): the outer vertex loop maps via QtConcurrent::blockingMap over
 * vertex positions, using compactedMatrixIndex() (computed once, sequentially, beforehand)
 * in place of the old sequential i/j counters. Each worker thread writes only to its own
 * row of SIGMA - disjoint cells, safe for concurrent writes (Matrix storage is a flat
 * array with no locking/COW, see Matrix::setItem()). Cancel signals cannot be delivered
 * while graphThread's event loop is blocked in blockingMap, so the progressCanceled()
 * check only runs once, before the parallel step, not per-row as before.
 *
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::graphMatrixShortestPathsCreate(const bool &considerWeights,
                                           const bool &inverseWeights,
                                           const bool &dropIsolates)
{
    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate()";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    if (progressCanceled())
    {
        calculatedDistances = false;
        return;
    }

    int N = vertices(dropIsolates, false, true);

    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - Resizing matrix to hold "
             << N << " vertices";

    SIGMA.resize(N, N);

    QString pMsg = tr("Creating shortest paths matrix. \nPlease wait ");
    progressStatus(pMsg);

    if (progressCanceled())
    {
        calculatedDistances = false;
        return;
    }

    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - Writing shortest paths matrix...";

    const QVector<int> rowOf = compactedMatrixIndex(dropIsolates);
    const int total = m_graph.size();

    QList<int> positions;
    positions.reserve(total);
    for (int p = 0; p < total; ++p)
        positions.append(p);

    QtConcurrent::blockingMap(positions, [&](int p1) {
        const int i = rowOf[p1];
        if (i < 0)
            return;

        const int source = m_graph.at(p1)->number();

        for (int p2 = 0; p2 < total; ++p2)
        {
            const int j = rowOf[p2];
            if (j < 0)
                continue;

            const int target = m_graph.at(p2)->number();
            SIGMA.setItem(i, j, apspShortestPaths(source, target));
        }
    });
}

/**
 * @brief Creates the matrix DM of geodesic distances between vertices.
 *
 * Phase 1: calls graphDistancesGeodesic() which runs the DistanceEngine.
 *
 * Phase 2: fills the DM matrix from the cached per-vertex distances - an O(N²)
 * memory-write pass.
 *
 * Parallelization (WS15 P4): same pattern as graphMatrixShortestPathsCreate() above -
 * compactedMatrixIndex() replaces the old sequential i/j counters, each worker thread
 * writes only to its own disjoint row of DM. The progressCanceled() check that used to
 * only exist before Phase 2 (none inside the O(N²) loop itself) is unchanged in spirit -
 * still a single check before the parallel step, since a mid-loop check couldn't be
 * delivered under blockingMap anyway.
 *
 * @param considerWeights If true, edge weights are used in distance computations.
 * @param inverseWeights  If true, edge weights are inverted before use.
 * @param dropIsolates    If true, isolate nodes are excluded from the analysis.
 * @return true on success, false if the computation was cancelled.
 */
bool Graph::graphMatrixDistanceGeodesicCreate(const bool &considerWeights,
                                              const bool &inverseWeights,
                                              const bool &dropIsolates)
{
    qCDebug(lcDistances) << "Graph::graphMatrixDistanceGeodesicCreate()";

    // Phase 1: compute all geodesic distances via DistanceEngine.
    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    if (progressCanceled())
    {
        calculatedDistances = false;
        return false;
    }

    int N = vertices(dropIsolates, false, true);

    qCDebug(lcDistances) << "Graph::graphMatrixDistanceGeodesicCreate() - "
                "Resizing distance matrix to hold "
             << N << " vertices";

    DM.resize(N, N);

    // Phase 2: fill DM from cached per-vertex distances.
    progressStatus(tr("Creating geodesic distances matrix. \nPlease wait "));

    if (progressCanceled())
    {
        calculatedDistances = false;
        return false;
    }

    const QVector<int> rowOf = compactedMatrixIndex(dropIsolates);
    const int total = m_graph.size();

    QList<int> positions;
    positions.reserve(total);
    for (int p = 0; p < total; ++p)
        positions.append(p);

    QtConcurrent::blockingMap(positions, [&](int p1) {
        const int i = rowOf[p1];
        if (i < 0)
            return;

        const int source = m_graph.at(p1)->number();

        for (int p2 = 0; p2 < total; ++p2)
        {
            const int j = rowOf[p2];
            if (j < 0)
                continue;

            const int target = m_graph.at(p2)->number();
            DM.setItem(i, j, apspDistance(source, target));
        }
    });

    return true;
}
