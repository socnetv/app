/**
 * @file mainwindow_network_random.cpp
 * @brief Implements MainWindow Network menu: sample dataset selection and random network generation (Erdos-Renyi, Scale-Free, Small-World, Regular, Gaussian, Ring/general Lattice).
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
#include "webcrawler.h"
#include "forms/dialogpreviewfile.h"
#include "forms/dialogdatasetselect.h"
#include "forms/dialogranderdosrenyi.h"
#include "forms/dialograndsmallworld.h"
#include "forms/dialograndscalefree.h"
#include "forms/dialograndregular.h"
#include "forms/dialograndlattice.h"
#include "forms/dialogwebcrawler.h"
#include "forms/dialogexportimage.h"
#include "forms/dialogexportpdf.h"

#include <QtWidgets>
#include <QTextCodec>
#include <QNetworkReply>
#include <QNetworkAccessManager>

/**
 * @brief Displays the dataset selection dialog
 */
void MainWindow::slotNetworkDataSetSelect()
{
    qCDebug(lcMainWindow) << "MW::slotNetworkDataSetSelect()";

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    m_datasetSelectDialog = new DialogDataSetSelect(this);
    connect(m_datasetSelectDialog, SIGNAL(userChoices(QString)),
            this, SLOT(slotNetworkDataSetRecreate(QString)));

    m_datasetSelectDialog->exec();
}

/**
 * @brief Recreates famous and widely used data sets in network analysis studies
 *
 * @param m_fileName
 *
 */
void MainWindow::slotNetworkDataSetRecreate(const QString m_fileName)
{

    int fileFormat = 0;

    qCDebug(lcMainWindow) << "MW::slotNetworkDataSetRecreate() datadir+fileName: "
             << appSettings["dataDir"] + m_fileName;

    activeGraph->writeDataSetToFile(appSettings["dataDir"], m_fileName);

    if (m_fileName.endsWith(".graphml"))
    {
        fileFormat = FileType::GRAPHML;
    }
    else if (m_fileName.endsWith(".pajek") || m_fileName.endsWith(".paj") ||
             m_fileName.endsWith(".net"))
    {
        fileFormat = FileType::PAJEK;
    }
    else if (m_fileName.endsWith(".sm") || m_fileName.endsWith(".adj") || m_fileName.endsWith(".csv"))
    {
        fileFormat = FileType::ADJACENCY;
    }
    else if (m_fileName.endsWith(".dot"))
    {
        fileFormat = FileType::GRAPHVIZ;
    }
    else if (m_fileName.endsWith(".dl"))
    {
        fileFormat = FileType::UCINET;
    }
    else if (m_fileName.endsWith(".gml"))
    {
        fileFormat = FileType::GML;
    }
    else if (m_fileName.endsWith(".wlst"))
    {
        fileFormat = FileType::EDGELIST_WEIGHTED;
    }
    else if (m_fileName.endsWith(".lst"))
    {
        fileFormat = FileType::EDGELIST_SIMPLE;
    }
    else if (m_fileName.endsWith(".2sm"))
    {
        fileFormat = FileType::TWOMODE;
    }

    slotNetworkFileLoad(appSettings["dataDir"] + m_fileName, "UTF-8", fileFormat);
}

/**
 * @brief Shows a dialog to create an Erdos-Renyi random network
 */
void MainWindow::slotNetworkRandomErdosRenyiDialog()
{

    qCDebug(lcMainWindow) << "Showing the dialog to create a random Erdos-Renyi network ";

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage(tr("Generate a random Erdos-Renyi network. "));

    m_randErdosRenyiDialog = new DialogRandErdosRenyi(
        this, appSettings["randomErdosEdgeProbability"].toFloat(0));

    connect(m_randErdosRenyiDialog, &DialogRandErdosRenyi::userChoices,
            this, &MainWindow::slotNetworkRandomErdosRenyi);

    m_randErdosRenyiDialog->exec();
}

