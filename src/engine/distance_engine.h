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
#include "engine/per_source_scratch.h"

#include <QVector>

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

    // Parallel SSSP source loop (Phase 2).
    // Distributes source vertices across CPU cores via QtConcurrent::blockingMap.
    // Each thread owns a ThreadLocalState; unsafe graph writes (BC, SC, distance
    // sum, geodesics count, diameter) are accumulated into per-thread state and
    // reduced into graph-global state in a single-threaded step after the map.
    void runAllSources(const bool computeCentralities,
                       const bool considerWeights,
                       const bool inverseWeights,
                       const bool dropIsolates,
                       struct DistanceScratch &ds,
                       IDistanceProgressSink &sink);

    void finalize(const bool computeCentralities,
                  const bool dropIsolates,
                  struct DistanceScratch &ds,
                  struct CentralityScratchFinalize &csfin,
                  IDistanceProgressSink &sink);

    // Breadth-First Search SSSP for unweighted graphs.
    // Writes distances and sigma to pss; accumulates unsafe graph-wide values into
    // pss.sourceDistanceSum / sourceGeodesicsCount / sourceDiameter instead of calling
    // graph methods directly (safe for parallel execution from multiple threads).
    // SC increments go into partialSC[ui] rather than vertex->setSC() to avoid races
    // on intermediate vertices that may be visited by concurrent source threads.
    void bfsSSSP(const int &s, const int &si,
                 const bool &computeCentralities,
                 const bool &dropIsolates,
                 PerSourceScratch &pss,
                 QVector<qreal> &partialSC);

    // Dijkstra SSSP for weighted graphs (directed or not).
    // Same thread-safety contract as bfsSSSP: unsafe graph-wide writes go to
    // pss scratch fields and partialSC instead of touching graph/vertex state directly.
    void dijkstraSSSP(const int &s, const int &si,
                      const bool &computeCentralities,
                      const bool &inverseWeights,
                      const bool &dropIsolates,
                      PerSourceScratch &pss,
                      QVector<qreal> &partialSC);
};

#endif // SOCNETV_DISTANCE_ENGINE_H
