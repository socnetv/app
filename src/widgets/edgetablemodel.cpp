/**
 * @file edgetablemodel.cpp
 * @brief Implements EdgeTableModel — local cache of edge data for the data table
 *        panel (#225).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "edgetablemodel.h"

#include "../graph.h"
#include "../graphvertex.h"

#include <QColor>

EdgeTableModel::EdgeTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

/**
 * @brief Rebuilds the internal row cache from the current relation of @p graph.
 *
 * Only enabled edges belonging to the current relation are included.
 * Custom attribute keys are taken from Graph::graphHasEdgeCustomAttributes().
 */
void EdgeTableModel::populate(Graph *graph)
{
    beginResetModel();

    m_graph = graph;
    m_rows.clear();
    m_attrKeys.clear();

    if (!m_graph) {
        endResetModel();
        return;
    }

    m_attrKeys = m_graph->graphHasEdgeCustomAttributes();

    const int curRel     = m_graph->relationCurrent();
    const QString relName = m_graph->relationCurrentName();

    for (auto vIt = m_graph->verticesBegin(); vIt != m_graph->verticesEnd(); ++vIt) {
        GraphVertex *v = *vIt;

        // Iterate over the out-edges of this vertex.
        // H_edges: QMultiHash<int, QPair<int, QPair<qreal, bool>>>
        //   key   = target vertex number
        //   value = (relation index, (weight, enabled))
        const H_edges &edges = v->outEdges();
        for (auto eIt = edges.cbegin(); eIt != edges.cend(); ++eIt) {
            const int relation      = eIt.value().first;
            const qreal weight      = eIt.value().second.first;
            const bool enabled      = eIt.value().second.second;

            // Skip edges from other relations or disabled edges
            if (relation != curRel || !enabled)
                continue;

            const int source = v->number();
            const int target = eIt.key();

            EdgeRow row;
            row.source   = source;
            row.target   = target;
            row.relation = relName;
            row.weight   = weight;
            row.label    = m_graph->edgeLabel(source, target);
            row.color    = QColor(m_graph->edgeColor(source, target)).name();

            const QHash<QString, QString> attrs =
                m_graph->edgeCustomAttributes(source, target);
            for (const QString &key : m_attrKeys) {
                row.attrValues.append(attrs.value(key, QString()));
            }

            m_rows.append(row);
        }
    }

    endResetModel();
}

int EdgeTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int EdgeTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return FIXED_COLS + m_attrKeys.size();
}

QVariant EdgeTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_rows.size() || col < 0 || col >= columnCount())
        return QVariant();

    const EdgeRow &r = m_rows.at(row);

    switch (role) {
    case Qt::DisplayRole:
        switch (col) {
        case COL_SOURCE:   return QString::number(r.source);
        case COL_TARGET:   return QString::number(r.target);
        case COL_RELATION: return r.relation;
        case COL_WEIGHT:   return QString::number(r.weight, 'f', 2);
        case COL_LABEL:    return r.label;
        case COL_COLOR:    return r.color;
        default:
            return r.attrValues.value(col - FIXED_COLS);
        }

    case Qt::DecorationRole:
        if (col == COL_COLOR)
            return QColor(r.color);
        return QVariant();

    case Qt::TextAlignmentRole:
        if (col == COL_SOURCE || col == COL_TARGET || col == COL_WEIGHT)
            return static_cast<int>(Qt::AlignCenter);
        return QVariant();

    case Qt::BackgroundRole:
        if (col == COL_SOURCE || col == COL_TARGET || col == COL_RELATION)
            return QBrush(QColor("#f0f0f0"));
        return QVariant();

    default:
        break;
    }

    return QVariant();
}

QVariant EdgeTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case COL_SOURCE:   return tr("Source");
        case COL_TARGET:   return tr("Target");
        case COL_RELATION: return tr("Relation");
        case COL_WEIGHT:   return tr("Weight");
        case COL_LABEL:    return tr("Label");
        case COL_COLOR:    return tr("Color");
        default:
            if (section - FIXED_COLS < m_attrKeys.size())
                return m_attrKeys.at(section - FIXED_COLS);
            break;
        }
    } else {
        return section + 1;
    }

    return QVariant();
}

Qt::ItemFlags EdgeTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int col = index.column();

    // Read-only columns: Source, Target, Relation
    if (col == COL_SOURCE || col == COL_TARGET || col == COL_RELATION)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    // Editable columns: Weight, Label, Color, custom attrs
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool EdgeTableModel::setData(const QModelIndex &index, const QVariant &value,
                             int role)
{
    if (!index.isValid() || role != Qt::EditRole || !m_graph)
        return false;

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_rows.size() || col < 0 || col >= columnCount())
        return false;

    EdgeRow &r = m_rows[row];

    switch (col) {
    case COL_WEIGHT: {
        const qreal newVal = value.toDouble();
        if (qFuzzyCompare(r.weight, newVal))
            return false;
        r.weight = newVal;
        m_graph->edgeWeightSet(r.source, r.target, newVal);
        break;
    }
    case COL_LABEL: {
        const QString newVal = value.toString();
        if (r.label == newVal)
            return false;
        r.label = newVal;
        m_graph->edgeLabelSet(r.source, r.target, newVal);
        break;
    }
    case COL_COLOR: {
        const QString newVal = QColor(value.toString()).name();
        if (r.color == newVal)
            return false;
        r.color = newVal;
        m_graph->edgeColorSet(r.source, r.target, newVal);
        break;
    }
    default:
        if (col >= FIXED_COLS) {
            const int attrIdx = col - FIXED_COLS;
            if (attrIdx >= m_attrKeys.size())
                return false;
            const QString newVal = value.toString();
            if (r.attrValues.value(attrIdx) == newVal)
                return false;
            r.attrValues[attrIdx] = newVal;
            QHash<QString, QString> attrs =
                m_graph->edgeCustomAttributes(r.source, r.target);
            attrs[m_attrKeys.at(attrIdx)] = newVal;
            m_graph->edgeCustomAttributesSet(r.source, r.target, attrs);
        } else {
            return false;
        }
        break;
    }

    emit dataChanged(index, index, {role});
    return true;
}
