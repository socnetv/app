/**
 * @file graph_centrality_katz.cpp
 * @brief Implements Katz Centrality for the Graph class.
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
 * @brief Computes Katz Centrality of each vertex, with attenuation factor alpha.
 *
 * Meaning: credit for indirect connections, discounted the further away they are. Unlike
 * degree centrality, which only counts direct ties, Katz centrality also counts
 * friends-of-friends, friends-of-friends-of-friends, and so on - but each extra hop counts for
 * less, shrunk by alpha (a direct tie counts full price, a 2-hop tie counts alpha times as
 * much, a 3-hop tie counts alpha^2 times as much).
 *
 * When to use: ranking actors by total reach through the network, with a user-tunable
 * "how much does distance matter" knob - useful when the natural decay eigenvector centrality
 * settles on isn't the one you want, or the graph is disconnected/sparse enough that
 * eigenvector centrality's power iteration doesn't produce a useful ranking.
 *
 * Compare to: Eigenvector Centrality (EVC, see centralityEigenvector()) generalizes the same
 * "connections to well-connected others matter" idea via eigen-decomposition instead of an
 * explicit decay parameter. Bonacich Power Centrality (WS11, #39, planned) extends this same
 * formula with a second parameter beta that can be negative, flipping whether being connected
 * to well-connected others helps or hurts. Not to be confused with Power Centrality (PC,
 * Gil-Schmidt, see graphDistancesGeodesic()), a same-sounding but unrelated measure.
 *
 * Math: C_Katz(i) = sum_{k=1}^inf sum_j alpha^k (A^k)_ji, i.e. the discounted count of walks
 * of every length arriving at i, since (A^k)_ij is the number of length-k walks between i and
 * j. Computed in closed form via the matrix identity
 * I + xA + x^2*A^2 + x^3*A^3 + ... = (I - xA)^-1 (valid for |x| < 1/lambda_max, the same
 * geometric-series identity used for ordinary numbers, applied to matrices):
 * C_Katz = ((I - alpha*A^T)^-1 - I) * 1 (the transpose only matters for directed graphs; for
 * undirected graphs A = A^T). alpha must satisfy |alpha| < 1/lambda_max(A), where lambda_max is
 * the adjacency matrix's dominant eigenvalue (found via Matrix::powerIteration()), or the
 * underlying geometric series does not converge and the inversion is singular/meaningless.
 *
 * @param alpha
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::centralityKatz(const qreal &alpha,
                           const bool &considerWeights,
                           const bool &inverseWeights,
                           const bool &dropIsolates)
{
    if (calculatedKC)
    {
        qCDebug(lcCentrality) << "Graph not changed - KC already computed. Return.";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing Katz centrality scores... alpha =" << alpha;

    progressStatus(tr("Calculating Katz Centrality scores..."));

    classesKC = 0;
    discreteKCs.clear();
    sumKC = 0;
    maxKC = 0;
    minKC = RAND_MAX;
    varianceKC = 0;
    meanKC = 0;
    maxNodeKC = 0;
    minNodeKC = 0;

    VList::const_iterator it;

    const bool symmetrize = false;
    const int N = vertices(dropIsolates);

    if (N == 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setKC(0);
            (*it)->setSKC(0);
        }
        calculatedKC = true;
        return;
    }

    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);
    if (progressCanceled())
    {
        return;
    }

    // Find lambda_max (AM's dominant eigenvalue) to validate alpha against the
    // |alpha| < 1/lambda_max convergence bound - see Matrix::powerIteration()'s doc comment.
    qreal *seed = new (nothrow) qreal[N];
    Q_CHECK_PTR(seed);
    for (int k = 0; k < N; k++)
        seed[k] = 1;
    qreal dummySum = 0, dummyMax = 0, dummyMin = RAND_MAX;
    int dummyMaxI = 0, dummyMinI = 0;
    qreal lambdaMax = 0;
    AM.powerIteration(seed, dummySum, dummyMax, dummyMaxI, dummyMin, dummyMinI,
                      0.0000001, 500,
                      [this] { return progressCanceled(); },
                      &lambdaMax);
    delete[] seed;

    if (progressCanceled())
    {
        return;
    }

    qCDebug(lcCentrality) << "Graph::centralityKatz() - lambdaMax =" << lambdaMax
             << "alpha =" << alpha;

    if (lambdaMax > 0 && qAbs(alpha) >= 1.0 / lambdaMax)
    {
        qCDebug(lcCentrality) << "Graph::centralityKatz() - alpha does not satisfy the "
                                  "convergence bound |alpha| < 1/lambdaMax. Aborting.";
        progressStatus(tr("Katz Centrality is not defined for alpha = %1: it must satisfy "
                          "|alpha| < %2 (1 / the network's largest eigenvalue), or the "
                          "underlying computation does not converge.")
                          .arg(alpha)
                          .arg(1.0 / lambdaMax));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setKC(0);
            (*it)->setSKC(0);
        }
        calculatedKC = true;
        return;
    }

    progressStatus(tr("Computing Katz Centrality scores. \nPlease wait..."));

    // Build WM = I - alpha * A^T, then invert it into invM = (I - alpha*A^T)^-1.
    // transpose() heap-allocates a new Matrix each call (existing Matrix API convention -
    // see its own call sites in graph_reports.cpp); we own it here and delete it once done.
    Matrix &AT = AM.transpose();
    WM.resize(N, N);
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            WM.setItem(i, j, AT.item(i, j));
        }
    }
    delete &AT;
    WM.multiplyScalar(alpha);
    WM.subtractFromI(); // WM now holds (I - alpha*A^T)

    invM.resize(N, N);
    progressStatus(tr("Inverting (I - alpha*A^T). Please wait..."));
    const bool invertible = invM.inverse(WM, [this] { return progressCanceled(); });

    if (progressCanceled())
    {
        return;
    }

    if (!invertible)
    {
        qCDebug(lcCentrality) << "Graph::centralityKatz() - (I - alpha*A^T) is singular.";
        progressStatus(tr("Katz Centrality is not defined: (I - alpha*A^T) is singular."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setKC(0);
            (*it)->setSKC(0);
        }
        calculatedKC = true;
        return;
    }

    // alpha is only cached as "last known good" once the computation actually succeeds -
    // layoutByProminenceIndex() (WS11, #10) relies on this to know Katz is safe to re-run.
    m_lastKatzAlpha = alpha;

    // C_Katz(i) = (row sum of invM, row i) - 1: the "-1" removes the k=0 "self" term that the
    // (I - alpha*A^T)^-1 identity always includes but Katz's own sum (starting at k=1) does not.
    qreal *KC = new (nothrow) qreal[N];
    Q_CHECK_PTR(KC);
    for (int i = 0; i < N; i++)
    {
        qreal rowSum = 0;
        for (int j = 0; j < N; j++)
        {
            rowSum += invM.item(i, j);
        }
        KC[i] = rowSum - 1.0;
    }

    int i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            (*it)->setKC(0);
            (*it)->setSKC(0);
            continue;
        }

        (*it)->setKC(KC[i]);
        sumKC += KC[i];

        if (KC[i] > maxKC)
        {
            maxKC = KC[i];
            maxNodeKC = (*it)->number();
        }
        if (KC[i] < minKC)
        {
            minKC = KC[i];
            minNodeKC = (*it)->number();
        }

        i++;
    }

    delete[] KC;

    meanKC = sumKC / static_cast<qreal>(N);

    qreal SKC = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            continue;
        }

        SKC = (maxKC != 0) ? ((*it)->KC() / maxKC) : 0;
        (*it)->setSKC(SKC);

        resolveClasses(SKC, discreteKCs, classesKC);
        varianceKC += ((*it)->KC() - meanKC) * ((*it)->KC() - meanKC);
    }

    varianceKC /= static_cast<qreal>(N);

    calculatedKC = true;
}
