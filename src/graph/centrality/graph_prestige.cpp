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
 *
 * Meaning: prestige measures flip centrality's question around - instead of "how many ties
 * does this actor have," they ask "how many ties point *at* this actor." Degree prestige is
 * the simplest version: how many others chose to connect to this actor, ignoring how many
 * connections the actor made outward. Meaningless on an undirected graph, where in- and
 * out-ties are the same thing.
 *
 * When to use: directed networks where "being chosen by others" is the thing worth measuring
 * - citation counts, follower counts, nomination/endorsement data - and a quick raw count is
 * enough (see PageRank Prestige below when the *quality* of who's choosing you also matters).
 *
 * Compare to: Degree Centrality (DC, see centralityDegree()) is this same raw-count idea for
 * outbound ties (or all ties, on an undirected graph).
 *
 * Weights: no inversion choice here (considerWeights only) - when considered, weights are
 * summed directly, so a stronger inbound tie always adds more.
 *
 * Math: DP(i) = number of inbound edges to i (or their summed weights, if weights are
 * considered). Standardized SDP(i) = DP(i) / (N-1).
 *
 * @param weights
 * @param dropIsolates
 */
void Graph::prestigeDegree(const bool &considerWeights, const bool &dropIsolates)
{

    if (calculatedDP)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute Degree Prestige scores. Returning";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing Degree Prestige scores...";

    int N = vertices(dropIsolates);
    int v2 = 0, v1 = 0;

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

    qCDebug(lcCentrality) << "vertices"
             << N
             << "graph modified. Recomputing...";

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (progressCanceled())
        {
            delete enabledInEdges;
            return;
        }
        v1 = (*it)->number();
        qCDebug(lcCentrality) << "computing DP for vertex" << v1;

        DP = 0;

        if (!(*it)->isEnabled())
        {
            qCDebug(lcCentrality) << "vertex disabled. Continue.";
            continue;
        }

        qCDebug(lcCentrality) << "Iterate over inbound edges of "
                 << v1;

        enabledInEdges = (*it)->inEdgesEnabledHash();

        hit = enabledInEdges->cbegin();

        while (hit != enabledInEdges->cend())
        {

            v2 = hit.key();

            qCDebug(lcCentrality) << "inbound edge from" << v2;

            if (!edgeExists(v2, v1))
            {
                // sanity check
                qCDebug(lcCentrality) << "Cannot verify inbound edge"
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

        qCDebug(lcCentrality) << "vertex " << (*it)->number()
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

        qCDebug(lcCentrality) << "vertex " << (*it)->number() << " DP  "
                 << DP << " SDP " << (*it)->SDP();

        resolveClasses(SDP, discreteDPs, classesSDP);

        qCDebug(lcCentrality, "DP classes = %i ", classesSDP);

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

    qCDebug(lcCentrality, "Graph: sumSDP = %f, meanSDP = %f", sumSDP, meanSDP);

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
        qCDebug(lcCentrality, "Graph: varianceSDP = %f, groupDP = %f", varianceSDP, groupDP);
    }

    delete enabledInEdges;
    calculatedDP = true;
}

/**
 * @brief Computes Proximity Prestige of each vertex
 * Also the mean value and the variance of it..
 *
 * Meaning: how close everyone else who can reach this actor actually is, on average -
 * prestige's answer to closeness centrality. It rewards being easy to reach *and* being
 * reachable by a large share of the network, so an actor reachable by only a couple of very
 * close others still scores lower than one reachable, at moderate distance, by almost
 * everyone.
 *
 * When to use: directed networks where you want a refined "how sought-after is this actor"
 * score that accounts for distance, not just a raw inbound-tie count (see Degree Prestige for
 * the simpler version).
 *
 * Compare to: Influence Range Closeness Centrality (IRCC, see centralityClosenessIR()) is this
 * same idea from the centrality side (distance *to* others) rather than prestige's (distance
 * *from* others), and likewise works on disconnected graphs.
 *
 * Weights: shortest-path-based, same as CC/IRCC - if a weight represents value/strength, invert
 * it so a strong tie behaves like a short/cheap path.
 *
 * Math: for actor i, let I_i be the set of actors that can reach i (its influence domain).
 * PP(i) = [ |I_i| / (V-1) ] / [ (sum of d(j,i) for j in I_i) / |I_i| ] - the fraction of the
 * network that can reach i, divided by their average distance to i.
 */
void Graph::prestigeProximity(const bool considerWeights,
                              const bool inverseWeights,
                              const bool dropIsolates)
{
    if (calculatedPP)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute proximity prestige. Returning";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing Proximity prestige scores...";

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

    QString pMsg = tr("Computing Proximity Prestige scores. \nPlease wait ...");
    progressStatus(pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (progressCanceled())
        {
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

            dist = apspDistance((*jt)->number(), (*it)->number());

            if (dist != RAND_MAX)
            {
                PP += dist;
                Ii++; // compute |Ii|
            }
        }

        qCDebug(lcCentrality) << "vertex"
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

    qCDebug(lcCentrality) << "sumPP = " << sumPP
             << " meanPP = " << meanPP
             << " variancePP " << variancePP;

    calculatedPP = true;
}

/**
 * @brief Calculates the PageRank Prestige of each vertex
 *
 * Meaning: the same idea Google originally used to rank web pages - an actor is prestigious
 * if prestigious actors point to it. Unlike plain degree prestige (which treats every inbound
 * tie equally), a link from someone with few outbound ties and high prestige counts for much
 * more than one of a hundred outbound ties from someone unremarkable, and that prestige keeps
 * circulating until every score stabilizes.
 *
 * When to use: directed networks where endorsement quality matters, not just quantity - web
 * links, citation networks, recommendation/referral graphs - anywhere "an endorsement from
 * someone important should count for more."
 *
 * Compare to: Degree Prestige (DP) counts inbound ties equally; PageRank weighs each one by
 * the endorser's own prestige divided by their out-degree. Eigenvector Centrality (EVC, see
 * centralityEigenvector()) is the closest centrality-side analogue - both are "importance
 * feeds back on itself" measures - but EVC is built for undirected/symmetric graphs while
 * PageRank is built for directed graphs with an explicit damping factor.
 *
 * Weights: no considerWeights/inverseWeights choice at all - every inbound link always counts
 * as weight 1, normalized by the endorser's out-degree (see Math below), regardless of any
 * edge weight set on the graph.
 *
 * Math: iteratively, PRP(i) = (1-d)/N + d * Sum_j( PRP(j) / outLinks(j) ) for every j linking
 * to i, where d is the damping factor (0.85, matching Google's original choice) and N is the
 * number of actors. Repeated until scores stop changing by more than a small delta.
 *
 * @param dropIsolates
 */
void Graph::prestigePageRank(const bool &dropIsolates)
{

    if (calculatedPRP)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute Pagerank scores. Return ";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing PageRank prestige scores...";

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

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        // At first, PR scores have probability distribution
        // from 0 to 1, so each one is set to 1/N
        (*it)->setPRP(1.0 / (qreal)N);

        // compute inEdgesCount() to warm up inEdgesConst for everyone
        inLinks = (*it)->inEdgesCount();
        outLinks = (*it)->outEdgesCount();
        qCDebug(lcCentrality) << "node "
                 << (*it)->number() << " PR = " << (*it)->PRP()
                 << " inLinks (set const): " << inLinks
                 << " outLinks (set const): " << outLinks;
    }

    if (edgesEnabled() == 0)
    {
        qCDebug(lcCentrality) << "all vertices are isolated and of equal PR. Stop";
        return;
    }

    if (progressCanceled())
    {
        return;
    }
    // begin iteration - continue until we reach our desired delta
    while (maxDelta > delta)
    {

        qCDebug(lcCentrality) << "ITERATION : " << iterations;

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

            qCDebug(lcCentrality) << "computing PR for node: "
                     << (*it)->number() << " current PR " << oldPRP;

            if ((*it)->isIsolated())
            {
                // isolates have constant PR = 1/N
                qCDebug(lcCentrality) << "isolated - CONTINUE ";
                continue;
            }

            jt = (*it)->m_inEdges.cbegin();

            qCDebug(lcCentrality) << "Iterate over inEdges of "
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

                qCDebug(lcCentrality) << "Node " << (*it)->number()
                         << " inLinked from neighbor " << referrer << " vpos "
                         << vpos[referrer];

                if (edgeExists(referrer, (*it)->number()))
                {
                    inLinks = m_graph[vpos[referrer]]->inEdgesCountConst();
                    outLinks = m_graph[vpos[referrer]]->outEdgesCountConst();

                    PRP = m_graph[vpos[referrer]]->PRP();

                    transferedPRP = (outLinks != 0) ? (PRP / outLinks) : PRP;

                    qCDebug(lcCentrality) << "neighbor " << referrer
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

            qCDebug(lcCentrality) << "Node "
                     << (*it)->number()
                     << " new PR = " << PRP
                     << " old PR was = " << oldPRP
                     << " diff = " << fabs(PRP - oldPRP);

            // calculate diff from last PageRank value for this vertex
            // and set it to minDelta if the latter is bigger.

            if (maxDelta < fabs(PRP - oldPRP))
            {
                maxDelta = fabs(PRP - oldPRP);
                qCDebug(lcCentrality) << "Setting new maxDelta = "
                         << maxDelta;
            }
        }

        // normalize in every iteration

        qCDebug(lcCentrality) << "sumPRP for this iteration " << sumPRP;

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

    if (progressCanceled())
    {
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

    qCDebug(lcCentrality) << "sumPRP = " << sumPRP << "  N = " << N
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

        qCDebug(lcCentrality) << "vertex: " << (*it)->number()
                 << " PR = " << PRP << " standard PR = " << SPRP
                 << " t_sumPRP " << t_sumPRP;

        t_variance = (PRP - meanPRP);
        t_variance *= t_variance;
        qCDebug(lcCentrality) << "PRP " << (*it)->PRP() << "  t_variance "
                 << PRP - meanPRP << " t_variance^2" << t_variance;
        variancePRP += t_variance;
    }

    qCDebug(lcCentrality) << "PRP' Variance   " << variancePRP << " N " << N;
    variancePRP = variancePRP / (qreal)N;
    qCDebug(lcCentrality) << "PRP' Variance: " << variancePRP;

    calculatedPRP = true;

    return;
}
