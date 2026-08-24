/**
 * @file graph_centrality.cpp
 * @brief Implements centrality-based prominence indices (e.g., degree, eigenvector, information, closeness IR) for the Graph class.
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
 * @brief Computes the Information centrality of each vertex - diagonal included
 *  Note that there is no known generalization of Stephenson & Zelen's theory
 *  for information centrality to directional data
 *
 * Plain language: most centrality measures only look at the single shortest path between
 * two actors. Information centrality gives an actor credit for *every* path connecting it
 * to others, not just the best one - shorter, less roundabout paths count for more - so an
 * actor sitting on many decent alternative routes can score as centrally as one sitting on
 * the single best route.
 *
 * Math: build matrix B where B_ii = 1 + (sum of i's edge weights) and B_ij = 1 - w_ij for
 * i != j, then invert it to get C = B^-1. Information centrality is
 * IC(i) = 1 / [ C_ii + (tr(C) - 2*R) / n ], where tr(C) is the trace of C and R is the sum
 * of any one row of C (interchangeable by construction of B).
 *
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityInformation(const bool considerWeights,
                                  const bool inverseWeights)
{
    qCDebug(lcCentrality) << "Graph::centralityInformation()";

    if (calculatedIC)
    {
        qCDebug(lcCentrality) << "Graph::centralityInformation() - already computed. Return.";
        return;
    }

    discreteICs.clear();
    sumIC = 0;
    maxIC = 0;
    t_sumIC = 0;
    minIC = RAND_MAX;
    classesIC = 0;
    varianceIC = 0;
    meanIC = 0;
    maxNodeIC = 0;
    minNodeIC = 0;

    VList::const_iterator it;

    int i = 0, j = 0;

    qreal m_weight = 0;
    qreal weightSum = 1;
    qreal traceC = 0;
    qreal commonRowSum = 0;
    qreal IC = 0;
    qreal SIC = 0;

    /* Note: isolated nodes must be dropped from the AM
       Otherwise, the SIGMA matrix might be singular, therefore non-invertible. */
    const bool dropIsolates = true;
    const bool symmetrize = true;
    const int n = vertices(dropIsolates, false, true);

    /* Degenerate case: no non-isolated vertices */
    if (n == 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setIC(0);
            (*it)->setSIC(0);
        }
        calculatedIC = true;
        return;
    }

    /* Degenerate case: IC is not meaningful for fewer than 2 non-isolated vertices */
    if (n < 2)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setIC(0);
            (*it)->setSIC(0);
        }
        calculatedIC = true;
        return;
    }

    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);
    if (progressCanceled())
    {
        return;
    }

    QString pMsg = tr("Computing Information Centralities. \nPlease wait...");
    progressStatus(pMsg);

    WM.resize(n, n);
    invM.resize(n, n);

    for (i = 0; i < n; i++)
    {
        weightSum = 1;
        for (j = 0; j < n; j++)
        {
            if (i == j)
                continue;

            m_weight = AM.item(i, j);
            weightSum += m_weight;              // sum of weights for all edges incident to i
            WM.setItem(i, j, 1 - m_weight);
        }
        WM.setItem(i, i, weightSum);
    }

    if (progressCanceled())
    {
        return;
    }

    progressStatus(tr("Computing inverse adjacency matrix. Please wait..."));
    const bool invertible = invM.inverse(WM, [this] { return progressCanceled(); });

    if (progressCanceled())
    {
        return;
    }

    // Fix #269: WM can be singular; without this check IC would be computed from invM
    // left as all zeros, silently reporting wrong scores instead of "not defined".
    if (!invertible)
    {
        qCDebug(lcCentrality) << "Graph::centralityInformation() - weight matrix is singular.";
        progressStatus(tr("Information Centrality is not defined: the weight matrix is singular."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setIC(0);
            (*it)->setSIC(0);
        }
        calculatedIC = true;
        return;
    }

    progressStatus(tr("Computing IC scores. Please wait..."));

    traceC = 0;
    commonRowSum = 0;

    for (j = 0; j < n; j++)
    {
        commonRowSum += invM.item(0, j);
    }

    for (i = 0; i < n; i++)
    {
        traceC += invM.item(i, i);
    }

    i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated())
        {
            (*it)->setIC(0);
            (*it)->setSIC(0);
            continue;
        }

        IC = 1.0 / (invM.item(i, i) + (traceC - 2.0 * commonRowSum) / n);
        (*it)->setIC(IC);
        t_sumIC += IC;
        ++i;
    }

    if (t_sumIC > 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if ((*it)->isIsolated())
            {
                (*it)->setSIC(0);
                continue;
            }

            IC = (*it)->IC();
            SIC = IC / t_sumIC;
            (*it)->setSIC(SIC);
            sumIC += SIC;
            resolveClasses(SIC, discreteICs, classesIC);
            minmax(SIC, (*it), maxIC, minIC, maxNodeIC, minNodeIC);
        }

        meanIC = sumIC / static_cast<qreal>(n);

        qreal x = 0;
        varianceIC = 0;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if ((*it)->isIsolated())
                continue;

            x = ((*it)->SIC() - meanIC);
            x *= x;
            varianceIC += x;
        }

        varianceIC /= static_cast<qreal>(n);
    }
    else
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setSIC(0);
        }
        sumIC = 0;
        meanIC = 0;
        varianceIC = 0;
    }

    calculatedIC = true;

    WM.clear();
}

