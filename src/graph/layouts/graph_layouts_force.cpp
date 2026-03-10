/**
 * @file graph_layouts_force.cpp
 * @brief Implements force-directed layout algorithms of the Graph class (e.g., spring-embedder and related force-based positioning methods).
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
#include <QApplication>
#include <QDebug>
#include <cmath>

/**
 * @brief Embeds a Force Directed Placement layout according to the initial Spring Embedder model proposed by Eades.
 * @param maxIterations
 *  The Spring Embedder model (Eades, 1984), part of the Force Directed Placement (FDP) family,
    assigns forces to all vertices and edges, as if nodes were electrically charged particles (Coulomb's law)
    and all edges were springs (i.e. Hooke's law).

    These forces are applied to the nodes iteratively, pulling them closer together or pushing them further apart,
    until the system comes to an equilibrium state (node positions do not change anymore).

    Note that, following Eades, we do not need to have a faithful simulation;
    we can -and we do- apply unrealistic forces in an unrealistic manner.
    For instance, instead of the forces described by Hooke's law,
    we will assume weaker logarithmic forces between far apart vertices...
 */
void Graph::layoutForceDirectedSpringEmbedder(const int maxIterations)
{

    int iteration = 1;
    int progressCounter = 0;
    qreal dist = 0;
    qreal f_rep = 0, f_att = 0;
    QPointF DV;
    qreal c4 = 0.1; // normalization factor for final displacement

    VList::const_iterator v1;
    VList::const_iterator v2;

    /**
     * compute max spring length as function of canvas area divided by the
     * total vertices area
     */
    qreal V = (qreal)vertices();
    qreal naturalLength = computeOptimalDistance(V);
    qDebug() << "\n\n layoutForceDirectedSpringEmbedder() "
             << " vertices " << V
             << " naturalLength " << naturalLength;

    /* apply an initial random layout */
    layoutRandomInMemory();

    QString pMsg = tr("Embedding Eades Spring-Gravitational model. \n"
                      "Please wait ....");
    progressStatus(pMsg);
    progressCreate(maxIterations, pMsg);

    for (iteration = 1; iteration <= maxIterations; iteration++)
    {

        // setup init disp
        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
            qDebug() << " 0000 s " << (*v1)->number() << " zeroing rx/ry";
        }

        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            qDebug() << "*********  Calculate forces for source s  "
                     << (*v1)->number()
                     << " pos " << (*v1)->x() << ", " << (*v1)->y();

            if (!(*v1)->isEnabled())
            {
                qDebug() << "  vertex s disabled. Continue";
                continue;
            }

            for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
            {
                if (!(*v2)->isEnabled())
                {
                    qDebug() << "   t " << (*v1)->number() << " disabled. Continue";
                    continue;
                }

                if (v2 == v1)
                {
                    qDebug() << "   s==t, continuing";
                    continue;
                }

                DV.setX((*v2)->x() - (*v1)->x());
                DV.setY((*v2)->y() - (*v1)->y());

                dist = graphDistanceEuclidean(DV);

                /**
                 *  calculate electric (repulsive) forces between
                 *  all vertices.
                 */
                f_rep = layoutForceDirected_F_rep("Eades", dist, naturalLength);
                (*v1)->disp().rx() += sign(DV.x()) * f_rep;
                (*v1)->disp().ry() += sign(DV.y()) * f_rep;
                qDebug() << "  s = " << (*v1)->number()
                         << " pushed away from t = " << (*v2)->number()
                         << " dist " << dist
                         << " f_rep=" << f_rep
                         << " sign * f_repx " << sign(DV.x()) * f_rep
                         << " sign * f_repy " << sign(DV.y()) * f_rep;

                /**
                 * calculate spring forces between adjacent nodes
                 * that pull them together (if d > naturalLength)
                 * or push them apart (if d < naturalLength)
                 */
                if (this->edgeExists((*v1)->number(), (*v2)->number()))
                {

                    f_att = layoutForceDirected_F_att("Eades", dist, naturalLength);

                    (*v1)->disp().rx() += sign(DV.x()) * f_att;
                    (*v1)->disp().ry() += sign(DV.y()) * f_att;
                    (*v2)->disp().rx() -= sign(DV.x()) * f_att;
                    (*v2)->disp().ry() -= sign(DV.y()) * f_att;

                    qDebug() << "  s= " << (*v1)->number()
                             << " attracted by t= " << (*v2)->number()
                             << " dist " << dist
                             << " f_att=" << f_att
                             << " sdx * f_att " << sign(DV.x()) * f_att
                             << " sdy * f_att " << sign(DV.y()) * f_att
                             << " disp_s.x=" << (*v1)->disp().rx()
                             << " disp_s.y=" << (*v1)->disp().ry()
                             << " disp_t.x=" << (*v2)->disp().rx()
                             << " disp_t.y=" << (*v2)->disp().ry();

                } // end if edgeExists

            } // end for v2

            qDebug() << "  >>> final s = " << (*v1)->number()
                     << " disp_s.x=" << (*v1)->disp().rx()
                     << " disp_s.y=" << (*v1)->disp().ry();

        } // end for v1

        layoutForceDirected_Eades_moveNodes(c4);

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            setModStatus(ModStatus::VertexPositions);
            return;
        }        

    } // end iterations

    // Emit all final node positions in one bulk pass now that the
    // iteration loop is complete — avoids N×maxIterations signal emissions.
    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        emit setNodePos((*v1)->number(), (*v1)->x(), (*v1)->y());
    }

    progressFinish();

    setModStatus(ModStatus::VertexPositions);
}

