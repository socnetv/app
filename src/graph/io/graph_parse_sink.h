/**
 * @file graph_parse_sink.h
 * @brief Transitional mutation sink interface for Parser -> Graph IO operations.
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

#include <QHash>
#include <QPointF>
#include <QString>

namespace SocNetV::IO
{

    /**
     * @brief Explicit mutation surface for parse-time graph construction.
     *
     * IMPORTANT (WS4/P3 guardrail):
     *   Method signatures intentionally mirror Parser signals 1:1.
     *   No cleanup, normalization, or semantic changes are permitted here.
     */
    class IGraphParseSink
    {
    public:
        virtual ~IGraphParseSink() = default;

        // Mirrors: Parser::signalAddNewRelation(const QString&, const bool&)
        virtual void addNewRelation(const QString &relName, const bool &changeRelation = false) = 0;

        // Mirrors: Parser::signalSetRelation(int, const bool&)
        virtual void setRelation(int relNum, const bool &updateUI = true) = 0;

        // Mirrors: Parser::signalCreateNode(...)
        virtual void createNode(const int &num,
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
                                    QHash<QString, QString>()) = 0;

        // Mirrors: Parser::signalCreateNodeAtPosRandom(const bool&)
        virtual void createNodeAtPosRandom(const bool &signalMW = false) = 0;

        // Mirrors: Parser::signalCreateNodeAtPosRandomWithLabel(const int&, const QString&, const bool&)
        virtual void createNodeAtPosRandomWithLabel(const int &num,
                                                    const QString &label,
                                                    const bool &signalMW = false) = 0;

        // Mirrors: Parser::signalCreateEdge(...)
        virtual void createEdge(const int &source,
                                const int &target,
                                const qreal &weight,
                                const QString &color,
                                const int &edgeDirType,
                                const bool &arrows,
                                const bool &bezier,
                                const QString &edgeLabel = QString(),
                                const bool &signalMW = false,
                                const QHash<QString,QString> &edgeCustomAttributes =
                                    QHash<QString,QString>()) = 0;

        // Mirrors: Parser::removeDummyNode(int)
        virtual void removeDummyNode(int num) = 0;

        // Mirrors: Parser::signalFileLoaded(...)
        virtual void fileLoaded(const int &fileType,
                                const QString &fileName,
                                const QString &netName,
                                const int &totalNodes,
                                const int &totalLinks,
                                const int &edgeDirType,
                                const qint64 &elapsedTime,
                                const QString &message = QString()) = 0;
    };

} // namespace SocNetV::IO