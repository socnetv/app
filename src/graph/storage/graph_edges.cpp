/**
 * @file graph_edges.cpp
 * @brief Implements edge storage and CRUD operations for the Graph class
 *        (create/add/remove, enable/disable, existence queries, weights).
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

//
// Create/add/remove
//

/**
 * @brief Checks a) if edge exists and b) if the reverse edge exists
 * Calls edgeAdd to add the new edge to the Graph,
 * then emits drawEdge() which calls GW::drawEdge() to draw the new edge.
 * Called from homonymous signal of Parser class.
 * Also called from MW when user clicks on the "add link" button
 * Also called (via MW) from GW when user middle-clicks on two nodes.
 * @param v1
 * @param v2
 * @param weight
 * @param color
 * @param reciprocal
 * @param drawArrows
 * @param bezier
 */
bool Graph::edgeCreate(const int &v1,
                       const int &v2,
                       const qreal &weight,
                       const QString &color,
                       const int &type,
                       const bool &drawArrows,
                       const bool &bezier,
                       const QString &label,
                       const bool &signalMW)
{

    // check whether there is already such an edge
    // (see #713617 - https://bugs.launchpad.net/socnetv/+bug/713617)

    if (edgeExists(v1, v2))
    {
        //        qDebug() << "-- Edge " << v1 << "->" << v2
        //                    << " declared previously (already exists) - nothing to do \n\n";

        return false;
    }

    if (type == EdgeType::Undirected)
    {

        //        qDebug() <<"-- Creating new UNDIRECTED edge:" << v1 << "-" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Signaling to GW...";

        edgeAdd(v1, v2,
                weight,
                type,
                label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label, ((weight == 0) ? "blue" : color), type,
                            drawArrows, bezier, initEdgeWeightNumbers);
    }
    else if (edgeExists(v2, v1))
    {

        //        qDebug() <<"-- Creating new RECIPROCAL edge:" << v1 << "->" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Reverse edge exists"
        //                 << "Signaling to GW...";

        edgeAdd(v1,
                v2,
                weight,
                EdgeType::Reciprocated,
                label,
                color);

        emit signalDrawEdge(v1, v2, weight, label, color, EdgeType::Reciprocated,
                            drawArrows, bezier, initEdgeWeightNumbers);
        m_graphIsDirected = true;
    }
    else
    {

        //        qDebug() <<"-- Creating new DIRECTED edge:" << v1 << "->" << v2
        //                 << "weight" << weight
        //                 << "type" << type
        //                 << "label" << label
        //                 << "signalMW" << signalMW
        //                 << "Signaling to GW...";

        edgeAdd(v1,
                v2,
                weight,
                EdgeType::Directed,
                label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label, ((weight == 0) ? "blue" : color), EdgeType::Directed,
                            drawArrows, bezier, initEdgeWeightNumbers);

        m_graphIsDirected = true;
        m_graphIsSymmetric = false;
    }

    // save the edge color so that new edges created when user clicks on the canvas
    // have the same color with those of the file loaded,
    initEdgeColor = color;

    setModStatus(ModStatus::EdgeCount, signalMW);

    return true;
}

/**
 * @brief Called from WebCrawler when it finds an new link
 * Calls edgeCreate() method with initEdgeColor
 * @param source
 * @param target
 */
void Graph::edgeCreateWebCrawler(const int &source, const int &target)
{
    //    qDebug()<< " will create edge from" << source << "to" << target ;
    qreal weight = 1.0;
    bool drawArrows = true;
    bool bezier = false;

    edgeCreate(source, target, weight, initEdgeColor, EdgeType::Directed, drawArrows, bezier);
}

/**
 * @brief Adds a directed edge from v1 to v2
 * If type == EdgeType::Undirected then it also adds the directed edge v2->v1
 * @param v1
 * @param v2
 * @param weight
 * @param label
 * @param color
 * @param type
 */
void Graph::edgeAdd(const int &v1, const int &v2,
                    const qreal &weight,
                    const int &type,
                    const QString &label,
                    const QString &color)
{

    int source = vpos[v1];
    int target = vpos[v2];

    //    qDebug()<< "Adding new edge from vertex "<< v1 << "["<< source
    //            << "] to vertex "<< v2 << "["<< target << "] of weight "<<weight
    //            << " and label " << label;

    m_graph[source]->addOutEdge(v2, weight, color, label);
    m_graph[target]->addInEdge(v1, weight);

    if (weight != 1 && weight != 0)
    {
        setWeighted(true);
    }
    if (type == EdgeType::Reciprocated)
    {
        // make existing reverse edge reciprocal
    }
    else if (type == EdgeType::Undirected)
    {
        // edge undirected, add reverse edge too.
        m_graph[target]->addOutEdge(v1, weight);
        m_graph[source]->addInEdge(v2, weight);
    }
}