/**
 * @brief Embeds a Force Directed Placement layout according to the Fruchterman-Reingold model.
  *  Fruchterman and Reingold (1991) refined the Spring Embedder model by replacing the forces.
    In this model, "the vertices behave as atomic particles or celestial bodies,
    exerting attractive and repulsive forces on one another." (ibid).
    Again, only vertices that are neighbours attract each other but, unlike Spring Embedder,
    all vertices repel each other.
    These forces induce movement. The algorithm might resemble molecular or planetary simulations,
    sometimes called n-body problems.
 * @param maxIterations
 */
void Graph::layoutForceDirectedFruchtermanReingold(const int maxIterations)
{
    int progressCounter = 0;
    qreal dist = 0;
    qreal f_att, f_rep;
    QPointF DV; // difference vector

    qreal V = (qreal)vertices();
    qreal C = 0.9; // this is found experimentally
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    qreal optimalDistance = C * computeOptimalDistance(V);

    VList::const_iterator v1, v2;
    int iteration = 1;

    /* apply an initial circular layout */
    // layoutCircular(canvasWidth/2.0, canvasHeight/2.0, optimalDistance/2.0,false);
    // layoutRandom();

    qDebug() << "Graph: layoutForceDirectedFruchtermanReingold() ";
    qDebug() << "Graph: Setting optimalDistance = " << optimalDistance
             << "...following Fruchterman-Reingold (1991) formula ";

    qDebug() << "Graph: canvasWidth " << canvasWidth << " canvasHeight " << canvasHeight;

    QString pMsg = tr("Embedding Fruchterman & Reingold forces model. \n"
                      "Please wait ...");
    progressStatus(pMsg);

    progressCreate(maxIterations, pMsg);

    for (iteration = 1; iteration <= maxIterations; iteration++)
    {

        // setup init disp
        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
            // qDebug() << " 0000 s " << (*v1)->number() << " zeroing rx/ry";
        }

        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            //            qDebug() << "*****  Calculate forces for s " << (*v1)->number()
            //                     << " vpos " <<  vpos[(*v1)->number()]
            //                     << " pos "<< (*v1)->x() << ", "<< (*v1)->y();

            if (!(*v1)->isEnabled())
            {
                //                qDebug() << "  vertex s " << (*v1)->number() << " disabled. Continue";
                continue;
            }

            for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
            {
                //                qDebug () << "  t = "<< (*v2)->number()
                //                          << "  pos (" <<  (*v2)->x() << "," << (*v2)->y() << ")";

                if (!(*v2)->isEnabled())
                {
                    //                    qDebug()<< " t "<< (*v2)->number()<< " disabled. Continue";
                    continue;
                }

                if (v2 == v1)
                {
                    //                    qDebug() << "  s==t, continuing";
                    continue;
                }

                DV.setX((*v2)->x() - (*v1)->x());
                DV.setY((*v2)->y() - (*v1)->y());

                dist = graphDistanceEuclidean(DV);

                // calculate repulsive force from _near_ vertices
                f_rep = layoutForceDirected_F_rep("FR", dist, optimalDistance);
                (*v1)->disp().rx() += sign(DV.x()) * f_rep;
                (*v1)->disp().ry() += sign(DV.y()) * f_rep;

                //                qDebug()<< " dist( " << (*v1)->number() <<  "," <<  (*v2)->number() <<  " = "
                //                        << dist
                //                        << " f_rep " << f_rep
                //                        << " disp_s.x="<< (*v1)->disp().rx()
                //                        << " disp_s.y="<< (*v1)->disp().ry();

                if (edgeExists((*v1)->number(), (*v2)->number()))
                {
                    // calculate attracting force
                    f_att = layoutForceDirected_F_att("FR", dist, optimalDistance);
                    (*v1)->disp().rx() += sign(DV.x()) * f_att;
                    (*v1)->disp().ry() += sign(DV.y()) * f_att;
                    (*v2)->disp().rx() -= sign(DV.x()) * f_att;
                    (*v2)->disp().ry() -= sign(DV.y()) * f_att;

                    qDebug() << "  s= " << (*v1)->number()
                             << " attracted by t= " << (*v2)->number()
                             << "  optimalDistance =" << optimalDistance
                             << " f_att " << f_att
                             << " disp_s.x=" << (*v1)->disp().rx()
                             << " disp_s.y=" << (*v1)->disp().ry()
                             << " disp_t.x=" << (*v2)->disp().rx()
                             << " disp_t.y=" << (*v2)->disp().ry();
                } // endif

            } // end for v2

        } // end for v1

        // limit the max displacement to the temperature t
        // prevent placement outside of the frame/canvas
        layoutForceDirected_FR_moveNodes(layoutForceDirected_FR_temperature(iteration));

        progressUpdate(++progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            setModStatus(ModStatus::VertexPositions);
            return;
        }        
    }

        // Emit all final node positions in one bulk pass now that the
    // iteration loop is complete — avoids N×maxIterations signal emissions.
    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        emit setNodePos((*v1)->number(), (*v1)->x(), (*v1)->y());
    }

    progressFinish();

    setModStatus(ModStatus::VertexPositions);  // was missing on normal exit path
}

