/**
 * @file graphtablewidget.h
 * @brief Declares GraphTableWidget — tabbed node/edge data table panel (#225).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QWidget>

class Graph;
class NodeTableModel;
class EdgeTableModel;
class QTabWidget;
class QTableView;
class QLineEdit;
class QSortFilterProxyModel;

/**
 * @brief A QWidget containing a QTabWidget with two tabs — Nodes and Edges —
 *        each backed by a sortable, searchable table view.
 *
 * Call refresh() whenever the graph data changes and the panel is visible.
 * The widget itself does not auto-refresh; the caller controls when to
 * rebuild the cache.
 */
class GraphTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraphTableWidget(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Repopulates both the node and edge models from @p graph.
     *
     * Stores a pointer to @p graph for subsequent Refresh button clicks.
     */
    void refresh(Graph *graph);

signals:
    /** Emitted when the user clicks a row in the Nodes tab. */
    void nodeSelected(int number);

private slots:
    void onNodeRowClicked(const QModelIndex &proxyIndex);

private:
    Graph                 *m_graph  = nullptr;
    QTabWidget            *m_tabs;
    QTableView            *m_nodeView;
    QTableView            *m_edgeView;
    NodeTableModel        *m_nodeModel;
    EdgeTableModel        *m_edgeModel;
    QSortFilterProxyModel *m_nodeProxy;
    QSortFilterProxyModel *m_edgeProxy;
    QLineEdit             *m_nodeSearch;
    QLineEdit             *m_edgeSearch;
};
