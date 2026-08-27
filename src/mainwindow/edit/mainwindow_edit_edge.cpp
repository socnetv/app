/**
 * @file mainwindow_edit_edge.cpp
 * @brief Implements MainWindow Edit menu edge operations: add/remove, properties, appearance, symmetrize/dichotomize/undirect transforms.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
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

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "widgets/graphtablewidget.h"
#include "forms/dialognodeedit.h"
#include "forms/dialogedgeedit.h"
#include "forms/dialogbulkedit.h"
#include "forms/dialognodefind.h"
#include "forms/dialogedgedichotomization.h"
#include "forms/dialogfilteredgesbyweight.h"
#include "forms/dialogfilternodesbycentrality.h"
#include "forms/dialogfilterbyattribute.h"
#include "forms/dialogquerybuilder.h"

#include <QtWidgets>

/**
 * @brief Called when user selects something in the Edge Transform
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditEdgeTransformSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected edge transform, index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotEditEdgeSymmetrizeAll();
        break;
    case 2:
        slotEditEdgeSymmetrizeStrongTies();
        break;
    case 3:
        slotEditEdgeSymmetrizeCocitation();
        break;
    case 4:
        slotEditEdgeDichotomizationDialog();
        break;
    };
}

/**
 * @brief Opens DialogBulkEdit for the currently selected edges (canvas shortcut).
 */
