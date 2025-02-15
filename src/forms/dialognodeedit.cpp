/**
 * @file dialognodeedit.cpp
 * @brief Implements the DialogNodeEdit class for editing node properties in the network graph visualization.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "dialognodeedit.h"
#include "ui_dialognodeedit.h"

#include "global.h"

#include <QGlobalStatic>
#include <QFileDialog>
#include <QDebug>
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>
#include <QRadioButton>
#include <QPushButton>
#include <QColorDialog>
#include <QPixmap>
#include <QGraphicsColorizeEffect>
#include <QTableWidget>

SOCNETV_USE_NAMESPACE

DialogNodeEdit::DialogNodeEdit(QWidget *parent,
                               const QStringList &nodeShapeList,
                               const QStringList &iconPathList,
                               const QString &label,
                               const int &size,
                               const QColor &color,
                               const QString &shape,
                               const QString &path,
                               const QHash<QString, QString> &customAttributes) : QDialog(parent),
                                                                                  m_shapeList(nodeShapeList),
                                                                                  m_iconList(iconPathList),
                                                                                  nodeLabel(label),
                                                                                  nodeSize(size),
                                                                                  nodeColor(color),
                                                                                  nodeShape(shape),
                                                                                  iconPath(path),
                                                                                  m_customAttributes(customAttributes),
                                                                                  ui(new Ui::DialogNodeEdit)
{
    ui->setupUi(this);

    qDebug() << "opening DialogNodeEdit."
             << "label" << nodeLabel
             << "size" << nodeSize
             << "color" << nodeColor
             << "shape" << shape
             << "iconPath" << iconPath
             << "customAttributes" << customAttributes;

    // Set the builtin node properties
    ui->labelEdit->setText(nodeLabel);
    ui->sizeSpin->setValue(nodeSize);

    ui->nodeShapeComboBox->addItems(m_shapeList);

    for (int i = 0; i < m_shapeList.size(); ++i)
    {
        ui->nodeShapeComboBox->setItemIcon(i, QIcon(m_iconList[i]));
    }

    ui->nodeIconSelectButton->setEnabled(false);
    ui->nodeIconSelectEdit->setEnabled(false);

    int index = -1;
    if ((index = m_shapeList.indexOf(nodeShape)) != -1)
    {

        ui->nodeShapeComboBox->setCurrentIndex(index);

        if (index == NodeShape::Custom)
        {

            ui->nodeShapeComboBox->setCurrentIndex(NodeShape::Custom);
            ui->nodeIconSelectButton->setEnabled(true);
            ui->nodeIconSelectEdit->setEnabled(true);
            ui->nodeIconSelectEdit->setText(iconPath);
            if (!iconPath.isEmpty())
            {
                ui->nodeShapeComboBox->setItemIcon(
                    NodeShape::Custom,
                    QIcon(iconPath));
            }
            else
            {
                QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
                effect->setColor(QColor("red"));
                ui->nodeIconSelectButton->setGraphicsEffect(effect);
                ui->nodeIconSelectEdit->setGraphicsEffect(effect);
                (ui->buttonBox)->button(QDialogButtonBox::Cancel)->setDefault(true);
                (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(false);
            }
        }
    }
    else
    {
        // default -- should never happen...
        ui->nodeShapeComboBox->setCurrentIndex(NodeShape::Circle);
    }

    pixmap = QPixmap(60, 20);
    pixmap.fill(nodeColor);
    ui->colorButton->setIcon(QIcon(pixmap));

    // Set the custom attributes
    // setCustomAttributes(customAttributes);
    ;
    ui->customAttributesTable->setRowCount(0);
    for (auto it = customAttributes.begin(); it != customAttributes.end(); ++it)
    {
        int row = ui->customAttributesTable->rowCount();
        ui->customAttributesTable->insertRow(row);
        ui->customAttributesTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        ui->customAttributesTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }


    // Connect signals and slots
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(getUserChoices()));

    (ui->buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);

    (ui->labelEdit)->setFocus();

    connect(ui->labelEdit, &QLineEdit::editingFinished,
            this, &DialogNodeEdit::checkErrors);

    connect(ui->colorButton, &QToolButton::clicked,
            this, &DialogNodeEdit::selectColor);

    connect(ui->nodeShapeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DialogNodeEdit::getNodeShape);

    connect(ui->nodeIconSelectButton, &QToolButton::clicked,
            this, &DialogNodeEdit::getNodeIconFile);

    connect(ui->addPropertyBtn, &QPushButton::clicked, this, &DialogNodeEdit::on_addPropertyButton_clicked);
}


DialogNodeEdit::~DialogNodeEdit()
{
    delete ui;
}


/**
 * @brief Sets the node shape based on the provided index and updates the UI accordingly.
 *
 * This function takes an integer index representing a node shape and sets the corresponding
 * shape name to the `nodeShape` member variable. It also updates the UI elements based on
 * whether the selected shape is custom or not.
 *
 * @param nodeShapeIndex The index representing the node shape. It should correspond to one of the
 *                       values defined in the NodeShape enumeration.
 *
 * The function performs the following actions:
 * - Maps the nodeShapeIndex to a string representation of the shape.
 * - If the node shape is custom, enables the text edit and file button, sets the icon path,
 *   and updates the UI elements with the custom icon.
 * - If the node shape is not custom, disables the text edit and file button, clears the icon path,
 *   and resets the UI elements.
 */
