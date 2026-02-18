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
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityInformation(const bool considerWeights,
                                  const bool inverseWeights)
{

    qDebug() << "Graph::centralityInformation()";

    if (calculatedIC)
    {
        qDebug() << "Graph::centralityInformation() - already computed. Return.";
        return;
    }

    discreteICs.clear();
    sumIC = 0;
    maxIC = 0;
    t_sumIC = 0;
    minIC = RAND_MAX;
    classesIC = 0;
    varianceIC = 0;

    VList::const_iterator it;

    int i = 0, j = 0;

    qreal m_weight = 0, weightSum = 1, diagonalEntriesSum = 0, rowSum = 0;
    qreal IC = 0, SIC = 0;
    /* Note: isolated nodes must be dropped from the AM
        Otherwise, the SIGMA matrix might be singular, therefore non-invertible. */
    bool dropIsolates = true;
    bool symmetrize = true;
    int n = vertices(dropIsolates, false, true);

    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);

    QString pMsg = tr("Computing Information Centralities. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(n, pMsg);

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
            weightSum += m_weight; // sum of weights for all edges incident to i
            WM.setItem(i, j, 1 - m_weight);
        }
        WM.setItem(i, i, weightSum);
    }

    emit signalProgressBoxUpdate(n / 3);
    emit statusMessage(tr("Computing inverse adjacency matrix. Please wait..."));

    invM.inverse(WM);

    emit statusMessage(tr("Computing IC scores. Please wait..."));

    emit signalProgressBoxUpdate(2 * n / 3);

    diagonalEntriesSum = 0;
    rowSum = 0;
    for (j = 0; j < n; j++)
    {
        rowSum += invM.item(0, j);
    }
    for (i = 0; i < n; i++)
    {
        diagonalEntriesSum += invM.item(i, i); // calculate the matrix trace
    }

    i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated())
        {
            (*it)->setIC(0);
            continue;
        }
        IC = 1.0 / (invM.item(i, i) + (diagonalEntriesSum - 2.0 * rowSum) / n);

        (*it)->setIC(IC);
        t_sumIC += IC;
        i++;
    }
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        IC = (*it)->IC();
        SIC = IC / t_sumIC;
        (*it)->setSIC(SIC);
        sumIC += SIC;
        resolveClasses(SIC, discreteICs, classesIC);
        minmax(SIC, (*it), maxIC, minIC, maxNodeIC, minNodeIC);
    }

    qreal x = 0;
    meanIC = sumIC / static_cast<qreal>(n);

    varianceIC = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        x = ((*it)->SIC() - meanIC);
        x *= x;
        varianceIC += x;
    }

    varianceIC /= (qreal)n;

    calculatedIC = true;

    WM.clear();

    emit signalProgressBoxUpdate(n);
    emit signalProgressBoxKill();
}

/**
 * @brief Computes Eigenvector centrality
 * @param considerWeights
 * @param inverseWeights
 */
