/**
 * @file dialogimportattributes.cpp
 * @brief Implements DialogImportAttributes (#227).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialogimportattributes.h"
#include "../graph/io/table_import.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>

static constexpr int PREVIEW_ROWS = 8;

DialogImportAttributes::DialogImportAttributes(Scope scope, bool isCSV, QWidget *parent)
    : QDialog(parent), m_scope(scope), m_isCSV(isCSV)
{
    const QString scopeStr  = (scope == Scope::Nodes) ? tr("Node") : tr("Edge");
    const QString formatStr = isCSV ? tr("CSV") : tr("JSON");

    setWindowTitle(tr("Import %1 Attributes from %2").arg(scopeStr, formatStr));
    setMinimumWidth(560);

    // -----------------------------------------------------------------------
    // File selection row
    // -----------------------------------------------------------------------
    m_fileLabel = new QLabel(tr("No file selected"), this);
    m_fileLabel->setWordWrap(false);
    m_fileLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QPushButton *browseBtn = new QPushButton(tr("Browse…"), this);
    browseBtn->setToolTip(tr("Choose a %1 file to import").arg(formatStr));

    QHBoxLayout *fileRow = new QHBoxLayout;
    fileRow->addWidget(m_fileLabel, 1);
    fileRow->addWidget(browseBtn);

    QGroupBox *fileGroup = new QGroupBox(tr("File"), this);
    fileGroup->setLayout(fileRow);

    // -----------------------------------------------------------------------
    // Preview table (read-only, up to PREVIEW_ROWS rows)
    // -----------------------------------------------------------------------
    m_previewTable = new QTableWidget(0, 0, this);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_previewTable->setAlternatingRowColors(true);
    m_previewTable->setMinimumHeight(160);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->verticalHeader()->hide();

    QGroupBox *previewGroup = new QGroupBox(tr("Preview (first %1 rows)").arg(PREVIEW_ROWS), this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    previewLayout->addWidget(m_previewTable);

    // -----------------------------------------------------------------------
    // Column mapping — Nodes or Edges
    // -----------------------------------------------------------------------
    QGroupBox *mappingGroup = new QGroupBox(tr("Column mapping"), this);
    QFormLayout *mappingForm = new QFormLayout(mappingGroup);

    if (scope == Scope::Nodes) {
        m_idCombo = new QComboBox(this);
        m_idCombo->setToolTip(tr("Column used to identify each node"));

        m_byNumberRadio = new QRadioButton(tr("Node number"), this);
        m_byLabelRadio  = new QRadioButton(tr("Node label"),  this);
        m_byNumberRadio->setChecked(true);

        QHBoxLayout *matchRow = new QHBoxLayout;
        matchRow->addWidget(m_byNumberRadio);
        matchRow->addWidget(m_byLabelRadio);
        matchRow->addStretch();

        mappingForm->addRow(tr("ID column:"),  m_idCombo);
        mappingForm->addRow(tr("Match by:"),   matchRow);

        QLabel *hint = new QLabel(
            tr("All other columns will be imported as node attributes."), this);
        hint->setWordWrap(true);
        mappingForm->addRow(hint);
    } else {
        m_srcCombo = new QComboBox(this);
        m_srcCombo->setToolTip(tr("Column containing the source node number"));
        m_tgtCombo = new QComboBox(this);
        m_tgtCombo->setToolTip(tr("Column containing the target node number"));

        mappingForm->addRow(tr("Source column:"), m_srcCombo);
        mappingForm->addRow(tr("Target column:"), m_tgtCombo);

        QLabel *hint = new QLabel(
            tr("All other columns will be imported as edge attributes."), this);
        hint->setWordWrap(true);
        mappingForm->addRow(hint);
    }

    // -----------------------------------------------------------------------
    // Button box — Import disabled until a valid file is loaded
    // -----------------------------------------------------------------------
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    QVBoxLayout *main = new QVBoxLayout(this);
    main->addWidget(fileGroup);
    main->addWidget(previewGroup, 1);
    main->addWidget(mappingGroup);
    main->addWidget(m_buttonBox);

    // -----------------------------------------------------------------------
    // Connections
    // -----------------------------------------------------------------------
    connect(browseBtn,   &QPushButton::clicked,
            this,        &DialogImportAttributes::onBrowse);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this,        &DialogImportAttributes::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this,        &QDialog::reject);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void DialogImportAttributes::onBrowse()
{
    const QString filter = m_isCSV
        ? tr("CSV files (*.csv *.txt);;All files (*)")
        : tr("JSON files (*.json);;All files (*)");

    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open %1 file").arg(m_isCSV ? tr("CSV") : tr("JSON")),
        QString(),
        filter);

    if (path.isEmpty())
        return;

    loadFile(path);
}

void DialogImportAttributes::onAccepted()
{
    accept();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void DialogImportAttributes::loadFile(const QString &path)
{
    m_table = m_isCSV ? TableImport::fromCSV(path)
                      : TableImport::fromJSON(path);

    if (!m_table.ok) {
        QMessageBox::warning(this, tr("Import error"), m_table.errorString);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    // Show truncated path in the label
    const QFileInfo fi(path);
    m_fileLabel->setText(fi.fileName());
    m_fileLabel->setToolTip(path);

    populatePreview();
    populateColumnCombos();

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DialogImportAttributes::populatePreview()
{
    const int colCount = m_table.headers.size();
    const int rowCount = qMin(m_table.rows.size(), PREVIEW_ROWS);

    m_previewTable->setColumnCount(colCount);
    m_previewTable->setRowCount(rowCount);
    m_previewTable->setHorizontalHeaderLabels(m_table.headers);

    for (int r = 0; r < rowCount; ++r) {
        const QStringList &row = m_table.rows.at(r);
        for (int c = 0; c < colCount; ++c) {
            const QString val = (c < row.size()) ? row.at(c) : QString();
            m_previewTable->setItem(r, c, new QTableWidgetItem(val));
        }
    }
    m_previewTable->resizeColumnsToContents();
}

void DialogImportAttributes::populateColumnCombos()
{
    if (m_scope == Scope::Nodes) {
        m_idCombo->clear();
        m_idCombo->addItems(m_table.headers);
    } else {
        m_srcCombo->clear();
        m_srcCombo->addItems(m_table.headers);
        m_tgtCombo->clear();
        m_tgtCombo->addItems(m_table.headers);

        // Auto-select sensible defaults if column names look right
        const QStringList &h = m_table.headers;
        for (int i = 0; i < h.size(); ++i) {
            const QString lower = h.at(i).toLower();
            if (lower == QLatin1String("source") || lower == QLatin1String("src"))
                m_srcCombo->setCurrentIndex(i);
            if (lower == QLatin1String("target") || lower == QLatin1String("tgt")
                || lower == QLatin1String("dest"))
                m_tgtCombo->setCurrentIndex(i);
        }
    }
}

// ---------------------------------------------------------------------------
// Result accessors
// ---------------------------------------------------------------------------

int DialogImportAttributes::idColumn() const
{
    return m_idCombo ? m_idCombo->currentIndex() : 0;
}

bool DialogImportAttributes::matchByLabel() const
{
    return m_byLabelRadio && m_byLabelRadio->isChecked();
}

int DialogImportAttributes::srcColumn() const
{
    return m_srcCombo ? m_srcCombo->currentIndex() : 0;
}

int DialogImportAttributes::tgtColumn() const
{
    return m_tgtCombo ? m_tgtCombo->currentIndex() : 1;
}