void DialogNodeEdit::getNodeShape(const int &nodeShapeIndex)
{

    switch (nodeShapeIndex)
    {
    case NodeShape::Box:
        nodeShape = "box";
        break;
    case NodeShape::Circle:
        break;
    case NodeShape::Diamond:
        nodeShape = "diamond";
        break;
    case NodeShape::Ellipse:
        nodeShape = "ellipse";
        break;
    case NodeShape::Triangle:
        nodeShape = "triangle";
        break;
    case NodeShape::Star:
        nodeShape = "star";
        break;
    case NodeShape::Person:
        nodeShape = "person";
        break;
    case NodeShape::PersonB:
        nodeShape = "person-b";
        break;
    case NodeShape::Bugs:
        nodeShape = "bugs";
        break;
    case NodeShape::Heart:
        nodeShape = "heart";
        break;
    case NodeShape::Dice:
        nodeShape = "dice";
        break;
    case NodeShape::Custom:
        nodeShape = "custom";
        break;
    default:
        break;
    }

    qDebug() << "DialogNodeEdit::getNodeShape() - new node shape " << nodeShape;

    if (nodeShapeIndex == NodeShape::Custom)
    {
        // enable textedit and file button and raise file dialog

        ui->nodeIconSelectButton->setEnabled(true);
        ui->nodeIconSelectEdit->setEnabled(true);
        ui->nodeIconSelectEdit->setText(iconPath);
        if (!iconPath.isEmpty())
        {
            ui->nodeShapeComboBox->setItemIcon(NodeShape::Custom, QIcon(iconPath));
            ui->nodeIconSelectButton->setGraphicsEffect(0);
            ui->nodeIconSelectEdit->setGraphicsEffect(0);
        }
        else
        {
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
            effect->setColor(QColor("red"));
            ui->nodeIconSelectButton->setGraphicsEffect(effect);
            ui->nodeIconSelectEdit->setGraphicsEffect(effect);
            (ui->buttonBox)->button(QDialogButtonBox::Cancel)->setDefault(true);
            (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(false);
        }
    }
    else
    {
        ui->nodeIconSelectButton->setEnabled(false);
        ui->nodeIconSelectEdit->setEnabled(false);
        ui->nodeIconSelectEdit->setText("");
        iconPath = QString();
        ui->nodeIconSelectButton->setGraphicsEffect(0);
        ui->nodeIconSelectEdit->setGraphicsEffect(0);
        (ui->buttonBox)->button(QDialogButtonBox::Ok)->setDefault(true);
        (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void DialogNodeEdit::getNodeIconFile()
{

    QString m_nodeIconFile = QFileDialog::getOpenFileName(this, tr("Select a new icon"),
                                                          ui->nodeIconSelectEdit->text(),
                                                          tr("Images (*.png *.jpg *.jpeg *.svg);;All (*.*)"));
    if (!m_nodeIconFile.isEmpty())
    {
        qDebug() << m_nodeIconFile;
        ui->nodeIconSelectEdit->setText(m_nodeIconFile);
        ui->nodeIconSelectButton->setGraphicsEffect(0);
        ui->nodeIconSelectEdit->setGraphicsEffect(0);
        ui->nodeShapeComboBox->setItemIcon(NodeShape::Custom, QIcon(m_nodeIconFile));
        (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else
    {
        // user pressed Cancel ?
        // stop
        if (ui->nodeIconSelectEdit->text().isEmpty())
        {
            (ui->buttonBox)->button(QDialogButtonBox::Cancel)->setDefault(true);
            (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(false);
        }
    }
}

/**
 * @brief DialogNodeEdit::getUserChoices
 */
void DialogNodeEdit::getUserChoices()
{
    qDebug() << " DialogNodeEdit::getUserChoices()";
    nodeLabel = ui->labelEdit->text();
    nodeSize = ui->sizeSpin->value();
    nodeShape = "circle";
    int nodeShapeIndex = ui->nodeShapeComboBox->currentIndex();

    switch (nodeShapeIndex)
    {
    case NodeShape::Box:
        nodeShape = "box";
        break;
    case NodeShape::Circle:
        nodeShape = "circle";
        break;
    case NodeShape::Diamond:
        nodeShape = "diamond";
        break;
    case NodeShape::Ellipse:
        nodeShape = "ellipse";
        break;
    case NodeShape::Triangle:
        nodeShape = "triangle";
        break;
    case NodeShape::Star:
        nodeShape = "star";
        break;
    case NodeShape::Person:
        nodeShape = "person";
        iconPath = m_iconList[nodeShapeIndex];
        break;
    case NodeShape::PersonB:
        nodeShape = "person-b";
        iconPath = m_iconList[nodeShapeIndex];
        break;
    case NodeShape::Bugs:
        nodeShape = "bugs";
        iconPath = m_iconList[nodeShapeIndex];
        break;
    case NodeShape::Heart:
        nodeShape = "heart";
        iconPath = m_iconList[nodeShapeIndex];
        break;
    case NodeShape::Dice:
        nodeShape = "dice";
        iconPath = m_iconList[nodeShapeIndex];
        break;

    case NodeShape::Custom:
        nodeShape = "custom";
        iconPath = ui->nodeIconSelectEdit->text();
        break;
    default:
        break;
    }

    // ui->customAttributesTable
    for (int i = 0; i < ui->customAttributesTable->rowCount(); ++i)
    {
        QString key = ui->customAttributesTable->item(i, 0)->text();
        QString value = ui->customAttributesTable->item(i, 1)->text();
        m_customAttributes.insert(key, value);
    }
    emit userChoices(nodeLabel, nodeSize, nodeColor, nodeShape, iconPath, m_customAttributes);
}

void DialogNodeEdit::selectColor()
{
    qDebug() << " DialogNodeEdit::selectColor()";
    nodeColor = QColorDialog::getColor(
        Qt::red, this, tr("Select node color"));
    if (nodeColor.isValid())
    {
        qDebug() << " color selected " << nodeColor.name();
        pixmap.fill(nodeColor);
        ui->colorButton->setIcon(QIcon(pixmap));
    }
    else
    {
        // user pressed Cancel
        qDebug() << " Aborted node color";
    }
}

void DialogNodeEdit::setCustomAttributes(const QHash<QString, QString> &attributes)
{

}

/**
 * @brief Slot function that is called when the "Add Property" button is clicked.
 *
 * This function retrieves the key and value from the respective line edits,
 * checks if both are non-empty, and if so, inserts the key-value pair into
 * the custom attributes map and updates the custom attributes table with the
 * new entry. After adding the new property, it clears the input fields.
 */
void DialogNodeEdit::on_addPropertyButton_clicked()
{
    QString key = ui->keyLineEdit->text();
    QString value = ui->valueLineEdit->text();
    if (!key.isEmpty() && !value.isEmpty())
    {
        m_customAttributes.insert(key, value);
        int row = ui->customAttributesTable->rowCount();
        ui->customAttributesTable->insertRow(row);
        ui->customAttributesTable->setItem(row, 0, new QTableWidgetItem(key));
        ui->customAttributesTable->setItem(row, 1, new QTableWidgetItem(value));
        ui->keyLineEdit->clear();
        ui->valueLineEdit->clear();
    }
}

/**
 * @brief DialogNodeEdit::checkErrors
 */
void DialogNodeEdit::checkErrors()
{
    qDebug() << " DialogNodeEdit::checkErrors()";
    QString userLabel = ui->labelEdit->text();
    userLabel = userLabel.simplified();
    ui->labelEdit->setText(userLabel);

    if (ui->labelEdit->text().isEmpty())
    {
        qDebug() << "empty label!";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui->labelEdit->setGraphicsEffect(effect);
        //(ui->buttonBox)->button (QDialogButtonBox::Ok)->setEnabled(false);
    }
    else
    {
        ui->labelEdit->setGraphicsEffect(0);
        (ui->buttonBox)->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    // getUserChoices();
}
