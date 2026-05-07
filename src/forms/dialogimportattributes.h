/**
 * @file dialogimportattributes.h
 * @brief Declares DialogImportAttributes — column-mapping dialog for CSV/JSON
 *        attribute import (#227).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QDialog>
#include "../graph/io/table_import.h"

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QRadioButton;
class QTableWidget;

/**
 * @brief Modal dialog that lets the user pick a CSV or JSON file and map its
 *        columns to graph node/edge identifiers before importing attributes.
 *
 * Usage:
 * @code
 *   DialogImportAttributes dlg(DialogImportAttributes::Scope::Nodes,
 *                              true,   // isCSV
 *                              parent);
 *   if (dlg.exec() == QDialog::Accepted) {
 *       graph->vertexAttributesImport(dlg.parsedTable().headers,
 *                                     dlg.parsedTable().rows,
 *                                     dlg.idColumn(),
 *                                     dlg.matchByLabel());
 *   }
 * @endcode
 */
class DialogImportAttributes : public QDialog
{
    Q_OBJECT
public:
    enum class Scope { Nodes, Edges };

    explicit DialogImportAttributes(Scope scope, bool isCSV, QWidget *parent = nullptr);

    /** Parsed file contents — valid only after the dialog is accepted. */
    const TableImport::ParsedTable &parsedTable() const { return m_table; }

    /**
     * @name Nodes mapping results
     * Valid when scope == Nodes and the dialog was accepted.
     */
    ///@{
    int  idColumn()      const;  ///< Index of the node-ID column in parsedTable().headers
    bool matchByLabel()  const;  ///< True → match by label; false → match by node number
    ///@}

    /**
     * @name Edges mapping results
     * Valid when scope == Edges and the dialog was accepted.
     */
    ///@{
    int srcColumn() const;  ///< Index of the source-node column
    int tgtColumn() const;  ///< Index of the target-node column
    ///@}

private slots:
    void onBrowse();
    void onAccepted();

private:
    void loadFile(const QString &path);
    void populatePreview();
    void populateColumnCombos();

    const Scope  m_scope;
    const bool   m_isCSV;

    TableImport::ParsedTable m_table;

    // File selection
    QLabel      *m_fileLabel;

    // Preview
    QTableWidget *m_previewTable;

    // Nodes mapping
    QComboBox    *m_idCombo       = nullptr;
    QRadioButton *m_byNumberRadio = nullptr;
    QRadioButton *m_byLabelRadio  = nullptr;

    // Edges mapping
    QComboBox    *m_srcCombo      = nullptr;
    QComboBox    *m_tgtCombo      = nullptr;

    QDialogButtonBox *m_buttonBox;
};
