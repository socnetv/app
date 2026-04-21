/**
 * @file graph_parse_sink_graph.cpp
 * @brief Graph-backed implementation of IGraphParseSink (Parser -> Graph bridge).
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
#include "graph_parse_sink_graph.h"

namespace SocNetV::IO
{

    GraphParseSinkGraph::GraphParseSinkGraph(Graph &graph)
        : m_graph(graph)
    {
    }

    void GraphParseSinkGraph::addNewRelation(const QString &relName, const bool &changeRelation)
    {
        m_graph.relationAdd(relName, changeRelation);
    }

    void GraphParseSinkGraph::setRelation(int relNum, const bool &updateUI)
    {
        m_graph.relationSet(relNum, updateUI);
    }

    void GraphParseSinkGraph::createNode(const int &num,
                                         const int &size,
                                         const QString &color,
                                         const QString &numColor,
                                         const int &numSize,
                                         const QString &label,
                                         const QString &lColor,
                                         const int &lSize,
                                         const QPointF &p,
                                         const QString &shape,
                                         const QString &iconPath,
                                         const bool &signalMW,
                                         const QHash<QString, QString> nodeCustomAttributes)
    {
        m_graph.vertexCreate(num,
                             size,
                             color,
                             numColor,
                             numSize,
                             label,
                             lColor,
                             lSize,
                             p,
                             shape,
                             iconPath,
                             signalMW,
                             nodeCustomAttributes);
    }

    void GraphParseSinkGraph::createNodeAtPosRandom(const bool &signalMW)
    {
        m_graph.vertexCreateAtPosRandom(signalMW);
    }

    void GraphParseSinkGraph::createNodeAtPosRandomWithLabel(const int &num,
                                                             const QString &label,
                                                             const bool &signalMW)
    {
        m_graph.vertexCreateAtPosRandomWithLabel(num, label, signalMW);
    }

    void GraphParseSinkGraph::createEdge(const int &source,
                                         const int &target,
                                         const qreal &weight,
                                         const QString &color,
                                         const int &edgeDirType,
                                         const bool &arrows,
                                         const bool &bezier,
                                         const QString &edgeLabel,
                                         const bool &signalMW,
                                         const QHash<QString,QString> &edgeCustomAttributes)
    {
        m_graph.edgeCreate(source,
                           target,
                           weight,
                           color,
                           edgeDirType,
                           arrows,
                           bezier,
                           edgeLabel,
                           signalMW,
                           edgeCustomAttributes);
    }

    void GraphParseSinkGraph::removeDummyNode(int num)
    {
        m_graph.vertexRemoveDummyNode(num);
    }

    void GraphParseSinkGraph::fileLoaded(const int &fileType,
                                         const QString &fileName,
                                         const QString &netName,
                                         const int &totalNodes,
                                         const int &totalLinks,
                                         const int &edgeDirType,
                                         const qint64 &elapsedTime,
                                         const QString &message)
    {
        m_graph.graphFileLoaded(fileType,
                                fileName,
                                netName,
                                totalNodes,
                                totalLinks,
                                edgeDirType,
                                elapsedTime,
                                message);
    }

} // namespace SocNetV::IO