/**
 * @file mainwindow_analyze_cohesion.cpp
 * @brief Implements MainWindow Analyze menu cohesion reports: connectedness, node/graph connectivity, clique census.
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
#include "chart.h"
#include "widgets/graphtablewidget.h"
#include "forms/dialogsimilaritypearson.h"
#include "forms/dialogsimilaritymatches.h"
#include "forms/dialogdissimilarities.h"
#include "forms/dialogclusteringhierarchical.h"

#include <QtWidgets>
#include <QtCharts>

/**
 * @brief Called when user selects something in the Cohesion
 * selectbox of the toolbox to compute basic graph theoretic / network properties
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisCohesionSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected cohesion analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotAnalyzeReciprocity();
        break;
    case 2:
        slotAnalyzeSymmetryCheck();
        break;
    case 3:
        slotAnalyzeDistance();
        break;
    case 4:
        slotAnalyzeDistanceAverage();
        break;
    case 5:
        slotAnalyzeGeodesicDistribution();
        break;
    case 6:
        slotAnalyzeMatrixDistances();
        break;
    case 7:
        slotAnalyzeMatrixGeodesics();
        break;
    case 8:
        slotAnalyzeEccentricity();
        break;
    case 9:
        slotAnalyzeDiameter();
        break;
    case 10:
        slotAnalyzeConnectedness();
        break;
    case 11:
        slotAnalyzeWalksLength();
        break;
    case 12:
        slotAnalyzeWalksTotal();
        break;
    case 13:
        slotAnalyzeReachabilityMatrix();
        break;
    case 14:
        slotAnalyzeClusteringCoefficient();
        break;
    case 15:
        slotAnalyzeNodeConnectivity();
        break;
    case 16:
        slotAnalyzeConnectivity();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Called when the user selects something in the Communities selectbox
 * of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisCommunitiesSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected community analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotAnalyzeCommunitiesCliqueCensus();
        break;
    case 2:
        slotAnalyzeCommunitiesTriadCensus();
        break;
    };
    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Reports the network connectedness
 */
void MainWindow::slotAnalyzeConnectedness()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeConnectedness()";

    int N = activeGraph->vertices();

    if (!N)
    {
        // null network with empty graph is connected
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("This empty network is considered connected!"),
            tr("Empty network is considered connected!"),
            tr("A null network (empty graph) is considered connected."));
    }
    else if (N == 1)
    {
        // 1-actor network with singleton graph is connected
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("This 1-actor network is considered connected!"),
            tr("This 1-actor network is considered connected!"),
            tr("A 1-actor network (singleton graph) is always considered connected."));
    }
    else
    {
        // Directed graphs have two distinct notions of connectivity (weak: ignores edge
        // direction; strong: requires all-pairs directed reachability) - ask which one to
        // check rather than silently picking one. Undirected graphs have no such ambiguity:
        // the two notions coincide, so there's nothing to ask.
        bool useStrong = false;
        if (activeGraph->isDirected())
        {
            bool ok = false;
            const QStringList connectivityTypes = {
                tr("Weak (ignores edge direction)"),
                tr("Strong (respects edge direction)")
            };
            const QString choice = QInputDialog::getItem(
                this,
                tr("Connectedness"),
                tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
                   "Weak: treats every edge as bidirectional. Answers \"how many disconnected "
                   "islands are there?\"\n\n"
                   "Strong: respects edge direction. Every node must be able to reach, and be "
                   "reached from, every other node."),
                connectivityTypes, 0, false, &ok);

            if (!ok)
            {
                statusMessage(tr("Connectedness check cancelled."));
                return;
            }
            useStrong = (connectivityTypes.indexOf(choice) == 1);
        }

        const int components = useStrong
            ? activeGraph->graphStronglyConnectedComponents()
            : activeGraph->graphWeaklyConnectedComponents();
        const bool isConnected = (components == 1);

        qCDebug(lcMainWindow) << "MW::slotAnalyzeConnectedness result connected:" << isConnected
                 << "components:" << components << "strong:" << useStrong;

        const QString compStr = QString::number(components);
        if (isConnected)
        {
            if (activeGraph->isDirected())
            {
                if (useStrong)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is strongly connected (1 component)."),
                        tr("This directed network is strongly connected."),
                        tr("Every node can reach, and be reached from, every other node "
                           "via directed paths."));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is weakly connected (1 component)."),
                        tr("This directed network is weakly connected."),
                        tr("All nodes belong to a single weakly connected component. "
                           "Weak connectivity ignores edge direction - run Connectedness again "
                           "and choose Strong to check directed reachability."));
                }
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("This undirected network is connected (1 component)."),
                    tr("This undirected network is connected."),
                    tr("All nodes belong to a single connected component. "
                       "There is a path between every pair of nodes."));
            }
        }
        else
        {
            if (activeGraph->isDirected())
            {
                if (useStrong)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is not strongly connected (%1 components).").arg(compStr),
                        tr("This directed network is not strongly connected."),
                        tr("There are %1 strongly connected components. "
                           "Some node pairs cannot reach each other via directed paths.")
                            .arg(compStr));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("This directed network is disconnected (%1 components).").arg(compStr),
                        tr("This directed network is disconnected."),
                        tr("There are %1 disconnected components. "
                           "Some node pairs are unreachable from each other, even ignoring "
                           "edge direction. "
                           "Use Layout > Node Color by Connected Component "
                           "to visualize the components.")
                            .arg(compStr));
                }
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("This undirected network is disconnected (%1 components).").arg(compStr),
                    tr("This undirected network is disconnected."),
                    tr("There are %1 disconnected components. "
                       "Some node pairs have no path between them. "
                       "Use Layout > Node Color by Connected Component "
                       "to visualize the components.")
                        .arg(compStr));
            }
        }
    }
}

