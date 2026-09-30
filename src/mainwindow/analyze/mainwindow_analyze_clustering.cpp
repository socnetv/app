/**
 * @file mainwindow_analyze_clustering.cpp
 * @brief Implements MainWindow Analyze menu clustering coefficient, triad census, and reciprocity reports.
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
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeReciprocity()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-reciprocity-" + dateTime + ext;

    askAboutEdgeWeights();

    bool considerWeights = optionsEdgeWeightConsiderAct->isChecked();
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeReciprocity(fn, considerWeights, reportFormat);
        },
        tr("Computing Reciprocity. Please wait..."),
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
            statusMessage(tr("Reciprocity report saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 * @brief Calls Graph::writeClusteringCoefficient() to write Clustering Coefficients
 * into a file, and displays it.
 *
 * Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeClusteringCoefficient()
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-clustering-coefficient-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeClusteringCoefficient(fn, considerWeights, reportFormat);
        },
        tr("Computing Clustering Coefficients. Please wait..."),
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
            statusMessage(tr("Clustering Coefficients saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calls Graph to compute and write a triad census into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 */
void MainWindow::slotAnalyzeCommunitiesTriadCensus()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-triad-census-" + dateTime + ext;

    bool considerWeights = true;
    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, considerWeights, reportFormat, success]() {
            *success = activeGraph->writeTriadCensus(fn, considerWeights, reportFormat);
        },
        tr("Computing Triad Census. Please wait..."),
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
            statusMessage(tr("Triad Census saved as: ") + QDir::toNativeSeparators(fn));
        });
}

/**
 *	Calls Graph to compute and write structural balance into a file, then displays it.
 *
 *  Report format (HTML or CSV) follows the Settings > Reports > Output format preference.
 *
 *  Pre-checks isDirected()/hasNegativeWeight() itself (rather than relying on
 *  writeStructuralBalance()'s plain bool return, which doesn't distinguish the two refusal
 *  reasons - see its own doc comment) so the user gets a precise, actionable message instead of
 *  a silent no-op.
 */
void MainWindow::slotAnalyzeCommunitiesStructuralBalance()
{

    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }

    if (activeGraph->isDirected())
    {
        slotHelpMessageToUserError(
            tr("Structural balance requires an undirected network. "
               "This network is directed - directed-graph balance semantics are not yet supported."));
        return;
    }

    if (!activeGraph->hasNegativeWeight())
    {
        slotHelpMessageToUserError(
            tr("Structural balance requires a signed network (at least one negative-weight edge). "
               "This network has no negative ties."));
        return;
    }

    const int reportFormat = appSettings["initReportsOutputFormat"].toInt();
    const QString ext = (reportFormat == ReportFormat::Csv) ? ".csv" : ".html";
    QString dateTime = QDateTime::currentDateTime().toString(QString("yy-MM-dd-hhmmss"));
    QString fn = appSettings["dataDir"] + "socnetv-report-structural-balance-" + dateTime + ext;

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, fn, reportFormat, success]() {
            *success = activeGraph->writeStructuralBalance(fn, reportFormat);
        },
        tr("Computing Structural Balance. Please wait..."),
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
            statusMessage(tr("Structural Balance saved as: ") + QDir::toNativeSeparators(fn));
        });
}
