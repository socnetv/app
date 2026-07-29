/**
 * @file graph_model.cpp
 * @brief Implements GraphModel, a read-only structural view over a Graph's vertex list.
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

#include "graph_model.h"
#include "graph.h"

GraphModel::GraphModel(Graph &graph) : m_graph(graph)
{
}

int GraphModel::vertexCount() const
{
    return m_graph.vertices();
}

int GraphModel::degreeOut(int vertexNumber) const
{
    const int idx = m_graph.vertexIndexByNumber(vertexNumber);
    if (idx < 0)
        return 0;
    return m_graph.vertexAtIndex(idx)->degreeOut();
}

int GraphModel::degreeIn(int vertexNumber) const
{
    const int idx = m_graph.vertexIndexByNumber(vertexNumber);
    if (idx < 0)
        return 0;
    return m_graph.vertexAtIndex(idx)->degreeIn();
}
