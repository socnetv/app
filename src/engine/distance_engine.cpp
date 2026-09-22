/**
 * @file distance_engine.cpp
 * @brief Implements the DistanceEngine class for computing geodesic distances and centralities in the graph.
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

#include "engine/distance_engine.h"

#include "graph.h"
#include "engine/graph_distance_progress_sink.h"
#include "engine/thread_local_state.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QMutex>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <queue>

Q_LOGGING_CATEGORY(lcEngine, "socnetv.engine")

namespace {

// Relative-tolerance comparison for two accumulated path-length sums. Plain qreal == is fragile
// once a distance is a sum of several independently-rounded edge weights - two paths that are
// mathematically tied can land a bit or two apart after summation, especially once WS18's
// Johnson's-reweighting path composes an extra per-edge potential term into every weight before
// this comparison runs. eps is fixed and relative to the larger operand's magnitude, so it scales
// sensibly across the wide range of edge-weight magnitudes real SNA datasets use, rather than
// being too loose for small weights or too tight for large ones.
bool distancesNearlyEqual(qreal a, qreal b)
{
    constexpr qreal eps = 1e-9;
    return std::abs(a - b) <= eps * std::max({qreal(1.0), std::abs(a), std::abs(b)});
}

} // namespace

/**
 * @brief Per-run scratch state for DistanceEngine::compute(), scoped to one compute() call.
 *
 * Not per-source (see PerSourceScratch/ThreadLocalState in per_source_scratch.h /
 * thread_local_state.h for that) — this is the state threaded through compute()'s three
 * phases (initRun / runAllSources / finalize) that used to be a long list of local variables
 * before WS1's Phase C extraction. One instance lives on compute()'s stack for the whole call.
 */
struct DistanceScratch
{
    // Iterators (kept exactly as locals were)
    VList::const_iterator it;
    VList::const_iterator it1;
    QList<int>::const_iterator it2;

    // Indices / counters
    int w = 0, u = 0, s = 0, si = 0, ui = 0, wi = 0;
    int progressCounter = 0;

    // Graph size snapshot
    int N = 0;
    int E = 0;

    // UI message
    QString pMsg;

    // Scratch scalars used across phases
    qreal distances_sum_for_s = 0;
    qreal maxEdgeWeightInNetwork = 0;
    qreal tempEdgeWeight = 0;

    // Used during finalize/connectivity scan
    qreal pairDistance = 0;

    // Johnson's-algorithm potentials, one entry per vertex position (same indexing as
    // PerSourceScratch::dist), computed once per compute() call by bellmanFordPotentials() and
    // shared read-only across every parallel per-source Dijkstra call. Empty/unused until
    // this reweighting is actually wired into dijkstraSSSP() - for now this is only computed
    // and validated, not consumed yet.
    QVector<qreal> potentials;
    bool negativeCycleDetected = false;
};

/**
 * @brief Per-run scratch for centrality values computed once per SSSP source, before the
 * parallel per-source loop existed to own its own copy (see ThreadLocalState::pss for the
 * per-thread equivalent used inside runAllSources()'s blockingMap lambda).
 *
 * Retained as a distinct type from CentralityScratchFinalize because these fields are
 * meaningful only during the single-source SSSP pass itself, not the finalize/aggregation
 * phase that runs once after all sources are done.
 */
struct CentralityScratchSSSP
{
    // Per-source values computed inside the SSSP loop
    qreal CC = 0;
    qreal PC = 0;
    qreal SPC = 0;

    // Brandes / dependency scratch
    qreal sigma_u = 0;
    qreal sigma_w = 0;
    qreal delta_u = 0;
    qreal delta_w = 0;
    qreal d_sw = 0;
    qreal d_su = 0;

    // Power Centrality iterator
    H_f_i::const_iterator hfi;
};

/**
 * @brief Scratch for the finalize() phase — the single-threaded pass that runs once after
 * runAllSources() completes, scanning every vertex to compute connectivity, group-level
 * aggregates, and normalised (standardised) centrality scores from the raw per-vertex values
 * runAllSources() already wrote. Distinct from CentralityScratchSSSP because these values are
 * only meaningful for this one post-loop scan, not during the per-source SSSP pass itself.
 */
struct CentralityScratchFinalize
{
    // Values used while scanning vertices and aggregating
    qreal CC = 0, BC = 0, SC = 0, eccentricity = 0, EC = 0;
    qreal SCC = 0, SBC = 0, SSC = 0, SEC = 0, SPC = 0;

    // Variance temps used in the final aggregation loop(s)
    qreal tempVarianceBC = 0, tempVarianceSC = 0, tempVarianceEC = 0;
    qreal tempVarianceCC = 0, tempVariancePC = 0;
};

DistanceEngine::DistanceEngine(Graph &g)
    : graph(g)
{
}

/**
 * @brief Runs the full geodesic distance (and optionally centrality) computation pipeline.
 *
 * Orchestrates three phases:
 * - Phase 0 (Init): initialises scratch structures, resets aggregates,
 *   handles the degenerate E==0 case.
 * - Phase 1+2 (SSSP loop): runs BFS or Dijkstra from every source vertex,
 *   accumulating per-source distance and centrality data.
 * - Phase 3 (Finalize): connectivity scan, group-level aggregation,
 *   normalisation of centrality scores.
 *
 * If the user cancels via the progress dialog, the function returns early
 * after Phase 1+2 without finalising, and @c calculatedDistances is left
 * @c false so the next call recomputes from scratch.
 *
 * @param computeCentralities  If true, also computes BC, CC, SC, EC, PC.
 * @param considerWeights      If true, uses edge weights (Dijkstra); otherwise BFS.
 * @param inverseWeights       If true, uses 1/weight as the distance metric.
 * @param dropIsolates         If true, excludes isolated vertices from all calculations.
 * @param negativeWeightSafe   If true, negative edge weights are not refused - instead, potentials
 * are computed via bellmanFordPotentials() (Johnson's algorithm) and every source's Dijkstra run
 * is reweighted to be non-negative. Refuses instead if the network has a reachable negative cycle
 * (see Graph::negativeCycleDetected()), since shortest paths are then undefined regardless of
 * algorithm. Has no effect unless considerWeights is also true.
 */
void DistanceEngine::compute(const bool computeCentralities,
                             const bool considerWeights,
                             const bool inverseWeights,
                             const bool dropIsolates,
                             const bool negativeWeightSafe)
{

    qCDebug(lcEngine) << "DistanceEngine::compute() - "
             << "centralities" << computeCentralities
             << "considerWeights:" << considerWeights
             << "inverseWeights:" << inverseWeights
             << "dropIsolates:" << dropIsolates
             << "negativeWeightSafe:" << negativeWeightSafe;

    if (computeCentralities)
    {
        if (graph.calculatedCentralities)
        {
            return;
        }
    }
    else if (graph.calculatedDistances)
    {
        return;
    }

    DistanceScratch ds;
    CentralityScratchSSSP csssp;
    CentralityScratchFinalize csfin;

    // Routes progress/status/cancel notifications back to graph's Qt signals (see
    // graph_distance_progress_sink.h) - keeps this file free of any direct Qt-signal
    // dependency of its own.
    GraphDistanceProgressSink sink(graph);

    // ---- Phase 0/Init (includes E==0 handling) ----
    initRun(computeCentralities,
            considerWeights,
            inverseWeights,
            dropIsolates,
            negativeWeightSafe,
            ds,
            csssp,
            csfin,
            sink);

    if (!negativeWeightSafe && graph.negativeWeightsDetected())
    {
        qCDebug(lcEngine) << "DistanceEngine::compute() - refused: negative edge weight(s) "
                              "detected, Dijkstra is undefined for those. Skipping computation.";
        return;
    }

    if (ds.E != 0)
    {
        // negativeWeightSafe: compute potentials once, single-threaded, before any per-source
        // work starts - every parallel Dijkstra call in runAllSources() needs to read the same
        // frozen h(v) vector. A reachable negative cycle makes shortest paths undefined for any
        // algorithm, so refuse the whole computation rather than a partial/best-effort result.
        if (negativeWeightSafe && considerWeights)
        {
            graph.resetNegativeCycleDetected();
            if (!bellmanFordPotentials(inverseWeights, ds))
            {
                graph.setNegativeCycleDetected();
                qCDebug(lcEngine) << "DistanceEngine::compute() - refused: reachable negative "
                                      "cycle detected, shortest paths are undefined. Skipping "
                                      "computation.";
                return;
            }
        }

        // ---- Phase 1+2: SSSP loop + per-source accumulation ----
        runAllSources(computeCentralities,
                      considerWeights,
                      inverseWeights,
                      dropIsolates,
                      ds,
                      sink);
        if (sink.progressCanceled())
        {
            qCDebug(lcEngine) << "DistanceEngine::compute() - canceled. Skipping finalize.";
            return;
        }
        // ---- Finalization: connectivity scan + aggregation ----
        finalize(computeCentralities,
                 dropIsolates,
                 ds,
                 csfin,
                 sink);
    }

    graph.calculatedDistances = true;

    qCDebug(lcEngine) << "Graph::graphDistancesGeodesic()- FINISHED computing distances";
}

