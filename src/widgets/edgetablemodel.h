/**
 * @file edgetablemodel.h
 * @brief Declares EdgeTableModel — a QAbstractTableModel for edge data (#225).
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
 * @brief Table model that caches edge data for the current relation from
 *        Graph and writes back via the Graph API.
 *
 * Fixed columns: Source, Target, Relation, Weight, Label, Color.
 * Dynamic columns: one per custom attribute key reported by the graph.
 */
class EdgeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EdgeTableModel(QObject *parent = nullptr);

    /**
     * @brief Rebuilds the internal cache from @p graph (current relation only).
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
    struct EdgeRow {
        int     source;
        int     target;
        QString relation;
        qreal   weight;
        QString label;
        QString color;
        QList<QString> attrValues; ///< parallel to m_attrKeys
    };

    Graph          *m_graph = nullptr;
    QList<EdgeRow>  m_rows;
    QStringList     m_attrKeys;

    static constexpr int FIXED_COLS  = 6;

    // Column index constants
    static constexpr int COL_SOURCE   = 0;
    static constexpr int COL_TARGET   = 1;
    static constexpr int COL_RELATION = 2;
    static constexpr int COL_WEIGHT   = 3;
    static constexpr int COL_LABEL    = 4;
    static constexpr int COL_COLOR    = 5;
};
