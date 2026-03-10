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
 * @param maxIterations  Maximum number of iterations to run. The loop may exit
 *   earlier if the layout converges (max displacement falls below epsilon).
 *
 *  The Spring Embedder model (Eades, 1984), part of the Force Directed Placement (FDP) family,
 *  assigns forces to all vertices and edges: non-adjacent node pairs repel each other via an
 *  inverse-square law (analogous to Coulomb's law), while adjacent pairs attract via a
 *  logarithmic spring — deliberately not Hooke's law, which Eades argued causes clumping
 *  at large distances.
 *  These forces are applied to the nodes iteratively, pulling them closer together or pushing
 *  them further apart, until the system comes to an equilibrium state (node positions do not
 *  change anymore).
 *
 *  Implementation notes:
 *  - Adjacency is pre-cached into a QSet<QPair<int,int>> before the iteration loop to avoid
 *    O(log N) edgeExists() calls inside the O(N²) inner loop.
 *  - Repulsion and attraction math is inlined directly (no QString model dispatch per call).
 *  - Vertex numbers are cached to local ints before the inner loop.
 *  - A linear cooling schedule decreases c4 from c4_init to c4_min over maxIterations,
 *    damping displacement progressively so the layout settles rather than oscillating.
 *  - Early exit when max displacement in an iteration falls below epsilon (convergence).
 */
void Graph::layoutForceDirectedSpringEmbedder(const int maxIterations)
{
    // --- Step size (c4 in Eades' notation) ---
    // Eades used a fixed c4=0.1. We keep a mild linear cooling schedule
    // (0.1 → 0.01) to help the layout settle without oscillating, while
    // staying close to the original spirit. The displacement cap in
    // layoutForceDirected_Eades_moveNodes() prevents early-iteration explosions.
    const qreal c4_init = 0.10;
    const qreal c4_min  = 0.01;

    // --- Force constants ---
    // Faithful to Eades (1984): c1=2, c3=1 in his notation.
    // c_spring (c1): logarithmic spring stiffness for adjacent pairs.
    // c_rep (c3): inverse-square repulsion strength for all pairs.
    const qreal c_spring = 2.0;
    const qreal c_rep    = 1.0;

    // --- Convergence threshold ---
    // Exit early when the largest single-node displacement in an iteration
    // falls below epsilon (canvas pixels). Avoids running all maxIterations
    // when the layout has already settled.
    // Must be large enough that the layout runs for sufficient iterations
    // before converging — 0.5 was too tight and caused exit after 1-2 iterations.
    const qreal epsilon = 2.0;

    int iteration = 1;
    int progressCounter = 0;
    qreal dist = 0;
    qreal f_rep = 0, f_att = 0;
    QPointF DV;
    VList::const_iterator v1;
    VList::const_iterator v2;

    qreal V = (qreal)vertices();
    // Natural length (c2 in Eades' notation): the ideal edge length in canvas pixels.
    // Eades worked in a normalised unit square with c2=1. Translating to pixel
    // coordinates: c2 = canvasMinDimension / sqrt(V), which gives the pixel-space
    // equivalent of one "unit" in his formulation. For N=100 on a 799px canvas
    // this yields ~80px — well within the range where log(d/c2) is meaningfully
    // attractive for connected pairs (d > c2) and repulsive for too-close pairs.
    qreal naturalLength = canvasMinDimension() / qSqrt(V);

    qDebug() << "\n\n layoutForceDirectedSpringEmbedder()"
             << "vertices" << V
             << "canvas" << canvasWidth << canvasHeight
             << "naturalLength" << naturalLength
             << "maxIterations" << maxIterations
             << "c4_init" << c4_init << "c4_min" << c4_min
             << "c_spring" << c_spring << "c_rep" << c_rep;

    // --- Pre-cache adjacency into a flat QSet for O(1) edge existence lookup.
    // This avoids N² × maxIterations calls to edgeExists() (which is O(log N))
    // inside the inner loop. We cache both directions so the undirected case
    // is handled transparently.
    QSet<QPair<int,int>> adjCache;
    adjCache.reserve(edgesEnabled() * 2);
    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        if (!(*v1)->isEnabled()) continue;
        int s = (*v1)->number();
        for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
        {
            if (!(*v2)->isEnabled()) continue;
            int t = (*v2)->number();
            if (s == t) continue;
            if (edgeExists(s, t))
                adjCache.insert(qMakePair(s, t));
        }
    }
    qDebug() << "layoutForceDirectedSpringEmbedder() - adjCache size" << adjCache.size();

    /* Apply an initial random layout */
    layoutRandomInMemory();

    QString pMsg = tr("Embedding Eades Spring-Gravitational model. \n"
                      "Please wait ....");
    progressStatus(pMsg);
    progressCreate(maxIterations, pMsg);

    for (iteration = 1; iteration <= maxIterations; iteration++)
    {
        // Linear cooling: c4 decreases from c4_init at iteration 1
        // to c4_min at iteration maxIterations.
        const qreal c4 = c4_init - (c4_init - c4_min)
                         * (qreal(iteration - 1) / qreal(maxIterations - 1));

        // Reset displacement vectors for all vertices
        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            (*v1)->disp().rx() = 0;
            (*v1)->disp().ry() = 0;
        }

        for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
        {
            if (!(*v1)->isEnabled())
                continue;

            // Cache source vertex number once — avoids repeated iterator
            // dereferences inside the O(N) inner loop.
            const int s = (*v1)->number();

            // qDebug() << "********* Calculate forces for source s" << s
            //          << " pos" << (*v1)->x() << "," << (*v1)->y();

            for (v2 = m_graph.cbegin(); v2 != m_graph.cend(); ++v2)
            {
                if (!(*v2)->isEnabled())
                    continue;
                if (v2 == v1)
                    continue;

                // Cache target vertex number once per inner iteration.
                const int t = (*v2)->number();

                DV.setX((*v2)->x() - (*v1)->x());
                DV.setY((*v2)->y() - (*v1)->y());
                dist = graphDistanceEuclidean(DV);

                // --- Repulsive force (Eades: inverse-square, all pairs) ---
                // Inlined from layoutForceDirected_F_rep("Eades",...).
                // No distance cutoff: every pair repels, regardless of distance.
                // This fixes the clustering bug caused by the old
                // "f_rep = 0 when dist > 2*optimalDistance" cutoff.
                if (dist > 0)
                    f_rep = -(c_rep / (dist * dist));
                else
                    f_rep = -naturalLength; // coincident vertices: push apart

                (*v1)->disp().rx() += sign(DV.x()) * f_rep;
                (*v1)->disp().ry() += sign(DV.y()) * f_rep;

                // qDebug() << "  s=" << s << " pushed away from t=" << t
                //          << " dist" << dist << " f_rep=" << f_rep;

                // --- Attractive (spring) force (Eades: log spring, adjacent pairs only) ---
                // Inlined from layoutForceDirected_F_att("Eades",...).
                // O(1) lookup via pre-cached QSet instead of edgeExists().
                if (adjCache.contains(qMakePair(s, t)))
                {
                    f_att = (dist > 0)
                                ? c_spring * log10(dist / naturalLength)
                                : 0.0;

                    (*v1)->disp().rx() += sign(DV.x()) * f_att;
                    (*v1)->disp().ry() += sign(DV.y()) * f_att;
                    (*v2)->disp().rx() -= sign(DV.x()) * f_att;
                    (*v2)->disp().ry() -= sign(DV.y()) * f_att;

                    // qDebug() << "  s=" << s << " attracted by t=" << t
                    //          << " dist" << dist << " f_att=" << f_att
                    //          << " disp_s=(" << (*v1)->disp().rx() << "," << (*v1)->disp().ry() << ")"
                    //          << " disp_t=(" << (*v2)->disp().rx() << "," << (*v2)->disp().ry() << ")";
                }
            } // end for v2

            // qDebug() << "  >>> final s=" << s
            //          << " disp=(" << (*v1)->disp().rx() << "," << (*v1)->disp().ry() << ")";
        } // end for v1

        // Apply displacements with current cooling factor and track max displacement
        // for convergence detection.
        const qreal maxDisp = layoutForceDirected_Eades_moveNodes(c4);

        progressUpdate(++progressCounter);

        if (progressCanceled())
        {
            progressFinish();
            setModStatus(ModStatus::VertexPositions);
            return;
        }

        qDebug() << "  iteration" << iteration << " c4=" << c4 << " maxDisp=" << maxDisp;

        // Early exit: layout has converged when no node moved more than epsilon pixels.
        if (maxDisp < epsilon)
        {
            qDebug() << "layoutForceDirectedSpringEmbedder() - converged at iteration"
                     << iteration << " maxDisp=" << maxDisp;
            break;
        }

    } // end iterations

    // Emit all final node positions in one bulk pass now that the iteration loop
    // is complete — avoids N×maxIterations signal emissions to the graphics scene.
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
 * @brief Computes the optimal inter-vertex distance for force-directed layouts.
 *
 * The optimal distance (also called k or naturalLength) represents the radius
 * of the empty area ideally surrounding each vertex. It is derived from the
 * available canvas area per vertex, plus the vertex's own diameter.
 *
 * Formula:
 *   vertexArea = ceil( sqrt( canvasArea / V ) )   — side of a square tile per vertex
 *   optimalDistance = vertexDiameter + vertexArea
 *
 * This gives the distance at which repulsive and attractive forces balance for
 * a uniformly distributed graph. For denser graphs or larger N, vertexArea
 * shrinks (fewer pixels per node), so the returned value decreases — callers
 * may apply their own scaling multiplier on top (e.g. FR uses C=0.9).
 *
 * @param V  Number of active vertices in the graph.
 * @return   Optimal inter-vertex spacing in canvas pixels.
 */
