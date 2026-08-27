/**
 * @file mainwindow_analyze_distance.cpp
 * @brief Implements MainWindow Analyze menu distance-based reports: diameter, distance, average distance, eccentricity, geodesic distribution, walks.
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
 * @brief Handles requests to compute the graph/geodesic distance between two user-specified nodes
 *
 * The geodesic distance of two nodes is the length of the shortest path between them.
 *
 */
void MainWindow::slotAnalyzeDistance()
{
    if (!activeNodes() || !activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    bool ok1 = false, ok2 = false;
    int min = 1, max = 1, sourceNum = -1, targetNum = -1;

    min = activeGraph->vertexNumberMin();
    max = activeGraph->vertexNumberMax();

    sourceNum = QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select source node (%1..%2):")
            .arg(QString::number(min))
            .arg(QString::number(max)),
        min, min, max, 1, &ok1);

    if (!ok1)
    {
        statusMessage("Distance calculation operation cancelled.");
        return;
    }

    targetNum = QInputDialog::getInt(
        this,
        tr("Distance between two nodes"),
        tr("Select target node (%1..%2):")
            .arg(QString::number(min), QString::number(max)),
        min, min, max, 1, &ok2);

    if (!ok2)
    {
        statusMessage(tr("Distance calculation operation cancelled."));
        return;
    }

    qCDebug(lcMainWindow) << "Computing geodesic distance:" << sourceNum << "->" << targetNum;

    if (activeGraph->isSymmetric() && sourceNum > targetNum)
    {
        qSwap(sourceNum, targetNum);
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto distanceGeodesic = std::make_shared<int>(0);

    runGraphOperationAsync(
        [this, sourceNum, targetNum, considerWeights, inverseWeightsFinal, distanceGeodesic]() {
            *distanceGeodesic = activeGraph->graphDistanceGeodesic(
                sourceNum, targetNum, considerWeights, inverseWeightsFinal);
        },
        tr("Computing geodesic distance. Please wait..."),
        [this, sourceNum, targetNum, considerWeights, inverseWeightsFinal, distanceGeodesic]() {
            if (*distanceGeodesic > 0 && *distanceGeodesic < RAND_MAX)
            {
                qCDebug(lcMainWindow) << "geodesic distance" << sourceNum << "->" << targetNum << "=" << *distanceGeodesic;

                // Reconstruct the actual shortest path so the user sees the intermediate nodes.
                // Cheap regardless of network size (single-source BFS/Dijkstra, not the full
                // APSP the call above just triggered) - safe to run synchronously here.
                const QList<int> path = activeGraph->graphGeodesicShortestPath(
                    sourceNum, targetNum, considerWeights, inverseWeightsFinal);

                // Format the path as "v1 → v2 → … → vN" using node labels where available.
                QString pathStr;
                if (path.size() >= 2) {
                    for (int i = 0; i < path.size(); ++i) {
                        if (i > 0) pathStr += " \xE2\x86\x92 ";   // → (UTF-8 right arrow)
                        const QString lbl = activeGraph->vertexLabel(path[i]).trimmed();
                        pathStr += lbl.isEmpty() ? QString::number(path[i]) : lbl;
                    }
                }

                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(*distanceGeodesic),
                    tr("Geodesic Distance: %1").arg(*distanceGeodesic),
                    tr("Nodes %1 and %2 are connected. The shortest path has length %3.\n\n"
                       "Shortest path:\n%4")
                        .arg(sourceNum)
                        .arg(targetNum)
                        .arg(*distanceGeodesic)
                        .arg(pathStr.isEmpty() ? tr("(path unavailable)") : pathStr));

                if (path.size() >= 2)
                    graphicsWidget->selectPath(path);
            }
            else
            {
                qCDebug(lcMainWindow) << "geodesic distance" << sourceNum << "->" << targetNum << "is infinite.";
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Geodesic Distance: %1").arg(QString("\xE2\x88\x9E")),
                    tr("Nodes %1 and %2 are not connected. "
                       "In this case, their geodesic distance is considered to be infinite.")
                        .arg(sourceNum)
                        .arg(targetNum));
            }
        });
}

/**
 * @brief Displays the network diameter (largest geodesic)
 */
void MainWindow::slotAnalyzeDiameter()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto netDiameter = std::make_shared<int>(0);
    auto isWeighted = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, considerWeights, inverseWeightsFinal, netDiameter, isWeighted]() {
            *netDiameter = activeGraph->graphDiameter(considerWeights, inverseWeightsFinal);
            *isWeighted = activeGraph->isWeighted();
        },
        tr("Computing graph diameter. Please wait..."),
        [this, considerWeights, netDiameter, isWeighted]() {
            if (*isWeighted)
            {
                if (considerWeights)
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                           "D = %1")
                            .arg(*netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                           "(maximum shortest path length) between any two nodes.\n\n"
                           "Note, since this is a weighted network, "
                           "the diameter can be greater than N."));
                }
                else
                {
                    slotHelpMessageToUser(
                        USER_MSG_INFO,
                        tr("Network diameter computed."),
                        tr("Network diameter computed. \n\n"
                           "D = %1")
                            .arg(*netDiameter),
                        tr("The diameter of a network is the maximum geodesic distance "
                           "(maximum shortest path length) between any two nodes.\n\n"
                           "Note, edge weights were disregarded during the computation. "
                           "This is the diameter of the corresponding network without weights."));
                }
            }
            else
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Network diameter computed."),
                    tr("Network diameter computed. \n\n"
                       "D = %1")
                        .arg(*netDiameter),
                    tr("The diameter of a network is the maximum geodesic distance "
                       "(maximum shortest path length) between any two nodes.\n\n"
                       "Note, since this is a non-weighted network, the diameter is always smaller than N-1."));
        });
}

