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
                 const bool dropIsolates,
                 const bool negativeWeightSafe = false);

    // Public probe for the potentials pass - see distance_engine.cpp for the doc comment.
    bool bellmanFordPotentials(const bool inverseWeights, QVector<qreal> &outPotentials);

private:
    Graph &graph;

    void initRun(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates,
                 const bool negativeWeightSafe,
                 struct DistanceScratch &ds,
                 struct CentralityScratchSSSP &csssp,
                 struct CentralityScratchFinalize &csfin,
                 IDistanceProgressSink &sink);

    // Bellman-Ford reweighting pass - see distance_engine.cpp for the doc comment.
    bool bellmanFordPotentials(const bool inverseWeights, struct DistanceScratch &ds);

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
    // Writes distances and sigma to pss. Graph-wide aggregates (distance sum, geodesics
    // count, diameter) are computed post-hoc from the final settled state, not accumulated
    // here.
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
    // potentials: empty for a plain Dijkstra run (the default); when non-empty, indexed by
    // vertex position like pss.dist, each edge weight is reweighted inline as
    // weight + potentials[ui] - potentials[wi] before relaxation (Johnson's algorithm - see
    // bellmanFordPotentials()). Caller is responsible for un-reweighting pss.dist afterward.
    void dijkstraSSSP(const int &s, const int &si,
                      const bool &computeCentralities,
                      const bool &inverseWeights,
                      const bool &dropIsolates,
                      PerSourceScratch &pss,
                      QVector<qreal> &partialSC,
                      const QVector<qreal> &potentials = QVector<qreal>());
};

#endif // SOCNETV_DISTANCE_ENGINE_H
