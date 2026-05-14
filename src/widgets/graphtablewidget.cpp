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
#include "../forms/dialogbulkedit.h"
#include "../graph.h"

#include <QTabWidget>
#include <QTableView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QFrame>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QItemSelection>
#include <QSet>

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
        view->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    m_nodeSearch->setMaximumWidth(140);

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

    QPushButton *nodeSetPropBtn = new QPushButton(tr("Set property..."), this);
    nodeSetPropBtn->setToolTip(
        tr("Set a single property on the selected rows (or all visible rows if none are selected). "
           "Applies to built-in fields (Label, Size, Color, Shape) and custom attributes."));

    QPushButton *nodeRemoveAttrBtn = new QPushButton(tr("Remove attribute..."), this);
    nodeRemoveAttrBtn->setToolTip(
        tr("Remove a custom attribute key from the selected rows (or all visible rows if none are selected)."));

    QPushButton *nodeAddAttrBtn = new QPushButton(tr("Add attribute..."), this);
    nodeAddAttrBtn->setToolTip(
        tr("Add a new custom attribute key/value to the selected rows (or all visible rows if none are selected)."));

    // VLine: Search | Refresh
    QFrame *nodeSep1 = new QFrame(this);
    nodeSep1->setFrameShape(QFrame::VLine);
    nodeSep1->setFrameShadow(QFrame::Sunken);

    // VLine: Refresh | Export/Import
    QFrame *nodeSep2 = new QFrame(this);
    nodeSep2->setFrameShape(QFrame::VLine);
    nodeSep2->setFrameShadow(QFrame::Sunken);

    // VLine: Export/Import | Bulk-edit
    QFrame *nodeSep3 = new QFrame(this);
    nodeSep3->setFrameShape(QFrame::VLine);
    nodeSep3->setFrameShadow(QFrame::Sunken);

    // VLine: Set property | Remove/Add attribute
    QFrame *nodeSep4 = new QFrame(this);
    nodeSep4->setFrameShape(QFrame::VLine);
    nodeSep4->setFrameShadow(QFrame::Sunken);

    QHBoxLayout *nodeTopBar = new QHBoxLayout;
    nodeTopBar->addWidget(m_nodeSearch);
    nodeTopBar->addWidget(nodeSep1);
    nodeTopBar->addWidget(nodeRefreshBtn);
    nodeTopBar->addWidget(nodeSep2);
    nodeTopBar->addWidget(nodeExportCSVBtn);
    nodeTopBar->addWidget(nodeExportJSONBtn);
    nodeTopBar->addWidget(nodeImportCSVBtn);
    nodeTopBar->addWidget(nodeImportJSONBtn);
    nodeTopBar->addWidget(nodeSep3);
    nodeTopBar->addWidget(nodeSetPropBtn);
    nodeTopBar->addWidget(nodeSep4);
    nodeTopBar->addWidget(nodeRemoveAttrBtn);
    nodeTopBar->addWidget(nodeAddAttrBtn);
    nodeTopBar->addStretch();

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
    m_edgeSearch->setMaximumWidth(140);

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

    QPushButton *edgeSetPropBtn = new QPushButton(tr("Set property..."), this);
    edgeSetPropBtn->setToolTip(
        tr("Set a single property on the selected rows (or all visible rows if none are selected). "
           "Applies to built-in fields (Label, Weight, Color) and custom attributes."));

    QPushButton *edgeRemoveAttrBtn = new QPushButton(tr("Remove attribute..."), this);
    edgeRemoveAttrBtn->setToolTip(
        tr("Remove a custom attribute key from the selected rows (or all visible rows if none are selected)."));

    QPushButton *edgeAddAttrBtn = new QPushButton(tr("Add attribute..."), this);
    edgeAddAttrBtn->setToolTip(
        tr("Add a new custom attribute key/value to the selected rows (or all visible rows if none are selected)."));

    // VLine: Search | Refresh
    QFrame *edgeSep1 = new QFrame(this);
    edgeSep1->setFrameShape(QFrame::VLine);
    edgeSep1->setFrameShadow(QFrame::Sunken);

    // VLine: Refresh | Export/Import
    QFrame *edgeSep2 = new QFrame(this);
    edgeSep2->setFrameShape(QFrame::VLine);
    edgeSep2->setFrameShadow(QFrame::Sunken);

    // VLine: Export/Import | Bulk-edit
    QFrame *edgeSep3 = new QFrame(this);
    edgeSep3->setFrameShape(QFrame::VLine);
    edgeSep3->setFrameShadow(QFrame::Sunken);

    // VLine: Set property | Remove/Add attribute
    QFrame *edgeSep4 = new QFrame(this);
    edgeSep4->setFrameShape(QFrame::VLine);
    edgeSep4->setFrameShadow(QFrame::Sunken);

    QHBoxLayout *edgeTopBar = new QHBoxLayout;
    edgeTopBar->addWidget(m_edgeSearch);
    edgeTopBar->addWidget(edgeSep1);
    edgeTopBar->addWidget(edgeRefreshBtn);
    edgeTopBar->addWidget(edgeSep2);
    edgeTopBar->addWidget(edgeExportCSVBtn);
    edgeTopBar->addWidget(edgeExportJSONBtn);
    edgeTopBar->addWidget(edgeImportCSVBtn);
    edgeTopBar->addWidget(edgeImportJSONBtn);
    edgeTopBar->addWidget(edgeSep3);
    edgeTopBar->addWidget(edgeSetPropBtn);
    edgeTopBar->addWidget(edgeSep4);
    edgeTopBar->addWidget(edgeRemoveAttrBtn);
    edgeTopBar->addWidget(edgeAddAttrBtn);
    edgeTopBar->addStretch();

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

    // Row click → selection signals
    connect(m_nodeView, &QTableView::clicked,
            this, &GraphTableWidget::onNodeRowClicked);
    connect(m_edgeView, &QTableView::clicked,
            this, &GraphTableWidget::onEdgeRowClicked);

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

    // Bulk-edit buttons
    connect(nodeSetPropBtn,    &QPushButton::clicked, this, &GraphTableWidget::onNodeSetPropertyClicked);
    connect(nodeRemoveAttrBtn, &QPushButton::clicked, this, &GraphTableWidget::onNodeRemoveAttributeClicked);
    connect(nodeAddAttrBtn,    &QPushButton::clicked, this, &GraphTableWidget::onNodeAddAttributeClicked);
    connect(edgeSetPropBtn,    &QPushButton::clicked, this, &GraphTableWidget::onEdgeSetPropertyClicked);
    connect(edgeRemoveAttrBtn, &QPushButton::clicked, this, &GraphTableWidget::onEdgeRemoveAttributeClicked);
    connect(edgeAddAttrBtn,    &QPushButton::clicked, this, &GraphTableWidget::onEdgeAddAttributeClicked);
}