/**
 * @brief Displays the average shortest path length (average graph distance)
 */
void MainWindow::slotAnalyzeDistanceAverage()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto averGraphDistance = std::make_shared<qreal>(0);
    auto isConnected = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, considerWeights, inverseWeightsFinal, dropIsolates, averGraphDistance, isConnected]() {
            *averGraphDistance = activeGraph->graphDistanceGeodesicAverage(
                considerWeights, inverseWeightsFinal, dropIsolates);
            // Cheap here: graphDistanceGeodesicAverage() just triggered the full APSP with
            // these exact params, so isConnected()'s cache hit is immediate - not a second
            // expensive computation.
            *isConnected = activeGraph->isConnected();
        },
        tr("Computing Average Graph Distance. Please wait..."),
        [this, averGraphDistance, isConnected]() {
            if (*isConnected)
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Average graph distance computed."),
                    tr("Average graph distance computed. \n\n"
                       "d = %1")
                        .arg(*averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                       "for all possible pairs of nodes.\n\n"
                       "The average distance in this connected network "
                       "is the sum of pair-wise distances divided by N * (N - 1)."));
            }
            else
            {
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Average distance computed."),
                    tr("Average distance computed. \n\n"
                       "d = %1")
                        .arg(*averGraphDistance),
                    tr("The average graph distance is the average length of shortest paths (geodesics) "
                       "for all possible pairs of nodes.\n\n"
                       "The average distance in this disconnected network "
                       "is the sum of pair-wise distances divided by the number of existing geodesics."));
            }
        });
}

/**
 * @brief Computes and displays the distribution of geodesic distances across
 * all ordered pairs of connected nodes as an HTML report.
 *
 * Triggered only by explicit user action (Analyze → Cohesion → Geodesic Distance
 * Distribution).  Uses the APSP cache when available; otherwise runs the full
 * APSP computation first.  Writes the result to an HTML file and opens it in the
 * system browser or the built-in TextEditor according to app settings.
 */
void MainWindow::slotAnalyzeGeodesicDistribution()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeGeodesicDistribution()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights();

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"]
                 + "socnetv-report-geodesic-distribution-" + dateTime + ".html";

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, success]() {
            *success = activeGraph->writeGeodesicDistribution(fn, considerWeights, inverseWeightsFinal);
        },
        tr("Computing geodesic distance distribution. Please wait..."),
        [this, fn, success]() {
            if (!*success)
            {
                statusMessage(tr("Error: could not write geodesic distribution report."));
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
            statusMessage(tr("Geodesic distance distribution saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Eccentricity indices into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeEccentricity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-eccentricity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeEccentricity(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eccentricity. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Eccentricities saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calculate and print the number of walks of a given length , between each pair of nodes.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeWalksLength()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    bool ok = false;

    int length = QInputDialog::getInt(
        this, "Number of walks",
        tr("Select desired length of walk: (2 to %1)").arg(activeNodes() - 1),
        2, 2, activeNodes() - 1, 1, &ok);
    if (!ok)
    {
        statusMessage("Cancelled.");
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-length-" + QString::number(length) + "-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);
    runGraphOperationAsync(
        [this, fn, length, reportFormat, success]() {
            *success = activeGraph->writeMatrixWalks(fn, length, false, reportFormat);
        },
        tr("Computing walks of length %1. Please wait...").arg(length),
        [this, fn, length, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage(tr("Walks of length %1 matrix saved as: ").arg(length) + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calculate and print the total number of walks of any length, between each pair of nodes.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeWalksTotal()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeNodes() > 50)
    {
        switch (QMessageBox::critical(
            this,
            "Slow function warning",
            tr("Please note that this function is VERY SLOW on large networks (n>50), "
               "since it will calculate all powers of the sociomatrix up to n-1 "
               "in order to find out all possible walks. \n\n"
               "If you need to make a simple reachability test, "
               "we advise to use the Reachability Matrix function instead. \n\n"
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

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-walks-total-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);
    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrixWalks(fn, 0, false, reportFormat);
        },
        tr("Computing total walks matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled, or could not write to file."));
                return;
            }
            if (reportFormat == ReportFormat::Csv || appSettings["viewReportsInSystemBrowser"] == "true")
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
            }
            else
            {
                TextEditor *ed = new TextEditor(fn, this, true);
                ed->show();
                m_textEditors << ed;
            }
            statusMessage("Total walks matrix saved as: " + QDir::toNativeSeparators(fn));
        });
}
