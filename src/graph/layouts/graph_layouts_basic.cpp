/**
 * @file graph_layouts_basic.cpp
 * @brief Implements basic graph layout algorithms of the Graph class (e.g., circular, random, grid and related positioning strategies).
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
#include <QColor>
#include <QtMath>
#include <cmath>

/**
 * @brief Repositions all nodes on random positions
 * Emits setNodePos(i, x,y) to tell GW that the node item should be moved.
 * @param maxWidth
 * @param maxHeight
 */
void Graph::layoutRandom()
{
    qDebug() << "Graph::layoutRandom() ";
    double new_x = 0, new_y = 0;
    VList::const_iterator it;

    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Embedding Random Layout. \n"
                      "Please wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        progressUpdate(++progressCounter);
        new_x = canvasRandomX();
        new_y = canvasRandomY();
        (*it)->setX(new_x);
        (*it)->setY(new_y);
        qDebug() << "Graph::layoutRandom() - "
                 << " vertex " << (*it)->number()
                 << " emitting setNodePos to new pos " << new_x << " , " << new_y;
        emit setNodePos((*it)->number(), new_x, new_y);
    }

    progressFinish();

    setModStatus(ModStatus::VertexPositions);
}

/**
 * @brief Repositions all nodes on the periphery of
 * different circles with random radius
 * @param x0
 * @param y0
 * @param maxRadius
 */
void Graph::layoutRadialRandom(const bool &guides)
{
    qDebug() << "Graph::layoutRadialRandom - ";
    double rad = 0, new_radius = 0, new_x = 0, new_y = 0;
    double i = 0;
    double x0 = canvasWidth / 2.0;
    double y0 = canvasHeight / 2.0;

    double maxRadius = canvasMaxRadius();
    // offset controls how far from the centre the central nodes be positioned
    qreal offset = 0.06, randomDecimal = 0;
    int vert = vertices();
    int progressCounter = 0;
    VList::const_iterator it;

    int N = vertices();

    QString pMsg = tr("Embedding Random Radial layout. \n"
                      "Please wait ....");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        randomDecimal = (qreal)(rand() % 100) / 100.0;
        new_radius = (maxRadius - (randomDecimal - offset) * maxRadius);

        qDebug() << "Vertice " << (*it)->number()
                 << " at x=" << (*it)->x()
                 << ", y= " << (*it)->y()
                 << ", maxradius " << maxRadius
                 << " randomDecimal " << randomDecimal
                 << " new radius " << new_radius;

        // Calculate new position
        rad = (2.0 * M_PI / vert);
        new_x = x0 + new_radius * cos(i * rad);
        new_y = y0 + new_radius * sin(i * rad);
        (*it)->setX(new_x);
        (*it)->setY(new_y);
        qDebug("Vertice will move to x=%f and y=%f ", new_x, new_y);
        // Move node to new position
        emit setNodePos((*it)->number(), new_x, new_y);
        i++;
        if (guides)
        {
            emit addGuideCircle(x0, y0, new_radius);
        }
    }

    progressFinish();
    setModStatus(ModStatus::VertexPositions);
}

/**
 * @brief Repositions all nodes on the periphery of a circle with given radius
 * @param x0
 * @param y0
 * @param maxRadius
 */
void Graph::layoutCircular(const double &x0, const double &y0,
                           const double &newRadius, const bool &guides)
{
    qDebug() << "Graph::layoutCircular - ";
    double rad = 0, new_x = 0, new_y = 0;
    double i = 0;
    VList::const_iterator it;
    int N = vertices();
    int progressCounter = 0;

    QString pMsg = tr("Applying circular layout. \nPlease wait...");
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        progressUpdate(++progressCounter);

        if (!(*it)->isEnabled())
        {
            qDebug() << "  vertex i" << (*it)->number() << " disabled. Continue";
            continue;
        }

        // Calculate new position
        rad = (2.0 * M_PI / N);
        new_x = x0 + newRadius * cos(i * rad);
        new_y = y0 + newRadius * sin(i * rad);
        (*it)->setX(new_x);
        (*it)->setY(new_y);
        qDebug("Vertice will move to x=%f and y=%f ", new_x, new_y);
        // Move node to new position
        emit setNodePos((*it)->number(), new_x, new_y);
        i++;

        if (guides)
        {
            emit addGuideCircle(x0, y0, newRadius);
        }
    }

    progressFinish();

    setModStatus(ModStatus::VertexPositions);
}

/**
 * @brief Convenience method
 * Changes the size of all nodes to be proportional to their outDegree (Degree Centrality)
 * Calls layoutByProminenceIndex
 */
void Graph::layoutVertexSizeByOutdegree()
{

    layoutByProminenceIndex(1, 2);
}

/**
 * @brief Convenience method
 * Changes the size of all nodes to be proportional to their InDegree (Degree Prestige)
 * Calls layoutByProminenceIndex
 */
void Graph::layoutVertexSizeByIndegree()
{

    layoutByProminenceIndex(10, 2);
}

