/**
 * @file graph_cliques.cpp
 * @brief Implements clique detection and cohesion-related algorithms for the Graph class.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "graph.h"
#include <QDebug>
#include <new>

/**
 * @brief Called from Graph::graphCliques to add a new clique (list of vertices)
 * Adds clique info to each clique member and updates co-membership matrix CLQM .
 * @param list
 * @return
 */
void Graph::graphCliqueAdd(const QList<int> &clique)
{

    m_cliques.insert(clique.size(), clique);

    qCDebug(lcCohesion) << "Graph::graphCliqueAdd() - Added clique:"
             << clique
             << "of size"
             << clique.size()
             << "Total cliques:"
             << m_cliques.size();
    int index1 = 0, index2 = 0, cliqueCount = 0;
    foreach (int actor1, clique)
    {
        index1 = vpos[actor1];
        qCDebug(lcCohesion) << "Graph::graphCliqueAdd() - Updating cliques in actor1:"
                 << actor1
                 << "vpos:"
                 << index1;
        m_graph[index1]->cliqueAdd(clique);
        foreach (int actor2, clique)
        {
            index2 = vpos[actor2];
            cliqueCount = CLQM.item(index1, index2);
            CLQM.setItem(index1, index2, (cliqueCount + 1));
            qCDebug(lcCohesion) << "Graph::graphCliqueAdd() - Updated co-membership matrix CLQM"
                     << "actor1:"
                     << actor1
                     << "actor2:"
                     << actor2
                     << "old matrix element: ("
                     << index1 << "," << index2 << ")=" << cliqueCount
                     << " -- updated to:"
                     << CLQM.item(index1, index2);
        }
    }
}

/**
 * Per-run state of the maximal-clique search. Lives on the stack of graphCliques() so a run
 * never leaves anything behind on Graph.
 */
struct Graph::CliqueSearchContext
{
    /// Mutual-tie neighbour set of every vertex, built once at the start of the run.
    QHash<int, QSet<int>> neighbours;
    /// Number of recursive calls so far, used to poll for cancellation at a fixed interval.
    quint64 calls = 0;
};

/// The cancel flag is polled once every (kCancelPollMask + 1) recursive calls. The flag is a
/// std::atomic<bool>, so this keeps the cost of the check negligible next to a call's own
/// QSet work while bounding the time a cancel request waits, whatever the shape of the search.
static constexpr quint64 kCancelPollMask = 1023;

/**
 * @brief Finds all maximal cliques in the graph using the Bron–Kerbosch algorithm
 *        with Tomita et al. (2006) pivot selection.
 *
 * Results are stored in m_cliques (by size), in each vertex's own clique list, and in the
 * clique co-membership matrix CLQM. If the run is canceled or fails, all three are cleared:
 * a partial census is never left behind.
 *
 * @return true if the census completed; false if it was canceled or ran out of memory.
 *
 * --- Algorithm overview ---
 *
 * The Bron–Kerbosch algorithm [1] is a recursive backtracking procedure that
 * maintains three disjoint vertex sets at each call:
 *
 *   R — the clique built so far (all vertices in R are mutually adjacent).
 *   P — candidate vertices that can still extend R (each is adjacent to all of R).
 *   X — vertices already processed that are also adjacent to all of R
 *       (used to avoid reporting the same clique more than once).
 *
 * When both P and X are empty, R cannot be extended and no super-set of R was
 * reported before — so R is a maximal clique.
 *
 * --- Pivot selection (Tomita et al., 2006) ---
 *
 * Without pivoting the algorithm iterates over every vertex in P at each level,
 * leading to a worst-case exponential blow-up even for graphs with few cliques.
 *
 * Tomita, Tanaka & Takahashi [2] proved that choosing a pivot vertex u ∈ P∪X
 * that maximises |N(u) ∩ P| (the size of u's neighbourhood intersected with P)
 * allows the main loop to enumerate only the vertices in P \ N(u) — the
 * non-neighbours of u inside the candidate set.
 *
 * Why this is correct: any maximal clique that extends R must contain at least
 * one vertex from P \ N(u), because if a clique contained only neighbours of u
 * it could be extended by u itself (since u is adjacent to all of them and to
 * all of R), contradicting maximality.  So we lose no cliques by restricting
 * the loop to P \ N(u).
 *
 * Why this is faster: the pivot u was chosen to maximise |N(u) ∩ P|, which
 * minimises |P \ N(u)|.  In the best case (a dense graph) |P \ N(u)| ≈ 1,
 * reducing each level of recursion to a single branch.  For sparse graphs the
 * improvement is smaller but still significant in practice.
 *
 * References:
 *   [1] Bron, C. & Kerbosch, J. (1973). "Algorithm 457: Finding all cliques of
 *       an undirected graph." Commun. ACM, 16(9), 575–577.
 *   [2] Tomita, E., Tanaka, A. & Takahashi, H. (2006). "The worst-case time
 *       complexity for generating all maximal cliques and computational
 *       experiments." Theoretical Computer Science, 363(1), 28–42.
 *       https://doi.org/10.1016/j.tcs.2006.06.015
 *
 *
 * See graphCliquesRecurse() for the recursion itself.
 */