/**
 * @brief Computes Eigenvector centrality
 *
 * Plain language: not just how many connections you have, but how important your
 * connections are - being tied to a few highly-connected people can outscore being tied to
 * many poorly-connected ones. It is a "popularity feeds back on itself" measure: your score
 * depends on your neighbors' scores, which depend on their neighbors' scores, and so on
 * until the whole network settles into one stable ranking.
 *
 * Math: the eigenvector centrality vector x is the dominant eigenvector of the adjacency
 * matrix A, i.e. the vector solving A*x = lambda_max*x for the largest eigenvalue lambda_max.
 * Computed here via power iteration (Matrix::powerIteration()): start from any vector,
 * repeatedly multiply by A and rescale to unit length - it converges to that eigenvector.
 *
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityEigenvector(const bool &considerWeights,
                                  const bool &inverseWeights,
                                  const bool &dropIsolates)
{
    if (calculatedEVC)
    {
        qCDebug(lcCentrality) << "Graph not changed - EVC already computed. Return.";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing Eigenvector centrality scores...";

    progressStatus(tr("Calculating EVC scores..."));

    classesEVC = 0;
    discreteEVCs.clear();
    sumEVC = 0;
    maxEVC = 0;
    minEVC = RAND_MAX;
    varianceEVC = 0;
    meanEVC = 0;
    maxNodeEVC = 0;
    minNodeEVC = 0;

    VList::const_iterator it;

    const bool symmetrize = false;
    const bool useDegrees = false;
    int i = 0;
    const int N = vertices(dropIsolates);

    if (N == 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setEVC(0);
            (*it)->setSEVC(0);
        }
        calculatedEVC = true;
        return;
    }

    qreal *EVC = new (nothrow) qreal[N];
    Q_CHECK_PTR(EVC);
    qreal SEVC = 0;

    createMatrixAdjacency(dropIsolates, considerWeights,
                          inverseWeights, symmetrize);
    if (progressCanceled())
    {
        delete[] EVC;
        return;
    }

    QString pMsg = tr("Computing Eigenvector Centrality scores. \nPlease wait...");
    progressStatus(pMsg);

    if (useDegrees)
    {
        qCDebug(lcCentrality) << "Using outDegree for initial EVC vector";

        progressStatus(tr("Computing outDegrees. Please wait..."));

        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if ((*it)->isIsolated() && dropIsolates)
                continue;

            EVC[i] = (*it)->degreeOut();
            i++;
        }
    }
    else
    {
        qCDebug(lcCentrality) << "Using unit initial EVC vector";
        for (int k = 0; k < N; k++)
            EVC[k] = 1;
    }

    if (progressCanceled())
    {
        delete[] EVC;
        return;
    }

    AM.powerIteration(EVC, sumEVC, maxEVC, maxNodeEVC,
                      minEVC, minNodeEVC,
                      0.0000001, 500,
                      [this] { return progressCanceled(); });

    if (progressCanceled())
    {
        delete[] EVC;
        return;
    }

    progressStatus(tr("Leading eigenvector computed. "
                      "Analysing centralities. Please wait..."));

    // Recompute sum defensively from final vector
    sumEVC = 0;
    for (int k = 0; k < N; ++k)
        sumEVC += EVC[k];

    meanEVC = sumEVC / static_cast<qreal>(N);

    i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            (*it)->setEVC(0);
            (*it)->setSEVC(0);
            continue;
        }

        (*it)->setEVC(EVC[i]);

        SEVC = (maxEVC != 0) ? (EVC[i] / maxEVC) : 0;
        (*it)->setSEVC(SEVC);

        resolveClasses(SEVC, discreteEVCs, classesEVC);
        varianceEVC += (EVC[i] - meanEVC) * (EVC[i] - meanEVC);

        i++;
    }

    varianceEVC /= static_cast<qreal>(N);

    calculatedEVC = true;

    delete[] EVC;
}

/**
 * @brief Calculates the degree (outDegree) centrality of each vertex - diagonal included
 *
 * Plain language: the simplest centrality there is - how many direct connections (ties)
 * does this actor have? More connections means more prominence, full stop; no attention is
 * paid to who those connections are or how the rest of the network is shaped.
 *
 * Math: DC(i) = number of edges incident to i (or the sum of their weights, if weights are
 * considered). Standardized SDC(i) = DC(i) / (N-1), the fraction of all other actors i is
 * directly tied to.
 *
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::centralityDegree(const bool &considerWeights, const bool &dropIsolates)
{

    if (calculatedDC)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute degree centralities. Returning.";
        return;
    }
    qreal DC = 0, nom = 0, denom = 0, SDC = 0;
    qreal weight;
    classesSDC = 0;
    discreteSDCs.clear();
    sumSDC = 0;
    sumDC = 0;
    maxSDC = 0;
    minSDC = RAND_MAX;
    varianceSDC = 0;
    meanSDC = 0;
    int N = vertices(dropIsolates);

    VList::const_iterator it, it1;

    QString pMsg = tr("Computing out-Degree Centralities for %1 nodes. \nPlease wait...").arg(N);
    qCDebug(lcCentrality) << pMsg;
    progressStatus(pMsg);

    if (progressCanceled())
    {
        return;
    }
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        DC = 0;

        if (!(*it)->isEnabled() || (dropIsolates && (*it)->isIsolated()))
        {
            continue;
        }

        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {

            if (!(*it1)->isEnabled() || (dropIsolates && (*it1)->isIsolated()))
            {
                continue;
            }

            if ((weight = edgeExists((*it)->number(), (*it1)->number())) != 0.0)
            {
                if (considerWeights)
                    DC += weight;
                else
                    DC++;

                // check here if the matrix is symmetric - we need this below
                if (weight != edgeExists((*it1)->number(), (*it)->number()))
                    m_graphIsSymmetric = false;
            }
        }

        (*it)->setDC(DC); // Set OutDegree

        sumDC += DC; // store sumDC (for std calc below)
    }

    if (progressCanceled())
    {
        return;
    }
    // Calculate std Out-Degree, min, max, classes and sumSDC
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        DC = (*it)->DC();
        if (!considerWeights)
        {
            SDC = (DC / (N - 1.0));
        }
        else
        {
            SDC = (DC / (sumDC));
        }
        (*it)->setSDC(SDC); // Set Standard DC

        qCDebug(lcCentrality) << "vertex" << (*it)->number() << "-- DC=" << DC << "SDC=" << SDC;
        sumSDC += SDC;

        resolveClasses(SDC, discreteSDCs, classesSDC);

        if (maxSDC < SDC)
        {
            maxSDC = SDC;
            maxNodeSDC = (*it)->number();
        }
        if (minSDC > SDC)
        {
            minSDC = SDC;
            minNodeSDC = (*it)->number();
        }
    }

    if (minSDC == maxSDC)
        maxNodeSDC = -1;

    meanSDC = sumSDC / (qreal)N;

    // Calculate Variance and the Degree Centralization of the whole graph.
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (dropIsolates && (*it)->isIsolated())
        {
            continue;
        }
        SDC = (*it)->SDC();
        nom += (maxSDC - SDC);
        varianceSDC += (SDC - meanSDC) * (SDC - meanSDC);
    }
    varianceSDC = varianceSDC / (qreal)N;

    if (m_graphIsSymmetric)
    {
        // we divide by N-1 because we use std C values
        denom = (N - 1.0) * (N - 2.0) / (N - 1.0);
    }
    else
    {
        denom = (N - 1.0) * (N - 1.0) / (N - 1.0);
    }

    if (N < 3)
    {
        denom = N - 1.0;
    }

    if (!considerWeights)
    {
        groupDC = nom / denom;
    }

    calculatedDC = true;
}

/**
 * @brief Computes an "improved" closeness centrality index, IRCC, which can be used
 * on disconnected graphs.
 * IRCC is an improved node-level centrality closeness index which focuses on the
 * influence range of each node (the set of nodes that are reachable from it)
 * For each node v, this index calculates the fraction of nodes in its influence
 * range and divides it by the average distance of those nodes from v,
 * ignoring nodes that are not reachable from it.
 *
 * Plain language: how efficiently can an actor reach the part of the network it *can*
 * reach? Plain closeness centrality breaks down on a disconnected network, since "distance
 * to everyone" is undefined once some actors are unreachable. IRCC sidesteps that by only
 * ever averaging distances to actors that are actually reachable, so it stays meaningful
 * even when the network is split into several disconnected pieces.
 *
 * Math: for actor i, let J_i be the set of nodes reachable from i (its influence range).
 * IRCC(i) = [ |J_i| / (N-1) ] / [ (sum of d(i,j) for j in J_i) / |J_i| ] - the fraction of
 * the network i can reach, divided by the average distance to that reachable set.
 *
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::centralityClosenessIR(const bool considerWeights,
                                  const bool inverseWeights,
                                  const bool dropIsolates)
{

    if (calculatedIRCC)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute IRCC. Returning";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing IRCC closeness centrality...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);
    if (progressCanceled())
    {
        return;
    }
    // calculate centralities
    VList::const_iterator it, jt;
    qreal IRCC = 0, SIRCC = 0;
    qreal Ji = 0;
    qreal dist = 0;
    qreal sumD = 0;
    qreal averageD = 0;
    qreal N = vertices(dropIsolates, false, true);
    classesIRCC = 0;
    discreteIRCCs.clear();
    sumIRCC = 0;
    maxIRCC = 0;
    minIRCC = N - 1;
    varianceIRCC = 0;
    meanIRCC = 0;

    QString pMsg = tr("Computing Influence Range Centrality scores. \n"
                      "Please wait");
    progressStatus(pMsg);

    qCDebug(lcCentrality) << "dropIsolates" << dropIsolates;
    qCDebug(lcCentrality) << "computing scores for actors: " << N;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (progressCanceled())
        {
            return;
        }
        IRCC = 0;
        sumD = 0;
        Ji = 0;
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

            dist = apspDistance((*it)->number(), (*jt)->number());

            if (dist != RAND_MAX)
            {
                sumD += dist;
                Ji++; // compute |Ji|
            }
            qCDebug(lcCentrality) << "dist(" << (*it)->number()
                     << "," << (*jt)->number() << ") =" << dist << "sumD" << sumD << " Ji" << Ji;
        }

        qCDebug(lcCentrality) << "" << (*it)->number()
                 << " sumD" << sumD
                 << "distanceSum" << (*it)->distanceSum();

        // sanity check for sumD=0 (=> node is disconnected)
        if (sumD != 0)
        {
            averageD = sumD / Ji;
            qCDebug(lcCentrality) << "averageD = sumD /  Ji" << averageD;
            qCDebug(lcCentrality) << "Ji / (N-1)" << Ji << "/" << N - 1;
            IRCC = (Ji / (qreal)(N - 1)) / averageD;
            qCDebug(lcCentrality) << "[ Ji / (N-1) ] / [ sumD / Ji]" << IRCC;
        }

        sumIRCC += IRCC;
        (*it)->setIRCC(IRCC);
        (*it)->setSIRCC(IRCC); // IRCC is a ratio, already std
        resolveClasses(IRCC, discreteIRCCs, classesIRCC);
        minmax(IRCC, (*it), maxIRCC, minIRCC, maxNodeIRCC, minNodeIRCC);
    }

    meanIRCC = sumIRCC / (qreal)N;

    if (minIRCC == maxIRCC)
        maxNodeIRCC = -1;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!dropIsolates || !(*it)->isIsolated())
        {
            SIRCC = (*it)->SIRCC();
            varianceIRCC += (SIRCC - meanIRCC) * (SIRCC - meanIRCC);
        }
    }

    varianceIRCC = varianceIRCC / (qreal)N;

    calculatedIRCC = true;
}

/**
 * @brief Computes minimum and maximum centralities during graphDistancesGeodesic()
 * @param C
 * @param v
 * @param max
 * @param min
 * @param maxNode
 * @param minNode
 */