/**
 * @brief Creates an Erdos-Renyi random symmetric network
 *
 * @param newNodes
 * @param model
 * @param edges
 * @param eprob
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomErdosRenyi(const int newNodes,
                                             const QString model,
                                             const int edges,
                                             const qreal eprob,
                                             const QString mode,
                                             const bool diag)
{
    qCDebug(lcMainWindow) << "Request to create an Erdos-Renyi random network...";
    appSettings["randomErdosEdgeProbability"] = QString::number(eprob);

    const qint64 expectedEdges = (model == "G(n,p)")
        ? static_cast<qint64>(eprob * newNodes * (newNodes - 1))
        : static_cast<qint64>(edges);
    if (!confirmGenerationSize(expectedEdges, tr("Erdős–Rényi network"), false))
    {
        return;
    }

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, model, edges, eprob, mode, diag, success]() {
            *success = activeGraph->randomNetErdosCreate(newNodes, model, edges, eprob, mode, diag);
        },
        tr("Creating new Erdos-Renyi random network. Please wait... "),
        [this, newNodes, eprob, success]() {
            if (!*success)
            {
                statusMessage(tr("Erdős–Rényi network creation cancelled or did not finish."));
                return;
            }
            setWindowTitle("Untitled Erdos-Renyi random network");
            double threshold = log(newNodes) / newNodes;
            if ((eprob) > threshold)
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Erdős–Rényi random network created."),
                    tr("Random network created. \n"
                       "A new random network has been created according to the Erdős–Rényi model."),
                    tr("On average, edges should be %1. This graph is almost surely connected because: \n"
                       "probability > ln(n) that is: %2 < %3")
                        .arg(QString::number(eprob * newNodes * (newNodes - 1)))
                        .arg(QString::number(eprob))
                        .arg(QString::number(threshold)));
            else
                slotHelpMessageToUser(
                    USER_MSG_INFO,
                    tr("Erdős–Rényi random network created."),
                    tr("Random network created. \n"
                       "A new random network has been created according to the Erdős–Rényi model."),
                    tr("On average, edges should be %1. This graph is almost surely not connected because: \n"
                       "probability < ln(n) that is: %2 < %3")
                        .arg(QString::number(eprob * newNodes * (newNodes - 1)))
                        .arg(QString::number(eprob))
                        .arg(QString::number(threshold)));
        });
}

/**
 * @brief Shows a dialog to create a scale-free random network
 */
void MainWindow::slotNetworkRandomScaleFreeDialog()
{

    qCDebug(lcMainWindow) << "Showing the dialog to create a random scale-free network ";

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage(tr("Generate a random Scale-Free network. "));
    m_randScaleFreeDialog = new DialogRandScaleFree(this);

    connect(m_randScaleFreeDialog, &DialogRandScaleFree::userChoices,
            this, &MainWindow::slotNetworkRandomScaleFree);

    m_randScaleFreeDialog->exec();
}

/**
 * @brief Creates a scale-free random network
 * @param nodes
 * @param power
 * @param initialNodes
 * @param edgesPerStep
 * @param zeroAppeal
 * @param mode
 */
void MainWindow::slotNetworkRandomScaleFree(const int &newNodes,
                                            const int &power,
                                            const int &initialNodes,
                                            const int &edgesPerStep,
                                            const qreal &zeroAppeal,
                                            const QString &mode)
{
    qCDebug(lcMainWindow) << "Request to create a new scale-free random network...";

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, power, initialNodes, edgesPerStep, zeroAppeal, mode, success]() {
            *success = activeGraph->randomNetScaleFreeCreate(newNodes, power, initialNodes,
                                                              edgesPerStep, zeroAppeal, mode);
        },
        tr("Creating new scale-free random network. Please wait... "),
        [this, newNodes, success]() {
            if (!*success)
            {
                statusMessage(tr("Scale-free network creation cancelled or did not finish."));
                return;
            }
            setWindowTitle("Untitled scale-free network");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Scale-free random network created."),
                tr("Random network created. \n"
                   "A new scale-free random network with %1 nodes has been created according to the Barabási–Albert model.")
                    .arg(newNodes),
                tr("A scale-free network is a network whose degree distribution follows a power law."));
        });
}

/**
 * @brief Shows a dialog to create a small-world random network
 */
void MainWindow::slotNetworkRandomSmallWorldDialog()
{
    qCDebug(lcMainWindow) << "Showing the dialog to create a random small-world network ";

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage(tr("Generate a random Small-World network. "));
    m_randSmallWorldDialog = new DialogRandSmallWorld(this);

    connect(m_randSmallWorldDialog, &DialogRandSmallWorld::userChoices,
            this, &MainWindow::slotNetworkRandomSmallWorld);

    m_randSmallWorldDialog->exec();
}

/**
 * @brief Creates a small-world random network
 *
 * @param newNodes
 * @param degree
 * @param beta
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomSmallWorld(const int &newNodes,
                                             const int &degree,
                                             const qreal &beta,
                                             const QString &mode,
                                             const bool &diag)
{
    Q_UNUSED(diag);
    qCDebug(lcMainWindow) << "Request to create a new small-world random network...";

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, degree, beta, mode, success]() {
            *success = activeGraph->randomNetSmallWorldCreate(newNodes, degree, beta, mode);
        },
        tr("Creating new small-world random network. Please wait... "),
        [this, newNodes, success]() {
            if (!*success)
            {
                statusMessage(tr("Small-world network creation cancelled or did not finish."));
                return;
            }
            setWindowTitle("Untitled small-world network");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Small-World random network created."),
                tr("Random network created. \n"
                   "A new random network with %1 nodes has been created according to the Watts & Strogatz model.")
                    .arg(newNodes),
                tr("A small-world network has short average path lengths and high clustering coefficient."));
        });
}

/**
 * @brief Shows a dialog to create a d-regular random network
 */
void MainWindow::slotNetworkRandomRegularDialog()
{
    qCDebug(lcMainWindow) << "Showing the dialog to create a random d-regular network ";

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    statusMessage(tr("Generate a d-regular random network. "));
    m_randRegularDialog = new DialogRandRegular(this);

    connect(m_randRegularDialog, &DialogRandRegular::userChoices,
            this, &MainWindow::slotNetworkRandomRegular);

    m_randRegularDialog->exec();
}

