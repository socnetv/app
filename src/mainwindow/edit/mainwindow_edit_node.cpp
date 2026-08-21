/**
 * @file mainwindow_edit_node.cpp
 * @brief Implements MainWindow Edit menu node operations: add/remove, properties, appearance, selection, subgraph-to-shape transforms.
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
 * @brief Called when user selects something in the Subgraph from Selected
 * Nodes selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxEditNodeSubgraphSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected subgraph creation, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotEditNodeSelectedToClique();
        break;
    case 2:
        slotEditNodeSelectedToStar();
        break;
    case 3:
        slotEditNodeSelectedToCycle();
        break;
    case 4:
        slotEditNodeSelectedToLine();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Selects all nodes
 */
void MainWindow::slotEditNodeSelectAll()
{
    qCDebug(lcMainWindow) << "Request to select all nodes...";
    graphicsWidget->selectAll();
    statusMessage(tr("Selected nodes: %1")
                      .arg(activeGraph->getSelectedVerticesCount()));
}

/**
 * @brief Selects no nodes.
 */
void MainWindow::slotEditNodeSelectNone()
{
    qCDebug(lcMainWindow) << "Clearing node selection...";
    graphicsWidget->selectNone();
    statusMessage(QString(tr("Selection cleared")));
}

/**
 * @brief Automatically runs, when the user moves a node on the canvas, to
 * update new vertex coordinates in Graph, and show a status message.
 *
 * Called from GraphicsWidget
 *
 * @param nodeNumber
 * @param x
 * @param y
 */
void MainWindow::slotEditNodePosition(const int &nodeNumber,
                                      const int &x, const int &y)
{
    qCDebug(lcMainWindow, "Updating position for node %i - x: %i, y: %i", nodeNumber, x, y);
    activeGraph->vertexPosSet(nodeNumber, x, y);
}

/**
 * @brief Adds a new random node
 *
 * Called when the "Add Node" btn is clicked
 */
void MainWindow::slotEditNodeAdd()
{
    qCDebug(lcMainWindow) << "Request to add a new random node...";
    activeGraph->vertexCreateAtPosRandom(true);
    statusMessage(tr("New random positioned node (numbered %1) added.")
                      .arg(activeGraph->vertexNumberMax()));
}

/**
 * @brief Opens the Find Node dialog
 */
void MainWindow::slotEditNodeFindDialog()
{
    qCDebug(lcMainWindow) << "Showing find node dialog...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    //@TODO - prominenceIndexList should be either
    // the list of all computes indices
    // or the last computed indice
    // or empty if the user has not computed any index yet.
    m_nodeFindDialog = new DialogNodeFind(this, prominenceIndexList);

    connect(m_nodeFindDialog, &DialogNodeFind::userChoices,
            this, &MainWindow::slotEditNodeFind);

    m_nodeFindDialog->exec();

    statusMessage(tr("Node find dialog opened. Enter your choices. "));

    return;
}

/**
 * @brief Finds one or more nodes, according to their number, label or centrality score.
 *
 * @param list
 * @param searchType
 * @param indexStr
 */
void MainWindow::slotEditNodeFind(const QStringList &nodeList,
                                  const QString &searchType,
                                  const QString &indexStr)
{

    qCDebug(lcMainWindow) << "Request to find nodes:" << nodeList
             << "search type:" << searchType
             << "indexStr" << indexStr;

    int indexType = 0;

    if (searchType == "numbers")
    {
        activeGraph->vertexFindByNumber(nodeList);
    }
    else if (searchType == "labels")
    {
        activeGraph->vertexFindByLabel(nodeList);
    }
    else if (searchType == "score")
    {

        indexType = activeGraph->getProminenceIndexByName(indexStr);

        const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
        const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
        const bool inverseWeightsFinal = inverseWeights;

        runGraphOperationAsync(
            [this, indexType, nodeList, considerWeights, inverseWeightsFinal, dropIsolates]() {
                activeGraph->vertexFindByIndexScore(indexType,
                                                    nodeList,
                                                    considerWeights,
                                                    inverseWeightsFinal,
                                                    dropIsolates);
            },
            tr("Finding nodes by index score. Please wait..."));
    }

    return;
}

/**
 * @brief Handles requests to delete a node and the attached objects (edges, etc).
 *
 * If the user has clicked on a node, it deletes it
 * Else it asks for a nodeNumber to remove.
 */