/**
 * @brief Toggles the status of outbound edge source -> target at source vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeOutboundStatusSet(const int &source, const int &target, const bool &toggle)
{

    m_graph[vpos[source]]->setOutEdgeEnabled(target, toggle);
}

/**
 * @brief Toggles the status of inbound edge target <- source at target vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeInboundStatusSet(const int &target, const int &source, const bool &toggle)
{

    m_graph[vpos[target]]->setInEdgeEnabled(source, toggle);
}

/**
 * @brief Removes the directed arc v1->v2 or, if the graph is undirected, the edge v1 <->v2
 *
 * Emits signal to GW to delete the graphics item.
 *
 * @param v1
 * @param v2
 * @param removeReverse if true also removes the reverse edge
 */
void Graph::edgeRemove(const int &v1,
                       const int &v2,
                       const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemove() - edge" << v1 << "[" << vpos[v1]
             << "] -->" << v2 << " to be removed. removeReverse:" << removeReverse;
    m_graph[vpos[v1]]->removeOutEdge(v2);
    m_graph[vpos[v2]]->removeInEdge(v1);

    if (isUndirected() || removeReverse)
    { // remove reverse edge too
        m_graph[vpos[v2]]->removeOutEdge(v1);
        m_graph[vpos[v1]]->removeInEdge(v2);
        m_graphIsSymmetric = true;
    }
    else
    {
        if (edgeExists(v2, v1) != 0)
        {
            m_graphIsSymmetric = false;
        }
    }

    emit signalRemoveEdge(v1, v2, (isDirected() || removeReverse));

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Removes a SelectedEdge
 * @param selectedEdge
 * @param removeReverse
 */
void Graph::edgeRemoveSelected(SelectedEdge &selectedEdge,
                               const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemoveSelected()" << selectedEdge;
    edgeRemove(selectedEdge.first, selectedEdge.second, removeReverse);
}

/**
 * @brief Removes all selected edges
 */
void Graph::edgeRemoveSelectedAll()
{
    qDebug() << "Graph::edgeRemoveSelectedAll()";

    foreach (SelectedEdge edgeToRemove, getSelectedEdges())
    {
        qDebug() << "Graph::edgeRemoveSelectedAll() - About to remove" << edgeToRemove;
        edgeRemoveSelected(edgeToRemove, true);
    }
}

//
// Existence / symmetry / counts
//

/**
 * @brief Checks if there is an edge from v1 to v2 and returns the weight, if the edge exists.
 *
   Complexity:  O(logN) for vpos retrieval + O(1) for QList index retrieval + O(logN) for checking edge(v2)

 * @param v1
 * @param v2
 * @param reciprocated: if true, checks if the edge is reciprocated (v1<->v2) with the same weight
 * @return zero if edge or reciprocated edge does not exist or non-zero if arc /reciprocated edge exists
 */
qreal Graph::edgeExists(const int &v1, const int &v2, const bool &checkReciprocal)
{

    edgeWeightTemp = m_graph[vpos[v1]]->hasEdgeTo(v2);
    //    qDebug() << "Checking if edge exists:" << v1 << "->" << v2 << "=" << edgeWeightTemp  ;

    if (!checkReciprocal)
    {
        return edgeWeightTemp;
    }
    else if (edgeWeightTemp != 0)
    {
        edgeReverseWeightTemp = m_graph[vpos[v2]]->hasEdgeTo(v1);
        //        qDebug() << "Checking if reverse edge exists: " << v2 << "->" << v1 << "=" << edgeWeightTemp  ;
        if (edgeWeightTemp == edgeReverseWeightTemp)
        {
            return edgeWeightTemp;
        }
    }
    return 0;
}

/**
 * @brief Checks if there is an edge from v1 to v2, even weight = 0 and returns the weight, if the edge exists
 * or RAND_MAX if the edge does not exist at all.
 *
 * This is only used in GraphML saving if the user has selected the Settings option to save zero-weight edges
 *
 * @see https://github.com/socnetv/app/issues/151
 *
 * @param v1
 * @param v2
 */
qreal Graph::edgeExistsVirtual(const int &v1, const int &v2)
{

    qreal m_weight = RAND_MAX;
    bool edgeStatus = false;
    H_edges::const_iterator it1;
    GraphVertex *source = m_graph[vpos[v1]];
    H_edges source_outEdges = source->m_outEdges;

    it1 = source_outEdges.constFind(v2);
    while (it1 != source_outEdges.constEnd() && it1.key() == v2)
    {
        if (it1.value().first == m_curRelation)
        {
            edgeStatus = it1.value().second.second;
            if (edgeStatus == true)
            {
                m_weight = it1.value().second.first;
            }
        }
        ++it1;
    }

    return m_weight;
}

/**
 * @brief Returns TRUE if edge(v1, v2) is symmetric, i.e. (v1,v2) == (v2,v1).
 * @param v1
 * @param v2
 * @return
 */
bool Graph::edgeSymmetric(const int &v1, const int &v2)
{
    if ((edgeExists(v1, v2, true)) != 0)
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is symmetric";
        return true;
    }
    else
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is not symmetric";
        return false;
    }
}

