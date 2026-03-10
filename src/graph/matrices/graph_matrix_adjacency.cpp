/**
 * @file graph_matrix_adjacency.cpp
 * @brief Implements Graph methods for building adjacency-based matrices (adjacency and inverse adjacency) from the current network state.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
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
 * @brief  Creates an adjacency matrix AM
 *  where AM(i,j)=1 if i is connected to j
 *  and AM(i,j)=0 if i not connected to j
 * @param dropIsolates
 * @param considerWeights
 * @param inverseWeights
 * @param symmetrize
 */
void Graph::createMatrixAdjacency(const bool dropIsolates,
                                  const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool symmetrize)
{
    qDebug() << "Graph::createMatrixAdjacency() "
             << "dropIsolates" << dropIsolates
             << "considerWeights" << considerWeights
             << "inverseWeights" << inverseWeights
             << "symmetrize" << symmetrize;
    qreal m_weight = RAND_MAX;
    int i = 0, j = 0;
    int N = vertices(dropIsolates, false, true), progressCounter = 0;
    VList::const_iterator it, jt;

    qDebug() << "Graph::createMatrixAdjacency() -resizing AM to" << N;
    AM.resize(N, N);

    QString pMsg = tr("Creating Adjacency Matrix. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        qDebug() << "Graph::createMatrixAdjacency() - i" << i << "name" << (*it)->number();

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            return;
        }
        if (!(*it)->isEnabled() || ((*it)->isIsolated() && dropIsolates))
        {
            qDebug() << "Graph::createMatrixAdjacency() - SKIP i" << i << "name" << (*it)->number();
            continue;
        }

        j = i;

        for (jt = it; jt != m_graph.end(); jt++)
        {

            qDebug() << "Graph::createMatrixAdjacency() - j" << j << "name" << (*jt)->number();

            if (!(*jt)->isEnabled() || ((*jt)->isIsolated() && dropIsolates))
            {
                qDebug() << "Graph::createMatrixAdjacency() - SKIP j" << j << "name" << (*jt)->number();
                continue;
            }

            if ((m_weight = edgeExists((*it)->number(), (*jt)->number())) != 0)
            {
                if (!considerWeights)
                {
                    AM.setItem(i, j, 1);
                }
                else
                {
                    if (inverseWeights)
                        AM.setItem(i, j, 1.0 / m_weight);
                    else
                        AM.setItem(i, j, m_weight);
                }
            }
            else
            {
                AM.setItem(i, j, 0);
            }

            qDebug() << " AM(" << i << "," << j << ") = " << AM.item(i, j);

            if (i != j)
            {
                if ((m_weight = edgeExists((*jt)->number(), (*it)->number())) != 0)
                {
                    if (!considerWeights)
                    {
                        AM.setItem(j, i, 1);
                    }
                    else
                    {
                        if (inverseWeights)
                            AM.setItem(j, i, 1.0 / m_weight);
                        else
                            AM.setItem(j, i, m_weight);
                    }
                    if (symmetrize && (AM.item(i, j) != AM.item(j, i)))
                    {
                        AM.setItem(i, j, AM.item(j, i));
                    }
                }
                else
                {
                    AM.setItem(j, i, 0);
                    if (symmetrize && (AM.item(i, j) != AM.item(j, i)))
                        AM.setItem(j, i, AM.item(i, j));
                }
                qDebug() << " AM(" << j << "," << i << ") = " << AM.item(j, i);
            }
            j++;
        }
        i++;
    }

    calculatedAdjacencyMatrix = true;

    progressFinish();
}

/**
 * @brief Computes the inverse of the current adjacency matrix
 * @param method
 * @return
 */
bool Graph::createMatrixAdjacencyInverse(const QString &method)
{
    qDebug() << "Graph::createMatrixAdjacencyInverse() ";

    bool considerWeights = false;
    int i = 0, j = 0;
    bool isSingular = true;

    bool dropIsolates = true; // always drop isolates else AM will be singular

    int N = vertices(dropIsolates, false, true);

    createMatrixAdjacency(dropIsolates, considerWeights);
    if (progressCanceled())
    {
        return false;
    }
    invAM.resize(N, N);

    if (method == "gauss")
    {
        invAM.inverseByGaussJordanElimination(AM);
    }
    else
    {
        invAM.inverse(AM);
    }

    VList::const_iterator it, it1;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled() || (*it)->isIsolated())
            continue;
        j = 0;
        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {
            if (!(*it1)->isEnabled() || (*it1)->isIsolated())
                continue;
            if (invAM.item(i, j) != 0)
                isSingular = false;
            j++;
        }
        i++;
    }

    return !isSingular;
}
