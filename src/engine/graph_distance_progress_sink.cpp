/**
 * @file graph_distance_progress_sink.cpp
 * @brief Implements the GraphDistanceProgressSink class, which forwards DistanceEngine's status messages and cancellation state to Graph.
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

#include "graph_distance_progress_sink.h"
#include "../graph.h" // adjust include if you prefer "graph.h" from include paths

GraphDistanceProgressSink::GraphDistanceProgressSink(Graph &g)
    : graph(g)
{
}

void GraphDistanceProgressSink::statusMessage(const QString &msg)
{
    emit graph.statusMessage(msg);
}

/**
 * @brief Resets the cancellation flag at the start of a distance computation.
 */
void GraphDistanceProgressSink::resetCancellation()
{
    graph.resetProgressCanceled();
}

bool GraphDistanceProgressSink::progressCanceled() const
{
    return graph.progressCanceled();
}
