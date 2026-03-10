/**
 * @file graph_structure_metrics.cpp
 * @brief Implements structural queries and basic metrics for the Graph class
 *        (degrees, neighborhoods, density, reciprocity, dichotomization).
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
 * @brief Returns the outDegree (sum of outbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeOut(int v1)
{
    qDebug() << "Returning outDegree of " << v1;
    return m_graph[vpos[v1]]->degreeOut();
}

/**
 * @brief Returns the inDegree (sum of inbound edge weights) of vertex v1
 * @param v1
 * @return
 */
int Graph::vertexDegreeIn(int v1)
{
    qDebug() << "Returning inDegree of " << v1;
    return m_graph[vpos[v1]]->degreeIn();
}

/**
 * @brief Returns a list of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QList<int> Graph::vertexNeighborhoodList(const int &v1)
{
    // qDebug() << "Returning the neighborhood list of " << v1;
    return m_graph[vpos[v1]]->neighborhoodList();
}

/**
 * @brief Returns the set of all vertices mutually connected to vertex v1 in the
 * current relation
 * @param v1
 * @return  QList<int>
 */
QSet<int> Graph::vertexNeighborhoodSet(const int &v1)
{
    // qDebug()<< "Graph::vertexNeighborhoodList()";
    QList<int> myNeightbors = m_graph[vpos[v1]]->neighborhoodList();
    return QSet<int>(myNeightbors.constBegin(), myNeightbors.constEnd());
}

/**
 * @brief Gets the graph density (if computed) or computes it again.
 *
 * The graph density is the ratio of present ties to total possible ties
 * for the current relation.
 *
 * IMPORTANT: edgesEnabled() semantics in SocNetV:
 *  - If the graph is UNDIRECTED, edgesEnabled() returns E (undirected edges),
 *    even though internally each undirected edge is stored as two symmetric arcs.
 *  - If the graph is DIRECTED, edgesEnabled() returns A (directed arcs).
 *
 * Therefore:
 *  - Undirected density: 2E / (V*(V-1))
 *  - Directed   density:  A / (V*(V-1))
 *
 * TODO / THINK: Self-loops (v->v)
 *  - The denominator V*(V-1) assumes loops are not allowed/considered.
 *  - If self-loops can exist and be enabled, decide whether to:
 *      (a) exclude loops from the numerator for density, or
 *      (b) use a loop-aware denominator (e.g., V*V for directed with loops).
 *
 * @return qreal
 */
qreal Graph::graphDensity()
{
    if (calculatedGraphDensity)
    {
        return m_graphDensity;
    }

    const int V = vertices();
    if (V != 0 && V != 1)
    {
        const int enabledEdges = edgesEnabled(); // E (undirected) or A (directed)
        m_graphDensity = (isUndirected())
                             ? (qreal)2 * enabledEdges / (qreal)(V * (V - 1.0))
                             : (qreal)enabledEdges / (qreal)(V * (V - 1.0));
    }
    else
    {
        m_graphDensity = 0;
    }

    calculatedGraphDensity = true;
    return m_graphDensity;
}

/**
 * @brief Returns the sum of vertices having edgesOutbound
 * @return
 */
int Graph::verticesWithOutboundEdges()
{
    return outboundEdgesVert;
}

/**
 * @brief Returns the sum of vertices having edgesInbound
 * @return
 */
int Graph::verticesWithInboundEdges()
{
    return inboundEdgesVert;
}

/**
 * @brief Returns the sum of vertices having reciprocal edges
 * @return
 */
int Graph::verticesWithReciprocalEdges()
{
    return reciprocalEdgesVert;
}

/**
 * @brief Gets the arc reciprocity of the graph.
 *
 * Also computes the dyad reciprocity and fills parameters with values.

 * @return qreal
 */
