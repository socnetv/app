/**
 * @file graph_parse_sink_graph.h
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

#pragma once

#include "graph_parse_sink.h"

class Graph;

namespace SocNetV::IO
{

    /**
     * @brief Forwards parse mutations directly into an existing Graph instance.
     *
     * NOTE: This is intentionally a thin adapter. No logic, no ordering changes.
     */
    class GraphParseSinkGraph final : public IGraphParseSink
    {
    public:
        explicit GraphParseSinkGraph(Graph &graph);

        void addNewRelation(const QString &relName, const bool &changeRelation = false) override;
        void setRelation(int relNum, const bool &updateUI = true) override;

        void createNode(const int &num,
                        const int &size,
                        const QString &color,
                        const QString &numColor,
                        const int &numSize,
                        const QString &label,
                        const QString &lColor,
                        const int &lSize,
                        const QPointF &p,
                        const QString &shape,
                        const QString &iconPath = QString(),
                        const bool &signalMW = false,
                        const QHash<QString, QString> nodeCustomAttributes =
                            QHash<QString, QString>()) override;

        void createNodeAtPosRandom(const bool &signalMW = false) override;

        void createNodeAtPosRandomWithLabel(const int &num,
                                            const QString &label,
                                            const bool &signalMW = false) override;

        void createEdge(const int &source,
                        const int &target,
                        const qreal &weight,
                        const QString &color,
                        const int &edgeDirType,
                        const bool &arrows,
                        const bool &bezier,
                        const QString &edgeLabel = QString(),
                        const bool &signalMW = false,
                        const QHash<QString,QString> &edgeCustomAttributes =
                            QHash<QString,QString>()) override;

        void removeDummyNode(int num) override;

        void fileLoaded(const int &fileType,
                        const QString &fileName,
                        const QString &netName,
                        const int &totalNodes,
                        const int &totalLinks,
                        const int &edgeDirType,
                        const qint64 &elapsedTime,
                        const QString &message = QString()) override;

    private:
        Graph &m_graph;
    };

} // namespace SocNetV::IO