void Graph::minmax(qreal C, GraphVertex *v, qreal &max, qreal &min, int &maxNode, int &minNode)
{
    qCDebug(lcCentrality) << "MINMAX C = " << C << "  max = " << max << "  min = " << min << " name = " << v->number();
    if (C > max)
    {
        max = C;
        maxNode = v->number();
    }
    if (C < min)
    {
        min = C;
        minNode = v->number();
    }
}

/**
 * @brief Checks if score C is a new prominence class
 * If yes, it stores that number in a QHash<QString,int> type where the score is the key.
 * If no, increases the frequency of this prominence score by 1
 * Called from graphDistancesGeodesic()
 * @param C
 * @param discreteClasses
 * @param classes
 */
void Graph::resolveClasses(qreal C, H_StrToInt &discreteClasses, int &classes)
{
    int frq = 0;
    H_StrToInt::iterator it2;
    it2 = discreteClasses.find(QString::number(C)); // Amort. O(1) complexity
    if (it2 == discreteClasses.end())
    {
        classes++;
        discreteClasses.insert(QString::number(C), 1);
    }
    else
    {
        frq = it2.value();
        discreteClasses.insert(QString::number(C), frq + 1);
    }
}

/**
 * @brief Overloaded method. It only adds displaying current vertex for debugging purposes.
 * @param C
 * @param discreteClasses
 * @param classes
 * @param vertex
 */
