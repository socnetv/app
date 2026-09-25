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
                 const bool allowNegativeWeights = false);

    // Public probe for the potentials pass - see distance_engine.cpp for the doc comment.
    bool bellmanFordPotentials(const bool inverseWeights, QVector<qreal> &outPotentials);

private:
    Graph &graph;

    /**
     * @brief Phase 0 of compute(): resets every scratch/aggregate field this run will populate,
     * scans for a negative edge weight up front (refusing the whole computation via
     * sink.reportNegativeWeights() unless allowNegativeWeights is set), and handles the
     * zero-edges (E==0) case entirely on its own since runAllSources() has nothing to do then.
     * @param computeCentralities Whether centrality scratch/aggregate fields need resetting too.
     * @param considerWeights Whether the negative-weight scan below runs at all (BFS never sees
     * weights, so there is nothing to detect).
     * @param inverseWeights Only used for the E==0 branch's own bookkeeping.
     * @param dropIsolates Exclude isolated vertices from the vertex count/E==0 population.
     * @param allowNegativeWeights If true, a detected negative edge weight is not refused here -
     * the caller (compute()) has opted into the negative-weight-safe (Johnson's-algorithm) path,
     * which is defined for negative weights.
     * @param ds Output: scratch state for this run (sizes, maxima, per-run accumulators).
     * @param csssp Output: SSSP-phase centrality scratch, zeroed for this run.
     * @param csfin Output: finalize-phase centrality scratch, zeroed for this run.
     * @param sink Progress/cancellation/negative-weight-refusal callback.
     */
    void initRun(const bool computeCentralities,
                 const bool considerWeights,
                 const bool inverseWeights,
                 const bool dropIsolates,
                 const bool allowNegativeWeights,
                 struct DistanceScratch &ds,
                 struct CentralityScratchSSSP &csssp,
                 struct CentralityScratchFinalize &csfin,
                 IDistanceProgressSink &sink);

    // Bellman-Ford reweighting pass - see distance_engine.cpp for the doc comment.
    bool bellmanFordPotentials(const bool inverseWeights, struct DistanceScratch &ds);

    /**
     * @brief Runs SSSP (BFS or Dijkstra, per considerWeights) from every enabled vertex, in
     * parallel across CPU cores via QtConcurrent::blockingMap. Each worker thread owns its own
     * ThreadLocalState; graph-wide writes that aren't safe to make concurrently (BC, SC,
     * distance sum, geodesics count, diameter) are accumulated into per-thread state during the
     * map and reduced into graph-global state in a single-threaded step immediately after.
     * @param computeCentralities Also accumulate BC/SC/CC/etc. per source, not just distances.
     * @param considerWeights Dijkstra (true) vs. BFS (false).
     * @param inverseWeights Use 1/weight as the per-edge distance metric.
     * @param dropIsolates Exclude isolated vertices from the source loop.
     * @param ds Scratch state populated by initRun() (potentials, if any, bounds, etc.).
     * @param sink Progress/cancellation callback.
     */
    void runAllSources(const bool computeCentralities,
                       const bool considerWeights,
                       const bool inverseWeights,
                       const bool dropIsolates,
                       struct DistanceScratch &ds,
                       IDistanceProgressSink &sink);

    /**
     * @brief Single-threaded aggregation pass run after runAllSources() completes: scans for
     * vertex pairs left unreachable (populating notConnectedPairs and the infinite-eccentricity/
     * zero-centrality bookkeeping that implies), determines overall graph connectedness, and
     * finishes the graph-wide centrality aggregates (sums, min/max, normalized forms) that
     * runAllSources() only partially accumulated per-source.
     * @param computeCentralities Whether centrality aggregates need finishing at all.
     * @param dropIsolates Exclude isolated vertices from the connectivity/aggregate scan.
     * @param ds Scratch state carried over from initRun()/runAllSources().
     * @param csfin Centrality-aggregation scratch (sums, min/max trackers) to finish into graph
     * state.
     * @param sink Progress/cancellation callback.
     */
    void finalize(const bool computeCentralities,
                  const bool dropIsolates,
                  struct DistanceScratch &ds,
                  struct CentralityScratchFinalize &csfin,
                  IDistanceProgressSink &sink);

    // Breadth-First Search SSSP for unweighted graphs.
    // Writes distances, sigma, and Ps (predecessor lists) to pss. Graph-wide aggregates
    // (distance sum, geodesics count, diameter) and centrality accumulators that depend on the
    // final settled shortest-path DAG (BC, SC) are computed post-hoc from pss.Ps/pss.sigma in
    // runAllSources()'s Brandes back-propagation loop, not accumulated here.
    void bfsSSSP(const int &s, const int &si,
                 const bool &computeCentralities,
                 const bool &dropIsolates,
                 PerSourceScratch &pss);

    // Dijkstra SSSP for weighted graphs (directed or not).
    // Same contract as bfsSSSP: unsafe graph-wide writes and DAG-dependent centralities go
    // through pss scratch fields, not touched directly here.
    // potentials: empty for a plain Dijkstra run (the default); when non-empty, indexed by
    // vertex position like pss.dist, each edge weight is reweighted inline as
    // weight + potentials[ui] - potentials[wi] before relaxation (Johnson's algorithm - see
    // bellmanFordPotentials()). Caller is responsible for un-reweighting pss.dist afterward.
    void dijkstraSSSP(const int &s, const int &si,
                      const bool &computeCentralities,
                      const bool &inverseWeights,
                      const bool &dropIsolates,
                      PerSourceScratch &pss,
                      const QVector<qreal> &potentials = QVector<qreal>());
};

#endif // SOCNETV_DISTANCE_ENGINE_H
