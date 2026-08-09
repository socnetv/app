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

#include <QDebug>

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
    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate()";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    if (progressCanceled())
    {
        calculatedDistances = false;
        return;
    }

    VList::const_iterator it, jt;
    int N = vertices(dropIsolates, false, true);
    int source = 0, target = 0;
    int i = 0, j = 0;

    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - Resizing matrix to hold "
             << N << " vertices";

    SIGMA.resize(N, N);

    QString pMsg = tr("Creating shortest paths matrix. \nPlease wait ");
    progressStatus(pMsg);

    qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - Writing shortest paths matrix...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (progressCanceled())
        {
            calculatedDistances = false;
            return;
        }

        source = (*it)->number();

        if ((*it)->isIsolated() && dropIsolates)
        {
            qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "isolated. SKIP";

            continue;
        }

        if (!(*it)->isEnabled())
        {
            qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - "
                     << source << "disabled. SKIP";
            continue;
        }

        qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - source" << source
                 << "i" << i;

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            target = (*jt)->number();

            if ((*jt)->isIsolated() && dropIsolates)
            {
                qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "isolated. SKIP";
                continue;
            }

            if (!(*jt)->isEnabled())
            {
                qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - "
                         << target << "disabled. SKIP";
                continue;
            }

            qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() - "
                     << "target" << target << "j" << j;

            qCDebug(lcDistances) << "Graph::graphMatrixShortestPathsCreate() -  setting SIGMA ("
                     << i << "," << j << ") =" << apspShortestPaths(source, target);
            SIGMA.setItem(i, j, apspShortestPaths(source, target));
            j++;
        }
        j = 0;
        i++;
    }
}

/**
 * @brief Creates the matrix DM of geodesic distances between vertices.
 *
 * Phase 1: calls graphDistancesGeodesic() which runs the DistanceEngine.
 * The engine owns its own progress dialog — it creates it, updates it,
 * and destroys it before returning. The dialog stack is empty on return.
 *
 * Phase 2: fills the DM matrix from the cached per-vertex distances.
 * This is an O(N²) memory-write pass — fast enough to need no progress
 * dialog of its own. No progressUpdate or progressFinish is called here;
 * the caller owns the dialog lifecycle for any subsequent phase.
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
    // The engine owns its own progress dialog for this phase.
    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    if (progressCanceled())
    {
        calculatedDistances = false;
        return false;
    }

    VList::const_iterator it, jt;
    int N = vertices(dropIsolates, false, true);
    int source = 0, target = 0;
    int i = 0, j = 0;

    qCDebug(lcDistances) << "Graph::graphMatrixDistanceGeodesicCreate() - "
                "Resizing distance matrix to hold "
             << N << " vertices";

    DM.resize(N, N);

    // Phase 2: fill DM from cached per-vertex distances.
    // No progressUpdate here — the DistanceEngine dialog is already destroyed
    // by this point. The matrix-fill is O(N²) memory writes and needs no
    // progress reporting of its own.
    progressStatus(tr("Creating geodesic distances matrix. \nPlease wait "));

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        source = (*it)->number();

        if ((*it)->isIsolated() && dropIsolates) {
            continue;
        }
        if (!(*it)->isEnabled()) {
            continue;
        }

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {
            target = (*jt)->number();

            if ((*jt)->isIsolated() && dropIsolates) {
                continue;
            }
            if (!(*jt)->isEnabled()) {
                continue;
            }


            DM.setItem(i, j, apspDistance(source, target));
            j++;
        }
        j = 0;
        i++;
    }

    // No progressFinish() here — the caller owns the outer dialog lifecycle.
    return true;
}