void MainWindow::slotEditEdgeSetPropertyForSelection()
{
    const QList<SelectedEdge> selectedEdges = activeGraph->getSelectedEdges();
    const int count = selectedEdges.size();
    if (count == 0)
    {
        statusMessage(tr("No edges selected."));
        return;
    }

    // Collect unique custom attribute keys across the selection
    QSet<QString> keySet;
    for (const SelectedEdge &e : selectedEdges)
        for (const QString &k : activeGraph->edgeCustomAttributes(e.first, e.second).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    auto dlg = std::make_unique<DialogBulkEdit>(
        DialogBulkEdit::Scope::Edges, existingKeys,
        QStringList(), QStringList(), count, false, this);

    connect(dlg.get(), &DialogBulkEdit::userChoices,
            this, [this, selectedEdges](const QString &property, const QString &value)
            {
        runGraphOperationAsync(
            [this, selectedEdges, property, value]() {
                for (const SelectedEdge &e : selectedEdges) {
                    if (property == QLatin1String("Label")) {
                        activeGraph->edgeLabelSet(e.first, e.second, value);
                    } else if (property == QLatin1String("Weight")) {
                        activeGraph->edgeWeightSet(e.first, e.second, value.toDouble());
                    } else if (property == QLatin1String("Color")) {
                        activeGraph->edgeColorSet(e.first, e.second, value);
                    } else {
                        activeGraph->edgeCustomAttributesSet(
                            e.first, e.second, {{property, value}});
                    }
                }
            },
            tr("Applying bulk edit..."),
            [this, selectedEdges, property]() {
                statusMessage(tr("Set '%1' on %2 edge(s).").arg(property).arg(selectedEdges.size()));
                if (m_tableDock && m_tableDock->isVisible())
                    m_tableWidget->refresh(activeGraph);
            }); });

    dlg->exec();
}

/**
 * @brief Displays information about the clicked edge on the statusbar
 *
 * Called by Graph when the user clicks on an edge or when we need to init the LCDs (i.e. clearing the graph).
 *
 * @param edge
 * @param openMenu
 */
void MainWindow::slotEditEdgeClicked(const MyEdge &edge,
                                     const bool &openMenu)
{

    int v1 = edge.source;
    int v2 = edge.target;
    qreal weight = edge.weight;
    qreal reverseWeight = edge.rWeight;
    int type = edge.type;


    if (v1 == 0 || v2 == 0)
    {
        rightPanelClickedEdgeNameLCD->setText("-");
        rightPanelClickedEdgeWeightLabel->setVisible(false);
        rightPanelClickedEdgeWeightLCD->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);
        return;
    }

    rightPanelClickedEdgeWeightLabel->setVisible(true);
    rightPanelClickedEdgeWeightLCD->setVisible(true);

    QString edgeName;

    if (type == EdgeType::Undirected)
    {
        statusMessage(QString(tr("Undirected edge %1 <--> %2 of weight %3 has been selected. "
                                 "Click anywhere else to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" -- ") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);
        if (openMenu)
        {
            edgeName = QString("EDGE: ") + QString::number(v1) + QString(" -- ") + QString::number(v2);
        }
    }
    else if (type == EdgeType::Reciprocated)
    {
        statusMessage(QString(tr("Reciprocated edge %1 <--> %2 has been selected. "
                                 "Weight %1 --> %2 = %3, "
                                 "Weight %2 --> %1 = %4. "
                                 "Click anywhere else to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight)
                          .arg(reverseWeight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" <-->") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setText("Recipr.:");
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(true);
        rightPanelClickedEdgeReciprocalWeightLCD->setText(QString::number(reverseWeight));
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(true);
        if (openMenu)
        {
            edgeName = QString("RECIPROCATED EDGE: ") + QString::number(v1) + QString(" <-->") + QString::number(v2);
        }
    }
    else
    {
        statusMessage(QString(tr("Directed edge %1 --> %2 of weight %3 has been selected. "
                                 "Click again to unselect it."))
                          .arg(v1)
                          .arg(v2)
                          .arg(weight));
        rightPanelClickedEdgeNameLCD->setText(QString::number(v1) + QString(" -->") + QString::number(v2));
        rightPanelClickedEdgeWeightLabel->setText(tr("Weight:"));
        rightPanelClickedEdgeWeightLCD->setText(QString::number(weight));
        rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
        rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);

        if (openMenu)
        {
            edgeName = QString("DIRECTED EDGE: ") + QString::number(v1) + QString(" -->") + QString::number(v2);
        }
    }

    if (openMenu)
    {
        slotEditEdgeOpenContextMenu(edgeName);
    }
}

/**
 * @brief Shows a context menu when the user right-clicks directly on an edge.
 *
 * The menu is selection-aware — it reflects the total number of currently
 * selected edges:
 * - Single edge selected: offers Edge Properties (per-edge dialog for label,
 *   weight, color, custom attributes) plus Remove, Weight, Label, Color actions.
 * - Multiple edges selected: offers "Edit Selection in Data Table" and
 *   "Set property on selected edges…" at the top, followed by per-edge actions.
 *
 * @param str  A human-readable identifier for the edge shown as the menu title
 *             (e.g. "3 --> 7").
 */
void MainWindow::slotEditEdgeOpenContextMenu(const QString &str)
{
    qCDebug(lcMainWindow) << "MW: slotEditEdgeOpenContextMenu() for" << str
             << "at" << QCursor::pos().x() << "," << QCursor::pos().y();

    const int edgesSelected = activeGraph->getSelectedEdgesCount();

    QMenu *edgeContextMenu = new QMenu(str, this);
    edgeContextMenu->addAction(str);
    edgeContextMenu->addSeparator();

    if (edgesSelected > 1)
    {
        // Multiple edges selected: offer bulk actions first
        edgeContextMenu->addAction(editNodeEditSelectionInTableAct);
        edgeContextMenu->addAction(editEdgeSetPropertyForSelectionAct);
        edgeContextMenu->addSeparator();
    }
    else
    {
        // Single edge: per-edge properties
        edgeContextMenu->addAction(editEdgePropertiesAct);
        edgeContextMenu->addSeparator();
    }

    edgeContextMenu->addAction(editEdgeRemoveAct);
    edgeContextMenu->addAction(editEdgeWeightAct);
    edgeContextMenu->addAction(editEdgeLabelAct);
    edgeContextMenu->addAction(editEdgeColorAct);
    edgeContextMenu->exec(QCursor::pos());
    delete edgeContextMenu;
}

/**
 * @brief Opens dialogs for the user to specify the source and the target node of a new edge
 *
 * Called when user clicks on the MW button/menu item "Add edge"
 *
 */
void MainWindow::slotEditEdgeAdd()
{
    qCDebug(lcMainWindow) << "Request to add a new edge through UI. Opening source/target node dialogs...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    qreal weight = 1; // weight of this new edge should be one...
    bool ok = false;
    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (min == max)
        return; // if there is only one node->no edge

    if (!activeGraph->vertexClicked())
    {
        sourceNode = QInputDialog::getInt(
            this,
            "Create new edge, Step 1",
            tr("This will draw a new edge between two nodes. \n"
               "Enter source node (" +
               QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Add edge operation cancelled.");
            return;
        }
    }
    else
        sourceNode = activeGraph->vertexClicked();

    qCDebug(lcMainWindow) << "sourceNode:" << sourceNode;

    if (!activeGraph->vertexExists(sourceNode))
    {
        qCDebug(lcMainWindow) << "Cannot find sourceNode" << sourceNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?"));
        return;
    }

    targetNode = QInputDialog::getInt(this, "Create new edge, Step 2",
                                      tr("Source node:") + QString::number(sourceNode) + tr(" \nNow enter a target node [") + QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "]:", min, min, max, 1, &ok);
    if (!ok)
    {
        statusMessage("Add edge target operation cancelled.");
        return;
    }
    if (!activeGraph->vertexExists(targetNode))
    {
        qCDebug(lcMainWindow) << "Cannot find targetNode" << targetNode;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That node does not exist!"),
                              tr("Error. That node does not exist!"),
                              tr("Are you sure you entered the correct node number?"));
        return;
    }

    weight = QInputDialog::getDouble(
        this, "Create new edge, Step 3",
        tr("Source and target nodes accepted. \n"
           "Please, enter the weight of new edge: "),
        1.0, -100.0, 100.0, 1, &ok);
    if (!ok)
    {
        statusMessage("Add edge operation cancelled.");
        return;
    }
    // Check if this edge already exists...
    if (activeGraph->edgeExists(sourceNode, targetNode) != 0)
    {
        qCDebug(lcMainWindow, "edge exists. Aborting");
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. That edge already exists!"),
                              tr("Error. That edge already exists!"),
                              tr("Are you sure you entered the correct node numbers?"));
        return;
    }

    slotEditEdgeCreate(sourceNode, targetNode, weight);
}

/**
 * @brief Handles UI requests to create new edges, when the user clicks the menu item or doubles-clicks two nodes
 *
 * @param source
 * @param target
 * @param weight
 */
void MainWindow::slotEditEdgeCreate(const int &source, const int &target, const qreal &weight)
{
    qCDebug(lcMainWindow) << "User requested to create a new edge"
             << source << "->" << target << "weight" << weight
             << "Setting user settings and calling Graph to to do the job...";

    bool bezier = false;
    bool result = activeGraph->edgeCreate(
        source, target, weight,
        appSettings["initEdgeColor"],
        (editEdgeUndirectedAllAct->isChecked()) ? 2 : 0,
        (editEdgeUndirectedAllAct->isChecked()) ? false : ((appSettings["initEdgeArrows"] == "true") ? true : false), bezier);

    if (result)
    {
        statusMessage(tr("New edge %1 -> %2 created, weight %3.").arg(source).arg(target).arg(weight));
    }
}

/**
 * @brief Opens the Edge Properties dialog for the currently clicked edge.
 */
void MainWindow::slotEditEdgePropertiesDialog()
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditEdgePropertiesDialog()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const QList<SelectedEdge> selectedEdges = activeGraph->getSelectedEdges();
    const int selectedEdgesCount = selectedEdges.size();

    if (selectedEdgesCount == 0)
    {
        statusMessage(tr("Select an edge first to edit its properties."));
        return;
    }

    if (selectedEdgesCount > 1)
    {
        slotEditEdgeSetPropertyForSelection();
        return;
    }

    // Exactly one edge selected — open the full properties dialog
    int v1 = selectedEdges.constFirst().first;
    int v2 = selectedEdges.constFirst().second;

    // A reciprocated edge is one GraphicsEdge item, but two independent directed arcs in the
    // model (each with its own weight/label/color) - ask which direction to edit rather than
    // silently always editing whichever direction was created first. Same pattern as the
    // direction-choice dialog in slotEditEdgeRemove(). See #251, #264.
    activeGraph->edgeClickedSet(v1, v2);
    if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
    {
        QStringList items;
        QString arcA = QString::number(v1) + " -->" + QString::number(v2);
        QString arcB = QString::number(v2) + " -->" + QString::number(v1);
        items << arcA << arcB;

        bool ok = false;
        QString selectedArc = QInputDialog::getItem(
            this, tr("Select edge"),
            tr("This is a reciprocated edge. "
               "Select direction to edit properties for:"),
            items, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Edge properties operation cancelled."));
            return;
        }
        if (selectedArc == arcB)
        {
            const int tmp = v1;
            v1 = v2;
            v2 = tmp;
        }
    }

    const QString label = activeGraph->edgeLabel(v1, v2);
    const double weight = static_cast<double>(activeGraph->edgeWeight(v1, v2));
    const QColor color = QColor(activeGraph->edgeColor(v1, v2));
    const QHash<QString, QString> attrs = activeGraph->edgeCustomAttributes(v1, v2);

    DialogEdgeEdit *dialog = new DialogEdgeEdit(this, v1, v2, label, weight, color, attrs);
    connect(dialog, &DialogEdgeEdit::userChoices,
            this, [this, v1, v2](const QString &label, const double &weight, const QColor &color, const QHash<QString, QString> &customAttributes)
            { slotEditEdgeProperties(v1, v2, label, weight, color, customAttributes); });
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

/**
 * @brief Applies updated edge properties received from DialogEdgeEdit.
 */
void MainWindow::slotEditEdgeProperties(const int &v1,
                                        const int &v2,
                                        const QString &label,
                                        const double &weight,
                                        const QColor &color,
                                        const QHash<QString, QString> &customAttributes)
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditEdgeProperties()";
    if (v1 == 0 || v2 == 0)
        return;

    activeGraph->edgeLabelSet(v1, v2, label);
    activeGraph->edgeWeightSet(v1, v2, static_cast<qreal>(weight));
    activeGraph->edgeColorSet(v1, v2, color.name());
    activeGraph->edgeCustomAttributesSet(v1, v2, customAttributes);

    statusMessage(tr("Updated properties of edge %1 \u2192 %2.").arg(v1).arg(v2));
}

/**
 * @brief Removes a clicked edge. Otherwise asks the user to specify one edge.
 */
void MainWindow::slotEditEdgeRemove()
{

    qCDebug(lcMainWindow) << "Removing selected edges...";

    if (!activeNodes() || activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int min = 0, max = 0, sourceNode = -1, targetNode = -1;
    bool ok = false;
    bool removeOpposite = false;

    int selectedEdgeCount = activeGraph->getSelectedEdgesCount();

    qCDebug(lcMainWindow) << "Selected edges:" << selectedEdgeCount;

    if (!selectedEdgeCount)
    {

        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - No edge selected. "
                    "Prompting user to select...";

        sourceNode = QInputDialog::getInt(
            this, tr("Remove edge"),
            tr("Source node:  (") + QString::number(min) +
                "..." + QString::number(max) + "):",
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Remove edge operation cancelled.");
            return;
        }

        targetNode = QInputDialog::getInt(
            this,
            tr("Remove edge"),
            tr("Target node:  (") + QString::number(min) + "..." +
                QString::number(max) + "):",
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Remove edge operation cancelled.");
            return;
        }
        if (activeGraph->edgeExists(sourceNode, targetNode, false) != 0)
        {
            removeOpposite = false;
            if (activeGraph->isUndirected())
            {
                removeOpposite = true;
            }
        }
        else
        {
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));
            return;
        }
    }
    else
    {

        if (selectedEdgeCount > 1)
        {

            qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - Multiple edges selected. "
                        "Calling Graph to remove all of them...";

            activeGraph->edgeRemoveSelectedAll();
            return;
        }

        // Exactly one edge is selected, but it may have been selected via rubber-band rather
        // than an individual click - in that case edgeClicked() below still holds stale (or
        // default 0,0) state from whatever was last individually clicked, not the actual
        // selected edge. Sync it explicitly so the reciprocated-edge dialog below shows the
        // right node numbers regardless of how the edge was selected.
        {
            const SelectedEdge selected = activeGraph->getSelectedEdges().first();
            activeGraph->edgeClickedSet(selected.first, selected.second);
        }

        qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() - One edge selected: "
                 << activeGraph->edgeClicked().source
                 << "->"
                 << activeGraph->edgeClicked().target;

        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
        {

            QStringList items;

            QString arcA = QString::number(activeGraph->edgeClicked().source) + " -->" + QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number(activeGraph->edgeClicked().target) + " -->" + QString::number(activeGraph->edgeClicked().source);

            items << arcA
                  << arcB
                  << "Both";

            ok = false;

            QString selectedArc = QInputDialog::getItem(
                this, tr("Select edge"),
                tr("This is a reciprocated edge. "
                   "Select direction to remove:"),
                items, 0, false, &ok);

            if (selectedArc == arcA)
            {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if (selectedArc == arcB)
            {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else
            { // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                removeOpposite = true;
            }
        }
        else
        {
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }
    }

    activeGraph->edgeRemove(sourceNode, targetNode, removeOpposite);

    qCDebug(lcMainWindow) << "MW::slotEditEdgeRemove() -"
             << "View items now:" << graphicsWidget->items().size()
             << "Scene items now:" << graphicsWidget->scene()->items().size();
}

/**
 * @brief Changes the label of an edge.
 */
void MainWindow::slotEditEdgeLabel()
{
    qCDebug(lcMainWindow) << "MW::slotEditEdgeLabel()";
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    bool ok = false;

    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target)
    { // no edge clicked. Ask user to define an edge.
        sourceNode = QInputDialog::getInt(this,
                                          "Change edge label",
                                          tr("Select edge source node:  (" +
                                             QString::number(min).toLatin1() +
                                             "..." + QString::number(max).toLatin1() +
                                             "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge label operation cancelled.");
            return;
        }
        targetNode = QInputDialog::getInt(this,
                                          "Change edge label...",
                                          tr("Select edge target node:  (" +
                                             QString::number(min).toLatin1() + "..." +
                                             QString::number(max).toLatin1() + "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge label operation cancelled.");
            return;
        }

        if (!activeGraph->edgeExists(sourceNode, targetNode))
        {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));

            return;
        }
    }
    else
    { // edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }

    QString label = QInputDialog::getText(this, tr("Change edge label"),
                                          tr("Enter label: "));

    if (!label.isEmpty())
    {
        qCDebug(lcMainWindow) << "MW::slotEditEdgeLabel() - " << sourceNode << "->"
                 << targetNode << " new label " << label;
        activeGraph->edgeLabelSet(sourceNode, targetNode, label);
        slotOptionsEdgeLabelsVisibility(true);
        statusMessage(tr("Changed edge label. "));
    }
    else
    {
        statusMessage(tr("Change edge label aborted. "));
    }
}

/**
 * @brief Changes the color of all edges weighted below threshold to parameter color
 *
 * If color is not valid, it opens a QColorDialog
 * If threshold == RAND_MAX it changes the color of all edges.
 *
 * @param color = QColor()
 * @param threshold = RAND_MAX
 */
void MainWindow::slotEditEdgeColorAll(QColor color, const int threshold)
{
    qCDebug(lcMainWindow) << "Changing the color of all matching edges to color: " << color.name() << " threshold " << threshold;
    if (!color.isValid())
    {
        QString text;
        if (threshold < RAND_MAX)
        {
            text = "Change the color of edges weighted < " + QString::number(threshold);
        }
        else
            text = "Change the color of all edges";
        color = QColorDialog::getColor(appSettings["initEdgeColor"], this,
                                       text);
    }
    if (color.isValid())
    {
        qCDebug(lcMainWindow) << "new edge color: " << color.name() << " threshold " << threshold;
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (threshold < 0)
        {
            appSettings["initEdgeColorNegative"] = color.name();
        }
        else if (threshold == 0)
        {
            appSettings["initEdgeColorZero"] = color.name();
        }
        else
        {
            appSettings["initEdgeColor"] = color.name();
        }
        activeGraph->edgeColorAllSet(color.name(), threshold);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Changed edges color. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("edges color change aborted. "));
    }
}

/**
 * @brief Changes the color of the clicked edge.
 * If no edge is clicked, then it asks the user to specify one.
 */
void MainWindow::slotEditEdgeColor()
{
    qCDebug(lcMainWindow) << "MW::slotEditEdgeColor()";
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    int sourceNode = -1, targetNode = -1;
    bool ok = false;

    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();

    if (!activeGraph->edgeClicked().source || !activeGraph->edgeClicked().target)
    { // no edge clicked. Ask user to define an edge.
        sourceNode = QInputDialog::getInt(this,
                                          "Change edge color",
                                          tr("Select edge source node:  (" +
                                             QString::number(min).toLatin1() +
                                             "..." + QString::number(max).toLatin1() +
                                             "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge color operation cancelled.");
            return;
        }
        targetNode = QInputDialog::getInt(this,
                                          "Change edge color...",
                                          tr("Select edge target node:  (" +
                                             QString::number(min).toLatin1() + "..." +
                                             QString::number(max).toLatin1() + "):"),
                                          min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge color operation cancelled.");
            return;
        }

        if (!activeGraph->edgeExists(sourceNode, targetNode))
        {

            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. Cannot find that edge!"),
                                  tr("Error. Cannot find that edge!"),
                                  tr("Are you sure you entered the correct node numbers?"));

            return;
        }
    }
    else
    { // edge has been clicked.
        sourceNode = activeGraph->edgeClicked().source;
        targetNode = activeGraph->edgeClicked().target;
    }
    QString curColor = activeGraph->edgeColor(sourceNode, targetNode);
    if (!QColor(curColor).isValid())
    {
        curColor = appSettings["initEdgeColor"];
    }
    QColor color = QColorDialog::getColor(
        curColor, this, tr("Select new color...."));

    if (color.isValid())
    {
        QString newColor = color.name();
        qCDebug(lcMainWindow) << "MW::slotEditEdgeColor() - " << sourceNode << "->"
                 << targetNode << " newColor "
                 << newColor;
        activeGraph->edgeColorSet(sourceNode, targetNode, newColor);
        statusMessage(tr("Edge color changed."));
    }
    else
    {
        statusMessage(tr("Change edge color aborted. "));
    }
}

