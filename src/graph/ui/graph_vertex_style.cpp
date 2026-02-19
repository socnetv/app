/**
 * @file graph_vertex_style.cpp
 * @brief Implements vertex appearance and labeling methods for the Graph class
 *        (size, shape/icons, colors, labels, number styling, custom attributes).
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
// Vertex size
// 

/**
 * @brief Sets the initial vertex size
 *
 * @param size
 */
void Graph::vertexSizeInit(const int size)
{
    initVertexSize = size;
}

/**
 * @brief Changes the size of a vertex v or all vertices if v=0
 *
 * Called from MW (i.e. user changing node properties)
 *
 * @param v
 * @param size
 */
void Graph::vertexSizeSet(const int &v, const int &size)
{
    if (v)
    {
        qDebug() << "Changing size of vertex" << v << "new size" << size;
        m_graph[vpos[v]]->setSize(size);
        emit setNodeSize(v, size);
    }
    else
    {
        qDebug() << "Changing size of all vertices, new size" << size;
        vertexSizeInit(size);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                (*it)->setSize(size);
                emit setNodeSize((*it)->number(), size);
            }
        }
    }

    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Returns the size of vertex v
 * @param v
 * @return int
 */
int Graph::vertexSize(const int &v) const
{
    return m_graph[vpos[v]]->size();
}

// 
// Vertex shape + icons
// 

/**
 * @brief Sets the default vertex shape and iconPath
 *
 * @param shape
 * @param iconPath
 */
void Graph::vertexShapeSetDefault(const QString shape, const QString &iconPath)
{
    initVertexShape = shape;
    initVertexIconPath = iconPath;
}

/**
 * @brief Changes the shape and iconPath of vertex v1, or all vertices if v1=-1
 * @param v1
 * @param shape
 * @param iconPath
 */
void Graph::vertexShapeSet(const int &v1, const QString &shape, const QString &iconPath)
{

    if (v1 == -1)
    {
        qDebug() << "Changing shape for all vertices, new shape:" << shape
                 << "iconPath:" << iconPath;
        vertexShapeSetDefault(shape, iconPath);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                (*it)->setShape(shape, iconPath);
                emit setNodeShape((*it)->number(), shape, iconPath);
            }
        }
    }
    else
    {
        qDebug() << "Changing shape for vertex:" << v1
                 << "new shape:" << shape
                 << "iconPath:" << iconPath;
        m_graph[vpos[v1]]->setShape(shape, iconPath);
        emit setNodeShape(v1, shape, iconPath);
    }
    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Returns the shape of this vertex
 * @param v1
 * @return
 */
QString Graph::vertexShape(const int &v1)
{
    return m_graph[vpos[v1]]->shape();
}

/**
 * @brief Returns the IconPath of vertex v1
 * @param v1
 * @return
 */
QString Graph::vertexShapeIconPath(const int &v1)
{
    return m_graph[vpos[v1]]->shapeIconPath();
}

/**
 * @brief Returns true if at least one vertex has a 'custom' shape (therefore a custom icon).
 * @return bool
 */
bool Graph::graphHasVertexCustomIcons() const
{
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;

        if ((*it)->shape() == "custom")
        {
            return true;
        }
    }
    return false;
}

// 
// Vertex Custom attributes
// 

/**
 * @brief Returns true if at least one vertex has a 'custom' attribute
 * @return bool
 */
QStringList Graph::graphHasVertexCustomAttributes() const
{
    VList::const_iterator it;
    QStringList m_customAttributesNames;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
        {
            continue;
        }
        QHashIterator<QString, QString> i((*it)->customAttributes());
        while (i.hasNext())
        {
            i.next();
            if (!m_customAttributesNames.contains(i.key()))
            {
                m_customAttributesNames.append(i.key());
            }
        }
    }
    return m_customAttributesNames;
}

/**
 * @brief Calls the customAttributes method for a specific vertex in the graph.
 *
 * This function retrieves the vertex at the position specified by the index `v1`
 * from the `vpos` map and calls its `customAttributes` method.
 *
 * @param v1 The index of the vertex whose custom attributes are to be accessed.
 */
QHash<QString, QString> Graph::vertexCustomAttributes(const int &v1) const
{
    return m_graph[vpos[v1]]->customAttributes();
}

//
// Vertex color
// 
/**
 * @brief Changes the color of vertex v1
 * @param v1
 * @param color
 */
