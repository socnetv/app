/**
 * @file dialogbulkedit.cpp
 * @brief Implements DialogBulkEdit for applying a single property value to multiple nodes or edges.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialogbulkedit.h"
#include "ui_dialogbulkedit.h"
#include "global.h"

#include <QDebug>
#include <QColorDialog>
#include <QPushButton>
#include <QToolButton>

SOCNETV_USE_NAMESPACE

DialogBulkEdit::DialogBulkEdit(Scope scope,
                               const QStringList &existingKeys,
                               const QStringList &nodeShapeList,
                               const QStringList &iconPathList,
                               int targetCount,
                               bool isFilterScope,
                               QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogBulkEdit)
    , m_nodeShapeList(nodeShapeList)
    , m_iconPathList(iconPathList)
    , m_color(Qt::gray)
{
    ui->setupUi(this);

    // Description label
    const QString scopeStr   = (scope == Scope::Nodes) ? tr("nodes") : tr("edges");
    const QString filterStr  = isFilterScope ? tr("visible") : tr("selected");
    ui->descLabel->setText(
        tr("Apply to %1 %2 %3:").arg(targetCount).arg(filterStr).arg(scopeStr));

    // Helper: append one property entry
    auto addProp = [this](const QString &displayName, const QString &key, int page) {
        ui->propertyCombo->addItem(displayName);
        m_propertyKeys  << key;
        m_propertyPages << page;
    };

    // Built-in properties — scope-dependent
    if (scope == Scope::Nodes) {
        addProp(tr("Label"),  QStringLiteral("Label"),  PAGE_TEXT);
        addProp(tr("Size"),   QStringLiteral("Size"),   PAGE_INT);
        addProp(tr("Color"),  QStringLiteral("Color"),  PAGE_COLOR);
        addProp(tr("Shape"),  QStringLiteral("Shape"),  PAGE_SHAPE);
    } else {
        addProp(tr("Label"),  QStringLiteral("Label"),  PAGE_TEXT);
        addProp(tr("Weight"), QStringLiteral("Weight"), PAGE_DOUBLE);
        addProp(tr("Color"),  QStringLiteral("Color"),  PAGE_COLOR);
    }

    // Custom attribute keys separated from built-ins
    if (!existingKeys.isEmpty()) {
        ui->propertyCombo->insertSeparator(ui->propertyCombo->count());
        m_propertyKeys  << QString();  // placeholder for separator
        m_propertyPages << -1;
        for (const QString &key : existingKeys)
            addProp(key, key, PAGE_TEXT);
    }

    // Populate shape combo (nodes only)
    if (scope == Scope::Nodes && !m_nodeShapeList.isEmpty()) {
        ui->shapeCombo->addItems(m_nodeShapeList);
        for (int i = 0; i < m_nodeShapeList.size() && i < m_iconPathList.size(); ++i)
            ui->shapeCombo->setItemIcon(i, QIcon(m_iconPathList[i]));
    }

    // Colour button: show a filled swatch as the icon (same pattern as DialogNodeEdit)
    m_pixmap = QPixmap(60, 20);
    m_pixmap.fill(m_color);
    ui->colorButton->setIcon(QIcon(m_pixmap));

    // Start on the page matching the first combo item
    ui->valueStack->setCurrentIndex(
        m_propertyPages.isEmpty() ? PAGE_TEXT : m_propertyPages.first());

    connect(ui->propertyCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DialogBulkEdit::onPropertyChanged);
    connect(ui->colorButton, &QToolButton::clicked,
            this, &DialogBulkEdit::onColorButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &DialogBulkEdit::getUserChoices);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    ui->textEdit->setFocus();
}

DialogBulkEdit::~DialogBulkEdit()
{
    delete ui;
}

void DialogBulkEdit::onPropertyChanged(int index)
{
    if (index < 0 || index >= m_propertyPages.size())
        return;

    const int page = m_propertyPages[index];
    if (page == -1) {
        // Landed on separator — advance to the next valid item
        const int next = index + 1;
        if (next < m_propertyPages.size() && m_propertyPages[next] != -1)
            ui->propertyCombo->setCurrentIndex(next);
        return;
    }

    ui->valueStack->setCurrentIndex(page);

    switch (page) {
    case PAGE_TEXT:   ui->textEdit->setFocus();    break;
    case PAGE_INT:    ui->intSpin->setFocus();     break;
    case PAGE_DOUBLE: ui->doubleSpin->setFocus();  break;
    case PAGE_COLOR:  ui->colorButton->setFocus(); break;
    case PAGE_SHAPE:  ui->shapeCombo->setFocus();  break;
    default: break;
    }
}

void DialogBulkEdit::onColorButtonClicked()
{
    const QColor chosen = QColorDialog::getColor(m_color, this, tr("Select color"));
    if (chosen.isValid()) {
        m_color = chosen;
        m_pixmap.fill(m_color);
        ui->colorButton->setIcon(QIcon(m_pixmap));
    }
}

void DialogBulkEdit::getUserChoices()
{
    const int idx = ui->propertyCombo->currentIndex();
    if (idx < 0 || idx >= m_propertyKeys.size())
        return;

    const QString &property = m_propertyKeys[idx];
    if (property.isEmpty())
        return;  // separator — shouldn't be reachable

    QString value;
    switch (m_propertyPages[idx]) {
    case PAGE_TEXT:   value = ui->textEdit->text();                              break;
    case PAGE_INT:    value = QString::number(ui->intSpin->value());             break;
    case PAGE_DOUBLE: value = QString::number(ui->doubleSpin->value(), 'f', 2); break;
    case PAGE_COLOR:  value = m_color.name();                                   break;
    case PAGE_SHAPE:  value = ui->shapeCombo->currentText();                    break;
    default: return;
    }

    qDebug() << "DialogBulkEdit: property" << property << "value" << value;
    emit userChoices(property, value);
    accept();
}