void DistanceEngine::initRun(const bool computeCentralities,
                             const bool considerWeights,
                             const bool inverseWeights,
                             const bool dropIsolates,
                             const bool negativeWeightSafe,
                             DistanceScratch &ds,
                             CentralityScratchSSSP &csssp,
                             CentralityScratchFinalize &csfin,
                             IDistanceProgressSink &sink)
{
    // drop isolated vertices from calculations (i.e. std C and group C).
    ds.N = graph.vertices(dropIsolates, false, true);
    ds.E = graph.edgesEnabled();

    ds.pMsg = QObject::tr("Computing geodesic distances. \nPlease wait...");

    sink.statusMessage(ds.pMsg);
    sink.resetCancellation();
    graph.resetNegativeWeightsDetected();

    graph.setSymmetricCached(graph.isSymmetric());

    if (ds.E == 0)
    {
        // All pairs unreachable. E==0 means no source loop runs (compute() skips
        // runAllSources() entirely for E==0), so this is the only place that needs to
        // populate m_apspDist/m_apspSigma for this case.
        int totalV = 0;
        for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
            ++totalV;
        const int relation = graph.relationCurrent();
        graph.m_apspDist[relation].resize(totalV, totalV);
        graph.m_apspSigma[relation].resize(totalV, totalV);
        graph.m_apspDist[relation].fillMatrix(RAND_MAX);
        // Sigma stays at 0 - Matrix::resize() zero-initializes every cell already.

        if (ds.N < 2)
        {
            // singleton graph consisting of a single isolated node
            // is considered connected
            graph.setConnectedCached(true);
        }
        else
        {
            // any non-empty and non-singleton graph with zero edges is disconnected
            graph.setConnectedCached(false);
        }
    }
    else
    {
        ds.distances_sum_for_s = 0;
        ds.maxEdgeWeightInNetwork = 0;
        ds.tempEdgeWeight = 0;

        // ---- SSSP scratch ----
        csssp.CC = 0;
        csssp.PC = 0;
        csssp.SPC = 0;

        csssp.sigma_u = 0;
        csssp.sigma_w = 0;
        csssp.delta_u = 0;
        csssp.delta_w = 0;
        csssp.d_sw = 0;
        csssp.d_su = 0;

        // ---- Finalize scratch ----
        csfin.CC = 0;
        csfin.BC = 0;
        csfin.SC = 0;
        csfin.eccentricity = 0;
        csfin.EC = 0;

        csfin.SCC = 0;
        csfin.SBC = 0;
        csfin.SSC = 0;
        csfin.SEC = 0;
        csfin.SPC = 0;

        csfin.tempVarianceBC = 0;
        csfin.tempVarianceSC = 0;
        csfin.tempVarianceEC = 0;
        csfin.tempVarianceCC = 0;
        csfin.tempVariancePC = 0;

        ds.pairDistance = 0;

        graph.maxSCC = 0;
        graph.minSCC = RAND_MAX;
        graph.nomSCC = 0;
        graph.denomSCC = 0;
        graph.groupCC = 0;
        graph.maxNodeSCC = 0;
        graph.minNodeSCC = 0;
        graph.sumSCC = 0;
        graph.sumCC = 0;
        graph.discreteCCs.clear();
        graph.classesSCC = 0;

        graph.maxSBC = 0;
        graph.minSBC = RAND_MAX;
        graph.nomSBC = 0;
        graph.denomSBC = 0;
        graph.groupSBC = 0;
        graph.maxNodeSBC = 0;
        graph.minNodeSBC = 0;
        graph.sumBC = 0;
        graph.sumSBC = 0;
        graph.discreteBCs.clear();
        graph.classesSBC = 0;

        graph.maxSSC = 0;
        graph.minSSC = RAND_MAX;
        graph.groupSC = 0;
        graph.maxNodeSSC = 0;
        graph.minNodeSSC = 0;
        graph.sumSC = 0;
        graph.sumSSC = 0;
        graph.discreteSCs.clear();
        graph.classesSSC = 0;

        graph.maxSPC = 0;
        graph.minSPC = RAND_MAX;
        graph.nomSPC = 0;
        graph.denomSPC = 0;
        graph.groupSPC = 0;
        graph.maxNodeSPC = 0;
        graph.minNodeSPC = 0;
        graph.sumSPC = 0;
        graph.sumPC = 0;
        graph.discretePCs.clear();
        graph.classesSPC = 0;

        graph.maxEccentricity = 0;
        graph.minEccentricity = RAND_MAX;
        graph.maxNodeEccentricity = 0;
        graph.minNodeEccentricity = 0;
        graph.discreteEccentricities.clear();
        graph.classesEccentricity = 0;

        graph.maxEC = 0;
        graph.minEC = RAND_MAX;
        graph.nomEC = 0;
        graph.denomEC = 0;
        graph.groupEC = 0;
        graph.maxNodeEC = 0;
        graph.minNodeEC = 0;
        graph.sumEC = 0;
        graph.discreteECs.clear();
        graph.classesEC = 0;

        graph.calculatedDistances = false;
        graph.resetDistanceAggregates();

        // Stores vertex pairs not connected
        // Vertices in keys have
        // Infinite Eccentricity
        // Zero Eccentricity Centrality
        // Zero Closeness Centrality
        graph.notConnectedPairsClear();

        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            for (ds.it1 = graph.verticesBegin(); ds.it1 != graph.verticesEnd(); ++ds.it1)
            {
                if (considerWeights)
                {
                    // hasEdgeTo() returns exactly 0 for "no edge" (see its own doc comment), so a
                    // negative return is unambiguously a real negative-weight edge, never a
                    // nonexistent one. Checked first, before any state below (setConnectedCached(),
                    // per-vertex centrality zeroing) is mutated - Dijkstra is mathematically
                    // undefined for negative weights, so refuse the whole computation rather than
                    // leaving partially-mutated state that looks legitimately computed but isn't.
                    // See #277/WS18 P1. Skipped when negativeWeightSafe is set: that caller has
                    // opted into the Johnson's-algorithm path (see compute()), which is defined
                    // for negative weights - only a negative cycle is refused there, not this.
                    ds.tempEdgeWeight = (*ds.it)->hasEdgeTo((*ds.it1)->number());
                    if (ds.tempEdgeWeight < 0 && !negativeWeightSafe)
                    {
                        sink.reportNegativeWeights();
                        return;
                    }
                    if (inverseWeights && ds.tempEdgeWeight > ds.maxEdgeWeightInNetwork)
                    {
                        // find the max weight in the network.
                        // it will be used for maxCC below
                        ds.maxEdgeWeightInNetwork = ds.tempEdgeWeight;
                    }
                }
            }

            // Zero centrality scores for each vertex
            if (computeCentralities)
            {
                (*ds.it)->setBC(0.0);
                (*ds.it)->setSC(0.0);
                (*ds.it)->setEccentricity(0.0);
                (*ds.it)->setEC(0.0);
                (*ds.it)->setCC(0.0);
                (*ds.it)->setIRCC(0.0);
                (*ds.it)->setPC(0.0);
            }
        }

        graph.setConnectedCached(true);

        if (graph.symmetricCached())
        {
            graph.maxIndexBC = (ds.N == 2) ? 1 : (ds.N - 1.0) * (ds.N - 2.0) / 2.0;
            graph.maxIndexSC = (ds.N == 2) ? 1 : (ds.N - 1.0) * (ds.N - 2.0) / 2.0;
            graph.maxIndexCC = ds.N - 1.0;
            graph.maxIndexPC = ds.N - 1.0;
        }
        else
        {
            graph.maxIndexBC = (ds.N == 2) ? 1 : (ds.N - 1.0) * (ds.N - 2.0); // fix N=2 case where maxIndex becomes zero
            graph.maxIndexSC = (ds.N == 2) ? 1 : (ds.N - 1.0) * (ds.N - 2.0);
            graph.maxIndexPC = ds.N - 1.0;
            graph.maxIndexCC = ds.N - 1.0;
        }

        if (considerWeights && inverseWeights)
        {
            graph.maxIndexCC = graph.maxIndexCC * (1.0 / ds.maxEdgeWeightInNetwork);
        }
    }
}

