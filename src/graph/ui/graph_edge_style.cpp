/**
 * @file graph_edge_style.cpp
 * @brief Implements edge appearance and label/visibility helpers for the Graph
 *        class (edge colors, labels, and display toggles).
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
// Visibility toggles
//
/**
 * @brief Changes the visibility of edge weight numbers
 * @param toggle
 */
void Graph::edgeWeightNumbersVisibilitySet(const bool &toggle)
{
    initEdgeWeightNumbers = toggle;
}

//
// Edge colors
//

/**
 * @brief Saves the default edge color
 * Used by random network creation methods
 * @param color
 */
void Graph::edgeColorInit(const QString &color)
{
    initEdgeColor = color;
}

/**
 * @brief Changes the color of all enabled edges.
 * @param color
 * @return
 */
bool Graph::edgeColorAllSet(const QString &color, const int &threshold)
{
    qDebug() << "Graph::edgeColorAllSet() - new color: " << color;
    int target = 0, source = 0;
    edgeColorInit(color);
    QHash<int, qreal> enabledOutEdges;
    QHash<int, qreal>::const_iterator it1;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {

        source = (*it)->number();
        if (!(*it)->isEnabled())
            continue;
        enabledOutEdges = (*it)->outEdgesEnabledHash();
        it1 = enabledOutEdges.cbegin();
        while (it1 != enabledOutEdges.cend())
        {
            target = it1.key();
            if (threshold == 0)
            {
                if (it1.value() == threshold)
                {
                    qDebug() << " Graph::edgeColorAllSet() zero weight threshold "
                             << threshold << " - edge "
                             << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else if (threshold != 0 && threshold != RAND_MAX)
            {
                if (it1.value() <= threshold)
                {
                    qDebug() << " Graph::edgeColorAllSet() below weight threshold "
                             << threshold << " - edge "
                             << source << "->" << target << " new color " << color;
                    (*it)->setOutLinkColor(target, color);
                    emit setEdgeColor(source, target, color);
                }
            }
            else
            {
                qDebug() << " Graph::edgeColorAllSet() : "
                         << source << "->" << target << " new color " << color;
                (*it)->setOutLinkColor(target, color);
                emit setEdgeColor(source, target, color);
            }
            ++it1;
        }
    }
    // delete enabledOutEdges;

    setModStatus(ModStatus::EdgeMetadata);

    return true;
}

/**
 * @brief Changes the color of edge v1->v2
 * @param v1
 * @param v2
 * @param color
 */
void Graph::edgeColorSet(const int &v1, const int &v2, const QString &color)
{
    qDebug() << "Graph::edgeColorSet() - " << v1 << "->" << v2
             << " vpos (" << vpos[v1] << "->" << vpos[v2] << ")"
             << " new color " << color;
    m_graph[vpos[v1]]->setOutLinkColor(v2, color);
    emit setEdgeColor(v1, v2, color);
    if (isSymmetric())
    {
        m_graph[vpos[v2]]->setOutLinkColor(v1, color);
        emit setEdgeColor(v2, v1, color);
    }

    setModStatus(ModStatus::EdgeMetadata);
}

/**
 * @brief Returns the color of the directed edge v1->v2
 * @param v1
 * @param v2
 * @return
 */
QString Graph::edgeColor(const int &v1, const int &v2)
{
    return m_graph[vpos[v1]]->outLinkColor(v2);
}

//
// Edge labels
//
/**
 * @brief Changes the label of edge v1->v2
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeLabelSet(const int &v1, const int &v2, const QString &label)
{
    qDebug() << "Graph::edgeLabelSet()  " << v1 << "[" << vpos[v1]
             << "]->" << v2 << "[" << vpos[v2] << "]" << " label " << label;
    m_graph[vpos[v1]]->setOutEdgeLabel(v2, label);

    emit setEdgeLabel(v1, v2, label);

    setModStatus(ModStatus::EdgeMetadata);
}

/**
 * @brief Returns the label of edge v1->v2
 * @param v1
 * @param v2
 * @return
 */
QString Graph::edgeLabel(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->outEdgeLabel(v2);
}

/**
 * @brief Sets all custom attributes on edge v1→v2, replacing any previously
 * stored attributes for that edge.
 * @param v1    Source vertex number.
 * @param v2    Target vertex number.
 * @param attrs Key/value map of custom attributes.
 */
void Graph::edgeCustomAttributesSet(const int &v1, const int &v2,
                                    const QHash<QString,QString> &attrs)
{
    m_graph[vpos[v1]]->setOutEdgeCustomAttributes(v2, attrs);
    setModStatus(ModStatus::EdgeMetadata);
}

/**
 * @brief Returns the custom attributes stored on edge v1→v2.
 * Returns an empty hash if no attributes have been set for that edge.
 * @param v1 Source vertex number.
 * @param v2 Target vertex number.
 */
QHash<QString,QString> Graph::edgeCustomAttributes(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->outEdgeCustomAttributes(v2);
}

/**
 * @brief Imports custom attributes from a parsed table into existing edges.
 *
 * Each row is matched to an edge by looking up vertex numbers from
 * @p srcColumn and @p tgtColumn.  All other columns become custom attributes
 * on the matched edge.  Rows that do not match any existing edge are skipped.
 *
 * Example — CSV input with srcColumn=0, tgtColumn=1:
 * @code
 * Source,Target,relationship,weight
 * 1,2,invested_in,0.8    ← sets edge 1→2: relationship="invested_in", weight="0.8"
 * 2,3,mentors,0.5        ← sets edge 2→3: relationship="mentors",     weight="0.5"
 * @endcode
 *
 * @return Number of edges that received at least one attribute update.
 */
int Graph::edgeAttributesImport(const QStringList &headers,
                                const QVector<QStringList> &rows,
                                int srcColumn,
                                int tgtColumn)
{
    int matched = 0;
    for (const QStringList &row : rows) {
        if (srcColumn >= row.size() || tgtColumn >= row.size())
            continue;

        bool okSrc = false, okTgt = false;
        const int src = row.at(srcColumn).toInt(&okSrc);
        const int tgt = row.at(tgtColumn).toInt(&okTgt);
        if (!okSrc || !okTgt)
            continue;

        if (edgeExists(src, tgt) == 0)
            continue;

        QHash<QString,QString> attrs = edgeCustomAttributes(src, tgt);
        for (int c = 0; c < headers.size(); ++c) {
            if (c == srcColumn || c == tgtColumn || c >= row.size())
                continue;
            attrs.insert(headers.at(c), row.at(c));
        }
        edgeCustomAttributesSet(src, tgt, attrs);
        ++matched;
    }
    qDebug() << "edgeAttributesImport: matched" << matched << "of" << rows.size() << "rows";
    return matched;
}

/**
 * @brief Returns a list of all unique custom attribute keys present across
 * all enabled edges in the current graph.
 */
QStringList Graph::graphHasEdgeCustomAttributes() const
{
    QStringList keys;
    for (auto it = m_graph.cbegin(); it != m_graph.cend(); ++it) {
        if (!(*it)->isEnabled())
            continue;
        for (auto eit = (*it)->m_outEdges.cbegin(); eit != (*it)->m_outEdges.cend(); ++eit) {
            const int v2 = eit.key();
            const QHash<QString,QString> attrs = (*it)->outEdgeCustomAttributes(v2);
            for (auto ait = attrs.cbegin(); ait != attrs.cend(); ++ait) {
                if (!keys.contains(ait.key()))
                    keys.append(ait.key());
            }
        }
    }
    return keys;
}

/**
 * @brief Toggles the visibility of edge labels.
 * @param toggle
 */
void Graph::edgeLabelsVisibilitySet(const bool &toggle)
{
    initEdgeLabels = toggle;
}
