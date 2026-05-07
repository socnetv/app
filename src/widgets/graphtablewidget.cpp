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
#include "../graph/io/table_export.h"
#include "../graph/io/table_import.h"
#include "../forms/dialogimportattributes.h"
#include "../graph.h"

#include <QTabWidget>
#include <QTableView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

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

    QPushButton *nodeExportCSVBtn = new QPushButton(tr("Export CSV"), this);
    nodeExportCSVBtn->setToolTip(tr("Export currently visible rows as CSV"));

    QPushButton *nodeExportJSONBtn = new QPushButton(tr("Export JSON"), this);
    nodeExportJSONBtn->setToolTip(tr("Export currently visible rows as JSON"));

    QPushButton *nodeImportCSVBtn = new QPushButton(tr("Import CSV"), this);
    nodeImportCSVBtn->setToolTip(tr("Import node attributes from a CSV file"));

    QPushButton *nodeImportJSONBtn = new QPushButton(tr("Import JSON"), this);
    nodeImportJSONBtn->setToolTip(tr("Import node attributes from a JSON file"));

    QHBoxLayout *nodeTopBar = new QHBoxLayout;
    nodeTopBar->addWidget(m_nodeSearch, 1);
    nodeTopBar->addWidget(nodeRefreshBtn);
    nodeTopBar->addWidget(nodeExportCSVBtn);
    nodeTopBar->addWidget(nodeExportJSONBtn);
    nodeTopBar->addWidget(nodeImportCSVBtn);
    nodeTopBar->addWidget(nodeImportJSONBtn);

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

    QPushButton *edgeExportCSVBtn = new QPushButton(tr("Export CSV"), this);
    edgeExportCSVBtn->setToolTip(tr("Export currently visible rows as CSV"));

    QPushButton *edgeExportJSONBtn = new QPushButton(tr("Export JSON"), this);
    edgeExportJSONBtn->setToolTip(tr("Export currently visible rows as JSON"));

    QPushButton *edgeImportCSVBtn = new QPushButton(tr("Import CSV"), this);
    edgeImportCSVBtn->setToolTip(tr("Import edge attributes from a CSV file"));

    QPushButton *edgeImportJSONBtn = new QPushButton(tr("Import JSON"), this);
    edgeImportJSONBtn->setToolTip(tr("Import edge attributes from a JSON file"));

    QHBoxLayout *edgeTopBar = new QHBoxLayout;
    edgeTopBar->addWidget(m_edgeSearch, 1);
    edgeTopBar->addWidget(edgeRefreshBtn);
    edgeTopBar->addWidget(edgeExportCSVBtn);
    edgeTopBar->addWidget(edgeExportJSONBtn);
    edgeTopBar->addWidget(edgeImportCSVBtn);
    edgeTopBar->addWidget(edgeImportJSONBtn);

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

    // Export buttons
    connect(nodeExportCSVBtn,  &QPushButton::clicked, this, &GraphTableWidget::exportNodesCSV);
    connect(nodeExportJSONBtn, &QPushButton::clicked, this, &GraphTableWidget::exportNodesJSON);
    connect(edgeExportCSVBtn,  &QPushButton::clicked, this, &GraphTableWidget::exportEdgesCSV);
    connect(edgeExportJSONBtn, &QPushButton::clicked, this, &GraphTableWidget::exportEdgesJSON);

    // Import buttons
    connect(nodeImportCSVBtn,  &QPushButton::clicked, this, &GraphTableWidget::importNodesCSV);
    connect(nodeImportJSONBtn, &QPushButton::clicked, this, &GraphTableWidget::importNodesJSON);
    connect(edgeImportCSVBtn,  &QPushButton::clicked, this, &GraphTableWidget::importEdgesCSV);
    connect(edgeImportJSONBtn, &QPushButton::clicked, this, &GraphTableWidget::importEdgesJSON);
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

QAbstractItemModel *GraphTableWidget::nodeModel() const
{
    return m_nodeModel;
}

QAbstractItemModel *GraphTableWidget::edgeModel() const
{
    return m_edgeModel;
}

void GraphTableWidget::exportNodesCSV()
{
    doExport(m_nodeProxy, tr("nodes"), true);
}

void GraphTableWidget::exportNodesJSON()
{
    doExport(m_nodeProxy, tr("nodes"), false);
}

void GraphTableWidget::exportEdgesCSV()
{
    doExport(m_edgeProxy, tr("edges"), true);
}

void GraphTableWidget::exportEdgesJSON()
{
    doExport(m_edgeProxy, tr("edges"), false);
}

/**
 * @brief Opens a save dialog and writes @p proxyModel via TableExport.
 *
 * @p defaultName is used to suggest a filename ("nodes" or "edges").
 * @p csv selects CSV (true) or JSON (false) format.
 */
void GraphTableWidget::doExport(QAbstractItemModel *proxyModel,
                                const QString &defaultName,
                                bool csv)
{
    const QString ext    = csv ? tr("csv")  : tr("json");
    const QString filter = csv ? tr("CSV files (*.csv)") : tr("JSON files (*.json)");

    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export %1 as %2").arg(defaultName.toUpper(), ext.toUpper()),
        defaultName + QLatin1Char('.') + ext,
        filter);

    if (path.isEmpty())
        return;

    const bool ok = csv ? TableExport::toCSV(proxyModel, path)
                        : TableExport::toJSON(proxyModel, path);

    if (ok) {
        emit exportStatusMessage(tr("Exported %1 to %2").arg(defaultName, path));
    } else {
        QMessageBox::warning(this,
                             tr("Export failed"),
                             tr("Could not write to %1").arg(path));
    }
}

void GraphTableWidget::importNodesCSV()  { doImport(true,  true);  }
void GraphTableWidget::importNodesJSON() { doImport(true,  false); }
void GraphTableWidget::importEdgesCSV()  { doImport(false, true);  }
void GraphTableWidget::importEdgesJSON() { doImport(false, false); }

/**
 * @brief Opens DialogImportAttributes and, on acceptance, calls the
 *        appropriate Graph import method then refreshes the table.
 *
 * @p forNodes  true → import node attributes; false → import edge attributes.
 * @p csv       true → CSV format; false → JSON format.
 */
void GraphTableWidget::doImport(bool forNodes, bool csv)
{
    if (!m_graph)
        return;

    const auto scope = forNodes ? DialogImportAttributes::Scope::Nodes
                                : DialogImportAttributes::Scope::Edges;

    DialogImportAttributes dlg(scope, csv, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const TableImport::ParsedTable &table = dlg.parsedTable();

    int matched = 0;
    if (forNodes) {
        matched = m_graph->vertexAttributesImport(
            table.headers, table.rows, dlg.idColumn(), dlg.matchByLabel());
    } else {
        matched = m_graph->edgeAttributesImport(
            table.headers, table.rows, dlg.srcColumn(), dlg.tgtColumn());
    }

    refresh(m_graph);

    const QString kind = forNodes ? tr("node") : tr("edge");
    emit importStatusMessage(
        tr("Imported attributes into %1 %2(s) from %3 rows")
            .arg(matched).arg(kind).arg(table.rows.size()));
}
