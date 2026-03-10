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
 * @brief Creates an erdos-renyi random network according to the given model
 * @param vert
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 */
void Graph::randomNetErdosCreate(const int &N,
                                 const QString &model,
                                 const int &m,
                                 const qreal &p,
                                 const QString &mode,
                                 const bool &diag)
{
    qDebug() << "Creating an erdos-renyi random network, vertices " << N
             << " model " << model
             << " edges " << m
             << " edge probability " << p
             << " graph mode " << mode
             << " diag " << diag;

    if (mode == "graph")
    {
        setDirected(false);
    }
    vpos.reserve(N);

    randomizeThings();

    int progressCounter = 0;
    int edgeCount = 0;

    qDebug() << "Creating nodes...";

    QString pMsg = tr("Creating Erdos-Renyi Random Network. \n"
                      " Please wait...");

    progressCreate((m != 0 ? m : N), pMsg);

    for (int i = 0; i < N; i++)
    {
        int x = canvasRandomX();
        int y = canvasRandomY();

        //        qDebug() << "creating new node" << i+1 << "at" << x << y;

        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    qDebug() << "Creating edges...";

    if (model == "G(n,p)")
    {
        qDebug() << "G(n,p) model...";
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {

                //                qDebug() << "Bernoulli trial "
                //                       << "for edge " <<  i+1 << "->" << j+1;

                if (!diag && i == j)
                {
                    //                    qDebug()<< " skip because " << i+1 << " = " << j+1 << " and diag " << diag;
                    continue;
                }
                if ((rand() % 100 + 1) / 100.0 < p)
                {
                    edgeCount++;

                    if (mode == "graph")
                    {
                        //                        qDebug() << " create undirected Edge no " << edgeCount;
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                    }
                    else
                    {
                        //                        qDebug() << " create directed Edge no "<< edgeCount;
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Directed, true, false,
                                   QString(), false);
                    }
                }
                //                else
                //                    qDebug() << "do not create Edge";
            }

            progressUpdate(++progressCounter);
        }
    }
    else
    {
        qDebug() << "G(n,M) model...";
        int source = 0, target = 0;
        do
        {
            source = rand() % N + 1;
            target = rand() % N + 1;
            //            qDebug() << "random pair " << " " << source << " , " << target ;
            if (!diag && source == target)
            {
                //                qDebug() << "skip self loop pair ";
                continue;
            }
            if (edgeExists(source, target))
            {
                //                qDebug() << "skip pair - exists";
                continue;
            }
            edgeCount++;
            if (mode == "graph")
            {
                //                qDebug() << "create " << " undirected Edge no " << edgeCount;
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Undirected, false, false,
                           QString(), false);
            }
            else
            {
                //                qDebug() << "create " << " directed Edge no " << edgeCount;
                edgeCreate(source, target, 1, initEdgeColor,
                           EdgeType::Directed, true, false,
                           QString(), false);
            }
            progressUpdate(++progressCounter);
        } while (edgeCount != m);
    }

    relationCurrentRename(tr("erdos-renyi"), true);

    progressUpdate((m != 0 ? m : N));
    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);
}

/**
 * @brief Creates a scale-free random-network network
 * @param N
 * @param power
 * @param m0
 * @param m
 * @param alpha
 * @param mode
 */