/**
 * @brief Embeds a Force Directed Placement layout according to the Kamada-Kawai model.
 * In this model, the network is considered to be a dynamic system
 * where every two actors are 'particles' mutually connected by a 'spring'.
 * Each spring has a desirable length, which corresponds to their graph
 * theoretic distance. In this way, the optimal layout of the graph
 * is the state with the minimum imbalance. The degree of
 * imbalance is formulated as the total spring energy:
 * the square summation of the differences between desirable
 * distances and real ones for all pairs of particles
 * Initially, the particles/actors are placed on the vertices of a regular n-polygon
 */
void Graph::layoutForceDirectedKamadaKawai(const int maxIterations,
                                           const bool considerWeights,
                                           const bool inverseWeights,
                                           const bool dropIsolates,
                                           const QString &initialPositions)
{

    qDebug() << "Embedding an FDP layout according to the Kamada-Kawai model, maxIterations:" << maxIterations;

    VList::const_iterator v1, v2;

    int progressCounter = 0, minimizationIterations = 0;

    int i = 0, j = 0, m = 0, pm = 0, pnm = 0, pn = 0;

    int N = vertices(); // active actors

    qreal K = 1;  // constant
    qreal L = 0;  // the desirable length of a single edge.
    qreal L0 = 0; // the length of a side of the display square area
    qreal D = 0;  // the graph diameter

    Matrix l; // the original spring length between pairs of particles/actors
    Matrix k; // the strength of the spring between pairs of particles/actors

    Matrix LIN_EQ_COEF(2, 2); // holds the coefficients of set of linear equations 11 and 12
    qreal b[2];               // holds the right hand vector of linear equations 11 and 12

    qreal partDrvtEx = 0;        // partial derivative of E by Xm
    qreal partDrvtEy = 0;        // partial derivative of E by Ym
    qreal partDrvtExSec_m = 0;   // partial second derivative of E by Xm
    qreal partDrvtEySec_m = 0;   // partial second derivative of E by Ym
    qreal partDrvtExEySec_m = 0; // partial second derivative of E by Xm Ym
    qreal partDrvtEyExSec_m = 0; // partial second derivative of E by Ym Xm

    qreal partDrvtEx_m = 0; // cache for partial derivative of E by Xm, for particle with max D_i
    qreal partDrvtEy_m = 0; // cache for partial derivative of E by Ym, for particle with max D_i

    qreal partDrvDenom = 0;
    qreal xm = 0, ym = 0;
    qreal xi = 0, yi = 0;
    qreal xpm = 0, ypm = 0; // cache for pos of particle with max D_i

    qreal dx = 0, dy = 0;

    qreal epsilon = 0.1;
    qreal Delta_m = 0;
    qreal Delta_max = epsilon + 0.0001;

    bool couldNotSolveLinearSystem = false;

    // Compute graph-theoretic distances dij for 1 <= i!=j <= n

    qDebug() << "Compute dij, where (i,j) in E";

    if (!graphMatrixDistanceGeodesicCreate(considerWeights, inverseWeights, dropIsolates))
    {
        return;
    }

    if (progressCanceled())
    {
        return;
    }

    // processEvents() ensures the geodesic progress dialog updates are
    // flushed before we start the KK iteration phase.
    QApplication::processEvents();

    // Use the cached diameter — graphMatrixDistanceGeodesicCreate() already
    // ran the DistanceEngine so re-calling graphDiameter() would redundantly
    // re-trigger it if the cache were ever invalidated.
    D = graphDiameterCached();
    L0 = canvasMinDimension() - 100;

    // Guard against degenerate graphs (no edges, all isolates, single node)
    // where D == 0, which would cause division by zero.
    if (D <= 0)
    {
        qDebug() << "KK layout: graph diameter is 0 (degenerate graph). Aborting.";
        progressFinish();
        setModStatus(ModStatus::VertexPositions);
        return;
    }

    // L = L0 / D: the desirable length of a single edge in the display pane
    // scales with the graph diameter so the layout fills the canvas.
    L = L0 / D;

    qDebug() << "Compute lij = L x dij. lij will be symmetric."
             << "L0=" << L0 << "D=" << D << "L=" << L;

    l.zeroMatrix(DM.rows(), DM.cols());
    l = DM;
    l.multiplyScalar(L);

    //    qDebug()<< "Graph::layoutForceDirectedKamadaKawai() - l=" ;
    //    l.printMatrixConsole();

    // Compute kij for 1 <= i!=j <= n using the formula:
    // kij = K / dij ^2
    // kij is the strength of the spring between pi and pj, K a constant

    qDebug() << "Compute kij = K / dij ^2. kij will be symmmetric. ";

    k.zeroMatrix(DM.rows(), DM.cols());

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (i == j)
                continue;
            qreal dij = DM.item(i, j);
            if (dij <= 0 || dij >= RAND_MAX)
            {
                // disconnected or degenerate pair — no spring force
                k.setItem(i, j, 0);
                continue;
            }
            k.setItem(i, j, K / (dij * dij));
        }
    }
    //    qDebug()<< "Graph::layoutForceDirectedKamadaKawai() - k=" ;
    //    k.printMatrixConsole();

    // initialize p1, p2, ... pn
    qDebug() << "Set particles to initial positions p";

    i = 0;

    if (initialPositions == "circle")
    {
        double x0 = 0, y0 = 0;
        x0 = canvasWidth / 2.0;
        y0 = canvasHeight / 2.0;
        // placing the particles on the vertices of a regular n-polygon
        // circumscribed by a circle whose diameter is L0
        layoutCircular(x0, y0, L0 / 2, false);
    }
    else if (initialPositions == "random")
    {
        layoutRandom();
    }

    QString pMsg = tr("Embedding Kamada & Kawai spring model.\n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(maxIterations, pMsg);

    // while ( max_D_i > e )
    while (Delta_max > epsilon)
    {
        couldNotSolveLinearSystem = false;

        progressCounter++;
        progressUpdate(progressCounter);
        if (progressCanceled())
        {
            progressFinish();
            setModStatus(ModStatus::VertexPositions);
            return;
        }
        if (progressCounter == maxIterations)
        {
            //            qDebug()<< "Reached maxIterations. BREAK";
            break;
        }

        Delta_max = epsilon;

        // choose particle with largest Delta_m = max Delta_i
        // compute partial derivatives of E by xm and ym for every particle m
        // using equations 7 and 8

        qDebug() << "Iteration:" << progressCounter
                 << "Choose particle with largest Delta_m = max Delta_i ";

        pnm = -1;

        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {

            pn = (*v1)->number();
            m = vpos[pn];
            xm = (*v1)->x();
            ym = (*v1)->y();

            //            qDebug()<< "Compute partial derivatives E for particle" << pn
            //                    << " vpos m" <<  m
            //                    << " pos"<< xm << ", "<< ym;

            if (!(*v1)->isEnabled())
            {

                //                qDebug() << "  particle " << pn
                //                         << " vpos m " << m << " disabled. Continue";
                continue;
            }

            partDrvtEx = 0;
            partDrvtEy = 0;

            for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
            {

                i = vpos[(*v2)->number()];
                xi = (*v2)->x();
                yi = (*v2)->y();

                //                qDebug () << "  particle vpos i"<< i
                //                          << "  pos (" <<  xi << "," << yi << ")";

                if (!(*v2)->isEnabled())
                {
                    qDebug() << " i " << (*v2)->number() << " disabled. Continue";
                    continue;
                }

                if (m == i)
                {
                    qDebug() << "  m==i, continuing";
                    continue;
                }
                qreal denom1 = sqrt((xm - xi) * (xm - xi) + (ym - yi) * (ym - yi));
                if (denom1 < 1e-6)
                {
                    // particles at same position — skip to avoid division by zero
                    continue;
                }
                partDrvtEx += k.item(m, i) * ((xm - xi) - (l.item(m, i) * (xm - xi)) / denom1);
                partDrvtEy += k.item(m, i) * ((ym - yi) - (l.item(m, i) * (ym - yi)) / denom1);

            } // end v2 for loop

            Delta_m = sqrt(partDrvtEx * partDrvtEx + partDrvtEy * partDrvtEy);

            //            qDebug()<< "m" << m << " Delta_m" << Delta_m;

            if (Delta_m > Delta_max)
            {

                //                qDebug()<< "m" << m << " Delta_m > Delta_max. Setting new Delta_max = "<< Delta_m;

                Delta_max = Delta_m;
                partDrvtEx_m = partDrvtEx;
                partDrvtEy_m = partDrvtEy;

                pnm = pn; // store name of particle satisfying Delta_m = max Delta_i
                pm = m;   // store vpos of particle satisfying Delta_m = max Delta_i
                xpm = xm; // store particle x pos
                ypm = ym; // store particle y pos
            }

        } // end v1 for loop

        if (pnm < 0)
        {
            //            qDebug () << "No particle left with Delta_m > epsilon -- BREAK";
            break;
        }

        // let pm the particle satisfying  D_m = max_D_i
        m = pm;
        xm = xpm;
        ym = ypm;

        //        qDebug () << "m"<< m<< "has max Delta_m"<< Delta_max
        //                  << " Starting minimizing Delta_m - "
        //                << " initial m pos " << xm << ym;

        minimizationIterations = 0;

        // while ( D_m > e)
        do
        {
            if (minimizationIterations > 10)
            {
                qDebug() << "Reached minimizationIterations threshold. BREAK";
                break;
            }
            minimizationIterations++;

            //            qDebug () << "Started minimizing Delta_m for m"<< m
            //                      << "First compute dx and dy by solving equations 11 and 12 ";

            // compute dx and dy by solving equations 11 and 12

            partDrvtEx = 0;
            partDrvtEy = 0;
            partDrvtEx_m = 0;
            partDrvtEy_m = 0;
            partDrvtExSec_m = 0;
            partDrvtEySec_m = 0;
            partDrvtExEySec_m = 0;
            partDrvtEyExSec_m = 0;

            // first compute coefficients of the linear system equations
            for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
            {

                i = vpos[(*v2)->number()];
                xi = (*v2)->x();
                yi = (*v2)->y();

                //                qDebug () << "  m"<< m << "  i"<< i
                //                          << "  pos_i (" <<  xi << "," << yi << ")";

                if (!(*v2)->isEnabled())
                {
                    qDebug() << " i " << (*v2)->number() << " disabled. Continue";
                    continue;
                }

                if (i == m)
                {
                    qDebug() << "  m==i, continuing";
                    continue;
                }
                qreal denom2 = sqrt((xm - xi) * (xm - xi) + (ym - yi) * (ym - yi));
                if (denom2 < 1e-6)
                {
                    // particles at same position — skip to avoid division by zero
                    continue;
                }
                partDrvDenom = denom2 * denom2 * denom2;
                partDrvtEx_m += k.item(m, i) * ((xm - xi) - (l.item(m, i) * (xm - xi)) / denom2);
                partDrvtEy_m += k.item(m, i) * ((ym - yi) - (l.item(m, i) * (ym - yi)) / denom2);

                partDrvtExSec_m += k.item(m, i) * (1 - (l.item(m, i) * (ym - yi) * (ym - yi)) / partDrvDenom);

                partDrvtExEySec_m += k.item(m, i) * ((l.item(m, i) * (xm - xi) * (ym - yi)) / partDrvDenom);

                partDrvtEyExSec_m += k.item(m, i) * ((l.item(m, i) * (xm - xi) * (ym - yi)) / partDrvDenom);

                partDrvtEySec_m += k.item(m, i) * (1 - (l.item(m, i) * (xm - xi) * (xm - xi)) / partDrvDenom);

            } // end v2 for loop

            Delta_m = sqrt(partDrvtEx_m * partDrvtEx_m + partDrvtEy_m * partDrvtEy_m);

            //            qDebug () << "m"<< m << " new Delta_m" << Delta_m;

            LIN_EQ_COEF.setItem(0, 0, partDrvtExSec_m);
            LIN_EQ_COEF.setItem(0, 1, partDrvtExEySec_m);
            LIN_EQ_COEF.setItem(1, 0, partDrvtEyExSec_m);
            LIN_EQ_COEF.setItem(1, 1, partDrvtEySec_m);
            //            qDebug()<< "Jacobian Matrix of coefficients for linear system (eq. 11 & 12) is:";
            //            LIN_EQ_COEF.printMatrixConsole();
            b[0] = -partDrvtEx_m;
            b[1] = -partDrvtEy_m;
            //            qDebug()<< "right hand vector is: \n"  << b[0] << " \n" << b[1];
            //            qDebug()<< "solving linear system...";
            if (!LIN_EQ_COEF.solve(b))
            {
                couldNotSolveLinearSystem = true;
                continue;
            }
            //            qDebug()<< "solved linear system.";
            dx = b[0];
            dy = b[1];
            //            qDebug()<< "Solution \n b[0] = dx =" << dx << "\n b[1] = dy =" << dy;

            //            qDebug () << "m"<< m << " current m pos " << xm << ym << " new m pos " << xm +dx << ym+dy;

            if ((xm + dx) < 50 || (xm + dx) > (canvasWidth - 50))
            {
                //                qDebug () << "new xm out of canvas, setting random x";
                xm = canvasRandomX();
            }
            else
            {
                xm = xm + dx;
            }
            if ((ym + dy) < 50 || (ym + dy) > (canvasHeight - 50))
            {
                //                qDebug() <<  "new ym out of canvas, setting random y";
                ym = canvasRandomY();
            }
            else
            {
                ym = ym + dy;
            }

            qDebug() << "m" << m << " new m pos " << xm << ym;

            // TODO CHECK IF WE HAVE REACHED A FIXED POINT LOOP

        } while (Delta_m > epsilon);

        if (couldNotSolveLinearSystem)
        {
            qDebug() << "Could not solve linear system for particle " << pnm << "vpos" << m;
        }

        qDebug() << "Finished minimizing Delta_m for particle" << pnm << "vpos" << m
                 << "Minimized Delta_m" << Delta_m
                 << "moving it to new pos" << xm << ym;

        m_graph[m]->setX(xm);
        m_graph[m]->setY(ym);


    } // end while (Delta_max > epsilon) {

    qDebug() << "Delta_max =< epsilon -- RETURN";

    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {

        emit setNodePos((*v1)->number(), (*v1)->pos().x(), (*v1)->pos().y());
    }
    progressFinish();

    setModStatus(ModStatus::VertexPositions);
}