/**
 * @brief Applies a layout according to each actor's prominence index score.
 * The layout type can be radial (0), level (1), node sizes (2) or node colors (3), as follows:
 * layoutType=0 - Repositions all nodes on the periphery of concentric circles with radius analogous to their prominence index
 * layoutType=1 - Repositions all nodes on different top-down levels according to their centrality
 * layoutType=2 - Changes node sizes to be proportional to their prominence index score
 * layoutType=2 - Changes node colors to reflect their prominence index score (from red to green)
 * @param prominenceIndex, 0-12
 * @param layoutType: 0,1,2,3
 * @param considerWeights
 * @param inverseWeights
 * @param dropIsolates
 */
void Graph::layoutByProminenceIndex(int prominenceIndex, int layoutType,
                                    const bool &considerWeights,
                                    const bool &inverseWeights,
                                    const bool &dropIsolates)
{
    qDebug() << "Applying layout by prominence index:"
             << prominenceIndex
             << "type:" << layoutType;

    double i = 0, std = 0, norm = 0;
    double new_x = 0, new_y = 0;
    qreal C = 0, maxC = 0;
    double x0 = 0, y0 = 0, maxRadius = 0, new_radius = 0, rad = 0;
    double maxWidth = 0, maxHeight = 0;
    qreal offset = 0;
    int new_size = 0;
    int progressCounter = 0;

    int N = vertices();
    VList::const_iterator it;

    QColor new_color;

    switch (layoutType)
    {
    case 0:
    { // radial
        x0 = canvasWidth / 2.0;
        y0 = canvasHeight / 2.0;
        offset = 0.06;
        maxRadius = canvasMaxRadius();
        break;
    }
    case 1:
    { // level
        offset = 50;
        maxHeight = canvasHeight - offset;
        maxWidth = canvasWidth - offset;
        break;
    }
    };

    progressStatus(tr("Computing centrality/prestige scores. Please wait..."));

    // first compute centrality indices if needed

    if (prominenceIndex == 0)
    {
        // do nothing
    }
    else if (prominenceIndex == IndexType::DC)
    {
        centralityDegree(considerWeights, dropIsolates);
    }
    else if (prominenceIndex == IndexType::IRCC)
    {
        centralityClosenessIR(considerWeights, inverseWeights, dropIsolates);
    }
    else if (prominenceIndex == IndexType::IC)
    {
        centralityInformation(considerWeights, inverseWeights);
    }
    else if (prominenceIndex == IndexType::EVC)
    {
        centralityEigenvector(considerWeights, dropIsolates);
    }
    else if (prominenceIndex == IndexType::DP)
    {
        prestigeDegree(considerWeights, dropIsolates);
    }
    else if (prominenceIndex == IndexType::PRP)
    {
        prestigePageRank(dropIsolates);
    }
    else if (prominenceIndex == IndexType::PP)
    {
        prestigeProximity(considerWeights, inverseWeights, dropIsolates);
    }
    else
    {
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);
    }

    QString pMsg;
    switch (layoutType)
    {
    case 0:
    { // radial
        pMsg = tr("Embedding Radial layout by Prominence Score. \nPlease wait...");
        break;
    }
    case 1:
    { // level
        pMsg = tr("Embedding Level layout by Prominence Score. \nPlease wait...");
        break;
    }
    case 2:
    { // node size
        pMsg = tr("Embedding Node Size by Prominence Score layout. \nPlease wait...");
        break;
    }
    case 3:
    { // node color
        pMsg = tr("Embedding Node Color by Prominence Score layout. \nPlease wait...");
        break;
    }
    }
    progressStatus(pMsg);
    progressCreate(N, pMsg);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        switch (prominenceIndex)
        {
        case 0:
        {
            C = 0;
            maxC = 0;
            break;
        }
        case IndexType::DC:
        {
            C = (*it)->SDC();
            std = (*it)->SDC();
            maxC = maxSDC;
            break;
        }
        case IndexType::CC:
        {
            C = (*it)->CC();
            std = (*it)->SCC();
            maxC = maxSCC;
            break;
        }
        case IndexType::IRCC:
        {
            C = (*it)->IRCC();
            std = (*it)->SIRCC();
            maxC = maxIRCC;
            break;
        }
        case IndexType::BC:
        {
            C = (*it)->BC();
            std = (*it)->SBC();
            maxC = maxSBC;
            break;
        }
        case IndexType::SC:
        {
            C = (*it)->SC();
            std = (*it)->SSC();
            maxC = maxSSC;
            break;
        }
        case IndexType::EC:
        {
            C = (*it)->EC();
            std = (*it)->SEC();
            maxC = maxEC;
            break;
        }
        case IndexType::PC:
        {
            C = (*it)->PC();
            std = (*it)->SPC();
            maxC = maxSPC;
            break;
        }
        case IndexType::IC:
        {
            C = (*it)->IC();
            std = (*it)->SIC();
            maxC = maxIC;
            break;
        }
        case IndexType::EVC:
        {
            C = (*it)->EVC();
            std = (*it)->SEVC();
            maxC = 1;
            break;
        }
        case IndexType::DP:
        {
            C = (*it)->SDP();
            std = (*it)->SDP();
            maxC = maxSDP;
            break;
        }
        case IndexType::PRP:
        {
            C = (*it)->PRP();
            std = (*it)->SPRP();
            maxC = 1;
            break;
        }
        case IndexType::PP:
        {
            C = (*it)->PP();
            std = (*it)->SPP();
            maxC = maxPP;
            break;
        }
        };

        norm = std / maxC;

        progressUpdate(++progressCounter);

        switch (layoutType)
        {

        case 0:
        { // radial

            qDebug() << "vertex" << (*it)->number()
                     << "pos x" << (*it)->x() << "y" << (*it)->y()
                     << "C" << C << "stdC" << std << "maxC" << maxC
                     << "norm (std/maxC)" << norm
                     << "maxradius" << maxRadius
                     << "newradius" << maxRadius - (norm - offset) * maxRadius;

            // Compute new vertex position
            switch (static_cast<int>(ceil(maxC)))
            {

            case 0:
            {
                qDebug("maxC=0.   Using maxRadius");
                new_radius = maxRadius;
                break;
            }

            default:
            {
                new_radius = (maxRadius - (norm - offset) * maxRadius);
                break;
            }
            };

            rad = (2.0 * M_PI / N);
            new_x = x0 + new_radius * cos(i * rad);
            new_y = y0 + new_radius * sin(i * rad);

            qDebug() << "Finished calculation. "
                        "new radial pos: x"
                     << new_x << "y" << new_y;

            // Move vertex to new position
            (*it)->setX(new_x);
            (*it)->setY(new_y);

            emit setNodePos((*it)->number(), new_x, new_y);

            i++;

            emit addGuideCircle(x0, y0, new_radius);

            break;
        }

        case 1:
        { // level

            qDebug() << "vertex" << (*it)->number()
                     << "pos x" << (*it)->x() << "y" << (*it)->y()
                     << "C" << C << "stdC" << std << "maxC " << maxC
                     << "norm (std/maxC)" << norm
                     << "maxWidth " << maxWidth
                     << " maxHeight " << maxHeight
                     << "maxHeight-(norm)*maxHeight "
                     << maxHeight - (norm)*maxHeight;

            // Compute new vertex position
            switch (static_cast<int>(ceil(maxC)))
            {

            case 0:
            {
                qDebug("maxC=0.   Using maxHeight");
                new_y = maxHeight;
                break;
            }

            default:
            {
                new_y = offset / 2.0 + maxHeight - (norm)*maxHeight;
                break;
            }
            };

            new_x = offset / 2.0 + rand() % (static_cast<int>(maxWidth));

            qDebug() << "Finished calculation. "
                        "new level pos: x"
                     << new_x << "y" << new_y;

            // Move vertex to new position
            (*it)->setX(new_x);
            (*it)->setY(new_y);

            emit setNodePos((*it)->number(), new_x, new_y);

            i++;

            emit addGuideHLine(new_y);

            break;
        }

        case 2:
        { // node size

            qDebug() << "vertex" << (*it)->number()
                     << "C=" << C << ", stdC=" << std << "maxC" << maxC
                     << "initVertexSize " << initVertexSize
                     << "norm (stdC/maxC) " << norm
                     << ", (norm) * initVertexSize " << (norm * initVertexSize);

            // Calculate new node size
            switch (static_cast<int>(ceil(maxC)))
            {

            case 0:
            {
                qDebug() << "maxC=0.   Using initVertexSize";
                new_size = initVertexSize;
                break;
            }

            default:
            {
                new_size = ceil(initVertexSize / 2.0 + (qreal)initVertexSize * (norm));
                break;
            }
            };

            // set new vertex size and emit signal to change node size
            qDebug() << "Finished calculation. "
                     << "new vertex size " << new_size << " call setSize()";
            (*it)->setSize(new_size);
            emit setNodeSize((*it)->number(), new_size);

            break;
        }

        case 3:
        { // node color

            qDebug() << "vertex" << (*it)->number()
                     << "C=" << C << ", stdC=" << std << "maxC" << maxC
                     << "initVertexColor " << initVertexColor
                     << "norm (stdC/maxC) " << norm;

            // Calculate new node color
            switch (static_cast<int>(ceil(maxC)))
            {
            case 0:
            {
                qDebug() << "maxC=0.   Using initVertexColor";
                new_color = QColor(initVertexColor);
                break;
            }
            default:
            {
                // H = 0 (red) for most important nodes
                // H = 240 (blue) for least important nodes
                new_color.setHsv(240 - norm * 240, 255, 255, 255);
                break;
            }
            };

            // change vertex color and emit signal to change node color as well
            qDebug() << "new vertex color " << new_color << " call setSize()";
            (*it)->setColor(new_color.name());

            emit setNodeColor((*it)->number(), new_color.name());

            break;
        }
        };
    }

    progressFinish();

    setModStatus(ModStatus::VertexPositions);

    prominenceDistribution(prominenceIndex, m_reportsChartType);
}
