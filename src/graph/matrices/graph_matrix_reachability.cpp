/**
 * @file graph_matrix_reachability.cpp
 * @brief Implements construction of the XRM (reachability) and XM/XSM
 *        (walks) matrices for the Graph class.
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
 * @brief Creates the reachability matrix XRM
 */
void Graph::createMatrixReachability()
{
    qCDebug(lcReachability) << "Creating the Reachability Matrix...";

    graphDistancesGeodesic(false);
    if (progressCanceled())
    {
        return;
    }
    VList::const_iterator it, jt;

    int N = vertices(false, false, true);

    int source = 0, target = 0;
    int i = 0, j = 0;
    int reachVal = 0;

    XRM.resize(N, N);

    QString pMsg = tr("Creating reachability matrix. \nPlease wait ");
    progressStatus(pMsg);

    qCDebug(lcReachability) << "Writing Reachability matrix...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (progressCanceled())
        {
            return;
        }
        source = (*it)->number();

        if (!(*it)->isEnabled())
        {
            qCDebug(lcReachability) << "source vertex" << source << "disabled. SKIP";
            continue;
        }

        qCDebug(lcReachability) << "source vertex" << source << "i" << i;

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            target = (*jt)->number();

            if (!(*jt)->isEnabled())
            {
                qCDebug(lcReachability) << "target vertex" << target << "disabled. SKIP";
                continue;
            }

            qCDebug(lcReachability) << "target vertex" << target << "j" << j;

            reachVal = (apspDistance(source, target) != RAND_MAX) ? 1 : 0;
            qCDebug(lcReachability) << "Setting XRM (" << i << "," << j << ") =" << reachVal;
            XRM.setItem(i, j, reachVal);

            j++;
        }
        j = 0;
        i++;
    }
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
 */
void Graph::graphWalksMatrixCreate(const int &N,
                                   const int &length,
                                   const bool &dropIsolates,
                                   const bool &considerWeights,
                                   const bool &inverseWeights,
                                   const bool &symmetrize)
{
    // Build adjacency matrix with explicit policy (BUGFIX: do not force weights)
    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);
    if (progressCanceled())
    {
        return;
    }
    if (length > 0)
    {
        qCDebug(lcReachability) << "Graph::graphWalksMatrixCreate() - "
                    "Calculating sociomatrix power"
                 << length;

        QString pMsg = tr("Computing walks of length %1. \nPlease wait...").arg(length);
        progressStatus(pMsg);

        XM = AM.pow(length, false);
    }
    else
    {
        qCDebug(lcReachability) << "Graph::graphWalksMatrixCreate() - "
                    "Calculating all sociomatrix powers up to"
                 << N - 1;

        XM = AM;  // product matrix
        XSM = AM; // sum of product matrices

        QString pMsg = tr("Computing sociomatrix powers up to %1. \nPlease wait...").arg(N - 1);
        progressStatus(pMsg);

        for (int i = 2; i <= (N - 1); ++i)
        {
            progressStatus(tr("Computing all sociomatrix powers up to %1. "
                              "Now computing A^%2. Please wait...")
                               .arg(N - 1)
                               .arg(i));

            XM *= AM;
            XSM += XM;

            if (progressCanceled())
            {
                return;
            }
        }
    }
}
