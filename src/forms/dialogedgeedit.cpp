/**
 * @file dialogedgeedit.cpp
 * @brief Implements the DialogEdgeEdit class for editing edge properties.
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

#include "dialogedgeedit.h"
#include "ui_dialogedgeedit.h"

#include <QDebug>
#include <QColorDialog>
#include <QGraphicsColorizeEffect>
#include <QTableWidget>
#include <QPushButton>
#include <QToolButton>

DialogEdgeEdit::DialogEdgeEdit(QWidget *parent,
                               const int &v1,
                               const int &v2,
                               const QString &label,
                               const double &weight,
                               const QColor &color,
                               const QHash<QString,QString> &customAttributes)
    : QDialog(parent),
      ui(new Ui::DialogEdgeEdit),
      m_v1(v1), m_v2(v2),
      m_label(label),
      m_weight(weight),
      m_color(color),
      m_customAttributes(customAttributes)
{
    ui->setupUi(this);

    qDebug() << "DialogEdgeEdit: edge" << v1 << "->" << v2
             << "label" << label << "weight" << weight
             << "color" << color << "attrs" << customAttributes;

    ui->edgeTitleLabel->setText(tr("Edge %1 \u2192 %2").arg(v1).arg(v2));
    ui->labelEdit->setText(m_label);
    ui->weightSpin->setValue(m_weight);

    m_pixmap = QPixmap(60, 20);
    m_pixmap.fill(m_color);
    ui->colorButton->setIcon(QIcon(m_pixmap));

    // Populate custom attributes table
    ui->customAttributesTable->setRowCount(0);
    for (auto it = customAttributes.cbegin(); it != customAttributes.cend(); ++it) {
        int row = ui->customAttributesTable->rowCount();
        ui->customAttributesTable->insertRow(row);
        ui->customAttributesTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        ui->customAttributesTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogEdgeEdit::getUserChoices);

    connect(ui->colorButton, &QToolButton::clicked, this, &DialogEdgeEdit::selectColor);

    connect(ui->addPropertyBtn, &QPushButton::clicked,
            this, &DialogEdgeEdit::on_addPropertyBtn_clicked);

    connect(ui->customAttributesTable, &QTableWidget::itemSelectionChanged, [this]() {
        ui->removePropertyBtn->setEnabled(
            !ui->customAttributesTable->selectedItems().isEmpty());
    });

    connect(ui->removePropertyBtn, &QPushButton::clicked,
            this, &DialogEdgeEdit::on_removePropertyBtn_clicked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    ui->labelEdit->setFocus();
}

DialogEdgeEdit::~DialogEdgeEdit()
{
    delete ui;
}

/**
 * @brief Opens a color picker and updates the color button icon.
 */
void DialogEdgeEdit::selectColor()
{
    QColor chosen = QColorDialog::getColor(m_color, this, tr("Select edge color"));
    if (chosen.isValid()) {
        m_color = chosen;
        m_pixmap.fill(m_color);
        ui->colorButton->setIcon(QIcon(m_pixmap));
    }
}

/**
 * @brief Adds a key/value pair from the input fields to the attributes table.
 */
void DialogEdgeEdit::on_addPropertyBtn_clicked()
{
    QString key   = ui->keyLineEdit->text().trimmed();
    QString value = ui->valueLineEdit->text().trimmed();

    QGraphicsColorizeEffect *effect = nullptr;
    if (key.isEmpty()) {
        effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->keyLineEdit->setGraphicsEffect(effect);
    } else {
        ui->keyLineEdit->setGraphicsEffect(nullptr);
    }
    if (value.isEmpty()) {
        effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->valueLineEdit->setGraphicsEffect(effect);
    } else {
        ui->valueLineEdit->setGraphicsEffect(nullptr);
    }
    if (key.isEmpty() || value.isEmpty())
        return;

    m_customAttributes.insert(key, value);
    int row = ui->customAttributesTable->rowCount();
    ui->customAttributesTable->insertRow(row);
    ui->customAttributesTable->setItem(row, 0, new QTableWidgetItem(key));
    ui->customAttributesTable->setItem(row, 1, new QTableWidgetItem(value));
    ui->keyLineEdit->clear();
    ui->valueLineEdit->clear();
}

/**
 * @brief Removes the selected row from the attributes table.
 */
void DialogEdgeEdit::on_removePropertyBtn_clicked()
{
    int row = ui->customAttributesTable->currentRow();
    if (row != -1) {
        QString key = ui->customAttributesTable->item(row, 0)->text();
        m_customAttributes.remove(key);
        ui->customAttributesTable->removeRow(row);
    }
    ui->removePropertyBtn->setEnabled(false);
}

/**
 * @brief Collects all field values and emits userChoices().
 */
void DialogEdgeEdit::getUserChoices()
{
    qDebug() << "DialogEdgeEdit::getUserChoices()";
    m_label  = ui->labelEdit->text();
    m_weight = ui->weightSpin->value();

    // Rebuild custom attributes from table (handles in-place cell edits)
    m_customAttributes.clear();
    for (int i = 0; i < ui->customAttributesTable->rowCount(); ++i) {
        QTableWidgetItem *keyItem   = ui->customAttributesTable->item(i, 0);
        QTableWidgetItem *valueItem = ui->customAttributesTable->item(i, 1);
        if (keyItem && valueItem && !keyItem->text().isEmpty())
            m_customAttributes.insert(keyItem->text(), valueItem->text());
    }

    emit userChoices(m_label, m_weight, m_color, m_customAttributes);
}