/**
 * @brief Public entry point for the potentials pass below, for callers with no DistanceScratch
 * of their own (currently just the "signed" CLI kernel). Owns a throwaway DistanceScratch and
 * copies its result out. Not part of compute()'s pipeline and not yet threaded into
 * dijkstraSSSP()/runAllSources() - see the overload below for the algorithm and status.
 * @param inverseWeights invert each edge weight before relaxing, same convention as elsewhere
 * @param outPotentials filled with h(v) per vertex (indexed by vertex position) on success;
 * not meaningful if this returns false
 * @return false if a reachable negative cycle was found, true otherwise
 */
bool DistanceEngine::bellmanFordPotentials(const bool inverseWeights, QVector<qreal> &outPotentials)
{
    DistanceScratch ds;
    const bool ok = bellmanFordPotentials(inverseWeights, ds);
    outPotentials = ds.potentials;
    return ok;
}

/**
 * @brief Bellman-Ford reweighting pass computing a potential h(v) for every vertex, so that
 * edges can later be reweighted as w'(u,v) = w(u,v) + h(u) - h(v) and handed to dijkstraSSSP()
 * unmodified on a graph guaranteed to have no negative edges. Every real vertex starts at
 * potential 0 - this is exactly the result an implicit virtual source with a zero-weight edge
 * to every real vertex would produce on round 0, so that virtual source never needs to be
 * materialized. Also detects negative cycles as a by-product of the same pass (the standard
 * "does relaxation round V still improve anything" check) - a negative cycle makes shortest
 * paths undefined, so ds.potentials is left incomplete/unusable and this returns false.
 * Not yet called from compute() or wired into dijkstraSSSP()/runAllSources() - calling it
 * unconditionally would cost every ordinary (non-negative-weight) computation a wasted full
 * edge relaxation pass; it should be gated behind an explicit opt-in once its result is
 * actually consumed by the SSSP loop.
 * @param inverseWeights invert each edge weight before relaxing, same convention as elsewhere
 * @param ds run-scratch state; ds.potentials/ds.negativeCycleDetected are written here
 * @return false if a reachable negative cycle was found, true otherwise
 */
bool DistanceEngine::bellmanFordPotentials(const bool inverseWeights, DistanceScratch &ds)
{
    int totalV = 0;
    for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
        ++totalV;

    // Every real vertex starts at potential 0. This is exactly the result a zero-weight edge
    // from an implicit virtual source to each real vertex would produce on round 0 of
    // Bellman-Ford, so the virtual source never needs to be materialized as an actual vertex.
    ds.potentials.assign(totalV, 0.0);
    ds.negativeCycleDetected = false;

    const int relation = graph.relationCurrent();

    // Standard Bellman-Ford: V-1 rounds of relaxing every edge is enough to find every
    // shortest path from the virtual source (which reaches every vertex directly), then one
    // more round checks whether anything still improves - if so, a negative cycle is
    // reachable and shortest paths (hence potentials) are undefined.
    for (int round = 0; round < totalV; ++round)
    {
        bool anyRelaxed = false;

        for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
        {
            const int u  = (*it)->number();
            const int ui = graph.vertexIndexByNumber(u);

            if (ds.potentials[ui] == RAND_MAX)
            {
                // u itself unreachable from the virtual source so far this round - can't relax
                // anything through it yet. Never true in practice since every real vertex starts
                // reachable from the virtual source at potential 0, kept only for safety.
                continue;
            }

            auto it1 = graph.vertexAtIndex(ui)->outEdges().cbegin();
            while (it1 != graph.vertexAtIndex(ui)->outEdges().cend())
            {
                if (it1.value().first != relation || it1.value().second.second != true)
                {
                    ++it1;
                    continue;
                }

                const int w  = it1.key();
                const int wi = graph.vertexIndexByNumber(w);

                qreal weight = it1.value().second.first;

                // Same zero-weight-edge and inverse-weight handling as dijkstraSSSP(), so the
                // graph this pass reasons about is exactly the one dijkstraSSSP() will traverse.
                if (weight == 0)
                {
                    ++it1;
                    continue;
                }
                if (inverseWeights)
                {
                    weight = 1.0 / weight;
                }

                if (ds.potentials[ui] + weight < ds.potentials[wi])
                {
                    if (round == totalV - 1)
                    {
                        // This is the extra (Vth) round: relaxation still finding an
                        // improvement here means a negative cycle is reachable.
                        ds.negativeCycleDetected = true;
                        return false;
                    }
                    ds.potentials[wi] = ds.potentials[ui] + weight;
                    anyRelaxed = true;
                }

                ++it1;
            }
        }

        if (!anyRelaxed)
        {
            // Converged early - no need to run the remaining rounds.
            break;
        }
    }

    return true;
}

