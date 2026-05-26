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
#include "../global.h"  // SelectedEdge = QPair<int,int>

class Graph;
class NodeTableModel;
class EdgeTableModel;
class QAbstractItemModel;
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

    /** Returns the raw (unfiltered) node source model. */
    QAbstractItemModel *nodeModel() const;
    /** Returns the raw (unfiltered) edge source model. */
    QAbstractItemModel *edgeModel() const;

    /**
     * @brief Supplies the node shape list and icon paths so the bulk-edit
     *        dialog can offer Shape as a property option for nodes.
     */
    void setNodeShapeLists(const QStringList &shapes, const QStringList &icons);

public slots:
    /**
     * @brief Repopulates both the node and edge models from @p graph.
     *
     * Stores a pointer to @p graph for subsequent Refresh button clicks.
     */
    void refresh(Graph *graph);

    /**
     * @brief Highlights the rows in the Nodes tab that match @p nodeNumbers.
     *
     * Rows not in the list are deselected. Skips nodes that are filtered
     * out by the current search. O(model_rows + selected).
     */
    void syncNodeSelection(const QList<int> &nodeNumbers);

    /**
     * @brief Highlights the rows in the Edges tab that match @p edges.
     *
     * Each SelectedEdge is a (source, target) pair. O(model_rows + selected).
     */
    void syncEdgeSelection(const QList<SocNetV::SelectedEdge> &edges);

    /** Switches the tab widget to the Nodes tab. */
    void showNodesTab();

    /** Switches the tab widget to the Edges tab. */
    void showEdgesTab();

    /** Prompts for a file path and exports visible node rows as CSV. */
    void exportNodesCSV();
    /** Prompts for a file path and exports visible node rows as JSON. */
    void exportNodesJSON();
    /** Prompts for a file path and exports visible edge rows as CSV. */
    void exportEdgesCSV();
    /** Prompts for a file path and exports visible edge rows as JSON. */
    void exportEdgesJSON();

    /** Opens the column-mapping dialog and imports node attributes from a CSV. */
    void importNodesCSV();
    /** Opens the column-mapping dialog and imports node attributes from a JSON. */
    void importNodesJSON();
    /** Opens the column-mapping dialog and imports edge attributes from a CSV. */
    void importEdgesCSV();
    /** Opens the column-mapping dialog and imports edge attributes from a JSON. */
    void importEdgesJSON();

signals:
    /** Emitted when the user clicks a row in the Nodes tab. */
    void nodeSelected(int number);

    /** Emitted when the user clicks a row in the Edges tab. */
    void edgeSelected(int source, int target);

    /** Emitted with a status message after an export attempt. */
    void exportStatusMessage(const QString &message);

    /** Emitted with a status message after an import attempt. */
    void importStatusMessage(const QString &message);

private slots:
    void onNodeRowClicked(const QModelIndex &proxyIndex);
    void onEdgeRowClicked(const QModelIndex &proxyIndex);
    void onNodeSetPropertyClicked();
    void onNodeRemoveAttributeClicked();
    void onNodeAddAttributeClicked();
    void onEdgeSetPropertyClicked();
    void onEdgeRemoveAttributeClicked();
    void onEdgeAddAttributeClicked();

private:
    void doExport(QAbstractItemModel *proxyModel,
                  const QString &defaultName,
                  bool csv);
    void doImport(bool forNodes, bool csv);

    /** Returns node numbers for selected rows, or all visible rows if none selected. */
    QList<int> resolveNodeTargets() const;
    /** Returns (source,target) pairs for selected rows, or all visible rows if none selected. */
    QList<SocNetV::SelectedEdge> resolveEdgeTargets() const;

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
    QStringList            m_nodeShapeList;
    QStringList            m_iconPathList;
};