/**
 * @brief Changes the weight of the clicked edge.
 * If no edge is clicked, asks the user to specify an Edge.
 */
void MainWindow::slotEditEdgeWeight()
{
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }

    qCDebug(lcMainWindow, "MW::slotEditEdgeWeight()");
    int sourceNode = -1, targetNode = -1;
    qreal newWeight = 1.0;
    int min = activeGraph->vertexNumberMin();
    int max = activeGraph->vertexNumberMax();
    bool changeBothEdges = false;
    bool ok = false;

    // Check if an edge has been clicked/selected.
    if (activeGraph->edgeClicked().source == 0 || activeGraph->edgeClicked().target == 0)
    {
        // No edge clicked/selected. Show dialog to select the edge by source/target nodes.
        sourceNode = QInputDialog::getInt(
            this,
            "Edge weight",
            tr("Select edge source node:  (" +
               QString::number(min).toLatin1() + "..." +
               QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge weight operation cancelled.");
            return;
        }

        targetNode = QInputDialog::getInt(
            this,
            "Edge weight",
            tr("Select edge target node:  (" +
               QString::number(min).toLatin1() + "..." +
               QString::number(max).toLatin1() + "):"),
            min, 1, max, 1, &ok);
        if (!ok)
        {
            statusMessage("Change edge weight operation cancelled.");
            return;
        }

        qCDebug(lcMainWindow, "source %i target %i", sourceNode, targetNode);
    }
    else
    {
        // An edge is clicked/selected.

        qCDebug(lcMainWindow) << "MW: slotEditEdgeWeight() - an Edge has already been clicked";

        // Check if clicked edge is reciprocated
        if (activeGraph->edgeClicked().type == EdgeType::Reciprocated)
        {
            // Clicked edge is reciprocated.
            // We need the user to let us know if she wants to change a single edge or both
            QStringList items;
            QString arcA = QString::number(activeGraph->edgeClicked().source) + " -->" + QString::number(activeGraph->edgeClicked().target);
            QString arcB = QString::number(activeGraph->edgeClicked().target) + " -->" + QString::number(activeGraph->edgeClicked().source);
            items << arcA
                  << arcB
                  << "Both";
            ok = false;
            QString selectedArc = QInputDialog::getItem(this, tr("Select edge"),
                                                        tr("This is a reciprocated edge. "
                                                           "Select direction:"),
                                                        items, 0, false, &ok);
            if (selectedArc == arcA)
            {
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
            }
            else if (selectedArc == arcB)
            {
                sourceNode = activeGraph->edgeClicked().target;
                targetNode = activeGraph->edgeClicked().source;
            }
            else
            { // both
                sourceNode = activeGraph->edgeClicked().source;
                targetNode = activeGraph->edgeClicked().target;
                changeBothEdges = true;
            }
        }
        else
        {
            // Clicked edge is not reciprocated. We are good to go.
            sourceNode = activeGraph->edgeClicked().source;
            targetNode = activeGraph->edgeClicked().target;
        }

        qCDebug(lcMainWindow) << "MW: slotEditEdgeWeight() from "
                 << sourceNode << " to " << targetNode;
    }

    qreal oldWeight = 0;

    QString dialogTitle = "Edge " + QString::number(sourceNode) + "->" + QString::number(targetNode);

    bool undirected = activeGraph->isUndirected();

    // Get the new edge weight -- only if the edge exists.
    if ((oldWeight = activeGraph->edgeWeight(sourceNode, targetNode)) != 0)
    {

        // Fix the dialog title.
        if (changeBothEdges || undirected)
        {
            dialogTitle = "Edge " + QString::number(sourceNode) + "<->" + QString::number(targetNode);
        }

        // Prompt the user for the new edge weight
        newWeight = (qreal)QInputDialog::getDouble(
            this,
            dialogTitle,
            tr("New edge weight: "),
            oldWeight, -RAND_MAX, RAND_MAX, 2, &ok);

        if (ok)
        {
            activeGraph->edgeWeightSet(sourceNode, targetNode, newWeight,
                                       undirected || changeBothEdges);
        }
        else
        {
            statusMessage(QString(tr("Change edge weight cancelled.")));
            return;
        }
    }
}

