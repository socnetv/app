/**
 * @file dialogquerybuilder.cpp
 * @brief Implements DialogQueryBuilder — multi-condition AND filter composer 
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialogquerybuilder.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>


DialogQueryBuilder::DialogQueryBuilder(const QStringList &nodeKeys,
                                       const QStringList &edgeKeys,
                                       QWidget *parent)
    : QDialog(parent)
    , m_nodeKeys(nodeKeys)
    , m_edgeKeys(edgeKeys)
{
    setWindowTitle(tr("Query Builder"));
    setMinimumWidth(500);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(10);

    // Scope group
    QGroupBox *scopeGroup = new QGroupBox(tr("Scope"), this);
    QHBoxLayout *scopeLayout = new QHBoxLayout(scopeGroup);
    m_nodesRadio = new QRadioButton(tr("Nodes"), scopeGroup);
    m_edgesRadio = new QRadioButton(tr("Edges"), scopeGroup);
    m_nodesRadio->setChecked(true);
    scopeLayout->addWidget(m_nodesRadio);
    scopeLayout->addWidget(m_edgesRadio);
    scopeLayout->addStretch();
    main->addWidget(scopeGroup);

    // Conditions group
    QGroupBox *condGroup = new QGroupBox(tr("Conditions  (ALL must match)"), this);
    QVBoxLayout *condOuter = new QVBoxLayout(condGroup);

    m_rowsContainer = new QWidget(condGroup);
    m_rowsLayout = new QVBoxLayout(m_rowsContainer);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(4);

    m_scrollArea = new QScrollArea(condGroup);
    m_scrollArea->setWidget(m_rowsContainer);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setMinimumHeight(130);
    condOuter->addWidget(m_scrollArea);

    m_addRowBtn = new QPushButton(tr("+ Add condition"), condGroup);
    m_addRowBtn->setFlat(true);
    condOuter->addWidget(m_addRowBtn, 0, Qt::AlignLeft);
    main->addWidget(condGroup);

    // Buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    main->addWidget(m_buttonBox);

    // Start with one empty row.
    addRow();

    connect(m_nodesRadio, &QRadioButton::toggled, this, &DialogQueryBuilder::onScopeChanged);
    connect(m_edgesRadio, &QRadioButton::toggled, this, &DialogQueryBuilder::onScopeChanged);
    connect(m_addRowBtn,  &QPushButton::clicked,  this, &DialogQueryBuilder::onAddRow);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &DialogQueryBuilder::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void DialogQueryBuilder::addRow()
{
    ConditionRow cr;

    cr.widget = new QWidget(m_rowsContainer);
    QHBoxLayout *rowLay = new QHBoxLayout(cr.widget);
    rowLay->setContentsMargins(0, 0, 0, 0);
    rowLay->setSpacing(4);

    cr.keyCombo = new QComboBox(cr.widget);
    cr.keyCombo->setEditable(true);
    cr.keyCombo->setMinimumWidth(120);
    cr.keyCombo->addItems(m_nodesRadio->isChecked() ? m_nodeKeys : m_edgeKeys);

    cr.opCombo = new QComboBox(cr.widget);
    cr.opCombo->addItems({tr("="), tr("≠"), tr(">"), tr("<"), tr("≥"), tr("≤"), tr("contains")});

    cr.valueEdit = new QLineEdit(cr.widget);
    cr.valueEdit->setPlaceholderText(tr("value…"));

    cr.removeBtn = new QPushButton(tr("−"), cr.widget);
    cr.removeBtn->setMaximumWidth(28);
    cr.removeBtn->setFlat(true);
    cr.removeBtn->setToolTip(tr("Remove this condition"));

    rowLay->addWidget(cr.keyCombo,  2);
    rowLay->addWidget(cr.opCombo,   1);
    rowLay->addWidget(cr.valueEdit, 2);
    rowLay->addWidget(cr.removeBtn, 0);

    m_rowsLayout->addWidget(cr.widget);
    m_rows.append(cr);

    connect(cr.removeBtn, &QPushButton::clicked, this, [this, w = cr.widget]() {
        removeRow(w);
    });

    updateRemoveButtons();
}

void DialogQueryBuilder::removeRow(QWidget *rowWidget)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].widget == rowWidget) {
            m_rowsLayout->removeWidget(rowWidget);
            rowWidget->deleteLater();
            m_rows.removeAt(i);
            break;
        }
    }
    updateRemoveButtons();
}

void DialogQueryBuilder::onScopeChanged()
{
    repopulateKeys();
}

void DialogQueryBuilder::repopulateKeys()
{
    const QStringList keys = m_edgesRadio->isChecked() ? m_edgeKeys : m_nodeKeys;
    for (auto &cr : m_rows) {
        const QString current = cr.keyCombo->currentText();
        cr.keyCombo->clear();
        cr.keyCombo->addItems(keys);
        if (!current.isEmpty())
            cr.keyCombo->setCurrentText(current);
    }
}

void DialogQueryBuilder::updateRemoveButtons()
{
    const bool canRemove = m_rows.size() > 1;
    for (auto &cr : m_rows)
        cr.removeBtn->setEnabled(canRemove);
}

FilterCondition::Scope DialogQueryBuilder::currentScope() const
{
    return m_edgesRadio->isChecked() ? FilterCondition::Scope::Edges
                                     : FilterCondition::Scope::Nodes;
}

void DialogQueryBuilder::onAddRow()
{
    addRow();
}

void DialogQueryBuilder::onApply()
{
    static const FilterCondition::Op opMap[] = {
        FilterCondition::Op::Eq,
        FilterCondition::Op::Neq,
        FilterCondition::Op::Gt,
        FilterCondition::Op::Lt,
        FilterCondition::Op::Gte,
        FilterCondition::Op::Lte,
        FilterCondition::Op::Contains
    };

    GraphQuery query;
    const FilterCondition::Scope scope = currentScope();

    for (const auto &cr : std::as_const(m_rows)) {
        const QString key   = cr.keyCombo->currentText().trimmed();
        const QString value = cr.valueEdit->text().trimmed();
        if (key.isEmpty())
            continue;

        FilterCondition cond;
        cond.scope = scope;
        cond.key   = key;
        cond.value = value;
        const int opIdx = cr.opCombo->currentIndex();
        cond.op = (opIdx >= 0 && opIdx < 7) ? opMap[opIdx] : FilterCondition::Op::Eq;
        query.conditions.append(cond);
    }

    if (query.conditions.isEmpty())
        return;

    emit userChoices(query);
}
