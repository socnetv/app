/**
 * @file graph_node_filters.cpp
 * @brief Implements vertex filtering functions for the Graph class.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "graph.h"
#include <QDebug>

/**
 * @brief Filters vertices by their score on the given centrality or prestige index.
 *
 * Performs two passes over the graph:
 *
 * - Pass 1: Enables or disables each vertex depending on whether its score
 *   satisfies the threshold condition. Emits setVertexVisibility for every
 *   vertex whose state changes.
 *
 * - Pass 2: Updates all edges. An edge is made visible only when both its
 *   source and target vertices are enabled after pass 1. Emits
 *   signalSetEdgeVisibility for every out-edge of every vertex.
 *
 * The two-pass design is required so that edge visibility can be determined
 * correctly: during pass 2 all vertices are already in their final enabled
 * state, so both endpoints of any edge can be queried reliably.
 *
 * Calling this function with a threshold that no vertex satisfies (e.g.
 * DC >= 999999) effectively re-enables all vertices and their edges,
 * serving as the undo mechanism.
 *
 * Returns early with a status message if the requested index has not been
 * computed yet — the user should run the corresponding analysis first.
 *
 * @param threshold        Score threshold to compare against.
 * @param overThreshold    If true, disable vertices with score >= threshold;
 *                         if false, disable vertices with score <= threshold.
 * @param centralityIndex  The centrality or prestige index to use, as defined
 *                         by the IndexType enum in global.h. Must match an
 *                         entry in prominenceIndexList (1-based).
 *
 * @see isCentralityIndexComputed()
 * @see IndexType
 */
void Graph::vertexFilterByCentrality(const float threshold,
                                     const bool overThreshold,
                                     const IndexType centralityIndex)
{
    qDebug() << "Graph::vertexFilterByCentrality()"
             << "index:" << static_cast<int>(centralityIndex)
             << "threshold:" << threshold
             << "overThreshold:" << overThreshold;

    if (!isCentralityIndexComputed(centralityIndex))
    {
        qDebug() << "Graph::vertexFilterByCentrality() - "
                    "index"
                 << static_cast<int>(centralityIndex)
                 << "not yet computed. Aborting.";
        progressStatus(
            tr("Please compute the selected centrality/prestige index first, "
               "then apply the filter."));
        return;
    }

    const QString condition = overThreshold ? QStringLiteral(">=") : QStringLiteral("<=");
    const qreal thresh = static_cast<qreal>(threshold);

    // ---------------------------------------------------------------
    // PASS 1: Enable / disable vertices only, no edge changes yet.
    // After this pass every vertex is in its correct enabled state,
    // so the edge pass can safely query both endpoints.
    // ---------------------------------------------------------------
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        qreal score = 0.0;
        switch (centralityIndex)
        {
        case IndexType::DC:
            score = (*it)->DC();
            break;
        case IndexType::CC:
            score = (*it)->CC();
            break;
        case IndexType::IRCC:
            score = (*it)->IRCC();
            break;
        case IndexType::BC:
            score = (*it)->BC();
            break;
        case IndexType::SC:
            score = (*it)->SC();
            break;
        case IndexType::EC:
            score = (*it)->EC();
            break;
        case IndexType::PC:
            score = (*it)->PC();
            break;
        case IndexType::IC:
            score = (*it)->IC();
            break;
        case IndexType::EVC:
            score = (*it)->EVC();
            break;
        case IndexType::DP:
            score = (*it)->DP();
            break;
        case IndexType::PRP:
            score = (*it)->PRP();
            break;
        case IndexType::PP:
            score = (*it)->PP();
            break;
        default:
            score = (*it)->DC();
            break;
        }

        const bool shouldDisable = overThreshold ? (score >= thresh) : (score <= thresh);
        const bool currentlyEnabled = (*it)->isEnabled();

        qDebug() << "PASS 1 - vertex" << (*it)->number()
                 << "score=" << score
                 << condition << threshold
                 << "shouldDisable:" << shouldDisable
                 << "currentlyEnabled:" << currentlyEnabled;

        if (currentlyEnabled == !shouldDisable)
            continue; // already in correct state

        (*it)->setEnabled(!shouldDisable);
        setModStatus(ModStatus::VertexCount);
        emit setVertexVisibility((*it)->number(), !shouldDisable);
    }

    // PASS 2 — Update edge visibility.
    // Mirrors edgeFilterByWeight's proven re-enable pattern exactly.
    // An edge is visible only when both endpoints are enabled.
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        const int v = (*it)->number();

        H_edges::iterator ed;
        for (ed = (*it)->m_outEdges.begin(); ed != (*it)->m_outEdges.end(); ++ed)
        {
            if (ed.value().first != m_curRelation)
                continue;

            const int target = ed.key();
            const qreal weight = ed.value().second.first;
            const qreal reverseEdgeWeight = (*it)->hasEdgeFrom(target);
            const bool preserveReverse = (reverseEdgeWeight != 0);

            const bool sourceEnabled = (*it)->isEnabled();
            const bool targetEnabled = vertexAtIndex(vertexIndexByNumber(target))->isEnabled();
            const bool edgeVisible = sourceEnabled && targetEnabled;

            qDebug() << "PASS 2 - edge" << v << "->" << target
                     << "sourceEnabled:" << sourceEnabled
                     << "targetEnabled:" << targetEnabled
                     << "edgeVisible:" << edgeVisible;

            // Update out-edge storage
            ed.value() = pair_i_fb(m_curRelation, pair_f_b(weight, edgeVisible));
            // Update in-edge bookkeeping on target
            edgeInboundStatusSet(target, v, edgeVisible);

            if (edgeVisible)
            {
                // Re-enable: use the same short form as edgeFilterByWeight
                emit signalSetEdgeVisibility(m_curRelation, v, target,
                                             true, preserveReverse);
            }
            else
            {
                // Disable: pass weights so GW can decide on arc rendering
                emit signalSetEdgeVisibility(m_curRelation, v, target,
                                             false, preserveReverse,
                                             weight, reverseEdgeWeight);
            }
        }
    }

    progressStatus(
        tr("Filter applied: vertices with score %1 %2 are now hidden.")
            .arg(condition, QString::number(threshold)));
}