/**
 * @brief Symmetrizes the ties between every two connected nodes.
 * If there is an arc from Node A to Node B,
 * then a new arc from Node B to Node is created of the same weight.
 * Thus, all arcs become reciprocal and the network becomes symmetric
 * with a symmetric adjacency matrix
 */
void MainWindow::slotEditEdgeSymmetrizeAll()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "Request to symmetrize all edges...";
    activeGraph->setSymmetric();
    slotHelpMessageToUser(USER_MSG_INFO,
                          tr("All ties have been symmetrized."),
                          tr("All ties between nodes have been symmetrized."),
                          tr("The network is now symmetric. "));
}

/**
 * @brief Adds a new cocitation symmetric relation to the network
 *
 * In the new relation, there are ties only between pairs of nodes who were cocited by others.
 */
void MainWindow::slotEditEdgeSymmetrizeCocitation()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "Request to add a new symmetric relation using cocited nodes...";
    runGraphOperationAsync(
        [this]() { activeGraph->relationAddCocitation(); },
        tr("Computing cocitation relation..."),
        [this]() {
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("New cocitation relation added. Ready"),
                                  tr("New cocitation relation has been added to the network."),
                                  tr("In the new relation, there are ties only between pairs of nodes who were cocited by others."));
        });
}

/**
 * @brief Opens up the edge dichotomization dialog
 */