void MainWindow::slotEditNodeRemove()
{
    qCDebug(lcMainWindow) << "Request to remove a node...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->relations() > 1)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error. Cannot remove node!"),
                              tr("Error. Cannot remove this node!"),
                              tr("This a network with more than 1 relations. If you remove "
                                 "a node from the active relation, and then ask me to go "
                                 "to the previous or the next relation, then I would crash "
                                 "because I would try to display edges from a deleted node."
                                 "You cannot remove nodes in multirelational networks."));
        return;
    }

    // if there are already multiple nodes selected, erase them
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if (nodesSelected > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        qCDebug(lcMainWindow) << "multiple nodes selected to be removed";
        foreach (int nodeNumber, activeGraph->getSelectedVertices())
        {
            activeGraph->vertexRemove(nodeNumber);
        }
        editNodeRemoveAct->setText(tr("Remove Node"));
        statusMessage(tr("Removed %1 nodes.").arg(nodesSelected));
        QApplication::restoreOverrideCursor();
    }

    else
    {
        int nodeNumber = -1, min = -1, max = -1;
        bool ok = false;
        min = activeGraph->vertexNumberMin();
        max = activeGraph->vertexNumberMax();

        if (min == -1 || max == -1)
        {
            qCDebug(lcMainWindow, "ERROR in finding min max nodeNumbers. Abort");
            return;
        }
        else
        {
            nodeNumber = QInputDialog::getInt(
                this,
                tr("Remove node"),
                tr("Choose a node to remove between (" + QString::number(min).toLatin1() + "..." +
                   QString::number(max).toLatin1() + "):"),
                min, 1, max, 1, &ok);
            if (!ok)
            {
                statusMessage("Remove node operation cancelled.");
                return;
            }
        }
        qCDebug(lcMainWindow, "removing vertex with number %i from Graph", nodeNumber);
        activeGraph->vertexRemove(nodeNumber);
        qCDebug(lcMainWindow, "Completed. Node %i removed completely.", nodeNumber);
        statusMessage(tr("Node removed completely."));
    }
}

/**
 * @brief Opens the Node Properties dialog for the selected nodes.
 * If no nodes are selected, prompts the user for a node number.
 */
void MainWindow::slotEditNodePropertiesDialog()
{
    qCDebug(lcMainWindow) << "Request to open the node properties dialog...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount == 0)
    {
        statusMessage(tr("Select a node first to edit its properties."));
        return;
    }

    if (selectedNodesCount > 1)
    {
        slotEditNodeSetPropertyForSelection();
        return;
    }

    // Exactly one node selected — open the full properties dialog
    const int nodeNumber = activeGraph->getSelectedVertices().constFirst();
    const QString label = activeGraph->vertexLabel(nodeNumber);
    const QColor color = activeGraph->vertexColor(nodeNumber);
    const QString shape = activeGraph->vertexShape(nodeNumber);
    const int size = activeGraph->vertexSize(nodeNumber);
    const QString iconPath = activeGraph->vertexShapeIconPath(nodeNumber);
    const QHash<QString, QString> customAttributes = activeGraph->vertexCustomAttributes(nodeNumber);

    qCDebug(lcMainWindow) << "opening DialogNodeEdit for node" << nodeNumber;

    std::unique_ptr<DialogNodeEdit> m_nodeEditDialog = std::make_unique<DialogNodeEdit>(
        this, nodeShapeList, iconPathList, label, size, color, shape, iconPath, customAttributes);

    connect(m_nodeEditDialog.get(), &DialogNodeEdit::userChoices,
            this, &MainWindow::slotEditNodeProperties);

    m_nodeEditDialog->exec();
}

/**
 * @brief Applies the selected properties to one or multiple nodes in the graph.
 *
 * This slot updates the properties of the selected nodes or a single node, as
 * specified by the user in DialogNodeEdit. It updates the label, size, color, shape, and custom
 * attributes of the nodes.
 *
 * @param label The new label for the node(s).
 * @param size The new size for the node(s).
 * @param color The new color for the node(s).
 * @param shape The new shape for the node(s).
 * @param iconPath The path to the icon for the node(s).
 * @param customAttributes A hash of custom attributes to set for the node(s).
 */
