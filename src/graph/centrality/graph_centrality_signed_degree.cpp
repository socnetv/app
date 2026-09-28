/**
 * @file graph_centrality_signed_degree.cpp
 * @brief Implements Graph::centralitySignedDegree() - degree centrality split by tie sign, for
 *        signed networks (WS18 P3).
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
#include <QtConcurrent/QtConcurrent>

/**
 * @brief Calculates signed degree centrality: out-degree split by tie sign, in four variants.
 *
 * Meaning: on a signed network, a plain degree count conflates "liked by many" with "disliked by
 * many" into one number - this splits it back apart. pos is how many positive ties an actor
 * sends (or their summed strength, if weights are considered); neg is the same for negative
 * ties; ratio is pos / (pos+neg), the fraction of an actor's ties that are positive; net is
 * pos - neg, a single signed balance score.
 *
 * When to use: a fast first-pass screen for "who's positively vs. negatively prominent" on a
 * signed network, the same role plain degree centrality (centralityDegree()) plays on an
 * unsigned one - cheap, easy to explain, no attention paid to indirect/structural effects (see
 * centralityPN() for that).
 *
 * Weights: like centralityDegree(), no inversion choice (considerWeights only) - when
 * considered, a tie's absolute weight is summed into pos or neg according to its sign, so a
 * stronger tie always adds more regardless of which side it falls on.
 *
 * Compare to: centralityDegree() (DC), the unsigned equivalent this splits apart. Unlike DC,
 * this is out-degree only for now - an in-degree companion (mirroring prestigeDegree()/DP) is a
 * deliberately separate, later addition, not folded in here.
 *
 * Math: for vertex i, pos(i) = sum over positive-weight out-ties (or their count, if weights
 * aren't considered); neg(i) = sum over |negative-weight| out-ties (or their count); ratio(i) =
 * pos(i) / (pos(i)+neg(i)), 0 if the denominator is 0 (an isolate or all-neutral actor); net(i) =
 * pos(i) - neg(i). No standardized/graph-wide statistics (sum, mean, variance, classes) unlike
 * DC/SDC - not needed yet, can be added later if a real use case (e.g. a distribution chart)
 * calls for it.
 *
 * Parallelization: same shape as centralityDegree() (WS15 P4) - each vertex's four values only
 * read edges and write that vertex's own GraphVertex, independent across vertices, parallelized
 * via QtConcurrent::blockingMap.
 *
 * @param considerWeights
 * @param dropIsolates
 */
void Graph::centralitySignedDegree(const bool &considerWeights, const bool &dropIsolates)
{
    if (calculatedSignedDegree)
    {
        qCDebug(lcCentrality) << "Graph not changed - no need to recompute signed degree centralities. Returning.";
        return;
    }

    QString pMsg = tr("Computing Signed Degree Centralities. \nPlease wait...");
    qCDebug(lcCentrality) << pMsg;
    progressStatus(pMsg);

    if (progressCanceled())
    {
        return;
    }

    // Cancel signals cannot be delivered while graphThread's event loop is blocked in
    // blockingMap, so (like centralityDegree()) we skip the cancel check inside the lambda.
    QtConcurrent::blockingMap(m_graph, [&](GraphVertex *v) {
        if (!v->isEnabled() || (dropIsolates && v->isIsolated()))
        {
            return;
        }

        qreal pos = 0, neg = 0;
        qreal weight;

        for (VList::const_iterator it1 = m_graph.cbegin(); it1 != m_graph.cend(); ++it1)
        {
            if (!(*it1)->isEnabled() || (dropIsolates && (*it1)->isIsolated()))
            {
                continue;
            }

            if ((weight = edgeExists(v->number(), (*it1)->number())) != 0.0)
            {
                const qreal magnitude = considerWeights ? qAbs(weight) : 1.0;
                if (weight > 0)
                {
                    pos += magnitude;
                }
                else
                {
                    neg += magnitude;
                }
            }
        }

        v->setSignedDegreePos(pos);
        v->setSignedDegreeNeg(neg);
        v->setSignedDegreeRatio((pos + neg) != 0 ? pos / (pos + neg) : 0);
        v->setSignedDegreeNet(pos - neg);
    });

    calculatedSignedDegree = true;
}
