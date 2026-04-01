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

    qDebug() << "Graph::graphCliqueAdd() - Added clique:"
             << clique
             << "of size"
             << clique.size()
             << "Total cliques:"
             << m_cliques.size();
    int index1 = 0, index2 = 0, cliqueCount = 0;
    foreach (int actor1, clique)
    {
        index1 = vpos[actor1];
        qDebug() << "Graph::graphCliqueAdd() - Updating cliques in actor1:"
                 << actor1
                 << "vpos:"
                 << index1;
        m_graph[index1]->cliqueAdd(clique);
        foreach (int actor2, clique)
        {
            index2 = vpos[actor2];
            cliqueCount = CLQM.item(index1, index2);
            CLQM.setItem(index1, index2, (cliqueCount + 1));
            qDebug() << "Graph::graphCliqueAdd() - Updated co-membership matrix CLQM"
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
 * @brief Finds all maximal cliques in an undirected (?) graph.
 * Implements the Bron–Kerbosch algorithm, a recursive backtracking algorithm
 * that searches for all maximal cliques in a given graph G.
 * Given three sets R, P, and X, the algorithm finds the maximal cliques that
 * include all of the vertices in R, some of the vertices in P, and none of
 * the vertices in X.
 * In each call to the algorithm, P and X are disjoint sets whose union consists
 * of those vertices that form cliques when added to R.
 * In other words, P ∪ X is the set of vertices which are joined to every element of R.
 * When P and X are both empty there are no further elements that can be added to R,
 * so R is a maximal clique and the algorithm outputs R.
 * The recursion is initiated by setting R and X to be the empty set and P to be
 * the vertex set of the graph.
 * Within each recursive call, the algorithm considers the vertices in P in turn.
 * if there are no vertices, it either reports R as a maximal clique (if X is empty),
 * or backtracks.
 * For each vertex v chosen from P, it makes a recursive call in which v is added to R
 * and in which P and X are restricted to the neighbor set NBS(v) of v,
 * which finds and reports all clique extensions of R that contain v.
 * Then, it moves v from P to X to exclude it from consideration in future cliques
 * and continues with the next vertex in P.
 * @param R
 * @param P
 * @param X
 */
void Graph::graphCliques(QSet<int> R, QSet<int> P, QSet<int> X)
{

    csRecDepth++;

    qDebug() << "Graph::graphCliques() - STARTS HERE. csRecDepth:"
             << csRecDepth
             << " - Check if we are at initialization step";

    QList<int> myNeightbors;

    if (R.isEmpty() && P.isEmpty() && X.isEmpty())
    {

        int V = vertices();
        P.reserve(V);
        R.reserve(V);
        X.reserve(V);
        P = verticesSet();

        qDebug() << "Graph::graphCliques() - initialization step. R, X empty and P=V(G): "
                 << P;

        CLQM.zeroMatrix(V, V); // co-membership matrix CLQM

        m_cliques.clear();

        VList::const_iterator it;
        int vertex = 0;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            vertex = (*it)->number();

            myNeightbors = (*it)->reciprocalNeighborhoodList();
            neighboursHash[vertex] = QSet<int>(myNeightbors.constBegin(), myNeightbors.constEnd());

            qDebug() << "Graph::graphCliques() - initialization step. NeighborhoodList of v"
                     << vertex
                     << ": "
                     << neighboursHash[vertex];
            (*it)->clearCliques();
        }
    }

    qDebug() << "Graph::graphCliques() - check if P and X are both empty (which would mean we have a clique in R)...";

    if (P.isEmpty() && X.isEmpty())
    {

        qDebug() << "Graph::graphCliques() - P and X are both empty. MAXIMAL clique R=" << R;

        QList<int> clique = R.values();

        graphCliqueAdd(clique);

        csRecDepth--;

        return;
    }

    int v;

    QSet<int> NBS;

    QSet<int> Rnext, Pnext, Xnext;

    QSet<int>::iterator i = P.begin();

    int counter = 0;

    // Loop over vertices in P, randomly

    qDebug() << "Graph::graphCliques() - Start looping over vertices in P (randomly)";

    while (i != P.end())
    {

        counter++;

        v = *i;

        qDebug() << "Graph::graphCliques() - CURRENT v:" << v
                 << " P:" << P << " P.count=" << P.size()
                 << " R:" << R
                 << " X:" << X;

        NBS = neighboursHash[v];

        if (NBS.size() == 1 && NBS.contains(v))
        {

            qDebug() << "Graph::graphCliques() - v:" << v
                     << "has only a tie to itself";

            // graphCliques( R, P, X );

            ++i;

            continue;
        }

        Rnext = R;
        Rnext << v;
        Pnext = P & NBS;
        Xnext = X & NBS;

        qDebug() << "Graph::graphCliques() - v:" << v
                 << "RECURSIVE CALL to graphCliques ( R ⋃ {v}, P ⋂ NB(v), X ⋂ NBS(v) )"
                 << "\n"
                 << "NBS(v):" << NBS
                 << "\n"
                 << "Rnext = R ⋃ {v}:" << Rnext
                 << "\n"
                 << "Pnext = P ⋂ NBS(v):" << Pnext
                 << "\n"
                 << "Xnext = X ⋂ NBS(v):" << Xnext;

        if (csRecDepth == 1)
        {
            progressUpdate(counter);
            progressStatus(tr("Finding cliques: Recursive backtracking for actor ") + QString::number(v));
            if (progressCanceled())
            {
                csRecDepth--;
                return;
            }
        }

        // find all clique extensions of R that contain v
        try
        {
            graphCliques(Rnext, Pnext, Xnext);
        }
        catch (...)
        {
            qDebug() << "Graph::graphCliques() - ERROR";
            return;
        }

        // Set P = P \ v
        i = P.erase(i); // P-=v;

        // Set X = X + v
        X.insert(v);

        qDebug() << "Graph::graphCliques() - v:" << v
                 << "RETURNED from recursive call - recDepth: "
                 << csRecDepth
                 << " Moved v:" << v
                 << " from P to X to be excluded in the future"
                 << " P=" << P << " P.count:" << P.size()
                 << " R=" << R << " R.count:" << R.size()
                 << " X=" << X << " X.count:" << X.size()
                 << " Continuing with next v in P";
        //++i;

    } // end while loop

    qDebug() << "Graph::graphCliques() - FINISHED loop over P:" << P
             << "at csRecDepth:" << csRecDepth;

    csRecDepth--;
}

/**
    Returns the number of maximal cliques which include a given actor
*/
int Graph::graphCliquesContaining(const int &actor, const int &size)
{
    qDebug() << "*** Graph::graphCliquesContaining(" << actor << ")";
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
    qDebug() << "Graph::graphCliquesOfSize()";

    return m_cliques.values(size).size();
}
