/**
 * @file graph_triad_census.cpp
 * @brief Implements triad census analysis for the Graph class, including MAN classification and triad type labeling.
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
 * @brief Conducts a triad census and updates QList::triadTypeFreqs,
 * 		which is the list carrying all triad type frequencies
 *  Complexity: O(n³) — three nested loops each bounded by N.
 *
 * Parallelization (WS15 P4): the outer vertex (v1) loop is mapped via
 * QtConcurrent::blockingMap over vertex positions, each worker thread running its own
 * v2/v3 loops exactly as before - every (v1,v2,v3) triad classification only reads edges
 * (GraphVertex::hasEdgeTo(), read-only) and is independent of every other triad, so there's
 * no cross-iteration dependency to preserve. The one shared state is the result itself:
 * triadTypeFreqs[16] used to be incremented directly by triadType_examine_MAN_label(),
 * which is a data race (non-atomic read-modify-write) once multiple worker threads reach it
 * concurrently. Fixed by having it increment a QAtomicInteger<int> counter per triad type
 * instead; a sequential pass after blockingMap returns copies each counter's final value
 * into triadTypeFreqs. Cancel signals cannot be delivered while graphThread's event loop is
 * blocked in blockingMap (same tradeoff as centralityDegree()/isSymmetric()/
 * clusteringCoefficient()), so the progressCanceled() check only runs once, before the
 * parallel step starts, not per-iteration as before.
 *
 * @return
 */
bool Graph::graphTriadCensus()
{
    qCDebug(lcClustering) << "Graph::graphTriadCensus()";
    /*
     * QList::triadTypeFreqs stores triad type frequencies with the following order:
     * 0	1	2	3		4	5	6	7	8		9	10	11	12		13	14	15
     * 003 012 102	021D 021U 021C 111D	111U 030T 030C 201 	120D 120U 120C 210 300
     */
    triadTypeFreqs.clear();
    for (int i = 0; i <= 15; ++i)
    {
        triadTypeFreqs.append(0);
        qCDebug(lcClustering) << " initializing triadTypeFreqs[" << i << "] = " << triadTypeFreqs[i];
    }

    QString pMsg = tr("Computing Triad Census. \nPlease wait...");
    progressStatus(pMsg);

    if (progressCanceled())
    {
        calculatedTriad = false;
        return false;
    }

    const int N = m_graph.size();

    // One QAtomicInteger<int> counter per triad type (0-15) - safe for multiple worker
    // threads (below) to increment concurrently, unlike a plain QList<int>.
    QVector<QAtomicInteger<int>> triadTypeCounts(16);

    QList<int> positions;
    positions.reserve(N);
    for (int i = 0; i < N; ++i)
        positions.append(i);

    QtConcurrent::blockingMap(positions, [&](int i) {
        GraphVertex *v1vert = m_graph.at(i);
        const int ver1 = v1vert->number();

        for (int j = i + 1; j < N; ++j)
        {
            GraphVertex *v2vert = m_graph.at(j);
            const int ver2 = v2vert->number();

            int temp_mut = 0, temp_asy = 0, temp_nul = 0;

            if (v1vert->hasEdgeTo(ver2))
            {
                if (v2vert->hasEdgeTo(ver1))
                    temp_mut++;
                else
                    temp_asy++;
            }
            else if (v2vert->hasEdgeTo(ver1))
                temp_asy++;
            else
                temp_nul++;

            for (int k = j + 1; k < N; ++k)
            {
                GraphVertex *v3vert = m_graph.at(k);
                int mut = temp_mut, asy = temp_asy, nul = temp_nul;

                const int ver3 = v3vert->number();

                if (v1vert->hasEdgeTo(ver3))
                {
                    if (v3vert->hasEdgeTo(ver1))
                        mut++;
                    else
                        asy++;
                }
                else if (v3vert->hasEdgeTo(ver1))
                    asy++;
                else
                    nul++;

                if (v2vert->hasEdgeTo(ver3))
                {
                    if (v3vert->hasEdgeTo(ver2))
                        mut++;
                    else
                        asy++;
                }
                else if (v3vert->hasEdgeTo(ver2))
                    asy++;
                else
                    nul++;

                qCDebug(lcClustering) << "triad of (" << ver1 << "," << ver2 << "," << ver3
                         << ") = (" << mut << "," << asy << "," << nul << ")";
                triadType_examine_MAN_label(mut, asy, nul, v1vert, v2vert, v3vert, triadTypeCounts);
            } // end 3rd loop
        } // end 2nd loop
    });

    // Every worker thread is done now (blockingMap only returns once all of them have
    // finished) - safe to read the final counts and copy them into triadTypeFreqs.
    for (int i = 0; i <= 15; ++i)
        triadTypeFreqs[i] = triadTypeCounts[i].loadAcquire();

    calculatedTriad = true;

    return true;
}

