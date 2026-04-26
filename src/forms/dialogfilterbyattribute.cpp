/**
 * @file dialogfilterbyattribute.cpp
 * @brief Implements DialogFilterByAttribute.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialogfilterbyattribute.h"
#include "ui_dialogfilterbyattribute.h"

#include <QSet>

DialogFilterByAttribute::DialogFilterByAttribute(const QStringList &nodeKeys,
                                                 const QStringList &edgeKeys,
                                                 QWidget *parent)
    : QDialog(parent),
      ui(new Ui::DialogFilterByAttribute),
      m_nodeKeys(nodeKeys),
      m_edgeKeys(edgeKeys)
{
    ui->setupUi(this);

    repopulateKeys(FilterCondition::Scope::Nodes);

    connect(ui->nodesRadio, &QRadioButton::toggled, this, &DialogFilterByAttribute::onScopeChanged);
    connect(ui->edgesRadio, &QRadioButton::toggled, this, &DialogFilterByAttribute::onScopeChanged);
    connect(ui->bothRadio,  &QRadioButton::toggled, this, &DialogFilterByAttribute::onScopeChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogFilterByAttribute::getUserChoices);

    ui->valueEdit->setFocus();
}

DialogFilterByAttribute::~DialogFilterByAttribute()
{
    delete ui;
}

void DialogFilterByAttribute::onScopeChanged()
{
    FilterCondition::Scope scope = FilterCondition::Scope::Nodes;
    if (ui->edgesRadio->isChecked())
        scope = FilterCondition::Scope::Edges;
    else if (ui->bothRadio->isChecked())
        scope = FilterCondition::Scope::Both;
    repopulateKeys(scope);
}

void DialogFilterByAttribute::repopulateKeys(FilterCondition::Scope scope)
{
    QStringList keys;
    switch (scope) {
    case FilterCondition::Scope::Nodes:
        keys = m_nodeKeys;
        break;
    case FilterCondition::Scope::Edges:
        keys = m_edgeKeys;
        break;
    case FilterCondition::Scope::Both: {
        QSet<QString> merged(m_nodeKeys.cbegin(), m_nodeKeys.cend());
        merged.unite(QSet<QString>(m_edgeKeys.cbegin(), m_edgeKeys.cend()));
        keys = QStringList(merged.cbegin(), merged.cend());
        keys.sort();
        break;
    }
    }

    const QString current = ui->keyCombo->currentText();
    ui->keyCombo->clear();
    ui->keyCombo->addItems(keys);
    // Restore user's typed key if still relevant
    if (!current.isEmpty())
        ui->keyCombo->setCurrentText(current);
}

void DialogFilterByAttribute::getUserChoices()
{
    FilterCondition cond;

    if (ui->edgesRadio->isChecked())
        cond.scope = FilterCondition::Scope::Edges;
    else if (ui->bothRadio->isChecked())
        cond.scope = FilterCondition::Scope::Both;
    else
        cond.scope = FilterCondition::Scope::Nodes;

    cond.key   = ui->keyCombo->currentText().trimmed();
    cond.value = ui->valueEdit->text().trimmed();

    static const FilterCondition::Op opMap[] = {
        FilterCondition::Op::Eq,
        FilterCondition::Op::Neq,
        FilterCondition::Op::Gt,
        FilterCondition::Op::Lt,
        FilterCondition::Op::Gte,
        FilterCondition::Op::Lte,
        FilterCondition::Op::Contains
    };
    const int idx = ui->opCombo->currentIndex();
    cond.op = (idx >= 0 && idx < 7) ? opMap[idx] : FilterCondition::Op::Eq;

    emit userChoices(cond);
}