qreal Graph::graphReciprocity()
{

    qDebug() << "Graph::graphReciprocity()";

    if (calculatedGraphReciprocity)
    {
        qDebug() << "Graph::graphReciprocity() - graph not modified and "
                    "already calculated reciprocity. Returning previous result: "
                 << m_graphReciprocityArc;
        return m_graphReciprocityArc;
    }

    qDebug() << "Graph::graphReciprocity() - Computing...";

    progressStatus((tr("Calculating the Arc Reciprocity of the graph...")));

    m_graphReciprocityArc = 0;
    m_graphReciprocityDyad = 0;
    m_graphReciprocityTiesReciprocated = 0;
    m_graphReciprocityTiesNonSymmetric = 0;
    m_graphReciprocityTiesTotal = 0;
    m_graphReciprocityPairsReciprocated = 0;
    m_graphReciprocityPairsTotal = 0;

    qreal weight = 0, reciprocalWeight = 0;

    int y = 0, v2 = 0, v1 = 0;

    QHash<int, qreal> enabledOutEdges;

    QHash<int, qreal>::const_iterator hit;
    VList::const_iterator it;

    H_StrToBool totalDyads;
    H_StrToBool reciprocatedDyads;
    QString pair, reversePair;

    // initialize counters
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        (*it)->setOutEdgesReciprocated(0);
        (*it)->setOutEdgesNonSym(0);
        (*it)->setInEdgesNonSym(0);
    }

    // Compute "arc" reciprocity
    //  the number of ties that are involved in reciprocal relations
    //  relative to the total number of actual ties (not possible ties)
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();

        if (!(*it)->isEnabled())
            continue;

        enabledOutEdges = (*it)->outEdgesEnabledHash();

        hit = enabledOutEdges.cbegin();

        while (hit != enabledOutEdges.cend())
        {

            v2 = hit.key();
            y = vpos[v2];
            weight = hit.value();
            m_graphReciprocityTiesTotal += weight;

            // Compute "dyad" reciprocity
            pair = QString::number(v1) + ">" + QString::number(v2);
            reversePair = QString::number(v2) + ">" + QString::number(v1);
            if (!totalDyads.contains(pair) && !totalDyads.contains(reversePair))
            {
                totalDyads[pair] = true;
            }

            qDebug() << pair
                     << "totalTies" << m_graphReciprocityTiesTotal
                     << "totalDyads" << totalDyads.size();

            if ((reciprocalWeight = edgeExists(v2, v1)) == weight)
            {

                (*it)->setOutEdgesReciprocated(); // increase reciprocated ties for ego
                (*it)->setOutEdgesReciprocated();

                m_graphReciprocityTiesReciprocated += reciprocalWeight;

                pair = QString::number(v2) + ">" + QString::number(v1);
                reversePair = QString::number(v1) + ">" + QString::number(v2);
                if (!reciprocatedDyads.contains(pair) && !reciprocatedDyads.contains(reversePair))
                {
                    reciprocatedDyads[pair] = true;
                }

                qDebug() << pair << "reciprocal!"
                         << "reciprocatedTies" << m_graphReciprocityTiesReciprocated
                         << "reciprocatedDyads" << reciprocatedDyads.size();
            }
            else
            {
                (*it)->setOutEdgesNonSym();
                m_graph[y]->setInEdgesNonSym();
                m_graphReciprocityTiesNonSymmetric++;
            }

            ++hit;
        }
    }
    // delete enabledOutEdges;

    m_graphReciprocityArc = (qreal)m_graphReciprocityTiesReciprocated / (qreal)m_graphReciprocityTiesTotal;

    m_graphReciprocityPairsReciprocated = reciprocatedDyads.size();
    m_graphReciprocityPairsTotal = totalDyads.size();

    m_graphReciprocityDyad = (qreal)m_graphReciprocityPairsReciprocated / (qreal)m_graphReciprocityPairsTotal;

    qDebug() << "Graph: graphReciprocity() - Finished. Arc reciprocity:"
             << m_graphReciprocityTiesReciprocated
             << "/"
             << m_graphReciprocityTiesTotal << "=" << m_graphReciprocityArc << "\n"
             << m_graphReciprocityPairsReciprocated
             << "/"
             << m_graphReciprocityPairsTotal << "=" << m_graphReciprocityDyad;

    calculatedGraphReciprocity = true;

    return m_graphReciprocityArc;
}

/**
 * @brief Creates a new binary relation in a valued network using edge
 * dichotomization according to the threshold parameter.
 * @param threshold
 */
void Graph::graphDichotomization(const qreal threshold)
{
    qDebug() << "Graph::graphDichotomization()"
             << "initial relations" << relations();

    int v2 = 0, v1 = 0;
    qreal weight = 0;

    VList::const_iterator it;

    QHash<int, qreal> outEdgesAll;
    QHash<int, qreal>::const_iterator it1;

    QHash<QString, qreal> *binaryTies = new QHash<QString, qreal>;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        qDebug() << "Graph::graphDichotomization() - v" << v1
                 << "iterate over outEdges in all relations";
        outEdgesAll = (*it)->outEdgesEnabledHash(false);
        it1 = outEdgesAll.cbegin();
        while (it1 != outEdgesAll.cend())
        {
            v2 = it1.key();
            weight = it1.value();

            qDebug() << v1 << "->" << v2 << "=" << weight << "Checking opposite.";
            if (weight > threshold)
            {
                if (!binaryTies->contains(QString::number(v1) + "--" + QString::number(v2)))
                {
                    qDebug() << v1 << "--" << v2 << " over threshold. Adding";
                    binaryTies->insert(QString::number(v1) + "--" + QString::number(v2), 1);
                }
                else
                {
                    qDebug() << v1 << "--" << v2 << " exists. Binary Tie already found. Continue";
                }
            }
            ++it1;
        }
    }

    relationAdd("Binary-" + QString::number(threshold), true);

    QHash<QString, qreal>::const_iterator it2;
    it2 = binaryTies->constBegin();
    QStringList vertices;
    qDebug() << "creating binary tie edges";
    while (it2 != binaryTies->constEnd())
    {
        vertices = it2.key().split("--");
        qDebug() << "binary tie " << it2.key()
                 << "vertices.at(0)" << vertices.at(0)
                 << "vertices.at(1)" << vertices.at(1);
        v1 = (vertices.at(0)).toInt();
        v2 = (vertices.at(1)).toInt();
        qDebug() << "calling edgeCreate for"
                 << v1 << "--" << v2;
        edgeCreate(v1, v2, 1, initEdgeColor, EdgeType::Undirected, true, false,
                   QString(), false);
        ++it2;
    }

    // delete outEdgesAll;
    delete binaryTies;
    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
    qDebug() << "final relations" << relations();
}