void Graph::vertexColorSet(const int &v1, const QString &color)
{

    if (v1)
    {
        qDebug() << "Setting vertex" << v1 << "new color" << color;
        m_graph[vpos[v1]]->setColor(color);
        emit setNodeColor(m_graph[vpos[v1]]->number(), color);
    }
    else
    {
        qDebug() << "Setting new color for all vertices:" << color;
        vertexColorInit(color);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                qDebug() << "for all, setting vertex" << (*it)->number()
                         << " new color" << color;
                (*it)->setColor(color);
                emit setNodeColor((*it)->number(), color);
            }
        }
    }
    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Graph::vertexColor
 * @param v1
 * @return
 */
QColor Graph::vertexColor(const int &v1) const
{
    return QColor(m_graph[vpos[v1]]->color());
}

/**
 * @brief Graph::vertexColorInit
 * default vertex color initialization
 * @param color
 */
void Graph::vertexColorInit(const QString &color)
{
    initVertexColor = color;
}

// 
// Vertex number styling (font/color/distance)
// 

/**
 * @brief Changes the initial color of the vertex numbers
 * @param color
 */
void Graph::vertexNumberColorInit(const QString &color)
{
    initVertexNumberColor = color;
}

/**
 * @brief Graph::vertexColorSet
 * Changes the color of vertex v1
 * @param v1
 * @param color
 */
void Graph::vertexNumberColorSet(const int &v1, const QString &color)
{
    qDebug() << "Setting number color for vertex:" << v1 << "new number color:" << color;
    if (v1)
    {
        m_graph[vpos[v1]]->setNumberColor(color);
        emit setNodeNumberColor(m_graph[vpos[v1]]->number(), color);
    }
    else
    {
        qDebug() << "Changing color for all node numbers";
        vertexNumberColorInit(color);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                (*it)->setNumberColor(color);
                emit setNodeNumberColor((*it)->number(), color);
            }
        }
    }
    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Changes the initial size of vertex numbers
 * @param size
 */
void Graph::vertexNumberSizeInit(const int &size)
{
    initVertexNumberSize = size;
}

/**
 * @brief Changes the size of vertex v number
 * @param v
 * @param size
 */
void Graph::vertexNumberSizeSet(const int &v, const int &size)
{

    if (v)
    {
        qDebug() << "Changing number size for vertex" << v << "new number size" << size;
        m_graph[vpos[v]]->setNumberSize(size);
        emit setNodeNumberSize(m_graph[vpos[v]]->number(), size);
    }
    else
    {
        qDebug() << "Setting new number size for all vertices to:" << size;
        vertexNumberSizeInit(size);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                qDebug() << "for all, setting vertex" << (*it)->number()
                         << " new number size " << size;
                (*it)->setNumberSize(size);
                emit setNodeNumberSize((*it)->number(), size);
            }
        }
    }

    setModStatus(ModStatus::MinorOptions);
}

/**
 * @brief Changes the initial distance of vertex numbers
 * @param distance
 */
void Graph::vertexNumberDistanceInit(const int &distance)
{
    initVertexNumberDistance = distance;
}

/**
 * @brief Changes the distance.of vertex v number from the vertex
 * @param v
 * @param size
 */
void Graph::vertexNumberDistanceSet(const int &v, const int &newDistance)
{
    if (v)
    {
        qDebug() << "Changing number distance for vertex" << v
                 << "new number distance"
                 << newDistance;

        m_graph[vpos[v]]->setNumberDistance(newDistance);
        emit setNodeNumberDistance(v, newDistance);
    }
    else
    {
        qDebug() << "Changing number distance for all vertices, "
                    "new number distance"
                 << newDistance;
        vertexNumberDistanceInit(newDistance);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                (*it)->setNumberDistance(newDistance);
                emit setNodeNumberDistance((*it)->number(), newDistance);
            }
        }
    }
    setModStatus(ModStatus::MinorOptions);
}

// 
// Vertex label styling (font/color/distance)
// 

/**
 * @brief Changes the label of a vertex v1
 * @param v1
 * @param label
 */
void Graph::vertexLabelSet(const int &v1, const QString &label)
{
    qDebug() << "Graph::vertexLabelSet() - vertex " << v1
             << "vpos " << vpos[v1]
             << "new label" << label;
    m_graph[vpos[v1]]->setLabel(label);
    emit setNodeLabel(m_graph[vpos[v1]]->number(), label);

    setModStatus(ModStatus::VertexMetadata);
}

/**
 * @brief Returns the label of a vertex v1
 * @param v1
 * @return
 */
