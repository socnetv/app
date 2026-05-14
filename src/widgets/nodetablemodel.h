/**
 * @file nodetablemodel.h
 * @brief Declares NodeTableModel — a QAbstractTableModel for node data (#225).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QAbstractTableModel>
#include <QBrush>
#include <QColor>
#include <QStringList>

class Graph;

/**
 * @brief Table model that caches node data from Graph and writes back via the
 *        Graph API.
 *
 * Fixed columns: #, Label, Visible, Shape, Size, Color.
 * Dynamic columns: one per custom attribute key reported by the graph.
 */
class NodeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit NodeTableModel(QObject *parent = nullptr);

    /**
     * @brief Rebuilds the internal cache from @p graph.
     *
     * Calls beginResetModel() / endResetModel() so connected views repaint.
     */
    void populate(Graph *graph);

    /** Custom attribute keys present in the current graph (dynamic columns). */
    QStringList attrKeys() const { return m_attrKeys; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

private:
    struct NodeRow {
        int     number;
        QString label;
        bool    visible;
        QString shape;
        int     size;
        QString color;          ///< hex string, e.g. "#ff0000"
        QList<QString> attrValues; ///< parallel to m_attrKeys
    };

    Graph          *m_graph = nullptr;
    QList<NodeRow>  m_rows;
    QStringList     m_attrKeys;

    static constexpr int FIXED_COLS = 6;

    // Column index constants
    static constexpr int COL_NUMBER  = 0;
    static constexpr int COL_LABEL   = 1;
    static constexpr int COL_VISIBLE = 2;
    static constexpr int COL_SHAPE   = 3;
    static constexpr int COL_SIZE    = 4;
    static constexpr int COL_COLOR   = 5;
};