/**
 * @brief Reduces the temperature as the layout approaches a better configuration
 * @return qreal temperature
 */
qreal Graph::layoutForceDirected_FR_temperature(const int iteration) const
{
    qreal temp = 0;
    // Simmering temperature: derived from canvas width to maintain continuity
    // with the quenching phase formula canvasWidth / (iteration + 10).
    // At the quench/simmer boundary (iteration=10) the quench formula gives
    // canvasWidth/20; we use canvasWidth/210 as the constant low simmering
    // value (the quench formula evaluated at the simmer/freeze boundary,
    // iteration=200). Previously hardcoded as 5.8309518948453, which assumed
    // a fixed canvas width of ~1224px and broke on other display sizes.
    qreal temperature = canvasWidth / 210.0;
    qDebug() << "Graph::layoutForceDirected_FR_temperature(): cool iteration " << iteration;
    // For the temperature (which limits the displacement of each vertex),
    // Fruchterman & Reingold suggested in their paper that it might start
    // at an initial high value (i.e. "one tenth the width of the frame")
    // and then decay to 0 in an inverse linear fashion.
    // They also suggested "a combination of quenching and simmering",
    // which is a high initial temperature and then a constant low one.
    // This is what SocNetV does. In fact after 200 iterations we follow Eades
    // advice and stop movement (temp = 0).
    if (iteration < 10)
    {
        // quenching: starts at a high temperature (canvasWidth / 10)
        // and cools steadily and rapidly
        temp = canvasWidth / (iteration + 10.0);
    }
    else if (iteration > 200)
    {
        // follow Eades advice: freeze all movement
        temp = 0;
    }
    else
    {
        // simmering: stay at a constant low temperature
        temp = temperature;
    }
    qDebug() << "Graph::layoutForceDirected_FR_temperature() - iteration " << iteration
             << " temp " << temp;
    return temp;
}