/**
 * @brief Handles requests to compute the local vertex connectivity between two user-specified
 * nodes: the minimum number of other nodes that must be removed to disconnect them.
 */
void MainWindow::slotAnalyzeNodeConnectivity()
{
    if (!activeNodes() || !activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok1 = false, ok2 = false;
    const int min = activeGraph->vertexNumberMin();
    const int max = activeGraph->vertexNumberMax();

    const int sourceNum = QInputDialog::getInt(
        this,
        tr("Node Connectivity"),
        tr("Select source node (%1..%2):")
            .arg(QString::number(min))
            .arg(QString::number(max)),
        min, min, max, 1, &ok1);

    if (!ok1)
    {
        statusMessage(tr("Node connectivity calculation cancelled."));
        return;
    }

    const int targetNum = QInputDialog::getInt(
        this,
        tr("Node Connectivity"),
        tr("Select target node (%1..%2):")
            .arg(QString::number(min), QString::number(max)),
        min, min, max, 1, &ok2);

    if (!ok2)
    {
        statusMessage(tr("Node connectivity calculation cancelled."));
        return;
    }

    bool respectDirection = false;
    if (activeGraph->isDirected())
    {
        bool ok = false;
        const QStringList connectivityTypes = {
            tr("Weak (ignores edge direction)"),
            tr("Strong (respects edge direction)")
        };
        const QString choice = QInputDialog::getItem(
            this,
            tr("Node Connectivity"),
            tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
               "Weak: treats every edge as bidirectional.\n\n"
               "Strong: respects edge direction - counts nodes needed to block directed paths "
               "from the source to the target."),
            connectivityTypes, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Node connectivity calculation cancelled."));
            return;
        }
        respectDirection = (connectivityTypes.indexOf(choice) == 1);
    }

    qCDebug(lcMainWindow) << "Computing node connectivity:" << sourceNum << "->" << targetNum
                          << "respectDirection:" << respectDirection;

    auto result = std::make_shared<Graph::NodeConnectivityResult>();

    runGraphOperationAsync(
        [this, sourceNum, targetNum, respectDirection, result]() {
            *result = activeGraph->graphNodeConnectivity(sourceNum, targetNum, respectDirection);
        },
        tr("Computing node connectivity. Please wait..."),
        [this, sourceNum, targetNum, result]() {
            switch (result->status)
            {
            case Graph::NodeConnectivityStatus::Adjacent:
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Nodes %1 and %2 are directly connected.").arg(sourceNum).arg(targetNum),
                    tr("Nodes %1 and %2 are directly connected.").arg(sourceNum).arg(targetNum),
                    tr("Node connectivity is not defined for directly connected nodes: "
                       "the edge between them means no removal of other nodes can ever "
                       "separate them."));
                break;
            case Graph::NodeConnectivityStatus::Ok:
                if (result->value > 0)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Node Connectivity: %1").arg(result->value),
                        tr("Node Connectivity: %1").arg(result->value),
                        tr("At least %1 other node(s) must be removed to disconnect "
                           "%2 from %3.")
                            .arg(result->value)
                            .arg(targetNum)
                            .arg(sourceNum));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Node Connectivity: %1").arg(QString("\xE2\x88\x9E")),
                        tr("Nodes %1 and %2 are already disconnected.").arg(sourceNum).arg(targetNum),
                        tr("%1 cannot reach %2 at all, so no node removal is needed.")
                            .arg(sourceNum)
                            .arg(targetNum));
                }
                break;
            case Graph::NodeConnectivityStatus::Invalid:
            default:
                slotHelpMessageToUser(
                    USER_MSG_CRITICAL,
                    tr("Invalid node connectivity request."),
                    tr("Invalid node connectivity request."),
                    tr("Source and target must be two distinct, existing nodes."));
                break;
            }
        });
}

