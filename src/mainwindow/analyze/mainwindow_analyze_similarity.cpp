/**
 * @file mainwindow_analyze_similarity.cpp
 * @brief Implements MainWindow Analyze menu structural equivalence reports: similarity measures, dissimilarities/tie profile, Pearson, hierarchical clustering.
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
 * @brief Called when the user selects something in the Structural Equivalence
 * selectbox of the toolbox
 * @param selectedIndex
 *
 */
void MainWindow::toolBoxAnalysisStrEquivalenceSelectChanged(const int &selectedIndex)
{
    qCDebug(lcMainWindow) << "selected struct. equivalence analysis, text index: " << selectedIndex;
    switch (selectedIndex)
    {
    case 0:
        break;
    case 1:
        slotAnalyzeStrEquivalencePearsonDialog();
        break;
    case 2:
        slotAnalyzeStrEquivalenceSimilarityMeasureDialog();
        break;
    case 3:
        slotAnalyzeStrEquivalenceDissimilaritiesDialog();
        break;
    case 4:
        slotAnalyzeStrEquivalenceClusteringHierarchicalDialog();
        break;
    };

    qCDebug(lcMainWindow) << "Calling initComboBoxes() ";
    initComboBoxes();
}

/**
 * @brief Displays the DialogSimilarityMatches dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceSimilarityMeasureDialog()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    m_dialogSimilarityMatches = new DialogSimilarityMatches(this);

    connect(m_dialogSimilarityMatches, &DialogSimilarityMatches::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure);

    m_dialogSimilarityMatches->exec();
}

/**
 * @brief Calls Graph::writeMatrixSimilarityMatching() to write a
 * similarity matrix according to given measure into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalenceSimilarityByMeasure(const QString &matrix,
                                                              const QString &varLocation,
                                                              const QString &measure,
                                                              const bool &diagonal)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString metric;
    if (measure.contains("Simple", Qt::CaseInsensitive))
        metric = "simple-matching";
    else if (measure.contains("Jaccard", Qt::CaseInsensitive))
        metric = "jaccard";
    else if (measure.contains("None", Qt::CaseInsensitive))
        metric = "none";
    else if (measure.contains("Hamming", Qt::CaseInsensitive))
        metric = "hamming";
    else if (measure.contains("Cosine", Qt::CaseInsensitive))
        metric = "cosine";
    else if (measure.contains("Euclidean", Qt::CaseInsensitive))
        metric = "euclidean";
    else if (measure.contains("Manhattan", Qt::CaseInsensitive))
        metric = "manhattan";
    else if (measure.contains("Pearson ", Qt::CaseInsensitive))
        metric = "pearson";
    else if (measure.contains("Chebyshev", Qt::CaseInsensitive))
        metric = "chebyshev";

    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-similarity-" + metric + "-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, measure, matrix, varLocation, diagonal, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeMatrixSimilarityMatching(
                fn, measure, matrix, varLocation, diagonal, considerWeights, reportFormat);
        },
        tr("Computing Similarity Matrix. Please wait..."),
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
            statusMessage(tr("Similarity matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the DialogDissimilarities dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceDissimilaritiesDialog()";

    m_dialogdissimilarities = new DialogDissimilarities(this);

    connect(m_dialogdissimilarities, &DialogDissimilarities::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile);

    m_dialogdissimilarities->exec();
}

/**
 * @brief Invokes calculation of pair-wise tie profile dissimilarities of the
 * network, then displays it.
 * @param metric
 * @param varLocation
 * @param diagonal
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile(const QString &metric,
                                                                    const QString &varLocation,
                                                                    const bool &diagonal)
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceDissimilaritiesTieProfile()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString metricStr;
    if (metric.contains("Simple", Qt::CaseInsensitive))
        metricStr = "simple-matching";
    else if (metric.contains("Jaccard", Qt::CaseInsensitive))
        metricStr = "jaccard";
    else if (metric.contains("None", Qt::CaseInsensitive))
        metricStr = "none";
    else if (metric.contains("Hamming", Qt::CaseInsensitive))
        metricStr = "hamming";
    else if (metric.contains("Cosine", Qt::CaseInsensitive))
        metricStr = "cosine";
    else if (metric.contains("Euclidean", Qt::CaseInsensitive))
        metricStr = "euclidean";
    else if (metric.contains("Manhattan", Qt::CaseInsensitive))
        metricStr = "manhattan";
    else if (metric.contains("Pearson ", Qt::CaseInsensitive))
        metricStr = "pearson";
    else if (metric.contains("Chebyshev", Qt::CaseInsensitive))
        metricStr = "chebyshev";

    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-dissimilarities-" + metricStr + "-" + dateTime + ext;

    askAboutEdgeWeights();

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, metric, varLocation, diagonal, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeMatrixDissimilarities(fn, metric, varLocation, diagonal,
                                                                considerWeights, reportFormat);
        },
        tr("Computing Tie Profile Dissimilarities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
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
            statusMessage(tr("Tie profile dissimilarities matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calls the m_dialogSimilarityPearson to display the Pearson statistics dialog
 */