/**
 * @brief Repopulates both models from @p graph and resizes columns.
 *
 * Also re-syncs the table selection from the graph's current selection state,
 * because beginResetModel/endResetModel clears all view selections.
 */
void GraphTableWidget::refresh(Graph *graph)
{
    m_graph = graph;

    m_nodeModel->populate(graph);
    m_edgeModel->populate(graph);

    m_nodeView->resizeColumnsToContents();
    m_edgeView->resizeColumnsToContents();

    // Re-apply graph selection so resolveNodeTargets/resolveEdgeTargets are correct
    syncNodeSelection(graph->getSelectedVertices());
    syncEdgeSelection(graph->getSelectedEdges());
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

/**
 * @brief Maps the proxy index to source/target columns and emits edgeSelected().
 */
void GraphTableWidget::onEdgeRowClicked(const QModelIndex &proxyIndex)
{
    const QModelIndex src0 = m_edgeProxy->mapToSource(proxyIndex);
    if (!src0.isValid())
        return;

    const QModelIndex src1 = m_edgeModel->index(src0.row(), 1);
    const int s = m_edgeModel->data(src0, Qt::DisplayRole).toInt();
    const int t = m_edgeModel->data(src1, Qt::DisplayRole).toInt();
    emit edgeSelected(s, t);
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

void GraphTableWidget::setNodeShapeLists(const QStringList &shapes,
                                         const QStringList &icons)
{
    m_nodeShapeList = shapes;
    m_iconPathList  = icons;
}

void GraphTableWidget::showNodesTab() { m_tabs->setCurrentIndex(0); }
void GraphTableWidget::showEdgesTab() { m_tabs->setCurrentIndex(1); }

/**
 * @brief Pre-selects the rows in the Nodes tab whose node number is in @p nodeNumbers.
 *
 * Rows filtered out by the current search are silently skipped.
 * Selection is cleared first; unmatched rows are left unselected.
 */
void GraphTableWidget::syncNodeSelection(const QList<int> &nodeNumbers)
{
    if (nodeNumbers.isEmpty()) {
        m_nodeView->clearSelection();
        return;
    }

    // Build number → source-row index map  O(model_rows)
    QHash<int, int> numToRow;
    numToRow.reserve(m_nodeModel->rowCount());
    for (int row = 0; row < m_nodeModel->rowCount(); ++row) {
        const int num = m_nodeModel->data(
            m_nodeModel->index(row, 0), Qt::DisplayRole).toInt();
        numToRow[num] = row;
    }

    // Map selection to proxy indices  O(selected)
    QItemSelection sel;
    const int lastCol = m_nodeProxy->columnCount() - 1;
    for (const int num : nodeNumbers) {
        auto it = numToRow.find(num);
        if (it == numToRow.end()) continue;
        const QModelIndex srcIdx   = m_nodeModel->index(it.value(), 0);
        const QModelIndex proxyIdx = m_nodeProxy->mapFromSource(srcIdx);
        if (!proxyIdx.isValid()) continue;  // filtered out
        sel.select(proxyIdx, m_nodeProxy->index(proxyIdx.row(), lastCol));
    }

    m_nodeView->selectionModel()->select(
        sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    if (!sel.isEmpty())
        m_nodeView->scrollTo(sel.indexes().constFirst());
}

/**
 * @brief Pre-selects the rows in the Edges tab matching @p edges (source, target pairs).
 */
void GraphTableWidget::syncEdgeSelection(const QList<SocNetV::SelectedEdge> &edges)
{
    if (edges.isEmpty()) {
        m_edgeView->clearSelection();
        return;
    }

    // Build (source, target) → source-row map  O(model_rows)
    QHash<SelectedEdge, int> edgeToRow;
    edgeToRow.reserve(m_edgeModel->rowCount());
    for (int row = 0; row < m_edgeModel->rowCount(); ++row) {
        const int src = m_edgeModel->data(m_edgeModel->index(row, 0), Qt::DisplayRole).toInt();
        const int tgt = m_edgeModel->data(m_edgeModel->index(row, 1), Qt::DisplayRole).toInt();
        edgeToRow[qMakePair(src, tgt)] = row;
    }

    QItemSelection sel;
    const int lastCol = m_edgeProxy->columnCount() - 1;
    for (const SocNetV::SelectedEdge &e : edges) {
        auto it = edgeToRow.find(e);
        if (it == edgeToRow.end()) continue;
        const QModelIndex srcIdx   = m_edgeModel->index(it.value(), 0);
        const QModelIndex proxyIdx = m_edgeProxy->mapFromSource(srcIdx);
        if (!proxyIdx.isValid()) continue;
        sel.select(proxyIdx, m_edgeProxy->index(proxyIdx.row(), lastCol));
    }

    m_edgeView->selectionModel()->select(
        sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    if (!sel.isEmpty())
        m_edgeView->scrollTo(sel.indexes().constFirst());
}

/**
 * @brief Returns the node numbers for the table's current node selection,
 *        or for all visible proxy rows if nothing is selected.
 */
QList<int> GraphTableWidget::resolveNodeTargets() const
{
    QList<int> result;
    const QModelIndexList selected =
        m_nodeView->selectionModel()->selectedRows(0);

    if (!selected.isEmpty()) {
        for (const QModelIndex &proxyIdx : selected) {
            const QModelIndex src = m_nodeProxy->mapToSource(proxyIdx);
            result << m_nodeModel->data(src, Qt::DisplayRole).toInt();
        }
    } else {
        // Fall back to all visible (proxy) rows
        for (int row = 0; row < m_nodeProxy->rowCount(); ++row) {
            const QModelIndex proxyIdx = m_nodeProxy->index(row, 0);
            const QModelIndex src      = m_nodeProxy->mapToSource(proxyIdx);
            result << m_nodeModel->data(src, Qt::DisplayRole).toInt();
        }
    }
    return result;
}

/**
 * @brief Returns (source, target) pairs for the table's current edge selection,
 *        or all visible proxy rows if nothing is selected.
 */
QList<SocNetV::SelectedEdge> GraphTableWidget::resolveEdgeTargets() const
{
    QList<SocNetV::SelectedEdge> result;
    const QModelIndexList selected =
        m_edgeView->selectionModel()->selectedRows(0);

    auto rowToPair = [this](const QModelIndex &proxyIdx) -> SelectedEdge {
        const QModelIndex src0 = m_edgeProxy->mapToSource(proxyIdx);
        const QModelIndex src1 = m_edgeProxy->mapToSource(
            m_edgeProxy->index(proxyIdx.row(), 1));
        const int s = m_edgeModel->data(src0, Qt::DisplayRole).toInt();
        const int t = m_edgeModel->data(src1, Qt::DisplayRole).toInt();
        return qMakePair(s, t);
    };

    if (!selected.isEmpty()) {
        for (const QModelIndex &idx : selected)
            result << rowToPair(idx);
    } else {
        for (int row = 0; row < m_edgeProxy->rowCount(); ++row)
            result << rowToPair(m_edgeProxy->index(row, 0));
    }
    return result;
}

void GraphTableWidget::onNodeSetPropertyClicked()
{
    if (!m_graph) return;

    const QList<int> targets = resolveNodeTargets();
    if (targets.isEmpty()) return;

    const bool isFilter = m_nodeView->selectionModel()->selectedRows().isEmpty();

    // Collect unique custom attribute keys across targets
    QSet<QString> keySet;
    for (const int v : targets)
        for (const QString &k : m_graph->vertexCustomAttributes(v).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    DialogBulkEdit dlg(DialogBulkEdit::Scope::Nodes, existingKeys,
                       m_nodeShapeList, m_iconPathList,
                       targets.size(), isFilter, this);

    connect(&dlg, &DialogBulkEdit::userChoices,
            this, [this, targets](const QString &property, const QString &value) {
        for (const int v : targets) {
            if (property == QLatin1String("Label")) {
                m_graph->vertexLabelSet(v, value);
            } else if (property == QLatin1String("Size")) {
                m_graph->vertexSizeSet(v, value.toInt());
            } else if (property == QLatin1String("Color")) {
                m_graph->vertexColorSet(v, value);
            } else if (property == QLatin1String("Shape")) {
                const int idx = m_nodeShapeList.indexOf(value);
                const QString icon = (idx >= 0 && idx < m_iconPathList.size())
                                         ? m_iconPathList[idx] : QString();
                m_graph->vertexShapeSet(v, value, icon);
            } else {
                m_graph->vertexCustomAttributeSet(v, property, value);
            }
        }
        refresh(m_graph);
        emit exportStatusMessage(
            tr("Set '%1' on %2 node(s).").arg(property).arg(targets.size()));
    });

    dlg.exec();
}

void GraphTableWidget::onNodeRemoveAttributeClicked()
{
    if (!m_graph) return;

    const QList<int> targets = resolveNodeTargets();
    if (targets.isEmpty()) return;

    // Collect custom attribute keys present in targets
    QSet<QString> keySet;
    for (const int v : targets)
        for (const QString &k : m_graph->vertexCustomAttributes(v).keys())
            keySet.insert(k);

    if (keySet.isEmpty()) {
        QMessageBox::information(this, tr("Remove Attribute"),
                                 tr("No custom attributes found on the selected nodes."));
        return;
    }

    const QStringList keys(keySet.begin(), keySet.end());
    bool ok = false;
    const QString key = QInputDialog::getItem(
        this, tr("Remove Attribute"),
        tr("Select attribute key to remove from %1 node(s):").arg(targets.size()),
        keys, 0, false, &ok);

    if (!ok || key.isEmpty()) return;

    for (const int v : targets)
        m_graph->vertexCustomAttributeRemove(v, key);

    refresh(m_graph);
    emit exportStatusMessage(
        tr("Removed attribute '%1' from %2 node(s).").arg(key).arg(targets.size()));
}

void GraphTableWidget::onNodeAddAttributeClicked()
{
    if (!m_graph) return;

    const QList<int> targets = resolveNodeTargets();
    if (targets.isEmpty()) return;

    bool ok = false;
    const QString key = QInputDialog::getText(
        this, tr("Add Attribute"),
        tr("Attribute name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || key.trimmed().isEmpty()) return;

    const QString value = QInputDialog::getText(
        this, tr("Add Attribute"),
        tr("Value for '%1':").arg(key.trimmed()), QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    const QString trimmedKey = key.trimmed();
    for (const int v : targets)
        m_graph->vertexCustomAttributeSet(v, trimmedKey, value);

    refresh(m_graph);
    emit exportStatusMessage(
        tr("Added attribute '%1' to %2 node(s).").arg(trimmedKey).arg(targets.size()));
}

void GraphTableWidget::onEdgeSetPropertyClicked()
{
    if (!m_graph) return;

    const QList<SocNetV::SelectedEdge> targets = resolveEdgeTargets();
    if (targets.isEmpty()) return;

    const bool isFilter = m_edgeView->selectionModel()->selectedRows().isEmpty();

    QSet<QString> keySet;
    for (const SocNetV::SelectedEdge &e : targets)
        for (const QString &k : m_graph->edgeCustomAttributes(e.first, e.second).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    DialogBulkEdit dlg(DialogBulkEdit::Scope::Edges, existingKeys,
                       QStringList(), QStringList(),
                       targets.size(), isFilter, this);

    connect(&dlg, &DialogBulkEdit::userChoices,
            this, [this, targets](const QString &property, const QString &value) {
        for (const SocNetV::SelectedEdge &e : targets) {
            if (property == QLatin1String("Label")) {
                m_graph->edgeLabelSet(e.first, e.second, value);
            } else if (property == QLatin1String("Weight")) {
                m_graph->edgeWeightSet(e.first, e.second, value.toDouble());
            } else if (property == QLatin1String("Color")) {
                m_graph->edgeColorSet(e.first, e.second, value);
            } else {
                m_graph->edgeCustomAttributesSet(e.first, e.second, {{property, value}});
            }
        }
        refresh(m_graph);
        emit exportStatusMessage(
            tr("Set '%1' on %2 edge(s).").arg(property).arg(targets.size()));
    });

    dlg.exec();
}

void GraphTableWidget::onEdgeRemoveAttributeClicked()
{
    if (!m_graph) return;

    const QList<SocNetV::SelectedEdge> targets = resolveEdgeTargets();
    if (targets.isEmpty()) return;

    QSet<QString> keySet;
    for (const SocNetV::SelectedEdge &e : targets)
        for (const QString &k : m_graph->edgeCustomAttributes(e.first, e.second).keys())
            keySet.insert(k);

    if (keySet.isEmpty()) {
        QMessageBox::information(this, tr("Remove Attribute"),
                                 tr("No custom attributes found on the selected edges."));
        return;
    }

    const QStringList keys(keySet.begin(), keySet.end());
    bool ok = false;
    const QString key = QInputDialog::getItem(
        this, tr("Remove Attribute"),
        tr("Select attribute key to remove from %1 edge(s):").arg(targets.size()),
        keys, 0, false, &ok);

    if (!ok || key.isEmpty()) return;

    for (const SocNetV::SelectedEdge &e : targets) {
        QHash<QString, QString> attrs = m_graph->edgeCustomAttributes(e.first, e.second);
        attrs.remove(key);
        m_graph->edgeCustomAttributesSet(e.first, e.second, attrs);
    }

    refresh(m_graph);
    emit exportStatusMessage(
        tr("Removed attribute '%1' from %2 edge(s).").arg(key).arg(targets.size()));
}

void GraphTableWidget::onEdgeAddAttributeClicked()
{
    if (!m_graph) return;

    const QList<SocNetV::SelectedEdge> targets = resolveEdgeTargets();
    if (targets.isEmpty()) return;

    bool ok = false;
    const QString key = QInputDialog::getText(
        this, tr("Add Attribute"),
        tr("Attribute name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || key.trimmed().isEmpty()) return;

    const QString value = QInputDialog::getText(
        this, tr("Add Attribute"),
        tr("Value for '%1':").arg(key.trimmed()), QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    const QString trimmedKey = key.trimmed();
    for (const SocNetV::SelectedEdge &e : targets)
        m_graph->edgeCustomAttributesSet(e.first, e.second, {{trimmedKey, value}});

    refresh(m_graph);
    emit exportStatusMessage(
        tr("Added attribute '%1' to %2 edge(s).").arg(trimmedKey).arg(targets.size()));
}

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