void MainWindow::slotEditNodeProperties(const QString &label,
                                        const int &size,
                                        const QColor &color,
                                        const QString &shape,
                                        const QString &iconPath,
                                        const QHash<QString, QString> &customAttributes)
{

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    qCDebug(lcMainWindow) << "Request to update node properties - new properties: "
             << " label " << label
             << " size " << size
             << " color " << color
             << " shape " << shape
             << " vertexClicked " << activeGraph->vertexClicked()
             << " selectedNodesCount " << selectedNodesCount
             << "customAttributes" << customAttributes;

    if (selectedNodesCount == 0 && activeGraph->vertexClicked() != 0)
    {
        // no node selected but user entered a node number in a dialog
        if (label != "" && appSettings["initNodeLabelsVisibility"] != "true")
            slotOptionsNodeLabelsVisibility(true);
        activeGraph->vertexLabelSet(activeGraph->vertexClicked(), label);
        activeGraph->vertexColorSet(activeGraph->vertexClicked(), color.name());
        activeGraph->vertexSizeSet(activeGraph->vertexClicked(), size);
        activeGraph->vertexShapeSet(activeGraph->vertexClicked(), shape, iconPath);
        activeGraph->vertexCustomAttributesSet(activeGraph->vertexClicked(), customAttributes);

        statusMessage(tr("Updated the properties of node %1. ").arg(activeGraph->vertexClicked()));
    }
    else
    {
        // some nodes are selected
        int nodeNumber = 0;
        foreach (nodeNumber, activeGraph->getSelectedVertices())
        {
            qCDebug(lcMainWindow) << "node " << nodeNumber;
            if (!label.isEmpty())
            {
                if (selectedNodesCount > 1)
                {
                    activeGraph->vertexLabelSet(
                        nodeNumber,
                        label + QString::number(nodeNumber));
                }
                else
                {
                    activeGraph->vertexLabelSet(nodeNumber, label);
                }
                // turn on labels visibility if they are hidden
                if (appSettings["initNodeLabelsVisibility"] != "true")
                {
                    slotOptionsNodeLabelsVisibility(true);
                }
            }
            activeGraph->vertexColorSet(nodeNumber, color.name());
            activeGraph->vertexSizeSet(nodeNumber, size);
            activeGraph->vertexShapeSet(nodeNumber, shape, iconPath);
            activeGraph->vertexCustomAttributesSet(nodeNumber, customAttributes);
        }
        statusMessage(tr("Updated the properties of %1 nodes. ").arg(selectedNodesCount));
    }
}

/**
 * @brief Raises the Data Table dock and pre-selects rows matching the current canvas selection.
 */
void MainWindow::slotEditNodeEditSelectionInTable()
{
    if (!m_tableDock || !m_tableWidget)
        return;

    m_tableDock->show();
    m_tableDock->raise();

    const QList<int> nodes = activeGraph->getSelectedVertices();
    const QList<SelectedEdge> edges = activeGraph->getSelectedEdges();

    // Only switch to Edges tab when the selection is entirely edges (no nodes)
    if (!edges.isEmpty() && nodes.isEmpty())
        m_tableWidget->showEdgesTab();
    else
        m_tableWidget->showNodesTab();

    m_tableWidget->syncNodeSelection(nodes);
    m_tableWidget->syncEdgeSelection(edges);
}

/**
 * @brief Opens DialogBulkEdit for the currently selected nodes (canvas shortcut).
 */
