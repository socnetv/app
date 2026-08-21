/**
 * @file mainwindow_layout.cpp
 * @brief Implements MainWindow Layout menu actions: colorization, random/radial/spring-embedder/Fruchterman-Reingold/Kamada-Kawai layouts, layouts by prominence index, and the layout toolbox apply buttons.
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
#include "texteditor.h"
#include "forms/dialogsettings.h"

#include <QtWidgets>

/**
 * @brief Called when the user selects a Prominence index in the Layout selectbox
 *  of the Control Panel.
 */
void MainWindow::toolBoxLayoutByIndexApplyBtnPressed()
{
    qCDebug(lcMainWindow) << "User request to apply prominence-based layout...";
    int selectedIndex = toolBoxLayoutByIndexSelect->currentIndex();
    QString selectedIndexText = toolBoxLayoutByIndexSelect->currentText();
    int selectedLayoutType = toolBoxLayoutByIndexTypeSelect->currentIndex();
    qCDebug(lcMainWindow) << "elected index is "
             << selectedIndexText << " : " << selectedIndex
             << " selected layout type is " << selectedLayoutType;
    switch (selectedIndex)
    {
    case 0: // None
        break;
    case 1: // Random
        if (selectedLayoutType == 1)
            slotLayoutRadialRandom();
        else if (selectedLayoutType == 2)
            slotLayoutRandom();
        break;
    default:
        if (selectedLayoutType == 0) // None type — do nothing
            break;
        else if (selectedLayoutType == 1) // Radial
            slotLayoutRadialByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 2) // On Levels
            slotLayoutLevelByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 3) // Node Size
            slotLayoutNodeSizeByProminenceIndex(selectedIndexText);
        else if (selectedLayoutType == 4) // Node Color
            slotLayoutNodeColorByProminenceIndex(selectedIndexText);
        break;
    };
}

/**
 * @brief Called when the user selects a model in the Layout by Force Directed
 * selectbox of left panel.
 */
void MainWindow::toolBoxLayoutForceDirectedApplyBtnPressed()
{
    qCDebug(lcMainWindow) << "User selected to apply a FDP layout...";
    int selectedModel = toolBoxLayoutForceDirectedSelect->currentIndex();
    QString selectedModelText = toolBoxLayoutForceDirectedSelect->currentText();
    qCDebug(lcMainWindow) << " selected index is " << selectedModelText << " : "
             << selectedModel;

    switch (selectedModel)
    {
    case 0:
        break;
    case 1:
        slotLayoutGuides(false);
        slotLayoutKamadaKawai();
        break;
    case 2:
        slotLayoutGuides(false);
        slotLayoutFruchterman();
        break;
    case 3:
        slotLayoutGuides(false);
        slotLayoutSpringEmbedder();
        break;
    default:
        toolBoxLayoutForceDirectedSelect->blockSignals(true);
        toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
        toolBoxLayoutForceDirectedSelect->blockSignals(false);
        break;
    };

    // FD model controls node positions — reset prominence Type if it was
    // Radial (1) or On Levels (2), but leave Node Size/Color (3/4) intact.
    if (selectedModel > 0)
    {
        int currentType = toolBoxLayoutByIndexTypeSelect->currentIndex();
        if (currentType == 1 || currentType == 2)
        {
            toolBoxLayoutByIndexTypeSelect->blockSignals(true);
            toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);
            toolBoxLayoutByIndexTypeSelect->blockSignals(false);
        }
    }
}

/**
    TODO slotLayoutColorationStrongStructural
*/
void MainWindow::slotLayoutColorationStrongStructural()
{
}

/**
    TODO slotLayoutColorationRegular
*/
void MainWindow::slotLayoutColorationRegular()
{
}

/**
 * @brief Calls Graph::layoutRandom
 * to reposition all nodes on a random layout
 */
void MainWindow::slotLayoutRandom()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    graphicsWidget->clearGuides();

    runGraphOperationAsync(
        [this]() {
            activeGraph->layoutRandom();
        },
        tr("Placing nodes in random positions. Please wait..."),
        tr("Nodes in random positions."));
}

/**
 * @brief Calls Graph::layoutRadialRandom
 * to reposition all nodes on a radial layout randomly
 */
void MainWindow::slotLayoutRadialRandom()
{
    qCDebug(lcMainWindow) << "MainWindow::slotLayoutRadialRandom()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    slotLayoutGuides(false);

    runGraphOperationAsync(
        [this]() {
            activeGraph->layoutRadialRandom(true);
        },
        tr("Placing nodes in random concentric circles. Please wait..."),
        [this]() {
            slotLayoutGuides(true);
            statusMessage(tr("Nodes in random concentric circles."));
        });
}

/**
 * @brief Calls Graph::layoutEgoRadial.
 * Resolves the ego vertex from the current selection (exactly one selected)
 * or from the last-clicked vertex, whichever is available.
 */
