/**
 * @file filter_condition.h
 * @brief Defines the FilterCondition struct shared by all attribute-based filters.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QString>

/**
 * @brief Describes a single attribute-based filter condition.
 *
 * Shared by Graph::vertexFilterByAttribute(), Graph::edgeFilterByAttribute(),
 * DialogFilterByAttribute, and the future filter bar (#219).
 */
struct FilterCondition
{
    enum class Scope { Nodes, Edges, Both };
    enum class Op    { Eq, Neq, Gt, Lt, Gte, Lte, Contains };

    Scope   scope = Scope::Nodes;
    QString key;
    Op      op    = Op::Eq;
    QString value;

    /** Returns true if @p attrValue satisfies this condition. */
    bool matches(const QString &attrValue) const
    {
        switch (op) {
        case Op::Eq:       return attrValue == value;
        case Op::Neq:      return attrValue != value;
        case Op::Contains: return attrValue.contains(value, Qt::CaseInsensitive);
        default: break;
        }
        bool okA = false, okB = false;
        const double a = attrValue.toDouble(&okA);
        const double b = value.toDouble(&okB);
        if (okA && okB) {
            switch (op) {
            case Op::Gt:  return a >  b;
            case Op::Lt:  return a <  b;
            case Op::Gte: return a >= b;
            case Op::Lte: return a <= b;
            default: break;
            }
        }
        switch (op) {
        case Op::Gt:  return attrValue >  value;
        case Op::Lt:  return attrValue <  value;
        case Op::Gte: return attrValue >= value;
        case Op::Lte: return attrValue <= value;
        default: break;
        }
        return false;
    }

    /** Short human-readable label for a filter bar chip. */
    QString label() const
    {
        QString scopeStr;
        switch (scope) {
        case Scope::Nodes: scopeStr = QStringLiteral("Nodes"); break;
        case Scope::Edges: scopeStr = QStringLiteral("Edges"); break;
        case Scope::Both:  scopeStr = QStringLiteral("Nodes+Edges"); break;
        }

        QString opStr;
        switch (op) {
        case Op::Eq:       opStr = QStringLiteral("=");        break;
        case Op::Neq:      opStr = QStringLiteral("≠");        break;
        case Op::Gt:       opStr = QStringLiteral(">");         break;
        case Op::Lt:       opStr = QStringLiteral("<");         break;
        case Op::Gte:      opStr = QStringLiteral("≥");        break;
        case Op::Lte:      opStr = QStringLiteral("≤");        break;
        case Op::Contains: opStr = QStringLiteral("contains"); break;
        }

        return scopeStr + QLatin1String(": ") + key + QLatin1Char(' ') + opStr + QLatin1Char(' ') + value;
    }
};