bool Graph::graphCliques()
{
    qCDebug(lcCohesion) << "Graph::graphCliques() - STARTS HERE";

    const int V = vertices();

    CLQM.zeroMatrix(V, V);
    m_cliques.clear();

    CliqueSearchContext ctx;
    ctx.neighbours.reserve(V);

    // Pre-compute the neighbour sets of all vertices so that every recursive call can perform
    // O(1) set lookups instead of traversing edge lists.
    for (auto it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        const int vertex = (*it)->number();
        // reciprocalNeighborhoodList() returns neighbours connected by edges in BOTH directions
        // (i.e. mutual ties), which is the correct notion of adjacency for maximal clique
        // detection in undirected graphs.
        const QList<int> myNeighbors = (*it)->reciprocalNeighborhoodList();
        ctx.neighbours[vertex] = QSet<int>(myNeighbors.constBegin(), myNeighbors.constEnd());

        qCDebug(lcCohesion) << "Graph::graphCliques() - init. NeighborhoodList of v" << vertex
                            << ": " << ctx.neighbours[vertex];
        (*it)->clearCliques();
    }

    bool ok = false;
    try
    {
        // R and X start empty, P = V(G).
        ok = graphCliquesRecurse(ctx, QSet<int>(), verticesSet(), QSet<int>(), 1);
    }
    catch (const std::bad_alloc &)
    {
        qCDebug(lcCohesion) << "Graph::graphCliques() - out of memory";
        progressStatus(tr("Clique census failed: out of memory."));
    }

    if (!ok)
    {
        CLQM.zeroMatrix(V, V);
        m_cliques.clear();
        for (auto it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            (*it)->clearCliques();
        }
    }
    return ok;
}

/**
 * @brief One level of the Bron–Kerbosch recursion behind graphCliques().
 *
 * When both P and X are empty, R cannot be extended and no super-set of R was reported before,
 * so R is a maximal clique and is recorded. Otherwise a pivot u in P∪X maximising |N(u) ∩ P|
 * is chosen, and the loop branches only on P \ N(u).
 *
 * @param ctx    Per-run state (neighbour sets, call counter).
 * @param R      Current clique under construction (vertices already chosen).
 * @param P      Candidate vertices that can extend R.
 * @param X      Excluded vertices (already processed at this level).
 * @param depth  Recursion depth, 1 for the top-level call.
 * @return false if the run was canceled, true otherwise.
 */
