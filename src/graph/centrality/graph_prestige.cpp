/**
 * @file graph_prestige.cpp
 * @brief Implements prestige (prominence) index algorithms for the Graph class.
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
 * @brief Computes the Degree Prestige (in-degree) of each vertex - diagonal included
 *	Also the mean value and the variance of the in-degrees.
 * @param weights
 * @param dropIsolates
 */
void Graph::prestigeDegree(const bool &considerWeights, const bool &dropIsolates)
{

    if (calculatedDP)
    {
        qDebug() << "Graph not changed - no need to recompute Degree Prestige scores. Returning";
        return;
    }

    qDebug() << "(Re)Computing Degree Prestige scores...";

    int N = vertices(dropIsolates);
    int v2 = 0, v1 = 0;
    int progressCounter = 0;

    VList::const_iterator it;

    QHash<int, qreal> *enabledInEdges = new QHash<int, qreal>;
    QHash<int, qreal>::const_iterator hit;

    qreal DP = 0, SDP = 0, nom = 0, denom = 0;
    qreal weight;

    classesSDP = 0;
    sumSDP = 0;
    sumDP = 0;
    maxSDP = 0;
    minSDP = N - 1;
    discreteDPs.clear();
    varianceSDP = 0;
    meanSDP = 0;
    m_graphIsSymmetric = true;

    QString pMsg = tr("Computing Degree Prestige (in-Degree). \n Please wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    qDebug() << "vertices"
             << N
             << "graph modified. Recomputing...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            delete enabledInEdges;
            progressFinish();
            return;
        }
        v1 = (*it)->number();
        qDebug() << "computing DP for vertex" << v1;

        DP = 0;

        if (!(*it)->isEnabled())
        {
            qDebug() << "vertex disabled. Continue.";
            continue;
        }

        qDebug() << "Iterate over inbound edges of "
                 << v1;

        enabledInEdges = (*it)->inEdgesEnabledHash();

        hit = enabledInEdges->cbegin();

        while (hit != enabledInEdges->cend())
        {

            v2 = hit.key();

            qDebug() << "inbound edge from" << v2;

            if (!edgeExists(v2, v1))
            {
                // sanity check
                qDebug() << "Cannot verify inbound edge"
                         << v2 << "CONTINUE";
                ++hit;
                continue;
            }

            weight = hit.value();

            if (considerWeights)
            {
                DP += weight;
            }
            else
            {
                DP++;
            }
            if (edgeExists(v1, v2) != weight)
            {
                m_graphIsSymmetric = false;
            }
            ++hit;
        }

        (*it)->setDP(DP); // Set DP
        sumDP += DP;

        qDebug() << "vertex " << (*it)->number()
                 << " DP " << DP;
    }

    // Calculate std DP, min,max, mean
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        DP = (*it)->DP();

        if (!considerWeights)
        {
            SDP = (DP / (N - 1.0)); // Set Standard InDegree
        }
        else
        {
            SDP = (DP / (sumDP));
        }
        (*it)->setSDP(SDP);
        sumSDP += SDP;

        qDebug() << "vertex " << (*it)->number() << " DP  "
                 << DP << " SDP " << (*it)->SDP();

        resolveClasses(SDP, discreteDPs, classesSDP);

        qDebug("DP classes = %i ", classesSDP);

        if (maxSDP < SDP)
        {
            maxSDP = SDP;
            maxNodeDP = (*it)->number();
        }
        if (minSDP > SDP)
        {
            minSDP = SDP;
            minNodeDP = (*it)->number();
        }
    }

    if (minSDP == maxSDP)
        maxNodeDP = -1;

    meanSDP = sumSDP / (qreal)N;

    qDebug("Graph: sumSDP = %f, meanSDP = %f", sumSDP, meanSDP);

    // Calculate Variance and the Degree Prestigation of the whole graph. :)
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (dropIsolates && (*it)->isIsolated())
        {
            continue;
        }
        SDP = (*it)->SDP();
        nom += maxSDP - SDP;
        varianceSDP += (SDP - meanSDP) * (SDP - meanSDP);
    }
    varianceSDP = varianceSDP / (qreal)N;

    if (m_graphIsSymmetric)
        denom = (N - 1.0) * (N - 2.0);
    else
        denom = (N - 1.0) * (N - 1.0);
    if (N < 3)
        denom = N - 1.0;

    if (!considerWeights)
    {
        groupDP = nom / denom;
        qDebug("Graph: varianceSDP = %f, groupDP = %f", varianceSDP, groupDP);
    }

    delete enabledInEdges;
    calculatedDP = true;

    progressFinish();
}

/**
 * @brief Computes Proximity Prestige of each vertex
 * Also the mean value and the variance of it..
 */