qreal Graph::computeOptimalDistance(const int &V)
{
    // Diameter of a vertex in canvas pixels
    qreal vertexWidth = (qreal)2.0 * initVertexSize;
    qreal screenArea = canvasHeight * canvasWidth;
    // Side length of the square canvas tile allocated to each vertex
    qreal vertexArea = qCeil(qSqrt(screenArea / V));
    // Optimal distance = vertex diameter + per-vertex tile side
    return (vertexWidth + vertexArea);
}

/**
 * @brief Computes the attractive (spring) force between two adjacent vertices.
 *
 * For the Eades model: logarithmic spring force scaled by c_spring = 2.
 *   Note: log10(dist / optimalDistance) is negative when dist < optimalDistance,
 *   meaning the spring pushes nodes apart when they are closer than the natural
 *   length — this is intentional per Eades (1984), who uses a log spring that
 *   both attracts far-apart neighbours and repels too-close ones.
 *
 * For the FR model: quadratic attraction as per Fruchterman & Reingold (1991).
 *
 * @param model      Layout model identifier: "Eades" or "FR".
 * @param dist       Euclidean distance between the two adjacent vertices (canvas pixels).
 * @param optimalDistance  The natural/optimal inter-vertex spacing for the current graph.
 * @return Attractive force magnitude (positive = pull toward each other).
 */