void DistanceEngine::runAllSources(const bool computeCentralities,
                                   const bool considerWeights,
                                   const bool inverseWeights,
                                   const bool dropIsolates,
                                   DistanceScratch &ds,
                                   IDistanceProgressSink &sink)
{
    qCDebug(lcEngine) << "*********** MAIN LOOP (parallel): "
                "solving SSSP from every source vertex across CPU cores...";

    // Count total VList positions (includes disabled vertices) for scratch-array sizing.
    // Scratch indices come from vertexIndexByNumber(), which returns VList positions,
    // so arrays must be sized by totalV (not ds.N which counts enabled-only).
    int totalV = 0;
    for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
        ++totalV;

    // WS5 A2: relation-keyed flat-matrix APSP storage, resized fresh for this run (same
    // always-reconstruct convention every other Graph matrix field already follows - see
    // createMatrixAdjacency()/graphMatrixDistanceGeodesicCreate()). Written alongside the
    // per-vertex QHash writes below; nothing reads from it yet.
    const int relation = graph.relationCurrent();
    graph.m_apspDist[relation].resize(totalV, totalV);
    graph.m_apspSigma[relation].resize(totalV, totalV);

    // Over-allocate the state vector generously.  The pool has at most
    // idealThreadCount() workers; doubling + 4 guards against unexpected
    // pool growth (e.g., other Qt internals adding threads mid-run) without
    // wasting meaningful memory on the per-thread scratch arrays.
    int threadCount = QThread::idealThreadCount();
    QVector<ThreadLocalState> allStates(threadCount * 2 + 4);
    for (auto &tls : allStates)
        tls.allocate(totalV);

    // Collect enabled source vertices upfront so the lambda receives a plain value
    // (QPair<int,int>) instead of iterating a shared container from multiple threads.
    // Element: (vertex_number, VList_position).
    QVector<QPair<int, int>> sources;
    sources.reserve(ds.N);
    for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
    {
        if ((*it)->isEnabled())
            sources.append({(*it)->number(), graph.vertexIndexByNumber((*it)->number())});
    }

    // Thread-ID → slot map: guarantees each distinct OS thread gets a unique, stable
    // slot index into allStates across all lambda invocations within this call.
    // Using a per-call map (not thread_local) avoids the "stale slot" bug that occurs
    // when runAllSources is called multiple times (e.g., warmup + bench runs):
    // thread_local would carry over a slot index from the previous call's allStates,
    // and any newly created pool thread would receive the same index — data race.
    QMutex slotMutex;
    QHash<Qt::HANDLE, int> threadSlots;
    QAtomicInt nextSlot{0};

    // ---- Parallel SSSP source loop ----
    // Safety analysis:
    //   - Source-vertex writes (APSP, CC, eccentricity, PC, SPC): safe — si is unique.
    //   - Intermediate-vertex writes (BC, SC): unsafe → go to tls.partialBC / tls.partialSC.
    //   - Graph-wide aggregates (distanceSum, geodesicsCount, diameter, sumPC, sumSPC):
    //     unsafe → go to tls accumulators; reduced into graph state after the map.
    //   - All graph reads (edges, vertex numbers, relation) are read-only during SSSP.
    //   - Cancel signals cannot be delivered while graphThread's event loop is blocked
    //     here, so we skip the cancel check inside the lambda; UX shows 0%→100% jump.
    QtConcurrent::blockingMap(sources, [&](const QPair<int, int> &src) {
        // Map this OS thread to a unique slot on first entry; reuse the slot on
        // subsequent invocations (same thread processes multiple sources).
        // slotMutex is held only for the brief hash lookup — never during SSSP.
        int mySlot;
        {
            QMutexLocker lock(&slotMutex);
            Qt::HANDLE me = QThread::currentThreadId();
            auto it = threadSlots.find(me);
            if (it != threadSlots.end()) {
                mySlot = it.value();
            } else {
                mySlot = nextSlot.fetchAndAddOrdered(1);
                threadSlots[me] = mySlot;
            }
        }
        ThreadLocalState &tls = allStates[mySlot];

        const int s  = src.first;
        const int si = src.second;

        qCDebug(lcEngine) << "***** PHASE 1 (SSSP) [thread slot" << mySlot << "]: source s" << s << "vpos" << si;

        // Reset per-source scratch (dist, sigma, and optionally Stack/Ps/nthOrder).
        // Also resets pss.sourceDistanceSum / sourceGeodesicsCount.
        tls.pss.resetPerSource(computeCentralities);

        // Run BFS or Dijkstra; unsafe graph calls go to tls.pss scratch fields / tls.partialSC.
        // ds.potentials is read-only from here on (populated once, single-threaded, before this
        // parallel loop starts - see bellmanFordPotentials()) so concurrent reads across source
        // threads are safe; empty unless a caller explicitly requested Johnson's reweighting.
        if (!considerWeights)
            bfsSSSP(s, si, computeCentralities, dropIsolates, tls.pss, tls.partialSC);
        else
            dijkstraSSSP(s, si, computeCentralities, inverseWeights, dropIsolates, tls.pss,
                        tls.partialSC, ds.potentials);

        // Accumulate per-source aggregates into thread-local running totals.
        // These will be reduced into graph-global state after the parallel loop.
        tls.totalDistanceSum    += tls.pss.sourceDistanceSum;
        tls.totalGeodesicsCount += tls.pss.sourceGeodesicsCount;

        qCDebug(lcEngine) << "***** PHASE 1 (SSSP): FINISHED BFS/DIJKSTRA for s" << s
                 << "— writing APSP results back to vertex" << si;

        // Un-reweight before anything below reads tls.pss.dist[]: d(s,v) = d'(s,v) - h(s) + h(v).
        // Must happen before both the APSP write-back just below and the CC/PC accumulation
        // further down, since both consume tls.pss.dist[] directly. A no-op when ds.potentials
        // is empty (plain Dijkstra/BFS, no Johnson's reweighting requested). RAND_MAX (unreached)
        // is left untouched - it's a sentinel, not a real distance to un-reweight.
        if (!ds.potentials.isEmpty())
        {
            for (int vi = 0; vi < totalV; ++vi)
            {
                if (tls.pss.dist[vi] != RAND_MAX)
                    tls.pss.dist[vi] += ds.potentials[vi] - ds.potentials[si];
            }
        }

        // APSP write-back: persist tls.pss.dist / sigma into row si of the flat matrices, and
        // (same pass, since it's already walking every final, un-reweighted distance) find this
        // source's own eccentricity/diameter contribution - the max over FINAL per-vertex
        // distances, not a running max sampled during relaxation (a vertex can be relaxed to a
        // smaller distance after an earlier, larger one; tracking every relaxation event instead
        // of the final value per vertex was a real bug - see #286). RAND_MAX (unreached) is
        // excluded, matching existing diameter semantics (an unreachable pair doesn't contribute).
        // Safe: si is unique across all concurrent lambda invocations, so no two sources ever
        // write the same row. Unconditional (every column vi, not just reached ones) -
        // tls.pss.dist[vi] already holds RAND_MAX for every unreached vi
        // (PerSourceScratch::resetPerSource() fills it before every source, unconditionally),
        // so this isn't new work - it reuses a reset that was already happening.
        int sourceMaxDist = 0;
        for (int vi = 0; vi < totalV; ++vi)
        {
            graph.m_apspDist[relation].setItem(si, vi, tls.pss.dist[vi]);
            graph.m_apspSigma[relation].setItem(si, vi, (qreal)tls.pss.sigma[vi]);

            if (tls.pss.dist[vi] != RAND_MAX && tls.pss.dist[vi] > sourceMaxDist)
                sourceMaxDist = (int)tls.pss.dist[vi];
        }
        if (sourceMaxDist > tls.maxDiameter)
            tls.maxDiameter = sourceMaxDist;

        if (computeCentralities)
        {
            // ---- Power Centrality ----
            // PC(s) = [1/(N-1)] * sum_i( nthOrder[i] / i )
            // where nthOrder[i] = number of nodes at distance i from s.
            tls.pss.componentSize = 1;
            qreal pc = 0;
            for (auto hfi = tls.pss.nthOrder.constBegin(); hfi != tls.pss.nthOrder.constEnd(); ++hfi)
            {
                pc += (1.0 / hfi.key()) * hfi.value();
                tls.pss.componentSize += hfi.value();
            }
            graph.vertexAtIndex(si)->setPC(pc);   // safe: source vertex
            // Accumulate into thread-local sum; graph.sumPC updated in reduction.
            tls.totalSumPC += pc;

            qreal spc = (tls.pss.componentSize != 1)
                            ? (1.0 / (tls.pss.componentSize - 1.0)) * pc
                            : 0;
            graph.vertexAtIndex(si)->setSPC(spc); // safe: source vertex
            tls.totalSumSPC += spc;

            qCDebug(lcEngine) << "***** PHASE 2 (CENTRALITIES): s" << s << "vpos" << si << "PC" << pc;

            // ---- Closeness Centrality + delta reset ----
            // Walk every vertex position to zero delta[] (needed for BC back-propagation)
            // and simultaneously sum distances for CC.  RAND_MAX propagates the
            // disconnected-graph sentinel so CC becomes 0 when s cannot reach all others.
            qreal distances_sum_for_s = 0;
            for (int vi1 = 0; vi1 < totalV; ++vi1)
            {
                tls.pss.delta[vi1] = 0.0;
                distances_sum_for_s += tls.pss.dist[vi1];
            }
            // Accumulate into thread-local total; graph.addToDistanceSum() in reduction.
            tls.totalCCDistanceSum += distances_sum_for_s;

            qreal cc = (distances_sum_for_s != 0 && distances_sum_for_s < RAND_MAX)
                           ? 1.0 / distances_sum_for_s
                           : 0;
            graph.vertexAtIndex(si)->setCC(cc);   // safe: source vertex

            qCDebug(lcEngine) << "***** PHASE 2 (CENTRALITIES): s" << s << "vpos" << si << "CC" << cc;

            // ---- Brandes BC back-propagation ----
            // Visit vertices in reverse BFS/Dijkstra order (deepest first) and propagate
            // dependency deltas up the shortest-path DAG.  Instead of writing to
            // vertex->BC() directly (which would race across threads for intermediate
            // vertices), accumulate into tls.partialBC[wi]; the reduction step merges all.
            qCDebug(lcEngine) << "***** PHASE 2 (BC/ACCUMULATION): back-propagating from s" << s
                     << "Stack size" << (int)tls.pss.Stack.size();

            while (!tls.pss.Stack.empty())
            {
                int w  = tls.pss.Stack.top();
                int wi = graph.vertexIndexByNumber(w);
                tls.pss.Stack.pop();

                const QList<int> &lst = tls.pss.Ps[wi];
                for (int u : lst)
                {
                    int ui = graph.vertexIndexByNumber(u);
                    if (tls.pss.sigma[wi] > 0)
                    {
                        // delta[u] += (1 + delta[w]) * sigma[u] / sigma[w]
                        tls.pss.delta[ui] += (1.0 + tls.pss.delta[wi]) *
                                             ((qreal)tls.pss.sigma[ui] /
                                              (qreal)tls.pss.sigma[wi]);
                    }
                }

                if (w != s)
                {
                    // Accumulate into per-thread partial BC instead of vertex->setBC().
                    // Intermediate vertex w may be processed by multiple source threads;
                    // partialBC[wi] is private to this thread, so no mutex needed.
                    tls.partialBC[wi] += tls.pss.delta[wi];
                }
            } // END BC back-propagation

        } // END if computeCentralities

    }); // END QtConcurrent::blockingMap

    qCDebug(lcEngine) << "*********** MAIN LOOP (parallel SSSP): FINISHED. Starting reduction.";

    // ---- Sequential reduction ----
    // Merge per-thread accumulators into graph-global state.  This runs on the calling
    // thread after blockingMap returns; no concurrent access, no mutexes needed.
    for (auto &tls : allStates)
    {
        // Distance sum from BFS inner-loop discoveries (0 for Dijkstra).
        graph.addToDistanceSum(tls.totalDistanceSum);
        if (computeCentralities)
            // Distance sum from the CC-denominator accumulation in the centralities block.
            graph.addToDistanceSum(tls.totalCCDistanceSum);

        // Geodesics count (reachable source-target pairs found by BFS / Dijkstra).
        graph.addGeodesicsCount(tls.totalGeodesicsCount);

        // Diameter: keep the overall maximum across all threads.
        if (tls.maxDiameter > graph.graphDiameterCached())
            graph.setDiameterCached(tls.maxDiameter);

        if (computeCentralities)
        {
            graph.sumPC  += tls.totalSumPC;
            graph.sumSPC += tls.totalSumSPC;
        }
    }

    // Reduce partial BC and SC arrays into per-vertex scores.
    // BC/SC were initialised to 0 in initRun; we accumulate all thread contributions here.
    if (computeCentralities)
    {
        for (int wi = 0; wi < totalV; ++wi)
        {
            qreal totalBC = 0, totalSC = 0;
            for (auto &tls : allStates)
            {
                totalBC += tls.partialBC[wi];
                totalSC += tls.partialSC[wi];
            }
            if (totalBC != 0.0)
                graph.vertexAtIndex(wi)->setBC(totalBC);
            if (totalSC != 0.0)
                graph.vertexAtIndex(wi)->setSC(totalSC);
        }
    }

    qCDebug(lcEngine) << "*********** MAIN LOOP (SSSP problem): FINISHED.";
}

