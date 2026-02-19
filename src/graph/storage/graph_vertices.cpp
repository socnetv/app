/**
 * @file graph_vertices.cpp
 * @brief Implements vertex storage and CRUD operations for the Graph class
 *        (create/remove/find/existence, iterators, vertex positions, isolation).
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
// Vertex CRUD operations
//

/**
 * @brief Creates a new vertex
 *
 * Main vertex creation slot, associated with homonymous signal from Parser.
 * Adds a vertex to the Graph and signals drawNode to GW
 * The new vertex has number num and specific color, label, label color, shape and position p.
 *
 * @param num
 * @param size
 * @param nodeColor
 * @param numColor
 * @param numSize
 * @param label
 * @param lColor
 * @param lSize
 * @param p
 * @param nodeShape
 * @param signalMW
 */
void Graph::vertexCreate(const int &number,
                         const int &size,
                         const QString &color,
                         const QString &numColor,
                         const int &numSize,
                         const QString &label,
                         const QString &labelColor,
                         const int &labelSize,
                         const QPointF &p,
                         const QString &shape,
                         const QString &iconPath,
                         const bool &signalMW,
                         const QHash<QString, QString> &customAttributes)
{

    qDebug() << "Creating a new vertex:" << number
             << "shape:" << shape
             << "icon:" << iconPath
             << "signalMW:" << signalMW
             << "- Appending the new vertex and signaling to GW to create the node";

    if (order)
        vpos[number] = m_totalVertices;
    else
        vpos[number] = m_graph.size();

    m_graph.append(
        new GraphVertex(
            this,
            number,
            m_curRelation,
            size,
            color,
            numColor,
            numSize,
            label,
            labelColor,
            labelSize,
            p,
            shape,
            iconPath,
            m_reserveEdgesPerVertexSize,
            customAttributes));

    m_totalVertices++;

    emit signalDrawNode(p,
                        number,
                        size,
                        shape,
                        iconPath,
                        color,
                        numColor,
                        numSize,
                        initVertexNumberDistance,
                        label,
                        labelColor,
                        labelSize,
                        initVertexLabelDistance);

    qDebug() << "Finished creating new vertex:" << number << "Setting graph mod status";

    setModStatus(ModStatus::VertexCount, signalMW);

    // to draw new vertices by user with the same style of the file loaded:
    // save color, size and shape as init values
    initVertexColor = color;
    initVertexSize = size;
    initVertexShape = shape;
    if (shape == "custom")
    {
        initVertexIconPath = iconPath;
    }
}

/**
 * @brief Creates a new vertex in the given position
 *
 * Called from GW, with i and p as parameters.
 * Calls the main creation slot with init node values.
 *
 * @param QPointF  The clicked pos of the new node.
 */
void Graph::vertexCreateAtPos(const QPointF &p)
{
    int i = vertexNumberMax() + 1;

    qDebug() << "Creating a new vertex:" << i << " in given position:" << p;

    vertexCreate(i, initVertexSize, initVertexColor,
                 initVertexNumberColor, initVertexNumberSize,
                 QString(), initVertexLabelColor, initVertexLabelSize,
                 p, initVertexShape, initVertexIconPath,
                 true);

    emit statusMessage(tr("New node (numbered %1) added at position (%2,%3). Double-click on it to start a new edge from it.")
                           .arg(vertexNumberMax())
                           .arg(p.x())
                           .arg(p.y()));
}

/**
 * @brief Creates a new randomly positioned vertex with default values
 *
 * Computes a random position p inside the useable canvas area
 * Then calls the main creation slot with init node values.
 *
 * @param bool
 *
 */
void Graph::vertexCreateAtPosRandom(const bool &signalMW)
{

    QPointF p;
    p.setX(canvasRandomX());
    p.setY(canvasRandomY());
    qDebug() << "Creating a new random positioned vertex at:" << p;
    vertexCreate(vertexNumberMax() + 1, initVertexSize, initVertexColor,
                 initVertexNumberColor, initVertexNumberSize,
                 QString(), initVertexLabelColor, initVertexLabelSize,
                 p, initVertexShape, initVertexIconPath, signalMW);
}

