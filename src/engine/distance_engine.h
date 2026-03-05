/**
 * @file distance_engine.h
 * @brief Declares the DistanceEngine class for computing geodesic distances and centralities in the graph.
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

#ifndef SOCNETV_DISTANCE_ENGINE_H
#define SOCNETV_DISTANCE_ENGINE_H

#include "engine/graph_distance_progress_sink.h"

class Graph;

class DistanceEngine
{
public:
    explicit DistanceEngine(Graph &g);
    void compute(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates);

private:
    Graph &graph;

    void initRun(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates,
                 struct DistanceScratch &ds,
                 struct CentralityScratchSSSP &csssp,
                 struct CentralityScratchFinalize &csfin,
                 IDistanceProgressSink &sink);

    void runAllSources(const bool computeCentralities,
                       const bool considerWeights,
                       const bool inverseWeights,
                       const bool dropIsolates,
                       struct DistanceScratch &ds,
                       struct CentralityScratchSSSP &csssp,
                       IDistanceProgressSink &sink);

    void finalize(const bool computeCentralities,
                  const bool dropIsolates,
                  struct DistanceScratch &ds,
                  struct CentralityScratchFinalize &csfin,
                  IDistanceProgressSink &sink);

    void bfsSSSP(const int &s, const int &si, const bool &computeCentralities,
                 const bool &dropIsolates);

    void dijkstraSSSP(const int &s, const int &si,
                      const bool &computeCentralities,
                      const bool &inverseWeights,
                      const bool &dropIsolates);
};

#endif // SOCNETV_DISTANCE_ENGINE_H
