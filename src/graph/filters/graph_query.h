/**
 * @file graph_query.h
 * @brief Defines GraphQuery — a multi-condition AND filter applied as one chip.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QList>
#include "filter_condition.h"

/**
 * @brief A compound AND-logic filter applied in one operation (one snapshot, one chip).
 *
 * All conditions must be satisfied for a node or edge to remain visible.
 * The scope (Nodes or Edges) is set on each FilterCondition; all conditions
 * in one GraphQuery carry the same scope (set by DialogQueryBuilder).
 *
 * Applied via Graph::vertexFilterByQuery() (Nodes) or Graph::edgeFilterByQuery() (Edges).
 */
struct GraphQuery
{
    QList<FilterCondition> conditions;
};