void MainWindow::slotAnalyzeStrEquivalencePearsonDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalencePearsonDialog()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    m_dialogSimilarityPearson = new DialogSimilarityPearson(this);

    connect(m_dialogSimilarityPearson, &DialogSimilarityPearson::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalencePearson);

    m_dialogSimilarityPearson->exec();
}

/**
 * @brief Calls Graph::writeMatrixSimilarityPearson() to write Pearson
 * Correlation Coefficients into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeStrEquivalencePearson(const QString &matrix,
                                                  const QString &varLocation,
                                                  const bool &diagonal)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-pearson-coefficients-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, matrix, varLocation, diagonal, reportFormat, success]() {
            *success = activeGraph->writeMatrixSimilarityPearson(
                fn, considerWeights, matrix, varLocation, diagonal, reportFormat);
        },
        tr("Computing Pearson Correlation Coefficients. Please wait..."),
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
            statusMessage(tr("Pearson correlation coefficients matrix saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Displays the slotAnalyzeStrEquivalenceClusteringHierarchicalDialog dialog.
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    QString preselectMatrix = "Adjacency";

    if (!activeGraph->isWeighted())
    {
        preselectMatrix = "Distances";
    }
    m_dialogClusteringHierarchical = new DialogClusteringHierarchical(this, preselectMatrix);

    connect(m_dialogClusteringHierarchical, &DialogClusteringHierarchical::userChoices,
            this, &MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchical);

    m_dialogClusteringHierarchical->exec();
}

/**
 * @brief Called from DialogClusteringHierarchical with user choices. Calls
 * Graph::writeClusteringHierarchical() to compute and write HCA and displays the report.
 * @param matrix
 * @param similarityMeasure
 * @param linkageCriterion
 * @param diagonal
 */
void MainWindow::slotAnalyzeStrEquivalenceClusteringHierarchical(const QString &matrix,
                                                                 const QString &varLocation,
                                                                 const QString &metric,
                                                                 const QString &method,
                                                                 const bool &diagonal,
                                                                 const bool &diagram)
{

    qCDebug(lcMainWindow) << "MW::slotAnalyzeStrEquivalenceClusteringHierarchical()";

    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-equivalence-hierarchical-clustering-" + dateTime + ".html";

    bool considerWeights = activeGraph->isWeighted();
    bool inverseWeights = false;
    bool dropIsolates = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, varLocation, matrix, metric, method, diagonal, diagram,
         considerWeights, inverseWeights, dropIsolates, success]() {
            *success = activeGraph->writeClusteringHierarchical(fn,
                                                                 varLocation,
                                                                 matrix,
                                                                 metric,
                                                                 method,
                                                                 diagonal,
                                                                 diagram,
                                                                 considerWeights,
                                                                 inverseWeights,
                                                                 dropIsolates);
        },
        tr("Computing Hierarchical Cluster Analysis. Please wait..."),
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
            statusMessage(tr("Hierarchical Cluster Analysis saved as: ") + QDir::toNativeSeparators(fn));
        });
}
