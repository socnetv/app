/**
 * @file nodetablemodel.cpp
 * @brief Implements NodeTableModel — local cache of node data for the data table
 *        panel (#225).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "nodetablemodel.h"

#include "../graph.h"
#include "../graphvertex.h"

#include <QColor>

NodeTableModel::NodeTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

/**
 * @brief Rebuilds the internal row cache from @p graph.
 *
 * All vertices (enabled and disabled) are included; the Visible column
 * reflects the vertex's enabled state.  Custom attribute keys are taken from
 * Graph::graphHasVertexCustomAttributes().
 */
void NodeTableModel::populate(Graph *graph)
{
    beginResetModel();

    m_graph = graph;
    m_rows.clear();
    m_attrKeys.clear();

    if (!m_graph) {
        endResetModel();
        return;
    }

    m_attrKeys = m_graph->graphHasVertexCustomAttributes();

    for (auto it = m_graph->verticesBegin(); it != m_graph->verticesEnd(); ++it) {
        GraphVertex *v = *it;

        NodeRow row;
        row.number  = v->number();
        row.label   = m_graph->vertexLabel(row.number);
        row.visible = v->isEnabled();
        row.shape   = m_graph->vertexShape(row.number);
        row.size    = m_graph->vertexSize(row.number);
        row.color   = QColor(m_graph->vertexColor(row.number)).name();

        const QHash<QString, QString> attrs = m_graph->vertexCustomAttributes(row.number);
        for (const QString &key : m_attrKeys) {
            row.attrValues.append(attrs.value(key, QString()));
        }

        m_rows.append(row);
    }

    endResetModel();
}

int NodeTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int NodeTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return FIXED_COLS + m_attrKeys.size();
}

QVariant NodeTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_rows.size() || col < 0 || col >= columnCount())
        return QVariant();

    const NodeRow &r = m_rows.at(row);

    switch (role) {
    case Qt::DisplayRole:
        switch (col) {
        case COL_NUMBER:  return QString::number(r.number);
        case COL_LABEL:   return r.label;
        case COL_VISIBLE: return QString();   // represented by checkbox only
        case COL_SHAPE:   return r.shape;
        case COL_SIZE:    return QString::number(r.size);
        case COL_COLOR:   return r.color;
        default:
            return r.attrValues.value(col - FIXED_COLS);
        }

    case Qt::CheckStateRole:
        if (col == COL_VISIBLE)
            return r.visible ? Qt::Checked : Qt::Unchecked;
        return QVariant();

    case Qt::DecorationRole:
        if (col == COL_COLOR)
            return QColor(r.color);
        return QVariant();

    case Qt::TextAlignmentRole:
        if (col == COL_NUMBER || col == COL_SIZE)
            return static_cast<int>(Qt::AlignCenter);
        return QVariant();

    case Qt::BackgroundRole:
        if (col == COL_NUMBER || col == COL_VISIBLE || col == COL_SHAPE)
            return QBrush(QColor("#f0f0f0"));
        return QVariant();

    default:
        break;
    }

    return QVariant();
}

QVariant NodeTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case COL_NUMBER:  return tr("#");
        case COL_LABEL:   return tr("Label");
        case COL_VISIBLE: return tr("Visible");
        case COL_SHAPE:   return tr("Shape");
        case COL_SIZE:    return tr("Size");
        case COL_COLOR:   return tr("Color");
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

Qt::ItemFlags NodeTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int col = index.column();

    // Read-only columns: #, Visible, Shape
    if (col == COL_NUMBER || col == COL_VISIBLE || col == COL_SHAPE)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    // Editable columns: Label, Size, Color, custom attrs
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool NodeTableModel::setData(const QModelIndex &index, const QVariant &value,
                             int role)
{
    if (!index.isValid() || role != Qt::EditRole || !m_graph)
        return false;

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_rows.size() || col < 0 || col >= columnCount())
        return false;

    NodeRow &r = m_rows[row];

    switch (col) {
    case COL_LABEL: {
        const QString newVal = value.toString();
        if (r.label == newVal)
            return false;
        r.label = newVal;
        m_graph->vertexLabelSet(r.number, newVal);
        break;
    }
    case COL_SIZE: {
        const int newVal = value.toInt();
        if (r.size == newVal)
            return false;
        r.size = newVal;
        m_graph->vertexSizeSet(r.number, newVal);
        break;
    }
    case COL_COLOR: {
        const QString newVal = QColor(value.toString()).name();
        if (r.color == newVal)
            return false;
        r.color = newVal;
        m_graph->vertexColorSet(r.number, newVal);
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
            m_graph->vertexCustomAttributeSet(r.number, m_attrKeys.at(attrIdx), newVal);
        } else {
            return false;
        }
        break;
    }

    emit dataChanged(index, index, {role});
    return true;
}
