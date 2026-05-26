/**
 * @file filter_spec.h
 * @brief Replay descriptor stored alongside each GraphVisibilitySnapshot.
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
#include "../../global.h"   // IndexType

/**
 * @brief Records how to re-apply one filter operation from scratch.
 *
 * One FilterSpec is embedded in every GraphVisibilitySnapshot.
 * When the user removes an arbitrary filter chip, Graph::vertexFilterRemoveAt()
 * restores to the pre-first-filter base state and replays the remaining specs
 * in order via vertexFilterReplaySpec().
 *
 * Every filter operation (node OR edge) pushes exactly one snapshot, so the
 * filter bar's chip order maps 1:1 to the snapshot stack (barIndex == stackIndex).
 * This invariant is maintained by chipCloseRequested using barIndex directly.
 *
 * Centrality replay requires the relevant index to still be computed;
 * if it is not, vertexFilterByCentrality() returns early with a status message
 * and that step is silently skipped during replay.
 */
struct FilterSpec
{
    enum class Type {
        None,
        Attribute,      // node attribute  — replay via vertexFilterByAttribute(condition)
        Selection,      // node selection  — replay via vertexFilterBySelection(selection)
        Ego,            // ego network     — replay via vertexFilterByEgoNetwork(egoVertex, egoDepth)
        Centrality,     // node centrality — replay via vertexFilterByCentrality(...)
        EdgeAttribute,  // edge attribute  — replay via edgeFilterByAttribute(condition)
        EdgeWeight,     // edge weight     — replay via edgeFilterByWeight(threshold, overThreshold)
        Query,          // node AND-query  — replay via vertexFilterByQuery(queryConditions)
        EdgeQuery       // edge AND-query  — replay via edgeFilterByQuery(queryConditions)
    };

    Type type = Type::None;

    // Type::Attribute / Type::EdgeAttribute
    FilterCondition condition;

    // Type::Ego
    int egoVertex = 0;
    int egoDepth  = 1;

    // Type::Selection
    QList<int> selection;

    // Type::Centrality
    float              centralityThreshold     = 0.0f;
    bool               centralityOverThreshold = false;
    SocNetV::IndexType centralityIndex         = SocNetV::IndexType::DC;

    // Type::EdgeWeight
    float edgeWeightThreshold     = 0.0f;
    bool  edgeWeightOverThreshold = false;

    // Type::Query / Type::EdgeQuery
    QList<FilterCondition> queryConditions;
};