void Graph::randomNetScaleFreeCreate(const int &N,
                                     const int &power,
                                     const int &m0,
                                     const int &m,
                                     const qreal &alpha,
                                     const QString &mode)
{
    qDebug() << "Graph::randomNetScaleFreeCreate() - max nodes n" << N
             << "power" << power
             << "edges added in every round m" << m
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

    qDebug() << "Graph::randomNetScaleFreeCreate() - "
             << "Create initial connected net of m0 nodes";

    QString pMsg = tr("Creating Scale-Free Random Network. \n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (int i = 0; i < m0; ++i)
    {
        x = x0 + radius * cos(i * rad);
        y = y0 + radius * sin(i * rad);

        qDebug() << "Graph::randomNetScaleFreeCreate() - "
                 << " initial node i " << i + 1 << " pos " << x << "," << y;
        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    for (int i = 0; i < m0; ++i)
    {
        qDebug() << "Graph::randomNetScaleFreeCreate() - "
                 << " Creating all edges for initial node i " << i + 1;
        for (int j = i + 1; j < m0; ++j)
        {
            qDebug() << "Graph::randomNetScaleFreeCreate() ---- "
                        "Creating initial edge "
                     << i + 1 << " <->" << j + 1;
            edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                       EdgeType::Undirected, false, false,
                       QString(), false);
        }
        progressUpdate(++progressCounter);
    }

    qDebug() << "Graph::randomNetScaleFreeCreate() - @@@@ "
             << " start network growth to " << N
             << " nodes with preferential attachment";

    for (int i = m0; i < N; ++i)
    {

        x = x0 + radius * cos(i * rad);
        y = y0 + radius * sin(i * rad);

        qDebug() << "Graph::randomNetScaleFreeCreate() - ++++"
                 << " adding new node i " << i + 1
                 << " pos " << x << "," << y;

        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);

        progressUpdate(++progressCounter);

        // need to multiply by 2, since we have a undirected graph
        // and edgesEnabled reports edges/2
        sumDegrees = 2 * edgesEnabled();

        newEdges = 0;

        qDebug() << "Graph::randomNetScaleFreeCreate() - repeat until we reach"
                 << m << "new edges for node" << i + 1;
        for (;;)
        { // do until we create m new edges

            for (int j = 0; j < i; ++j)
            {
                qDebug() << "Graph::randomNetScaleFreeCreate() - "
                         << " preferential attachment test of new node i "
                         << i + 1
                         << " with node j " << j + 1
                         << " - newEdges " << newEdges;

                if (newEdges == m)
                    break;

                k_j = vertexDegreeIn(j + 1);
                k_j = pow(k_j, power);
                if (sumDegrees < 1)
                    prob_j = 1; // always create edge if no other edge exist
                else
                    prob_j = (alpha + k_j) / sumDegrees;

                prob = (rand() % 100 + 1) / 100.0;

                qDebug() << "Graph::randomNetScaleFreeCreate() - "
                         << " Edge probability with old node "
                         << j + 1 << " is: alpha + k_j ^ power " << alpha + k_j
                         << " / sumDegrees " << sumDegrees
                         << " = prob_j " << prob_j
                         << " prob " << prob;

                if (prob <= prob_j)
                {
                    if (mode == "graph")
                    {
                        qDebug() << "Graph::randomNetScaleFreeCreate()  <----->"
                                    "Creating pref.att. undirected edge "
                                 << i + 1 << " <->" << j + 1;
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Undirected, false, false,
                                   QString(), false);
                        newEdges++;
                    }
                    else
                    {
                        qDebug() << "Graph::randomNetScaleFreeCreate()  ----->"
                                    "Creating pref.att. directed edge "
                                 << i + 1 << " <->" << j + 1;
                        edgeCreate(i + 1, j + 1, 1, initEdgeColor,
                                   EdgeType::Directed, true, false,
                                   QString(), false);
                        newEdges++;
                    }
                }
            }
            if (newEdges == m)
                break;
        }
        qDebug() << "Graph::randomNetScaleFreeCreate() - " << m << "edges reached "
                                                                   "for node"
                 << i + 1;
    }

    relationCurrentRename(tr("scale-free"), true);

    qDebug() << "Graph::randomNetScaleFreeCreate() - finished.";

    setModStatus(ModStatus::VertexEdgeCount);

    progressFinish();

    layoutVertexSizeByIndegree();
}

/**
 * @brief Creates a small world random network
 * @param vert
 * @param degree
 * @param beta
 */