void Graph::resolveClasses(qreal C, H_StrToInt &discreteClasses, int &classes, int vertex)
{
    int frq = 0;
    H_StrToInt::iterator it2;
    Q_UNUSED(vertex);
    it2 = discreteClasses.find(QString::number(C)); // Amort. O(1) complexity
    if (it2 == discreteClasses.end())
    {
        classes++;
        discreteClasses.insert(QString::number(C), 1);
    }
    else
    {
        frq = it2.value();
        discreteClasses.insert(QString::number(C), frq + 1);
    }
}

/**
 * @brief Returns true if the given centrality/prestige index has been computed.
 *
 * Uses the per-index calculated* flags set by each analysis method.
 * Matches the IndexType enum defined in global.h.
 */
bool Graph::isCentralityIndexComputed(const IndexType index) const
{
    switch (index)
    {
    case IndexType::DC:
        return calculatedDC;
    case IndexType::CC:   // CC, IRCC, BC, SC, EC, PC are all
    case IndexType::IRCC: // byproducts of graphDistancesGeodesic
    case IndexType::BC:
    case IndexType::SC:
    case IndexType::EC:
    case IndexType::PC:
        return calculatedCentralities;
    case IndexType::IC:
        return calculatedIC;
    case IndexType::EVC:
        return calculatedEVC;
    case IndexType::DP:
        return calculatedDP;
    case IndexType::PRP:
        return calculatedPRP;
    case IndexType::PP:
        return calculatedPP;
    case IndexType::CLC:
        return !m_graph.isEmpty() && m_graph.first()->hasCLC();
    default:
        return false;
    }
}