void MainWindow::slotEditNodeSetPropertyForSelection()
{
    const QList<int> selectedNodes = activeGraph->getSelectedVertices();
    const int count = selectedNodes.size();
    if (count == 0)
    {
        statusMessage(tr("No nodes selected."));
        return;
    }

    // Collect unique custom attribute keys across the selection
    QSet<QString> keySet;
    for (const int v : selectedNodes)
        for (const QString &k : activeGraph->vertexCustomAttributes(v).keys())
            keySet.insert(k);
    const QStringList existingKeys(keySet.begin(), keySet.end());

    auto dlg = std::make_unique<DialogBulkEdit>(
        DialogBulkEdit::Scope::Nodes, existingKeys,
        nodeShapeList, iconPathList, count, false, this);

    connect(dlg.get(), &DialogBulkEdit::userChoices,
            this, [this, selectedNodes](const QString &property, const QString &value)
            {
        // Resolved once here (doesn't depend on which vertex) rather than per-iteration inside
        // the dispatched lambda below, which also keeps that lambda from reading
        // nodeShapeList/iconPathList (MainWindow-owned) on graphThread at all.
        QString shapeIconPath;
        if (property == QLatin1String("Shape")) {
            const int idx = nodeShapeList.indexOf(value);
            shapeIconPath = (idx >= 0 && idx < iconPathList.size()) ? iconPathList[idx] : QString();
        }
        runGraphOperationAsync(
            [this, selectedNodes, property, value, shapeIconPath]() {
                for (const int v : selectedNodes) {
                    if (property == QLatin1String("Label")) {
                        activeGraph->vertexLabelSet(v, value);
                    } else if (property == QLatin1String("Size")) {
                        activeGraph->vertexSizeSet(v, value.toInt());
                    } else if (property == QLatin1String("Color")) {
                        activeGraph->vertexColorSet(v, value);
                    } else if (property == QLatin1String("Shape")) {
                        activeGraph->vertexShapeSet(v, value, shapeIconPath);
                    } else {
                        activeGraph->vertexCustomAttributeSet(v, property, value);
                    }
                }
            },
            tr("Applying bulk edit..."),
            [this, selectedNodes, property]() {
                statusMessage(tr("Set '%1' on %2 node(s).").arg(property).arg(selectedNodes.size()));
                if (m_tableDock && m_tableDock->isVisible())
                    m_tableWidget->refresh(activeGraph);
            }); });

    dlg->exec();
}

/**
 * @brief Creates a complete subgraph (clique) from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToClique()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToClique()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Error. Not enough nodes selected."),
                              tr("Cannot create new clique because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_CLIQUE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Clique created."),
                                  tr("A new clique has been created from ") + QString::number(selectedNodesCount) + tr(" nodes"));
        });
}

/**
 * @brief Creates a star subgraph from selected nodes.
 * User must choose a central node.
 */
void MainWindow::slotEditNodeSelectedToStar()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToStar()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new star subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    int center;
    bool ok = false;

    int min = activeGraph->getSelectedVerticesMin();
    int max = activeGraph->getSelectedVerticesMax();
    center = QInputDialog::getInt(
        this,
        "Create star subgraph",
        tr("To create a star subgraph from selected nodes, \n"
           "enter the number of the central actor (" +
           QString::number(min).toLatin1() + "..." + QString::number(max).toLatin1() + "):"),
        min, 1, max, 1, &ok);
    if (!ok)
    {
        statusMessage("Create star subgraph cancelled.");
        return;
    }

    runGraphOperationAsync(
        [this, center]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_STAR, center);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Star subgraph created."),
                                  tr("A new star subgraph has been created with ") +
                                      QString::number(selectedNodesCount) + tr(" nodes."));
        });
}

/**
 * @brief Creates a cycle subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToCycle()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToCycle()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new cycle subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_CYCLE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Cycle subgraph created."),
                                  tr("A new cycle subgraph has been created with ") + QString::number(selectedNodesCount) + tr(" select nodes."));
        });
}

/**
 * @brief Creates a line subgraph from selected nodes.
 */
void MainWindow::slotEditNodeSelectedToLine()
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeSelectedToLine()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int selectedNodesCount = activeGraph->getSelectedVerticesCount();

    if (selectedNodesCount < 3)
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Not enough nodes selected."),
                              tr("Cannot create new line subgraph because you have "
                                 "not selected enough nodes."),
                              tr("Select at least three nodes first."));
        return;
    }

    runGraphOperationAsync(
        [this]() {
            activeGraph->verticesCreateSubgraph(QList<int>(), SUBGRAPH_LINE);
        },
        tr("Creating subgraph. Please wait..."),
        [this, selectedNodesCount]() {
            if (activeGraph->progressCanceled())
            {
                return;
            }
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Line subgraph created."),
                                  tr("A new line subgraph has been created with ") + QString::number(selectedNodesCount) + tr(" selected nodes."));
        });
}

/**
 * @brief Changes the color of all nodes to parameter color
 *
 * If the color is invalid, opens a QColorDialog to
 * select a new node color for all nodes.
 *
 * @param color
 */
void MainWindow::slotEditNodeColorAll(QColor color)
{
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeColor"]),
                                       this,
                                       "Change the color of all nodes");
    }
    if (color.isValid())
    {
        appSettings["initNodeColor"] = color.name();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        qCDebug(lcMainWindow) << "MW::slotEditNodeColorAll() : "
                 << appSettings["initNodeColor"];
        activeGraph->vertexColorSet(0, appSettings["initNodeColor"]);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Change all nodes' color. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
    }
}