QString Graph::vertexLabel(const int &v) const
{
    return m_graph[vpos[v]]->label();
}

/**
 * @brief Graph::vertexLabelSizeInit
 * Changes the default size of vertex labels
 * @param newSize
 */
void Graph::vertexLabelSizeInit(int newSize)
{
    initVertexLabelSize = newSize;
}

/**
 * @brief Changes the label size of vertex v1 or all vertices if v1=0
 * @param v1
 * @param size
 */
void Graph::vertexLabelSizeSet(const int &v1, const int &labelSize)
{
    if (v1)
    {
        qDebug() << "Changing the label size of vertex" << v1
                 << "new label size:" << labelSize;
        m_graph[vpos[v1]]->setLabelSize(labelSize);
        emit setNodeLabelSize(v1, labelSize);
    }
    else
    {
        qDebug() << "Changing the label size of all vertices, new label size"
                 << labelSize;
        vertexLabelSizeInit(labelSize);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                qDebug() << "Changing label size of all vertices, set vertex"
                         << (*it)->number()
                         << "new label size"
                         << labelSize;
                (*it)->setLabelSize(labelSize);
                emit setNodeLabelSize((*it)->number(), labelSize);
            }
        }
    }

    setModStatus(ModStatus::MinorOptions);
}

/**
 * @brief Changes the label color of vertex v1 or all vertices if v1 = 0
 * @param v1
 * @param color
 */
void Graph::vertexLabelColorSet(const int &v1, const QString &color)
{
    if (v1)
    {
        qDebug() << "Changing the label color of vertex" << v1
                 << "new label color" << color;
        m_graph[vpos[v1]]->setLabelColor(color);
        emit setNodeLabelColor(v1, color);
    }
    else
    {
        qDebug() << "Changing the label color of all vertices, "
                    "new label color"
                 << color;
        vertexLabelColorInit(color);
        VList::const_iterator it;
        for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
        {
            if (!(*it)->isEnabled())
            {
                continue;
            }
            else
            {
                qDebug() << "Changing the label color of all, set vertex"
                         << v1
                         << "new label color"
                         << color;
                (*it)->setLabelColor(color);
                emit setNodeLabelColor((*it)->number(), color);
            }
        }
    }
    setModStatus(ModStatus::MinorOptions);
}

/**
 * @brief Graph::vertexLabelColorInit
 * Changes the default vertex label color
 * @param color
 */
void Graph::vertexLabelColorInit(QString color)
{
    initVertexLabelColor = color;
}

/**
 * @brief Changes the distance.of vertex v label from the vertex
 * @param v
 * @param size
 */
void Graph::vertexLabelDistanceSet(const int &v, const int &newDistance)
{
    m_graph[vpos[v]]->setLabelDistance(newDistance);

    setModStatus(ModStatus::MinorOptions);
    emit setNodeLabelDistance(v, newDistance);
}

/**
 * @brief Changes the distance of all vertex labels from their vertices
 * @param size
 */
void Graph::vertexLabelDistanceAllSet(const int &newDistance)
{
    qDebug() << "Changing the label distance of all vertices to:" << newDistance;
    vertexLabelDistanceInit(newDistance);
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled())
        {
            continue;
        }
        else
        {
            qDebug() << "vertex" << (*it)->number()
                     << " new label distance:" << newDistance;
            (*it)->setLabelDistance(newDistance);
            emit setNodeLabelDistance((*it)->number(), newDistance);
        }
    }

    setModStatus(ModStatus::MinorOptions);
}

/**
 * @brief Changes the default distance of vertex labels
 * @param distance
 */
void Graph::vertexLabelDistanceInit(const int &distance)
{
    initVertexLabelDistance = distance;
}

/**
 * @brief Sets custom attributes for a specified vertex.
 *
 * This function assigns a set of custom attributes to a vertex identified by its index.
 * It also updates the modification status to indicate that vertex metadata has been changed.
 *
 * @param v1 The index of the vertex for which custom attributes are being set.
 * @param customAttributes A QHash containing the custom attributes to be set for the vertex.
 *                         The keys and values of the QHash are both QStrings.
 */
void Graph::vertexCustomAttributesSet(const int &v1, const QHash<QString, QString> &customAttributes)
{
    // qDebug() << "Setting custom attributes for vertex" << v1 << ":"<< customAttributes;
    m_graph[vpos[v1]]->setCustomAttributes(customAttributes);
    setModStatus(ModStatus::VertexMetadata);
}