void Graph::randomNetSmallWorldCreate(const int &N, const int &degree,
                                      const double &beta, const QString &mode)
{
    qDebug() << "Creating small-world randome network. Vertices: " << N
             << "degree: " << degree
             << "beta: " << beta
             << "mode: " << mode
             << "First creating a ring lattice";

    if (mode == "graph")
    {
        setDirected(false);
    }

    randomNetRingLatticeCreate(N, degree, true);

    QString pMsg = tr("Creating Small-World Random Network. \n"
                      "Please wait ...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    qDebug("******** Graph: REWIRING starts...");

    int candidate = 0;
    int progressCounter = 1;

    for (int i = 1; i < N; i++)
    {
        for (int j = i + 1; j < N; j++)
        {
            //            qDebug()<<">>>>> REWIRING: Check if  "<< i << " is linked to " << j;
            if (edgeExists(i, j))
            {
                //                qDebug()<<">>>>> REWIRING: They're linked. Do a random REWIRING "
                //                          "Experiment between "<< i<< " and " << j
                //                       << " Beta parameter is " << beta;
                if (rand() % 100 < (beta * 100))
                {
                    //                    qDebug(">>>>> REWIRING: We'l break this edge!");
                    edgeRemove(i, j, true);
                    //                    qDebug()<<">>>>> REWIRING: OK. Let's create a new edge!";
                    for (;;)
                    {                                 // do until we create a new edge
                        candidate = rand() % (N + 1); // pick another vertex.
                        if (candidate == 0 || candidate == i)
                            continue;
                        //                        qDebug()<<">>>>> REWIRING: Candidate: "<< candidate;
                        // Only if differs from i and hasnot edge with it
                        //                        qDebug("<---->Random New Edge Experiment between %i and %i:", i, candidate);
                        if (edgeExists(i, candidate) != 0)
                            continue;
                        if (rand() % 100 > 0.5)
                        {
                            //                            qDebug() << "Creating new edge";
                            edgeCreate(i, candidate, 1, initEdgeColor,
                                       EdgeType::Undirected, false, false,
                                       QString(), false);
                            break;
                        }
                    }
                }
                else
                    qDebug("Will not break link!");
            }
        }
        progressUpdate(++progressCounter);
    }

    relationCurrentRename(tr("small-world"), true);

    progressFinish();

    layoutVertexSizeByIndegree();

    setModStatus(ModStatus::VertexEdgeCount);
}

/**
 * @brief Creates a random network where nodes have the same degree.
 * @param vert
 * @param degree
 */
void Graph::randomNetRegularCreate(const int &N,
                                   const int &degree,
                                   const QString &mode, const bool &diag)
{
    qDebug() << "Creating d-regular random network...";
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
    progressCreate(N, pMsg);

    qDebug() << " creating vertices";

    for (int i = 0; i < N; i++)
    {
        x = canvasRandomX();
        y = canvasRandomY();

        //        qDebug() << " creating new vertex at "
        //                    << x << "," << y;

        vertexCreate(
            i + 1, initVertexSize, initVertexColor,
            initVertexNumberColor, initVertexNumberSize,
            QString::number(i + 1), initVertexLabelColor, initVertexLabelSize,
            QPoint(x, y), initVertexShape, initVertexIconPath, false);
    }

    qDebug() << " Creating initial edges";

    if (mode == "graph")
    {
        for (int i = 0; i < N; i++)
        {
            qDebug() << " "
                        "Creating undirected edges for node  "
                     << i + 1;
            for (int j = 0; j < degree / 2; j++)
            {
                target = i + j + 1;
                if (target > (N - 1))
                    target = target - N;
                qDebug() << " undirected edge "
                         << i + 1 << "<->" << target + 1;
                m_edges.append(QString::number(i + 1) + "->" + QString::number(target + 1));
            }
        }
    }
    else
    {
        for (int i = 0; i < N; i++)
        {
            //            qDebug()<< "Creating directed edges for node  "<< i+1;
            for (int j = 0; j < degree; j++)
            {
                target = i + j + 1;
                if (target > (N - 1))
                    target = target - N;
                //                qDebug()<< " directed edge "<< i+1 << "->"<< target+1;
                m_edges.append(QString::number(i + 1) + "->" + QString::number(target + 1));
            }
        }
    }
    qDebug() << "Edge list count:" << m_edges.size()
             << "Now reordering all edges in pairs...";

    // take randomly two edges, of different vertices and combine their source
    // and target vertices to two different edges
    for (int i = 1; i < m_edges.size(); ++i)
    {

        firstEdgeVertices.clear();
        secondEdgeVertices.clear();
        firstEdgeVertices << "";
        firstEdgeVertices << "";
        secondEdgeVertices << "";
        secondEdgeVertices << "";
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
            //            qDebug()<< " firstEdgeVertices:"
            //                    << firstEdgeVertices
            //                    << " secondEdgeVertices:" << secondEdgeVertices;
        }
        //        qDebug()<< " removing edges:" <<firstEdge << secondEdge;
        m_edges.removeAll(firstEdge);
        m_edges.removeAll(secondEdge);

        //        qDebug()<< " 2 edges deleted for reordering:"
        //                << firstEdgeVertices[0] << "->" << firstEdgeVertices[1]
        //                << "and"
        //                << secondEdgeVertices[0] << "->" << secondEdgeVertices[1]
        //                << "edge list count:" << m_edges.size();

        m_edges.append(firstEdgeVertices[0] + "->" + secondEdgeVertices[1]);
        m_edges.append(secondEdgeVertices[0] + "->" + firstEdgeVertices[1]);

        //        qDebug()<< " 2 new edges added:"
        //                << firstEdgeVertices[0] << "->" << secondEdgeVertices[1]
        //                <<"and"
        //                << secondEdgeVertices[0]<<"->"<<firstEdgeVertices[1]
        //                << "final edge list count:" << m_edges.size();
    }

    //
    // draw edges
    //
    for (int i = 0; i < m_edges.size(); ++i)
    {

        m_edge = m_edges.at(i).split("->");

        //        qDebug() << "Drawing undirected Edge" <<
        //                 << m_edge[0].toInt(0) << "<->" << m_edge[1].toInt(0);

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
        }
    }

    relationCurrentRename(tr("d-regular"), true);

    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);
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
 * @brief Creates a lattice network
 * @param N
 * @param length
 * @param dimension
 * @param neighborhood
 * @param mode
 * @param diag
 */