/**
 * @brief Changes the size of nodes to newSize.
 *
 * If newSize = 0 asks the user a new size for all nodes
 * If normalized = true, changes node sizes according to their amount
 *
 * @param newSize
 * @param normalized
 */
void MainWindow::slotEditNodeSizeAll(int newSize, const bool &normalized)
{
    Q_UNUSED(normalized);
    qCDebug(lcMainWindow) << "MW: slotEditNodeSizeAll() - "
             << " newSize " << newSize;
    if (newSize == 0 && !normalized)
    {
        bool ok = true;
        newSize = QInputDialog::getInt(
            this,
            "Change node size",
            tr("Select new size for all nodes:"),
            appSettings["initNodeSize"].toInt(0, 10), 1, 100, 1, &ok);

        if (!ok)
        {
            statusMessage("Change node size operation cancelled.");
            return;
        }
    }

    appSettings["initNodeSize"] = QString::number(newSize);

    activeGraph->vertexSizeSet(0, newSize);

    statusMessage(tr("Ready"));
    return;
}

/**
 * @brief Change the shape of a node or all nodes.
 * If shape == null, prompts the user a list of available node shapes to select.
 * Then changes the shape of all nodes/vertices accordingly.
 * If vertex is non-zero, changes the shape of that node only.
 * Called when user clicks on Edit->Node > Change all nodes shapes
 * Called from DialogSettings when the user has selected a new default node shape
 * @param shape
 * @param vertex
 */
void MainWindow::slotEditNodeShape(const int &vertex, QString shape,
                                   QString nodeIconPath)
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeShape() - vertex " << vertex
             << "(0 means all)"
             << "new shape" << shape
             << "nodeIconPath" << nodeIconPath;

    if (shape.isNull())
    {

        bool ok = false;

        int curShapeIndex = nodeShapeList.indexOf(appSettings["initNodeShape"]);

        if (curShapeIndex == -1)
        {
            curShapeIndex = 1;
        }
        shape = QInputDialog::getItem(this,
                                      "Node shape",
                                      "Select a shape for all nodes: ",
                                      nodeShapeList, curShapeIndex, true, &ok);
        if (!ok)
        {
            // user pressed Cancel
            statusMessage(tr("Change node shapes aborted."));
            return;
        }
        if (shape == "custom")
        {
            nodeIconPath = QFileDialog::getOpenFileName(
                this, tr("Select an icon"), getLastPath(),
                tr("Images (*.png *.jpg *.jpeg *.svg);;All (*.*)"));
            if (nodeIconPath.isNull())
            {
                // user pressed Cancel
                statusMessage(tr("Change node shapes aborted."));
                return;
            }
        }
        else
        {
            nodeIconPath = iconPathList[nodeShapeList.indexOf(shape)];
        }
    }

    if (vertex == 0)
    { // change all nodes shapes
        activeGraph->vertexShapeSet(-1, shape, nodeIconPath);
        appSettings["initNodeShape"] = shape;
        appSettings["initNodeIconPath"] = nodeIconPath;
        statusMessage(tr("All shapes have been changed."));
    }
    else
    { // only one
        activeGraph->vertexShapeSet(vertex, shape, nodeIconPath);
        statusMessage(tr("Node shape has been changed."));
    }
}

/**
 * @brief Changes the size of one or all node numbers.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node number font size
 * if v1=0, it changes all node numbers
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeNumberSize(int v1, int newSize, const bool prompt)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumberSize - newSize " << newSize;
    if (prompt)
    {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node numbers size to: (1-16)"),
                                       appSettings["initNodeNumberSize"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change font size: Aborted."));
            return;
        }
    }
    if (v1)
    { // change one node number only
        activeGraph->vertexNumberSizeSet(v1, newSize);
    }
    else
    { // change all
        appSettings["initNodeNumberSize"] = QString::number(newSize);
        activeGraph->vertexNumberSizeSet(0, newSize);
    }
    statusMessage(tr("Changed node numbers size."));
}

/**
 * @brief Changes the text color of all node numbers
 * Called from Edit menu option and Settings dialog.
 * If color is invalid, asks the user to enter a new node number color
 * @param color
 */
