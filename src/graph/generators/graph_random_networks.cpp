/**
 * @file graph_random_networks.cpp
 * @brief Implements random network generation algorithms of the Graph class (e.g., Erdős–Rényi, Watts–Strogatz and related models).
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
#include <cmath>

/**
 * @brief Adds a little universal randomness :)
 */
void Graph::randomizeThings()
{
    time_t now;                 /* define 'now'. time_t is probably a typedef	*/
    now = time((time_t *)NULL); /* Get the system time and put it
                                 * into 'now' as 'calender time' the number of seconds since  1/1/1970   	*/

    srand((unsigned int)now);
}

/**
 * @brief Creates an Erdős–Rényi random network.
 *
 * Supports two models:
 * - G(n,p): each possible edge is included independently with probability p.
 * - G(n,M): exactly M edges are placed at random among all possible pairs.
 *
 * @param N     Number of nodes.
 * @param model "G(n,p)" or "G(n,M)".
 * @param m     Number of edges (used in G(n,M) model; 0 for G(n,p)).
 * @param p     Edge probability (used in G(n,p) model; 0 for G(n,M)).
 * @param mode  "graph" for undirected, anything else for directed.
 * @param diag  If true, self-loops are allowed.
 * @return true on success, false if the user cancelled.
 */
bool Graph::randomNetErdosCreate(const int &N,
                                 const QString &model,
                                 const int &m,
                                 const qreal &p,
                                 const QString &mode,
                                 const bool &diag)
{
    qDebug() << "Creating Erdos-Renyi random network:"
             << "N" << N
             << "model" << model
             << "edges" << m
             << "edge probability" << p
             << "mode" << mode
             << "diag" << diag;

    if (mode == "graph")
    {
        setDirected(false);
    }

    vpos.reserve(N);
    randomizeThings();

    int progressCounter = 0;
    int edgeCount = 0;

    QString pMsg = tr("Creating Erdos-Renyi Random Network. \n"
                      "Please wait...");

    // Progress max: M edges for G(n,M), N nodes for G(n,p)
    progressCreate((m != 0 ? m : N), pMsg);

    // Create all nodes first
    for (int i = 0; i < N; i++)
    {
        int x = canvasRandomX();
        int y = canvasRandomY();
        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    qDebug() << "Nodes created. Creating edges using model" << model;

    if (model == "G(n,p)")
    {
        // Bernoulli trials: each pair (i,j) gets an edge with probability p
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                if (!diag && i == j)
                    continue;

                if ((rand() % 100 + 1) / 100.0 < p)
                {
                    edgeCount++;
                    if (mode == "graph")
                    {
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                    }
                    else
                    {
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Directed, true, false,
                                   QString(), false);
                    }
                }
            }

            progressUpdate(++progressCounter);
            if (progressCanceled())
            {
                progressFinish();
                return false;
            }
        }
    }
    else
    {
        // G(n,M): place exactly m edges at random positions
        int source = 0, target = 0;
        bool cancelled = false;
        do
        {
            source = rand() % N + 1;
            target = rand() % N + 1;

            if (!diag && source == target)
                continue;
            if (edgeExists(source, target))
                continue;

            edgeCount++;
            if (mode == "graph")
            {
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Undirected, false, false,
                           QString(), false);
            }
            else
            {
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Directed, true, false,
                           QString(), false);
            }

            progressUpdate(++progressCounter);
            if (progressCanceled())
            {
                cancelled = true;
                break;
            }

        } while (edgeCount != m);

        if (cancelled)
        {
            progressFinish();
            return false;
        }
    }

    relationCurrentRename(tr("erdos-renyi"), true);

    progressUpdate((m != 0 ? m : N));
    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);

    return true;
}

/**
 * @brief Creates a Barabási–Albert scale-free random network.
 *
 * The algorithm works in two phases:
 * 1. Seed: build a fully connected clique of m0 initial nodes.
 * 2. Growth: add nodes one by one up to N, each connecting to m existing
 *    nodes via preferential attachment — nodes with higher degree are more
 *    likely to receive new edges (rich-get-richer effect).
 *
 * The attachment probability for node j is:
 *   P(j) = (alpha + degree(j)^power) / sumDegrees
 *
 * @param N      Total number of nodes in the final network.
 * @param power  Exponent of the preferential attachment (typically 1).
 * @param m0     Number of nodes in the initial seed clique.
 * @param m      Number of edges each new node attaches to existing nodes.
 * @param alpha  Additive constant in the attachment probability (zero-appeal).
 * @param mode   "graph" for undirected, anything else for directed.
 * @return true on success, false if the user cancelled.
 */
