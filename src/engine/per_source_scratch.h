/**
 * @file per_source_scratch.h
 * @brief Per-source scratch state for the Brandes SSSP / centrality computation.
 *
 * All mutable state that is local to a single source-vertex iteration of the
 * DistanceEngine source loop lives here.  Moving it out of Graph and GraphVertex
 * is WS3 Phase 1; making it thread-local enables Phase 2 parallelism.
 *
 * Dead Graph/GraphVertex members (Stack, sizeOfNthOrderNeighborhood,
 * sizeOfComponent, myPs, m_delta) are kept intact until Phase 2 passes the
 * regression harness, then removed.
 */

#ifndef SOCNETV_PER_SOURCE_SCRATCH_H
#define SOCNETV_PER_SOURCE_SCRATCH_H

#include <stack>
#include <QHash>
#include <QList>
#include <QVector>
#include <QtGlobal>
#include <cstdlib>  // RAND_MAX

struct PerSourceScratch
{
    // Brandes traversal stack — vertices ordered by non-increasing distance from s
    std::stack<int> Stack;

    // Predecessor lists: Ps[vi] = list of vertices that precede vertex vi
    // on shortest paths from the current source s.  Indexed by vertex position.
    QVector<QList<int>> Ps;

    // Dependency accumulators for Brandes BC back-propagation.
    // Indexed by vertex position.
    QVector<qreal> delta;

    // BFS / Dijkstra distances from current source s.
    // dist[vi] == RAND_MAX means vertex vi has not been reached.
    // Indexed by vertex position.
    QVector<qreal> dist;

    // Shortest-path counts (sigma) from current source s.
    // Indexed by vertex position.
    QVector<int> sigma;

    // Number of vertices at each distance order from s (used for Power Centrality).
    // Key = distance, Value = count.
    QHash<qreal, int> nthOrder;

    // Size of the connected component reachable from s (used for SPC normalisation).
    int componentSize = 0;

    // Per-source graph-aggregate accumulators.
    // Phase 2: SSSP functions write here instead of calling graph.addToDistanceSum() /
    // graph.incGeodesicsCount() / graph.setDiameterCached() directly, which would be
    // unsafe when multiple threads process different sources concurrently.
    // The owning thread accumulates these into ThreadLocalState totals after each source,
    // and the post-loop reduction merges them into the graph-level aggregates.
    qreal sourceDistanceSum    = 0;  // replaces graph.addToDistanceSum(dist_w) in BFS
    int   sourceGeodesicsCount = 0;  // replaces graph.incGeodesicsCount() in BFS / Dijkstra
    int   sourceDiameter       = 0;  // replaces graph.setDiameterCached() in BFS / Dijkstra

    // Allocate all containers once for totalVertices positions.
    // Call this once before the source loop.
    void allocate(int totalVertices)
    {
        Ps.resize(totalVertices);
        delta.resize(totalVertices);
        dist.resize(totalVertices);
        sigma.resize(totalVertices);
    }

    // Reset per-source state before each new source vertex.
    // computeCentralities controls whether the Brandes-only structures are reset.
    void resetPerSource(bool computeCentralities)
    {
        dist.fill((qreal)RAND_MAX);
        sigma.fill(0);
        // Reset per-source graph-aggregate accumulators so they reflect only this source.
        sourceDistanceSum    = 0;
        sourceGeodesicsCount = 0;
        sourceDiameter       = 0;
        if (computeCentralities)
        {
            while (!Stack.empty())
                Stack.pop();
            for (auto &ps : Ps)
                ps.clear();
            nthOrder.clear();
        }
        // delta is zeroed in the per-source back-propagation setup loop,
        // not here, to preserve the existing guard on computeCentralities.
    }

    // Increment the nth-order neighbourhood count for distance d.
    void nthOrderIncrement(qreal d)
    {
        nthOrder.insert(d, nthOrder.value(d, 0) + 1);
    }
};

#endif // SOCNETV_PER_SOURCE_SCRATCH_H
