/**
 * @file graph_centrality_pn.cpp
 * @brief Implements PN Centrality (Everett & Borgatti 2014) for the Graph class.
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
 * @brief Computes PN Centrality (Everett & Borgatti 2014) of each vertex.
 *
 * Meaning: a purpose-built centrality measure for signed networks - unlike re-running an
 * unsigned measure on |weight|, this is defined directly in terms of tie sign. The core idea:
 * receiving a negative tie from someone who is themselves highly (positively) prominent hurts
 * you more than receiving one from someone marginalized - conversely, a negative tie from
 * someone universally disliked can even read as a positive signal. PN captures this by weighting
 * each negative tie by the sender's own positive standing, propagated through indirect
 * connections the same way Katz Centrality propagates ordinary ties.
 *
 * When to use: the standard choice for centrality on a signed network, when re-computing an
 * unsigned measure on tie magnitude alone would throw away the sign information that's the whole
 * point of the network being signed in the first place.
 *
 * Weights: strictly binary (tie sign only) - matches the confirmed reference formula exactly.
 * Tie magnitude is deliberately discarded, even on a weighted network; there is no
 * considerWeights parameter, unlike every other centrality measure in this codebase.
 *
 * Compare to: Katz Centrality (KC, see centralityKatz()) - PN's All-mode formula is the exact
 * same closed-form "geometric series of walks" identity Katz uses
 * (C_Katz = ((I - alpha*A^T)^-1 - I) * 1), just built from the signed matrix A = P - 2N with a
 * fixed beta instead of a user-tunable alpha, and without the "-1" term. The Out/In directed
 * forms are a different, more involved closed form specific to this measure, not a simple Katz
 * analogue.
 *
 * Math: A = P - 2N (see createMatrixSignedPN()), beta = 1/(2n-2). Mode All (undirected only):
 * PN = rowSum((I - beta*A)^-1). Mode Out (directed): PN = rowSum((I - beta^2*A*A^T)^-1 *
 * (I + beta*A)). Mode In (directed): PN = rowSum((I - beta^2*A^T*A)^-1 * (I + beta*A^T)) - the
 * transpose-symmetric counterpart of Out. An undirected graph is only ever valid with All; a
 * directed graph must pick Out or In.
 *
 * @param mode All (undirected graphs only), Out or In (directed graphs only).
 * @param dropIsolates
 */
void Graph::centralityPN(const PNMode mode, const bool &dropIsolates)
{
    if (calculatedPN && m_lastPNMode == mode)
    {
        qCDebug(lcCentrality) << "Graph not changed and mode unchanged - PN already computed. Return.";
        return;
    }

    qCDebug(lcCentrality) << "(Re)Computing PN Centrality scores... mode ="
             << static_cast<int>(mode);

    progressStatus(tr("Calculating PN Centrality scores..."));

    VList::const_iterator it;

    const int N = vertices(dropIsolates);

    if (N == 0)
    {
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setPN(0);
        }
        calculatedPN = true;
        m_lastPNMode = mode;
        return;
    }

    if (isDirected() && mode == PNMode::All)
    {
        qCDebug(lcCentrality) << "Graph::centralityPN() - mode All is only valid on an "
                                  "undirected graph. Aborting.";
        progressStatus(tr("PN Centrality mode \"All\" is only valid on an undirected network - "
                          "pick Out or In for a directed network."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setPN(0);
        }
        calculatedPN = true;
        m_lastPNMode = mode;
        return;
    }

    if (!isDirected() && mode != PNMode::All)
    {
        qCDebug(lcCentrality) << "Graph::centralityPN() - Out/In are only valid on a directed "
                                  "graph. Aborting.";
        progressStatus(tr("PN Centrality mode \"Out\"/\"In\" is only valid on a directed "
                          "network - pick All for an undirected network."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setPN(0);
        }
        calculatedPN = true;
        m_lastPNMode = mode;
        return;
    }

    createMatrixSignedPN(dropIsolates);
    if (progressCanceled())
    {
        return;
    }

    const qreal beta = 1.0 / (2.0 * N - 2.0);

    progressStatus(tr("Computing PN Centrality scores. \nPlease wait..."));

    Matrix identity;
    identity.identityMatrix(N);

    Matrix solved;
    bool invertible = true;

    if (mode == PNMode::All)
    {
        WM.resize(N, N);
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                WM.setItem(i, j, PNM.item(i, j));
            }
        }
        WM.multiplyScalar(beta);
        WM.subtractFromI();

        invM.resize(N, N);
        invertible = invM.inverse(WM, [this] { return progressCanceled(); });
        solved = invM;
    }
    else
    {
        Matrix &AT = PNM.transpose();
        const qreal beta2 = beta * beta;

        Matrix AAt;
        if (mode == PNMode::Out)
        {
            AAt.product(PNM, AT);
        }
        else
        {
            AAt.product(AT, PNM);
        }

        WM.resize(N, N);
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                WM.setItem(i, j, AAt.item(i, j));
            }
        }
        WM.multiplyScalar(beta2);
        WM.subtractFromI();

        invM.resize(N, N);
        invertible = invM.inverse(WM, [this] { return progressCanceled(); });

        Matrix rhs(N, N);
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                rhs.setItem(i, j, (mode == PNMode::Out ? PNM.item(i, j) : AT.item(i, j)));
            }
        }
        rhs.multiplyScalar(beta);
        rhs += identity;

        delete &AT;

        solved = invM * rhs;
    }

    if (progressCanceled())
    {
        return;
    }

    if (!invertible)
    {
        qCDebug(lcCentrality) << "Graph::centralityPN() - matrix is singular.";
        progressStatus(tr("PN Centrality is not defined for this network: the underlying matrix "
                          "is singular."));
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->setPN(0);
        }
        calculatedPN = true;
        m_lastPNMode = mode;
        return;
    }

    m_lastPNMode = mode;

    qreal *PN = new (nothrow) qreal[N];
    Q_CHECK_PTR(PN);
    for (int i = 0; i < N; i++)
    {
        qreal rowSum = 0;
        for (int j = 0; j < N; j++)
        {
            rowSum += solved.item(i, j);
        }
        PN[i] = rowSum;
    }

    int i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->isIsolated() && dropIsolates)
        {
            (*it)->setPN(0);
            continue;
        }

        (*it)->setPN(PN[i]);
        i++;
    }

    delete[] PN;

    calculatedPN = true;
}
