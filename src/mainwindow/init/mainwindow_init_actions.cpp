/**
 * @file mainwindow_init_actions.cpp
 * @brief Implements MainWindow::initActions() - constructs every QAction in the application.
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
#include "widgets/filterbarwidget.h"
#include "widgets/graphtablewidget.h"

#include <QtWidgets>
#include <QtCharts>

/**
 * @brief Initializes all QActions of the application
 *
 * Take a breath, the listing below is HUGE.
 *
 */
void MainWindow::initActions()
{

    qCDebug(lcMainWindow) << "initializing actions...";

    /**
    Network menu actions
    */
    networkNewAct = new QAction(QIcon(":/images/new_folder_48px.svg"), tr("&New"), this);
    networkNewAct->setShortcut(Qt::CTRL | Qt::Key_N);
    networkNewAct->setStatusTip(tr("Create a new network"));
    networkNewAct->setToolTip(tr("New network"));
    networkNewAct->setWhatsThis(tr("New\n\n"
                                   "Creates a new social network. "
                                   "First, checks if current network needs to be saved."));
    connect(networkNewAct, SIGNAL(triggered()), this, SLOT(slotNetworkNew()));

    networkOpenAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&Open"), this);
    networkOpenAct->setShortcut(Qt::CTRL | Qt::Key_O);
    networkOpenAct->setToolTip(tr("Open network"));
    networkOpenAct->setStatusTip(tr("Open a GraphML formatted file of social network data."));
    networkOpenAct->setWhatsThis(tr("Open\n\n"
                                    "Opens a file of a social network in GraphML format"));
    connect(networkOpenAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileChoose()));

    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(slotNetworkFileLoadRecent()));
    }

    networkImportGMLAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&GML"), this);
    networkImportGMLAct->setStatusTip(tr("Import GML-formatted file"));
    networkImportGMLAct->setWhatsThis(tr("Import GML\n\n"
                                         "Imports a social network from a GML-formatted file"));
    connect(networkImportGMLAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportGML()));

    networkImportPajekAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&Pajek"), this);
    networkImportPajekAct->setStatusTip(tr("Import Pajek-formatted file"));
    networkImportPajekAct->setWhatsThis(tr("Import Pajek \n\n"
                                           "Imports a social network from a Pajek-formatted file"));
    connect(networkImportPajekAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportPajek()));

    networkImportAdjAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&Adjacency Matrix"), this);
    networkImportAdjAct->setStatusTip(tr("Import Adjacency matrix"));
    networkImportAdjAct->setWhatsThis(tr("Import Sociomatrix \n\n"
                                         "Imports a social network from an Adjacency matrix-formatted file"));
    connect(networkImportAdjAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportAdjacency()));

    networkImportGraphvizAct = new QAction(QIcon(":/images/open_48px.svg"), tr("Graph&Viz (.dot)"), this);
    networkImportGraphvizAct->setStatusTip(tr("Import dot file"));
    networkImportGraphvizAct->setWhatsThis(tr("Import GraphViz \n\n"
                                              "Imports a social network from a GraphViz formatted file"));
    connect(networkImportGraphvizAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportGraphviz()));

    networkImportUcinetAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&UCINET (.dl)..."), this);
    networkImportUcinetAct->setStatusTip(tr("ImportDL-formatted file (UCINET)"));
    networkImportUcinetAct->setWhatsThis(tr("Import UCINET\n\n"
                                            "Imports social network data from a DL-formatted file"));
    connect(networkImportUcinetAct, SIGNAL(triggered()), this, SLOT(slotNetworkImportUcinet()));

    networkImportListAct = new QAction(QIcon(":/images/open_48px.svg"), tr("&Edge list"), this);
    networkImportListAct->setStatusTip(tr("Import an edge list file. "));
    networkImportListAct->setWhatsThis(
        tr("Import edge list\n\n"
           "Import a network from an edgelist file. "
           "SocNetV supports EdgeList files with edge weights "
           "as well as simple EdgeList files where the edges are non-value (see manual)"));
    connect(networkImportListAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportEdgeList()));

    networkImportTwoModeSM = new QAction(QIcon(":/images/open_48px.svg"), tr("&Two Mode Sociomatrix"), this);
    networkImportTwoModeSM->setStatusTip(tr("Import two-mode sociomatrix (affiliation network) file"));
    networkImportTwoModeSM->setWhatsThis(tr("Import Two-Mode Sociomatrix \n\n"
                                            "Imports a two-mode network from a sociomatrix file. "
                                            "Two-mode networks are described by affiliation "
                                            "network matrices, where A(i,j) codes the "
                                            "events/organizations each actor is affiliated with."));
    connect(networkImportTwoModeSM, SIGNAL(triggered()),
            this, SLOT(slotNetworkImportTwoModeSM()));

    networkSaveAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&Save"), this);
    networkSaveAct->setShortcut(QKeySequence::Save);
    networkSaveAct->setStatusTip(tr("Save social network to a file"));
    networkSaveAct->setWhatsThis(tr("Save.\n\n"
                                    "Saves the social network to file"));
    connect(networkSaveAct, SIGNAL(triggered()), this, SLOT(slotNetworkSave()));

    networkSaveAsAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("Save As..."), this);
    networkSaveAsAct->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    networkSaveAsAct->setStatusTip(tr("Save network under a new filename"));
    networkSaveAsAct->setWhatsThis(tr("Save As\n\n"
                                      "Saves the social network under a new filename"));
    connect(networkSaveAsAct, SIGNAL(triggered()), this, SLOT(slotNetworkSaveAs()));

    networkExportImageAct = new QAction(QIcon(":/images/export_photo_48px.svg"), tr("Export to I&mage..."), this);
    networkExportImageAct->setStatusTip(tr("Export the visible part of the network to image"));
    networkExportImageAct->setWhatsThis(tr("Export to Image\n\n"
                                           "Exports the visible part of the current social network to an image"));
    connect(networkExportImageAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportImageDialog()));

    networkExportPDFAct = new QAction(QIcon(":/images/export_pdf_48px.svg"), tr("E&xport to PDF..."), this);
    networkExportPDFAct->setStatusTip(tr("Export the visible part of the network to a PDF file"));
    networkExportPDFAct->setWhatsThis(tr("Export to PDF\n\n"
                                         "Exports the visible part of the current social network to a PDF document."));
    connect(networkExportPDFAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportPDFDialog()));

    networkExportSMAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&Adjacency Matrix"), this);
    networkExportSMAct->setStatusTip(tr("Export social network to an adjacency/sociomatrix file"));
    networkExportSMAct->setWhatsThis(tr("Export network to Adjacency format\n\n"
                                        "Exports the social network to an "
                                        "adjacency matrix-formatted file"));
    connect(networkExportSMAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportSM()));

    networkExportPajek = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&Pajek"), this);
    networkExportPajek->setStatusTip(tr("Export social network to a Pajek-formatted file"));
    networkExportPajek->setWhatsThis(tr("Export Pajek \n\n"
                                        "Exports the social network to a Pajek-formatted file"));
    connect(networkExportPajek, SIGNAL(triggered()), this, SLOT(slotNetworkExportPajek()));

    networkExportListAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&List"), this);
    networkExportListAct->setStatusTip(tr("Export to List-formatted file. "));
    networkExportListAct->setWhatsThis(tr("Export List\n\n"
                                          "Exports the network to a List-formatted file"));
    connect(networkExportListAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportList()));

    networkExportDLAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&DL..."), this);
    networkExportDLAct->setStatusTip(tr("Export network to UCINET-formatted file"));
    networkExportDLAct->setWhatsThis(tr("Export UCINET\n\n"
                                        "Exports the active network to a DL-formatted"));
    connect(networkExportDLAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportDL()));

    networkExportGWAct = new QAction(QIcon(":/images/file_download_48px.svg"), tr("&GW..."), this);
    networkExportGWAct->setStatusTip(tr("Export to GW-formatted file"));
    networkExportGWAct->setWhatsThis(tr("Export\n\n"
                                        "Exports the active network to a GW formatted file"));
    connect(networkExportGWAct, SIGNAL(triggered()), this, SLOT(slotNetworkExportGW()));

    networkExportDotAct = new QAction(QIcon(":/images/file_download_48px.svg"),
                                      tr("&GraphViz DOT..."), this);
    networkExportDotAct->setStatusTip(tr("Export the active relation to a GraphViz DOT (.dot) file"));
    networkExportDotAct->setWhatsThis(tr("Export GraphViz DOT\n\n"
                                         "Exports the currently active relation to a GraphViz DOT file. "
                                         "Node labels, colors, shapes, and custom attributes are preserved. "
                                         "Only the active relation is exported."));
    connect(networkExportDotAct, &QAction::triggered,
            this, &MainWindow::slotNetworkExportDot);

    networkExportNodesCSVAct = new QAction(QIcon(":/images/file_download_48px.svg"),
                                           tr("Nodes as &CSV..."), this);
    networkExportNodesCSVAct->setStatusTip(tr("Export all node data to a CSV file"));
    connect(networkExportNodesCSVAct, &QAction::triggered,
            this, &MainWindow::slotNetworkExportNodesCSV);

    networkExportEdgesCSVAct = new QAction(QIcon(":/images/file_download_48px.svg"),
                                           tr("Edges as C&SV..."), this);
    networkExportEdgesCSVAct->setStatusTip(tr("Export all edge data to a CSV file"));
    connect(networkExportEdgesCSVAct, &QAction::triggered,
            this, &MainWindow::slotNetworkExportEdgesCSV);

    networkExportNodesJSONAct = new QAction(QIcon(":/images/file_download_48px.svg"),
                                            tr("Nodes as &JSON..."), this);
    networkExportNodesJSONAct->setStatusTip(tr("Export all node data to a JSON file"));
    connect(networkExportNodesJSONAct, &QAction::triggered,
            this, &MainWindow::slotNetworkExportNodesJSON);

    networkExportEdgesJSONAct = new QAction(QIcon(":/images/file_download_48px.svg"),
                                            tr("Edges as J&SON..."), this);
    networkExportEdgesJSONAct->setStatusTip(tr("Export all edge data to a JSON file"));
    connect(networkExportEdgesJSONAct, &QAction::triggered,
            this, &MainWindow::slotNetworkExportEdgesJSON);

    networkCloseAct = new QAction(QIcon(":/images/close_24px.svg"), tr("&Close"), this);
    networkCloseAct->setShortcut(QKeySequence::Close);
    networkCloseAct->setStatusTip(tr("Close the actual network"));
    networkCloseAct->setWhatsThis(tr("Close \n\nCloses the actual network"));
    connect(networkCloseAct, SIGNAL(triggered()), this, SLOT(slotNetworkClose()));

    networkPrintAct = new QAction(QIcon(":/images/print_48px.svg"), tr("&Print"), this);
    networkPrintAct->setShortcut(QKeySequence::Print);
    networkPrintAct->setStatusTip(tr("Send the currrent social network to the printer"));
    networkPrintAct->setWhatsThis(tr("Print \n\n"
                                     "Sends whatever is viewable on "
                                     "the canvas to your printer. \n"
                                     "To print the whole social network, "
                                     "you might want to zoom-out."));
    connect(networkPrintAct, SIGNAL(triggered()), this, SLOT(slotNetworkPrint()));

    networkQuitAct = new QAction(QIcon(":/images/exit_24px.svg"), tr("E&xit"), this);
    networkQuitAct->setShortcut(QKeySequence::Quit);
    networkQuitAct->setStatusTip(tr("Quit SocNetV. Are you sure?"));
    networkQuitAct->setWhatsThis(tr("Exit\n\n"
                                    "Quits the application"));
    connect(networkQuitAct, SIGNAL(triggered()), this, SLOT(close()));

    openTextEditorAct = new QAction(QIcon(":/images/text_edit_48px.svg"),
                                    tr("Open &Text Editor"), this);
    openTextEditorAct->setShortcut(Qt::SHIFT | Qt::Key_F5);
    openTextEditorAct->setStatusTip(tr("Open a text editor "
                                       "to take notes, copy/paste network data, etc"));
    openTextEditorAct->setWhatsThis(
        tr("<p><b>Text Editor</b></p>"
           "<p>Opens a simple text editor where you can "
           "copy paste network data, of any supported format, "
           "and save to a file. Then you can import that file to SocNetV. </p>"));
    connect(openTextEditorAct, SIGNAL(triggered()), this, SLOT(slotNetworkTextEditor()));

    networkViewFileAct = new QAction(QIcon(":/images/code_48px.svg"),
                                     tr("&View Loaded File"), this);
    networkViewFileAct->setShortcut(Qt::Key_F5);
    networkViewFileAct->setStatusTip(tr("Display the loaded social network file."));
    networkViewFileAct->setWhatsThis(tr("View Loaded File\n\n"
                                        "Displays the loaded social network file "));
    connect(networkViewFileAct, SIGNAL(triggered()), this, SLOT(slotNetworkFileView()));

    networkViewSociomatrixAct = new QAction(QIcon(":/images/sociomatrix_48px.svg"),
                                            tr("View &Adjacency Matrix"), this);
    networkViewSociomatrixAct->setShortcut(Qt::Key_F6);
    networkViewSociomatrixAct->setStatusTip(tr("Display the adjacency matrix of the network."));
    networkViewSociomatrixAct->setWhatsThis(
        tr("<p><b>View Adjacency Matrix</b></p>"
           "<p>Displays the adjacency matrix of the active network. </p>"
           "<p>The adjacency matrix of a social network is a matrix "
           "where each element a(i,j) is equal to the weight "
           "of the arc from actor (node) i to actor j. "
           "<p>If the actors are not connected, then a(i,j)=0. </p>"));
    connect(networkViewSociomatrixAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrix()));

    networkViewSociomatrixPlotAct = new QAction(QIcon(":/images/adjacencyplot.png"),
                                                tr("P&lot Adjacency Matrix (text)"), this);
    networkViewSociomatrixPlotAct->setShortcut(Qt::SHIFT | Qt::Key_F6);
    networkViewSociomatrixPlotAct->setStatusTip(
        tr("Plots the adjacency matrix in a text file using unicode characters."));
    networkViewSociomatrixPlotAct->setWhatsThis(
        tr("<p><b>Plot Adjacency Matrix (text)</b></p>"
           "<p>Plots the adjacency matrix in a text file using "
           "unicode characters. </p>"
           "<p>In every element (i,j) of the \"image\", "
           "a black square means actors i and j are connected"
           "whereas a white square means they are disconnected.</p>"));
    connect(networkViewSociomatrixPlotAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkViewSociomatrixPlotText()));

    networkDataSetSelectAct = new QAction(QIcon(":/images/science_48px.svg"),
                                          tr("Create From &Known Data Sets"), this);
    networkDataSetSelectAct->setShortcut(Qt::Key_F7);
    networkDataSetSelectAct->setStatusTip(
        tr("Load one of the \'famous\' social network data sets included in SocNetV."));
    networkDataSetSelectAct->setWhatsThis(
        tr("<p><b>Famous Data Sets</b></p>"
           "<p>SocNetV includes a number of known "
           "(also called famous) data sets in Social Network Analysis, "
           "such as Krackhardt's high-tech managers, etc. "
           "Click this menu item or press F7 to load a data set.</p> "));
    connect(networkDataSetSelectAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkDataSetSelect()));

    networkRandomScaleFreeAct = new QAction(
        QIcon(":/images/scalefree.png"), tr("Scale-free"), this);

    networkRandomScaleFreeAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_S));
    networkRandomScaleFreeAct->setStatusTip(
        tr("Create a random network with a power-law degree distribution."));
    networkRandomScaleFreeAct->setWhatsThis(
        tr("<p><b>Scale-free (power-law)</b></p>"
           "<p>A scale-free network is a network whose degree distribution "
           "follows a power law."
           " SocNetV generates random scale-free networks according to the "
           " Barabási–Albert (BA) model using a preferential attachment mechanism.</p>"));
    connect(networkRandomScaleFreeAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomScaleFreeDialog()));

    networkRandomSmallWorldAct = new QAction(QIcon(":/images/sw.png"), tr("Small World"), this);
    networkRandomSmallWorldAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_M));
    networkRandomSmallWorldAct->setStatusTip(tr("Create a small-world random network, according to the Watts & Strogatz model."));
    networkRandomSmallWorldAct->setWhatsThis(
        tr("<p><b>Small World </b></p>"
           "<p>Creates a random small-world network, according to the "
           "Watts & Strogatz model. </p>"
           "<p>A small-world network has short average path lengths and "
           "high clustering coefficient.</p>"));
    connect(networkRandomSmallWorldAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomSmallWorldDialog()));

    networkRandomErdosRenyiAct = new QAction(QIcon(":/images/erdos.png"),
                                             tr("Erdős–Rényi"), this);
    networkRandomErdosRenyiAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_E));
    networkRandomErdosRenyiAct->setStatusTip(
        tr("Create a random network according to the Erdős–Rényi model"));
    networkRandomErdosRenyiAct->setWhatsThis(
        tr("<p><b>Erdős–Rényi </b></p>"
           "<p>Creates a random network either of G(n, p) model or G(n,M) model. </p>"
           "<p>The former model creates edges with Bernoulli trials (probability p).</p>"
           "<p>The latter creates a graph of exactly M edges.</p>"));
    connect(networkRandomErdosRenyiAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomErdosRenyiDialog()));

    networkRandomLatticeAct = new QAction(QIcon(":/images/lattice.png"), tr("Lattice"), this);
    networkRandomLatticeAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_T));
    networkRandomLatticeAct->setStatusTip(tr("Create a lattice network."));
    networkRandomLatticeAct->setWhatsThis(
        tr("<p><b>Lattice </b></p>"
           "<p>Creates a random lattice network</p>"
           "<p>A lattice is a network whose drawing forms a regular tiling. "
           "Lattices are also known as meshes or grids.</p>"));
    connect(networkRandomLatticeAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomLatticeDialog()));

    networkRandomRegularSameDegreeAct = new QAction(QIcon(":/images/net.png"), tr("d-Regular"), this);
    networkRandomRegularSameDegreeAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_R));
    networkRandomRegularSameDegreeAct->setStatusTip(
        tr("Create a d-regular random network, "
           "where every actor has the same degree d."));
    networkRandomRegularSameDegreeAct->setWhatsThis(
        tr("<p><b>d-Regular</b></p>"
           "<p>Creates a random network where each actor has the same "
           "number <em>d</em> of neighbours, aka the same degree d.</p>"));
    connect(networkRandomRegularSameDegreeAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomRegularDialog()));

    networkRandomLatticeRingAct = new QAction(QIcon(":/images/net1.png"),
                                              tr("Ring Lattice"), this);
    networkRandomLatticeRingAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_L));
    networkRandomLatticeRingAct->setStatusTip(tr("Create a ring lattice random network."));
    networkRandomLatticeRingAct->setWhatsThis(
        tr("<p><b>Ring Lattice </b></p>"
           "<p>Creates a ring lattice random network. </p>"
           "<p>A ring lattice is a graph with N vertices each connected to d neighbors, d / 2 on each side.</p>"));
    connect(networkRandomLatticeRingAct, SIGNAL(triggered()),
            this, SLOT(slotNetworkRandomRingLattice()));

    networkRandomGaussianAct = new QAction(tr("Gaussian"), this);
    networkRandomGaussianAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_G));
    networkRandomGaussianAct->setStatusTip(tr("Create a Gaussian distributed random network."));
    networkRandomGaussianAct->setWhatsThis(tr("Gaussian \n\nCreates a random network of Gaussian distribution"));
    connect(networkRandomGaussianAct, SIGNAL(triggered()), this, SLOT(slotNetworkRandomGaussian()));

    networkWebCrawlerAct = new QAction(QIcon(":/images/webcrawler_48px.svg"), tr("&Web Crawler"), this);
    networkWebCrawlerAct->setShortcut(Qt::SHIFT | Qt::Key_C);
    networkWebCrawlerAct->setEnabled(true);
    networkWebCrawlerAct->setStatusTip(tr("Use the web crawler to create a network from all links found in a given website"));
    networkWebCrawlerAct->setWhatsThis(
        tr("<p><b>Web Crawler </b></p>"
           "<p>Creates a network of linked webpages, starting "
           "from an initial webpage using the built-in Web Crawler. </p>"
           "<p>The web crawler visits the given URL (website or webpage) "
           "and parses its contents to find links to other pages (internal or external). "
           "If there are such links, it adds them to a queue of URLs. "
           "Then, all the URLs in the queue list are visited in a FIFO order "
           "and parsed to find more links which are also added to the url queue. "
           "The process repeats until it reaches user-defined "
           "limits: </p>"
           "<p>Maximum urls to visit (max nodes in the resulting network)</p> "
           "<p>Maximum links per page</p>"
           "<p>Except the initial url and the limits, you can also "
           "specify patterns of urls to include or exclude, "
           "types of links to follow (internal, external or both) as well as "
           "if you want delay between requests (strongly advised)</p>."));

    connect(networkWebCrawlerAct, SIGNAL(triggered()), this, SLOT(slotNetworkWebCrawlerDialog()));

    /**
    Edit menu actions
    */

    editMouseModeInteractiveAct = new QAction(QIcon(":/images/cursor-pointer.svg"),
                                              tr("Select/Move"), this);
    editMouseModeInteractiveAct->setCheckable(true);
    editMouseModeInteractiveAct->setChecked(true);
    editMouseModeInteractiveAct->setToolTip(tr("<p><b>Mouse mode: Interactive</b></p> "
                                               "<p>In this interactive mode, you can click on nodes/edges and move them around with your mouse. </p>"
                                               "<p>Also, you can select multiple items with a rubber band selection area. To move the canvas, use the keyboard arrows.</p>"));
    editMouseModeInteractiveAct->setStatusTip(tr("Enable the interactive mouse mode to be able to click and move items and select them with a rubber band."));
    editMouseModeInteractiveAct->setWhatsThis(tr("<p><b>Mouse Mode: Interactive</b></p>"
                                                 "<p>In this mode, you can interact with the items on the canvas using the mouse: </p>"
                                                 "<p>a) double-click to create new nodes, "
                                                 "<p>b) left-click or right-click on items (i.e. nodes, edges) to edit their properties</p>"
                                                 "<p>c) move nodes by dragging them with your mouse.  </p>"
                                                 "<p>d) select multiple items with a rubber band.</p>"
                                                 "<p>To move the canvas (up/down, left/right), use the keyboard arrows."));

    editMouseModeScrollAct = new QAction(QIcon(":/images/cursor-hand-drag.svg"),
                                         tr("Scroll/Pan"), this);
    editMouseModeScrollAct->setCheckable(true);
    editMouseModeScrollAct->setChecked(false);
    editMouseModeScrollAct->setToolTip(tr("<p><b>Mouse mode: Scrolling</b></p> "
                                          "<p>In this non-interactive mode, you can easily scroll the canvas by dragging the mouse around. All mouse actions are disabled.</p>"));
    editMouseModeScrollAct->setStatusTip(tr("Enable this non-interactive mode to easily scroll the canvas by dragging the mouse around."));
    editMouseModeScrollAct->setWhatsThis(tr("<p><b>Mouse mode: Scrolling</b></p>"
                                            "<p>In this mode, you cannot interact with the canvas using the mouse.</p>"
                                            "<p>The cursor changes into a pointing hand, and dragging the mouse around will only scroll the scrolbars.</p> "
                                            "<p>You will not be able to select any items or move them around with the mouse.</p>"
                                            "<p>Note: You will still be able to edit the network using the menu or the toolbar actions and icons.</p>"));

    editRelationNextAct = new QAction(QIcon(":/images/chevron_right_48px.svg"),
                                      tr("Next Relation"), this);
    editRelationNextAct->setShortcut(Qt::CTRL | Qt::Key_Right);
    editRelationNextAct->setToolTip(tr("Goto the next relation of the network (if any)."));
    editRelationNextAct->setStatusTip(tr("Goto the next relation of the network (if any)."));
    editRelationNextAct->setWhatsThis(tr("Next Relation\n\nLoads the next relation of the network (if any)"));
    editRelationNextAct->setEnabled(false);

    editRelationPreviousAct = new QAction(QIcon(":/images/chevron_left_48px.svg"),
                                          tr("Previous Relation"), this);
    editRelationPreviousAct->setShortcut(Qt::CTRL | Qt::Key_Left);
    editRelationPreviousAct->setToolTip(
        tr("Goto the previous relation of the network (if any)."));
    editRelationPreviousAct->setStatusTip(
        tr("Goto the previous relation of the network (if any)."));
    editRelationPreviousAct->setWhatsThis(
        tr("Previous Relation\n\n"
           "Loads the previous relation of the network (if any)"));
    editRelationPreviousAct->setEnabled(false);

    editRelationAddAct = new QAction(QIcon(":/images/add_48px.svg"),
                                     tr("Add New Relation"), this);
    editRelationAddAct->setShortcut(Qt::ALT | Qt::CTRL | Qt::Key_N);
    editRelationAddAct->setToolTip(
        tr("Add a new relation to the network. Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setStatusTip(
        tr("Add a new relation to the network. Nodes will be preserved, edges will be removed. "));
    editRelationAddAct->setWhatsThis(
        tr("Add New Relation\n\n"
           "Adds a new relation to the active network. "
           "Nodes will be preserved, edges will be removed. "));

    editRelationRenameAct = new QAction(QIcon(":/images/relation_edit_48px.svg"),
                                        tr("Rename Relation"), this);
    editRelationRenameAct->setToolTip(tr("Rename current relation"));
    editRelationRenameAct->setStatusTip(tr("Rename the current relation of the network."));
    editRelationRenameAct->setWhatsThis(tr("Rename Relation\n\n"
                                           "Renames the current relation of the network."));

    zoomInAct = new QAction(QIcon(":/images/zoom_in_24px.svg"), tr("Zoom In"), this);
    zoomInAct->setShortcut(Qt::CTRL | Qt::Key_Plus);
    zoomInAct->setStatusTip(tr("Zoom In.\n\nZooms in the network"));
    zoomInAct->setWhatsThis(tr("Zoom in the network. Alternatives: use the canvas button, or press Ctrl++, or use mouse wheel while pressing Ctrl."));

    zoomOutAct = new QAction(QIcon(":/images/zoom_in_24px.svg"), tr("Zoom Out"), this);
    zoomOutAct->setShortcut(Qt::CTRL | Qt::Key_Minus);
    zoomOutAct->setStatusTip(tr("Zoom Out.\n\nZooms out of the actual network"));
    zoomOutAct->setWhatsThis(tr("Zoom out the network. Alternatives: use the canvas button, or press Ctrl+-, or use mouse wheel while pressing Ctrl."));

    editRotateLeftAct = new QAction(QIcon(":/images/rotate_left_48px.svg"), tr("Rotate counterclockwise"), this);
    editRotateLeftAct->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::LeftArrow);
    editRotateLeftAct->setStatusTip(tr("Rotate counterclockwise. You can also use the button underneath the canvas."));
    editRotateLeftAct->setWhatsThis(tr("Rotates the network counterclockwise. You can also use the far left button below the canvas."));

    editRotateRightAct = new QAction(QIcon(":/images/rotate_right_48px.svg"), tr("Rotate clockwise"), this);
    editRotateRightAct->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::RightArrow);
    editRotateRightAct->setStatusTip(tr("Rotate clockwise. You can also use the button underneath the canvas."));
    editRotateRightAct->setWhatsThis(tr("Rotates the network clockwise. You can also use the far right button below the canvas."));

    editResetSlidersAct = new QAction(QIcon(":/images/refresh_48px.svg"), tr("Reset Zoom and Rotation"), this);
    editResetSlidersAct->setShortcut(Qt::CTRL | Qt::Key_0);
    editResetSlidersAct->setStatusTip(tr("Reset zoom and rotation to zero."));
    editResetSlidersAct->setWhatsThis(tr("Resets any zoom and rotation transformations to zero."));

    editNodeSelectAllAct = new QAction(QIcon(":/images/select_all_48px.svg"), tr("Select All"), this);
    editNodeSelectAllAct->setShortcut(QKeySequence::SelectAll);
    editNodeSelectAllAct->setStatusTip(tr("Select all nodes"));
    editNodeSelectAllAct->setWhatsThis(tr("Select All\n\nSelects all nodes and edges in the network"));
    connect(editNodeSelectAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectAll()));

    editNodeSelectNoneAct = new QAction(QIcon(":/images/select_none_48px.svg"), tr("Select None"), this);
    editNodeSelectNoneAct->setShortcut(tr("Ctrl+Alt+A"));
    editNodeSelectNoneAct->setStatusTip(tr("Deselect all nodes and edges"));
    editNodeSelectNoneAct->setWhatsThis(tr("Deselect all\n\n Clears the node selection"));
    connect(editNodeSelectNoneAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSelectNone()));

    editNodeFindAct = new QAction(QIcon(":/images/search_48px.svg"), tr("Find Nodes "), this);
    editNodeFindAct->setShortcut(QKeySequence::Find);
    editNodeFindAct->setToolTip(tr("Find and select one or more nodes by their number or label."));
    editNodeFindAct->setStatusTip(tr("Find and select one or more nodes by their number or label."));
    editNodeFindAct->setWhatsThis(tr("Find Node\n\n"
                                     "Finds one or more nodes by their number or label and "
                                     "highlights them by doubling its size. "));
    connect(editNodeFindAct, SIGNAL(triggered()), this, SLOT(slotEditNodeFindDialog()));

    editNodeAddAct = new QAction(QIcon(":/images/node_add_48px.svg"), tr("Add Node"), this);
    editNodeAddAct->setShortcut(tr("Ctrl+."));
    editNodeAddAct->setStatusTip(tr("Add a new node to the network in a random position. Alternately, double-click on a specific position the canvas. "));
    editNodeAddAct->setToolTip(
        tr("Add a new node to the network in a random position.\n\n"
           "Alternately, create a new node by double-clicking on a specific position the canvas. "));
    editNodeAddAct->setWhatsThis(
        tr("Add new node\n\n"
           "Add a new node to the network in a random position. \n\n"
           "Alternately, you can create a new node by double-clicking on a specific position the canvas."));

    connect(editNodeAddAct, SIGNAL(triggered()), this, SLOT(slotEditNodeAdd()));

    editNodeRemoveAct = new QAction(QIcon(":/images/node_remove_48px.svg"), tr("Remove Node"), this);
    editNodeRemoveAct->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Period);
    // Single key shortcuts with backspace or del do no work in Mac http://goo.gl/7hz7Dx
    editNodeRemoveAct->setToolTip(tr("Remove selected node(s). \n\n"
                                     "If no nodes are selected, you will be prompted for a node number. "));

    editNodeRemoveAct->setStatusTip(tr("Remove selected node(s). If no nodes are selected, you will be prompted for a node number. "));
    editNodeRemoveAct->setWhatsThis(
        tr("Remove node\n\n"
           "Removes selected node(s) from the network. \n"
           "Alternately, you can remove a node by right-clicking on it. \n"
           "If no nodes are selected, you will be prompted for a node number. "));

    connect(editNodeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditNodeRemove()));

    editNodePropertiesAct = new QAction(QIcon(":/images/node_properties_24px.svg"), tr("Selected Node Properties"), this);
    editNodePropertiesAct->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Period);
    editNodePropertiesAct->setToolTip(tr("Change the properties of the selected node(s) \n\n"
                                         "There must be some nodes on the canvas!"));
    editNodePropertiesAct->setStatusTip(tr("Change the basic properties of the selected node(s). There must be some nodes on the canvas!"));
    editNodePropertiesAct->setWhatsThis(tr("Selected Node Properties\n\n"
                                           "If there are one or more nodes selected, "
                                           "it opens a properties dialog to edit "
                                           "their label, size, color, shape etc. \n"
                                           "You must have some node selected."));
    connect(editNodePropertiesAct, SIGNAL(triggered()), this, SLOT(slotEditNodePropertiesDialog()));

    editNodeEditSelectionInTableAct = new QAction(tr("Edit Selection in Data Table"), this);
    editNodeEditSelectionInTableAct->setStatusTip(
        tr("Open the Data Table and pre-select the rows matching the current canvas selection."));
    connect(editNodeEditSelectionInTableAct, &QAction::triggered,
            this, &MainWindow::slotEditNodeEditSelectionInTable);

    editNodeSetPropertyForSelectionAct = new QAction(tr("Set property on selected nodes..."), this);
    editNodeSetPropertyForSelectionAct->setStatusTip(
        tr("Set a single property (built-in or custom attribute) on all selected nodes."));
    connect(editNodeSetPropertyForSelectionAct, &QAction::triggered,
            this, &MainWindow::slotEditNodeSetPropertyForSelection);

    editEdgeSetPropertyForSelectionAct = new QAction(tr("Set property on selected edges..."), this);
    editEdgeSetPropertyForSelectionAct->setStatusTip(
        tr("Set a single property (built-in or custom attribute) on all selected edges."));
    connect(editEdgeSetPropertyForSelectionAct, &QAction::triggered,
            this, &MainWindow::slotEditEdgeSetPropertyForSelection);

    editNodeSelectedToCliqueAct = new QAction(QIcon(":/images/cliquenew.png"),
                                              tr("Create a clique from selected nodes "), this);
    editNodeSelectedToCliqueAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_C));
    editNodeSelectedToCliqueAct->setStatusTip(tr("Connect all selected nodes with edges to create a clique -- "
                                                 "There must be some nodes selected!"));
    editNodeSelectedToCliqueAct->setWhatsThis(tr("Clique from Selected Nodes\n\n"
                                                 "Adds all possible edges between selected nodes, "
                                                 "so that they become a complete subgraph (clique)\n"
                                                 "You must have some nodes selected."));
    connect(editNodeSelectedToCliqueAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToClique()));

    editNodeSelectedToStarAct = new QAction(QIcon(":/images/subgraphstar_128px.svg"),
                                            tr("Create a star from selected nodes "), this);
    editNodeSelectedToStarAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_S));
    editNodeSelectedToStarAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                               "There must be some nodes selected!"));
    editNodeSelectedToStarAct->setWhatsThis(tr("Star from Selected Nodes\n\n"
                                               "Adds edges between selected nodes, "
                                               "so that they become a star subgraph.\n"
                                               "You must have some nodes selected."));
    connect(editNodeSelectedToStarAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToStar()));

    editNodeSelectedToCycleAct = new QAction(QIcon(":/images/subgraphcycle_48px.svg"),
                                             tr("Create a cycle from selected nodes "), this);
    editNodeSelectedToCycleAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_Y));
    editNodeSelectedToCycleAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a star -- "
                                                "There must be some nodes selected!"));
    editNodeSelectedToCycleAct->setWhatsThis(tr("Cycle from Selected Nodes\n\n"
                                                "Connect selected nodes "
                                                "so that they become a cycle subgraph.\n"
                                                "A cycle graph or circular graph is a graph that consists of a single cycle, or in other words, the vertices are connected in a closed chain. The cycle graph with n vertices is called Cₙ\n"
                                                "You must have some nodes selected."));
    connect(editNodeSelectedToCycleAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToCycle()));

    editNodeSelectedToLineAct = new QAction(QIcon(":/images/subgraphline.png"),
                                            tr("Create a line from selected nodes "), this);
    editNodeSelectedToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_L));
    editNodeSelectedToLineAct->setStatusTip(tr("Connect selected nodes with edges/arcs to create a line-- "
                                               "There must be some nodes selected!"));
    editNodeSelectedToLineAct->setWhatsThis(tr("Line from Selected Nodes\n\n"
                                               "Adds edges between selected nodes, "
                                               "so that they become a line subgraph.\n"
                                               "You must have some nodes selected."));
    connect(editNodeSelectedToLineAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeSelectedToLine()));

    editNodeColorAll = new QAction(QIcon(":/images/colorize_48px.svg"), tr("Change All Nodes Color"), this);
    editNodeColorAll->setStatusTip(tr("Choose a new color for all nodes."));
    editNodeColorAll->setWhatsThis(tr("Nodes Color\n\n"
                                      "Changes all nodes color at once."));
    connect(editNodeColorAll, SIGNAL(triggered()), this, SLOT(slotEditNodeColorAll()));

    editNodeSizeAllAct = new QAction(QIcon(":/images/size_select_24px.svg"), tr("Change All Nodes Size"), this);
    editNodeSizeAllAct->setStatusTip(tr("Change the size of all nodes"));
    editNodeSizeAllAct->setWhatsThis(tr("Change All Nodes Size\n\n"
                                        "Click to select and apply a new size for all nodes at once."));
    connect(editNodeSizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditNodeSizeAll()));

    editNodeShapeAll = new QAction(QIcon(":/images/format_shapes_48px.svg"), tr("Change All Nodes Shape"), this);
    editNodeShapeAll->setStatusTip(tr("Change the shape of all nodes"));
    editNodeShapeAll->setWhatsThis(tr("Change All Nodes Shape\n\n"
                                      "Click to select and apply a new shape for all nodes at once."));
    connect(editNodeShapeAll, SIGNAL(triggered()), this, SLOT(slotEditNodeShape()));

    editNodeNumbersSizeAct = new QAction(QIcon(":/images/nodenumbersize_48px.svg"),
                                         tr("Change All Node Numbers Size"), this);
    editNodeNumbersSizeAct->setStatusTip(tr("Change the font size of the numbers of all nodes"));
    editNodeNumbersSizeAct->setWhatsThis(tr("Change Node Numbers Size\n\n"
                                            "Click to select and apply a new font size for all node numbers."));
    connect(editNodeNumbersSizeAct, SIGNAL(triggered()),
            this, SLOT(slotEditNodeNumberSize()));

    editNodeNumbersColorAct = new QAction(QIcon(":/images/format_color_text_48px.svg"),
                                          tr("Change All Node Numbers Color"), this);
    editNodeNumbersColorAct->setStatusTip(tr("Change the color of the numbers of all nodes."));
    editNodeNumbersColorAct->setWhatsThis(tr("Node Numbers Color\n\n"
                                             "Click to select and apply a new color "
                                             "to all node numbers."));
    connect(editNodeNumbersColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeNumbersColor()));

    editNodeLabelsSizeAct = new QAction(QIcon(":/images/format_textsize_48px.svg"), tr("Change All Node Labels Size"), this);
    editNodeLabelsSizeAct->setStatusTip(tr("Change the font size of the labels of all nodes"));
    editNodeLabelsSizeAct->setWhatsThis(tr("Node Labels Size\n\n"
                                           "Click to select and apply a new font-size to all node labels."));
    connect(editNodeLabelsSizeAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelSize()));

    editNodeLabelsColorAct = new QAction(QIcon(":/images/format_color_text_48px.svg"), tr("Change All Node Labels Color"), this);
    editNodeLabelsColorAct->setStatusTip(tr("Change the color of the labels of all nodes"));
    editNodeLabelsColorAct->setWhatsThis(tr("Labels Color\n\n"
                                            "Click to select and apply a new color to all node labels."));
    connect(editNodeLabelsColorAct, SIGNAL(triggered()), this, SLOT(slotEditNodeLabelsColor()));

    editEdgeAddAct = new QAction(QIcon(":/images/edge_add_48px.svg"), tr("Add Edge (arc)"), this);
    editEdgeAddAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    editEdgeAddAct->setStatusTip(tr("Add a directed edge (arc) from a node to another. "));
    editEdgeAddAct->setToolTip(
        tr("Add a new edge from a node to another.\n\n"
           "You can also create an edge between two nodes \n"
           "by double-clicking on them consecutively."));
    editEdgeAddAct->setWhatsThis(
        tr("Add edge\n\n"
           "Adds a new edge from a node to another.\n\n"
           "Alternately, you can create a new edge between two nodes "
           "by double-clicking on them consecutively."));
    connect(editEdgeAddAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeAdd()));

    editEdgeRemoveAct = new QAction(QIcon(":/images/edge_remove_48px.svg"), tr("Remove Edge"), this);
    editEdgeRemoveAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Slash));
    editEdgeRemoveAct->setToolTip(tr("Remove selected edges from the network. \n\n"
                                     "If no edge has been clicked or selected, you will be prompted \n"
                                     "to enter edge source and target nodes for the edge to remove."));
    editEdgeRemoveAct->setStatusTip(tr("Remove selected Edge(s)"));
    editEdgeRemoveAct->setWhatsThis(tr("Remove Edge\n\n"
                                       "Removes edges from the network. \n"
                                       "If one or more edges has been clicked or selected, they are removed. "
                                       "Otherwise, you will be prompted to enter edge source and target "
                                       "nodes for the edge to remove."));
    connect(editEdgeRemoveAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeRemove()));

    editEdgePropertiesAct = new QAction(QIcon(":/images/edge_properties_48px.svg"), tr("Edge Properties"), this);
    editEdgePropertiesAct->setStatusTip(tr("Edit the properties and custom attributes of the clicked edge"));
    editEdgePropertiesAct->setWhatsThis(tr("Edge Properties\n\n"
                                           "Opens a dialog to edit the label, weight, color "
                                           "and custom attributes of the selected edge."));
    connect(editEdgePropertiesAct, &QAction::triggered, this, &MainWindow::slotEditEdgePropertiesDialog);

    editEdgeLabelAct = new QAction(QIcon(":/images/format_textsize_48px.svg"), tr("Change Edge Label"), this);
    editEdgeLabelAct->setStatusTip(tr("Change the Label of an Edge"));
    editEdgeLabelAct->setWhatsThis(tr("Change Edge Label\n\n"
                                      "Changes the label of an Edge"));
    connect(editEdgeLabelAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeLabel()));

    editEdgeColorAct = new QAction(QIcon(":/images/colorize_48px.svg"), tr("Change Edge Color"), this);
    editEdgeColorAct->setStatusTip(tr("Change the Color of an Edge"));
    editEdgeColorAct->setWhatsThis(tr("Change Edge Color\n\n"
                                      "Changes the Color of an Edge"));
    connect(editEdgeColorAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColor()));

    editEdgeWeightAct = new QAction(QIcon(":/images/line_weight_48px.svg"), tr("Change Edge Weight"), this);
    editEdgeWeightAct->setStatusTip(tr("Change the weight of an Edge"));
    editEdgeWeightAct->setWhatsThis(tr("Edge Weight\n\n"
                                       "Changes the Weight of an Edge"));
    connect(editEdgeWeightAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeWeight()));

    editEdgeColorAllAct = new QAction(QIcon(":/images/colorize_48px.svg"), tr("Change All Edges Color"), this);
    editEdgeColorAllAct->setStatusTip(tr("Change the color of all Edges."));
    editEdgeColorAllAct->setWhatsThis(tr("All Edges Color\n\n"
                                         "Changes the color of all Edges"));
    connect(editEdgeColorAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeColorAll()));

    editEdgeSymmetrizeAllAct = new QAction(QIcon(":/images/symmetrize.png"), tr("Symmetrize All Edges"), this);
    editEdgeSymmetrizeAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_S));
    editEdgeSymmetrizeAllAct->setStatusTip(tr("Make all directed ties to be reciprocated (thus, a symmetric graph)."));
    editEdgeSymmetrizeAllAct->setWhatsThis(
        tr("<p><b>Symmetrize All Edges</b></p>"
           "<p>Forces all edges in this relation to be reciprocated: "
           "<p>If there is a directed edge from node A to node B \n"
           "then a new directed edge from node B to node A will be \n"
           " created, with the same weight. </p>"
           "<p>The result is a symmetric network.</p>"));
    connect(editEdgeSymmetrizeAllAct, SIGNAL(triggered()), this, SLOT(slotEditEdgeSymmetrizeAll()));

    editEdgeSymmetrizeStrongTiesAct = new QAction(QIcon(":/images/symmetrize_48px.svg"), tr("Symmetrize by Strong Ties"), this);
    editEdgeSymmetrizeStrongTiesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_T));
    editEdgeSymmetrizeStrongTiesAct->setStatusTip(tr("Create a new symmetric relation by counting reciprocated ties only (strong ties)."));
    editEdgeSymmetrizeStrongTiesAct->setWhatsThis(
        tr("<p><b>Symmetrize Edges by Strong Ties:</b></p>"
           "<p>Creates a new symmetric relation by keeping strong ties only. </p>"
           "<p>A tie between actors A and B is considered strong if both A -> B and B -> A exist. "
           "Therefore, in the new relation, a reciprocated edge will be created between actors A and B "
           "only if both arcs A->B and B->A were present in the current or all relations. </p>"
           "<p>If the network is multi-relational, it will ask you whether "
           "ties in the current relation or all relations are to be considered.</p>"));
    connect(editEdgeSymmetrizeStrongTiesAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeStrongTies()));

    // TODO Separate action for Directed/Undirected graph drawing (without changing all existing edges).
    editEdgeUndirectedAllAct = new QAction(tr("Undirected Edges"), this);
    editEdgeUndirectedAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_U));
    editEdgeUndirectedAllAct->setStatusTip(tr("Enable to transform all arcs to undirected edges and hereafter work with undirected edges ."));
    editEdgeUndirectedAllAct->setWhatsThis(
        tr("Undirected Edges\n\n"
           "Transforms all directed arcs to undirected edges. \n"
           "The result is a undirected and symmetric network."
           "After that, every new edge you add, will be undirected too."
           "If you disable this, then all edges become directed again."));
    editEdgeUndirectedAllAct->setCheckable(true);
    editEdgeUndirectedAllAct->setChecked(false);
    connect(editEdgeUndirectedAllAct, SIGNAL(triggered(bool)),
            this, SLOT(slotEditEdgeUndirectedAll(bool)));

    editEdgesCocitationAct = new QAction(QIcon(":/images/cocitation_48px.svg"), tr("Cocitation Network"), this);
    editEdgesCocitationAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_C));
    editEdgesCocitationAct->setStatusTip(tr("Create a new symmetric relation by "
                                            "connecting actors that are cocitated by others."));
    editEdgesCocitationAct->setWhatsThis(
        tr("<p><b>Symmetrize Edges by examining Cocitation:</b></p>"
           "<p>Creates a new symmetric relation by connecting actors "
           "that are cocitated by others. "
           "In the new relation, an edge will be created between actor i and "
           "actor j only if C(i,j) > 0, where C the Cocitation Matrix. </p>"
           "<p>Thus the actor pairs cited by more common neighbors will appear "
           "with a stronger tie between them than pairs those cited by fewer "
           "common neighbors. "
           "The resulting relation is symmetric.</p>"));
    connect(editEdgesCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeSymmetrizeCocitation()));

    editEdgeDichotomizeAct = new QAction(QIcon(":/images/filter_list_48px.svg"), tr("Dichotomize Valued Edges"), this);
    editEdgeDichotomizeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_D));
    editEdgeDichotomizeAct->setStatusTip(tr("Create a new binary relation/graph in a valued network "
                                            "using edge dichotomization."));
    editEdgeDichotomizeAct->setWhatsThis(
        tr("Dichotomize Edges\n\n"
           "Creates a new binary relation in a valued network using "
           "edge dichotomization according to a given threshold value. \n"
           "In the new dichotomized relation, an edge will exist between actor i and "
           "actor j only if e(i,j) > threshold, where threshold is a user-defined value."
           "Thus the dichotomization procedure is as follows: "
           "Choose a threshold value, set all ties with equal or higher values "
           "to equal one, and all lower to equal zero."
           "The result is a binary (dichotomized) graph. "
           "The process is also known as compression and slicing"));
    connect(editEdgeDichotomizeAct, SIGNAL(triggered()),
            this, SLOT(slotEditEdgeDichotomizationDialog()));

    transformNodes2EdgesAct = new QAction(tr("Transform Nodes to Edges"), this);
    transformNodes2EdgesAct->setStatusTip(tr("Transforms the network so that "
                                             "nodes become Edges and vice versa"));
    transformNodes2EdgesAct->setWhatsThis(tr("Transform Nodes EdgesAct\n\n"
                                             "Transforms network so that nodes become Edges and vice versa"));
    connect(transformNodes2EdgesAct, SIGNAL(triggered()),
            this, SLOT(slotEditTransformNodes2Edges()));

    filterNodesByCentralityAct = new QAction(QIcon(":/images/filter_centrality_48px.svg"), tr("Filter Nodes By Centrality"), this);
    filterNodesByCentralityAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_E));
    filterNodesByCentralityAct->setStatusTip(tr("Temporarily filter out nodes according to their centrality score."));
    filterNodesByCentralityAct->setWhatsThis(tr("Filter Nodes By Centrality\n\n"
                                                "Filters out nodes according to their score in a user-selected centrality index."));
    connect(filterNodesByCentralityAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesDialogByCentrality()));

    filterNodesByAttributeAct = new QAction(QIcon(":/images/filter_attribute_48px.svg"), tr("Filter by Attribute..."), this);
    filterNodesByAttributeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_A));
    filterNodesByAttributeAct->setStatusTip(tr("Show only nodes or edges matching a custom attribute condition (key, operator, value)."));
    filterNodesByAttributeAct->setWhatsThis(tr("Filter by Attribute\n\n"
                                               "Opens a dialog to filter nodes, edges, or both by a custom attribute. "
                                               "Non-matching elements are hidden (reversible via Restore All Nodes)."));
    connect(filterNodesByAttributeAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesByAttribute()));

    filterByQueryBuilderAct = new QAction(QIcon(":/images/filter_list_48px.svg"), tr("Query Builder..."), this);
    filterByQueryBuilderAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_B));
    filterByQueryBuilderAct->setStatusTip(tr("Build a multi-condition AND filter for nodes or edges."));
    filterByQueryBuilderAct->setWhatsThis(tr("Query Builder\n\n"
                                             "Opens the Query Builder dialog to compose a filter with "
                                             "multiple attribute conditions (AND logic). All conditions "
                                             "must match for a node or edge to remain visible."));
    connect(filterByQueryBuilderAct, SIGNAL(triggered()), this, SLOT(slotFilterByQueryBuilder()));

    filterNodesBySelectionAct = new QAction(tr("Focus on Selection"), this);
    filterNodesBySelectionAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_O));
    filterNodesBySelectionAct->setStatusTip(tr("Show only the selected nodes and edges between them."));
    filterNodesBySelectionAct->setWhatsThis(tr("Focus on Selection\n\n"
                                               "Hides all nodes except the currently selected ones. "
                                               "Only edges between selected nodes remain visible. "
                                               "Use 'Restore All Nodes' to undo."));
    filterNodesBySelectionAct->setEnabled(false); // enabled when >= 1 node selected
    connect(filterNodesBySelectionAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesBySelection()));

    filterNodesByEgoNetworkAct = new QAction(tr("Focus on Node (Ego Network)"), this);
    filterNodesByEgoNetworkAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_F));
    filterNodesByEgoNetworkAct->setStatusTip(tr("Show only the selected node and its direct neighbors."));
    filterNodesByEgoNetworkAct->setWhatsThis(tr("Focus on Node (Ego Network)\n\n"
                                                "Hides all nodes except the selected node and its direct neighbors. "
                                                "Use 'Restore All Nodes' to undo."));
    filterNodesByEgoNetworkAct->setEnabled(false); // enabled only when exactly 1 node selected
    connect(filterNodesByEgoNetworkAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesByEgoNetwork()));

    filterNodesRestoreAllAct = new QAction(QIcon(":/images/filter_restore_nodes_48px.svg"), tr("Restore All Nodes"), this);
    filterNodesRestoreAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_R));
    filterNodesRestoreAllAct->setStatusTip(tr("Restore all nodes hidden by the last filter."));
    filterNodesRestoreAllAct->setEnabled(false); // enabled only when history stack is non-empty
    connect(filterNodesRestoreAllAct, SIGNAL(triggered()), this, SLOT(slotFilterNodesRestoreAll()));

    editFilterNodesIsolatesAct = new QAction(tr("Disable Isolate Nodes"), this);
    editFilterNodesIsolatesAct->setEnabled(true);
    editFilterNodesIsolatesAct->setCheckable(true);
    editFilterNodesIsolatesAct->setChecked(false);
    editFilterNodesIsolatesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_I));
    editFilterNodesIsolatesAct->setStatusTip(tr("Temporarily filter out nodes with no edges"));
    editFilterNodesIsolatesAct->setWhatsThis(tr("Filter Isolate Nodes\n\n"
                                                "Enables or disables displaying of isolate nodes. "
                                                "Isolate nodes are those with no edges..."));
    connect(editFilterNodesIsolatesAct, SIGNAL(toggled(bool)),
            this, SLOT(slotEditFilterNodesIsolates(bool)));

    editFilterEdgesByWeightAct = new QAction(QIcon(":/images/filter_edges_48px.svg"), tr("Filter Edges by Weight"), this);
    editFilterEdgesByWeightAct->setEnabled(true);
    editFilterEdgesByWeightAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_F));
    editFilterEdgesByWeightAct->setStatusTip(tr("Temporarily filter edges of some weight out of the network"));
    editFilterEdgesByWeightAct->setWhatsThis(tr("Filter Edges\n\n"
                                                "Filters edges according to their weight."));
    connect(editFilterEdgesByWeightAct, SIGNAL(triggered()),
            this, SLOT(slotEditFilterEdgesByWeightDialog()));

    editFilterEdgesRestoreAllAct = new QAction(QIcon(":/images/filter_restore_edges_48px.svg"), tr("Restore All Edges"), this);
    editFilterEdgesRestoreAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_R));
    editFilterEdgesRestoreAllAct->setStatusTip(tr("Restore all edges hidden by the weight filter."));
    editFilterEdgesRestoreAllAct->setWhatsThis(tr("Restore All Edges\n\n"
                                                  "Re-enables all edges hidden by the weight filter. "
                                                  "No data is modified."));
    editFilterEdgesRestoreAllAct->setEnabled(false);
    connect(editFilterEdgesRestoreAllAct, SIGNAL(triggered()),
            this, SLOT(slotEditFilterEdgesReset()));

    editFilterEdgesUnilateralAct = new QAction(tr("Disable unilateral edges"), this);
    editFilterEdgesUnilateralAct->setEnabled(true);
    editFilterEdgesUnilateralAct->setCheckable(true);
    editFilterEdgesUnilateralAct->setChecked(false);
    editFilterEdgesUnilateralAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E, Qt::CTRL | Qt::Key_E));
    editFilterEdgesUnilateralAct->setStatusTip(tr("Temporarily disable all unilateral (non-reciprocal) edges in this relation. Keeps only \"strong\" ties."));
    editFilterEdgesUnilateralAct->setWhatsThis(tr("Unilateral edges\n\n"
                                                  "In directed networks, a tie between two actors "
                                                  "is unilateral when only one actor identifies the other "
                                                  "as connected (i.e. friend, vote, etc). "
                                                  "A unilateral tie is depicted as a single arc. "
                                                  "These ties are considered weak, as opposed to "
                                                  "reciprocal ties where both actors identify each other as connected. "
                                                  "Strong ties are depicted as either a single undirected edge "
                                                  "or as two reciprocated arcs between two nodes. "
                                                  "By selecting this option, all unilateral edges in this relation will be disabled."));
    connect(editFilterEdgesUnilateralAct, SIGNAL(triggered(bool)),
            this, SLOT(slotEditFilterEdgesUnilateral(bool)));

    editSubgraphExtractAct = new QAction(tr("Save visible nodes as subgraph..."), this);
    editSubgraphExtractAct->setEnabled(true);
    editSubgraphExtractAct->setStatusTip(tr("Copy the currently visible (non-filtered) nodes and their inter-edges into a new graph file."));
    editSubgraphExtractAct->setWhatsThis(tr("Save visible nodes as subgraph\n\n"
                                            "Creates an independent copy of the nodes and edges that are currently "
                                            "visible on the canvas (i.e. not hidden by any filter). "
                                            "Vertices are renumbered from 1; all visual properties and custom "
                                            "attributes are preserved. You will be prompted for a name and a "
                                            "save location."));
    connect(editSubgraphExtractAct, &QAction::triggered,
            this, &MainWindow::slotEditSubgraphExtract);

    editSubgraphExtractFromSelectionAct = new QAction(tr("Save selected nodes as subgraph..."), this);
    editSubgraphExtractFromSelectionAct->setEnabled(false); // enabled when >= 1 node selected
    editSubgraphExtractFromSelectionAct->setStatusTip(tr("Copy the currently selected nodes and their inter-edges into a new graph file."));
    editSubgraphExtractFromSelectionAct->setWhatsThis(tr("Save selected nodes as subgraph\n\n"
                                                         "Creates an independent copy of the selected nodes and the edges "
                                                         "that run between them. Vertices are renumbered from 1; all visual "
                                                         "properties and custom attributes are preserved. You will be "
                                                         "prompted for a name and a save location."));
    connect(editSubgraphExtractFromSelectionAct, &QAction::triggered,
            this, &MainWindow::slotEditSubgraphExtractFromSelection);

    /**
    Layout menu actions
    */
    strongColorationAct = new QAction(tr("Strong Structural"), this);
    strongColorationAct->setStatusTip(tr("Nodes are assigned the same color if they have identical in and out neighborhoods"));
    strongColorationAct->setWhatsThis(tr("Click this to colorize nodes; Nodes are assigned the same color if they have identical in and out neighborhoods"));
    connect(strongColorationAct, SIGNAL(triggered()), this, SLOT(slotLayoutColorationStrongStructural()));

    regularColorationAct = new QAction(tr("Regular"), this);
    regularColorationAct->setStatusTip(
        tr("Nodes are assigned the same color if they have "
           "neighborhoods of the same set of colors"));
    regularColorationAct
        ->setWhatsThis(
            tr("Click this to colorize nodes; "
               "Nodes are assigned the same color if they have neighborhoods "
               "of the same set of colors"));
    connect(regularColorationAct, SIGNAL(triggered()), this, SLOT(slotLayoutColorationRegular())); // TODO

    layoutRandomAct = new QAction(tr("Random"), this);
    layoutRandomAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_0));
    layoutRandomAct->setStatusTip(tr("Layout the network actors in random positions."));
    layoutRandomAct->setWhatsThis(tr("Random Layout\n\n "
                                     "This layout algorithm repositions all "
                                     "network actors in random positions."));
    connect(layoutRandomAct, SIGNAL(triggered()), this, SLOT(slotLayoutRandom()));

    layoutRandomRadialAct = new QAction(tr("Random Circles"), this);
    layoutRandomRadialAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_0));
    layoutRandomRadialAct->setStatusTip(tr("Layout the network in random concentric circles"));
    layoutRandomRadialAct->setWhatsThis(
        tr("Random Circles Layout\n\n Repositions the nodes randomly on circles"));
    connect(layoutRandomRadialAct, SIGNAL(triggered()), this, SLOT(slotLayoutRadialRandom()));

    layoutEgoRadialAct = new QAction(QIcon(":/images/ego_radial_layout_48px.svg"), tr("Ego Radial layout"), this);
    layoutEgoRadialAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E));
    layoutEgoRadialAct->setStatusTip(tr("Place the selected vertex at center, its neighbors on an inner ring, all others on an outer ring"));
    layoutEgoRadialAct->setWhatsThis(tr("Ego Radial Layout\n\nPlaces the selected vertex at the canvas center, its 1-hop out-neighbors on an inner ring, and all remaining nodes on an outer ring."));
    connect(layoutEgoRadialAct, SIGNAL(triggered()), this, SLOT(slotLayoutEgoRadial()));

    layoutRadialProminence_DC_Act = new QAction(tr("Degree Centrality"), this);
    layoutRadialProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_1));
    layoutRadialProminence_DC_Act
        ->setStatusTip(
            tr("Place all nodes on concentric circles of radius inversely "
               "proportional to their Degree Centrality."));
    layoutRadialProminence_DC_Act->setWhatsThis(
        tr("Degree Centrality (DC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Degree Centrality score. "
           "Nodes with higher DC are closer to the centre."));
    connect(layoutRadialProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_CC_Act = new QAction(tr("Closeness Centrality"), this);
    layoutRadialProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_2));
    layoutRadialProminence_CC_Act
        ->setStatusTip(
            tr("Place all nodes on concentric circles of radius inversely "
               "proportional to their Closeness Centrality."));
    layoutRadialProminence_CC_Act->setWhatsThis(
        tr("Closeness Centrality (CC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Closeness Centrality. "
           "Nodes having higher CC are closer to the centre."));
    connect(layoutRadialProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_IRCC_Act = new QAction(
        tr("Influence Range Closeness Centrality"), this);
    layoutRadialProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_3));
    layoutRadialProminence_IRCC_Act
        ->setStatusTip(
            tr("Place all nodes on concentric circles of radius inversely "
               "proportional to their Influence Range Closeness Centrality."));
    layoutRadialProminence_IRCC_Act->setWhatsThis(
        tr("Influence Range Closeness Centrality (IRCC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their IRCC score. "
           "Nodes having higher IRCC are closer to the centre."));
    connect(layoutRadialProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_BC_Act = new QAction(tr("Betweenness Centrality"), this);
    layoutRadialProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_4));
    layoutRadialProminence_BC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Betweenness Centrality."));
    layoutRadialProminence_BC_Act->setWhatsThis(
        tr("Betweenness Centrality (BC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Betweenness Centrality. "
           "Nodes having higher BC are closer to the centre."));
    connect(layoutRadialProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_SC_Act = new QAction(tr("Stress Centrality"), this);
    layoutRadialProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_5));
    layoutRadialProminence_SC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Stress Centrality."));
    layoutRadialProminence_SC_Act->setWhatsThis(
        tr("Stress Centrality (SC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Stress Centrality score. "
           "Nodes having higher SC are closer to the centre."));
    connect(layoutRadialProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_EC_Act = new QAction(tr("Eccentricity Centrality"), this);
    layoutRadialProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_6));
    layoutRadialProminence_EC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutRadialProminence_EC_Act->setWhatsThis(
        tr("Eccentricity Centrality (EC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Eccentricity Centrality "
           "(aka Harary Graph Centrality) score. "
           "Nodes having higher EC are closer to the centre."));
    connect(layoutRadialProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_PC_Act = new QAction(tr("Power Centrality"), this);
    layoutRadialProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_7));
    layoutRadialProminence_PC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Power Centrality."));
    layoutRadialProminence_PC_Act->setWhatsThis(
        tr("Power Centrality (PC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Power Centrality score. "
           "Nodes having higher PC are closer to the centre."));
    connect(layoutRadialProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_IC_Act = new QAction(tr("Information Centrality"), this);
    layoutRadialProminence_IC_Act->setEnabled(true);
    layoutRadialProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_8));
    layoutRadialProminence_IC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Information Centrality."));
    layoutRadialProminence_IC_Act->setWhatsThis(
        tr("Information Centrality (IC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Information Centrality score. "
           "Nodes of higher IC are closer to the centre."));
    connect(layoutRadialProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_EVC_Act = new QAction(tr("Eigenvector Centrality"), this);
    layoutRadialProminence_EVC_Act->setEnabled(true);
    layoutRadialProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_9));
    layoutRadialProminence_EVC_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Eigenvector Centrality."));
    layoutRadialProminence_EVC_Act->setWhatsThis(
        tr("Eigenvector Centrality (EVC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Eigenvector Centrality score. "
           "Nodes of higher EVC are closer to the centre."));
    connect(layoutRadialProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_KATZ_Act = new QAction(tr("Katz Centrality"), this);
    layoutRadialProminence_KATZ_Act->setEnabled(true);
    layoutRadialProminence_KATZ_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Katz Centrality."));
    layoutRadialProminence_KATZ_Act->setWhatsThis(
        tr("Katz Centrality (KC) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their Katz Centrality score. "
           "Nodes of higher KC are closer to the centre. "
           "Requires Katz Centrality to have been computed first "
           "(Analyze > Centrality > Katz Centrality)."));
    connect(layoutRadialProminence_KATZ_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_DP_Act = new QAction(tr("Degree Prestige"), this);
    layoutRadialProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_I));
    layoutRadialProminence_DP_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Degree Prestige (inDegree)."));
    layoutRadialProminence_DP_Act->setWhatsThis(
        tr("Degree Prestige (DP) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their inDegree score. "
           "Nodes having higher DP are closer to the centre."));
    connect(layoutRadialProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_PRP_Act = new QAction(tr("PageRank Prestige"), this);
    layoutRadialProminence_PRP_Act->setEnabled(true);
    layoutRadialProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_K));
    layoutRadialProminence_PRP_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their PRP index."));
    layoutRadialProminence_PRP_Act->setWhatsThis(
        tr("PageRank Prestige (PRP) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their PageRank score. "
           "Nodes having higher PRP are closer to the centre."));
    connect(layoutRadialProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutRadialProminence_PP_Act = new QAction(tr("Proximity Prestige"), this);
    layoutRadialProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_R, Qt::CTRL | Qt::Key_Y));
    layoutRadialProminence_PP_Act->setStatusTip(
        tr("Place all nodes on concentric circles of radius inversely "
           "proportional to their Proximity Prestige."));
    layoutRadialProminence_PP_Act->setWhatsThis(
        tr("Proximity Prestige (PP) Radial Layout\n\n"
           "Repositions all nodes on concentric circles of radius "
           "inversely proportional to their PP index. "
           "Nodes having higher PP score are closer to the centre."));
    connect(layoutRadialProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutRadialByProminenceIndex()));

    layoutLevelProminence_DC_Act = new QAction(tr("Degree Centrality"), this);
    layoutLevelProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_1));
    layoutLevelProminence_DC_Act
        ->setStatusTip(
            tr("Place all nodes on horizontal levels of height "
               "proportional to their Degree Centrality."));
    layoutLevelProminence_DC_Act->setWhatsThis(
        tr("Degree Centrality (DC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their DC score. "
           "Nodes having higher DC are closer to the top.\n\n"));
    connect(layoutLevelProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_CC_Act = new QAction(tr("Closeness Centrality"), this);
    layoutLevelProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_2));
    layoutLevelProminence_CC_Act
        ->setStatusTip(
            tr("Place all nodes on horizontal levels of height "
               "proportional to their Closeness Centrality."));
    layoutLevelProminence_CC_Act->setWhatsThis(
        tr("Closeness Centrality (CC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Closeness Centrality score. "
           "Nodes of higher CC are closer to the top.\n\n"
           "This layout can be computed only for connected graphs. "));
    connect(layoutLevelProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_IRCC_Act = new QAction(
        tr("Influence Range Closeness Centrality"), this);
    layoutLevelProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_3));
    layoutLevelProminence_IRCC_Act
        ->setStatusTip(
            tr("Place all nodes on horizontal levels of height "
               "proportional to their Influence Range Closeness Centrality."));
    layoutLevelProminence_IRCC_Act->setWhatsThis(
        tr("Influence Range Closeness Centrality (IRCC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their IRCC score. "
           "Nodes having higher IRCC are closer to the top.\n\n"
           "This layout can be computed for not connected graphs. "));
    connect(layoutLevelProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_BC_Act = new QAction(tr("Betweenness Centrality"), this);
    layoutLevelProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_4));
    layoutLevelProminence_BC_Act->setStatusTip(
        tr("Place all nodes on horizontal levels of height "
           "proportional to their Betweenness Centrality."));
    layoutLevelProminence_BC_Act->setWhatsThis(
        tr("Betweenness Centrality (BC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Betweenness Centrality score. "
           "Nodes having higher BC are closer to the top."));
    connect(layoutLevelProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_SC_Act = new QAction(tr("Stress Centrality"), this);
    layoutLevelProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_5));
    layoutLevelProminence_SC_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Stress Centrality."));
    layoutLevelProminence_SC_Act->setWhatsThis(
        tr("Stress Centrality (SC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Stress Centrality score. "
           "Nodes having higher SC are closer to the top."));
    connect(layoutLevelProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EC_Act = new QAction(tr("Eccentricity Centrality"), this);
    layoutLevelProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_6));
    layoutLevelProminence_EC_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutLevelProminence_EC_Act->setWhatsThis(
        tr("Eccentricity Centrality (EC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Eccentricity Centrality "
           "(aka Harary Graph Centrality) score. "
           "Nodes having higher EC are closer to the top."));
    connect(layoutLevelProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_PC_Act = new QAction(tr("Power Centrality"), this);
    layoutLevelProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_7));
    layoutLevelProminence_PC_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Power Centrality."));
    layoutLevelProminence_PC_Act->setWhatsThis(
        tr("Power Centrality (PC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Power Centrality score. "
           "Nodes having higher PC are closer to the top."));
    connect(layoutLevelProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_IC_Act = new QAction(tr("Information Centrality"), this);
    layoutLevelProminence_IC_Act->setEnabled(true);
    layoutLevelProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_8));
    layoutLevelProminence_IC_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Information Centrality."));
    layoutLevelProminence_IC_Act->setWhatsThis(
        tr("Information Centrality (IC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Information Centrality score. "
           "Nodes having higher IC are closer to the top."));
    connect(layoutLevelProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_EVC_Act = new QAction(tr("Eigenvector Centrality"), this);
    layoutLevelProminence_EVC_Act->setEnabled(true);
    layoutLevelProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_9));
    layoutLevelProminence_EVC_Act->setStatusTip(
        tr(
            "Place nodes on horizontal levels of height "
            "proportional to their Eigenvector Centrality."));
    layoutLevelProminence_EVC_Act->setWhatsThis(
        tr("Eigenvector Centrality (EVC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Eigenvector Centrality score. "
           "Nodes having higher EVC are closer to the top."));
    connect(layoutLevelProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_KATZ_Act = new QAction(tr("Katz Centrality"), this);
    layoutLevelProminence_KATZ_Act->setEnabled(true);
    layoutLevelProminence_KATZ_Act->setStatusTip(
        tr(
            "Place nodes on horizontal levels of height "
            "proportional to their Katz Centrality."));
    layoutLevelProminence_KATZ_Act->setWhatsThis(
        tr("Katz Centrality (KC) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height "
           "proportional to their Katz Centrality score. "
           "Nodes having higher KC are closer to the top. "
           "Requires Katz Centrality to have been computed first "
           "(Analyze > Centrality > Katz Centrality)."));
    connect(layoutLevelProminence_KATZ_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_DP_Act = new QAction(tr("Degree Prestige"), this);
    layoutLevelProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_I));
    layoutLevelProminence_DP_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Degree Prestige."));
    layoutLevelProminence_DP_Act->setWhatsThis(
        tr("Degree Prestige (DP) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Degree Prestige score. "
           "Nodes having higher DP are closer to the top."));
    connect(layoutLevelProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_PRP_Act = new QAction(tr("PageRank Prestige"), this);
    layoutLevelProminence_PRP_Act->setEnabled(true);
    layoutLevelProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_K));
    layoutLevelProminence_PRP_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their PageRank Prestige."));
    layoutLevelProminence_PRP_Act->setWhatsThis(
        tr("PageRank Prestige (PRP) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their PageRank Prestige score. "
           "Nodes having higher PRP are closer to the top."));
    connect(layoutLevelProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutLevelProminence_PP_Act = new QAction(tr("Proximity Prestige"), this);
    layoutLevelProminence_PP_Act->setEnabled(true);
    layoutLevelProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_Y));
    layoutLevelProminence_PP_Act->setStatusTip(
        tr("Place nodes on horizontal levels of height "
           "proportional to their Proximity Prestige."));
    layoutLevelProminence_PP_Act->setWhatsThis(
        tr("Proximity Prestige (PP) Levels Layout\n\n"
           "Repositions all nodes on horizontal levels of height"
           "proportional to their Proximity Prestige score. "
           "Nodes having higher PP are closer to the top."));
    connect(layoutLevelProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutLevelByProminenceIndex()));

    layoutNodeSizeProminence_DC_Act = new QAction(tr("Degree Centrality"), this);
    layoutNodeSizeProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_1));
    layoutNodeSizeProminence_DC_Act
        ->setStatusTip(
            tr("Resize all nodes to be "
               "proportional to their Degree Centrality."));
    layoutNodeSizeProminence_DC_Act->setWhatsThis(
        tr(
            "Degree Centrality (DC) Node Size Layout\n\n"
            "Changes the size of all nodes to be "
            "proportional to their DC (inDegree) score. \n\n"
            "Nodes having higher DC will appear bigger."));
    connect(layoutNodeSizeProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_CC_Act = new QAction(tr("Closeness Centrality"), this);
    layoutNodeSizeProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_2));
    layoutNodeSizeProminence_CC_Act
        ->setStatusTip(
            tr("Resize all nodes to be "
               "proportional to their Closeness Centrality."));
    layoutNodeSizeProminence_CC_Act->setWhatsThis(
        tr("Closeness Centrality (CC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their CC score. "
           "Nodes of higher CC will appear bigger.\n\n"
           "This layout can be computed only for connected graphs. "));
    connect(layoutNodeSizeProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_IRCC_Act = new QAction(
        tr("Influence Range Closeness Centrality"), this);
    layoutNodeSizeProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_3));
    layoutNodeSizeProminence_IRCC_Act
        ->setStatusTip(
            tr("Resize all nodes to be proportional "
               "to their Influence Range Closeness Centrality."));
    layoutNodeSizeProminence_IRCC_Act->setWhatsThis(
        tr("Influence Range Closeness Centrality (IRCC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their IRCC score. "
           "Nodes having higher IRCC will appear bigger.\n\n"
           "This layout can be computed for not connected graphs. "));
    connect(layoutNodeSizeProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_BC_Act = new QAction(tr("Betweenness Centrality"), this);
    layoutNodeSizeProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_4));
    layoutNodeSizeProminence_BC_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Betweenness Centrality."));
    layoutNodeSizeProminence_BC_Act->setWhatsThis(
        tr("Betweenness Centrality (BC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Betweenness Centrality score. "
           "Nodes having higher BC will appear bigger."));
    connect(layoutNodeSizeProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_SC_Act = new QAction(tr("Stress Centrality"), this);
    layoutNodeSizeProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_5));
    layoutNodeSizeProminence_SC_Act->setStatusTip(
        tr("Resize all nodes to be  "
           "proportional to their Stress Centrality."));
    layoutNodeSizeProminence_SC_Act->setWhatsThis(
        tr("Stress Centrality (SC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Stress Centrality score. "
           "Nodes having higher SC will appear bigger."));
    connect(layoutNodeSizeProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EC_Act = new QAction(tr("Eccentricity Centrality"), this);
    layoutNodeSizeProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_6));
    layoutNodeSizeProminence_EC_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutNodeSizeProminence_EC_Act->setWhatsThis(
        tr("Eccentricity Centrality (EC) NodeSizes Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Eccentricity Centrality (aka Harary Graph Centrality) score. "
           "Nodes having higher EC will appear bigger."));
    connect(layoutNodeSizeProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_PC_Act = new QAction(tr("Power Centrality"), this);
    layoutNodeSizeProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_7));
    layoutNodeSizeProminence_PC_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Power Centrality."));
    layoutNodeSizeProminence_PC_Act->setWhatsThis(
        tr("Power Centrality (PC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Power Centrality score. "
           "Nodes having higher PC will appear bigger."));
    connect(layoutNodeSizeProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_IC_Act = new QAction(tr("Information Centrality"), this);
    layoutNodeSizeProminence_IC_Act->setEnabled(true);
    layoutNodeSizeProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_8));
    layoutNodeSizeProminence_IC_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Information Centrality."));
    layoutNodeSizeProminence_IC_Act->setWhatsThis(
        tr("Information Centrality (IC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Information Centrality score. "
           "Nodes having higher IC will appear bigger."));
    connect(layoutNodeSizeProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_EVC_Act = new QAction(tr("Eigenvector Centrality"), this);
    layoutNodeSizeProminence_EVC_Act->setEnabled(true);
    layoutNodeSizeProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_9));
    layoutNodeSizeProminence_EVC_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Eigenvector Centrality."));
    layoutNodeSizeProminence_EVC_Act->setWhatsThis(
        tr("Eigenvector Centrality (EVC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Eigenvector Centrality score. "
           "Nodes having higher EVC will appear bigger."));
    connect(layoutNodeSizeProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_KATZ_Act = new QAction(tr("Katz Centrality"), this);
    layoutNodeSizeProminence_KATZ_Act->setEnabled(true);
    layoutNodeSizeProminence_KATZ_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Katz Centrality."));
    layoutNodeSizeProminence_KATZ_Act->setWhatsThis(
        tr("Katz Centrality (KC) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Katz Centrality score. "
           "Nodes having higher KC will appear bigger. "
           "Requires Katz Centrality to have been computed first "
           "(Analyze > Centrality > Katz Centrality)."));
    connect(layoutNodeSizeProminence_KATZ_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_DP_Act = new QAction(tr("Degree Prestige"), this);
    layoutNodeSizeProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_I));
    layoutNodeSizeProminence_DP_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Degree Prestige."));
    layoutNodeSizeProminence_DP_Act->setWhatsThis(
        tr("Degree Prestige (DP) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Degree Prestige score. "
           "Nodes having higher DP will appear bigger."));
    connect(layoutNodeSizeProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_PRP_Act = new QAction(tr("PageRank Prestige"), this);
    layoutNodeSizeProminence_PRP_Act->setEnabled(true);
    layoutNodeSizeProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_K));
    layoutNodeSizeProminence_PRP_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their PageRank Prestige."));
    layoutNodeSizeProminence_PRP_Act->setWhatsThis(
        tr("PageRank Prestige (PRP) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their PageRank Prestige score. "
           "Nodes having higher PRP will appear bigger."));
    connect(layoutNodeSizeProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeSizeProminence_PP_Act = new QAction(tr("Proximity Prestige"), this);
    layoutNodeSizeProminence_PP_Act->setEnabled(true);
    layoutNodeSizeProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_Y));
    layoutNodeSizeProminence_PP_Act->setStatusTip(
        tr("Resize all nodes to be "
           "proportional to their Proximity Prestige."));
    layoutNodeSizeProminence_PP_Act->setWhatsThis(
        tr("Proximity Prestige (PP) Node Size Layout\n\n"
           "Changes the size of all nodes to be "
           "proportional to their Proximity Prestige score. "
           "Nodes having higher PP will appear bigger."));
    connect(layoutNodeSizeProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeSizeByProminenceIndex()));

    layoutNodeColorProminence_DC_Act = new QAction(tr("Degree Centrality"), this);
    layoutNodeColorProminence_DC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_1));
    layoutNodeColorProminence_DC_Act
        ->setStatusTip(
            tr("Change the color of all nodes to "
               "reflect their Degree Centrality."));
    layoutNodeColorProminence_DC_Act->setWhatsThis(
        tr("Degree Centrality (DC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their DC (inDegree) score. \n\n"
           "Nodes having higher DC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_DC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_CC_Act = new QAction(tr("Closeness Centrality"), this);
    layoutNodeColorProminence_CC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_2));
    layoutNodeColorProminence_CC_Act
        ->setStatusTip(
            tr("Change the color of all nodes to "
               "reflect their Closeness Centrality."));
    layoutNodeColorProminence_CC_Act->setWhatsThis(
        tr("Closeness Centrality (CC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their CC score. "
           "Nodes of higher CC will have warmer color (i.e. red).\n\n"
           "This layout can be computed only for connected graphs. "));
    connect(layoutNodeColorProminence_CC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_IRCC_Act = new QAction(
        tr("Influence Range Closeness Centrality"), this);
    layoutNodeColorProminence_IRCC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_3));
    layoutNodeColorProminence_IRCC_Act
        ->setStatusTip(
            tr("Change the color of all nodes to proportional "
               "to their Influence Range Closeness Centrality."));
    layoutNodeColorProminence_IRCC_Act->setWhatsThis(
        tr("Influence Range Closeness Centrality (IRCC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their IRCC score. "
           "Nodes having higher IRCC will have warmer color (i.e. red).\n\n"
           "This layout can be computed for not connected graphs. "));
    connect(layoutNodeColorProminence_IRCC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_BC_Act = new QAction(tr("Betweenness Centrality"), this);
    layoutNodeColorProminence_BC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_4));
    layoutNodeColorProminence_BC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Betweenness Centrality."));
    layoutNodeColorProminence_BC_Act->setWhatsThis(
        tr("Betweenness Centrality (BC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Betweenness Centrality score. "
           "Nodes having higher BC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_BC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_SC_Act = new QAction(tr("Stress Centrality"), this);
    layoutNodeColorProminence_SC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_5));
    layoutNodeColorProminence_SC_Act->setStatusTip(
        tr("Change the color of all nodes to  "
           "reflect their Stress Centrality."));
    layoutNodeColorProminence_SC_Act->setWhatsThis(
        tr("Stress Centrality (SC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Stress Centrality score. "
           "Nodes having higher SC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_SC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EC_Act = new QAction(tr("Eccentricity Centrality"), this);
    layoutNodeColorProminence_EC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_6));
    layoutNodeColorProminence_EC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Eccentricity Centrality (aka Harary Graph Centrality)."));
    layoutNodeColorProminence_EC_Act->setWhatsThis(
        tr("Eccentricity Centrality (EC) NodeColors Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Eccentricity Centrality (aka Harary Graph Centrality) score. "
           "Nodes having higher EC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_EC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_PC_Act = new QAction(tr("Power Centrality"), this);
    layoutNodeColorProminence_PC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_7));
    layoutNodeColorProminence_PC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Power Centrality."));
    layoutNodeColorProminence_PC_Act->setWhatsThis(
        tr("Power Centrality (PC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Power Centrality score. "
           "Nodes having higher PC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_PC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_IC_Act = new QAction(tr("Information Centrality"), this);
    layoutNodeColorProminence_IC_Act->setEnabled(true);
    layoutNodeColorProminence_IC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_8));
    layoutNodeColorProminence_IC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Information Centrality."));
    layoutNodeColorProminence_IC_Act->setWhatsThis(
        tr("Information Centrality (IC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Information Centrality score. "
           "Nodes having higher IC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_IC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_EVC_Act = new QAction(tr("Eigenvector Centrality"), this);
    layoutNodeColorProminence_EVC_Act->setEnabled(true);
    layoutNodeColorProminence_EVC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_9));
    layoutNodeColorProminence_EVC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Eigenvector Centrality."));
    layoutNodeColorProminence_EVC_Act->setWhatsThis(
        tr("Eigenvector Centrality (EVC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Eigenvector Centrality score. "
           "Nodes having higher EVC will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_EVC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_KATZ_Act = new QAction(tr("Katz Centrality"), this);
    layoutNodeColorProminence_KATZ_Act->setEnabled(true);
    layoutNodeColorProminence_KATZ_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Katz Centrality."));
    layoutNodeColorProminence_KATZ_Act->setWhatsThis(
        tr("Katz Centrality (KC) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Katz Centrality score. "
           "Nodes having higher KC will have warmer color (i.e. red). "
           "Requires Katz Centrality to have been computed first "
           "(Analyze > Centrality > Katz Centrality)."));
    connect(layoutNodeColorProminence_KATZ_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_DP_Act = new QAction(tr("Degree Prestige"), this);
    layoutNodeColorProminence_DP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_I));
    layoutNodeColorProminence_DP_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Degree Prestige."));
    layoutNodeColorProminence_DP_Act->setWhatsThis(
        tr("Degree Prestige (DP) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their Degree Prestige score. "
           "Nodes having higher DP will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_DP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_PRP_Act = new QAction(tr("PageRank Prestige"), this);
    layoutNodeColorProminence_PRP_Act->setEnabled(true);
    layoutNodeColorProminence_PRP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_K));
    layoutNodeColorProminence_PRP_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their PageRank Prestige."));
    layoutNodeColorProminence_PRP_Act->setWhatsThis(
        tr("PageRank Prestige (PRP) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their PageRank Prestige score. "
           "Nodes having higher PRP will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_PRP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_PP_Act = new QAction(tr("Proximity Prestige"), this);
    layoutNodeColorProminence_PP_Act->setEnabled(true);
    layoutNodeColorProminence_PP_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_Y));
    layoutNodeColorProminence_PP_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their Proximity Prestige."));
    layoutNodeColorProminence_PP_Act->setWhatsThis(
        tr("Proximity Prestige (PP) Node Color Layout\n\n"
           "Changes the color of all nodes to "
           "reflect their PageRank Prestige score. "
           "Nodes of higher PP will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_PP_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorProminence_CLC_Act = new QAction(tr("Clustering Coefficient"), this);
    layoutNodeColorProminence_CLC_Act->setEnabled(true);
    layoutNodeColorProminence_CLC_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_G));
    layoutNodeColorProminence_CLC_Act->setStatusTip(
        tr("Change the color of all nodes to "
           "reflect their local Clustering Coefficient."));
    layoutNodeColorProminence_CLC_Act->setWhatsThis(
        tr("Clustering Coefficient Node Color Layout\n\n"
           "Changes the color of all nodes to reflect their local "
           "Clustering Coefficient (Watts-Strogatz). "
           "Nodes with higher clustering will have warmer color (i.e. red)."));
    connect(layoutNodeColorProminence_CLC_Act, SIGNAL(triggered()),
            this, SLOT(slotLayoutNodeColorByProminenceIndex()));

    layoutNodeColorByComponentAct = new QAction(tr("Node Color by Connected Component"), this);
    layoutNodeColorByComponentAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_C, Qt::CTRL | Qt::Key_0));
    layoutNodeColorByComponentAct->setStatusTip(
        tr("Color nodes by their weakly connected component."));
    layoutNodeColorByComponentAct->setWhatsThis(
        tr("Node Color by Connected Component\n\n"
           "Assigns a distinct color to each weakly connected component. "
           "All nodes in the same component share the same color, making "
           "isolated sub-networks immediately visible."));
    connect(layoutNodeColorByComponentAct, &QAction::triggered,
            this, &MainWindow::slotLayoutNodeColorByComponent);

    layoutFDP_Eades_Act = new QAction(tr("Spring Embedder (Eades)"), this);
    layoutFDP_Eades_Act->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_E));
    layoutFDP_Eades_Act->setStatusTip(
        tr("Layout Eades Spring-Gravitational model."));
    layoutFDP_Eades_Act->setWhatsThis(
        tr("Spring Embedder Layout\n\n "
           "The Spring Embedder model (Eades, 1984), part of the "
           "Force Directed Placement (FDP) family, embeds a mechanical "
           "system in the graph by replacing nodes with rings and edges "
           "with springs. \n"
           "In our implementation, nodes are replaced by physical bodies "
           "(i.e. electrons) which exert repelling forces to each other, "
           "while edges are replaced by springs which exert attractive "
           "forces to the adjacent nodes. "
           "The nodes are placed in some initial layout and let go "
           "so that the spring forces move the system to a minimal energy state. "
           "The algorithm continues until the system retains an equilibrium state "
           "in which all forces cancel each other. "));
    connect(layoutFDP_Eades_Act, SIGNAL(triggered(bool)), this, SLOT(slotLayoutSpringEmbedder()));

    layoutFDP_FR_Act = new QAction(tr("Fruchterman-Reingold"), this);
    layoutFDP_FR_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_F));
    layoutFDP_FR_Act->setStatusTip(
        tr("Repelling forces between all nodes, and attracting forces between adjacent nodes."));
    layoutFDP_FR_Act->setWhatsThis(
        tr("Fruchterman-Reingold Layout\n\n "
           "Embeds a layout all nodes according to a model in which	repelling "
           "forces are used between every pair of nodes, while attracting "
           "forces are used only between adjacent nodes. "
           "The algorithm continues until the system retains its equilibrium "
           "state where all forces cancel each other."));
    connect(layoutFDP_FR_Act, SIGNAL(triggered()), this, SLOT(slotLayoutFruchterman()));

    layoutFDP_KamadaKawai_Act = new QAction(tr("Kamada-Kawai"), this);
    layoutFDP_KamadaKawai_Act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L, Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_K));
    layoutFDP_KamadaKawai_Act->setStatusTip(
        tr("Embeds the Kamada-Kawai FDP layout model, the best variant of the Spring Embedder family of models."));
    layoutFDP_KamadaKawai_Act->setWhatsThis(
        tr(
            "<p><em>Kamada-Kawai</em></p>"
            "<p>The best variant of the Spring Embedder family of models. "
            "<p>In this the graph is considered to be a dynamic system where "
            "every edge is between two actors is a 'spring' of a desirable "
            "length, which corresponds to their graph theoretic distance. </p>"
            "<p>In this way, the optimal layout of the graph \n"
            "is the state with the minimum imbalance. The degree of "
            "imbalance is formulated as the total spring energy: "
            "the square summation of the differences between desirable "
            "distances and real ones for all pairs of vertices.</p>"

            ));
    connect(layoutFDP_KamadaKawai_Act, SIGNAL(triggered()), this, SLOT(slotLayoutKamadaKawai()));

    layoutGuidesAct = new QAction(QIcon(":/images/gridlines.png"), tr("Layout GuideLines"), this);
    layoutGuidesAct->setStatusTip(tr("Toggles layout guidelines on or off."));
    layoutGuidesAct->setWhatsThis(tr("Layout Guidelines\n\n"
                                     "Layout Guidelines are circular or horizontal lines \n"
                                     "usually created when embedding prominence-based \n"
                                     "visualization models on the network.\n"
                                     "Disable this checkbox to hide guidelines"));
    layoutGuidesAct->setCheckable(true);
    layoutGuidesAct->setChecked(true);

    /**
    Analysis menu actions
    */

    analyzeMatrixAdjInvertAct = new QAction(
        QIcon(":/images/invertmatrix.png"), tr("Invert Adjacency Matrix"), this);
    analyzeMatrixAdjInvertAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_I));
    analyzeMatrixAdjInvertAct->setStatusTip(tr("Invert the adjacency matrix, if possible"));
    analyzeMatrixAdjInvertAct->setWhatsThis(tr("Invert  Adjacency Matrix \n\n"
                                               "Inverts the adjacency matrix using linear algebra methods."));
    connect(analyzeMatrixAdjInvertAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyInverse()));

    analyzeMatrixAdjTransposeAct = new QAction(
        QIcon(":/images/transposematrix.png"), tr("Transpose Adjacency Matrix"), this);
    analyzeMatrixAdjTransposeAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_T));
    analyzeMatrixAdjTransposeAct->setStatusTip(tr("View the transpose of adjacency matrix"));
    analyzeMatrixAdjTransposeAct->setWhatsThis(tr("Transpose Adjacency Matrix \n\n"
                                                  "Computes and displays the adjacency matrix tranpose."));
    connect(analyzeMatrixAdjTransposeAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyTranspose()));

    analyzeMatrixAdjCocitationAct = new QAction(
        QIcon(":/images/cocitation.png"), tr("Cocitation Matrix"), this);
    analyzeMatrixAdjCocitationAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_C));
    analyzeMatrixAdjCocitationAct->setStatusTip(tr("Compute the Cocitation matrix of this network."));
    analyzeMatrixAdjCocitationAct->setWhatsThis(tr("Cocitation Matrix \n\n "
                                                   "Computes and displays the cocitation matrix of the network. "
                                                   "The Cocitation matrix, C=A*A^T, is a NxN matrix where "
                                                   "each element (i,j) is the number of actors that have "
                                                   "outbound ties/links to both actors i and j. "));
    connect(analyzeMatrixAdjCocitationAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixAdjacencyCocitation()));

    analyzeMatrixDegreeAct = new QAction(
        QIcon(":/images/degreematrix.png"), tr("Degree Matrix"), this);
    analyzeMatrixDegreeAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_D));
    analyzeMatrixDegreeAct->setStatusTip(tr("Compute the Degree matrix of the network"));
    analyzeMatrixDegreeAct->setWhatsThis(tr("Degree Matrix "
                                            "\n\n Compute the Degree matrix of the network."));
    connect(analyzeMatrixDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixDegree()));

    analyzeMatrixLaplacianAct = new QAction(
        QIcon(":/images/laplacian.png"), tr("Laplacian Matrix"), this);
    analyzeMatrixLaplacianAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_L));
    analyzeMatrixLaplacianAct->setStatusTip(tr("Compute the Laplacian matrix of the network"));
    analyzeMatrixLaplacianAct->setWhatsThis(tr("Laplacian Matrix \n\n"
                                               "Compute the Laplacian matrix of the network."));
    connect(analyzeMatrixLaplacianAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixLaplacian()));

    analyzeGraphReciprocityAct = new QAction(
        QIcon(":/images/symmetry-edge.png"), tr("Reciprocity"), this);
    analyzeGraphReciprocityAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_R));
    analyzeGraphReciprocityAct->setStatusTip(tr("Compute the arc and dyad reciprocity of the network."));
    analyzeGraphReciprocityAct->setWhatsThis(
        tr("Arc and Dyad Reciprocity\n\n"
           "The arc reciprocity of a network/graph is the fraction of "
           "reciprocated ties over all present ties of the graph. \n"
           "The dyad reciprocity of a network/graph is the fraction of "
           "actor pairs that have reciprocated ties over all connected "
           "pairs of actors. \n"
           "In a directed network, the arc reciprocity measures the proportion "
           "of directed edges that are bidirectional. If the reciprocity is 1, \n"
           "then the adjacency matrix is structurally symmetric. \n"
           "Likewise, in a directed network, the dyad reciprocity measures "
           "the proportion of connected actor dyads that have bidirectional ties "
           "between them. \n"
           "In an undirected graph, all edges are reciprocal. Thus the "
           "reciprocity of the graph is always 1. \n"
           "Reciprocity can be computed on undirected, directed, and weighted graphs."));
    connect(analyzeGraphReciprocityAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeReciprocity()));

    analyzeGraphSymmetryAct = new QAction(
        QIcon(":/images/symmetry_48px.svg"), tr("Symmetry Test"), this);
    analyzeGraphSymmetryAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_S));
    analyzeGraphSymmetryAct->setStatusTip(tr("Check whether the network is symmetric or not"));
    analyzeGraphSymmetryAct->setWhatsThis(
        tr("Symmetry\n\n"
           "Checks whether the network is symmetric or not. \n"
           "A network is symmetric when all edges are reciprocal, or, "
           "in mathematical language, when the adjacency matrix is "
           "symmetric."));
    connect(analyzeGraphSymmetryAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeSymmetryCheck()));

    analyzeGraphDistanceAct = new QAction(
        QIcon(":/images/distance.png"), tr("Geodesic Distance between 2 nodes"), this);
    analyzeGraphDistanceAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_G));
    analyzeGraphDistanceAct->setStatusTip(
        tr("Compute the length of the shortest path (geodesic distance) between 2 nodes."));
    analyzeGraphDistanceAct->setWhatsThis(
        tr("Distance\n\n"
           "Computes the geodesic distance between two nodes."
           "In graph theory, the geodesic distance of two "
           "nodes is the length (number of edges) of the shortest path "
           "between them."));
    connect(analyzeGraphDistanceAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDistance()));

    analyzeMatrixDistancesGeodesicAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesic Distances Matrix"), this);
    analyzeMatrixDistancesGeodesicAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_M));
    analyzeMatrixDistancesGeodesicAct->setStatusTip(
        tr("Compute the matrix of geodesic distances between all pair of nodes."));
    analyzeMatrixDistancesGeodesicAct->setWhatsThis(
        tr("Distances Matrix\n\n"
           "Computes the matrix of distances between all "
           "pairs of actors/nodes in the social network."
           "A distances matrix is a n x n matrix, in which the "
           "(i,j) element is the distance from node i to node j"
           "The distance of two nodes is the length of the shortest path between them."));
    connect(analyzeMatrixDistancesGeodesicAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeMatrixDistances()));

    analyzeMatrixGeodesicsAct = new QAction(QIcon(":/images/dm.png"), tr("Geodesics Matrix"), this);
    analyzeMatrixGeodesicsAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_P));
    analyzeMatrixGeodesicsAct->setStatusTip(tr("Compute the number of shortest paths (geodesics) between each pair of nodes "));
    analyzeMatrixGeodesicsAct->setWhatsThis(
        tr(
            "Geodesics Matrix\n\n"
            "Displays a n x n matrix, where the (i,j) element "
            "is the number of shortest paths (geodesics) between "
            "node i and node j. "));
    connect(analyzeMatrixGeodesicsAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeMatrixGeodesics()));

    analyzeGraphDiameterAct = new QAction(QIcon(":/images/diameter_48px.svg"), tr("Graph Diameter"), this);
    analyzeGraphDiameterAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_D));
    analyzeGraphDiameterAct->setStatusTip(tr("Compute the diameter of the network, "
                                             "the maximum geodesic distance between any actors."));
    analyzeGraphDiameterAct->setWhatsThis(tr("Diameter\n\n "
                                             "The diameter of a network is the maximum geodesic distance "
                                             "(maximum shortest path length) between any two nodes of the network."));
    connect(analyzeGraphDiameterAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeDiameter()));

    averGraphDistanceAct = new QAction(QIcon(":/images/avdistance.png"), tr("Average Distance"), this);
    averGraphDistanceAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_A));
    averGraphDistanceAct->setStatusTip(tr("Compute the average graph distance for all possible pairs of nodes."));
    averGraphDistanceAct->setWhatsThis(
        tr("Average Graph Distance\n\n "
           "This is the average length of shortest paths (geodesics) "
           "for all possible pairs of nodes. "
           "It is a measure of the efficiency or compactness of the network."));
    connect(averGraphDistanceAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeDistanceAverage()));

    analyzeGeodesicDistributionAct = new QAction(tr("Geodesic Distance Distribution"), this);
    analyzeGeodesicDistributionAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_I));
    analyzeGeodesicDistributionAct->setStatusTip(
        tr("Show the distribution of geodesic distances across all pairs of nodes."));
    analyzeGeodesicDistributionAct->setWhatsThis(
        tr("Geodesic Distance Distribution\n\n"
           "Counts how many ordered pairs of nodes are separated by each "
           "geodesic distance d (d = 1, 2, …, diameter).  "
           "Useful for understanding the shape of the network's distance structure — "
           "e.g. whether most pairs are at distance 2 (\"small world\") "
           "or spread across many distances."));
    connect(analyzeGeodesicDistributionAct, &QAction::triggered,
            this, &MainWindow::slotAnalyzeGeodesicDistribution);

    analyzeGraphEccentricityAct = new QAction(QIcon(":/images/eccentricity.png"), tr("Eccentricity"), this);
    analyzeGraphEccentricityAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_E));
    analyzeGraphEccentricityAct->setStatusTip(tr("Compute the Eccentricity of each actor and group Eccentricity"));
    analyzeGraphEccentricityAct->setWhatsThis(tr("Eccentricity\n\n"
                                                 "The eccentricity of each node i in a network "
                                                 "or graph is the largest geodesic distance "
                                                 "between node i and any other node j. "
                                                 "Therefore, it reflects how far, at most, "
                                                 "is each node from every other node. \n"
                                                 "The maximum eccentricity is the graph diameter "
                                                 "while the minimum is the graph radius.\n"
                                                 "This index can be calculated in both graphs "
                                                 "and digraphs but is usually best suited "
                                                 "for undirected graphs. \n"
                                                 "It can also be calculated in weighted graphs "
                                                 "although the weight of each edge (v,u) in E is "
                                                 "always considered to be 1."));
    connect(analyzeGraphEccentricityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeEccentricity()));

    analyzeGraphConnectednessAct = new QAction(QIcon(":/images/distance.png"), tr("Connectedness"), this);
    analyzeGraphConnectednessAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_C));
    analyzeGraphConnectednessAct->setStatusTip(tr("Check whether the network is a connected "
                                                  "graph, a connected digraph or "
                                                  "a disconnected graph/digraph..."));
    analyzeGraphConnectednessAct->setWhatsThis(tr("Connectedness\n\n In graph theory, a "
                                                  "graph is <b>connected</b> if there is a "
                                                  "path between every pair of nodes. \n"
                                                  "A digraph is <b>strongly connected</b> "
                                                  "if there the a path from i to j and "
                                                  "from j to i for all pairs (i,j).\n"
                                                  "A digraph is weakly connected if at least "
                                                  "a pair of nodes are joined by a semipath.\n"
                                                  "A digraph or a graph is disconnected if "
                                                  "at least one node is isolate."));
    connect(analyzeGraphConnectednessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeConnectedness()));

    analyzeNodeConnectivityAct = new QAction(QIcon(":/images/distance.png"), tr("Node Connectivity"), this);
    analyzeNodeConnectivityAct->setStatusTip(
        tr("Compute the minimum number of nodes that must be removed to disconnect two nodes."));
    analyzeNodeConnectivityAct->setWhatsThis(
        tr("Node Connectivity\n\n"
           "The local vertex connectivity of two nodes s and t is the minimum number "
           "of other nodes that must be removed to disconnect t from s.\n"
           "If s and t are directly connected by an edge, no such removal exists, "
           "since the edge itself always keeps them connected.\n"
           "For directed networks, asks whether to respect edge direction (strong) "
           "or ignore it (weak)."));
    connect(analyzeNodeConnectivityAct, &QAction::triggered, this, &MainWindow::slotAnalyzeNodeConnectivity);

    analyzeConnectivityAct = new QAction(QIcon(":/images/avdistance.png"), tr("Graph Connectivity"), this);
    analyzeConnectivityAct->setStatusTip(
        tr("Compute the network's overall vertex connectivity."));
    analyzeConnectivityAct->setWhatsThis(
        tr("Graph Connectivity\n\n"
           "The vertex connectivity of the whole network is the minimum, over every pair "
           "of nodes, of their local vertex connectivity - i.e. the fewest nodes that "
           "would need to be removed to disconnect the network at its weakest point.\n"
           "For directed networks, asks whether to respect edge direction (strong) "
           "or ignore it (weak)."));
    connect(analyzeConnectivityAct, &QAction::triggered, this, &MainWindow::slotAnalyzeConnectivity);

    analyzeGraphWalksAct = new QAction(QIcon(":/images/walk.png"), tr("Walks of a given length"), this);
    analyzeGraphWalksAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_W));
    analyzeGraphWalksAct->setStatusTip(tr("Compute the number of walks of a given length between any nodes."));
    analyzeGraphWalksAct->setWhatsThis(tr("Walks of a given length\n\n"
                                          "A walk is a sequence of alternating vertices and edges "
                                          "such as v<sub>0</sub>e<sub>1</sub>, v<sub>1</sub>e<sub>2</sub>, "
                                          "v<sub>2</sub>e<sub>3</sub>, …, e<sub>k</sub>v<sub>k</sub>, "
                                          "where each edge, e<sub>i</sub> is defined as "
                                          "e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                                          "This function counts the number of walks of a given "
                                          "length between each pair of nodes, by studying the powers of the sociomatrix.\n"));
    connect(analyzeGraphWalksAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksLength()));

    analyzeGraphWalksTotalAct = new QAction(QIcon(":/images/walk.png"), tr("Total Walks"), this);
    analyzeGraphWalksTotalAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_T));
    analyzeGraphWalksTotalAct->setStatusTip(tr("Calculate the total number of walks of every possible length between all nodes"));
    analyzeGraphWalksTotalAct->setWhatsThis(tr("Total Walks\n\n"
                                               "A walk is a sequence of alternating vertices "
                                               "and edges such as v<sub>0</sub>e<sub>1</sub>, "
                                               "v<sub>1</sub>e<sub>2</sub>, v<sub>2</sub>e<sub>3</sub>, …, "
                                               "e<sub>k</sub>v<sub>k</sub>, where each edge, e<sub>i</sub> "
                                               "is defined as e<sub>i</sub> = {v<sub>i-1</sub>, v<sub>i</sub>}. "
                                               "This function counts the number of walks of any length "
                                               "between each pair of nodes, by studying the powers of the sociomatrix. \n"));
    connect(analyzeGraphWalksTotalAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeWalksTotal()));

    analyzeMatrixReachabilityAct = new QAction(QIcon(":/images/walk.png"), tr("Reachability Matrix"), this);
    analyzeMatrixReachabilityAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_R));
    analyzeMatrixReachabilityAct->setStatusTip(tr("Compute the Reachability Matrix of the network."));
    analyzeMatrixReachabilityAct->setWhatsThis(tr("Reachability Matrix\n\n"
                                                  "Calculates the reachability matrix X<sup>R</sup> of "
                                                  "the graph where the {i,j} element is 1 if "
                                                  "the vertices i and j are reachable. \n\n"
                                                  "Actually, this just checks whether the corresponding element "
                                                  "of Distances matrix is not zero.\n"));
    connect(analyzeMatrixReachabilityAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeReachabilityMatrix()));

    clusteringCoefAct = new QAction(QIcon(":/images/clucof.png"), tr("Local and Network Clustering Coefficient"), this);
    clusteringCoefAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_L));
    clusteringCoefAct->setStatusTip(tr("Compute the Watts & Strogatz Clustering Coefficient for every actor and the network average."));
    clusteringCoefAct->setWhatsThis(tr("Local and Network Clustering Coefficient\n\n"
                                       "The local Clustering Coefficient  (Watts & Strogatz, 1998) "
                                       "of an actor quantifies how close "
                                       "the actor and her neighbors are to being a clique and "
                                       "can be used as an indication of network transitivity. \n"));
    connect(clusteringCoefAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeClusteringCoefficient()));

    analyzeCommunitiesCliquesAct = new QAction(QIcon(":/images/clique.png"), tr("Clique Census"), this);
    analyzeCommunitiesCliquesAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_U, Qt::CTRL | Qt::Key_C));
    analyzeCommunitiesCliquesAct->setStatusTip(tr("Compute the clique census: find all maximal connected subgraphs."));
    analyzeCommunitiesCliquesAct->setWhatsThis(tr("Clique Census\n\n"
                                                  "Produces the census of network cliques (maximal connected subgraphs), "
                                                  "along with disaggregation by actor and co-membership information. "));
    connect(analyzeCommunitiesCliquesAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesCliqueCensus()));

    analyzeCommunitiesTriadCensusAct = new QAction(QIcon(":/images/triad.png"), tr("Triad Census (M-A-N labeling)"), this);
    analyzeCommunitiesTriadCensusAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_U, Qt::CTRL | Qt::Key_T));
    analyzeCommunitiesTriadCensusAct->setStatusTip(tr("Calculate the triad census for all actors."));
    analyzeCommunitiesTriadCensusAct->setWhatsThis(tr("Triad Census\n\n"
                                                      "A triad census counts all the different kinds of observed triads "
                                                      "within a network and codes them according to their number of mutual, "
                                                      "asymmetric and non-existent dyads using the M-A-N labeling scheme. \n"));
    connect(analyzeCommunitiesTriadCensusAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCommunitiesTriadCensus()));

    analyzeStrEquivalencePearsonAct = new QAction(QIcon(":/images/similarity.png"),
                                                  tr("Pearson correlation coefficients"), this);
    analyzeStrEquivalencePearsonAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_P));
    analyzeStrEquivalencePearsonAct->setStatusTip(
        tr("Compute Pearson Correlation Coefficients between pairs of actors. "
           "Most useful with valued/weighted ties (non-binary). "));
    analyzeStrEquivalencePearsonAct->setWhatsThis(
        tr("Pearson correlation coefficients\n\n"
           "Computes a correlation matrix, where the elements are the "
           "Pearson correlation coefficients between pairs of actors "
           "in terms of their tie profiles or distances (in, out or both). \n\n"
           "The Pearson product-moment correlation coefficient (PPMCC or PCC or Pearson's r)"
           "is a measure of the linear dependence/association between two variables X and Y. \n\n"
           "This correlation measure of similarity is particularly useful "
           "when ties are valued/weighted denoting strength, cost or probability.\n\n"
           "Note that in very sparse networks (very low density), measures such as"
           "\"exact matches\", \"correlation\" and \"distance\" "
           "will show little variation among the actors, causing "
           "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalencePearsonAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalencePearsonDialog()));

    analyzeStrEquivalenceMatchesAct = new QAction(QIcon(":/images/similarity.png"),
                                                  tr("Similarity by measure (Exact, Jaccard, Hamming, Cosine, Euclidean)"), this);
    analyzeStrEquivalenceMatchesAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_E));
    analyzeStrEquivalenceMatchesAct->setStatusTip(tr("Compute a pair-wise actor similarity "
                                                     "matrix based on a measure of their ties (or distances) \"matches\" ."));
    analyzeStrEquivalenceMatchesAct->setWhatsThis(
        tr("Actor Similarity by measure\n\n"
           "Computes a pair-wise actor similarity matrix, where each element (i,j) is "
           "the ratio of tie (or distance) matches of actors i and j to all other actors. \n\n"
           "SocNetV supports the following matching measures: "
           "Simple Matching (Exact Matches)"
           "Jaccard Index (Positive Matches or Co-citation)"
           "Hamming distance"
           "Cosine similarity"
           "Euclidean distance"
           "For instance, if you select Exact Matches, a matrix element (i,j) = 0.5, "
           "means that actors i and j have the same ties present or absent "
           "to other actors 50% of the time. \n\n"
           "These measures of similarity are particularly useful "
           "when ties are binary (not valued).\n\n"
           "Note that in very sparse networks (very low density), measures such as"
           "\"exact matches\", \"correlation\" and \"distance\" "
           "will show little variation among the actors, causing "
           "difficulty in classifying the actors in structural equivalence classes."));
    connect(analyzeStrEquivalenceMatchesAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceSimilarityMeasureDialog()));

    analyzeStrEquivalenceTieProfileDissimilaritiesAct = new QAction(QIcon(":/images/dm.png"),
                                                                    tr("Tie Profile Dissimilarities/Distances"), this);
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_T));
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->setStatusTip(
        tr("Compute tie profile dissimilarities/distances "
           "(Euclidean, Manhattan, Jaccard, Hamming) between all pair of nodes."));
    analyzeStrEquivalenceTieProfileDissimilaritiesAct->setWhatsThis(
        tr("Tie Profile Dissimilarities/Distances\n\n"
           "Computes a matrix of tie profile distances/dissimilarities "
           "between all pairs of actors/nodes in the social network "
           "using an ordinary metric such as Euclidean distance, "
           "Manhattan distance, Jaccard distance or Hamming distance)."
           "The resulted distance matrix is a n x n matrix, in which the "
           "(i,j) element is the distance or dissimilarity between "
           "the tie profiles of node i and node j."));
    connect(analyzeStrEquivalenceTieProfileDissimilaritiesAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceDissimilaritiesDialog()));

    analyzeStrEquivalenceClusteringHierarchicalAct = new QAction(QIcon(":/images/hierarchical.png"),
                                                                 tr("Hierarchical clustering"), this);
    analyzeStrEquivalenceClusteringHierarchicalAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::Key_H));

    analyzeStrEquivalenceClusteringHierarchicalAct->setStatusTip(
        tr("Perform agglomerative cluster analysis of the actors in the social network"));
    analyzeStrEquivalenceClusteringHierarchicalAct->setWhatsThis(
        tr("Hierarchical clustering\n\n"
           "Hierarchical clustering (or hierarchical cluster analysis, HCA) "
           "is a method of cluster analysis which builds a hierarchy "
           "of clusters, based on their elements dissimilarity. "
           "In SNA context these clusters usually consist of "
           "network actors. \n"

           "This method takes the social network distance matrix as input and uses "
           "the Agglomerative \"bottom up\" approach where each "
           "actor starts in its own cluster (Level 0). In each subsequent Level, "
           "as we move up the clustering hierarchy, a pair of clusters "
           "are merged into a larger cluster, until "
           "all actors end up in the same cluster. "

           "To decide which clusters should be combined at each level, a measure of "
           "dissimilarity between sets of observations is required. "
           "This measure consists of a metric for the distance between actors "
           "(i.e. manhattan distance) and a linkage criterion (i.e. single-linkage clustering). "
           "This linkage criterion (essentially a definition of distance between clusters), "
           "differentiates between the different HCA methods."

           "Note that the complexity of agglomerative clustering is O( n^2 log(n) ), "
           "therefore is too slow for large data sets."));
    connect(analyzeStrEquivalenceClusteringHierarchicalAct, SIGNAL(triggered()),
            this, SLOT(slotAnalyzeStrEquivalenceClusteringHierarchicalDialog()));

    cDegreeAct = new QAction(tr("Degree Centrality (DC)"), this);
    cDegreeAct->setShortcut(Qt::CTRL | Qt::Key_1);
    cDegreeAct
        ->setStatusTip(tr("Compute Degree Centrality indices for every actor and group Degree Centralization."));
    cDegreeAct
        ->setWhatsThis(
            tr("Degree Centrality (DC)\n\n"
               "For each node v, the DC index is the number of edges "
               "attached to it (in undirected graphs) or the total number "
               "of arcs (outLinks) starting from it (in digraphs).\n"
               "This is often considered a measure of actor activity. \n\n"
               "This index can be calculated in both graphs and digraphs "
               "but is usually best suited for undirected graphs. "
               "It can also be calculated in weighted graphs. "
               "In weighted relations, DC is the sum of weights of all "
               "edges/outLinks attached to v."));
    connect(cDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityDegree()));

    cClosenessAct = new QAction(tr("Closeness Centrality (CC)"), this);
    cClosenessAct->setShortcut(Qt::CTRL | Qt::Key_2);
    cClosenessAct
        ->setStatusTip(
            tr(
                "Compute Closeness Centrality indices for every actor and group Closeness Centralization."));
    cClosenessAct
        ->setWhatsThis(
            tr("Closeness Centrality (CC)\n\n"
               "For each node v, CC the inverse sum of "
               "the shortest distances between v and every other node. CC is "
               "interpreted as the ability to access information through the "
               "\"grapevine\" of network members. Nodes with high closeness "
               "centrality are those who can reach many other nodes in few steps. "
               "\n\nThis index can be calculated in both graphs and digraphs. "
               "It can also be calculated in weighted graphs although the weight of "
               "each edge (v,u) in E is always considered to be 1. "));
    connect(cClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityCloseness()));

    cInfluenceRangeClosenessAct = new QAction(tr("Influence Range Closeness Centrality (IRCC)"), this);
    cInfluenceRangeClosenessAct->setShortcut(Qt::CTRL | Qt::Key_3);
    cInfluenceRangeClosenessAct
        ->setStatusTip(
            tr("Compute Influence Range Closeness Centrality indices for every actor "
               "focusing on how proximate each one is"
               "to others in its influence range"));
    cInfluenceRangeClosenessAct
        ->setWhatsThis(
            tr("Influence Range Closeness Centrality (IRCC)\n\n"
               "For each node v, IRCC is the standardized inverse average distance "
               "between v and every reachable node.\n"
               "This improved CC index is optimized for graphs and directed graphs which "
               "are not strongly connected. Unlike the ordinary CC, which is the inverted "
               "sum of distances from node v to all others (thus undefined if a node is isolated "
               "or the digraph is not strongly connected), IRCC considers only "
               "distances from node v to nodes in its influence range J (nodes reachable from v). "
               "The IRCC formula used is the ratio of the fraction of nodes reachable by v "
               "(|J|/(n-1)) to the average distance of these nodes from v (sum(d(v,j))/|J|"));
    connect(cInfluenceRangeClosenessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityClosenessIR()));

    cBetweennessAct = new QAction(tr("Betweenness Centrality (BC)"), this);
    cBetweennessAct->setShortcut(Qt::CTRL | Qt::Key_4);
    cBetweennessAct->setWhatsThis(tr("Betweenness Centrality (BC)\n\n"
                                     "For each node v, BC is the ratio of all geodesics between pairs of nodes which run through v. "
                                     "It reflects how often an node lies on the geodesics between the other nodes of the network. "
                                     "It can be interpreted as a measure of control. "
                                     "A node which lies between many others is assumed to have a higher likelihood of being able "
                                     "to control information flow in the network. \n\n"
                                     "Note that betweenness centrality assumes that all geodesics "
                                     "have equal weight or are equally likely to be chosen for the flow of information "
                                     "between any two nodes. This is reasonable only on \"regular\" networks where all "
                                     "nodes have similar degrees. On networks with significant degree variance you might want "
                                     "to try informational centrality instead. \n\nThis index can be calculated in both graphs "
                                     "and digraphs but is usually best suited for undirected graphs. It can also be calculated"
                                     " in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    cBetweennessAct->setStatusTip(tr("Compute Betweenness Centrality indices and group Betweenness Centralization."));
    connect(cBetweennessAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityBetweenness()));

    cStressAct = new QAction(tr("Stress Centrality (SC)"), this);
    cStressAct->setShortcut(Qt::CTRL | Qt::Key_5);
    cStressAct->setStatusTip(tr("Compute Stress Centrality indices for every actor and group Stress Centralization."));
    cStressAct->setWhatsThis(tr("Stress Centrality (SC)\n\n"
                                "For each node v, SC is the total number of geodesics between all other nodes which run through v. "
                                "A node with high SC is considered 'stressed', since it is traversed by a high number of geodesics. "
                                "When one node falls on all other geodesics between all the remaining (N-1) nodes, "
                                "then we have a star graph with maximum Stress Centrality. \n\n"
                                "This index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. "
                                "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cStressAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityStress()));

    cEccentAct = new QAction(tr("Eccentricity Centrality (EC)"), this);
    cEccentAct->setShortcut(Qt::CTRL | Qt::Key_6);
    cEccentAct->setStatusTip(tr("Compute Eccentricity Centrality (aka Harary Graph Centrality) scores for each node."));
    cEccentAct->setWhatsThis(
        tr("Eccentricity Centrality (EC)\n\n "
           "This index is also known as Harary Graph Centrality. "
           "For each node i, "
           "the EC is the inverse of the maximum geodesic distance "
           "of that v to all other nodes in the network. \n"
           "Nodes with high EC have short distances to all other nodes "
           "This index can be calculated in both graphs and digraphs "
           "but is usually best suited for undirected graphs. "
           "It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1."));
    connect(cEccentAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEccentricity()));

    cPowerAct = new QAction(tr("Gil and Schmidt Power Centrality (PC)"), this);
    cPowerAct->setShortcut(Qt::CTRL | Qt::Key_7);
    cPowerAct->setStatusTip(tr("Compute Power Centrality indices (aka Gil-Schmidt Power Centrality) for every actor and group Power Centralization"));
    cPowerAct->setWhatsThis(tr("Power Centrality (PC)\n\n "
                               "For each node v, this index sums its degree (with weight 1), with the size of the 2nd-order neighbourhood (with weight 2), and in general, with the size of the kth order neighbourhood (with weight k). Thus, for each node in the network the most important other nodes are its immediate neighbours and then in decreasing importance the nodes of the 2nd-order neighbourhood, 3rd-order neighbourhood etc. For each node, the sum obtained is normalised by the total numbers of nodes in the same component minus 1. Power centrality has been devised by Gil-Schmidt. \n\nThis index can be calculated in both graphs and digraphs but is usually best suited for undirected graphs. It can also be calculated in weighted graphs although the weight of each edge (v,u) in E is always considered to be 1 (therefore not considered)."));
    connect(cPowerAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityPower()));

    cInformationAct = new QAction(tr("Information Centrality (IC)"), this);
    cInformationAct->setShortcut(Qt::CTRL | Qt::Key_8);
    cInformationAct->setEnabled(true);
    cInformationAct->setStatusTip(tr("Compute Information Centrality indices and group Information Centralization"));
    cInformationAct->setWhatsThis(
        tr("Information Centrality (IC)\n\n"
           "Information centrality counts all paths between "
           "nodes weighted by strength of tie and distance. "
           "This centrality  measure developed by Stephenson and Zelen (1989) "
           "focuses on how information might flow through many different paths. \n\n"
           "This index should be calculated only for  graphs. \n\n"
           "Note: To compute this index, SocNetV drops all isolated nodes."));
    connect(cInformationAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityInformation()));

    cEigenvectorAct = new QAction(tr("Eigenvector Centrality (EVC)"), this);
    cEigenvectorAct->setShortcut(Qt::CTRL | Qt::Key_9);
    cEigenvectorAct->setEnabled(true);
    cEigenvectorAct->setStatusTip(tr("Compute Eigenvector Centrality indices and group Eigenvector Centralization"));
    cEigenvectorAct->setWhatsThis(
        tr("Eigenvector Centrality (EVC)\n\n"
           "Computes the Eigenvector centrality of each node in a social network "
           "which is defined as the ith element of the leading eigenvector "
           "of the adjacency matrix. The leading eigenvector is the "
           "eigenvector corresponding to the largest positive eigenvalue."
           "The Eigenvector Centrality, proposed by Bonacich (1989), is "
           "an extension of the simpler Degree Centrality because it gives "
           "each actor a score proportional to the scores of its neighbors. "
           "Thus, a node may be important, in terms of its EC, because it "
           "has lots of ties or it has fewer ties to important other nodes."));
    connect(cEigenvectorAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityEigenvector()));

    cKatzAct = new QAction(tr("Katz Centrality (KC)"), this);
    cKatzAct->setEnabled(true);
    cKatzAct->setStatusTip(tr("Compute Katz Centrality indices"));
    cKatzAct->setWhatsThis(
        tr("Katz Centrality (KC)\n\n"
           "Computes the Katz centrality of each node: credit for indirect "
           "connections, not just direct ones, discounted the further away "
           "they are by an attenuation factor alpha that you choose. "
           "Proposed by Katz (1953), it generalizes Degree Centrality by "
           "counting walks of every length, not just length 1."));
    connect(cKatzAct, SIGNAL(triggered()), this, SLOT(slotAnalyzeCentralityKatz()));

    cInDegreeAct = new QAction(tr("Degree Prestige (DP)"), this);
    cInDegreeAct->setStatusTip(tr("Compute Degree Prestige (InDegree) indices "));
    cInDegreeAct->setShortcut(Qt::CTRL | Qt::Key_I);
    cInDegreeAct->setWhatsThis(tr("InDegree (Degree Prestige)\n\n"
                                  "For each node k, this the number of arcs ending at k. "
                                  "Nodes with higher in-degree are considered more prominent among others. "
                                  "In directed graphs, this index measures the prestige of each node/actor. "
                                  "Thus it is called Degree Prestige. "
                                  "Nodes who are prestigious tend to receive many nominations or choices (in-links). "
                                  "The largest the index is, the more prestigious is the node. \n\n"
                                  "This index can be calculated only for digraphs. "
                                  "In weighted relations, DP is the sum of weights of all arcs/inLinks ending at node v."));
    connect(cInDegreeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeDegree()));

    cPageRankAct = new QAction(tr("PageRank Prestige (PRP)"), this);
    cPageRankAct->setShortcut(Qt::CTRL | Qt::Key_K);
    cPageRankAct->setEnabled(true);
    cPageRankAct->setStatusTip(tr("Compute PageRank Prestige indices for every actor"));
    cPageRankAct->setWhatsThis(tr("PageRank Prestige\n\n"
                                  "An importance ranking for each node based on the link structure of the network. "
                                  "PageRank, developed by Page and Brin (1997), focuses on how nodes are "
                                  "connected to each other, treating each edge from a node as a citation/backlink/vote to another. "
                                  "In essence, for each node PageRank counts all backlinks to it, "
                                  "but it does so by not counting all edges equally while it "
                                  "normalizes each edge from a node by the total number of edges from it. "
                                  "PageRank is calculated iteratively and it corresponds to the principal "
                                  "eigenvector of the normalized link matrix. \n\n"
                                  "This index can be calculated in both graphs and digraphs but is "
                                  "usually best suited for directed graphs since it is a prestige measure. "
                                  "It can also be calculated in weighted graphs. "
                                  "In weighted relations, each backlink to a node v from another node u is "
                                  "considered to have weight=1 but it is normalized by the sum of "
                                  "outLinks weights (outDegree) of u. Therefore, nodes with high outLink "
                                  "weights give smaller percentage of their PR to node v."));
    connect(cPageRankAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigePageRank()));

    cProximityPrestigeAct = new QAction(tr("Proximity Prestige (PP)"), this);
    cProximityPrestigeAct->setShortcut(Qt::CTRL | Qt::Key_Y);
    cProximityPrestigeAct->setEnabled(true);
    cProximityPrestigeAct->setStatusTip(tr("Calculate and display Proximity Prestige (digraphs only)"));
    cProximityPrestigeAct
        ->setWhatsThis(
            tr("Proximity Prestige (PP) \n\n"
               "This index measures how proximate a node v is to the nodes "
               "in its influence domain I (the influence domain I of a node "
               "is the number of other nodes that can reach it).\n\n"
               "In PP calculation, proximity is based on distances to rather "
               "than distances from node v. \n"
               "To put it simply, in PP what matters is how close are all "
               "the other nodes to node v. \n\n"
               "The algorithm takes the average distance to node v of all "
               "nodes in its influence domain, standardizes it by "
               "multiplying with (N-1)/I and takes its reciprocal. "
               "In essence, the formula SocNetV uses to calculate PP "
               "is the ratio of the fraction of nodes that can reach node v, "
               "to the average distance of that nodes to v: \n"
               "PP = (I/(N-1))/(sum{d(u,v)}/I) \n"
               "where the sum is over all nodes in I."));
    connect(cProximityPrestigeAct, SIGNAL(triggered()), this, SLOT(slotAnalyzePrestigeProximity()));

    /**
    Options menu actions
    */
    optionsNodeNumbersVisibilityAct = new QAction(tr("Display Node Numbers"), this);
    optionsNodeNumbersVisibilityAct->setStatusTip(
        tr("Toggle displaying of node numbers"));
    optionsNodeNumbersVisibilityAct->setWhatsThis(
        tr("Display Node Numbers\n\n"
           "Enables or disables displaying of node numbers."));
    optionsNodeNumbersVisibilityAct->setCheckable(true);
    optionsNodeNumbersVisibilityAct->setChecked(
        (appSettings["initNodeNumbersVisibility"] == "true") ? true : false);
    connect(optionsNodeNumbersVisibilityAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsNodeNumbersVisibility(bool)));

    optionsNodeNumbersInsideAct = new QAction(tr("Display Numbers Inside Nodes"), this);
    optionsNodeNumbersInsideAct->setStatusTip(
        tr("Toggle displaying of numbers inside nodes"));
    optionsNodeNumbersInsideAct->setWhatsThis(
        tr("Display Numbers Inside Nodes\n\n"
           "Enables or disables displaying node numbers inside nodes."));
    optionsNodeNumbersInsideAct->setCheckable(true);
    optionsNodeNumbersInsideAct->setChecked(
        (appSettings["initNodeNumbersInside"] == "true") ? true : false);
    connect(optionsNodeNumbersInsideAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsNodeNumbersInside(bool)));

    optionsNodeLabelsVisibilityAct = new QAction(tr("Display Node Labels"), this);
    optionsNodeLabelsVisibilityAct->setStatusTip(
        tr("Toggle displaying of node labels"));
    optionsNodeLabelsVisibilityAct->setWhatsThis(
        tr("Display Node Labels\n\n"
           "Enables or disables node labels."));
    optionsNodeLabelsVisibilityAct->setCheckable(true);
    optionsNodeLabelsVisibilityAct->setChecked(
        (appSettings["initNodeLabelsVisibility"] == "true") ? true : false);
    connect(optionsNodeLabelsVisibilityAct, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsNodeLabelsVisibility(bool)));

    optionsEdgesVisibilityAct = new QAction(tr("Display Edges"), this);
    optionsEdgesVisibilityAct->setStatusTip(tr("Toggle displaying edges"));
    optionsEdgesVisibilityAct->setWhatsThis(
        tr("Display Edges\n\n"
           "Enables or disables displaying of edges."));
    optionsEdgesVisibilityAct->setCheckable(true);
    optionsEdgesVisibilityAct->setChecked(
        (appSettings["initEdgesVisibility"] == "true") ? true : false);
    connect(optionsEdgesVisibilityAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesVisibility(bool)));

    optionsEdgeWeightNumbersAct = new QAction(tr("Display Edge Weights"), this);
    optionsEdgeWeightNumbersAct->setStatusTip(
        tr("Toggle displaying of numbers of edge weights"));
    optionsEdgeWeightNumbersAct->setWhatsThis(
        tr("Display Edge Weights\n\n"
           "Enables or disables displaying edge weight numbers."));
    optionsEdgeWeightNumbersAct->setCheckable(true);
    connect(optionsEdgeWeightNumbersAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightNumbersVisibility(bool)));

    optionsEdgeWeightConsiderAct = new QAction(tr("Consider Edge Weights in Calculations"), this);
    optionsEdgeWeightConsiderAct->setStatusTip(
        tr("Toggle considering edge weights during calculations "
           "(i.e. distances, centrality, etc) (this session only)"));
    optionsEdgeWeightConsiderAct->setWhatsThis(
        tr("Consider Edge Weights in Calculations\n\n"
           "Enables or disables considering edge weights during "
           "calculations (i.e. distances, centrality, etc).\n"
           "This setting will apply to this session only. \n"
           "To permanently change it, go to Settings."));
    optionsEdgeWeightConsiderAct->setCheckable(true);
    optionsEdgeWeightConsiderAct->setChecked(false);
    connect(optionsEdgeWeightConsiderAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeWeightsDuringComputation(bool)));

    optionsEdgeLabelsAct = new QAction(tr("Display Edge Labels"), this);
    optionsEdgeLabelsAct->setStatusTip(
        tr("Toggle displaying of edge labels, if any"));
    optionsEdgeLabelsAct->setWhatsThis(
        tr("Display Edge Labels\n\n"
           "Enables or disables displaying edge labels."));
    optionsEdgeLabelsAct->setCheckable(true);
    optionsEdgeLabelsAct->setChecked(
        (appSettings["initEdgeLabelsVisibility"] == "true") ? true : false);
    connect(optionsEdgeLabelsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeLabelsVisibility(bool)));

    optionsEdgeArrowsAct = new QAction(tr("Display Edge Arrows"), this);
    optionsEdgeArrowsAct->setStatusTip(
        tr("Toggle displaying directional arrows on edges"));
    optionsEdgeArrowsAct->setWhatsThis(
        tr("Display Edge Arrows\n\n"
           "Enables or disables displaying of arrows on edges.\n\n"
           "Useful if all links are reciprocal (undirected graph)."));
    optionsEdgeArrowsAct->setCheckable(true);
    optionsEdgeArrowsAct->setChecked(
        (appSettings["initEdgeArrows"] == "true") ? true : false);
    connect(optionsEdgeArrowsAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeArrowsVisibility(bool)));

    optionsEdgeThicknessPerWeightAct = new QAction(tr("Edge Thickness reflects Weight"), this);
    optionsEdgeThicknessPerWeightAct->setStatusTip(tr("Draw edges as thick as their weights (if specified)"));
    optionsEdgeThicknessPerWeightAct->setWhatsThis(
        tr("Edge thickness reflects weight\n\n"
           "Click to toggle having all edges as thick as their weight (if specified)"));
    optionsEdgeThicknessPerWeightAct->setCheckable(true);
    optionsEdgeThicknessPerWeightAct->setChecked(
        (appSettings["initEdgeThicknessPerWeight"] == "true") ? true : false);
    connect(optionsEdgeThicknessPerWeightAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgeThicknessPerWeight(bool)));
    optionsEdgeThicknessPerWeightAct->setEnabled(false);

    drawEdgesBezier = new QAction(tr("Bezier Curves"), this);
    drawEdgesBezier->setStatusTip(tr("Draw Edges as Bezier curves"));
    drawEdgesBezier->setWhatsThis(
        tr("Edges Bezier\n\n"
           "Enables or disables drawing edges as Bezier curves."));
    drawEdgesBezier->setCheckable(true);
    drawEdgesBezier->setChecked(
        (appSettings["initEdgeShape"] == "bezier") ? true : false);
    connect(drawEdgesBezier, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsEdgesBezier(bool)));

    changeBackColorAct = new QAction(QIcon(":/images/format_color_fill_48px.svg"), tr("Change Background Color"), this);
    changeBackColorAct->setStatusTip(tr("Change the canvasbackground color"));
    changeBackColorAct->setWhatsThis(tr("Background Color\n\n"
                                        "Changes the background color of the canvas"));
    connect(changeBackColorAct, SIGNAL(triggered()),
            this, SLOT(slotOptionsBackgroundColor()));

    backgroundImageAct = new QAction(QIcon(":/images/wallpaper_48px.svg"), tr("Background Image"), this);
    backgroundImageAct->setStatusTip(
        tr("Select and display a custom image in the canvas background"));
    backgroundImageAct->setWhatsThis(
        tr("Background Image\n\n"
           "Enable to select an image file from your computer, "
           "which will be displayed in the background instead of plain color."));
    backgroundImageAct->setCheckable(true);
    backgroundImageAct->setChecked(false);
    connect(backgroundImageAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsBackgroundImageSelect(bool)));

    fullScreenModeAct = new QAction(QIcon(":/images/fullscreen_48px.svg"), tr("Full screen (this session)"), this);
    fullScreenModeAct->setShortcut(QKeySequence::FullScreen);
    fullScreenModeAct->setStatusTip(
        tr("Toggle full screen mode (for this session only)"));
    fullScreenModeAct->setWhatsThis(
        tr("Full Screen Mode\n\n"
           "Enable to show application window in full screen mode. "
           "This setting will apply to this session only. \n"
           "To permanently change it, go to Settings."));
    fullScreenModeAct->setCheckable(true);
    fullScreenModeAct->setChecked(false);
    connect(fullScreenModeAct, SIGNAL(triggered(bool)),
            this, SLOT(slotOptionsWindowFullScreen(bool)));

    openSettingsAct = new QAction(QIcon(":/images/settings_48px.svg"), tr("Settings"), this);
    openSettingsAct->setShortcut(Qt::CTRL | Qt::Key_Comma);
    openSettingsAct->setEnabled(true);
    openSettingsAct->setToolTip(
        tr("Open the Settings dialog where you can save your preferences "
           "for all future sessions"));
    openSettingsAct->setStatusTip(
        tr("Open the Settings dialog to save your preferences "
           "for all future sessions"));
    openSettingsAct->setWhatsThis(
        tr("Settings\n\n"
           "Opens the Settings dialog where you can edit and save settings "
           "permanently for all subsequent sessions."));
    connect(openSettingsAct, SIGNAL(triggered()),
            this, SLOT(slotOpenSettingsDialog()));

    viewDataTableAct = new QAction(QIcon(":/images/data_table_48px.svg"), tr("Data Table"), this);
    viewDataTableAct->setCheckable(true);
    viewDataTableAct->setChecked(false);
    viewDataTableAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    viewDataTableAct->setStatusTip(tr("Show/hide the node and edge data table panel"));
    viewDataTableAct->setToolTip(tr("Toggle the node/edge data table panel (Ctrl+D)"));
    connect(viewDataTableAct, &QAction::toggled,
            this, &MainWindow::slotViewDataTable);

    /**
    Help menu actions
    */
    helpApp = new QAction(QIcon(":/images/help_48px.svg"), tr("Manual"), this);
    helpApp->setShortcut(Qt::Key_F1);
    helpApp->setStatusTip(tr("Read the manual..."));
    helpApp->setWhatsThis(tr("Manual\n\nDisplays the documentation of SocNetV"));
    connect(helpApp, SIGNAL(triggered()), this, SLOT(slotHelp()));

    tipsApp = new QAction(QIcon(":/images/tip_24px.svg"), tr("Tip of the Day"), this);
    tipsApp->setStatusTip(tr("Read useful tips"));
    tipsApp->setWhatsThis(tr("Quick Tips\n\nDisplays some useful and quick tips"));
    connect(tipsApp, SIGNAL(triggered()), this, SLOT(slotHelpTips()));

    helpCheckUpdatesApp = new QAction(
        QIcon(":/images/system_update_alt_48px.svg"), tr("Check for Updates"), this);
    helpCheckUpdatesApp->setStatusTip(tr("Open a browser to SocNetV website "
                                         "to check for a new version..."));
    helpCheckUpdatesApp->setWhatsThis(tr("Check Updates\n\n"
                                         "Open a browser to SocNetV website so "
                                         "that you can check yourself for updates"));
    connect(helpCheckUpdatesApp, SIGNAL(triggered()),
            this, SLOT(slotHelpCheckUpdateDialog()));

    helpSystemInfoAct = new QAction(QIcon(":/images/about_24px.svg"), tr("System Information"), this);
    helpSystemInfoAct->setEnabled(true);
    helpSystemInfoAct->setStatusTip(tr("Show information about your system"));
    helpSystemInfoAct->setWhatsThis(
        tr("<p><b>System Information</b></p>"
           "<p>Shows useful information about your system, "
           "which you can include in your bug reports. </p>"));

    connect(helpSystemInfoAct, SIGNAL(triggered()), this, SLOT(slotHelpSystemInfo()));

    helpAboutApp = new QAction(QIcon(":/images/about_24px.svg"), tr("About SocNetV"), this);
    helpAboutApp->setStatusTip(tr("About SocNetV"));
    helpAboutApp->setWhatsThis(tr("About\n\nBasic information about SocNetV"));
    connect(helpAboutApp, SIGNAL(triggered()), this, SLOT(slotHelpAbout()));

    helpAboutQt = new QAction(QIcon(":/images/qt.png"), tr("About Qt"), this);
    helpAboutQt->setStatusTip(tr("About Qt"));
    helpAboutQt->setWhatsThis(tr("About\n\nAbout Qt"));
    connect(helpAboutQt, SIGNAL(triggered()), this, SLOT(slotAboutQt()));

    qCDebug(lcMainWindow) << "Finished actions initialization.";
}
