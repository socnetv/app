/**
 * @file graph_subgraphs.cpp
 * @brief Implements subgraph extraction for the Graph class.
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
#include "graphvertex.h"

#include <QDebug>
#include <QHash>

/**
 * @brief Core helper: builds an independent Graph copy from an explicit list
 *        of vertex numbers.
 *
 * Shared by subgraphExtract() (visible nodes) and subgraphExtractFromSelection()
 * (selected nodes). Vertices are renumbered from 1. For every relation, enabled
 * edges whose both endpoints are in @p vertexNums are copied. All visual
 * properties and, when @p includeCustomAttributes is true, node and edge custom
 * attribute maps are preserved verbatim.
 *
 * The caller takes ownership of the returned Graph object.
 *
 * @param vertexNums            Source vertex numbers to include.
 * @param name                  Name assigned to the new graph.
 * @param includeCustomAttributes  When true, node and edge custom
 *                              attribute maps are copied verbatim.
 * @return Heap-allocated Graph* containing the subgraph, or nullptr
 *         if @p vertexNums is empty.
 */
Graph *Graph::subgraphFromVertexList(const QList<int> &vertexNums,
                                      const QString &name,
                                      const bool &includeCustomAttributes)
{
    if (vertexNums.isEmpty()) {
        qDebug() << "subgraphFromVertexList(): vertex list is empty — returning nullptr";
        return nullptr;
    }

    // Build old-number → new-number map (renumber from 1)
    QHash<int, int> oldToNew;
    int newNum = 1;
    for (int oldNum : vertexNums)
        oldToNew[oldNum] = newNum++;

    // --- Create the new graph -------------------------------------------
    // No UI signals: signalMW is false throughout; the new graph has no
    // GraphicsWidget attached, so emitting draw signals would be a no-op
    // anyway, but passing false is explicit and safe.
    Graph *sub = new Graph(vertexNums.size(), m_reserveEdgesPerVertexSize);
    sub->setName(name);
    sub->setDirected(isDirected(), false);
    sub->setWeighted(isWeighted());
    // Propagate canvas dimensions so that IO routines normalize pixel
    // coordinates correctly on export (default is 700x600 which is
    // smaller than the actual canvas and produces out-of-range values).
    sub->canvasSizeSetQuiet(canvasWidth, canvasHeight);

    // Mirror every relation that exists in the source graph.
    // relationAdd() with changeRelation=false keeps the current relation at 0.
    const int relCount = m_relationsList.size();
    for (int r = 0; r < relCount; ++r)
        sub->relationAdd(m_relationsList.at(r), false);

    // Activate relation 0 in the subgraph (matches the default after construction).
    sub->relationSet(0, false);

    // --- Pass 1: create vertices -----------------------------------------
    for (int oldNum : vertexNums) {
        GraphVertex *v = vertexPtr(oldNum);
        sub->vertexCreate(
            oldToNew[oldNum],
            v->size(),
            v->color(),
            v->numberColor(),
            v->numberSize(),
            v->label(),
            v->labelColor(),
            v->labelSize(),
            v->pos(),
            v->shape(),
            v->shapeIconPath(),
            false,   // signalMW
            includeCustomAttributes ? v->customAttributes()
                                    : QHash<QString, QString>());
    }

    // --- Pass 2: create edges --------------------------------------------
    // Iterate every source vertex; for each out-edge whose enabled flag is
    // true and whose target is also in the vertex list, add it to the subgraph.
    // We handle each relation index independently so multi-relation networks
    // are preserved correctly.
    for (int oldSrc : vertexNums) {
        GraphVertex *sv = vertexPtr(oldSrc);
        const H_edges &oe = sv->outEdges();

        for (auto ei = oe.constBegin(); ei != oe.constEnd(); ++ei) {
            const int oldTgt       = ei.key();
            const int relIdx       = ei.value().first;
            const qreal weight     = ei.value().second.first;
            const bool edgeEnabled = ei.value().second.second;

            if (!edgeEnabled)
                continue;
            if (!oldToNew.contains(oldTgt))
                continue;   // target is not in the vertex list

            const int newSrc = oldToNew[oldSrc];
            const int newTgt = oldToNew[oldTgt];

            // Temporarily switch subgraph to the right relation so edgeCreate
            // stores the edge under the correct relation index.
            sub->relationSet(relIdx, false);

            sub->edgeCreate(
                newSrc,
                newTgt,
                weight,
                sv->outLinkColor(oldTgt),
                EdgeType::Directed,   // always directed internally; Graph::isDirected()
                                      // controls the logical interpretation
                true,    // drawArrows
                false,   // bezier
                sv->outEdgeLabel(oldTgt),
                false,   // signalMW
                includeCustomAttributes ? sv->outEdgeCustomAttributes(oldTgt)
                                        : QHash<QString, QString>());
        }
    }

    // Restore subgraph to relation 0 before handing it back.
    sub->relationSet(0, false);

    qDebug() << "subgraphFromVertexList(): created subgraph" << name
             << "— vertices:" << sub->vertices()
             << "edges:"      << sub->edgesEnabled();

    return sub;
}


/**
 * @brief Extracts currently visible (non-filtered) nodes and their
 *        inter-edges into an independent Graph copy.
 *
 * Collects all enabled vertices, then delegates to subgraphFromVertexList().
 *
 * @param name                  Name assigned to the new graph.
 * @param includeCustomAttributes  When true, node and edge custom
 *                              attribute maps are copied verbatim.
 * @return Heap-allocated Graph* containing the subgraph, or nullptr
 *         if no visible vertices exist.
 */
Graph *Graph::subgraphExtract(const QString &name,
                               const bool &includeCustomAttributes)
{
    // --- Collect visible vertices ----------------------------------------
    QList<int> visibleNums;
    for (VList::const_iterator vi = verticesBegin(); vi != verticesEnd(); ++vi) {
        if ((*vi)->isEnabled())
            visibleNums.append((*vi)->number());
    }

    if (visibleNums.isEmpty())
        qDebug() << "subgraphExtract(): no visible vertices";

    return subgraphFromVertexList(visibleNums, name, includeCustomAttributes);
}


/**
 * @brief Extracts currently selected nodes and their inter-edges into an
 *        independent Graph copy.
 *
 * Uses the current canvas selection (getSelectedVertices()), then delegates
 * to subgraphFromVertexList(). Only edges between selected nodes are included.
 *
 * @param name                  Name assigned to the new graph.
 * @param includeCustomAttributes  When true, node and edge custom
 *                              attribute maps are copied verbatim.
 * @return Heap-allocated Graph* containing the subgraph, or nullptr
 *         if no nodes are selected.
 */
Graph *Graph::subgraphExtractFromSelection(const QString &name,
                                            const bool &includeCustomAttributes)
{
    const QList<int> selected = getSelectedVertices();

    if (selected.isEmpty())
        qDebug() << "subgraphExtractFromSelection(): no selected vertices";

    return subgraphFromVertexList(selected, name, includeCustomAttributes);
}
