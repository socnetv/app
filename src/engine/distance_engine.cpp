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

#include <QDebug>
#include <cstdlib>
#include <queue>

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
};

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
 */
void DistanceEngine::compute(const bool computeCentralities,
                             const bool considerWeights,
                             const bool inverseWeights,
                             const bool dropIsolates)
{

    qDebug() << "DistanceEngine::compute() - "
             << "centralities" << computeCentralities
             << "considerWeights:" << considerWeights
             << "inverseWeights:" << inverseWeights
             << "dropIsolates:" << dropIsolates;

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

    GraphDistanceProgressSink sink(graph);

    // ---- Phase 0/Init (includes E==0 handling) ----
    initRun(computeCentralities,
            considerWeights,
            inverseWeights,
            dropIsolates,
            ds,
            csssp,
            csfin,
            sink);

    if (ds.E != 0)
    {
        // ---- Phase 1+2: SSSP loop + per-source accumulation ----
        runAllSources(computeCentralities,
                      considerWeights,
                      inverseWeights,
                      dropIsolates,
                      ds,
                      csssp,
                      sink);
        if (sink.progressCanceled())
        {
            qDebug() << "DistanceEngine::compute() - canceled. Skipping finalize.";
            sink.progressKill();
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

    qDebug() << "Graph::graphDistancesGeodesic()- FINISHED computing distances";

    sink.progressKill();
}

void DistanceEngine::initRun(const bool computeCentralities,
                             const bool considerWeights,
                             const bool inverseWeights,
                             const bool dropIsolates,
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
    sink.progressCreate(ds.N, ds.pMsg);

    graph.setSymmetricCached(graph.isSymmetric());

    if (ds.E == 0)
    {
        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            for (ds.it1 = graph.verticesBegin(); ds.it1 != graph.verticesEnd(); ++ds.it1)
            {
                // Set all pair-wise distances to RAND_MAX
                (*ds.it)->setDistance((*ds.it1)->number(), RAND_MAX);
                // Set all pair-wise shortest-path counts (sigmas) to 0
                (*ds.it)->setShortestPaths((*ds.it1)->number(), 0);
            }
        }
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

        graph.setConnectedCached(true);

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
                // All pair-wise distances are set to RAND_MAX by default
                // inside GraphVertex::distance()
                // so we don't need to explicitly set them here.
                // We just clear distance hashmap of each actor.
                (*ds.it)->clearDistance();
                // Set all pair-wise shortest-path counts (sigmas) to 0
                // (*it)->setShortestPaths((*it1)->number(), 0);
                (*ds.it)->clearShortestPaths();

                if (considerWeights && inverseWeights)
                {
                    // find the max weight in the network.
                    // it will be used for maxCC below
                    ds.tempEdgeWeight = (*ds.it)->hasEdgeTo((*ds.it1)->number());
                    if (ds.tempEdgeWeight > ds.maxEdgeWeightInNetwork)
                    {
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

void DistanceEngine::runAllSources(const bool computeCentralities,
                                   const bool considerWeights,
                                   const bool inverseWeights,
                                   const bool dropIsolates,
                                   DistanceScratch &ds,
                                   CentralityScratchSSSP &csssp,
                                   IDistanceProgressSink &sink)
{
    qDebug() << "*********** MAIN LOOP: "
                "for every s in V solve the Single Source Shortest Path (SSSP) problem...";

    // Count total VList positions (includes disabled vertices) for scratch sizing.
    int totalV = 0;
    for (auto it = graph.verticesBegin(); it != graph.verticesEnd(); ++it)
        ++totalV;

    // Allocate per-source scratch once; resetPerSource() is called each iteration.
    PerSourceScratch pss;
    pss.allocate(totalV);

    for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
    {
        ds.s = (*ds.it)->number();
        ds.si = graph.vertexIndexByNumber(ds.s);
        ds.distances_sum_for_s = 0;

        qDebug() << "***** PHASE 1 (SSSP): "
                 << "Source vertex s" << ds.s << "vpos" << ds.si;

        sink.progressUpdate(++ds.progressCounter);
        if (sink.progressCanceled())
        {
            qDebug() << "DistanceEngine::runAllSources() - canceled by user. Aborting.";
            return;
        }
        if (!(*ds.it)->isEnabled())
        {
            qDebug() << "***** PHASE 1 (SSSP): s" << ds.s << "disabled. SKIP/CONTINUE";
            continue;
        }

        // Reset scratch for this source.
        // dist and sigma are always reset; Stack/Ps/nthOrder only when needed.
        pss.resetPerSource(computeCentralities);

        if (computeCentralities)
        {
            qDebug() << "***** PHASE 1 (SSSP): "
                        "Empty Stack which will return vertices in "
                        "order of their (non increasing) distance from s ...";
            qDebug() << "***** PHASE 1 (SSSP): "
                        "...and for each vertex: predecessor list Ps cleared in resetPerSource";
        }

        qDebug() << "***** PHASE 1 (SSSP): "
                    "Call BFS or dijkstra for s"
                 << ds.s << " vpos " << ds.si
                 << " to compute distance and shortest paths to every vertex t";

        if (!considerWeights)
        {
            bfsSSSP(ds.s, ds.si, computeCentralities, dropIsolates, pss);
        }
        else
        {
            dijkstraSSSP(ds.s, ds.si, computeCentralities, inverseWeights, dropIsolates, pss);
        }

        qDebug() << "***** PHASE 1 (SSSP): "
                    "FINISHED BFS / DIJKSTRA ALGORITHM. "
                    "Writing back APSP results to vertex and continuing to calculate centralities";

        // Write-back: persist pss.dist and pss.sigma into vertex si's m_distance /
        // m_shortestPaths so that finalize() and external callers see correct values.
        // Only reachable vertices (dist != RAND_MAX) are written; the vertex hash
        // default already returns RAND_MAX for missing entries.
        for (int vi = 0; vi < totalV; ++vi)
        {
            if (pss.dist[vi] != (qreal)RAND_MAX)
            {
                int vnum = graph.vertexAtIndex(vi)->number();
                graph.vertexAtIndex(ds.si)->setDistance(vnum, pss.dist[vi]);
                graph.vertexAtIndex(ds.si)->setShortestPaths(vnum, pss.sigma[vi]);
            }
        }

        if (computeCentralities)
        {
            qDebug() << "***** PHASE 2 (CENTRALITIES): "
                        "s"
                     << ds.s << "vpos" << ds.si << "CC" << csssp.CC;

            // Compute Power Centrality
            // In = [ 1/(N-1) ] * ( Nd1 + Nd2 * 1/2 + ... + Ndi * 1/i )
            // where
            // Ndi (nthOrder) is the number of nodes at distance i from this node.
            // N is the sum Nd0 + Nd1 + Nd2 + ... + Ndi, that is the amount of nodes in the same component as the current node

            pss.componentSize = 1;
            csssp.PC = 0;
            csssp.hfi = pss.nthOrder.constBegin();
            // FIXME do we need to check for disabled nodes somewhere?
            while (csssp.hfi != pss.nthOrder.constEnd())
            {
                qDebug() << " nthOrder.value(" << csssp.hfi.key() << ")"
                         << csssp.hfi.value();
                csssp.PC += (1.0 / csssp.hfi.key()) * csssp.hfi.value();
                pss.componentSize += csssp.hfi.value();
                ++csssp.hfi;
            }

            (*ds.it)->setPC(csssp.PC);
            graph.sumPC += csssp.PC;
            if (pss.componentSize != 1)
                csssp.SPC = (1.0 / (pss.componentSize - 1.0)) * csssp.PC;
            else
                csssp.SPC = 0;

            (*ds.it)->setSPC(csssp.SPC); // Set std PC

            graph.sumSPC += csssp.SPC; // add to sumSPC -- used later to compute mean and variance

            qDebug() << "***** PHASE 2 (CENTRALITIES): "
                        "s"
                     << ds.s << "vpos" << ds.si << "PC" << csssp.PC;

            // Compute Betweenness Centrality

            qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                        "Start back propagation of dependencies."
                     << "Reset dependency delta[vi]=0 on each vertex and compute distance sum";

            for (ds.it1 = graph.verticesBegin(); ds.it1 != graph.verticesEnd(); ++ds.it1)
            {
                int vi1 = graph.vertexIndexByNumber((*ds.it1)->number());
                pss.delta[vi1] = 0.0;

                // Accumulate sum of distances from source s to every other vertex.
                // RAND_MAX for unreachable vertices propagates the disconnected-graph
                // sentinel through to the CC computation below.
                ds.distances_sum_for_s += pss.dist[vi1];

                qDebug() << "    Compute Centralities: "
                            "For CC: sum of distances. dist["
                         << vi1 << "] = " << pss.dist[vi1]
                         << "new sum of distances for s =" << ds.distances_sum_for_s;
            }
            qDebug() << "    Compute Centralities: "
                        "For CC: total sum of distances for s ="
                     << ds.distances_sum_for_s;

            graph.addToDistanceSum(ds.distances_sum_for_s);

            // Compute Closeness Centrality
            if (ds.distances_sum_for_s != 0 && ds.distances_sum_for_s < RAND_MAX)
            {
                // Connected actor:
                // There is a path from this actor to all others
                // Invert the sum of distances and set it as CC
                csssp.CC = 1.0 / ds.distances_sum_for_s;
            }
            else
            {
                // Not connected actor. Cases:
                // a) Isolated: The actor has no outbound links
                // b) Disconnected graph: There is no path from this actor
                // to some of the other actors, which means her distance to
                // them is infinite
                // For these two cases, set CC as zero.
                csssp.CC = 0;
            }
            (*ds.it)->setCC(csssp.CC);

            qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                        "Visit all vertices in reverse order of their discovery (from s = "
                     << ds.s
                     << " ) to sum dependencies. Initial Stack size " << (int)pss.Stack.size();

            while (!pss.Stack.empty())
            {
                ds.w = pss.Stack.top();
                ds.wi = graph.vertexIndexByNumber(ds.w);

                qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                            "Stack top is vertex w "
                         << ds.w
                         << "This is the furthest vertex from s. Popping it.";

                pss.Stack.pop();
                const QList<int> &lst = pss.Ps[ds.wi];

                qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                            "preLOOP: Size of predecessors list Ps[w]"
                         << lst.size();

                qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                            "LOOP over every vertex u in Ps of w"
                         << ds.w;

                if (lst.size() > 0) // just in case...do a sanity check
                    for (ds.it2 = lst.cbegin(); ds.it2 != lst.cend(); ds.it2++)
                    {
                        ds.u = (*ds.it2);
                        ds.ui = graph.vertexIndexByNumber(ds.u);
                        csssp.sigma_u = pss.sigma[ds.ui];
                        csssp.sigma_w = pss.sigma[ds.wi];
                        csssp.delta_u = pss.delta[ds.ui];
                        csssp.delta_w = pss.delta[ds.wi];

                        qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                                    "Selecting Ps[w] element u"
                                 << ds.u
                                 << "with delta_u" << csssp.delta_u
                                 << "sigma(s,u)" << csssp.sigma_u
                                 << "sigma(s,w)" << csssp.sigma_w
                                 << "delta_w" << csssp.delta_w;

                        if (pss.sigma[ds.wi] > 0)
                        {
                            // delta[u]=delta[u]+(1+delta[w])*(sigma[u]/sigma[w]) ;
                            csssp.d_su = csssp.delta_u + (1.0 + csssp.delta_w) * ((qreal)csssp.sigma_u / (qreal)csssp.sigma_w);
                        }
                        else
                        {
                            csssp.d_su = csssp.delta_u;
                            qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                                        "zero shortest paths from s to w - "
                                        "using SAME DELTA for vertex u";
                        }
                        qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                                    "Assigning new delta d_su"
                                 << csssp.d_su
                                 << " to u" << ds.u;

                        pss.delta[ds.ui] = csssp.d_su;

                    } // end for

                qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                            "Adding delta_w to BC of w";

                if (ds.w != ds.s)
                {
                    qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                                "w!=s. For this furthest vertex we need to add its new delta"
                             << csssp.delta_w
                             << "to old BC index:"
                             << graph.vertexAtIndex(ds.wi)->BC();

                    csssp.d_sw = graph.vertexAtIndex(ds.wi)->BC() + pss.delta[ds.wi];

                    qDebug() << "***** PHASE 2 (BC/ACCUMULATION): "
                                "s"
                             << ds.s << "vpos" << ds.si << "BC = d_sw" << csssp.d_sw;

                    graph.vertexAtIndex(ds.wi)->setBC(csssp.d_sw);

                } // END if
            } // END while stack
        } // END if computeCentralities

    } // END for SSSP problem

    qDebug() << "*********** MAIN LOOP (SSSP problem): FINISHED.";
}

void DistanceEngine::finalize(const bool computeCentralities,
                              const bool dropIsolates,
                              DistanceScratch &ds,
                              CentralityScratchFinalize &csf,
                              IDistanceProgressSink &sink)
{
    // check if there are disconnected nodes
    // and get the distance sums
    qDebug() << "Checking if there are disconnected nodes";

    graph.setConnectedCached(true);

    for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
    {
        if (!(*ds.it)->isEnabled())
        {
            qDebug() << "actor i" << (*ds.it)->number() << "disabled. SKIP/CONTINUE";
            continue;
        }

        ds.pairDistance = 0;

        for (ds.it1 = graph.verticesBegin(); ds.it1 != graph.verticesEnd(); ++ds.it1)
        {
            if (!(*ds.it1)->isEnabled())
            {
                qDebug() << "   actor j" << (*ds.it1)->number() << "disabled. SKIP/CONTINUE";
                continue;
            }
            if ((*ds.it1)->number() == (*ds.it)->number())
            {
                qDebug() << "   == actor j" << (*ds.it1)->number() << "SKIP/CONTINUE";
                continue;
            }

            ds.pairDistance = (*ds.it)->distance((*ds.it1)->number());

            if (ds.pairDistance == RAND_MAX)
            {
                graph.notConnectedPairsInsert((*ds.it)->number(), (*ds.it1)->number());
                (*ds.it)->setEccentricity(RAND_MAX);
                graph.setConnectedCached(false);

                qDebug() << "actor i" << (*ds.it)->number()
                         << "has infinite eccentricity. "
                            "There is no path from it to actor j"
                         << (*ds.it1)->number();
            }
            else
            {
                qDebug() << "actor i" << (*ds.it)->number()
                         << "distanceSum" << (*ds.it)->distanceSum();
                (*ds.it)->setDistanceSum((*ds.it)->distanceSum() + ds.pairDistance);
            }
        } // end for

        qDebug() << "actor i" << (*ds.it)->number()
                 << "Final distanceSum" << (*ds.it)->distanceSum();

        if (computeCentralities)
        {
            // Compute Eccentricity (max geodesic distance)
            csf.eccentricity = (*ds.it)->eccentricity();

            qDebug() << "actor"
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

                qDebug() << "actor i" << (*ds.it)->number()
                         << "EC"
                         << csf.EC;
            }
            else
            {
                csf.EC = 0;
                (*ds.it)->setEC(csf.EC);  // Set Eccentricity Centrality
                (*ds.it)->setSEC(csf.EC); // Set std EC = EC
                graph.sumEC += csf.EC;    // set sum EC

                qDebug() << "actor i" << (*ds.it)->number()
                         << "EC=0 (disconnected graph)";
            }

        } // end if compute centralities

    } // end for disconnected checking

    // Compute average path length...
    if (graph.notConnectedPairsSize() == 0)
    {
        graph.setAverageDistanceCached(graph.graphSumDistanceCached() / (ds.N * (ds.N - 1.0)));
        qDebug() << "Graph::graphDistancesGeodesic() - Average distance:"
                 << graph.graphDistanceGeodesicAverageCached();
    }
    else
    {
        // TODO In not connected nets, it would be nice to ask the user what to do
        //  with unconnected pairs (make M or drop (default?)
        qDebug() << "Graph::graphDistancesGeodesic() - Average distance:"
                 << graph.graphDistanceGeodesicAverageCached();
        graph.setAverageDistanceCached(graph.graphSumDistanceCached() / graph.graphGeodesicsCountCached());
    }

    if (computeCentralities)
    {
        qDebug() << "Graph: graphDistancesGeodesic() - "
                    "Computing centralities...";
        for (ds.it = graph.verticesBegin(); ds.it != graph.verticesEnd(); ++ds.it)
        {
            if (dropIsolates && (*ds.it)->isIsolated())
            {
                qDebug() << "vertex " << (*ds.it)->number()
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
                qDebug() << "Betweenness centrality must be divided by"
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
                qDebug() << "SC of " << (*ds.it)->number()
                         << "  divided by 2 (because the graph is symmetric) "
                         << (*ds.it)->SC();
            }
            graph.sumSC += csf.SC;

            qDebug() << "vertex " << (*ds.it)->number() << " - "
                     << " EC: " << (*ds.it)->EC()
                     << " CC: " << (*ds.it)->CC()
                     << " BC: " << (*ds.it)->BC()
                     << " SC: " << (*ds.it)->SC()
                     << " PC: " << (*ds.it)->PC();
        } // end for

        qDebug() << "Graph: graphDistancesGeodesic() -"
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

        qDebug() << "Graph: graphDistancesGeodesic() - "
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
                             PerSourceScratch &pss)
{
    Q_UNUSED(dropIsolates);

    qDebug() << "BFS:";
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

    qDebug() << "BFS: LOOP: While Q not empty ";
    while (!Q.empty())
    {

        u = Q.front();
        Q.pop();
        ui = graph.vertexIndexByNumber(u);
        qDebug() << "BFS: Dequeue: first element of Q is u" << u << "graph.vertexIndexByNumber" << ui;

        if (!graph.vertexAtIndex(ui)->isEnabled())
        {
            continue;
        }

        if (computeCentralities)
        {
            qDebug() << "BFS: Compute centralities: Pushing u" << u
                     << "to Stack ";
            pss.Stack.push(u);
        }
        qDebug() << "BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u";
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
            qDebug("BFS: u=%i is connected with node w=%i of graph.vertexIndexByNumber wi=%i. ", u, w, wi);

            qDebug("BFS: Start path discovery");

            // if dist[wi] is RAND_MAX, w is found for the first time.
            if (pss.dist[wi] == (qreal)RAND_MAX)
            {

                qDebug("BFS: First time visiting w=%i. Enqueuing w to the end of Q", w);

                Q.push(w);

                dist_u = (int)pss.dist[ui];
                dist_w = dist_u + 1;

                qDebug() << "BFS: Setting dist_w = d ( s" << s << ", w" << w
                         << ") equal to dist_u=d(s,u) plus 1. New dist_w" << dist_w;

                pss.dist[wi] = (qreal)dist_w;

                graph.addToDistanceSum(dist_w);
                graph.incGeodesicsCount();

                qDebug() << "== BFS  - d("
                         << s << "," << w
                         << ")=" << pss.dist[wi];

                if (computeCentralities)
                {
                    qDebug() << "BFS: Calculate PC: store the number of nodes at distance "
                             << dist_w << "from s";

                    pss.nthOrderIncrement((qreal)dist_w);
                    qDebug() << "BFS: Calculate CC: the sum of distances (will invert it l8r)";
                    graph.vertexAtIndex(si)->setCC(graph.vertexAtIndex(si)->CC() + dist_w);

                    qDebug() << "BFS: Calculate Eccentricity: the maximum distance ";
                    if (graph.vertexAtIndex(si)->eccentricity() < dist_w)
                        graph.vertexAtIndex(si)->setEccentricity(dist_w);
                }
                if (dist_w > graph.graphDiameterCached())
                {
                    graph.setDiameterCached(dist_w);
                }
            }

            qDebug() << "BFS: Start path counting";

            // Is edge (u,w) on a shortest path from s to w via u?
            if ((int)pss.dist[wi] == (int)pss.dist[ui] + 1)
            {

                temp = pss.sigma[wi] + pss.sigma[ui];

                qDebug() << "BFS: Found a NEW SHORTEST PATH from s" << s
                         << "to w" << w << "via u" << u
                         << "Setting Sigma(s, w)" << temp;
                if (s != w)
                {
                    pss.sigma[wi] = temp;
                }
                if (computeCentralities)
                {
                    qDebug() << "BFS/SC: Computing centralities: Computing SC ";
                    if (s != w && s != u && u != w)
                    {
                        qDebug() << "BFS: setSC of u=" << u << " to " << graph.vertexAtIndex(ui)->SC() + 1;
                        graph.vertexAtIndex(ui)->setSC(graph.vertexAtIndex(ui)->SC() + 1);
                    }
                    qDebug() << "BFS: appending u" << u << " to list Ps[w=" << w
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
                                  PerSourceScratch &pss)
{

    Q_UNUSED(dropIsolates);

    int u = 0, ui = 0, w = 0, wi = 0, v = 0, sp_w = 0;
    int relation = 0;
    qreal weight = 0, dist_u = 0, dist_w = 0, cur_dist_w = 0;
    bool edgeStatus = false;
    H_edges::const_iterator it1;
    VList::const_iterator it;

    // Construct a priority queue where we will store discovered vertices along with their distances from source
    qDebug() << "### dijkstra: Construct a priority queue prQ to store discovered vertices-distances from source";

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
    qDebug() << "### dijkstra: push s" << s << "to prQ with 0 distance from s";
    // Note: without it the priority prQ would pop arbitrary node at first loop
    prQ.push(GraphDistance(s, 0));

    qDebug() << "### dijkstra: LOOP: While prQ not empty ";
    while (!prQ.empty())
    {

        qDebug() << "    *** dijkstra: prQ size: " << prQ.size();

        // Get the first vertex in the priority queue
        u = prQ.top().target;
        // Get the vertex index
        ui = graph.vertexIndexByNumber(u);

        // Pop it
        qDebug() << "    *** dijkstra: first vertex in prQ is u" << u << "graph.vertexIndexByNumber" << ui
                 << ". It has minimum distance from s " << s << "=" << prQ.top().distance << " Popping it from the queue.";
        prQ.pop();

        if (visited_vertices.contains(u))
        {
            qDebug() << "    *** dijkstra: vertex already visited. Skipping!";
            continue;
        }
        // Add it to visited
        visited_vertices.insert(u);

        // Skip if that vertex is disabled
        if (!graph.vertexAtIndex(ui)->isEnabled())
        {
            qDebug() << "    *** dijkstra: vertex disabled. Skipping!";
            continue;
        }

        // Check if we need to compute centralities
        if (computeCentralities)
        {

            qDebug() << "    *** dijkstra: Compute centralities: pushing u ="
                     << u
                     << " to Stack ";

            pss.Stack.push(u);
        }

        // LOOP over every edge of u
        qDebug() << "    --- dijkstra: LOOP over every edge of u (" << u << ", w ) e E... ";
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

            qDebug() << "    --- dijkstra: edge (u, w) = (" << u << "," << w << ") =" << weight;

            // Invert edge weight if the user told us to do so
            if (inverseWeights)
            {
                weight = 1.0 / weight;
                qDebug() << "    --- dijkstra: inverting weight to " << weight;
            }

            // Start path discovery
            qDebug() << "    --- dijkstra: Start path discovery";

            // Get the distance of u from source
            dist_u = pss.dist[ui];

            // If dist_u not finite, this means that dist_w also not finite
            if (dist_u == RAND_MAX || dist_u < 0)
            {
                dist_w = RAND_MAX;
                qDebug() << "    --- dijkstra: dist_w = RAND_MAX " << RAND_MAX;
            }
            else
            {
                // dist_u finite, therefore dist_w is (dist_u + edge weight)
                dist_w = dist_u + weight;
                qDebug() << "    --- dijkstra: dist_w = dist_u + weight = "
                         << dist_u << "+" << weight << "=" << dist_w;
            }

            // Get the currently computed distance of w from source
            cur_dist_w = pss.dist[wi];

            qDebug() << "    --- dijkstra: RELAXATION: check if dist_w =" << dist_w
                     << "  shorter than current d(s=" << s << ",w=" << w << ")="
                     << cur_dist_w;

            if ((dist_w == cur_dist_w) && dist_w < RAND_MAX)
            {

                qDebug() << "    --- dijkstra: dist_w : " << dist_w
                         << " ==  current d(s,w) : " << cur_dist_w;

                sp_w = pss.sigma[wi] + pss.sigma[ui];

                // WRONG! We do not know for sure that we are in a shortest path!!!
                qDebug() << "    --- dijkstra: (POSSIBLE BUG?) Found ANOTHER SP from s ="
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

                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                    "setSC of u"
                                 << u
                                 << "to" << graph.vertexAtIndex(ui)->SC() + 1;

                        graph.vertexAtIndex(ui)->setSC(graph.vertexAtIndex(ui)->SC() + 1);
                    }
                    else
                    {
                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                    "Skipping setSC of u, because s="
                                 << s << " w=" << w << " u=" << u;
                    }
                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "SC is "
                             << graph.vertexAtIndex(ui)->SC();

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "Appending u="
                             << u << " to list Ps[w =" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    pss.Ps[wi].append(u);
                }
            }

            else if (dist_w > 0 && dist_w < cur_dist_w)
            {

                qDebug() << "    --- dijkstra: dist_w " << dist_w
                         << " <  current d(s,w) =" << cur_dist_w
                         << " Pushing w" << w << "to prQ with distance" << dist_w << "from s" << s;

                // FIXME: w might have been already visited?
                // If so, we might use QMap<int> which is sorted (minimum)
                // and also provides contain()
                prQ.push(GraphDistance(w, dist_w));

                pss.dist[wi] = dist_w;

                graph.incGeodesicsCount();

                qDebug() << "    --- dijkstra: "
                            "Set d ( s="
                         << s << ", w=" << w
                         << " ) = " << dist_w << "=" << pss.dist[wi];

                if (dist_w > graph.graphDiameterCached())
                {
                    graph.setDiameterCached(dist_w);

                    qDebug() << "    --- dijkstra: "
                                "New graph diameter ="
                             << graph.graphDiameterCached();
                }

                if (s != w)
                {
                    qDebug() << "    --- dijkstra: "
                                "Found NEW shortest path from s ="
                             << s
                             << " to w =" << w << " via u =" << u
                             << " - Setting Sigma(s, w) = 1 ";
                    pss.sigma[wi] = 1;
                }

                if (computeCentralities)
                {

                    pss.nthOrderIncrement(dist_w);

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "For PC: nthOrder: number of nodes at distance "
                             << dist_w << "from s is "
                             << pss.nthOrder.value(dist_w, 0);

                    if (graph.vertexAtIndex(si)->eccentricity() < dist_w)
                    {
                        graph.vertexAtIndex(si)->setEccentricity(dist_w);
                        qDebug() << "    --- dijkstra: Compute Centralities: "
                                    "For EC: max distance ="
                                 << graph.vertexAtIndex(si)->eccentricity();
                    }

                    qDebug() << "    --- dijkstra: Compute Centralities: "
                                "Appending u="
                             << u << " to list Ps[w =" << w
                             << "] with the predecessors of w on all shortest paths from s ";
                    pss.Ps[wi].append(u);
                }
            }
            else
            {
                qDebug() << "    --- dijkstra: "
                            "NOT a new SP";
            }

            ++it1;

        } // END loop for every outEdge of u

        qDebug() << "    --- dijkstra: LOOP END over every edge (" << u << ", w ) e E... ";

    } // END loop while prQ not empty

    qDebug() << "### dijkstra: LOOP END. prQ is empty - Returning.";
} // END dijkstraSSSP()