void MainWindow::slotEditNodeNumbersColor(const int &v1, QColor color)
{
    qCDebug(lcMainWindow) << "MW:slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeNumberColor"]),
                                       this,
                                       "Change the color of all node numbers");
    }

    if (color.isValid())
    {

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (v1)
        {
            activeGraph->vertexNumberColorSet(v1, color.name());
        }
        else
        {
            appSettings["initNodeNumberColor"] = color.name();
            activeGraph->vertexNumberColorSet(0, color.name());
        }

        QApplication::restoreOverrideCursor();
        statusMessage(tr("Node number color changed. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
    }
}

/**
 * @brief Changes the distance of one or all node numbers from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node number distance
 * if v1=0, it changes all node number distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeNumberDistance(int v1, int newDistance)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumberDistance - newSize " << newDistance;
    if (!newDistance)
    {
        newDistance = QInputDialog::getInt(
            this, "Change node number distance",
            tr("Change all node numbers distance from their nodes to: (1-16)"),
            appSettings["initNodeNumberDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change node number distance aborted."));
            return;
        }
    }
    if (v1)
    { // change one node number distance only
        activeGraph->vertexNumberDistanceSet(v1, newDistance);
    }
    else
    { // change all
        appSettings["initNodeNumberDistance"] = QString::number(newDistance);
        activeGraph->vertexNumberDistanceSet(0, newDistance);
    }
    statusMessage(tr("Changed node number distance."));
}

/**
 * @brief Changes the size of one or all node Labels.
 * Called from Edit menu option and DialogSettings
 * if newSize=0, asks the user to enter a new node Label font size
 * if v1=0, it changes all node Labels
 * @param v1
 * @param newSize
 */
void MainWindow::slotEditNodeLabelSize(const int v1, int newSize)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeLabelSize - newSize " << newSize;
    if (!newSize)
    {
        newSize = QInputDialog::getInt(this, "Change text size",
                                       tr("Change all node labels text size to: (1-16)"),
                                       appSettings["initNodeLabelSize"].toInt(0, 10), 1, 32, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change font size: Aborted."));
            return;
        }
    }
    if (v1)
    { // change one node Label only
        activeGraph->vertexLabelSizeSet(v1, newSize);
    }
    else
    { // change all
        appSettings["initNodeLabelSize"] = QString::number(newSize);
        activeGraph->vertexLabelSizeSet(0, newSize);
    }
    statusMessage(tr("Changed node label size."));
}

/**
 * @brief Changes the color of all node labels.
 * Asks the user to enter a new node label color
 */
void MainWindow::slotEditNodeLabelsColor(QColor color)
{
    qCDebug(lcMainWindow) << "MW::slotEditNodeNumbersColor() - new color " << color;
    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initNodeLabelColor"]),
                                       this,
                                       "Change the color of all node labels");
    }
    if (color.isValid())
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        activeGraph->vertexLabelColorSet(0, color.name());
        appSettings["initNodeLabelColor"] = color.name();
        optionsNodeLabelsVisibilityAct->setChecked(true);
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Label colors changed. "));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
    }
}

/**
 * @brief MainWindow::slotEditNodeLabelDistance
 * Changes the distance of one or all node label from their nodes.
 * Called from Edit menu option and DialogSettings
 * if newDistance=0, asks the user to enter a new node label distance
 * if v1=0, it changes all node label distances
 * @param v1
 * @param newDistance
 */
void MainWindow::slotEditNodeLabelDistance(int v1, int newDistance)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotEditNodeLabelDistance - newSize " << newDistance;
    if (!newDistance)
    {
        newDistance = QInputDialog::getInt(
            this, "Change node label distance",
            tr("Change all node labels distance from their nodes to: (1-16)"),
            appSettings["initNodeLabelDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change node label distance aborted."));
            return;
        }
    }
    if (v1)
    { // change one node label distance only
        activeGraph->vertexLabelDistanceSet(v1, newDistance);
    }
    else
    { // change all
        appSettings["initNodeLabelDistance"] = QString::number(newDistance);
        activeGraph->vertexLabelDistanceAllSet(newDistance);
    }
    statusMessage(tr("Changed node label distance."));
}

