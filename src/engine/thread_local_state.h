/**
 * @file thread_local_state.h
 * @brief Per-thread state for the parallel SSSP source loop in DistanceEngine.
 *
 * During Phase 2 parallelisation (QtConcurrent::blockingMap over source vertices),
 * each worker thread owns one ThreadLocalState.  It holds:
 *  - A PerSourceScratch (reused across all sources the thread processes).
 *  - Partial BC and SC accumulator arrays — intermediate vertices are shared across
 *    sources, so BC/SC are NOT written to GraphVertex directly inside the loop.
 *    Instead each thread accumulates into partialBC / partialSC, and a single-threaded
 *    reduction step at the end writes the totals to the vertex objects.
 *  - Running totals for graph-wide aggregates (distance sum, geodesics count, diameter,
 *    PC/SPC sums) that would otherwise require a mutex around every graph call.
 *
 * Lifecycle:
 *   allocate(totalV)   — called once per thread before the parallel loop
 *   (partialBC / partialSC are zeroed at allocation; pss is reset per source)
 *   Post-loop reduction reads totalDistanceSum, totalGeodesicsCount, maxDiameter,
 *   totalSumPC, totalSumSPC, partialBC[*], partialSC[*] and merges into graph state.
 */

#ifndef SOCNETV_THREAD_LOCAL_STATE_H
#define SOCNETV_THREAD_LOCAL_STATE_H

#include "engine/per_source_scratch.h"

#include <QVector>
#include <QtGlobal>

struct ThreadLocalState
{
    // Reused SSSP scratch — reset once per source via pss.resetPerSource().
    PerSourceScratch pss;

    // Partial betweenness centrality sums, indexed by vertex position.
    // Each source contributes pss.delta[wi] for every intermediate vertex w != s
    // instead of calling vertex->setBC() directly (which would race across threads).
    // Post-loop: vertex[wi]->setBC( sum over all threads of partialBC[wi] )
    QVector<qreal> partialBC;

    // Partial stress centrality sums, indexed by vertex position.
    // Each new shortest path through vertex ui increments partialSC[ui] by 1
    // instead of calling vertex->setSC() directly.
    // Post-loop: vertex[ui]->setSC( sum over all threads of partialSC[ui] )
    QVector<qreal> partialSC;

    // Sum of pss.sourceDistanceSum across all sources this thread has processed.
    // Originates from BFS inner-loop "dist_w" accumulation (not used by Dijkstra).
    // Reduced into graph.addToDistanceSum() after the parallel loop.
    qreal totalDistanceSum = 0;

    // Sum of per-source distances_sum_for_s (computeCentralities path only).
    // This is the CC-denominator sum that also goes into graphSumDistance.
    // Reduced into graph.addToDistanceSum() after the parallel loop.
    qreal totalCCDistanceSum = 0;

    // Total geodesic-path count across all sources this thread has processed.
    // Replaces repeated graph.incGeodesicsCount() calls inside BFS / Dijkstra.
    // Reduced via graph.addGeodesicsCount() after the parallel loop.
    int totalGeodesicsCount = 0;

    // Maximum geodesic distance (diameter) seen by this thread.
    // Replaces graph.setDiameterCached() inside BFS / Dijkstra.
    // Post-loop: graph.setDiameterCached( max over all threads of maxDiameter )
    int maxDiameter = 0;

    // Accumulated Power Centrality and Standardised Power Centrality sums.
    // Replaces direct graph.sumPC += and graph.sumSPC += inside the source loop
    // (those graph members are public but would race across threads).
    // Reduced into graph.sumPC and graph.sumSPC after the parallel loop.
    qreal totalSumPC  = 0;
    qreal totalSumSPC = 0;

    // Initialise all per-vertex arrays to totalV slots.
    // Must be called once per thread before the parallel source loop starts.
    void allocate(int totalV)
    {
        pss.allocate(totalV);
        partialBC.fill(0.0, totalV);
        partialSC.fill(0.0, totalV);
    }
};

#endif // SOCNETV_THREAD_LOCAL_STATE_H
