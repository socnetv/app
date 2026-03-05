/**
 * @file graph_distance_progress_sink.h
 * @brief Declares the GraphDistanceProgressSink class that implements IDistanceProgressSink to receive progress updates from the DistanceEngine and forward them to the Graph for UI updates.
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

#pragma once

#include "distance_progress_sink.h"

class Graph; // forward declaration

class GraphDistanceProgressSink final : public IDistanceProgressSink
{
public:
    explicit GraphDistanceProgressSink(Graph &g);

    void statusMessage(const QString &msg) override;
    void progressCreate(int total, const QString &msg) override;
    void progressUpdate(int value) override;
    void progressKill() override;
    bool progressCanceled() const override;

private:
    Graph &graph;
};
