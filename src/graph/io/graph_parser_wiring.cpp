/**
 * @file graph_parser_wiring.cpp
 * @brief Implements centralized Parser -> Graph signal wiring for deterministic IO loading.
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
 * @website http://dimitris.apeiro.gr
 */

#include "graph.h"

#include "parser.h"

#include <QObject>

#include "graph/io/graph_parser_wiring.h"

namespace SocNetV::IO {

void wireParserToGraph(Parser *parser, Graph *graph)
{
    // ---- signal wiring ----
    QObject::connect(parser, &Parser::signalAddNewRelation,
                     graph, &Graph::relationAdd);

    QObject::connect(parser, &Parser::signalSetRelation,
                     graph, &Graph::relationSet);

    QObject::connect(parser, &Parser::signalCreateNode,
                     graph, &Graph::vertexCreate);

    QObject::connect(parser, &Parser::signalCreateNodeAtPosRandom,
                     graph, &Graph::vertexCreateAtPosRandom);

    QObject::connect(parser, &Parser::signalCreateNodeAtPosRandomWithLabel,
                     graph, &Graph::vertexCreateAtPosRandomWithLabel);

    QObject::connect(parser, &Parser::signalCreateEdge,
                     graph, &Graph::edgeCreate);

    QObject::connect(parser, &Parser::signalFileLoaded,
                     graph, &Graph::graphFileLoaded);

    // Legacy signal/slot style retained to preserve exact behavior/signature matching.
    QObject::connect(parser, SIGNAL(removeDummyNode(int)),
                     graph, SLOT(vertexRemoveDummyNode(int)));
}

} // namespace SocNetV::IO