void MainWindow::slotLayoutEgoRadial()
{
    qCDebug(lcMainWindow) << "MainWindow::slotLayoutEgoRadial()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int egoVertex = 0;
    if (activeGraph->getSelectedVerticesCount() == 1)
        egoVertex = activeGraph->getSelectedVertices().first();
    else if (activeGraph->vertexClicked() != 0)
        egoVertex = activeGraph->vertexClicked();
    else
    {
        statusMessage(tr("Please select or click one node first."));
        return;
    }
    runGraphOperationAsync(
        [this, egoVertex]() {
            activeGraph->layoutEgoRadial(egoVertex);
        },
        tr("Computing ego radial layout. Please wait..."),
        [this, egoVertex]() {
            statusMessage(tr("Ego radial layout centered on vertex %1.").arg(egoVertex));
        });
}

/**
 * @brief Embed the Eades spring-gravitational model to the network.
 * Called from menu or toolbox checkbox.
 */
void MainWindow::slotLayoutSpringEmbedder()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutSpringEmbedder";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    // Eades (1984) designed this algorithm for small graphs (N < 30).
    // For larger graphs the system frequently gets stuck in local minima
    // and the O(N²×I) complexity makes it slow. Warn the user.
    if (activeNodes() > 30)
    {
        QMessageBox::warning(
            this,
            tr("Eades Spring Embedder — Size Warning"),
            tr("The Eades Spring Embedder was designed for graphs with fewer than 30 nodes.\n\n"
               "This network has %1 nodes. The layout may get stuck in local minima "
               "and the result may not be aesthetically pleasing.\n\n"
               "Consider using the Fruchterman-Reingold or Kamada-Kawai models instead, "
               "which handle larger graphs better.")
                .arg(activeNodes()));
    }
    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedSpringEmbedder(300); },
        tr("Computing Spring-Gravitational (Eades) layout. Please wait..."),
        tr("Spring-Gravitational (Eades) model embedded."));
}

/**
 * @brief Calls Graph::layoutForceDirectedFruchtermanReingold to embed
 * the Fruchterman-Reingold model of repelling-attracting forces to the network.
 * Called from menu or toolbox
 */
void MainWindow::slotLayoutFruchterman()
{
    qCDebug(lcMainWindow, "MW: slotLayoutFruchterman ()");
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedFruchtermanReingold(100); },
        tr("Computing Fruchterman & Reingold layout. Please wait..."),
        tr("Fruchterman & Reingold model embedded."));
}

/**
 * @brief Layouts the network according to the Kamada-Kawai FDP model
 */
void MainWindow::slotLayoutKamadaKawai()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutKamadaKawai ()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    runGraphOperationAsync(
        [this]() { activeGraph->layoutForceDirectedKamadaKawai(400); },
        tr("Computing Kamada & Kawai layout. Please wait..."),
        tr("Kamada & Kawai model embedded."));
}

/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutRadialByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a radial layout by prominence index. Checking what index is requested...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
    QString menuItemText = menuitem->text();

    slotLayoutRadialByProminenceIndex(menuItemText);
}

/**
 * @brief Applies a radial layout on the social network, where each node is placed on concentric circles according to their index score.
 *
 *  More prominent nodes are closer to the centre of the screen.
 *
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutRadialByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a radial layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    slotLayoutGuides(true);

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(1); // Radial
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    toolBoxLayoutForceDirectedSelect->blockSignals(true);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0); // None — FD position overridden
    toolBoxLayoutForceDirectedSelect->blockSignals(false);

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 0,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 radial layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes in inner circles have higher %1 score. ").arg(prominenceIndexName));
}


/**
 * @brief Runs when the user selects a radial layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutLevelByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a level layout by prominence index. Checking what index is requested...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
    QString menuItemText = menuitem->text();

    slotLayoutLevelByProminenceIndex(menuItemText);
}

/**
 * @brief Applies a level layout on the social network, where each node is placed on different top-down levels according to their index score.
 *
 *  More prominent nodes are closer to the the top of the screen
 *
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutLevelByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a level layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    slotLayoutGuides(true);

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(2); // On Levels
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    toolBoxLayoutForceDirectedSelect->blockSignals(true);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0); // None — FD position overridden
    toolBoxLayoutForceDirectedSelect->blockSignals(false);

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 1,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 level layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes in upper levels have higher %1 score. ").arg(prominenceIndexName));
}


/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeSizeByProminenceIndex()
{
    qCDebug(lcMainWindow) << "Got request to apply a color layout by prominence index. Checking what index is requested...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
    QString menuItemText = menuitem->text();

    slotLayoutNodeSizeByProminenceIndex(menuItemText);
}

/**
 * @brief Applies a node size layout on the social network, where the size of each of node is analogous to their index score.
 *
 *  More prominent nodes are bigger.
 *
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutNodeSizeByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a node size layout by prominence index: " << prominenceIndexName;

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(3); // Node Size
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    // Node size coexists with FD positional layout — don't reset FD combobox

    bool dropIsolates = false;

    if (indexType == IndexType::IC && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 2,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 node size layout. Please wait...").arg(prominenceIndexName),
        tr("Bigger nodes have greater %1 score.").arg(prominenceIndexName));
}


/**
 * @brief Runs when the user selects a color layout menu option
 *
 * Checks sender text() to find out what QMenu item was pressed and so the requested index
 *
 */