bool Graph::graphCliquesRecurse(CliqueSearchContext &ctx,
                                QSet<int> R, QSet<int> P, QSet<int> X,
                                int depth)
{
    // Poll for cancellation at every depth, not only at the top level: a single top-level
    // branch can hold most of the search.
    if ((++ctx.calls & kCancelPollMask) == 0 && progressCanceled())
    {
        return false;
    }

    // -----------------------------------------------------------------------
    // Base case: P and X are both empty.
    // R is a maximal clique — record it and return.
    // -----------------------------------------------------------------------
    if (P.isEmpty() && X.isEmpty())
    {
        qCDebug(lcCohesion) << "Graph::graphCliquesRecurse() - P and X are both empty. MAXIMAL clique R=" << R;
        graphCliqueAdd(R.values());
        return true;
    }

    // -----------------------------------------------------------------------
    // Pivot selection (Tomita et al., 2006 — see graphCliques()).
    //
    // Scan every vertex u in P∪X and compute |N(u) ∩ P|.
    // Keep the u that maximises this count.  Ties are broken arbitrarily
    // (we just keep the first maximum found).
    // -----------------------------------------------------------------------
    int pivot = -1;
    int bestCoverage = -1;           // tracks max |N(u) ∩ P| seen so far

    // Combine P and X into a single candidate pool for pivot search.
    const QSet<int> PunionX = P | X;

    for (int u : PunionX)
    {
        // |N(u) ∩ P|: count how many candidate vertices u is adjacent to.
        const int coverage = (ctx.neighbours[u] & P).size();

        if (coverage > bestCoverage)
        {
            bestCoverage = coverage;
            pivot = u;
        }
    }

    // P \ N(pivot): the vertices we actually need to branch on.
    // Every maximal clique must contain at least one vertex from this set
    // (see graphCliques() for the correctness argument).
    const QSet<int> candidates = P - ctx.neighbours[pivot];

    qCDebug(lcCohesion) << "Graph::graphCliquesRecurse() - pivot:" << pivot
                        << " |N(pivot)∩P|:" << bestCoverage
                        << " |P\\N(pivot)|:" << candidates.size()
                        << " (saved" << (P.size() - candidates.size()) << "branches)";

    // -----------------------------------------------------------------------
    // Main loop: iterate over candidates = P \ N(pivot) only.
    // -----------------------------------------------------------------------
    QSet<int> Rnext, Pnext, Xnext;

    // We need a stable copy to iterate because P is mutated inside the loop
    // (v is moved from P to X after its recursive subtree is explored).
    const QList<int> candidateList = candidates.values();

    for (int v : candidateList)
    {
        const QSet<int> &NBS = ctx.neighbours[v];   // neighbours of v (pre-computed)

        // Build the arguments for the recursive call:
        //   R ∪ {v}   — extend the current clique with v
        //   P ∩ N(v)  — restrict candidates to neighbours of v (they can still extend the clique)
        //   X ∩ N(v)  — restrict excluded set to neighbours of v
        Rnext = R;
        Rnext << v;               // R ∪ {v}
        Pnext = P & NBS;          // P ∩ N(v)
        Xnext = X & NBS;          // X ∩ N(v)

        // Emit progress only at the top level to avoid flooding the event loop.
        if (depth == 1)
        {
            progressStatus(tr("Finding cliques: Recursive backtracking for actor ") + QString::number(v));
            if (progressCanceled())
            {
                return false;
            }
        }

        if (!graphCliquesRecurse(ctx, Rnext, Pnext, Xnext, depth + 1))
        {
            return false;
        }

        // After exploring all cliques that contain v, move v from P to X.
        // X records that v has been processed at this level; it blocks future
        // candidates from forming a clique with exactly the same members as R∪{v}.
        P.remove(v);
        X.insert(v);
    }

    return true;
}

/**
 * @brief Graph::graphCliquesOfSize
 * Returns the number of maximal cliques of a given size
 * @param size
 * @return
 */
int Graph::graphCliquesOfSize(const int &size)
{
    qCDebug(lcCohesion) << "Graph::graphCliquesOfSize()";

    return m_cliques.values(size).size();
}