/**
 * @brief Creates a new randomly positioned vertex with specific number and label.
 * All other values are from the defaults.
 *
 * Called from WebCrawler and Parser with parameters label and i.
 * Computes a random position p the useable canvas area
 * Then calls the main creation slot with init node values.
 *
 * @param i
 * @param label
 * @param signalMW
 */
void Graph::vertexCreateAtPosRandomWithLabel(const int &i,
                                             const QString &label,
                                             const bool &signalMW)
{

    qDebug() << "Creates a new randomly positioned vertex:" << i
             << "with label:" << label;
    QPointF p;
    p.setX(canvasRandomX());
    p.setY(canvasRandomY());
    vertexCreate((i < 0) ? vertexNumberMax() + 1 : i, initVertexSize, initVertexColor,
                 initVertexNumberColor, initVertexNumberSize,
                 label, initVertexLabelColor, initVertexLabelSize,
                 p, initVertexShape, initVertexIconPath, signalMW);
}

/**
 * @brief Removes the vertex v1 from the graph
 * First, it removes all edges to doomed from other vertices
 * Then it changes the vpos of all subsequent vertices inside m_graph
 * Finally, it removes the vertex.
 * @param int v1
 */
void Graph::vertexRemove(const int &v1)
{
    qDebug() << "Removing vertex:"
             << m_graph[vpos[v1]]->number()
             << "vpos:" << vpos[v1]
             << "Removing all inbound and outbound edges ";
    int doomedPos = vpos[v1];

    // Remove links to v1 from each other vertex
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (qAbs((*it)->hasEdgeTo(v1)) > 0)
        {
            qDebug() << "another vertex" << (*it)->number()
                     << " has outbound Edge to " << v1 << ". Removing it.";
            (*it)->removeOutEdge(v1);
        }
        if (qAbs((*it)->hasEdgeFrom(v1)) > 0)
        {
            qDebug() << "another vertex" << (*it)->number()
                     << " has inbound Edge from " << v1 << ". Removing it.";
            (*it)->removeInEdge(v1);
        }
    }

    qDebug() << "Finished with vertices. "
                "Update the vpos which maps vertices inside m_graph ";
    int prevIndex = doomedPos;

    qDebug() << "Updating vpos of all subsequent vertices ";
    H_Int::const_iterator it1 = vpos.cbegin();
    while (it1 != vpos.cend())
    {
        if (it1.value() > doomedPos)
        {
            prevIndex = it1.value();
            qDebug() << "vertex" << it1.key()
                     << "had prevIndex:" << prevIndex
                     << " > doomedPos" << doomedPos
                     << "Setting new vpos. vpos size was: " << vpos.size();
            vpos.insert(it1.key(), --prevIndex);
            qDebug() << "vertex" << it1.key()
                     << "new vpos:" << vpos.value(it1.key(), -666)
                     << "vpos size now:" << vpos.size();
        }
        else
        {
            qDebug() << "vertex" << it1.key() << "with vpos"
                     << it1.value() << " =< doomedPos. CONTINUE";
        }
        ++it1;
    }

    // Now remove vertex Doomed from m_graph
    qDebug() << "graph vertices=size=" << vertices() << "="
             << m_graph.size() << "removing vertex at vpos " << doomedPos;
    m_graph.removeAt(doomedPos);
    m_totalVertices--;
    qDebug() << "Now graph vertices=size=" << vertices() << "="
             << m_graph.size();

    order = false;

    // Check if this was the clicked vertex and unset it
    if (vertexClicked() == v1)
    {
        vertexClickedSet(0, QPointF(0, 0));
    }

    setModStatus(ModStatus::VertexCount);

    emit signalRemoveNode(v1);
}

/**
 * @brief Deletes a dummy node
 *
 * This is called from Parser (as pajek) to delete any redundant (dummy) nodes.
 *
 * @param int i number of node
 */
void Graph::vertexRemoveDummyNode(int i)
{
    qDebug() << "Removing dummy node from graph: " << i;
    vertexRemove(i);
}

//
// Vertex access and retrieval
//

/**
 * @brief Returns the index of a vertex by its number
 *
 * Returns the vpos or -1
 *
 * Complexity: O(logN) for vpos retrieval
 *
 * @param vertex number
 * @return vertex pos or -1
 */
int Graph::vertexIndexByNumber(int v) const
{
    return vpos.value(v, -1);
}