bool Graph::randomNetScaleFreeCreate(const int &N,
                                     const int &power,
                                     const int &m0,
                                     const int &m,
                                     const qreal &alpha,
                                     const QString &mode)
{
    qDebug() << "Graph::randomNetScaleFreeCreate() -"
             << "N" << N
             << "power" << power
             << "m0" << m0
             << "m" << m
             << "alpha" << alpha
             << "mode" << mode;

    randomizeThings();

    if (mode == "graph")
    {
        setDirected(false);
    }

    int x = 0;
    int y = 0;
    int newEdges = 0;
    double sumDegrees = 0;
    double k_j;
    double x0 = canvasWidth / 2.0;
    double y0 = canvasHeight / 2.0;
    double radius = canvasMaxRadius();
    double rad = (2.0 * M_PI / N);
    double prob_j = 0, prob = 0;
    int progressCounter = 0;

    vpos.reserve(N);

    QString pMsg = tr("Creating Scale-Free Random Network. \n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    // Phase 1: create the initial seed clique of m0 nodes
    qDebug() << "Graph::randomNetScaleFreeCreate() - creating seed clique of" << m0 << "nodes";

    for (int i = 0; i < m0; ++i)
    {
        x = x0 + radius * cos(i * rad);
        y = y0 + radius * sin(i * rad);
        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    for (int i = 0; i < m0; ++i)
    {
        for (int j = i + 1; j < m0; ++j)
        {
            // Respect mode when creating seed clique edges,
            // consistent with the growth phase below
            if (mode == "graph")
            {
                edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                           EdgeType::Undirected, false, false,
                           QString(), false);
            }
            else
            {
                edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                           EdgeType::Directed, true, false,
                           QString(), false);
            }
        }
        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            return false;
        }
    }

    // Phase 2: grow the network to N nodes via preferential attachment
    qDebug() << "Graph::randomNetScaleFreeCreate() - growing network to" << N << "nodes";

    for (int i = m0; i < N; ++i)
    {
        x = x0 + radius * cos(i * rad);
        y = y0 + radius * sin(i * rad);

        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            return false;
        }

        // Sum of degrees used as denominator in attachment probability
        // Multiply by 2 for undirected graphs since edgesEnabled() counts each edge once
        sumDegrees = 2 * edgesEnabled();

        newEdges = 0;

        // Repeat until this new node has attached to exactly m existing nodes
        for (;;)
        {
            for (int j = 0; j < i; ++j)
            {
                if (newEdges == m)
                    break;

                k_j = vertexDegreeIn(j + 1);
                k_j = pow(k_j, power);

                // If no edges exist yet, connect with certainty;
                // otherwise use preferential attachment probability
                if (sumDegrees < 1)
                    prob_j = 1;
                else
                    prob_j = (alpha + k_j) / sumDegrees;

                prob = (rand() % 100 + 1) / 100.0;

                if (prob <= prob_j)
                {
                    if (mode == "graph")
                    {
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                    }
                    else
                    {
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Directed, true, false,
                                   QString(), false);
                    }
                    newEdges++;
                }
            }
            if (newEdges == m)
                break;
        }

        qDebug() << "Graph::randomNetScaleFreeCreate() -"
                 << m << "edges attached for node" << i + 1;
    }

    relationCurrentRename(tr("scale-free"), true);

    qDebug() << "Graph::randomNetScaleFreeCreate() - finished.";

    setModStatus(ModStatus::VertexEdgeCount);

    progressFinish();

    return true;
}

/**
 * @brief Creates a Watts–Strogatz small-world random network.
 *
 * The algorithm works in two phases:
 * 1. Build a ring lattice of N nodes each connected to `degree` neighbours.
 * 2. Rewire each edge with probability `beta` to a randomly chosen node,
 *    producing the characteristic short path lengths and high clustering.
 *
 * @param N      Number of nodes.
 * @param degree Number of neighbours each node is initially connected to (must be even).
 * @param beta   Rewiring probability in [0,1]. 0 = pure lattice, 1 = random graph.
 * @param mode   "graph" for undirected, anything else for directed.
 * @return true on success, false if the user cancelled.
 */
