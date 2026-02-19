/**
 * @file graph_canvas.cpp
 * @brief Implements canvas geometry and coordinate helper methods for the Graph
 *        class (canvas size, visible coordinates, random coordinates).
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

/**
 * @brief Sets the size of the canvas
 *
 * Called when the MW is resized to update canvasWidth/canvasHeight, and node positions
 *
 * @param w
 * @param h
 */
void Graph::canvasSizeSet(const int &width, const int &height)
{

    qreal fX = (static_cast<qreal>(width)) / canvasWidth;
    qreal fY = (static_cast<qreal>(height)) / canvasHeight;
    qreal newX, newY;

    qDebug() << "Canvas was resized: " << width << "x" << height
             << "Adjusting node positions, if any. Please wait...";
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        newX = (*it)->x() * fX;
        newY = (*it)->y() * fY;
        (*it)->setX(newX);
        (*it)->setY(newY);
        emit setNodePos((*it)->number(), newX, newY);
    }
    canvasWidth = width;
    canvasHeight = height;
    //    progressStatus(tr("Canvas size: (%1, %2)px")
    //                       .arg(QString::number(canvasWidth))
    //                       .arg(QString::number(canvasHeight))
    //                       );

    setModStatus(ModStatus::VertexPositions, false);
    qDebug() << "Finished resizing.";
}

/**
 * @brief Gets the max radius of the canvas
 * @return double
 */
double Graph::canvasMaxRadius() const
{
    return (canvasHeight < canvasWidth) ? canvasHeight / 2.0 - 30 : canvasWidth / 2.0 - 30;
}

/**
 * @brief Gets the min dimensions of the canvas
 * @return qreal
 */
qreal Graph::canvasMinDimension() const
{
    return (canvasHeight < canvasWidth) ? canvasHeight - 30 : canvasWidth - 30;
}

/**
 * @brief Checks if x is visible inside the canvas usable area and if not returns an adjusted x-coordinate
 * @param x
 * @return double
 */
double Graph::canvasVisibleX(const double &x) const
{
    return qMin(canvasWidth - 50.0, qMax(50.0, x));
}

/**
 * @brief Checks if y is visible inside the canvas usable area and if not returns an adjusted y-coordinate
 * @param y
 * @return double
 */
double Graph::canvasVisibleY(const double &y) const
{
    return qMin(canvasHeight - 50.0, qMax(50.0, y));
}

/**
 * @brief Returns a random x-coordinate adjusted to be visible inside the canvas usable area
 * @return double
 */
double Graph::canvasRandomX() const
{
    qreal randX = static_cast<qreal>(rand() % static_cast<int>(canvasWidth));
    return qMin(canvasWidth - 30.0, qMax(30.0, randX));
}

/**
 * @brief Returns a random y-coordinate adjusted to be visible inside the canvas usable area
 * @return double
 */
double Graph::canvasRandomY() const
{
    qreal randY = static_cast<qreal>(rand() % static_cast<int>(canvasHeight));
    return qMin(canvasHeight - 30.0, qMax(30.0, randY));
}