/**
 * @brief Returns the vertex at a given index
 * @param idx
 * @return GraphVertex*
 */
GraphVertex *Graph::vertexAtIndex(int idx)
{
    return m_graph[idx];
}

/**
 * @brief Returns the vertex at a given index
 * @param idx
 * @return GraphVertex*
 */
const GraphVertex *Graph::vertexAtIndex(int idx) const
{
    return m_graph[idx];
}

//
// Vertex iterators and positions
//

/**
 * @brief iterator helpers
 */
VList::const_iterator Graph::verticesBegin() const { return m_graph.cbegin(); }

VList::const_iterator Graph::verticesEnd() const { return m_graph.cend(); }

/**
 * @brief Returns the number of the last vertex in the graph.
 *
 * @return  int
 */
int Graph::vertexNumberMax()
{
    if (m_totalVertices > 0)
        return m_graph.back()->number();
    else
        return 0;
}

/**
 * @brief Returns the number of the first vertex in the graph.
 *
 * @return int
 */
int Graph::vertexNumberMin()
{
    if (m_totalVertices > 0)
        return m_graph.front()->number();
    else
        return 0;
}

//
// Vertex existence and find operations
//

/**
 * @brief Returns true if the current graph has no vertices at all
 */
bool Graph::isEmpty() const
{
    return m_graph.isEmpty();
}

/**
 * @brief Checks if the given vertex exists in the graph.
 *
 * Returns the vpos or -1
 *
 * Complexity:  O(logN) for vpos retrieval
 *
 * @param vertex number
 * @return vertex pos or -1
 */
int Graph::vertexExists(const int &v1)
{
    //    qDebug () << "Checking if vertex exists, with number:" << v1;
    if (vpos.contains(v1))
    {
        if (m_graph[vpos[v1]]->number() == v1)
        {
            return vpos[v1];
        }
        else
        {
            qCritical() << "Error in vpos for vertex number v:" << v1;
        }
    }

    return -1;
}

/**
 * @brief Checks if there is a vertex with a specific label exists in the graph
 *
 * Returns the vpos or -1
 *
 * Complexity:  O(N)
 *
 * @param label
 * @return vpos or -1
 */
int Graph::vertexExists(const QString &label)
{
    qDebug() << "Checking if vertex exists, with label:" << label.toUtf8();
    VList::const_iterator it;
    int i = 0;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if ((*it)->label().contains(label, Qt::CaseInsensitive))
        {
            return i;
        }
        i++;
    }
    return -1;
}

/**
 * @brief Finds vertices in strList by their number
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByNumber(const QStringList &numList)
{
    qDebug() << "Finding vertices by number - searchList:" << numList;
    QString vStr;
    QList<int> foundList;
    QStringList notFound;
    int v = -1;
    bool intOk = false;
    bool searchResult = false;
    for (int i = 0; i < numList.size(); ++i)
    {
        vStr = numList.at(i);
        v = vStr.toInt(&intOk);
        if (intOk)
        {
            if (vertexExists(v) != -1)
            {
                qDebug() << "vertex number" << v
                         << "exists. Adding it to found list";
                foundList << v;
            }
            else
            {
                qDebug() << "vertex number" << v
                         << "does not exist. Adding it to notFound list";
                notFound << vStr;
            }
        }
        else
        {
            qDebug() << "cannot read" << vStr;
        }
    }

    if (!foundList.isEmpty())
    {
        searchResult = true;
        qDebug() << "One or more matching nodes found. Signaling to GW to highlight them...";
        emit statusMessage(tr("Found %1 matching nodes.").arg(foundList.size()));
        emit signalNodesFound(foundList);
    }
    else
    {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage(tr("Could not find any nodes matching your choices."));
    }

    return searchResult;
}

/**
 * @brief Finds vertices by their label
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByLabel(const QStringList &labelList)
{
    qDebug() << "Finding vertices by label - searchList:" << labelList;

    QString vLabel;
    QList<int> foundList;
    int vFoundPos = -1;
    QStringList notFound;
    bool searchResult = false;
    for (int i = 0; i < labelList.size(); ++i)
    {
        vLabel = labelList.at(i);

        if ((vFoundPos = vertexExists(vLabel)) != -1)
        {
            qDebug() << "vertex with label" << vLabel
                     << "exists. Adding it to found list";
            foundList << m_graph[vFoundPos]->number();
        }
        else
        {
            qDebug() << "vertex with label" << vLabel
                     << "does not exist. Adding it to notFound list ";
            notFound << vLabel;
        }
    }

    if (!foundList.isEmpty())
    {
        searchResult = true;
        qDebug() << "One or more matchin nodes found. Signaling to GW to highlight them...";
        emit statusMessage(tr("Found %1 matching nodes.").arg(foundList.size()));
        emit signalNodesFound(foundList);
    }
    else
    {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage(tr("Could not find any nodes matching your choices."));
    }

    return searchResult;
}

/**
 * @brief Finds vertices by their index score
 * @param QStringList
 * @return
 */