bool Graph::randomNetSmallWorldCreate(const int &N, const int &degree,
                                      const double &beta, const QString &mode)
{
    qDebug() << "Creating small-world random network. Vertices:" << N
             << "degree:" << degree
             << "beta:" << beta
             << "mode:" << mode;

    if (mode == "graph")
    {
        setDirected(false);
    }

    // Phase 1: build the underlying ring lattice.
    // We pass updateProgress=false so the lattice builder does not create its
    // own progress dialog — we own the single dialog for the whole operation.
    if (!randomNetRingLatticeCreate(N, degree, false))
    {
        return false;
    }

    // Phase 2: rewire edges with probability beta (Watts-Strogatz).
    QString pMsg = tr("Creating Small-World Random Network. \n"
                      "Please wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    qDebug() << "Rewiring starts...";

    int candidate = 0;
    int progressCounter = 1;

    for (int i = 1; i < N; i++)
    {
        for (int j = i + 1; j < N; j++)
        {
            if (edgeExists(i, j))
            {
                // Rewire this edge with probability beta
                if (rand() % 100 < (beta * 100))
                {
                    edgeRemove(i, j, true);

                    // Find a valid rewiring target: not a self-loop,
                    // not already a neighbour of i
                    for (;;)
                    {
                        candidate = rand() % (N + 1);
                        if (candidate == 0 || candidate == i)
                            continue;
                        if (edgeExists(i, candidate) != 0)
                            continue;
                        edgeCreate(i, candidate, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                        break;
                    }
                }
            }
        }

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            return false;
        }
    }

    relationCurrentRename(tr("small-world"), true);

    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);

    return true;
}

/**
 * @brief Creates a pseudo-random d-regular network.
 *
 * Every node ends up with exactly `degree` neighbours. The algorithm:
 * 1. Build an initial ordered edge list where each node i connects to the
 *    next degree/2 nodes (undirected) or degree nodes (directed).
 * 2. Repeatedly pick two edges at random and swap their endpoints,
 *    ensuring no self-loops or duplicate edges result, until all edges
 *    have been processed. This produces a random regular graph.
 * 3. Draw the resulting edges.
 *
 * @param N      Number of nodes.
 * @param degree Desired degree of every node (must be even for undirected).
 * @param mode   "graph" for undirected, anything else for directed.
 * @param diag   Reserved, currently unused.
 * @return true on success, false if the user cancelled.
 */
bool Graph::randomNetRegularCreate(const int &N,
                                   const int &degree,
                                   const QString &mode, const bool &diag)
{
    qDebug() << "Creating d-regular random network..."
             << "N" << N << "degree" << degree << "mode" << mode;
    Q_UNUSED(diag);

    if (mode == "graph")
    {
        setDirected(false);
    }

    int x = 0, y = 0;
    qreal progressCounter = 0;
    qreal progressFraction = (isUndirected()) ? 2 / (qreal)degree : 1 / (qreal)degree;

    int target = 0;

    QList<QString> m_edges;
    QStringList firstEdgeVertices, secondEdgeVertices, m_edge;
    QString firstEdge, secondEdge;

    randomizeThings();
    vpos.reserve(N);

    QString pMsg = tr("Creating pseudo-random d-regular network. \n"
                      "Please wait...");
    progressStatus(pMsg);

    // Create all nodes
    for (int i = 0; i < N; i++)
    {
        x = canvasRandomX();
        y = canvasRandomY();
        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    // Build initial ordered edge list
    if (mode == "graph")
    {
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < degree / 2; j++)
            {
                target = i + j + 1;
                if (target > (N - 1))
                    target = target - N;
                m_edges.append(QString::number(i + 1) + "->" + QString::number(target + 1));
            }
        }
    }
    else
    {
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < degree; j++)
            {
                target = i + j + 1;
                if (target > (N - 1))
                    target = target - N;
                m_edges.append(QString::number(i + 1) + "->" + QString::number(target + 1));
            }
        }
    }

    qDebug() << "Edge list count:" << m_edges.size()
             << "Randomising by swapping edge endpoint pairs...";

    // Randomise by repeatedly swapping endpoints of two randomly chosen edges,
    // ensuring the result has no self-loops or duplicate edges
    for (int i = 1; i < m_edges.size(); ++i)
    {
        firstEdgeVertices.clear();
        secondEdgeVertices.clear();
        firstEdgeVertices << "" << "";
        secondEdgeVertices << "" << "";

        while (firstEdgeVertices[0] == firstEdgeVertices[1] ||
               firstEdgeVertices[0] == secondEdgeVertices[0] ||
               firstEdgeVertices[0] == secondEdgeVertices[1] ||
               firstEdgeVertices[1] == secondEdgeVertices[0] ||
               firstEdgeVertices[1] == secondEdgeVertices[1] ||
               secondEdgeVertices[0] == secondEdgeVertices[1] ||
               m_edges.contains(firstEdgeVertices[0] + "->" + secondEdgeVertices[1]) ||
               m_edges.contains(secondEdgeVertices[0] + "->" + firstEdgeVertices[1]) ||
               (isUndirected() && m_edges.contains(secondEdgeVertices[1] + "->" + firstEdgeVertices[0])) ||
               (isUndirected() && m_edges.contains(firstEdgeVertices[1] + "->" + secondEdgeVertices[0])))
        {
            firstEdge = m_edges.at(rand() % m_edges.size());
            firstEdgeVertices = firstEdge.split("->");
            secondEdge = m_edges.at(rand() % m_edges.size());
            secondEdgeVertices = secondEdge.split("->");
        }

        m_edges.removeAll(firstEdge);
        m_edges.removeAll(secondEdge);
        m_edges.append(firstEdgeVertices[0] + "->" + secondEdgeVertices[1]);
        m_edges.append(secondEdgeVertices[0] + "->" + firstEdgeVertices[1]);
    }

    // Now set progress max to actual edge count for accurate progress reporting
    progressCreate(m_edges.size(), pMsg);

    // Draw the randomised edges
    for (int i = 0; i < m_edges.size(); ++i)
    {
        m_edge = m_edges.at(i).split("->");
        edgeCreate(m_edge[0].toInt(0), m_edge[1].toInt(0), 1,
                   initEdgeColor,
                   (isUndirected()) ? EdgeType::Undirected : EdgeType::Directed,
                   (isUndirected()) ? false : true,
                   false,
                   QString(), false);

        progressCounter += progressFraction;
        if (fmod(progressCounter, 1.0) == 0)
        {
            progressUpdate((int)progressCounter);
            if (progressCanceled())
            {
                progressFinish();
                return false;
            }
        }
    }

    relationCurrentRename(tr("d-regular"), true);

    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);

    return true;
}