void DistanceEngine::finalize(const bool computeCentralities,
                              const bool dropIsolates,
                              DistanceScratch &ds,
                              CentralityScratchFinalize &csf,
                              IDistanceProgressSink &sink)
{
    Q_UNUSED(sink); // progress dialog is managed by compute(); finalize() has no steps to report

    // check if there are disconnected nodes
    // and get the distance sums
    qCDebug(lcEngine) << "Checking if there are disconnected nodes";

    graph.setConnectedCached(true);

    // WS5 A2: this pair loop is O(N^2) - the actual hot path A2 targets. i is looked up once per
    // outer iteration (O(N) total, negligible); the inner loop iterates positions directly
    // (vertexAtIndex(j), not an iterator + a per-pair position lookup), so each pair read is a
    // genuine O(1) matrix access with no hash lookup involved.
    int totalV = 0;
    for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
        ++totalV;
    const int relation = graph.relationCurrent();

    for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
    {
        if (!(*ds.it)->isEnabled())
        {
            qCDebug(lcEngine) << "actor i" << (*ds.it)->number() << "disabled. SKIP/CONTINUE";
            continue;
        }

        ds.pairDistance = 0;
        const int i = graph.vertexIndexByNumber((*ds.it)->number());

        for (int j = 0; j < totalV; ++j)
        {
            GraphVertex *v1 = graph.vertexAtIndex(j);

            if (!v1->isEnabled())
            {
                qCDebug(lcEngine) << "   actor j" << v1->number() << "disabled. SKIP/CONTINUE";
                continue;
            }
            if (v1->number() == (*ds.it)->number())
            {
                qCDebug(lcEngine) << "   == actor j" << v1->number() << "SKIP/CONTINUE";
                continue;
            }

            ds.pairDistance = graph.m_apspDist[relation].item(i, j);

            if (ds.pairDistance == RAND_MAX)
            {
                graph.notConnectedPairsInsert((*ds.it)->number(), v1->number());
                (*ds.it)->setEccentricity(RAND_MAX);
                graph.setConnectedCached(false);

                qCDebug(lcEngine) << "actor i" << (*ds.it)->number()
                         << "has infinite eccentricity. "
                            "There is no path from it to actor j"
                         << v1->number();
            }
            else
            {
                qCDebug(lcEngine) << "actor i" << (*ds.it)->number()
                         << "distanceSum" << (*ds.it)->distanceSum();
                (*ds.it)->setDistanceSum((*ds.it)->distanceSum() + ds.pairDistance);
            }
        } // end for

        qCDebug(lcEngine) << "actor i" << (*ds.it)->number()
                 << "Final distanceSum" << (*ds.it)->distanceSum();

        if (computeCentralities)
        {
            // Compute Eccentricity (max geodesic distance)
            csf.eccentricity = (*ds.it)->eccentricity();

            qCDebug(lcEngine) << "actor"
                     << (*ds.it)->number()
                     << "eccentricity" << csf.eccentricity;

            if (csf.eccentricity != RAND_MAX)
            {
                // Find min/max Eccentricity
                graph.minmax(csf.eccentricity, (*ds.it),
                             graph.maxEccentricity,
                             graph.minEccentricity,
                             graph.maxNodeEccentricity,
                             graph.minNodeEccentricity);

                graph.resolveClasses(csf.eccentricity,
                                     graph.discreteEccentricities,
                                     graph.classesEccentricity,
                                     (*ds.it)->number());

                // Eccentricity Centrality is the inverted Eccentricity
                csf.EC = 1.0 / csf.eccentricity;
                (*ds.it)->setEC(csf.EC);  // Set Eccentricity Centrality
                (*ds.it)->setSEC(csf.EC); // Set std EC = EC
                graph.sumEC += csf.EC;    // set sum EC

                qCDebug(lcEngine) << "actor i" << (*ds.it)->number()
                         << "EC"
                         << csf.EC;
            }
            else
            {
                csf.EC = 0;
                (*ds.it)->setEC(csf.EC);  // Set Eccentricity Centrality
                (*ds.it)->setSEC(csf.EC); // Set std EC = EC
                graph.sumEC += csf.EC;    // set sum EC

                qCDebug(lcEngine) << "actor i" << (*ds.it)->number()
                         << "EC=0 (disconnected graph)";
            }

        } // end if compute centralities

    } // end for disconnected checking

    // Compute average path length...
    if (graph.notConnectedPairsSize() == 0)
    {
        graph.setAverageDistanceCached(graph.graphSumDistanceCached() / (ds.N * (ds.N - 1.0)));
        qCDebug(lcEngine) << "Graph::graphDistancesGeodesic() - Average distance:"
                 << graph.graphDistanceGeodesicAverageCached();
    }
    else
    {
        // TODO In not connected nets, it would be nice to ask the user what to do
        //  with unconnected pairs (make M or drop (default?)
        qCDebug(lcEngine) << "Graph::graphDistancesGeodesic() - Average distance:"
                 << graph.graphDistanceGeodesicAverageCached();
        graph.setAverageDistanceCached(graph.graphSumDistanceCached() / graph.graphGeodesicsCountCached());
    }

    if (computeCentralities)
    {
        qCDebug(lcEngine) << "Graph: graphDistancesGeodesic() - "
                    "Computing centralities...";
        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            if (dropIsolates && (*ds.it)->isIsolated())
            {
                qCDebug(lcEngine) << "vertex " << (*ds.it)->number()
                         << " isolated, continue. ";
                continue;
            }

            // Compute classes and min/maxEC
            csf.SEC = (*ds.it)->SEC();
            graph.resolveClasses(csf.SEC, graph.discreteECs, graph.classesEC, (*ds.it)->number());
            graph.minmax(csf.SEC, (*ds.it), graph.maxEC, graph.minEC, graph.maxNodeEC, graph.minNodeEC);

            // Compute classes and min/maxSPC
            csf.SPC = (*ds.it)->SPC(); // same as PC
            graph.resolveClasses(csf.SPC, graph.discretePCs, graph.classesSPC, (*ds.it)->number());
            graph.minmax(csf.SPC, (*ds.it), graph.maxSPC, graph.minSPC, graph.maxNodeSPC, graph.minNodeSPC);

            // Compute std BC, classes and min/maxSBC
            if (graph.symmetricCached())
            {
                qCDebug(lcEngine) << "Betweenness centrality must be divided by"
                         << " two if the graph is undirected";
                (*ds.it)->setBC((*ds.it)->BC() / 2.0);
            }
            csf.BC = (*ds.it)->BC();
            graph.sumBC += csf.BC;
            csf.SBC = csf.BC / graph.maxIndexBC;
            (*ds.it)->setSBC(csf.SBC);
            graph.resolveClasses(csf.SBC, graph.discreteBCs, graph.classesSBC);
            graph.sumSBC += csf.SBC;
            graph.minmax(csf.SBC, (*ds.it), graph.maxSBC, graph.minSBC, graph.maxNodeSBC, graph.minNodeSBC);

            // Compute std CC, classes and min/maxSCC
            csf.CC = (*ds.it)->CC();
            graph.sumCC += csf.CC;
            csf.SCC = graph.maxIndexCC * csf.CC;
            (*ds.it)->setSCC(csf.SCC);
            graph.resolveClasses(csf.SCC, graph.discreteCCs, graph.classesSCC, (*ds.it)->number());
            graph.sumSCC += csf.SCC;
            graph.minmax(csf.SCC, (*ds.it), graph.maxSCC, graph.minSCC, graph.maxNodeSCC, graph.minNodeSCC);

            // prepare to compute stdSC
            csf.SC = (*ds.it)->SC();
            if (graph.symmetricCached())
            {
                (*ds.it)->setSC(csf.SC / 2.0);
                csf.SC = (*ds.it)->SC();
                qCDebug(lcEngine) << "SC of " << (*ds.it)->number()
                         << "  divided by 2 (because the graph is symmetric) "
                         << (*ds.it)->SC();
            }
            graph.sumSC += csf.SC;

            qCDebug(lcEngine) << "vertex " << (*ds.it)->number() << " - "
                     << " EC: " << (*ds.it)->EC()
                     << " CC: " << (*ds.it)->CC()
                     << " BC: " << (*ds.it)->BC()
                     << " SC: " << (*ds.it)->SC()
                     << " PC: " << (*ds.it)->PC();
        } // end for

        qCDebug(lcEngine) << "Graph: graphDistancesGeodesic() -"
                    "Computing mean centrality values...";

        // Compute mean values and prepare to compute variances
        graph.meanSBC = graph.sumSBC / (qreal)ds.N;
        graph.varianceSBC = 0;
        csf.tempVarianceBC = 0;

        graph.meanSCC = graph.sumSCC / (qreal)ds.N;
        graph.varianceSCC = 0;
        csf.tempVarianceCC = 0;

        graph.meanSPC = graph.sumSPC / (qreal)ds.N;
        graph.varianceSPC = 0;
        csf.tempVariancePC = 0;

        graph.meanEC = graph.sumEC / (qreal)ds.N;
        graph.varianceEC = 0;
        csf.tempVarianceEC = 0;

        qCDebug(lcEngine) << "Graph: graphDistancesGeodesic() - "
                    "Computing std centralities ...";

        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            if (dropIsolates && (*ds.it)->isIsolated())
            {
                continue;
            }
            // Compute std SC, classes and min/maxSSC
            csf.SC = (*ds.it)->SC();
            csf.SSC = csf.SC / graph.sumSC;
            (*ds.it)->setSSC(csf.SSC);
            graph.resolveClasses(csf.SSC, graph.discreteSCs, graph.classesSSC);
            graph.sumSSC += csf.SSC;
            graph.minmax(csf.SSC, (*ds.it), graph.maxSSC, graph.minSSC, graph.maxNodeSSC, graph.minNodeSSC);

            // Compute numerator of groupSBC
            csf.SBC = (*ds.it)->SBC();
            graph.nomSBC += (graph.maxSBC - csf.SBC);

            // calculate BC variance
            csf.tempVarianceBC = (csf.SBC - graph.meanSBC);
            csf.tempVarianceBC *= csf.tempVarianceBC;
            graph.varianceSBC += csf.tempVarianceBC;

            // Compute numerator of groupCC
            graph.nomSCC += graph.maxSCC - (*ds.it)->SCC();

            // calculate CC variance
            csf.tempVarianceCC = ((*ds.it)->SCC() - graph.meanSCC);
            csf.tempVarianceCC *= csf.tempVarianceCC;
            graph.varianceSCC += csf.tempVarianceCC;

            // Compute numerator of groupSPC
            csf.SPC = (*ds.it)->SPC();
            graph.nomSPC += (graph.maxSPC - csf.SPC);

            // calculate PC variance
            csf.tempVariancePC = ((*ds.it)->SPC() - graph.meanSPC);
            csf.tempVariancePC *= csf.tempVariancePC;
            graph.varianceSPC += csf.tempVariancePC;

            // calculate EC variance
            csf.tempVarianceEC = ((*ds.it)->EC() - graph.meanEC);
            csf.tempVarianceEC *= csf.tempVarianceEC;
            graph.varianceEC += csf.tempVarianceEC;

        } // end for

        // compute final variances
        graph.varianceSBC /= (qreal)ds.N;
        graph.varianceSCC /= (qreal)ds.N;
        graph.varianceSPC /= (qreal)ds.N;

        graph.varianceEC /= (qreal)ds.N;

        // calculate SC mean value and prepare to compute variance
        graph.meanSSC = graph.sumSSC / (qreal)ds.N;
        graph.varianceSSC = 0;
        csf.tempVarianceSC = 0;
        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            if (dropIsolates && (*ds.it)->isIsolated())
            {
                continue;
            }
            csf.tempVarianceSC = ((*ds.it)->SSC() - graph.meanSSC);
            csf.tempVarianceSC *= csf.tempVarianceSC;
            graph.varianceSSC += csf.tempVarianceSC;
        }
        // calculate final SC variance
        graph.varianceSSC /= (qreal)ds.N;

        graph.denomSPC = ((ds.N - 2.0)) / (2.0); // only for connected nets
        if (ds.N < 3)
            graph.denomSPC = ds.N - 1.0;
        // what if the net is disconnected (isolates exist) ?
        graph.groupSPC = graph.nomSPC / graph.denomSPC;

        graph.denomSCC = ((ds.N - 1.0) * (ds.N - 2.0)) / (2.0 * ds.N - 3.0);
        if (ds.N < 3)
            graph.denomSCC = ds.N - 1.0;

        graph.groupCC = graph.nomSCC / graph.denomSCC; // Calculate group Closeness centrality

        // nomSBC*=2.0;
        //             denomSBC =   (N-1.0) *  (N-1.0) * (N-2.0);
        graph.denomSBC = (ds.N - 1.0);                  // Wasserman&Faust - formula 5.14
        graph.groupSBC = graph.nomSBC / graph.denomSBC; // Calculate group Betweenness centrality

        graph.calculatedCentralities = true;

    } // END if computeCentralities
}