void Graph::prestigeProximity(const bool considerWeights,
                              const bool inverseWeights,
                              const bool dropIsolates)
{
    if (calculatedPP)
    {
        qDebug() << "Graph not changed - no need to recompute proximity prestige. Returning";
        return;
    }

    qDebug() << "(Re)Computing Proximity prestige scores...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, inverseWeights);
    if (progressCanceled())
    {
        return;
    }
    // calculate centralities
    VList::const_iterator it, jt;
    qreal PP = 0;
    qreal dist = 0;
    qreal Ii = 0;
    qreal V = vertices(dropIsolates);
    classesPP = 0;
    discretePPs.clear();
    sumPP = 0;
    maxPP = 0;
    minPP = V - 1;
    variancePP = 0;
    meanPP = 0;

    int progressCounter = 0;

    QString pMsg = tr("Computing Proximity Prestige scores. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(V, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            return;
        }
        PP = 0;
        Ii = 0;

        if ((*it)->isIsolated())
        {
            continue;
        }

        for (jt = m_graph.cbegin(); jt != m_graph.cend(); ++jt)
        {

            if ((*it)->number() == (*jt)->number())
            {
                continue;
            }
            if (!(*jt)->isEnabled())
            {
                continue;
            }

            dist = (*jt)->distance((*it)->number());

            if (dist != RAND_MAX)
            {
                PP += dist;
                Ii++; // compute |Ii|
            }
        }

        qDebug() << "vertex"
                 << (*it)->number()
                 << "actors in influence domain Ii" << Ii
                 << "actors in network" << (V - 1)
                 << "fraction of actors who reach i |Ii|/(V-1)=" << Ii / (V - 1)
                 << "distance to actors in Ii" << PP
                 << "average distance to actors in Ii" << PP / Ii
                 << "PP= "
                 << Ii / (V - 1) << " / " << PP / Ii << " = " << (Ii / (V - 1)) / (PP / Ii);

        // sanity check for PP=0 (=> node is disconnected)
        if (PP != 0)
        {
            PP /= Ii;
            PP = (Ii / (V - 1)) / PP;
        }
        sumPP += PP;

        (*it)->setPP(PP);
        (*it)->setSPP(PP); // PP is already stdized

        resolveClasses(PP, discretePPs, classesPP);

        // qDebug("PP classes = %i ", classesPP);
        if (maxPP < PP)
        {
            maxPP = PP;
            maxNodePP = (*it)->number();
        }
        if (minPP > PP)
        {
            minPP = PP;
            minNodePP = (*it)->number();
        }
    }

    if (minPP == maxPP)
        maxNodePP = -1;

    meanPP = sumPP / V;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (dropIsolates && (*it)->isIsolated())
        {
            continue;
        }
        PP = (*it)->PP();
        variancePP += (PP - meanPP) * (PP - meanPP);
    }

    variancePP = variancePP / V;

    qDebug() << "sumPP = " << sumPP
             << " meanPP = " << meanPP
             << " variancePP " << variancePP;

    calculatedPP = true;

    progressFinish();
}

/**
 * @brief Calculates the PageRank Prestige of each vertex
 * @param dropIsolates
 */