void MainWindow::slotEditEdgeDichotomizationDialog()
{

    // @TODO: Check if the network is already binary and abord?

    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "MW: slotEditEdgeDichotomizationDialog() - "
                "spawning edgeDichotomizationDialog";

    m_edgeDichotomizationDialog = new DialogEdgeDichotomization(this);

    connect(m_edgeDichotomizationDialog, &DialogEdgeDichotomization::userChoices,
            this, &MainWindow::slotEditEdgeDichotomization);

    m_edgeDichotomizationDialog->exec();
}

/**
 * @brief Calls Graph::graphDichotomization() to create a new binary relation
 * in a valued network using edge dichotomization according to threshold value.
 */
void MainWindow::slotEditEdgeDichotomization(const qreal threshold)
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow, "MW: slotEditEdgeDichotomization() calling graphDichotomization()");
    activeGraph->graphDichotomization(threshold);
    slotHelpMessageToUser(USER_MSG_INFO, tr("New binary relation added."),
                          tr("New dichotomized relation created"),
                          tr("A new relation called \"%1\" has been added to the network, "
                             "using the given dichotomization threshold. ")
                              .arg("Binary"));

    statusMessage(tr("Edge dichotomization finished. "));
}

/**
 * @brief MainWindow::slotEditEdgeSymmetrizeStrongTies
 */