/**
 * @brief Computes Optimal Distance. Used in Spring Embedder and FR algorithms.
 * @return qreal optimalDistance
 */
qreal Graph::computeOptimalDistance(const int &V)
{
    qreal vertexWidth = (qreal)2.0 * initVertexSize;
    qreal screenArea = canvasHeight * canvasWidth;
    qreal vertexArea = qCeil(qSqrt(screenArea / V));
    // optimalDistance (or k) is the radius of the empty area around a  vertex -
    // we add vertexWidth to it
    return (vertexWidth + vertexArea);
}

qreal Graph::layoutForceDirected_F_att(const QString model, const qreal &dist,
                                       const qreal &optimalDistance)
{
    qreal f_att;
    if (model == "Eades")
    {
        qreal c_spring = 2;
        f_att = c_spring * log10(dist / optimalDistance);
    }
    else
    { // model->FR
        f_att = (dist * dist) / optimalDistance;
    }

    return f_att;
}

qreal Graph::layoutForceDirected_F_rep(const QString model, const qreal &dist,
                                       const qreal &optimalDistance)
{
    qreal f_rep;
    if (model == "Eades")
    {
        if (dist != 0)
        {
            qreal c_rep = 1.0;
            f_rep = c_rep / (dist * dist);
            if (dist > (2.0 * optimalDistance))
            {
                // neglect vertices outside circular area of radius 2 * optimalDistance
                f_rep = 0;
            }
        }
        else
        {
            f_rep = optimalDistance; // move away
        }
    }
    else
    { // model->FR
        // To speed up our algorithm we use the grid-variant algorithm.
        if ((2.0 * optimalDistance) < dist)
        {
            // neglect vertices outside circular area of radius 2*optimalDistance
            f_rep = 0;
        }
        else
        {
            // repelsive forces are computed only for vertices within a circular area
            // of radius 2*optimalDistance
            f_rep = (optimalDistance * optimalDistance / dist);
        }
    }

    return -f_rep;
}