void Graph::randomNetLatticeCreate(const int &N,
                                   const int &length,
                                   const int &dimension,
                                   const int &neighborhoodLength,
                                   const QString &mode,
                                   const bool &circular)
{
    qDebug() << "Creating lattice network...";
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
    progressCreate(N, pMsg);

    // create vertices

    qDebug() << "creating vertices";

    nCount = 0;
    canvasPadding = 20;
    nodeHPadding = (canvasWidth) / (double)(length + 2);
    nodeVPadding = (canvasHeight) / (double)(length + 2);

    qDebug() << "canvasPadding" << canvasPadding
             << "nodeHPadding" << nodeHPadding;

    for (int i = 0; i < length; i++)
    {

        // compute vertical pos
        y = canvasPadding + nodeVPadding * (i + 1);

        for (int j = 0; j < length; j++)
        {
            nCount++;
            // compute horizontal pos
            x = canvasPadding + nodeHPadding * (j + 1);

            //            qDebug() << "creating new vertex at"
            //                     << x << "," << y;

            // create vertex
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

    //
    // compute and then draw edges
    //

    qDebug() << "Computing edges";

    if (mode == "graph")
    {

        // undirected graph

        for (int i = 1; i <= N; i++)
        {

            //            qDebug()<< "Creating undirected edges for node  "<< i;

            for (int j = 1; j < neighborhoodLength + 1; j++)
            {

                for (int p = 0; p < 2; p++)
                {

                    for (int q = 0; q < 2; q++)
                    {

                        target = i + pow((-1), p) * j * pow(length, q);

                        if (i % length == 0 && target == i + 1)
                        {
                            //                            qDebug()<< i << "<->"<< target << "OOB RIGHT";
                            continue;
                        }
                        if (i % length == 1 && target == i - 1)
                        {
                            //                            qDebug()<< i << "<->"<< target << "OOB LEFT";
                            continue;
                        }
                        if (target > N)
                        {
                            //                            qDebug()<< i << "<->"<< target << "OOB DOWN";
                            target = target % N;
                            continue;
                        }

                        if (target < 1)
                        {
                            //                            qDebug()<< i << "<->"<< target << "OOB UP";
                            target = N - target;
                            continue;
                        }
                        //                        qDebug()<< i << "<->"<< target << "OK";

                        edge = QString::number(i) + "<->" + QString::number(target);
                        oppEdge = QString::number(target) + "<->" + QString::number(i);

                        if (!latticeEdges.contains(edge) && !latticeEdges.contains(oppEdge))
                        {
                            latticeEdges.append(QString::number(i) + "<->" + QString::number(target));
                        }
                    }
                }
            }
        }
    }

    else
    {

        // directed graph
    }

    //
    // draw edges
    //

    qDebug() << "drawing edges";
    progressFraction = (latticeEdges.size() > 0) ? 1.0 / (qreal)latticeEdges.size() : 1.0;
    for (int i = 0; i < latticeEdges.size(); ++i)
    {

        m_edge = latticeEdges.at(i).split("<->");

        //        qDebug() << "Drawing undirected Edge no" << i + 1 << ":"
        //                 << m_edge[0].toInt(0) << "<->" << m_edge[1].toInt(0);

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
        }
    }

    relationCurrentRename(tr("lattice"), true);

    progressFinish();

    setModStatus(ModStatus::VertexEdgeCount);
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