bool Graph::vertexFindByIndexScore(const int &index, const QStringList &thresholds,
                                   const bool &considerWeights, const bool &inverseWeights, const bool &dropIsolates)
{

    qDebug() << "Finding vertices by index" << index
             << "threshold list" << thresholds
             << "considerWeights" << considerWeights
             << "inverseWeights" << inverseWeights
             << "dropIsolates" << dropIsolates;

    QList<int> foundList;

    bool searchResult = false;

    VList::const_iterator it;

    QString thresholdStr = "";

    bool gtThan = false;
    bool gtEqual = false;
    bool lsThan = false;
    bool lsEqual = false;
    bool convertedOk = false;
    qreal threshold = 0;
    qreal score = 0;

    switch (index)
    {
    case 0:
    {
        // do nothing
        break;
    }
    case IndexType::DC:
    {
        centralityDegree(considerWeights, dropIsolates);
        break;
    }
    case IndexType::IRCC:
    {
        centralityClosenessIR();
        break;
    }
    case IndexType::IC:
    {
        centralityInformation(considerWeights, inverseWeights);
        break;
    }
    case IndexType::EVC:
    {
        centralityEigenvector(considerWeights, inverseWeights, dropIsolates);
        break;
    }
    case IndexType::DP:
    {
        prestigeDegree(considerWeights, dropIsolates);
        break;
    }
    case IndexType::PRP:
    {
        prestigePageRank(dropIsolates);
        break;
    }
    case IndexType::PP:
    {
        prestigeProximity(considerWeights, inverseWeights);
        break;
    }
    default:
        graphDistancesGeodesic(true, considerWeights,
                               inverseWeights, dropIsolates);
        break;
    }

    // Parse threshold user input
    for (int i = 0; i < thresholds.size(); ++i)
    {

        thresholdStr = thresholds.at(i);
        thresholdStr = thresholdStr.simplified();

        gtThan = false;
        gtEqual = false;
        lsThan = false;
        lsEqual = false;

        convertedOk = false;

        if (thresholdStr.startsWith(">=") || thresholdStr.startsWith("=>"))
        {
            gtEqual = true;
            thresholdStr.remove(">=");
            thresholdStr.remove("=>");
            qDebug() << "thresholdStr starts with >=";
        }
        else if (thresholdStr.startsWith(">"))
        {
            gtThan = true;
            thresholdStr.remove(">");
            qDebug() << "thresholdStr starts with > ";
        }
        else if (thresholdStr.startsWith("<=") || thresholdStr.startsWith("=<"))
        {
            lsEqual = true;
            thresholdStr.remove("<=");
            thresholdStr.remove("=<");
            qDebug() << "thresholdStr starts with <=";
        }
        else if (thresholdStr.startsWith("<"))
        {
            lsThan = true;
            thresholdStr.remove("<");
            qDebug() << "thresholdStr starts with < ";
        }
        else
        {
            qDebug() << "thresholdStr does not start with > or <";
            continue;
        }

        // Parse score threshold
        threshold = thresholdStr.toDouble(&convertedOk);

        if (!convertedOk)
        {
            qDebug() << "cannot convert thresholdStr to float";
            continue;
        }
        else
        {
            qDebug() << "threshold" << threshold;
        }

        // Iterate over all vertices and get their scores
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {

            switch (index)
            {
            case 0:
            {
                score = 0;
                break;
            }
            case IndexType::DC:
            {
                score = (*it)->SDC();
                break;
            }
            case IndexType::CC:
            {
                score = (*it)->SCC();
                break;
            }
            case IndexType::IRCC:
            {
                score = (*it)->SIRCC();
                break;
            }
            case IndexType::BC:
            {
                score = (*it)->SBC();
                break;
            }
            case IndexType::SC:
            {
                score = (*it)->SSC();
                break;
            }
            case IndexType::EC:
            {
                score = (*it)->SEC();
                break;
            }
            case IndexType::PC:
            {
                score = (*it)->SPC();
                break;
            }
            case IndexType::IC:
            {
                score = (*it)->SIC();
                break;
            }
            case IndexType::EVC:
            {
                score = (*it)->SEVC();
                break;
            }
            case IndexType::DP:
            {
                score = (*it)->SDP();
                break;
            }
            case IndexType::PRP:
            {
                score = (*it)->SPRP();
                break;
            }
            case IndexType::PP:
            {
                score = (*it)->SPP();
                break;
            }
            }

            if (gtThan)
            {
                if (score > threshold)
                {
                    qDebug() << "matching vertex" << (*it)->number() << "score" << score;
                    foundList << (*it)->number();
                }
            }
            else if (gtEqual)
            {
                if (score >= threshold)
                {
                    qDebug() << "matching vertex" << (*it)->number() << "score" << score;
                    foundList << (*it)->number();
                }
            }
            else if (lsThan)
            {
                if (score < threshold)
                {
                    qDebug() << "matching vertex" << (*it)->number() << "score" << score;
                    foundList << (*it)->number();
                }
            }
            else if (lsEqual)
            {
                if (score <= threshold)
                {
                    qDebug() << "matching vertex" << (*it)->number() << "score" << score;
                    foundList << (*it)->number();
                }
            }
        }
    }

    if (!foundList.isEmpty())
    {
        searchResult = true;
        qDebug() << "One or more matching nodes found. Signaling to GW to highlight them...";
        emit statusMessage(tr("Found %1 matching nodes.").arg(foundList.size()));
        emit signalNodesFound(foundList);
    }
    else
    {
        qDebug() << "No matching nodes found. Return.";
        emit statusMessage(tr("Could not find any nodes matching your choices."));
    }

    return searchResult;
}