void Graph::centralityEigenvector(const bool &considerWeights,
                                  const bool &inverseWeights,
                                  const bool &dropIsolates)
{

    if (calculatedEVC)
    {
        qDebug() << "Graph not changed - EVC already computed. Return.";
        return;
    }

    qDebug() << "(Re)Computing Eigenvector centrality scores...";

    emit statusMessage((tr("Calculating EVC scores...")));

    classesEVC = 0;
    discreteEVCs.clear();
    sumEVC = 0;
    maxEVC = 0;
    minEVC = RAND_MAX;
    varianceEVC = 0;
    meanEVC = 0;
    VList::const_iterator it;

    bool symmetrize = false;
    bool useDegrees = false;
    int i = 0;
    int N = vertices(dropIsolates);

    qreal *EVC = new (nothrow) qreal[N];
    Q_CHECK_PTR(EVC);
    qreal SEVC = 0;

    createMatrixAdjacency(dropIsolates, considerWeights,
                          inverseWeights, symmetrize);

    QString pMsg = tr("Computing Eigenvector Centrality scores. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    if (useDegrees)
    {

        qDebug() << "Using outDegree for initial EVC vector";

        emit statusMessage(tr("Computing outDegrees. Please wait..."));

        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {

            if (!(*it)->isIsolated() && dropIsolates)
            {
                continue;
            }

            EVC[i] = (*it)->degreeOut();

            i++;
        }
    }
    else
    {
        qDebug() << "Using unit initial EVC vector";
        for (int i = 0; i < N; i++)
        {
            EVC[i] = 1;
        }
    }

    emit signalProgressBoxUpdate(N / 3);

    AM.powerIteration(EVC, sumEVC, maxEVC, maxNodeEVC,
                      minEVC, minNodeEVC,
                      0.0000001, 500);

    emit signalProgressBoxUpdate(2 * N / 3);

    emit statusMessage(tr("Leading eigenvector computed. "
                          "Analysing centralities. Please wait..."));

    i = 0;

    meanEVC = sumEVC / (qreal)N;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        if (!(*it)->isIsolated() && dropIsolates)
        {
            continue;
        }

        (*it)->setEVC(EVC[i]);
        if (maxEVC != 0)
        {
            SEVC = EVC[i] / maxEVC;
        }
        else
        {
            SEVC = 0;
        }

        (*it)->setSEVC(SEVC);

        resolveClasses(SEVC, discreteEVCs, classesEVC);

        varianceEVC += (EVC[i] - meanEVC) * (EVC[i] - meanEVC);

        i++;
    }

    varianceEVC = varianceEVC / (qreal)N;

    // group eigenvector centralization measure is
    // S(cmax - c(vi)) divided by the maximum value possible,
    // where c(vi) is the eigenvector centrality of vertex vi.

    calculatedEVC = true;

    delete[] EVC;

    emit signalProgressBoxUpdate(N);
    emit signalProgressBoxKill();
}

/**
 * @brief Calculates the degree (outDegree) centrality of each vertex - diagonal included
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::centralityDegree(const bool &considerWeights, const bool &dropIsolates)
{

    if (calculatedDC)
    {
        qDebug() << "Graph not changed - no need to recompute degree centralities. Returning.";
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
    qDebug() << pMsg;
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    emit signalProgressBoxUpdate(N / 3);

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

    emit signalProgressBoxUpdate(2 * N / 3);

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

        qDebug() << "vertex" << (*it)->number() << "-- DC=" << DC << "SDC=" << SDC;
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

    emit signalProgressBoxUpdate(N);
    emit signalProgressBoxKill();
}

/**
 * @brief Computes an "improved" closeness centrality index, IRCC, which can be used
 * on disconnected graphs.
 * IRCC is an improved node-level centrality closeness index which focuses on the
 * influence range of each node (the set of nodes that are reachable from it)
 * For each node v, this index calculates the fraction of nodes in its influence
 * range and divides it by the average distance of those nodes from v,
 * ignoring nodes that are not reachable from it.
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
        qDebug() << "Graph not changed - no need to recompute IRCC. Returning";
        return;
    }

    qDebug() << "(Re)Computing IRCC closeness centrality...";

    graphDistancesGeodesic(false, considerWeights, inverseWeights, dropIsolates);

    // calculate centralities
    VList::const_iterator it, jt;
    int progressCounter = 0;
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
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    qDebug() << "dropIsolates" << dropIsolates;
    qDebug() << "computing scores for actors: " << N;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        emit signalProgressBoxUpdate(++progressCounter);

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

            dist = (*it)->distance((*jt)->number());

            if (dist != RAND_MAX)
            {
                sumD += dist;
                Ji++; // compute |Ji|
            }
            qDebug() << "dist(" << (*it)->number()
                     << "," << (*jt)->number() << ") =" << dist << "sumD" << sumD << " Ji" << Ji;
        }

        qDebug() << "" << (*it)->number()
                 << " sumD" << sumD
                 << "distanceSum" << (*it)->distanceSum();

        // sanity check for sumD=0 (=> node is disconnected)
        if (sumD != 0)
        {
            averageD = sumD / Ji;
            qDebug() << "averageD = sumD /  Ji" << averageD;
            qDebug() << "Ji / (N-1)" << Ji << "/" << N - 1;
            IRCC = (Ji / (qreal)(N - 1)) / averageD;
            qDebug() << "[ Ji / (N-1) ] / [ sumD / Ji]" << IRCC;
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

    emit signalProgressBoxKill();
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
    qDebug() << "MINMAX C = " << C << "  max = " << max << "  min = " << min << " name = " << v->number();
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