qreal Graph::layoutForceDirected_F_att(const QString model, const qreal &dist,
                                       const qreal &optimalDistance)
{
    qreal f_att;
    if (model == "Eades")
    {
        // NOTE: This branch is retained for reference only.
        // The Eades path in layoutForceDirectedSpringEmbedder() inlines
        // the attraction force directly and does NOT call this function.
        qreal c_spring = 2;
        f_att = c_spring * log10(dist / optimalDistance);
    }
    else
    { // model == "FR"
        f_att = (dist * dist) / optimalDistance;
    }
    return f_att;
}

/**
 * @brief Computes the repulsive force between two vertices for force-directed layouts.
 *
 * For the Eades model: uses an inverse-square law with constant c_rep.
 *   - The previous cutoff (f_rep = 0 when dist > 2 * optimalDistance) has been removed.
 *     That cutoff was the primary cause of node clustering: distant nodes received zero
 *     repulsion and were never pushed apart. All vertex pairs now repel each other,
 *     regardless of distance.
 *   - c_rep = c3 = 1.0 per Eades (1984). The inlined Eades path in
 *     layoutForceDirectedSpringEmbedder() uses the same value directly.
 *
 * For the FR model: uses the grid-variant with a 2*optimalDistance cutoff radius,
 * as described by Fruchterman & Reingold (1991).
 *
 * @param model      Layout model identifier: "Eades" or "FR".
 * @param dist       Euclidean distance between the two vertices (canvas pixels).
 * @param optimalDistance  The natural/optimal inter-vertex spacing for the current graph.
 * @return Repulsive force magnitude (negative, i.e. pushing apart).
 */
