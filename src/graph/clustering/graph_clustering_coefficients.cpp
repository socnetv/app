/**
 * @file graph_clustering_coefficients.cpp
 * @brief Implements clustering coefficient calculations for the Graph class.
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
 * @brief  Returns the local clustering coefficient (CLUCOF) of a vertex v1
 * @param v1
 * @return
 */
qreal Graph::clusteringCoefficientLocal(const int &v1)
{
    if (!isModified() && (m_graph[vpos[v1]]->hasCLC()))
    {
        qreal clucof = m_graph[vpos[v1]]->CLC();
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") - "
                 << " Not modified. Returning previous clucof = " << clucof;
        return clucof;
    }

    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") - "
             << " Graph changed or clucof not calculated.";

    bool isSymmetric = false;

    if (this->isSymmetric())
    {
        isSymmetric = true;
    }
    else
    {
        isSymmetric = false;
    }

    qreal clucof = 0, denom = 0, nom = 0;
    int u1 = 0, u2 = 0, k = 0;

    H_StrToBool neighborhoodEdges;
    neighborhoodEdges.clear();

    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") - vertex " << v1
             << "[" << vpos[v1] << "] "
             << " Checking adjacent edges ";

    QHash<int, qreal> reciprocalEdges;
    reciprocalEdges = m_graph[vpos[v1]]->reciprocalEdgesHash();

    QHash<int, qreal>::const_iterator it1;
    QHash<int, qreal>::const_iterator it2;

    it1 = reciprocalEdges.cbegin();

    while (it1 != reciprocalEdges.cend())
    {
        u1 = it1.key();

        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << v1
                 << "<->"
                 << u1
                 << "[" << vpos[u1] << "] exists"
                 << "weight " << it1.value();

        if (v1 == u1)
        {
            qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                     << "v1 == u1 - CONTINUE";
            ++it1;
            continue;
        }

        it2 = reciprocalEdges.cbegin();
        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "Checking if neighbor" << u1
                 << "is connected to other neighbors of" << v1;

        while (it2 != reciprocalEdges.cend())
        {

            u2 = it2.key();

            qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                     << "Other neighbor" << u2
                     << "Check if there is an edge"
                     << u1
                     << "[" << vpos[u1] << "]"
                     << "->" << u2;

            if (u1 == u2)
            {
                qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                         << "u1 == u2 - CONTINUE";
                ++it2;
                continue;
            }

            if (edgeExists(u1, u2) != 0)
            {
                qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                         << "Connected neighbors: "
                         << u1 << "->" << u2;

                QString edge = QString::number(u1) + "->" + QString::number(u2);
                QString revedge = QString::number(u2) + "->" + QString::number(u1);

                if (isSymmetric)
                {
                    if (!neighborhoodEdges.contains(edge) &&
                        !neighborhoodEdges.contains(revedge))
                    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Edge added to neighborhoodEdges : " << edge;
                    }
                    else
                    {
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Edge not added, discovered previously : " << edge;
                    }
                }
                else
                {
                    if (!neighborhoodEdges.contains(edge))
                    {
                        neighborhoodEdges.insert(edge, true);
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Edge added to neighborhoodEdges : " << edge;
                    }
                    else
                    {
                        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                                 << "Edge not added, discovered previously : " << edge;
                    }
                }
            }

            ++it2;
        }
        ++it1;
    }

    nom = neighborhoodEdges.size();

    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "neighborhoodEdges.size() =" << nom;

    if (nom == 0)
        return 0; // stop if we're at a leaf.

    if (isSymmetric)
    {
        k = reciprocalEdges.size(); // k_{i} is the number of neighbours of a vertex
        denom = k * (k - 1.0) / 2.0;

        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
                 << "Symmetric graph. "
                 << "Max edges in neighborhood" << denom;
    }
    else
    {
        // fixme : normally we should have a special method
        // to compute the number of vertices k_i = |N_i|, in the neighborhood N_i
        k = reciprocalEdges.size();
        denom = k * (k - 1.0);

        qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") - "
                 << "Not symmetric graph. "
                 << "Max edges in neighborhood" << denom;
    }

    clucof = nom / denom;

    qDebug() << "Graph::clusteringCoefficientLocal(" << v1 << ") -"
             << "CLUCOF = " << clucof;

    m_graph[vpos[v1]]->setCLC(clucof);

    reciprocalEdges.clear();
    neighborhoodEdges.clear();
    return clucof;
}

/**
 * @brief Computes local clustering coefficients and returns
 * the network average Clustering Coefficient
 * @param updateProgress
 * @return
 */
qreal Graph::clusteringCoefficient(const bool updateProgress)
{
    qDebug() << "Graph::clusteringCoefficient()";
    averageCLC = 0;
    varianceCLC = 0;
    maxCLC = 0;
    minCLC = 1;
    qreal temp = 0;
    qreal x = 0;
    qreal N = vertices();
    int progressCounter = 0;
    VList::const_iterator vertex;

    QString pMsg = tr("Computing Clustering Coefficient. \n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex)
    {

        if (updateProgress)
        {
            progressUpdate(++progressCounter);
        }

        temp = clusteringCoefficientLocal((*vertex)->number());

        if (temp > maxCLC)
        {
            maxCLC = temp;
            maxNodeCLC = (*vertex)->number();
        }
        if (temp < minCLC)
        {
            minNodeCLC = (*vertex)->number();
            minCLC = temp;
        }
        averageCLC += temp;
    }

    averageCLC = averageCLC / N;

    qDebug() << "Graph::clusteringCoefficient() network average " << averageCLC;

    for (vertex = m_graph.cbegin(); vertex != m_graph.cend(); ++vertex)
    {
        x = ((*vertex)->CLC() - averageCLC);
        x *= x;
        varianceCLC += x;
    }

    varianceIC /= N;

    if (updateProgress)
    {
        progressFinish();
    }

    return averageCLC;
}