/**
 * @brief Graph::sign
 * returns the sign of number D as integer (1 or -1)
 * @param D
 * @return
 */
int Graph::sign(const qreal &D)
{
    if (D != 0)
    {
        return (D / qAbs(D));
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Graph::compute_angles
 * Computes the two angles of the orthogonal triangle shaped by two points
 * of difference vector DV and distance dist
 * A = 90
 * B = angle1
 * C = angle2
 *
 * @param DV
 * @param dist
 * @param angle1
 * @param angle2
 * @param degrees1
 * @param degrees2
 */
void Graph::compute_angles(const QPointF &DV,
                           const qreal &dist,
                           qreal &angle1,
                           qreal &angle2,
                           qreal &degrees1,
                           qreal &degrees2)
{
    if (dist > 0)
    {
        angle1 = qAcos(qAbs(DV.x()) / dist); // radians
        angle2 = (M_PI / 2.0) - angle1;      // radians (pi/2 -a1)
    }
    else
    {
        angle1 = 0;
        angle2 = 0;
    }
    degrees1 = angle1 * 180.0 / M_PI; // convert to degrees
    degrees2 = angle2 * 180.0 / M_PI; // convert to degrees
    qDebug() << "angle1 " << angle1 << " angle2 " << angle2
             << "deg1 " << degrees1 << " deg2 " << degrees2
             << " qCos " << qCos(angle1) << " qSin" << qSin(angle2);
}

/**
 * @brief Computes the euclideian distance between QPointF a and b
 * @param a
 * @param b
 * @return
 */
qreal Graph::graphDistanceEuclidean(const QPointF &a, const QPointF &b)
{
    return qSqrt(
        (b.x() - a.x()) * (b.x() - a.x()) +
        (b.y() - a.y()) * (b.y() - a.y()));
}

/**
 * @brief the euclideian distance of QPointF a (where a is difference vector)
 * @param a
 * @return
 */
qreal Graph::graphDistanceEuclidean(const QPointF &a)
{
    return qSqrt(
        a.x() * a.x() +
        a.y() * a.y());
}

/**
 * @brief Moves all vertices to their new positions as computed by the Eades Spring Embedder model.
 *
 * Called once per iteration from layoutForceDirectedSpringEmbedder(). Applies the
 * accumulated displacement vectors to each vertex position, scaled by the normalization
 * factor c4, and clamps the result to the visible canvas area.
 *
 * Node positions are updated in-memory only (setX/setY). The caller is responsible
 * for emitting setNodePos signals in a single bulk pass after the iteration loop
 * completes — this avoids N×maxIterations signal emissions to the graphics scene.
 *
 * @param c4 Normalization factor for the final displacement (typically 0.1).
 *           Dampens the movement to prevent oscillation.
 */
void Graph::layoutForceDirected_Eades_moveNodes(const qreal &c4)
{
    qDebug() << "\n *****  layoutForceDirected_Eades_moveNodes() ";
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    VList::const_iterator v1;

    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        // calculate new overall velocity vector
        xvel = c4 * (*v1)->disp().rx();
        yvel = c4 * (*v1)->disp().ry();
        qDebug() << " ##### source vertex  " << (*v1)->number()
                 << " xvel,yvel = (" << xvel << ", " << yvel << ")";

        // fix Qt error a positive QPoint to the floor
        //  when we ask for setNodePos to happen.
        if (xvel < 1 && xvel > 0)
            xvel = 1;
        if (yvel < 1 && yvel > 0)
            yvel = 1;

        // Move source node to new position according to overall velocity
        newPos = QPointF((qreal)(*v1)->x() + xvel, (qreal)(*v1)->y() + yvel);

        qDebug() << " source vertex v1 " << (*v1)->number()
                 << " current pos: (" << (*v1)->x()
                 << " , " << (*v1)->y()
                 << " Possible new pos (" << newPos.x()
                 << " , " << newPos.y();

        // check if new pos is out of usable screen and adjust
        newPos.rx() = canvasVisibleX(newPos.x());
        newPos.ry() = canvasVisibleY(newPos.y());

        qDebug() << "  Final new pos (" << newPos.x() << ","
                 << newPos.y() << ")";
        (*v1)->setX(newPos.x());
        (*v1)->setY(newPos.y());

    }
}

/**
 * @brief Moves all vertices to their new positions as computed by the Fruchterman-Reingold model.
 *
 * Called once per iteration from layoutForceDirectedFruchtermanReingold(). Limits each
 * vertex's displacement to the current temperature value, which decreases with each
 * iteration to act as a simulated annealing cooling schedule, then clamps the result
 * to the visible canvas area.
 *
 * Node positions are updated in-memory only (setX/setY). The caller is responsible
 * for emitting setNodePos signals in a single bulk pass after the iteration loop
 * completes — this avoids N×maxIterations signal emissions to the graphics scene.
 *
 * @param temperature Current annealing temperature, controlling the maximum displacement
 *                    per iteration. Computed by layoutForceDirected_FR_temperature().
 */
void Graph::layoutForceDirected_FR_moveNodes(const qreal &temperature)
{

    qDebug() << "\n\n *****  layoutForceDirected_FR_moveNodes() \n\n ";
    qDebug() << " temperature " << temperature;
    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    VList::const_iterator v1;

    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        // compute the new position
        // limit the maximum displacement to a maximum temperature
        xvel = sign((*v1)->disp().rx()) * qMin(qAbs((*v1)->disp().rx()), temperature);
        yvel = sign((*v1)->disp().ry()) * qMin(qAbs((*v1)->disp().ry()), temperature);
        newPos = QPointF((*v1)->x() + xvel, (*v1)->y() + yvel);
        qDebug() << " source vertex v1 " << (*v1)->number()
                 << " current pos: (" << (*v1)->x() << "," << (*v1)->y() << ")"
                 << "Possible new pos (" << newPos.x() << ","
                 << newPos.y() << ")";

        newPos.rx() = canvasVisibleX(newPos.x());
        newPos.ry() = canvasVisibleY(newPos.y());

        // Move node to new position
        qDebug() << " final new pos "
                 << newPos.x() << ","
                 << newPos.y() << ")";
        (*v1)->setX(newPos.x());
        (*v1)->setY(newPos.y());

    }
}