qreal Graph::layoutForceDirected_F_rep(const QString model, const qreal &dist,
                                       const qreal &optimalDistance)
{
    qreal f_rep;
    if (model == "Eades")
    {
        if (dist > 0)
        {
            // c_rep = c3 = 1.0 per Eades (1984).
            // No distance cutoff: every pair repels regardless of distance,
            // fixing the clustering bug from the original implementation.
            qreal c_rep = 1.0;
            f_rep = c_rep / (dist * dist);
        }
        else
        {
            f_rep = 0; // coincident vertices handled by inlined Eades path
        }
    }
    else
    { // model == "FR"
        // Grid-variant: neglect repulsion outside 2*optimalDistance radius
        // to approximate the Barnes-Hut speedup described by FR (1991).
        if ((2.0 * optimalDistance) < dist)
        {
            f_rep = 0;
        }
        else
        {
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
 * Displacement clamping: each component (x, y) is capped at
 * ±(canvasMinDimension * 0.05) before being applied. This prevents the
 * "iteration-2 explosion" caused by near-coincident node pairs at the start
 * of the simulation — without the cap, f_rep = c_rep/dist² diverges for
 * dist→0 and a single node pair can produce a 100px+ spike that locks the
 * layout into a bad configuration for all remaining iterations.
 *
 * Node positions are updated in-memory only (setX/setY). The caller is responsible
 * for emitting setNodePos signals in a single bulk pass after the iteration loop
 * completes — this avoids N×maxIterations signal emissions to the graphics scene.
 *
 * @param c4  Displacement normalization factor for this iteration. Supplied by the
 *            caller's linear cooling schedule (decreases from c4_init to c4_min over
 *            maxIterations) to progressively damp movement and allow the layout to settle.
 * @return    The largest Euclidean displacement applied to any single vertex this
 *            iteration (canvas pixels). Used by the caller for convergence detection.
 */
qreal Graph::layoutForceDirected_Eades_moveNodes(const qreal &c4)
{
    qDebug() << "\n ***** layoutForceDirected_Eades_moveNodes() c4=" << c4;

    // Maximum displacement allowed per node per iteration, per component (x or y).
    // Caps the effect of near-coincident node pairs where f_rep = c_rep/dist²
    // diverges as dist→0, preventing a single large spike from locking the
    // layout into a bad configuration in the first few iterations.
    // Scales with canvas size so it works across different display resolutions.
    const qreal maxAllowedDisp = canvasMinDimension() * 0.05;

    QPointF newPos;
    qreal xvel = 0, yvel = 0;
    qreal maxDisp = 0;
    VList::const_iterator v1;

    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); ++v1)
    {
        // Scale displacement by the current cooling factor
        xvel = c4 * (*v1)->disp().rx();
        yvel = c4 * (*v1)->disp().ry();

        // Clamp each component to ±maxAllowedDisp to prevent explosion
        // from near-coincident node pairs at simulation start
        xvel = qBound(-maxAllowedDisp, xvel, maxAllowedDisp);
        yvel = qBound(-maxAllowedDisp, yvel, maxAllowedDisp);

        qDebug() << " ##### vertex" << (*v1)->number()
                 << " xvel,yvel=(" << xvel << "," << yvel << ")"
                 << " maxAllowedDisp=" << maxAllowedDisp;

        // Fix Qt sub-pixel rounding: a fractional positive displacement < 1
        // gets floored to 0 by integer QPoint conversion; bump it to 1.
        if (xvel < 1 && xvel > 0) xvel = 1;
        if (yvel < 1 && yvel > 0) yvel = 1;

        // Track largest displacement for convergence detection in the caller
        qreal disp = qSqrt(xvel * xvel + yvel * yvel);
        if (disp > maxDisp)
            maxDisp = disp;

        newPos = QPointF((*v1)->x() + xvel, (*v1)->y() + yvel);

        qDebug() << " vertex" << (*v1)->number()
                 << " current pos:(" << (*v1)->x() << "," << (*v1)->y() << ")"
                 << " possible new pos:(" << newPos.x() << "," << newPos.y() << ")";

        // Clamp to visible canvas area
        newPos.rx() = canvasVisibleX(newPos.x());
        newPos.ry() = canvasVisibleY(newPos.y());

        qDebug() << "  final new pos:(" << newPos.x() << "," << newPos.y() << ")";

        (*v1)->setX(newPos.x());
        (*v1)->setY(newPos.y());
    }
    return maxDisp;
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
