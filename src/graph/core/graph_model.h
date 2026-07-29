/**
 * @file graph_model.h
 * @brief Declares GraphModel, a read-only structural view over a Graph's vertex list.
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

#ifndef GRAPH_MODEL_H
#define GRAPH_MODEL_H

class Graph;

/**
 * @brief Read-only, QtCore-only view over a Graph's vertex list and adjacency (WS3 M2).
 *
 * An adapter, not a data migration: GraphModel owns no storage of its own and copies
 * nothing. It's built entirely on Graph's existing public accessors (vertexIndexByNumber(),
 * vertexAtIndex(), vertices()) - no new friend access, same complexity as the direct
 * m_graph[vpos[v]] pattern used internally by Graph's own methods. The goal is a narrow,
 * headlessly-testable read surface that algorithm slices can eventually depend on instead
 * of the full Graph façade.
 */
class GraphModel
{
public:
    explicit GraphModel(Graph &graph);

    /** @brief Number of vertices currently in the graph. */
    int vertexCount() const;

    /** @brief Out-degree (sum of outbound edge weights) of the vertex numbered vertexNumber. */
    int degreeOut(int vertexNumber) const;

    /** @brief In-degree (sum of inbound edge weights) of the vertex numbered vertexNumber. */
    int degreeIn(int vertexNumber) const;

private:
    Graph &m_graph;
};

#endif