/**
 * @brief Handles requests to compute the network's overall vertex connectivity: the minimum,
 * over every pair of nodes, of their local vertex connectivity.
 */
void MainWindow::slotAnalyzeConnectivity()
{
    const int N = activeGraph->vertices();
    if (N < 2)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool respectDirection = false;
    if (activeGraph->isDirected())
    {
        bool ok = false;
        const QStringList connectivityTypes = {
            tr("Weak (ignores edge direction)"),
            tr("Strong (respects edge direction)")
        };
        const QString choice = QInputDialog::getItem(
            this,
            tr("Graph Connectivity"),
            tr("This is a directed network. Which kind of connectivity do you want to check?\n\n"
               "Weak: treats every edge as bidirectional.\n\n"
               "Strong: respects edge direction."),
            connectivityTypes, 0, false, &ok);

        if (!ok)
        {
            statusMessage(tr("Graph connectivity calculation cancelled."));
            return;
        }
        respectDirection = (connectivityTypes.indexOf(choice) == 1);
    }

    qCDebug(lcMainWindow) << "Computing graph connectivity, respectDirection:" << respectDirection;

    auto kappa = std::make_shared<int>(0);

    runGraphOperationAsync(
        [this, respectDirection, kappa]() {
            *kappa = activeGraph->graphConnectivity(respectDirection);
        },
        tr("Computing graph connectivity. Please wait..."),
        [this, kappa]() {
            if (*kappa > 0)
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Graph Connectivity: %1").arg(*kappa),
                    tr("Graph Connectivity: %1").arg(*kappa),
                    tr("At least %1 node(s) must be removed to disconnect some pair "
                       "of nodes in this network - its worst-case robustness to "
                       "node removal.")
                        .arg(*kappa));
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Graph Connectivity: 0"),
                    tr("This network is already disconnected."),
                    tr("Some pair of nodes is already unreachable, so no node removal "
                       "is needed to disconnect the network further."));
            }
        });
}

/**
 * @brief Calls Graph:: writeCliqueCensus() to write the Clique Census
 *  into a file, then displays it.
 */
void MainWindow::slotAnalyzeCommunitiesCliqueCensus()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() == 1)
    {
        slotHelpMessageToUserError("Only one node is present, therefore 1 clique");
        return;
    }

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clique-census-" + dateTime + ".html";

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, success]() {
            *success = activeGraph->writeCliqueCensus(fn, considerWeights);
        },
        tr("Computing Clique Census. Please wait..."),
        [this, fn, success]() {
            if (!*success)
            {
                return;
            }
            if (appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Clique Census saved as: ") + QDir::toNativeSeparators(fn));
        });
}
