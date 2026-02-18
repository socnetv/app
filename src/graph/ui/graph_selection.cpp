/**
 * @file graph_selection.cpp
 * @brief Implements selection and clicked-state helper methods for the Graph
 *        class (selected vertices/edges, click handling, selection change hooks).
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
 * @brief Resets the clicked edge and node
 *
 * Usually, called when the user clicks on an empty space.
 *
 * @param p
 */
void Graph::graphClickedEmptySpace(const QPointF &p)
{
    qDebug() << "Click on empty space at" << p << " - resetting clicked edge and node...";
    // Reset clicked vertices
    this->vertexClickedSet(0, p);
    // Reset clicked edges
    this->edgeClickedSet(0, 0);
}

/**
 * @brief Sets the user-selected vertices and edges
 *
 * Usually called from GW, it emits selection counts to MW
 *
 * @param selectedVertices
 * @param selectedEdges
 */
void Graph::setSelectionChanged(const QList<int> selectedVertices,
                                const QList<SelectedEdge> selectedEdges)
{

    m_verticesSelected = selectedVertices;
    m_selectedEdges = selectedEdges;

    qDebug() << "Selection changed. Vertices" << m_verticesSelected << "Edges" << m_selectedEdges << "Emitting to MW...";

    emit signalSelectionChanged(m_verticesSelected.size(), m_selectedEdges.size());
}

/**
 * @brief Returns a QList of user-selected vertices
 * @return
 */
QList<int> Graph::getSelectedVertices() const
{
    return m_verticesSelected;
}

/**
 * @brief Returns count of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesCount() const
{
    return m_verticesSelected.size();
}

/**
 * @brief Returns min of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesMin() const
{
    int min = RAND_MAX;
    foreach (int i, m_verticesSelected)
    {
        if (i < min)
            min = i;
    }
    return min;
}

/**
 * @brief Returns max of user-selected vertices
 * @return
 */
int Graph::getSelectedVerticesMax() const
{
    int max = 0;
    foreach (int i, m_verticesSelected)
    {
        if (i > max)
            max = i;
    }
    return max;
}

/**
 * @brief Returns a QList of user-selected edges in pair<int,int>
 * @return
 */
QList<SelectedEdge> Graph::getSelectedEdges() const
{
    return m_selectedEdges;
}

/**
 * @brief Returns the count of user-selected edges
 * @return
 */
int Graph::getSelectedEdgesCount() const
{
    return m_selectedEdges.size();
}

/**
 * @brief Sets the clicked vertex.
 *
 * Signals to MW to show node info on the status bar.
 *
 * @param v1
 * @param p
 */
void Graph::vertexClickedSet(const int &v1, const QPointF &p)
{
    qDebug() << "Setting clicked vertex: " << v1 << "click at " << p;
    m_vertexClicked = v1;
    if (v1 == 0)
    {
        emit signalNodeClickedInfo(0, p);
    }
    else
    {
        edgeClickedSet(0, 0);
        emit signalNodeClickedInfo(v1,
                                   vertexPos(v1),
                                   vertexLabel(v1),
                                   vertexDegreeIn(v1),
                                   vertexDegreeOut(v1));
    }
}

/**
 * @brief Returns the number of the clicked vertex
 * @return  int
 */
int Graph::vertexClicked() const
{
    return m_vertexClicked;
}

/**
 * @brief Sets the clicked edge
 *
 * Parameters are the source and target node of the edge.
 * It emits signal to MW, which displays a relevant  message on the status bar.
 *
 * @param v1
 * @param v2
 */
void Graph::edgeClickedSet(const int &v1, const int &v2, const bool &openMenu)
{

    m_clickedEdge.source = v1;
    m_clickedEdge.target = v2;

    if (m_clickedEdge.source == 0 && m_clickedEdge.target == 0)
    {
        emit signalEdgeClicked();
        return;
    }
    qreal weight = m_graph[vpos[m_clickedEdge.source]]->hasEdgeTo(m_clickedEdge.target);
    qDebug() << "Setting clicked edge: " << v1 << "->" << v2 << "weight:" << weight;

    int type = EdgeType::Directed;
    // Check if the reverse tie exists. If yes, this is a reciprocated edge
    qreal oppositeWeight = edgeExists(m_clickedEdge.target, m_clickedEdge.source, false);
    if (oppositeWeight)
    {
        qDebug() << "Reverse tie" << v2 << "->" << v2 << "exists. Weight:" << oppositeWeight;
        if (!isDirected())
        {
            type = EdgeType::Undirected;
        }
        else
        {
            type = EdgeType::Reciprocated;
        }
    }
    m_clickedEdge.type = type;
    m_clickedEdge.weight = weight;
    m_clickedEdge.rWeight = oppositeWeight;

    emit signalEdgeClicked(m_clickedEdge, openMenu);
}

/**
 * @brief Returns clicked edge
 * @return
 */
MyEdge Graph::edgeClicked()
{
    return m_clickedEdge;
}