void MainWindow::slotEditEdgeSymmetrizeStrongTies()
{
    if (activeEdges() == 0)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_EDGES);
        return;
    }
    qCDebug(lcMainWindow) << "MW::slotEditEdgeSymmetrizeStrongTies() - calling addRelationSymmetricStrongTies()";
    int oldRelationsCounter = activeGraph->relations();
    // callIt stays false (silent return, no info dialog) if the multi-relation dialog below is
    // dismissed without picking 1 or 2 - matches the original switch's lack of a default case.
    bool callIt = true;
    bool allRelations = false;
    if (oldRelationsCounter > 0)
    {
        switch (
            slotHelpMessageToUser(USER_MSG_QUESTION_CUSTOM, tr("Select"),
                                  tr("Symmetrize social network by examining strong ties"),
                                  tr("This network has multiple relations. "
                                     "Symmetrize by examining reciprocated ties across all relations or just the current relation?"),
                                  QMessageBox::NoButton, QMessageBox::NoButton,
                                  tr("all relations"), tr("current relation")))
        {
        case 1:
            allRelations = true;
            break;
        case 2:
            allRelations = false;
            break;
        default:
            callIt = false;
            break;
        }
    }

    if (!callIt)
        return;

    runGraphOperationAsync(
        [this, allRelations]() { activeGraph->addRelationSymmetricStrongTies(allRelations); },
        tr("Symmetrizing strong ties..."),
        [this]() {
            slotHelpMessageToUser(USER_MSG_INFO, tr("New symmetric relation created from strong ties"),
                                  tr("New relation created from strong ties"),
                                  tr("A new relation \"%1\" has been added to the network. "
                                     "by counting reciprocated ties only. "
                                     "This relation is binary and symmetric. ")
                                      .arg("Strong Ties"));
        });
}