void MainWindow::slotLayoutNodeColorByProminenceIndex()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QAction *menuitem = (QAction *)sender();
    QString menuItemText = menuitem->text();

    slotLayoutNodeColorByProminenceIndex(menuItemText);
}

/**
 * @brief Applies a color layout on the social network. Changes the colors of all nodes according to their index score.
 *
 *  More prominent nodes have more warm colors
 *
 * RED=rgb(255,0,0) most prominent
 * BLUE=rgb(0,0,255) least prominent
 *
 * @param prominenceIndexName
 */
void MainWindow::slotLayoutNodeColorByProminenceIndex(QString prominenceIndexName = "")
{
    qCDebug(lcMainWindow) << "Will apply a node color layout by prominence index: " << prominenceIndexName;
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    int indexType = 0;

    indexType = activeGraph->getProminenceIndexByName(prominenceIndexName);

    qCDebug(lcMainWindow) << "indexType" << indexType;

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(indexType + 1);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(4); // Node Color
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    // Node color coexists with FD positional layout — don't reset FD combobox

    bool dropIsolates = false;

    if (indexType == 8 && activeNodes() > 200)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function is <b>CPU-intensive</b> on large "
                   "networks (n>200), since it will calculate  a (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "Next, it will compute the inverse matrix C of A. "
                   "The computation of the inverse matrix is a CPU intensive function "
                   "although it uses LU decomposition. <br>"
                   "This can take a while on large networks, but the app stays responsive "
                   "while it works in the background. <br>"
                   "Are you sure you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
        case QMessageBox::Ok:
            break;

        case QMessageBox::Cancel:
            // Cancel was clicked
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    askAboutEdgeWeights();

    graphicsWidget->clearGuides();

    // Read UI/member state on the GUI thread now - the lambda below runs on graphThread
    // (see runGraphOperationAsync), where reading QAction/MainWindow state would be unsafe.
    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolatesFinal = editFilterNodesIsolatesAct->isChecked() || dropIsolates;
    const bool inverseWeightsFinal = inverseWeights;

    runGraphOperationAsync(
        [this, indexType, considerWeights, inverseWeightsFinal, dropIsolatesFinal]() {
            activeGraph->layoutByProminenceIndex(
                indexType, 3,
                considerWeights,
                inverseWeightsFinal,
                dropIsolatesFinal);
        },
        tr("Computing %1 node color layout. Please wait...").arg(prominenceIndexName),
        tr("Nodes with warmer color have greater %1 score.").arg(prominenceIndexName));
}


/**
 * @brief Colors nodes by their weakly connected component.
 *
 * Runs a BFS to identify weakly connected components and assigns a distinct
 * color to each one. If the network is already fully connected (1 component),
 * reports that and leaves colors unchanged.
 */
void MainWindow::slotLayoutNodeColorByComponent()
{
    qCDebug(lcMainWindow) << "MW::slotLayoutNodeColorByComponent()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int components = activeGraph->graphWeaklyConnectedComponents();

    if (components <= 1)
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Network is fully connected — only one component."),
            tr("Network is fully connected."),
            tr("All nodes belong to a single connected component. No coloring applied."));
        return;
    }

    // Visually distinct palette — cycles if there are more components than entries
    static const QStringList palette = {
        "#e74c3c", "#3498db", "#2ecc71", "#f39c12", "#9b59b6",
        "#1abc9c", "#e67e22", "#34495e", "#e91e63", "#00bcd4",
        "#8bc34a", "#ff5722", "#673ab7", "#009688", "#ffc107"};

    const QHash<int, int> &compMap = activeGraph->vertexComponentId();
    for (auto it = activeGraph->verticesBegin(); it != activeGraph->verticesEnd(); ++it)
    {
        if (!(*it)->isEnabled())
            continue;
        const int compId = compMap.value((*it)->number(), 1);
        activeGraph->vertexColorSet((*it)->number(),
                                    palette.at((compId - 1) % palette.size()));
    }

    statusMessage(tr("Nodes colored by component: %1 components found.").arg(components));
}

/**
 * @brief Shows or hides (clears) layout guides
 *
 * @param toggle
 */
void MainWindow::slotLayoutGuides(const bool &toggle)
{
    qCDebug(lcMainWindow) << "MW:slotLayoutGuides()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    if (toggle)
    {
        layoutGuidesAct->setChecked(true);
        statusMessage(tr("Layout Guides are displayed"));
    }
    else
    {
        layoutGuidesAct->setChecked(false);
        graphicsWidget->clearGuides();
        statusMessage(tr("Layout Guides removed"));
    }
}
