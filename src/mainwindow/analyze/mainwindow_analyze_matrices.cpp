/**
 * @file mainwindow_analyze_matrices.cpp
 * @brief Implements MainWindow Analyze menu matrix reports: adjacency (cocitation/inverse/transpose), degree, distances, geodesics, laplacian, reachability, and the symmetry check.
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
 * @brief Called when user selects something in the Matrices
 * selectbox of the toolbox
 * @param selectedIndex
 */
void MainWindow::toolBoxAnalysisMatricesSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected matrix analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotNetworkViewSociomatrix();
        break;
    case 2:
        slotNetworkViewSociomatrixPlotText();
        break;
    case 3:
        slotAnalyzeMatrixAdjacencyInverse();
        break;
    case 4:
        slotAnalyzeMatrixAdjacencyTranspose();
        break;
    case 5:
        slotAnalyzeMatrixAdjacencyCocitation();
        break;
    case 6:
        slotAnalyzeMatrixDegree();
        break;
    case 7:
        slotAnalyzeMatrixLaplacian();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

void MainWindow::slotAnalyzeSymmetryCheck()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric())
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Symmetric network."),
            tr("The adjacency matrix is symmetric."));
    }
    else
    {
        slotHelpMessageToUser(
            USER_MSG_INFO,
            tr("Non symmetric network."),
            tr("The adjacency matrix is not symmetric."));
    }

    statusMessage(QString(tr("Ready")));
}

/**
 * @brief Writes the adjacency matrix inverse
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyInverse()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-inverse-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_ADJACENCY_INVERSE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Inverting adjacency matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Inverse matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the transpose adjacency matrix
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyTranspose()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-adjacency-transpose-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_ADJACENCY_TRANSPOSE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Transposing adjacency matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Transpose adjacency matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the cocitation matrix
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixAdjacencyCocitation()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-cocitation-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_COCITATION, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Cocitation matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Cocitation matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the degree matrix of the graph
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-degree-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_DEGREE, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Degree matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Degree matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes the Laplacian matrix of the graph
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixLaplacian()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    qCDebug(lcMainWindow) << "MW:slotAnalyzeMatrixLaplacian()";

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-laplacian-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_LAPLACIAN, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing Laplacian matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Laplacian matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Invokes calculation of the matrix of geodesic distances for the loaded network, then displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixDistances()
{
    qCDebug(lcMainWindow) << "Request to compute the matrix of geodesic distances. Please wait...";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesic-distances-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_DISTANCES,
                                                considerWeights, inverseWeightsFinal, dropIsolates,
                                                "Rows", false, reportFormat);
        },
        tr("Computing geodesic distances. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Geodesic Distances matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Invokes calculation of the geodedics matrix (the number of shortest paths
 * between each pair of nodes in the loaded network), then displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeMatrixGeodesics()
{
    qCDebug(lcMainWindow) << "Request to compute the matrix of geodesics. Please wait...";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-geodesics-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_GEODESICS,
                                                considerWeights, inverseWeightsFinal, dropIsolates,
                                                "Rows", false, reportFormat);
        },
        tr("Computing geodesics (number of shortest paths) for each pair. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Geodesics Matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calls Graph::writeMatrix(fn, MATRIX_REACHABILITY) to calculate and print
 *   the Reachability Matrix of the network.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeReachabilityMatrix()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-matrix-reachability-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeMatrix(fn, MATRIX_REACHABILITY, true, false, false, "Rows", false, reportFormat);
        },
        tr("Computing reachability matrix. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                statusMessage(tr("Computation canceled."));
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
            statusMessage(tr("Reachability matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}