/**
 * @brief Returns the number |E| of graph - only the enabled edges
 *
 * @return
 */
int Graph::edgesEnabled()
{

    int enabledEdges = 0;
    if (calculatedEdges)
    {
        enabledEdges = ((isUndirected()) ? m_totalEdges / 2 : m_totalEdges);
        //        qDebug()<< "Graph unchanged. Returning current enabled edges count:" <<  enabledEdges;
        return enabledEdges;
    }
    // Compute the edge count from scratch
    m_totalEdges = 0;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        m_totalEdges += (*it)->outEdgesCount();
    }
    calculatedEdges = true;
    enabledEdges = ((isUndirected()) ? m_totalEdges / 2 : m_totalEdges);
    //    qDebug()<< "Computed enabled edges new count:" <<  enabledEdges;
    return enabledEdges;
}

/**
 * @brief Returns the number of outbound edges (arcs) from vertex v1
 * @param v1
 * @return
 */
int Graph::vertexEdgesOutbound(int v1)
{
    qDebug("Graph: vertexEdgesOutbound()");
    return m_graph[vpos[v1]]->outEdgesCount();
}

/**
 * @brief Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::vertexEdgesInbound(int v1)
{
    qDebug("Graph: vertexEdgesInbound()");
    return m_graph[vpos[v1]]->inEdgesCount();
}

//
// Weight core
//

/**
 * @brief Changes the weight of the edge from vertex v1 to v2 (and optionally of the reverse edge)
 *
 * @param v1
 * @param v2
 * @param weight
 * @param undirected
 */
void Graph::edgeWeightSet(const int &v1, const int &v2,
                          const qreal &weight, const bool &undirected)
{
    qDebug() << "Changing the weight of edge" << v1 << "[" << vpos[v1]
             << "]->" << v2 << "[" << vpos[v2] << "]" << " to new weight " << weight;
    m_graph[vpos[v1]]->setOutEdgeWeight(v2, weight);
    if (undirected)
    {
        qDebug() << "Changing the weight of the reverse edge too";
        m_graph[vpos[v2]]->setOutEdgeWeight(v1, weight);
    }
    emit setEdgeWeight(v1, v2, weight);
    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Returns the weight of the edge v1->v2
 * @param v1
 * @param v2
 * @return qreal
 */
qreal Graph::edgeWeight(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->hasEdgeTo(v2);
}

/**
 * @brief Changes the direction type of an existing edge
 *
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeTypeSet(const int &v1,
                        const int &v2,
                        const qreal &weight,
                        const int &dirType)
{

    qDebug() << "Changing the direction type of edge: " << v1
             << "->" << v2 << "new edgeType:" << dirType;

    if (dirType != EdgeType::Directed)
    {

        // check if reverse edge exists
        qreal revEdgeWeight = edgeExists(v2, v1);

        if (revEdgeWeight == 0)
        {
            // Reverse edge does not exist, add it
            qDebug() << "reverse  edge" << v1 << " <- " << v2 << " does not exist - Adding it...";
            // Note: Even if dirType=EdgeType::Undirected we add the opposite edge as EdgeType::Reciprocated
            edgeAdd(v2, v1, weight, EdgeType::Reciprocated, "", initEdgeColor);
        }
        else
        {
            // Reverse edge does exist
            if (dirType == EdgeType::Undirected)
            {
                // Make the edge weights equal
                // TOFIX: how do we decide which of the two weights to keep?
                qDebug() << "Graph::edgeTypeSet(): opposite  " << v1
                         << " <- " << v2 << " exists - equaling weights.";
                if (weight != revEdgeWeight)
                {
                    edgeWeightSet(v2, v1, weight);
                }
            }
            else
            {
                // if dirType is EdgeType::Reciprocated we don't need  to equalize weights
            }
        }
        emit signalEdgeType(v1, v2, dirType);
    }
}
