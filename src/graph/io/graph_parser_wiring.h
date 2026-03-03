/**
 * @file graph_parser_wiring.h
 * @brief Centralizes Parser -> Graph signal wiring for deterministic IO loading.
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

#ifndef SOCNETV_GRAPH_PARSER_WIRING_H
#define SOCNETV_GRAPH_PARSER_WIRING_H

class Graph;
class Parser;

namespace SocNetV::IO {

/**
 * @brief Wires Parser mutation signals into Graph mutator slots.
 *
 * This is a mechanical extraction of the existing wiring used by Graph::loadFile()
 * and the CLI headless loader. No behavior changes.
 */
void wireParserToGraph(Parser *parser, Graph *graph);

} // namespace SocNetV::IO

#endif // SOCNETV_GRAPH_PARSER_WIRING_H