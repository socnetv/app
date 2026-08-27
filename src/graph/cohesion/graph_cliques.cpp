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
 * @brief Finds all maximal cliques in the graph using the Bron–Kerbosch algorithm
 *        with Tomita et al. (2006) pivot selection.
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
 * @param R  Current clique under construction (vertices already chosen).
 * @param P  Candidate vertices that can extend R.
 * @param X  Excluded vertices (already processed at this level).
 */
void Graph::graphCliques(QSet<int> R, QSet<int> P, QSet<int> X)
{
    csRecDepth++;

    qCDebug(lcCohesion) << "Graph::graphCliques() - STARTS HERE. csRecDepth:"
             << csRecDepth
             << " - Check if we are at initialization step";

    QList<int> myNeightbors;

    // -----------------------------------------------------------------------
    // Initialisation step (first call only): R, P and X are all empty.
    // Build the full candidate set P = V(G) and pre-compute the neighbour
    // sets for all vertices into neighboursHash so that every recursive call
    // can perform O(1) set lookups instead of traversing edge lists.
    // -----------------------------------------------------------------------
    if (R.isEmpty() && P.isEmpty() && X.isEmpty())
    {
        int V = vertices();
        P.reserve(V);
        R.reserve(V);
        X.reserve(V);
        P = verticesSet();  // P starts as the full vertex set

        qCDebug(lcCohesion) << "Graph::graphCliques() - initialization step. R, X empty and P=V(G): " << P;

        CLQM.zeroMatrix(V, V);  // co-membership matrix reset
        m_cliques.clear();

        VList::const_iterator it;
        int vertex = 0;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            vertex = (*it)->number();
            // reciprocalNeighborhoodList() returns neighbours connected by edges
            // in BOTH directions (i.e. mutual ties), which is the correct notion
            // of adjacency for maximal clique detection in undirected graphs.
            myNeightbors = (*it)->reciprocalNeighborhoodList();
            neighboursHash[vertex] = QSet<int>(myNeightbors.constBegin(), myNeightbors.constEnd());

            qCDebug(lcCohesion) << "Graph::graphCliques() - init. NeighborhoodList of v" << vertex
                     << ": " << neighboursHash[vertex];
            (*it)->clearCliques();
        }
    }

    qCDebug(lcCohesion) << "Graph::graphCliques() - check if P and X are both empty...";

    // -----------------------------------------------------------------------
    // Base case: P and X are both empty.
    // R is a maximal clique — record it and return.
    // -----------------------------------------------------------------------
    if (P.isEmpty() && X.isEmpty())
    {
        qCDebug(lcCohesion) << "Graph::graphCliques() - P and X are both empty. MAXIMAL clique R=" << R;
        QList<int> clique = R.values();
        graphCliqueAdd(clique);
        csRecDepth--;
        return;
    }

    // -----------------------------------------------------------------------
    // Pivot selection (Tomita et al., 2006 — see header comment).
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
        const int coverage = (neighboursHash[u] & P).size();

        if (coverage > bestCoverage)
        {
            bestCoverage = coverage;
            pivot = u;
        }
    }

    // P \ N(pivot): the vertices we actually need to branch on.
    // Every maximal clique must contain at least one vertex from this set
    // (see header comment for the correctness argument).
    const QSet<int> candidates = P - neighboursHash[pivot];

    qCDebug(lcCohesion) << "Graph::graphCliques() - pivot:" << pivot
             << " |N(pivot)∩P|:" << bestCoverage
             << " |P\\N(pivot)|:" << candidates.size()
             << " (saved" << (P.size() - candidates.size()) << "branches)";

    // -----------------------------------------------------------------------
    // Main loop: iterate over candidates = P \ N(pivot) only.
    // -----------------------------------------------------------------------
    QSet<int> NBS;
    QSet<int> Rnext, Pnext, Xnext;

    // We need a stable copy to iterate because P is mutated inside the loop
    // (v is moved from P to X after its recursive subtree is explored).
    const QList<int> candidateList = candidates.values();

    qCDebug(lcCohesion) << "Graph::graphCliques() - Start looping over candidates P\\N(pivot)";

    for (int v : candidateList)
    {
        qCDebug(lcCohesion) << "Graph::graphCliques() - CURRENT v:" << v
                 << " P:" << P << " P.count=" << P.size()
                 << " R:" << R << " X:" << X;

        NBS = neighboursHash[v];   // neighbours of v (pre-computed at init)

        // Skip self-loops: a vertex with only a tie to itself cannot join any clique.
        if (NBS.size() == 1 && NBS.contains(v))
        {
            qCDebug(lcCohesion) << "Graph::graphCliques() - v:" << v << "has only a self-tie, skip";
            // Move v from P to X so it is not re-visited.
            P.remove(v);
            X.insert(v);
            continue;
        }

        // Build the arguments for the recursive call:
        //   R ∪ {v}   — extend the current clique with v
        //   P ∩ N(v)  — restrict candidates to neighbours of v (they can still extend the clique)
        //   X ∩ N(v)  — restrict excluded set to neighbours of v
        Rnext = R;
        Rnext << v;               // R ∪ {v}
        Pnext = P & NBS;          // P ∩ N(v)
        Xnext = X & NBS;          // X ∩ N(v)

        qCDebug(lcCohesion) << "Graph::graphCliques() - v:" << v
                 << "RECURSIVE CALL: R⋃{v}=" << Rnext
                 << " P⋂N(v)=" << Pnext
                 << " X⋂N(v)=" << Xnext;

        // Emit progress only at recursion depth 1 (top-level branches) to
        // avoid flooding the event loop on deep recursions.
        if (csRecDepth == 1)
        {
            progressStatus(tr("Finding cliques: Recursive backtracking for actor ") + QString::number(v));
            if (progressCanceled())
            {
                csRecDepth--;
                return;
            }
        }

        try
        {
            graphCliques(Rnext, Pnext, Xnext);
        }
        catch (...)
        {
            qCDebug(lcCohesion) << "Graph::graphCliques() - ERROR in recursive call";
            return;
        }

        // After exploring all cliques that contain v, move v from P to X.
        // X records that v has been processed at this level; it blocks future
        // candidates from forming a clique with exactly the same members as R∪{v}.
        P.remove(v);
        X.insert(v);

        qCDebug(lcCohesion) << "Graph::graphCliques() - v:" << v
                 << " returned from recursion. Moved to X."
                 << " P=" << P << " X=" << X;

    } // end for candidateList

    qCDebug(lcCohesion) << "Graph::graphCliques() - FINISHED candidate loop at csRecDepth:" << csRecDepth;

    csRecDepth--;
}

/**
    Returns the number of maximal cliques which include a given actor
*/
int Graph::graphCliquesContaining(const int &actor, const int &size)
{
    qCDebug(lcCohesion) << "*** Graph::graphCliquesContaining(" << actor << ")";
    int cliqueCounter = 0;
    foreach (QList<int> clique, m_cliques)
    {
        if (size != 0)
        {
            if (clique.size() != size)
                continue;
        }
        if (clique.contains(actor))
        {
            cliqueCounter++;
        }
    }
    return cliqueCounter;
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