//
// Vertex isolation
//
/**
 * @brief Toggles the status of all isolated vertices (thos without links)
 *
 * For each isolate vertex in the Graph, emits the setVertexVisibility signal
 *
 * @param toggle
 */
void Graph::vertexIsolatedAllToggle(const bool &toggle)
{
    qDebug() << "Setting all isolated vertices to" << toggle;

    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isIsolated())
        {
            continue;
        }
        else
        {
            qDebug() << "vertex" << (*it)->number()
                     << "is isolated. Toggling it and emitting setVertexVisibility signal to GW...";
            (*it)->setEnabled(toggle);

            setModStatus(ModStatus::VertexCount);

            emit setVertexVisibility((*it)->number(), toggle);
        }
    }
}

/**
 * @brief Checks if vertex is isolated
 * @param v1
 * @return
 */
bool Graph::vertexIsolated(const int &v1) const
{
    if (m_graph[vpos[v1]]->isIsolated())
    {
        qDebug() << "vertex:" << v1 << "is isolated";
        return true;
    }
    qDebug() << "vertex:" << v1 << "not isolated";
    return false;
}

//
// Vertex positions
//
/**
 * @brief Changes the position of the given vertex
 *
 * Called from MW/GW when node moves to update its position
 *
 * @param v1
 * @param x
 * @param y
 */
void Graph::vertexPosSet(const int &v1, const int &x, const int &y)
{

    m_graph[vpos[v1]]->setX(x);
    m_graph[vpos[v1]]->setY(y);
    setModStatus(ModStatus::VertexPositions, false);
}

/**
 * @brief Returns the position of the given vertex
 * @param v1
 * @return
 */
QPointF Graph::vertexPos(const int &v1) const
{
    return m_graph[vpos[v1]]->pos();
}

GraphVertex *Graph::vertexPtr(const int v)
{
    if (!vpos.contains(v))
        return nullptr;

    const int idx = vpos.value(v);
    if (idx < 0 || idx >= m_graph.size())
        return nullptr;

    return m_graph.at(idx);
}
