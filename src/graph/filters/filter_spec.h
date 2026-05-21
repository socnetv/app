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
 * When the user removes an arbitrary filter chip (not just the top one),
 * Graph::vertexFilterRemoveAt() restores to the pre-first-filter base state
 * and replays the remaining specs in order via vertexFilterReplaySpec().
 *
 * Centrality replay requires the relevant index to still be computed;
 * if it is not, vertexFilterByCentrality() returns early with a status message
 * and that step is silently skipped during replay.
 */
struct FilterSpec
{
    enum class Type { None, Attribute, Selection, Ego, Centrality };

    Type type = Type::None;

    // Type::Attribute — replay via vertexFilterByAttribute(condition)
    FilterCondition condition;

    // Type::Ego — replay via vertexFilterByEgoNetwork(egoVertex, egoDepth)
    int egoVertex = 0;
    int egoDepth  = 1;

    // Type::Selection — replay via vertexFilterBySelection(selection)
    QList<int> selection;

    // Type::Centrality — replay via vertexFilterByCentrality(...)
    float           centralityThreshold     = 0.0f;
    bool            centralityOverThreshold = false;
    SocNetV::IndexType centralityIndex      = SocNetV::IndexType::DC;
};