/**
 * @brief Creates a random ring lattice network.
 * @param vert
 * @param degree
 * @param x0
 * @param y0
 * @param radius
 * @param updateProgress
 */
bool Graph::randomNetRingLatticeCreate(const int &N, const int &degree,
                                       const bool updateProgress)
{
    qDebug() << "Creating ring lattice random network...";
    int x = 0;
    int y = 0;
    int progressCounter = 0;

    double x0 = canvasWidth / 2.0;
    double y0 = canvasHeight / 2.0;
    double radius = canvasMaxRadius();
    double rad = (2.0 * M_PI / N);

    setDirected(false);

    randomizeThings();

    vpos.reserve(N);

    QString pMsg = tr("Creating ring-lattice network. \n"
                      "Please wait...");
    progressStatus(pMsg);

    if (updateProgress)
        progressCreate(N, pMsg);

    for (int i = 0; i < N; i++)
    {
        x = x0 + radius * cos(i * rad);
        y = y0 + radius * sin(i * rad);
        vertexCreate(i + 1, initVertexSize, initVertexColor,
                     initVertexNumberColor, initVertexNumberSize,
                     QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
                     QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    int target = 0;
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < degree / 2; j++)
        {
            target = i + j + 1;
            if (target > (N - 1))
                target = target - N;
            edgeCreate(i + 1, target + 1, 1, initEdgeColor,
                       EdgeType::Undirected, false, false,
                       QString(), false);
        }
        if (updateProgress)
        {
            progressUpdate(++progressCounter);
            if (progressCanceled())
            {
                progressFinish();
                return false;
            }
        }
    }

    // Always rename the relation, regardless of updateProgress (fix: was inside updateProgress guard)
    relationCurrentRename(tr("ring-lattice"), true);

    if (updateProgress)
        progressFinish();

    setModStatus(ModStatus::VertexEdgeCount, updateProgress);

    return true;
}


/**
 * @brief Creates a lattice (mesh/grid) random network.
 *
 * Nodes are arranged in a length×length grid. Each node connects to its
 * neighbours within a given neighbourhood distance in both horizontal and
 * vertical directions. The resulting network forms a regular tiling pattern.
 *
 * Note: the `dimension` and `circular` parameters are reserved for future
 * extension to higher-dimensional and toroidal lattices.
 *
 * @param N                 Total number of nodes (should equal length^2).
 * @param length            Number of nodes along each dimension of the grid.
 * @param dimension         Reserved for future use (higher-dimensional lattices).
 * @param neighborhoodLength  Distance within which nodes are connected.
 * @param mode              "graph" for undirected, anything else for directed.
 * @param circular          Reserved for future use (toroidal/circular lattice).
 * @return true on success, false if the user cancelled.
 */