/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

    INPUT:
        a 'source' vertex with vpos s and a boolean computeCentralities.
        (Implicitly, BFS uses the m_graph structure)

    OUTPUT:
        For every vertex t: pss.dist[ti] is set to the distance of each t from s
        For every vertex t: pss.sigma[ti] is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then BFS does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases pss.nthOrder[ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to pss.Ps[y], thus Ps stores all predecessors of y on all shortest paths from s
            c) Each vertex u popped from Q is pushed to pss.Stack

*/
void DistanceEngine::bfsSSSP(const int &s, const int &si,
                             const bool &computeCentralities,
                             const bool &dropIsolates,
                             PerSourceScratch &pss,
                             QVector<qreal> &partialSC)
{
    Q_UNUSED(dropIsolates);

    qCDebug(lcEngine) << "BFS:";
    int u = 0, ui = 0, w = 0, wi = 0;
    int dist_u = 0, temp = 0, dist_w = 0;
    int relation = 0;
    qreal weight = 0; // Fix #30: needed to skip zero-weight edges
    bool edgeStatus = false;
    H_edges::const_iterator it1;

    // set distance of s from s equal to 0
    pss.dist[si] = 0;

    // set sigma of s from s equal to 1
    pss.sigma[si] = 1;

    std::queue<int> Q;

    Q.push(s);

    qCDebug(lcEngine) << "BFS: LOOP: While Q not empty ";
    while (!Q.empty())
    {

        u = Q.front();
        Q.pop();
        ui = graph.vertexIndexByNumber(u);
        qCDebug(lcEngine) << "BFS: Dequeue: first element of Q is u" << u << "graph.vertexIndexByNumber" << ui;

        if (!graph.vertexAtIndex(ui)->isEnabled())
        {
            continue;
        }

        if (computeCentralities)
        {
            qCDebug(lcEngine) << "BFS: Compute centralities: Pushing u" << u
                     << "to Stack ";
            pss.Stack.push(u);
        }
        qCDebug(lcEngine) << "BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u";
        it1 = graph.vertexAtIndex(ui)->outEdges().cbegin();
        while (it1 != graph.vertexAtIndex(ui)->outEdges().cend())
        {
            relation = it1.value().first;
            if (relation != graph.relationCurrent())
            {
                ++it1;
                continue;
            }
            edgeStatus = it1.value().second.second;
            if (edgeStatus != true)
            {
                ++it1;
                continue;
            }
            // Fix #30: zero-weight edges are visual-only and must not be
            // traversed as structural connections. Without this guard, BFS
            // would report incorrect geodesic distances and reachability.
            weight = it1.value().second.first;
            if (weight == 0)
            {
                ++it1;
                continue;
            }
            w = it1.key();
            wi = graph.vertexIndexByNumber(w);
            qCDebug(lcEngine, "BFS: u=%i is connected with node w=%i of graph.vertexIndexByNumber wi=%i. ", u, w, wi);

            qCDebug(lcEngine, "BFS: Start path discovery");

            // if dist[wi] is RAND_MAX, w is found for the first time.
            if (pss.dist[wi] == (qreal)RAND_MAX)
            {

                qCDebug(lcEngine, "BFS: First time visiting w=%i. Enqueuing w to the end of Q", w);

                Q.push(w);

                dist_u = (int)pss.dist[ui];
                dist_w = dist_u + 1;

                qCDebug(lcEngine) << "BFS: Setting dist_w = d ( s" << s << ", w" << w
                         << ") equal to dist_u=d(s,u) plus 1. New dist_w" << dist_w;

                pss.dist[wi] = (qreal)dist_w;

                // Accumulate into scratch instead of calling graph methods directly.
                // Multiple threads run bfsSSSP concurrently; these graph methods
                // are not thread-safe.  The owning thread reduces the scratch totals
                // into graph state after QtConcurrent::blockingMap returns.
                pss.sourceDistanceSum += dist_w;
                ++pss.sourceGeodesicsCount;

                qCDebug(lcEngine) << "== BFS  - d("
                         << s << "," << w
                         << ")=" << pss.dist[wi];

                if (computeCentralities)
                {
                    qCDebug(lcEngine) << "BFS: Calculate PC: store the number of nodes at distance "
                             << dist_w << "from s";

                    pss.nthOrderIncrement((qreal)dist_w);
                    qCDebug(lcEngine) << "BFS: Calculate CC: the sum of distances (will invert it l8r)";
                    // Source-vertex writes (si is unique per thread): safe for parallelism.
                    graph.vertexAtIndex(si)->setCC(graph.vertexAtIndex(si)->CC() + dist_w);

                    qCDebug(lcEngine) << "BFS: Calculate Eccentricity: the maximum distance ";
                    if (graph.vertexAtIndex(si)->eccentricity() < dist_w)
                        graph.vertexAtIndex(si)->setEccentricity(dist_w);
                }
            }

            qCDebug(lcEngine) << "BFS: Start path counting";

            // Is edge (u,w) on a shortest path from s to w via u?
            if ((int)pss.dist[wi] == (int)pss.dist[ui] + 1)
            {

                temp = pss.sigma[wi] + pss.sigma[ui];

                qCDebug(lcEngine) << "BFS: Found a NEW SHORTEST PATH from s" << s
                         << "to w" << w << "via u" << u
                         << "Setting Sigma(s, w)" << temp;
                if (s != w)
                {
                    pss.sigma[wi] = temp;
                }
                if (computeCentralities)
                {
                    qCDebug(lcEngine) << "BFS/SC: Computing centralities: Computing SC ";
                    if (s != w && s != u && u != w)
                    {
                        qCDebug(lcEngine) << "BFS: partialSC[ui=" << ui << "] += 1";
                        // Intermediate vertex ui may be processed by concurrent threads
                        // (other sources pass through the same u).  Write to partialSC[ui]
                        // — a per-thread array — instead of vertex->setSC() to avoid races.
                        partialSC[ui] += 1.0;
                    }
                    qCDebug(lcEngine) << "BFS: appending u" << u << " to list Ps[w=" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    pss.Ps[wi].append(u);
                }
            }
            ++it1;
        } // end while (it1...)
    } // end while (!Q.empty())
} // end bfsSSSP()