void Graph::prestigePageRank(const bool &dropIsolates)
{

    if (calculatedPRP)
    {
        qDebug() << "Graph not changed - no need to recompute Pagerank scores. Return ";
        return;
    }

    qDebug() << "(Re)Computing PageRank prestige scores...";

    discretePRPs.clear();
    sumPRP = 0;
    t_sumPRP = 0;
    maxPRP = 0;
    minPRP = RAND_MAX;
    classesPRP = 0;
    variancePRP = 0;
    // The parameter d is a damping factor which can be set between 0 and 1.
    // Google creators set d to 0.85.
    d_factor = 0.85;

    qreal PRP = 0, oldPRP = 0;
    qreal SPRP = 0;
    int iterations = 1; // a counter
    int referrer;
    qreal delta = 0.00001; // The delta where we will stop the iterative calculation
    qreal maxDelta = RAND_MAX;
    qreal sumInLinksPR = 0; // temporary var for inlinks sum PR
    qreal transferedPRP = 0;
    qreal inLinks = 0;  // temporary var
    qreal outLinks = 0; // temporary var
    qreal t_variance = 0;
    int N = vertices(dropIsolates);

    VList::const_iterator it;
    H_edges::const_iterator jt;

    int relation = 0;
    bool edgeStatus = false;

    QString pMsg = tr("Computing PageRank Prestige scores. \nPlease wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        // At first, PR scores have probability distribution
        // from 0 to 1, so each one is set to 1/N
        (*it)->setPRP(1.0 / (qreal)N);

        // compute inEdgesCount() to warm up inEdgesConst for everyone
        inLinks = (*it)->inEdgesCount();
        outLinks = (*it)->outEdgesCount();
        qDebug() << "node "
                 << (*it)->number() << " PR = " << (*it)->PRP()
                 << " inLinks (set const): " << inLinks
                 << " outLinks (set const): " << outLinks;
    }

    if (edgesEnabled() == 0)
    {
        qDebug() << "all vertices are isolated and of equal PR. Stop";
        return;
    }

    progressUpdate(N / 3);
    if (progressCanceled())
    {
        progressFinish();
        return;
    }
    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta)
    {

        qDebug() << "ITERATION : " << iterations;

        sumPRP = 0;
        maxDelta = 0;
        maxPRP = 0;
        minPRP = RAND_MAX;
        maxNodePRP = 0;
        minNodePRP = 0;

        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            sumInLinksPR = 0;
            oldPRP = (*it)->PRP();

            qDebug() << "computing PR for node: "
                     << (*it)->number() << " current PR " << oldPRP;

            if ((*it)->isIsolated())
            {
                // isolates have constant PR = 1/N
                qDebug() << "isolated - CONTINUE ";
                continue;
            }

            jt = (*it)->m_inEdges.cbegin();

            qDebug() << "Iterate over inEdges of "
                     << (*it)->number();

            while (jt != (*it)->m_inEdges.cend())
            {
                relation = jt.value().first;
                if (relation != relationCurrent())
                {
                    ++jt;
                    continue;
                }
                edgeStatus = jt.value().second.second;
                if (edgeStatus != true)
                {
                    ++jt;
                    continue;
                }

                referrer = jt.key();

                qDebug() << "Node " << (*it)->number()
                         << " inLinked from neighbor " << referrer << " vpos "
                         << vpos[referrer];

                if (edgeExists(referrer, (*it)->number()))
                {
                    inLinks = m_graph[vpos[referrer]]->inEdgesCountConst();
                    outLinks = m_graph[vpos[referrer]]->outEdgesCountConst();

                    PRP = m_graph[vpos[referrer]]->PRP();

                    transferedPRP = (outLinks != 0) ? (PRP / outLinks) : PRP;

                    qDebug() << "neighbor " << referrer
                             << " has PR = " << PRP
                             << " and outLinks = " << outLinks
                             << "  will transfer " << transferedPRP;

                    sumInLinksPR += transferedPRP;
                }
                ++jt;
            }

            PRP = (1 - d_factor) / (qreal)N + d_factor * sumInLinksPR;

            (*it)->setPRP(PRP);

            sumPRP += PRP;

            qDebug() << "Node "
                     << (*it)->number()
                     << " new PR = " << PRP
                     << " old PR was = " << oldPRP
                     << " diff = " << fabs(PRP - oldPRP);

            // calculate diff from last PageRank value for this vertex
            // and set it to minDelta if the latter is bigger.

            if (maxDelta < fabs(PRP - oldPRP))
            {
                maxDelta = fabs(PRP - oldPRP);
                qDebug() << "Setting new maxDelta = "
                         << maxDelta;
            }
        }

        // normalize in every iteration

        qDebug() << "sumPRP for this iteration " << sumPRP;

        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            PRP = (*it)->PRP();

            if (PRP > maxPRP)
            {
                maxPRP = PRP;
                maxNodePRP = (*it)->number();
            }
            if (PRP < minPRP)
            {
                minPRP = PRP;
                minNodePRP = (*it)->number();
            }
        }
        iterations++;
    }

    progressUpdate(2 * N / 3);
    if (progressCanceled())
    {
        progressFinish();
        return;
    }
    if (N != 0)
    {
        meanPRP = sumPRP / (qreal)N;
    }
    else
    {
        meanPRP = SPRP;
    }

    qDebug() << "sumPRP = " << sumPRP << "  N = " << N
             << "  meanPRP = " << meanPRP;

    // calculate std and min/max PRPs
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (dropIsolates && (*it)->isIsolated())
        {
            continue;
        }

        PRP = (*it)->PRP();

        resolveClasses(PRP, discretePRPs, classesPRP);

        SPRP = PRP / maxPRP;
        (*it)->setSPRP(SPRP);

        qDebug() << "vertex: " << (*it)->number()
                 << " PR = " << PRP << " standard PR = " << SPRP
                 << " t_sumPRP " << t_sumPRP;

        t_variance = (PRP - meanPRP);
        t_variance *= t_variance;
        qDebug() << "PRP " << (*it)->PRP() << "  t_variance "
                 << PRP - meanPRP << " t_variance^2" << t_variance;
        variancePRP += t_variance;
    }

    qDebug() << "PRP' Variance   " << variancePRP << " N " << N;
    variancePRP = variancePRP / (qreal)N;
    qDebug() << "PRP' Variance: " << variancePRP;

    calculatedPRP = true;

    progressUpdate(N);
    progressFinish();

    return;
}
