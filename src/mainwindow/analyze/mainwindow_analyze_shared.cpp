/**
 * @file mainwindow_analyze_shared.cpp
 * @brief Implements MainWindow::askAboutEdgeWeights() - the shared modal "use edge weights?" prompt reused across most Analyze menu actions.
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
 * @brief If the network has weighted / valued edges, it asks the user
 * if the app should consider weights or not.
 */
void MainWindow::askAboutEdgeWeights(const bool userTriggered)
{

    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - checking if graph weighted.";

    if (userTriggered)
    {
        if (!activeGraph->isWeighted())
        {
            slotHelpMessageToUser(USER_MSG_INFO,
                                  tr("Non-Weighted Network"),
                                  tr("You do not work on a weighted network at the moment. \n"
                                     "Therefore, I will not consider edge weights during "
                                     "computations. \n"
                                     "This option applies only when you load or create "
                                     "a weighted network "));
            optionsEdgeWeightConsiderAct->setChecked(false);
            return;
        }
    }
    else
    {
        if (!activeGraph->isWeighted())
        {
            optionsEdgeWeightConsiderAct->setChecked(false);
            return;
        }
    }
    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - graph weighted - checking if we have asked user.";

    if (askedAboutWeights)
    {
        return;
    }

    qCDebug(lcMainWindow) << "MW::askAboutEdgeWeights() - graph weighted - let's ask the user.";

    switch (
        slotHelpMessageToUser(USER_MSG_QUESTION,
                              tr("Weighted Network"),
                              tr("This is a weighted network. Consider edge weights?"),
                              tr("The ties in this network have weights (non-unit values) assigned to them. "
                                 "Do you want me to take these edge weights into account (i.e. when computing distances) ?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)

    )
    {
    case QMessageBox::Yes:
        optionsEdgeWeightConsiderAct->setChecked(true);
        break;
    case QMessageBox::No:
        optionsEdgeWeightConsiderAct->setChecked(false);
        break;
    default: // just for sanity
        optionsEdgeWeightConsiderAct->setChecked(false);
        return;
        break;
    }

    if (optionsEdgeWeightConsiderAct->isChecked())
    {
        switch (

            slotHelpMessageToUser(
                USER_MSG_QUESTION, tr("Inverse edge weights during calculations? "),
                tr("Inverse edge weights during calculations? "),
                tr("If the edge weights denote cost or real distances (i.e. miles between cities), "
                   "press No, since the distance between two nodes should be the quickest "
                   "or cheaper one. \n\n"
                   "If the weights denote value or strength (i.e. votes or interaction): for "
                   "distance-based measures (Closeness, Betweenness, Stress, Eccentricity, "
                   "IR Closeness, Power Centrality), press Yes, so a strong tie acts like a "
                   "short path. For measures based on counting walks instead of shortest paths "
                   "(Eigenvector, Katz, Bonacich, PageRank Prestige), press No instead, so a "
                   "strong tie keeps contributing more rather than less. "
                   "See the manual's Edge Weights section for the full explanation."),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)

        )
        {
        case QMessageBox::Yes:
            inverseWeights = true;
            break;
        case QMessageBox::No:
            inverseWeights = false;
            break;
        default: // just for sanity
            inverseWeights = true;
            return;
            break;
        }
    }
    askedAboutWeights = true;
    return;
}