/**
    Examines the triad type (in Mutual-Asymmetric-Null label format)
    and increases by one the proper frequency element inside triadTypeCounts.
 *
 * Thread-safety (WS15 P4): increments triadTypeCounts (QAtomicInteger<int>, one per triad
 * type) instead of writing Graph::triadTypeFreqs directly, since this is called from
 * multiple worker threads concurrently by graphTriadCensus() - see its own doc comment.
*/
void Graph::triadType_examine_MAN_label(int mut, int asy, int nul,
                                        GraphVertex *vert1,
                                        GraphVertex *vert2,
                                        GraphVertex *vert3,
                                        QVector<QAtomicInteger<int>> &triadTypeCounts)
{
    VList m_triad;
    bool isDown = false, isUp = false, isCycle = false, isTrans = false;
    bool isOutLinked = false, isInLinked = false;

    qCDebug(lcClustering) << "Graph::triadType_examine_MAN_label() "
             << " adding (" << vert1->number() << "," << vert2->number()
             << "," << vert3->number() << ") to m_triad ";

    m_triad << vert1 << vert2 << vert3;

    switch (mut)
    {
    case 0:
        switch (asy)
        {
        case 0: //"003";
            triadTypeCounts[0].fetchAndAddOrdered(1);
            break;
        case 1: //"012";
            triadTypeCounts[1].fetchAndAddOrdered(1);
            break;
        case 2:
            // "021?" - find out!
            foreach (GraphVertex *source, m_triad)
            {
                isOutLinked = false;
                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {
                        if (isOutLinked)
                        {
                            triadTypeCounts[3].fetchAndAddOrdered(1); //"021D"
                            break;
                        }
                        else if (isInLinked)
                        {
                            triadTypeCounts[5].fetchAndAddOrdered(1); //"021C"
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                    else if (target->hasEdgeTo(source->number()))
                    {
                        if (isInLinked)
                        {
                            triadTypeCounts[4].fetchAndAddOrdered(1); //"021U"
                            break;
                        }
                        else if (isOutLinked)
                        {
                            triadTypeCounts[5].fetchAndAddOrdered(1); //"021C"
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
            }
            break;
        case 3:
            qCDebug(lcClustering) << "triad vertices: ( " << vert1->number() << ", " << vert2->number() << ", " << vert3->number() << " ) = (" << mut << "," << asy << "," << nul << ")";
            isTrans = false;
            foreach (GraphVertex *source, m_triad)
            {
                qCDebug(lcClustering) << "  vertex " << source->number();

                isOutLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {

                        if (isOutLinked)
                        {
                            triadTypeCounts[8].fetchAndAddOrdered(1); //"030T"
                            isTrans = true;
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                }
            }
            if (!isTrans)
            { //"030C"
                triadTypeCounts[9].fetchAndAddOrdered(1);
            }
            break;
        }
        break;

    case 1:
        switch (asy)
        {
        case 0: //"102";
            triadTypeCounts[2].fetchAndAddOrdered(1);
            break;
        case 1:
            isUp = false;
            foreach (GraphVertex *source, m_triad)
            {

                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (target->hasEdgeTo(source->number()))
                    {

                        if (isInLinked)
                        {
                            triadTypeCounts[6].fetchAndAddOrdered(1); //"030T"
                            isUp = true;
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
            }
            if (!isUp)
            { //"111U"
                triadTypeCounts[7].fetchAndAddOrdered(1);
            }
            break;
        case 2:
            isDown = false;
            isUp = false;
            isCycle = true;
            qCDebug(lcClustering) << "triad vertices: ( " << vert1->number() << ", "
                     << vert2->number() << ", " << vert3->number() << " ) = ("
                     << mut << "," << asy << "," << nul << ")";

            foreach (GraphVertex *source, m_triad)
            {
                isOutLinked = false;
                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {
                        if (target->hasEdgeTo(source->number()))
                        {
                            isInLinked = true;
                            isOutLinked = true;
                            continue;
                        }
                        else if (isOutLinked && !isInLinked)
                        {
                            triadTypeCounts[11].fetchAndAddOrdered(1); //"120D"
                            isDown = true;
                            isCycle = false;
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                    else if (target->hasEdgeTo(source->number()))
                    {
                        if (source->hasEdgeTo(target->number()))
                        {
                            isOutLinked = true;
                            isInLinked = true;
                            continue;
                        }
                        else if (isInLinked && !isOutLinked)
                        {
                            triadTypeCounts[12].fetchAndAddOrdered(1); //"120U"
                            isUp = true;
                            isCycle = false;
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
                if (isUp || isDown)
                    break;
            }
            if (isCycle)
            { //"120C"
                triadTypeCounts[13].fetchAndAddOrdered(1);
            }
            break;
        case 3:
            // nothing here!
            break;
        }

        break;
    case 2:
        switch (asy)
        {
        case 0: // "201"
            triadTypeCounts[10].fetchAndAddOrdered(1);
            break;
        case 1: // "210"
            triadTypeCounts[14].fetchAndAddOrdered(1);
            break;
        }
        break;
    case 3: // "300"
        if (asy == 0 && nul == 0)
            triadTypeCounts[15].fetchAndAddOrdered(1);
        break;
    }
}
