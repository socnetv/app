/**
 * @file graph_clustering_hierarchical.cpp
 * @brief Implements hierarchical clustering methods for the Graph class.
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
 * @brief Performs an hierarchical clustering process (Johnson, 1967) on a given
 * NxN distance/dissimilarity matrix. The input matrix can be the
 * the adjacency matrix, the geodesic distance matrix or a derived from them
 * dissimilarities matrix using a user-specified metric, i.e. euclidean distance.
 * The method parameter defines how to compute distances (similarities) between
 * a new cluster the old clusters. Valid values can be:
 * - Clustering::Single_Linkage: "single-link" or "connectedness" or "minimum"
 * - Clustering::Complete_Linkage: "complete-link" or "diameter" or "maximum"
 * - Clustering::Average_Linkage: "average-link" or UPGMA
 * @param matrix
 * @param metric
 * @param method
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
bool Graph::graphClusteringHierarchical(Matrix &STR_EQUIV,
                                        const QString &varLocation,
                                        const int &metric,
                                        const int &method,
                                        const bool &diagonal,
                                        const bool &diagram,
                                        const bool &considerWeights,
                                        const bool &inverseWeights,
                                        const bool &dropIsolates)
{

    Q_UNUSED(inverseWeights);

    qDebug() << "Graph::graphClusteringHierarchical() - "
             << "metric" << metric
             << "method" << graphClusteringMethodTypeToString(method)
             << "diagonal" << diagonal
             << "diagram" << diagram
             << "dropIsolates" << dropIsolates;

    qDebug() << "Graph::graphClusteringHierarchical() - STR_EQUIV matrix:";
    // STR_EQUIV.printMatrixConsole(true);

    qreal min = RAND_MAX;
    qreal max = 0;
    int imin, jmin, imax, jmax, mergedClusterIndex, deletedClusterIndex;
    qreal distanceNewCluster;

    // temporarily stores cluster members at each clustering level
    QList<int> clusteredItems;

    // maps original and clustered items per their DSM matrix index
    // so that we know that at Level X the matrix index 0 corresponds to the cluster i.e. { 1,2,4}
    QMap<int, V_int> m_clustersIndex;
    QMap<int, V_int>::iterator it;
    QMap<int, V_int>::iterator prev;

    QMap<QString, V_int>::const_iterator sit;

    // variables for diagram computation
    QList<QString> clusterPairNames;
    QString cluster1, cluster2;

    Matrix DSM; // dissimilarities matrix. Note: will be destroyed in the end.

    // TODO: needs fix when distances matrix with -1 (infinity) elements is used.

    // compute, if needed, the dissimilarities matrix
    switch (metric)
    {
    case METRIC_NONE:
        DSM = STR_EQUIV;
        break;
    case METRIC_JACCARD_INDEX:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric, varLocation, diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_MANHATTAN_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric, varLocation, diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_HAMMING_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric, varLocation, diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_EUCLIDEAN_DISTANCE:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric, varLocation, diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    case METRIC_CHEBYSHEV_MAXIMUM:
        createMatrixDissimilarities(STR_EQUIV, DSM, metric, varLocation, diagonal, considerWeights);
        STR_EQUIV = DSM;
        break;
    default:
        break;
    }

    int N = DSM.rows();

    qDebug() << "Graph::graphClusteringHierarchical() -"
             << "initial matrix DSM contents:";
    // DSM.printMatrixConsole();

    if (DSM.illDefined())
    {
        //        DSM.clear();
        //        STR_EQUIV.clear();
        progressStatus("ERROR computing dissimilarities matrix");
        return false;
    }

    clusteredItems.reserve(N);
    if (diagram)
    {
        clusterPairNames.reserve(N);
    }

    m_clustersIndex.clear();

    m_clustersPerSequence.clear();
    m_clusteringLevel.clear();

    m_clustersByName.clear();
    m_clusterPairNamesPerSeq.clear();

    //
    // Step 1: Assign each of the N items to its own cluster.
    //        We have N unit clusters
    //
    int clustersLeft = N;
    int seq = 1; // clustering stage/level sequence number

    VList::const_iterator vit;
    int i = 0;
    for (vit = m_graph.cbegin(); vit != m_graph.cend(); ++vit)
    {
        //        if (dropIsolates) {
        //            if ((*vit)->isIsolated()) {
        //                continue;
        //            }
        //        }
        //         if (!(*vit)->isEnabled()) {
        //            continue;
        //         }

        if ((*vit)->isEnabled() && (!(*vit)->isIsolated()))
        {
            clusteredItems.clear();
            clusteredItems << (*vit)->number();
            m_clustersIndex[i] = clusteredItems;
            if (diagram)
            {
                m_clustersByName.insert(QString::number(i + 1), clusteredItems);
            }
            i++;
        }
    }

    QString pMsg = tr("Computing Hierarchical Clustering. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    while (clustersLeft > 1)
    {

        progressUpdate(seq);

        qDebug() << "matrix DSM contents now:";
        // DSM.printMatrixConsole();

        //
        // Step 2. Find the most similar pair of clusters.
        //        Merge them into a single new cluster.
        //
        DSM.NeighboursNearestFarthest(min, max, imin, jmin, imax, jmax);
        mergedClusterIndex = (imin < jmin) ? imin : jmin;
        deletedClusterIndex = (mergedClusterIndex == imin) ? jmin : imin;

        m_clusteringLevel << min;

        clusteredItems.clear();
        clusteredItems = m_clustersIndex[mergedClusterIndex] + m_clustersIndex[deletedClusterIndex];

        qDebug() << "Graph::graphClusteringHierarchical() -"
                 << "level" << min
                 << "seq" << seq
                 << "clusteredItems in level" << clusteredItems;

        m_clustersPerSequence.insert(seq, clusteredItems);

        if (diagram)
        {

            cluster1.clear();
            cluster2.clear();
            clusterPairNames.clear();

            for (sit = m_clustersByName.constBegin(); sit != m_clustersByName.constEnd(); ++sit)
            {
                if (sit.value() == m_clustersIndex[mergedClusterIndex])
                {
                    cluster1 = sit.key();
                }
                else if (sit.value() == m_clustersIndex[deletedClusterIndex])
                {
                    cluster2 = sit.key();
                }
            }
            if (cluster1.isNull() && m_clustersIndex[mergedClusterIndex].size() == 1)
            {
                cluster1 = QString::number(m_clustersIndex[mergedClusterIndex].first());
            }
            if (cluster2.isNull() && m_clustersIndex[deletedClusterIndex].size() == 1)
            {
                cluster1 = QString::number(m_clustersIndex[deletedClusterIndex].first());
            }

            clusterPairNames.append(cluster1);
            clusterPairNames.append(cluster2);

            m_clusterPairNamesPerSeq.insert(seq, clusterPairNames);

            m_clustersByName.insert("c" + QString::number(seq), clusteredItems);

            qDebug() << "Computing diagram variables..." << "\n"
                     << "cluster1" << cluster1 << "\n"
                     << "cluster2" << cluster2 << "\n"
                     << "clusterPairNames" << clusterPairNames << "\n"
                     << "m_clusterPairNamesPerSeq" << m_clusterPairNamesPerSeq << "\n"
                     << "m_clustersByName" << m_clustersByName;

        } // end if diagram

        // map new cluster to a matrix index
        m_clustersIndex[mergedClusterIndex] = clusteredItems;

        qDebug() << "  Clustering seq:"
                 << seq << "\n"
                 << "  Level:" << min << "\n"
                 << "  Neareast neighbors: (" << imin + 1 << "," << jmin + 1 << ")"
                 << "Minimum/distance:" << min << "\n"
                 << "  Farthest neighbors: (" << imax + 1 << "," << jmax + 1 << ")"
                 << "Maximum/distance:" << max << "\n"
                 << "  Merge nearest neighbors into a single new cluster:"
                 << mergedClusterIndex + 1 << "\n"
                 << "  m_clustersPerSequence" << m_clustersPerSequence;

        qDebug() << "  Remove key" << deletedClusterIndex
                 << "and shift next values to left... ";

        it = m_clustersIndex.find(deletedClusterIndex);

        while (it != m_clustersIndex.end())
        {
            prev = it;
            ++it;
            if (it != m_clustersIndex.end())
            {
                prev.value() = it.value();
                // qDebug() << "  key now"<< prev.key() << ": " << prev.value() ;
            }
        }
        m_clustersIndex.erase(--it); // erase the last element in map

        qDebug() << "Finished. " << "\n"
                 << "  m_clustersIndex now" << m_clustersIndex << "\n"
                 << "  Compute distances "
                    "between the new cluster and the old ones";

        //
        // Step 3. Compute distances (or similarities) between
        //        the single new cluster and the old clusters
        //
        int j = mergedClusterIndex;

        qDebug() << "j = mergedClusterIndex " << mergedClusterIndex + 1;

        for (int i = 0; i < clustersLeft; i++)
        {
            if (i == deletedClusterIndex)
            {
                //                qDebug() << "Graph::graphClusteringHierarchical() -"
                //                          <<"SKIP this as it is one of the merged clusters.";
                continue;
            }

            switch (method)
            {
            case Clustering::Single_Linkage: //"single-linkage":
                if (i == j)
                {
                    distanceNewCluster = 0;
                }
                else
                {
                    distanceNewCluster = (DSM.item(i, imin) < DSM.item(i, jmin)) ? DSM.item(i, imin) : DSM.item(i, jmin);
                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                         << "  DSM(" << i + 1 << "," << imin + 1 << ") =" << DSM.item(i, imin)
                         << "  DSM(" << i + 1 << "," << jmin + 1 << ") =" << DSM.item(i, jmin)
                         << " ? minimum DSM(" << i + 1 << "," << j + 1 << " =" << distanceNewCluster;
                break;

            case Clustering::Complete_Linkage: // "complete-linkage":
                if (i == j)
                {
                    distanceNewCluster = 0;
                }
                else
                {
                    distanceNewCluster = (DSM.item(i, imin) > DSM.item(i, jmin)) ? DSM.item(i, imin) : DSM.item(i, jmin);
                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                         << "  DSM(" << i + 1 << "," << imin + 1 << ") =" << DSM.item(i, imin)
                         << "  DSM(" << i + 1 << "," << jmin + 1 << ") =" << DSM.item(i, jmin)
                         << " ? maximum DSM(" << i + 1 << "," << j + 1 << " =" << distanceNewCluster;
                break;

            case Clustering::Average_Linkage: // mean or "average-linkage" or UPGMA
                if (i == j)
                {
                    distanceNewCluster = 0;
                }
                else
                {
                    distanceNewCluster = (DSM.item(i, imin) + DSM.item(i, jmin)) / 2;
                }
                qDebug() << "Graph::graphClusteringHierarchical() - "
                         << "  DSM(" << i + 1 << "," << imin + 1 << ") =" << DSM.item(i, imin)
                         << "  DSM(" << i + 1 << "," << jmin + 1 << ") =" << DSM.item(i, jmin)
                         << " ? average DSM(" << i + 1 << "," << j + 1 << " =" << distanceNewCluster;
                break;

            default:
                distanceNewCluster = (DSM.item(i, imin) < DSM.item(i, jmin)) ? DSM.item(i, imin) : DSM.item(i, jmin);
                break;
            }

            DSM.setItem(i, j, distanceNewCluster);
            DSM.setItem(j, i, distanceNewCluster);

            // DSM.setItem(deletedClusterIndex, j, RAND_MAX);
            // DSM.setItem(j, deletedClusterIndex, RAND_MAX);
        }

        qDebug() << "Graph::graphClusteringHierarchical() - Finished."
                 << "Resizing old DSM matrix";
        // DSM.printMatrixConsole();
        DSM.deleteRowColumn(deletedClusterIndex);

        clustersLeft--;
        seq++;

        //
        // Step 4. Repeat steps 2 and 3 until all remaining items/clusters
        //        are clustered into a single cluster of size N
        //

    } // end while clustersLeft

    clusteredItems.clear();
    m_clustersIndex.clear();

    qDebug() << "m_clustersByName" << m_clustersByName;

    progressFinish();

    return true;
}
