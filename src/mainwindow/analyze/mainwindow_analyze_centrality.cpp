/**
 * @file mainwindow_analyze_centrality.cpp
 * @brief Implements MainWindow Analyze menu centrality and prestige index reports.
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
 *	Writes Out-Degree Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    askAboutEdgeWeights(false);

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-out-degree-" + dateTime + ext;

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityDegree(fn, considerWeights, dropIsolates, reportFormat);
        },
        tr("Computing Degree Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Out-Degree Centralities report..."));
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
            statusMessage(tr("Out-Degree Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Closeness Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityCloseness()
{
    qCDebug(lcMainWindow) << "MW::slotAnalyzeCentralityCloseness()";
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    askAboutEdgeWeights();

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-" + dateTime + ext;

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityCloseness(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Closeness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Closeness Centralities report..."));
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
            statusMessage(tr("Closeness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityClosenessIR
 *	Writes Centrality Closeness (based on Influence Range) indices into a file,
 *   then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityClosenessIR()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-closeness-influence-range-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityClosenessInfluenceRange(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Influence Range Closeness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Influence Range Closeness Centralities report..."));
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
            statusMessage(tr("Influence Range Closeness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityBetweenness()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-betweenness-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityBetweenness(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Betweenness Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Betweenness Centralities report..."));
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
            statusMessage(tr("Betweenness Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes Degree Prestige indices (In-Degree Centralities) into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigeDegree()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    if (activeGraph->isSymmetric())
    {
        slotHelpMessageToUser(USER_MSG_INFO,
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("Warning! Running Degree Prestige index on an undirected network."),
                              tr("This network is not directed (undirected graph). "
                                 "The Degree Prestige index counts inbound edges, "
                                 "therefore it is meaningful on directed networks. "
                                 "For undirected networks, such as this one, Degree Prestige is the same as Degree Centrality."));
    }

    askAboutEdgeWeights(false);

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-degree-" + dateTime + ext;

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigeDegree(fn, considerWeights, dropIsolates, reportFormat);
        },
        tr("Computing Degree Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Degree Prestige (in-degree) report..."));
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
            statusMessage(tr("Degree Prestige (in-degree) report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Writes PageRank Prestige indices into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigePageRank()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-pagerank-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigePageRank(fn, dropIsolates, reportFormat);
        },
        tr("Computing PageRank Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening PageRank Prestige report..."));
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
            statusMessage(tr("PageRank Prestige report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzePrestigeProximity
 * Writes Proximity Prestige indices into a file, then displays them.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzePrestigeProximity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-prestige-proximity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writePrestigeProximity(fn, true, false, dropIsolates, reportFormat);
        },
        tr("Computing Proximity Prestige. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
            {
                return;
            }
            statusMessage(tr("Opening Proximity Prestige report..."));
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
            statusMessage(tr("Proximity Prestige report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityInformation
 * Writes Informational Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityInformation()
{

    qCDebug(lcMainWindow) << "MW::slotAnalyzeCentralityInformation()";

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    // WS14: recalibrated 2026-07-31 after L4 removed matrix.cpp's qDebug() formatting tax, which
    // (per real measurement) had been the dominant cost at the old n>200 threshold -- the previous
    // "2 minutes for 600 nodes on an i7 4790K" claim is now off by roughly two orders of magnitude
    // (600 nodes measures ~1.2s post-fix). The genuine O(n^3) matrix-inversion cost this warns
    // about is real and unaffected by that fix, though, and does become slow again at large n --
    // measured (MacBook Pro M5, 24GB RAM): 1000 nodes ~3.1s, 2000 nodes ~15.8s, growing faster than
    // linearly beyond that. Threshold raised accordingly; wording updated to current numbers, to lead
    // with the more important caveat now that speed alone rarely justifies interrupting the user:
    // this computation is not yet cancellable once started (WS5 roadmap, issue I1).
    if (activeNodes() > 2000)
    {
        switch (
            QMessageBox::critical(
                this, "Slow function warning",
                tr("Please note that this function can be <b>SLOW</b> on very large "
                   "networks (n>2000), and cannot currently be canceled once started. <br><br>"
                   "It computes the (n x n) matrix A with: <br>"
                   "Aii=1+weighted_degree_ni <br>"
                   "Aij=1 if (i,j)=0 <br>"
                   "Aij=1-wij if (i,j)=wij <br>"
                   "then computes the inverse matrix C of A via LU decomposition -- an O(n&sup3;) "
                   "operation. <br><br>"
                   "How slow is this? On a modern machine, a 2,000-node network takes about 15 "
                   "seconds; since the cost grows roughly cubically with network size, much larger "
                   "networks (tens of thousands of nodes) can take several minutes or more. <br>"
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
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-information-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, reportFormat, success]() {
            *success = activeGraph->writeCentralityInformation(
                fn, considerWeights, inverseWeightsFinal, reportFormat);
        },
        tr("Computing Information Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Information Centralities report..."));
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
            statusMessage(tr("Information Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Writes Eigenvector Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityEigenvector()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eigenvector-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    const bool dropIsolates = false;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityEigenvector(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eigenvector Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Eigenvector Centralities report..."));
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
            statusMessage(tr("Eigenvector Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityStress
 * Writes Stress Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityStress()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-stress-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityStress(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Stress Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Stress Centralities report..."));
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
            statusMessage(tr("Stress Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityPower
 * Writes Gil-Schmidt Power Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityPower()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-power-Gil-Schmidt-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityPower(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Power Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Gil-Schmidt Power Centralities report..."));
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
            statusMessage(tr("Gil-Schmidt Power Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief MainWindow::slotAnalyzeCentralityEccentricity
 * Writes Eccentricity Centralities into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCentralityEccentricity()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-centrality-eccentricity-" + dateTime + ext;

    askAboutEdgeWeights();

    const bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    const bool dropIsolates = editFilterNodesIsolatesAct->isChecked();
    const bool inverseWeightsFinal = inverseWeights;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat, success]() {
            *success = activeGraph->writeCentralityEccentricity(
                fn, considerWeights, inverseWeightsFinal, dropIsolates, reportFormat);
        },
        tr("Computing Eccentricity Centralities. Please wait..."),
        [this, fn, reportFormat, success]() {
            if (!*success)
                return;
            statusMessage(tr("Opening Closeness Centralities report..."));
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
            statusMessage(tr("Eccentricity Centralities report saved as: ") + QDir::toNativeSeparators(fn));
        });
}
