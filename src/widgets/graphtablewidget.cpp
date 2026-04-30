/**
 * @file graphtablewidget.cpp
 * @brief Implements GraphTableWidget — the tabbed node/edge data table panel
 *        (#225).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "graphtablewidget.h"
#include "nodetablemodel.h"
#include "edgetablemodel.h"

#include <QTabWidget>
#include <QTableView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QLabel>

/**
 * @brief Constructs the GraphTableWidget.
 *
 * Builds both tabs (Nodes, Edges) with search bar, Refresh button, and a
 * sortable/searchable QTableView backed by a QSortFilterProxyModel.
 */
GraphTableWidget::GraphTableWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("GraphTableWidget");

    // -----------------------------------------------------------------------
    // Create models
    // -----------------------------------------------------------------------
    m_nodeModel = new NodeTableModel(this);
    m_edgeModel = new EdgeTableModel(this);

    // -----------------------------------------------------------------------
    // Proxy models
    // -----------------------------------------------------------------------
    m_nodeProxy = new QSortFilterProxyModel(this);
    m_nodeProxy->setSourceModel(m_nodeModel);
    m_nodeProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_nodeProxy->setFilterKeyColumn(-1); // search all columns

    m_edgeProxy = new QSortFilterProxyModel(this);
    m_edgeProxy->setSourceModel(m_edgeModel);
    m_edgeProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_edgeProxy->setFilterKeyColumn(-1);

    // -----------------------------------------------------------------------
    // Helper lambda to build a configured QTableView
    // -----------------------------------------------------------------------
    auto makeView = [this]() -> QTableView * {
        QTableView *view = new QTableView(this);
        view->setSortingEnabled(true);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setAlternatingRowColors(true);
        view->setEditTriggers(QAbstractItemView::DoubleClicked
                              | QAbstractItemView::EditKeyPressed);
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->hide();
        return view;
    };

    // -----------------------------------------------------------------------
    // Nodes tab
    // -----------------------------------------------------------------------
    m_nodeView = makeView();
    m_nodeView->setModel(m_nodeProxy);

    m_nodeSearch = new QLineEdit(this);
    m_nodeSearch->setPlaceholderText(tr("Search..."));
    m_nodeSearch->setClearButtonEnabled(true);

    QPushButton *nodeRefreshBtn = new QPushButton(tr("Refresh"), this);
    nodeRefreshBtn->setToolTip(tr("Reload node data from the current graph"));

    QHBoxLayout *nodeTopBar = new QHBoxLayout;
    nodeTopBar->addWidget(m_nodeSearch, 1);
    nodeTopBar->addWidget(nodeRefreshBtn);

    QWidget *nodeTab = new QWidget(this);
    QVBoxLayout *nodeLayout = new QVBoxLayout(nodeTab);
    nodeLayout->setContentsMargins(4, 4, 4, 4);
    nodeLayout->setSpacing(4);
    nodeLayout->addLayout(nodeTopBar);
    nodeLayout->addWidget(m_nodeView, 1);

    // -----------------------------------------------------------------------
    // Edges tab
    // -----------------------------------------------------------------------
    m_edgeView = makeView();
    m_edgeView->setModel(m_edgeProxy);

    m_edgeSearch = new QLineEdit(this);
    m_edgeSearch->setPlaceholderText(tr("Search..."));
    m_edgeSearch->setClearButtonEnabled(true);

    QPushButton *edgeRefreshBtn = new QPushButton(tr("Refresh"), this);
    edgeRefreshBtn->setToolTip(tr("Reload edge data from the current graph"));

    QHBoxLayout *edgeTopBar = new QHBoxLayout;
    edgeTopBar->addWidget(m_edgeSearch, 1);
    edgeTopBar->addWidget(edgeRefreshBtn);

    QWidget *edgeTab = new QWidget(this);
    QVBoxLayout *edgeLayout = new QVBoxLayout(edgeTab);
    edgeLayout->setContentsMargins(4, 4, 4, 4);
    edgeLayout->setSpacing(4);
    edgeLayout->addLayout(edgeTopBar);
    edgeLayout->addWidget(m_edgeView, 1);

    // -----------------------------------------------------------------------
    // Tab widget
    // -----------------------------------------------------------------------
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(nodeTab, tr("Nodes"));
    m_tabs->addTab(edgeTab, tr("Edges"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_tabs);

    // -----------------------------------------------------------------------
    // Connections
    // -----------------------------------------------------------------------

    // Search boxes → proxy filter
    connect(m_nodeSearch, &QLineEdit::textChanged,
            m_nodeProxy, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_edgeSearch, &QLineEdit::textChanged,
            m_edgeProxy, &QSortFilterProxyModel::setFilterFixedString);

    // Refresh buttons
    connect(nodeRefreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_graph)
            refresh(m_graph);
    });
    connect(edgeRefreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_graph)
            refresh(m_graph);
    });

    // Node row click → nodeSelected signal
    connect(m_nodeView, &QTableView::clicked,
            this, &GraphTableWidget::onNodeRowClicked);
}

/**
 * @brief Repopulates both models from @p graph and resizes columns.
 */
void GraphTableWidget::refresh(Graph *graph)
{
    m_graph = graph;

    m_nodeModel->populate(graph);
    m_edgeModel->populate(graph);

    m_nodeView->resizeColumnsToContents();
    m_edgeView->resizeColumnsToContents();
}

/**
 * @brief Maps the proxy index to a source index and emits nodeSelected().
 */
void GraphTableWidget::onNodeRowClicked(const QModelIndex &proxyIndex)
{
    const QModelIndex src = m_nodeProxy->mapToSource(proxyIndex);
    if (!src.isValid())
        return;

    const QVariant num =
        m_nodeModel->data(m_nodeModel->index(src.row(), 0), Qt::DisplayRole);
    emit nodeSelected(num.toInt());
}