/**
*	Dijkstra's algorithm for solving the SSSP problem in weighted graphs (directed or not).
*   It uses a min-priority queue prQ to provide constant time lookup of the minimum
*   distance. The priority queue is implemented with std::priority_queue

    INPUT:
        a 'source' vertex with vpos s and a boolean computeCentralities.
        (Implicitly, the algorithm uses the m_graph structure)

    OUTPUT:
        For every vertex t: pss.dist[ti] is set to the distance of each t from s
        For every vertex t: pss.sigma[ti] is set to the number of shortest paths between s and t

        Also, if computeCentralities is true then it does extra operations:
            a) For source vertex s:
                it calculates CC(s) as the sum of its distances from every other vertex.
                it calculates eccentricity(s) as the maximum distance from all other vertices.
                it increases pss.nthOrder[ N ] by one, to store the number of nodes at distance n from source s
            b) For every vertex u:
                it increases SC(u) by one, when it finds a new shor. path from s to t through u.
                appends each neighbor y of u to pss.Ps[y], thus Ps stores all predecessors of y on all shortest paths from s
            c) Each vertex u popped from prQ is pushed to pss.Stack

*/
void DistanceEngine::dijkstraSSSP(const int &s, const int &si,
                                  const bool &computeCentralities,
                                  const bool &inverseWeights,
                                  const bool &dropIsolates,
                                  PerSourceScratch &pss,
                                  QVector<qreal> &partialSC,
                                  const QVector<qreal> &potentials)
{

    Q_UNUSED(dropIsolates);

    int u = 0, ui = 0, w = 0, wi = 0, v = 0, sp_w = 0;
    int relation = 0;
    qreal weight = 0, dist_u = 0, dist_w = 0, cur_dist_w = 0;
    bool edgeStatus = false;
    H_edges::const_iterator it1;
    VList::const_iterator it;

    // Construct a priority queue where we will store discovered vertices along with their distances from source
    qCDebug(lcEngine) << "### dijkstra: Construct a priority queue prQ to store discovered vertices-distances from source";

    // TODO: Check prQ functionality in weighted graphs, where edge weight denotes value (not cost)
    priority_queue<GraphDistance, vector<GraphDistance>, GraphDistancesCompare> prQ;

    // This is used to not allow duplicates in the priority queue (@see issue #123)
    QSet<int> visited_vertices;

    // set d( s, s ) = 0
    pss.dist[si] = 0;

    // set sp ( s , s ) = 1
    pss.sigma[si] = 1;

    for (it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
    {
        v = graph.vertexIndexByNumber((*it)->number());
        if (v != s)
        {
            // NOTE: d(i,j) init to RAND_MAX already done via pss.dist.fill(RAND_MAX)
            //            prQ.push(GraphDistance(v,RAND_MAX));

            // TODO // Previous node in optimal path from source
            //     previous[v]  := undefined
        }
    }
    qCDebug(lcEngine) << "### dijkstra: push s" << s << "to prQ with 0 distance from s";
    // Note: without it the priority prQ would pop arbitrary node at first loop
    prQ.push(GraphDistance(s, 0));

    qCDebug(lcEngine) << "### dijkstra: LOOP: While prQ not empty ";
    while (!prQ.empty())
    {

        qCDebug(lcEngine) << "    *** dijkstra: prQ size: " << prQ.size();

        // Get the first vertex in the priority queue
        u = prQ.top().target;
        // Get the vertex index
        ui = graph.vertexIndexByNumber(u);

        // Pop it
        qCDebug(lcEngine) << "    *** dijkstra: first vertex in prQ is u" << u << "graph.vertexIndexByNumber" << ui
                 << ". It has minimum distance from s " << s << "=" << prQ.top().distance << " Popping it from the queue.";
        prQ.pop();

        if (visited_vertices.contains(u))
        {
            qCDebug(lcEngine) << "    *** dijkstra: vertex already visited. Skipping!";
            continue;
        }
        // Add it to visited
        visited_vertices.insert(u);

        // Skip if that vertex is disabled
        if (!graph.vertexAtIndex(ui)->isEnabled())
        {
            qCDebug(lcEngine) << "    *** dijkstra: vertex disabled. Skipping!";
            continue;
        }

        // Check if we need to compute centralities
        if (computeCentralities)
        {

            qCDebug(lcEngine) << "    *** dijkstra: Compute centralities: pushing u ="
                     << u
                     << " to Stack ";

            pss.Stack.push(u);
        }

        // LOOP over every edge of u
        qCDebug(lcEngine) << "    --- dijkstra: LOOP over every edge of u (" << u << ", w ) e E... ";
        it1 = graph.vertexAtIndex(ui)->outEdges().cbegin();
        while (it1 != graph.vertexAtIndex(ui)->outEdges().cend())
        {

            // Skip if the edge is not of the current relation
            relation = it1.value().first;
            if (relation != graph.relationCurrent())
            {
                ++it1;
                continue;
            }
            // Skip if the edge is disabled
            edgeStatus = it1.value().second.second;
            if (edgeStatus != true)
            {
                ++it1;
                continue;
            }

            // Get the target vertex of this edge and its index
            w = it1.key();
            wi = graph.vertexIndexByNumber(w);

            // Get the edge weight
            weight = it1.value().second.first;

            // Fix #30: zero-weight edges are visual-only and must not be
            // traversed. Normal weights: a zero-weight path would be "free"
            // (dist_w = dist_u + 0), collapsing distances incorrectly.
            // Inverse weights: 1/0 is undefined — potential crash or +inf.
            if (weight == 0)
            {
                ++it1;
                continue;
            }

            qCDebug(lcEngine) << "    --- dijkstra: edge (u, w) = (" << u << "," << w << ") =" << weight;

            // Invert edge weight if the user told us to do so
            if (inverseWeights)
            {
                weight = 1.0 / weight;
                qCDebug(lcEngine) << "    --- dijkstra: inverting weight to " << weight;
            }

            // Johnson's-algorithm reweighting: w'(u,v) = w(u,v) + h(u) - h(v). Applied after
            // inverseWeights above, since potentials reweight the actual edge cost being
            // minimized, not the pre-inversion raw weight. potentials is empty for a plain
            // Dijkstra run, so this is a no-op unless a caller explicitly opted in.
            if (!potentials.isEmpty())
            {
                weight = weight + potentials[ui] - potentials[wi];
                qCDebug(lcEngine) << "    --- dijkstra: reweighted to " << weight;
            }

            // Start path discovery
            qCDebug(lcEngine) << "    --- dijkstra: Start path discovery";

            // Get the distance of u from source
            dist_u = pss.dist[ui];

            // If dist_u not finite, this means that dist_w also not finite
            if (dist_u == RAND_MAX || dist_u < 0)
            {
                dist_w = RAND_MAX;
                qCDebug(lcEngine) << "    --- dijkstra: dist_w = RAND_MAX " << RAND_MAX;
            }
            else
            {
                // dist_u finite, therefore dist_w is (dist_u + edge weight)
                dist_w = dist_u + weight;
                qCDebug(lcEngine) << "    --- dijkstra: dist_w = dist_u + weight = "
                         << dist_u << "+" << weight << "=" << dist_w;
            }

            // Get the currently computed distance of w from source
            cur_dist_w = pss.dist[wi];

            qCDebug(lcEngine) << "    --- dijkstra: RELAXATION: check if dist_w =" << dist_w
                     << "  shorter than current d(s=" << s << ",w=" << w << ")="
                     << cur_dist_w;

            if (distancesNearlyEqual(dist_w, cur_dist_w) && dist_w < RAND_MAX)
            {

                qCDebug(lcEngine) << "    --- dijkstra: dist_w : " << dist_w
                         << " ==  current d(s,w) : " << cur_dist_w;

                sp_w = pss.sigma[wi] + pss.sigma[ui];

                // This branch only runs when dist_w == cur_dist_w, i.e. (u,w) is confirmed to lie
                // on a shortest path from s to w that ties the current best - so accumulating
                // sigma(s,u) into sigma(s,w) here is correct, not speculative.
                qCDebug(lcEngine) << "    --- dijkstra: Found ANOTHER SP from s ="
                         << s
                         << " to w=" << w << " via u=" << u
                         << " - Setting Sigma(s, w) = " << sp_w;

                if (s != w)
                {
                    pss.sigma[wi] = sp_w;
                }

                if (computeCentralities)
                {
                    if (s != w && s != u && u != w)
                    {
                        qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: partialSC[ui=" << ui << "] += 1";
                        // Intermediate vertex: use partialSC to avoid race with other threads.
                        partialSC[ui] += 1.0;
                    }
                    else
                    {
                        qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: "
                                    "Skipping SC of u, because s="
                                 << s << " w=" << w << " u=" << u;
                    }

                    qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: "
                                "Appending u="
                             << u << " to list Ps[w =" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    pss.Ps[wi].append(u);
                }
            }

            // Reached only when the tie check above didn't match, so dist_w is guaranteed to be
            // outside distancesNearlyEqual()'s tolerance of cur_dist_w here - a genuine strict
            // improvement, not a near-tie that happens to round slightly lower.
            //
            // >= 0, not > 0: dist_w == 0 for w != s is impossible under plain Dijkstra (the
            // #30 fix above already skips every zero-weight edge, so a non-source vertex can
            // never land at exactly 0), but is a legitimate relaxed distance once Johnson's
            // reweighting is in play - w'(u,v) = w(u,v) + h(u) - h(v) is only guaranteed >= 0,
            // not > 0, so an ordinary positive-weight edge can reweight to exactly 0. A strict
            // dist_w > 0 here silently drops that relaxation, which is a real, pre-existing bug
            // this reweighting path is the first thing to actually reach.
            else if (dist_w >= 0 && dist_w < cur_dist_w)
            {

                qCDebug(lcEngine) << "    --- dijkstra: dist_w " << dist_w
                         << " <  current d(s,w) =" << cur_dist_w
                         << " Pushing w" << w << "to prQ with distance" << dist_w << "from s" << s;

                // FIXME: w might have been already visited?
                // If so, we might use QMap<int> which is sorted (minimum)
                // and also provides contain()
                prQ.push(GraphDistance(w, dist_w));

                pss.dist[wi] = dist_w;

                // Accumulate into scratch instead of calling graph.incGeodesicsCount() /
                // graph.setDiameterCached() directly — those are not thread-safe.
                ++pss.sourceGeodesicsCount;

                qCDebug(lcEngine) << "    --- dijkstra: "
                            "Set d ( s="
                         << s << ", w=" << w
                         << " ) = " << dist_w << "=" << pss.dist[wi];

                if (s != w)
                {
                    qCDebug(lcEngine) << "    --- dijkstra: "
                                "Found NEW shortest path from s ="
                             << s
                             << " to w =" << w << " via u =" << u
                             << " - Setting Sigma(s, w) = Sigma(s, u) =" << pss.sigma[ui];
                    // w's only shortest path so far is through u, so w inherits u's shortest-
                    // path count - not a hardcoded 1, which silently discards u's own tie count
                    // whenever u itself was reached via more than one tied shortest path.
                    pss.sigma[wi] = pss.sigma[ui];
                }

                if (computeCentralities)
                {

                    pss.nthOrderIncrement(dist_w);

                    qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: "
                                "For PC: nthOrder: number of nodes at distance "
                             << dist_w << "from s is "
                             << pss.nthOrder.value(dist_w, 0);

                    if (graph.vertexAtIndex(si)->eccentricity() < dist_w)
                    {
                        graph.vertexAtIndex(si)->setEccentricity(dist_w);
                        qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: "
                                    "For EC: max distance ="
                                 << graph.vertexAtIndex(si)->eccentricity();
                    }

                    qCDebug(lcEngine) << "    --- dijkstra: Compute Centralities: "
                                "Resetting Ps[w =" << w << "] to [u =" << u
                             << "], the sole predecessor of w on the new strictly-shorter "
                                "path from s - any predecessor recorded here from a prior, "
                                "now-superseded relaxation of w must not survive.";
                    pss.Ps[wi] = QList<int>{u};
                }
            }
            else
            {
                qCDebug(lcEngine) << "    --- dijkstra: "
                            "NOT a new SP";
            }

            ++it1;

        } // END loop for every outEdge of u

        qCDebug(lcEngine) << "    --- dijkstra: LOOP END over every edge (" << u << ", w ) e E... ";

    } // END loop while prQ not empty

    qCDebug(lcEngine) << "### dijkstra: LOOP END. prQ is empty - Returning.";
} // END dijkstraSSSP()