/**
 * @brief Creates a pseudo-random d-regular network where every node has the same degree
 *
 * @param newNodes
 * @param degree
 * @param mode
 * @param diag
 */
void MainWindow::slotNetworkRandomRegular(const int &newNodes, const int &degree,
                                          const QString &mode, const bool &diag)
{
    initApp();

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, degree, mode, diag, success]() {
            *success = activeGraph->randomNetRegularCreate(newNodes, degree, mode, diag);
        },
        tr("Creating new d-regular random network. Please wait... "),
        [this, newNodes, degree, success]() {
            if (!*success)
            {
                statusMessage(tr("d-regular network creation cancelled or did not finish."));
                return;
            }
            setWindowTitle("Untitled d-regular network");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("d-regular network created."),
                tr("Random network created. \n"
                   "A new d-regular random network with %1 nodes has been created.")
                    .arg(newNodes),
                tr("Each node has the same number <em>%1</em> of neighbours, aka the same degree d.")
                    .arg(degree));
        });
}

void MainWindow::slotNetworkRandomGaussian()
{
}

/**
 * @brief Creates a ring lattice network
 *
 * A ring lattice is a network where each node has degree d:
 * - d/2 edges to the "right"
 * - d/2 edges to the "left"
 */
void MainWindow::slotNetworkRandomRingLattice()
{

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    bool ok;
    statusMessage("You have selected to create a ring lattice network. ");
    int newNodes = (QInputDialog::getInt(
        this,
        tr("Create ring lattice"),
        tr("This will create a ring lattice network, "
           "where each node has degree d:\n d/2 edges to the right "
           "and d/2 to the left.\n"
           "Please enter the number of nodes you want:"),
        100, 4, maxRandomlyCreatedNodes, 1, &ok));
    if (!ok)
    {
        statusMessage("You did not enter an integer. Aborting.");
        return;
    }

    int degree = QInputDialog::getInt(
        this,
        tr("Create ring lattice..."),
        tr("Now, enter an even number d. \n"
           "This is the total number of edges each new node will have:"),
        2, 2, newNodes - 1, 2, &ok);

    if ((degree % 2) == 1)
    {

        slotHelpMessageToUser(
            USER_MSG_CRITICAL,
            tr("Error. Cannot create such network."),
            tr("Error. Cannot create such network!\n\n"
               "The degree %1 is not an even number.")
                .arg(degree),
            tr("A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side. \n"
               "Please try again entering an even number as degree.")

        );
        return;
    }

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, degree, success]() {
            *success = activeGraph->randomNetRingLatticeCreate(newNodes, degree, true);
        },
        tr("Creating new ring-lattice random network. Please wait... "),
        [this, newNodes, success]() {
            if (!*success)
            {
                return;
            }
            setWindowTitle("Untitled ring-lattice network");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Ring lattice random network created."),
                tr("Random network created. \n"
                   "A new ring-lattice random network with %1 nodes has been created.")
                    .arg(newNodes),
                tr("A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side.")

            );
        });
}

/**
 * @brief Shows a dialog to create a "random" lattice network.
 */
void MainWindow::slotNetworkRandomLatticeDialog()
{
    qCDebug(lcMainWindow) << "Showing the Random Lattice Dialog...";
    statusMessage(tr("Generate a lattice network. "));
    m_randLatticeDialog = new DialogRandLattice(this);

    connect(m_randLatticeDialog, &DialogRandLattice::userChoices,
            this, &MainWindow::slotNetworkRandomLattice);

    m_randLatticeDialog->exec();
}

/**
 * @brief Creates a 'random' lattice network, i.e. a connected network where every node
 * has the same degree and is connected with its neighborhood
 *
 * A lattice is a network whose drawing forms a regular tiling
 * Lattices are also known as meshes or grids.
 *
 * @param newNodes
 * @param length
 * @param dimension
 * @param nei
 * @param mode
 * @param circular
 */
void MainWindow::slotNetworkRandomLattice(const int &newNodes,
                                          const int &length,
                                          const int &dimension,
                                          const int &nei,
                                          const QString &mode,
                                          const bool &circular)
{
    qCDebug(lcMainWindow) << "Request to create a new lattice random network...";
    initApp();

    auto success = std::make_shared<bool>(false);

    runGraphOperationAsync(
        [this, newNodes, length, dimension, nei, mode, circular, success]() {
            *success = activeGraph->randomNetLatticeCreate(newNodes, length, dimension, nei, mode, circular);
        },
        tr("Creating new lattice random network. Please wait... "),
        [this, newNodes, success]() {
            if (!*success)
            {
                statusMessage(tr("Lattice network creation cancelled or did not finish."));
                return;
            }
            setWindowTitle("Untitled lattice network");
            slotHelpMessageToUser(
                USER_MSG_INFO,
                tr("Lattice random network created."),
                tr("Random network created. \n"
                   "A new lattice random network with %1 nodes has been created.")
                    .arg(newNodes),
                tr("A lattice is a network whose drawing forms a regular tiling. "
                   "Lattices are also known as meshes or grids."));
        });
}
