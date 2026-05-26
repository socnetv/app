/**
 * @file dialogquerybuilder.h
 * @brief Declares DialogQueryBuilder — multi-condition AND filter composer.
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
#include <QList>
#include <QStringList>
#include "../graph/filters/filter_condition.h"
#include "../graph/filters/graph_query.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QRadioButton;
class QDialogButtonBox;

/**
 * @brief Visual query builder that composes a multi-condition AND filter.
 *
 * The user picks a scope (Nodes or Edges), then adds one or more condition rows
 * (key / operator / value). Clicking Apply emits userChoices(GraphQuery) with
 * all conditions carrying the chosen scope. The caller applies the filter and
 * adds a single chip to the bar.
 *
 * Implemented in pure C++ (no .ui file) so condition rows can be added and
 * removed dynamically without UIC constraints.
 */
class DialogQueryBuilder : public QDialog
{
    Q_OBJECT
public:
    explicit DialogQueryBuilder(const QStringList &nodeKeys,
                                const QStringList &edgeKeys,
                                QWidget *parent = nullptr);

signals:
    void userChoices(const GraphQuery &query);

private slots:
    void onScopeChanged();
    void onAddRow();
    void onApply();

private:
    struct ConditionRow {
        QWidget     *widget   = nullptr;
        QComboBox   *keyCombo = nullptr;
        QComboBox   *opCombo  = nullptr;
        QLineEdit   *valueEdit= nullptr;
        QPushButton *removeBtn= nullptr;
    };

    QStringList m_nodeKeys;
    QStringList m_edgeKeys;

    QRadioButton    *m_nodesRadio;
    QRadioButton    *m_edgesRadio;
    QVBoxLayout     *m_rowsLayout;
    QWidget         *m_rowsContainer;
    QScrollArea     *m_scrollArea;
    QPushButton     *m_addRowBtn;
    QDialogButtonBox*m_buttonBox;
    QList<ConditionRow> m_rows;

    void addRow();
    void removeRow(QWidget *rowWidget);
    void repopulateKeys();
    void updateRemoveButtons();
    FilterCondition::Scope currentScope() const;
};