/**
 * @brief Shows a context menu when the user right-clicks directly on a node.
 *
 * The menu is always selection-aware — it reflects the total number of currently
 * selected nodes, not just the node that was right-clicked:
 * - Single node selected: offers Node Properties (full per-node dialog) and
 *   "Edit Selection in Data Table" to navigate to that row in the table.
 * - Multiple nodes selected: offers "Edit Selection in Data Table",
 *   "Set property on selected nodes…", and structural transforms instead of
 *   per-node properties (which would be ambiguous for mixed selections).
 * - Always offers Add Edge, Remove, filter actions, and Ego Radial Layout.
 */
void MainWindow::slotEditNodeOpenContextMenu()
{

    qCDebug(lcMainWindow, "MW: slotEditNodeOpenContextMenu() for node %i at %i, %i",
           activeGraph->vertexClicked(), QCursor::pos().x(), QCursor::pos().y());

    QMenu *nodeContextMenu = new QMenu(QString::number(activeGraph->vertexClicked()), this);
    Q_CHECK_PTR(nodeContextMenu); // displays "out of memory" if needed
    int nodesSelected = activeGraph->getSelectedVerticesCount();
    if (nodesSelected == 1)
    {
        nodeContextMenu->addAction(
            tr("## NODE ") + QString::number(activeGraph->vertexClicked()) + " ##  ");
    }
    else
    {
        nodeContextMenu->addAction(
            tr("## NODE ") + QString::number(activeGraph->vertexClicked()) + " ##  " + tr(" (selected nodes: ") + QString::number(nodesSelected) + ")");
    }

    nodeContextMenu->addSeparator();

    if (nodesSelected == 1)
    {
        // Single node: full per-node properties dialog is appropriate
        nodeContextMenu->addAction(editNodePropertiesAct);
    }
    else
    {
        // Multiple nodes: bulk-set shortcut instead of the ambiguous single-node dialog
        nodeContextMenu->addAction(editNodeSetPropertyForSelectionAct);
    }
    // Always offer "Edit in Data Table" so the user can inspect or bulk-edit
    // the selected row(s) regardless of how many nodes are selected
    nodeContextMenu->addAction(editNodeEditSelectionInTableAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editEdgeAddAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(editNodeRemoveAct);

    nodeContextMenu->addSeparator();

    nodeContextMenu->addAction(filterNodesBySelectionAct);
    nodeContextMenu->addAction(filterNodesByEgoNetworkAct);
    nodeContextMenu->addAction(filterNodesRestoreAllAct);

    nodeContextMenu->addSeparator();
    nodeContextMenu->addAction(tr("Ego Radial layout"), this, &MainWindow::slotLayoutEgoRadial);
    nodeContextMenu->addSeparator();

    // QCursor::pos() is good only for menus not related with node coordinates
    nodeContextMenu->exec(QCursor::pos());
    delete nodeContextMenu;
}

/**
 * @brief Displays information about the given node on the statusbar.
 *
 * Usually called by Graph, after the user clicks on a node.
 *
 * @param number
 * @param p
 * @param label
 * @param inDegree
 * @param outDegree
 */
void MainWindow::slotEditNodeInfoStatusBar(const int &number,
                                           const QPointF &p,
                                           const QString &label,
                                           const int &inDegree,
                                           const int &outDegree)
{

    qCDebug(lcMainWindow) << "Updating node info in status bar...";
    rightPanelClickedNodeLCD->setText(QString::number(number));
    const bool nodeClicked = (number != 0);
    rightPanelClickedNodeInDegreeLabel->setVisible(nodeClicked);
    rightPanelClickedNodeInDegreeLCD->setVisible(nodeClicked);
    rightPanelClickedNodeOutDegreeLabel->setVisible(nodeClicked);
    rightPanelClickedNodeOutDegreeLCD->setVisible(nodeClicked);
    if (nodeClicked)
    {
        rightPanelClickedNodeInDegreeLCD->setText(QString::number(inDegree));
        rightPanelClickedNodeOutDegreeLCD->setText(QString::number(outDegree));
    }

    if (number != 0)
    {

        statusMessage(QString(tr("Position (%1, %2):  Node %3, label %4 - "
                                 "In-Degree: %5, Out-Degree: %6"))
                          .arg(ceil(p.x()))
                          .arg(ceil(p.y()))
                          .arg(number)
                          .arg((label == "") ? "unset" : label)
                          .arg(inDegree)
                          .arg(outDegree));
    }
    else
    {
        statusMessage(tr("Position (%1,%2): Double-click to create a new node.")
                          .arg(p.x())
                          .arg(p.y()));
    }
}