bool Graph::randomNetLatticeCreate(const int &N,
                                   const int &length,
                                   const int &dimension,
                                   const int &neighborhoodLength,
                                   const QString &mode,
                                   const bool &circular)
{
    qDebug() << "Creating lattice network..."
             << "N" << N
             << "length" << length
             << "neighborhoodLength" << neighborhoodLength
             << "mode" << mode;

    Q_UNUSED(circular);
    Q_UNUSED(dimension);

    if (mode == "graph")
    {
        setDirected(false);
    }

    int x = 0;
    int y = 0;
    int nCount = 0;
    double nodeHPadding = 0;
    double nodeVPadding = 0;
    double canvasPadding = 100;
    qreal progressCounter = 0;
    qreal progressFraction = 0;

    int target = 0;

    QList<QString> latticeEdges;
    QStringList m_edge;
    QString edge;
    QString oppEdge;

    randomizeThings();
    vpos.reserve(N);

    QString pMsg = tr("Creating lattice network. \n"
                      "Please wait...");
    progressStatus(pMsg);

    // Create nodes arranged in a length x length grid
    nCount = 0;
    canvasPadding = 20;
    nodeHPadding = (canvasWidth) / (double)(length + 2);
    nodeVPadding = (canvasHeight) / (double)(length + 2);

    qDebug() << "Creating" << N << "vertices in a" << length << "x" << length
             << "grid - nodeHPadding" << nodeHPadding
             << "nodeVPadding" << nodeVPadding;

    for (int i = 0; i < length; i++)
    {
        y = canvasPadding + nodeVPadding * (i + 1);
        for (int j = 0; j < length; j++)
        {
            nCount++;
            x = canvasPadding + nodeHPadding * (j + 1);
            vertexCreate(
                nCount, initVertexSize,
                initVertexColor,
                initVertexNumberColor,
                initVertexNumberSize,
                QString::number(nCount),
                initVertexLabelColor,
                initVertexLabelSize,
                QPoint(x, y),
                initVertexShape,
                initVertexIconPath,
                false);
        }
    }

    // Compute the edge list: for each node, find all neighbours
    // within neighborhoodLength steps in both axes
    qDebug() << "Computing edges...";

    if (mode == "graph")
    {
        for (int i = 1; i <= N; i++)
        {
            for (int j = 1; j < neighborhoodLength + 1; j++)
            {
                for (int p = 0; p < 2; p++)
                {
                    for (int q = 0; q < 2; q++)
                    {
                        target = i + pow((-1), p) * j * pow(length, q);

                        // Skip out-of-bounds: wrap-around on right edge
                        if (i % length == 0 && target == i + 1)
                            continue;
                        // Skip out-of-bounds: wrap-around on left edge
                        if (i % length == 1 && target == i - 1)
                            continue;
                        // Skip out-of-bounds: below grid
                        if (target > N)
                        {
                            target = target % N;
                            continue;
                        }
                        // Skip out-of-bounds: above grid
                        if (target < 1)
                        {
                            target = N - target;
                            continue;
                        }

                        // Add edge only once (undirected: skip if reverse already recorded)
                        edge = QString::number(i) + "<->" + QString::number(target);
                        oppEdge = QString::number(target) + "<->" + QString::number(i);
                        if (!latticeEdges.contains(edge) && !latticeEdges.contains(oppEdge))
                        {
                            latticeEdges.append(edge);
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Directed lattice: reserved for future implementation
    }

    // Draw edges; progress tracks actual edge count for accuracy
    qDebug() << "Drawing" << latticeEdges.size() << "edges...";

    progressFraction = (latticeEdges.size() > 0) ? 1.0 / (qreal)latticeEdges.size() : 1.0;

    progressCreate(latticeEdges.size(), pMsg);

    for (int i = 0; i < latticeEdges.size(); ++i)
    {
        m_edge = latticeEdges.at(i).split("<->");
        edgeCreate(m_edge[0].toInt(0), m_edge[1].toInt(0), 1,
                   initEdgeColor,
                   (isUndirected()) ? EdgeType::Undirected : EdgeType::Directed,
                   (isUndirected()) ? false : true,
                   false,
                   QString(), false);

        progressCounter += progressFraction;
        if (fmod(progressCounter, 1.0) == 0)
        {
            progressUpdate((int)progressCounter);
            if (progressCanceled())
            {
                progressFinish();
                return false;
            }
        }
    }

    relationCurrentRename(tr("lattice"), true);

    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);

    return true;
}


/**
    Calculates and returns x! factorial...
    used in (n 2)p edges calculation
*/
int Graph::factorial(int x)
{
    int tmp;
    if (x <= 1)
        return 1;
    tmp = x * factorial(x - 1);
    return tmp;
}