/**
 * @brief Transforms all directed arcs to undirected edges.
 * The result is a undirected and symmetric network
 */
void MainWindow::slotEditEdgeUndirectedAll(const bool &toggle)
{
    qCDebug(lcMainWindow) << "MW: slotEditEdgeUndirectedAll() - calling Graph::graphUndirectedSet()";
    if (toggle)
    {
        activeGraph->setUndirected(true);
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready"));
        }
        else
        {
            statusMessage(tr("Undirected data mode. "
                             "Any edge you add will be undirected. Ready"));
        }
    }
    else
    {
        activeGraph->setDirected(true);
        optionsEdgeArrowsAct->trigger();
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Directed data mode. "
                             "All existing undirected edges transformed to "
                             "directed. Ready"));
        }
        else
        {
            statusMessage(tr("Directed data mode. "
                             "Any new edge you add will be directed. Ready"));
        }
    }
}

/**
 * @brief Toggles between directed (mode=0) and undirected edges (mode=1)
 *
 * @param mode
 */
void MainWindow::slotEditEdgeMode(const int &mode)
{
    if (mode == 1)
    {
        qCDebug(lcMainWindow) << "Changing edge mode to undirected. Informing Graph...";
        activeGraph->setUndirected(true);
        qCDebug(lcMainWindow) << "Setting optionsEdgeArrowsAct to false";
        optionsEdgeArrowsAct->setChecked(false);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Undirected data mode. "
                             "All existing directed edges transformed to "
                             "undirected. Ready"));
        }
        else
        {
            statusMessage(tr("Undirected data mode. "
                             "Any edge you add will be undirected. Ready"));
        }
    }
    else
    {
        qCDebug(lcMainWindow) << "Changing edge mode to directed. Informing Graph...";
        activeGraph->setDirected(true);
        qCDebug(lcMainWindow) << "Triggering optionsEdgeArrowsAct checkbox";
        optionsEdgeArrowsAct->trigger();
        qCDebug(lcMainWindow) << "Setting optionsEdgeArrowsAct to true";
        optionsEdgeArrowsAct->setChecked(true);
        if (activeEdges() != 0)
        {
            statusMessage(tr("Directed data mode. "
                             "All existing undirected edges transformed to "
                             "directed."));
        }
        else
        {
            statusMessage(tr("Directed data mode. "
                             "Any new edge you add will be directed."));
        }
    }
}
