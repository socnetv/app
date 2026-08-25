/**
 * @file graph_centrality_bonacich.cpp
 * @brief Implements Bonacich Power Centrality for the Graph class.
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
 * @brief Computes Bonacich Power Centrality of each vertex, with parameters alpha and beta.
 *
 * Meaning: like Katz Centrality, credit for indirect connections discounted by distance - but
 * with a twist: beta can be negative, in which case being tied to well-connected others can
 * *hurt* rather than help a node's score (e.g. a buyer connected to powerful sellers has less
 * bargaining power the more powerful those sellers are). alpha is a free overall scale factor
 * that doesn't change the relative ranking, only the numbers' size.
 *
 * When to use: whenever the sign of "connections to well-connected others" itself matters
 * (bargaining power, exchange networks) rather than always being a good thing - Katz Centrality
 * and Eigenvector Centrality both assume more indirect connections is always better.
 *
 * Compare to: Katz Centrality (KC, see centralityKatz()) is the same walk-counting idea with a
 * single always-positive decay parameter; Bonacich generalizes it with a second, possibly
 * negative parameter beta and a free scale factor alpha. Not to be confused with Power
 * Centrality (PC, Gil-Schmidt, see graphDistancesGeodesic()), a same-sounding but unrelated
 * measure.
 *
 * Math: b = alpha * (I - beta*R)^-1 * R * 1, where R = A^T (the transpose only matters for
 * directed graphs, same convention as Katz). Only beta must satisfy |beta| < 1/lambda_max(A) for
 * the underlying geometric series to converge; alpha is an unconstrained outer scale factor.
 * Unlike every other centrality measure in this app, b(i) can come out negative when beta is
 * negative - that is a genuine property of the measure, not a bug, and SBPC (b divided by max b)
 * is not guaranteed to land in [0,1] in that case.
 *
 * @param alpha
 * @param beta
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::centralityBonacich(const qreal &alpha,
                               const qreal &beta,
                               const bool &considerWeights,
                               const bool &inverseWeights,
                               const bool &dropIsolates)
{
    if (calculatedBPC && m_lastBonacichAlpha == alpha && m_lastBonacichBeta == beta)
    {
        qCDebug(lcCentrality) << "Graph not changed and alpha/beta unchanged - BPC already computed. Return.";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing Bonacich Power centrality scores... alpha ="
             << alpha << "beta =" << beta;

    progressStatus(tr("Calculating Bonacich Power Centrality scores..."));

    classesBPC = 0;
    discreteBPCs.clear();
    sumBPC = 0;
    maxBPC = 0;
    minBPC = RAND_MAX;
    varianceBPC = 0;
    meanBPC = 0;
    maxNodeBPC = 0;
    minNodeBPC = 0;

    VList::const_iterator it;

    const bool symmetrize = false;
    const int N = vertices(dropIsolates);

    if (N == 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setBPC(0);
            (*it)->setSBPC(0);
        }
        calculatedBPC = true;
        return;
    }

    createMatrixAdjacency(dropIsolates, considerWeights, inverseWeights, symmetrize);
    if (progressCanceled())
    {
        return;
    }

    // Find lambda_max (AM's dominant eigenvalue) to validate beta against the
    // |beta| < 1/lambda_max convergence bound - see Matrix::powerIteration()'s doc comment.
    // Unlike Katz, alpha here is a free outer scale factor and has no such bound.
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

    qCDebug(lcCentrality) << "Graph::centralityBonacich() - lambdaMax =" << lambdaMax
             << "beta =" << beta;

    if (lambdaMax > 0 && qAbs(beta) >= 1.0 / lambdaMax)
    {
        qCDebug(lcCentrality) << "Graph::centralityBonacich() - beta does not satisfy the "
                                  "convergence bound |beta| < 1/lambdaMax. Aborting.";
        progressStatus(tr("Bonacich Power Centrality is not defined for beta = %1: it must "
                          "satisfy |beta| < %2 (1 / the network's largest eigenvalue), or the "
                          "underlying computation does not converge.")
                          .arg(beta)
                          .arg(1.0 / lambdaMax));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setBPC(0);
            (*it)->setSBPC(0);
        }
        calculatedBPC = true;
        return;
    }

    progressStatus(tr("Computing Bonacich Power Centrality scores. \nPlease wait..."));

    // Build R = A^T into WM (same transpose convention as Katz). transpose() heap-allocates a
    // new Matrix each call; we own it here and delete it once copied.
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

    // R*1 (row sums of R) must be captured before WM is overwritten into (I - beta*R) below.
    qreal *Rsum = new (nothrow) qreal[N];
    Q_CHECK_PTR(Rsum);
    for (int i = 0; i < N; i++)
    {
        qreal rowSum = 0;
        for (int j = 0; j < N; j++)
        {
            rowSum += WM.item(i, j);
        }
        Rsum[i] = rowSum;
    }

    WM.multiplyScalar(beta);
    WM.subtractFromI(); // WM now holds (I - beta*R)

    invM.resize(N, N);
    progressStatus(tr("Inverting (I - beta*R). Please wait..."));
    const bool invertible = invM.inverse(WM, [this] { return progressCanceled(); });

    if (progressCanceled())
    {
        delete[] Rsum;
        return;
    }

    if (!invertible)
    {
        qCDebug(lcCentrality) << "Graph::centralityBonacich() - (I - beta*R) is singular.";
        progressStatus(tr("Bonacich Power Centrality is not defined: (I - beta*R) is singular."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setBPC(0);
            (*it)->setSBPC(0);
        }
        calculatedBPC = true;
        delete[] Rsum;
        return;
    }

    // beta/alpha are only cached as "last known good" once the computation actually succeeds -
    // layoutByProminenceIndex() (WS11, #39) relies on this to know Bonacich is safe to re-run.
    m_lastBonacichAlpha = alpha;
    m_lastBonacichBeta = beta;

    // b = alpha * (I - beta*R)^-1 * R * 1 = alpha * invM * Rsum
    qreal *out = new (nothrow) qreal[N];
    Q_CHECK_PTR(out);
    invM.productByVector(Rsum, out, false);
    delete[] Rsum;

    qreal *BPC = new (nothrow) qreal[N];
    Q_CHECK_PTR(BPC);
    for (int i = 0; i < N; i++)
    {
        BPC[i] = alpha * out[i];
    }
    delete[] out;

    int i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            (*it)->setBPC(0);
            (*it)->setSBPC(0);
            continue;
        }

        (*it)->setBPC(BPC[i]);
        sumBPC += BPC[i];

        if (BPC[i] > maxBPC)
        {
            maxBPC = BPC[i];
            maxNodeBPC = (*it)->number();
        }
        if (BPC[i] < minBPC)
        {
            minBPC = BPC[i];
            minNodeBPC = (*it)->number();
        }

        i++;
    }

    delete[] BPC;

    meanBPC = sumBPC / static_cast<qreal>(N);

    qreal SBPC = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            continue;
        }

        SBPC = (maxBPC != 0) ? ((*it)->BPC() / maxBPC) : 0;
        (*it)->setSBPC(SBPC);

        resolveClasses(SBPC, discreteBPCs, classesBPC);
        varianceBPC += ((*it)->BPC() - meanBPC) * ((*it)->BPC() - meanBPC);
    }

    varianceBPC /= static_cast<qreal>(N);

    calculatedBPC = true;
}
