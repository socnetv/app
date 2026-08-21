/**
 * @file mainwindow_init_menu.cpp
 * @brief Implements MainWindow menu bar and toolbar construction.
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
 * @brief Populates the menu bar with our menu items.
 */
void MainWindow::initMenuBar()
{

    qCDebug(lcMainWindow) << "Initializing menu bar...";

    /** NETWORK MENU */
    networkMenu = menuBar()->addMenu(tr("&Network"));
    networkMenu->addAction(networkNewAct);
    networkMenu->addAction(networkOpenAct);
    networkMenu->addSeparator();
    recentFilesSubMenu = new QMenu(tr("Recent &files..."));
    recentFilesSubMenu->setIcon(QIcon(":/images/recent_48px.svg"));
    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        recentFilesSubMenu->addAction(recentFileActs[i]);
    }

    slotNetworkFileRecentUpdateActions();

    networkMenu->addMenu(recentFilesSubMenu);
    networkMenu->addSeparator();
    importSubMenu = new QMenu(tr("&Import ..."));
    importSubMenu->setIcon(QIcon(":/images/file_upload_48px.svg"));
    importSubMenu->addAction(networkImportGMLAct);
    importSubMenu->addAction(networkImportPajekAct);
    importSubMenu->addAction(networkImportAdjAct);
    importSubMenu->addAction(networkImportTwoModeSM);
    importSubMenu->addAction(networkImportListAct);
    importSubMenu->addAction(networkImportUcinetAct);
    importSubMenu->addAction(networkImportGraphvizAct);
    networkMenu->addMenu(importSubMenu);

    networkMenu->addSeparator();
    networkMenu->addAction(openTextEditorAct);
    networkMenu->addAction(networkViewFileAct);
    networkMenu->addSeparator();
    networkMenu->addAction(networkViewSociomatrixAct);
    networkMenu->addAction(networkViewSociomatrixPlotAct);
    networkMenu->addSeparator();

    networkMenu->addAction(networkDataSetSelectAct);
    networkMenu->addSeparator();

    randomNetworkMenu = new QMenu(tr("Create &Random Network..."));
    randomNetworkMenu->setIcon(QIcon(":/images/random_48px.svg"));
    networkMenu->addMenu(randomNetworkMenu);

    randomNetworkMenu->addAction(networkRandomScaleFreeAct);
    randomNetworkMenu->addAction(networkRandomSmallWorldAct);
    randomNetworkMenu->addAction(networkRandomErdosRenyiAct);
    randomNetworkMenu->addAction(networkRandomLatticeAct);
    randomNetworkMenu->addAction(networkRandomRegularSameDegreeAct);
    randomNetworkMenu->addAction(networkRandomLatticeRingAct);
    // networkRandomGaussianAct->addTo(randomNetworkMenu);
    networkMenu->addSeparator();

    networkMenu->addAction(networkWebCrawlerAct);

    networkMenu->addSeparator();
    networkMenu->addAction(networkSaveAct);
    networkMenu->addAction(networkSaveAsAct);
    networkMenu->addSeparator();

    networkMenu->addAction(networkExportImageAct);
    networkMenu->addAction(networkExportPDFAct);
    networkMenu->addSeparator();
    exportSubMenu = networkMenu->addMenu(tr("Export to other..."));
    exportSubMenu->setIcon(QIcon(":/images/file_download_48px.svg"));

    exportSubMenu->addAction(networkExportSMAct);
    exportSubMenu->addAction(networkExportPajek);
    exportSubMenu->addAction(networkExportDotAct);
    exportSubMenu->addAction(networkExportDLAct);
    exportSubMenu->addAction(networkExportListAct);
    // exportSubMenu->addAction (networkExportGW);

    exportSubMenu->addSeparator();
    exportSubMenu->addAction(networkExportNodesCSVAct);
    exportSubMenu->addAction(networkExportEdgesCSVAct);
    exportSubMenu->addAction(networkExportNodesJSONAct);
    exportSubMenu->addAction(networkExportEdgesJSONAct);

    networkMenu->addSeparator();
    networkMenu->addAction(networkPrintAct);
    networkMenu->addSeparator();
    networkMenu->addAction(networkCloseAct);
    networkMenu->addAction(networkQuitAct);

    // EDIT MENU
    editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu->addAction(editRelationPreviousAct);
    editMenu->addAction(editRelationNextAct);
    editMenu->addAction(editRelationAddAct);
    editMenu->addAction(editRelationRenameAct);

    editMenu->addSeparator();

    editMenu->addAction(zoomInAct);
    editMenu->addAction(zoomOutAct);

    editMenu->addSeparator();

    editMenu->addAction(editRotateLeftAct);
    editMenu->addAction(editRotateRightAct);

    editMenu->addSeparator();
    editMenu->addAction(editResetSlidersAct);

    editMenu->addSeparator();
    editNodeMenu = new QMenu(tr("Nodes..."));
    editNodeMenu->setIcon(QIcon(":/images/node_48px.svg"));
    editMenu->addMenu(editNodeMenu);
    editNodeMenu->addAction(editNodeSelectAllAct);
    editNodeMenu->addAction(editNodeSelectNoneAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction(editNodeFindAct);
    editNodeMenu->addAction(editNodeAddAct);
    editNodeMenu->addAction(editNodeRemoveAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction(editNodePropertiesAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction(editNodeSelectedToCliqueAct);
    editNodeMenu->addAction(editNodeSelectedToStarAct);
    editNodeMenu->addAction(editNodeSelectedToCycleAct);
    editNodeMenu->addAction(editNodeSelectedToLineAct);

    editNodeMenu->addSeparator();

    editNodeMenu->addAction(editNodeColorAll);
    editNodeMenu->addAction(editNodeSizeAllAct);
    editNodeMenu->addAction(editNodeShapeAll);
    editNodeMenu->addSeparator();
    editNodeMenu->addAction(editNodeNumbersSizeAct);
    editNodeMenu->addAction(editNodeNumbersColorAct);
    editNodeMenu->addSeparator();
    editNodeMenu->addAction(editNodeLabelsSizeAct);
    editNodeMenu->addAction(editNodeLabelsColorAct);
    editNodeMenu->addSeparator();
    editNodeMenu->addAction(editFilterNodesIsolatesAct);

    editEdgeMenu = new QMenu(tr("Edges..."));
    editEdgeMenu->setIcon(QIcon(":/images/edges_48px.svg"));
    editMenu->addMenu(editEdgeMenu);
    editEdgeMenu->addAction(editEdgeAddAct);
    editEdgeMenu->addAction(editEdgeRemoveAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeUndirectedAllAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeSymmetrizeAllAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeSymmetrizeStrongTiesAct);
    editEdgeMenu->addAction(editEdgesCocitationAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeDichotomizeAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editFilterEdgesUnilateralAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeLabelAct);
    editEdgeMenu->addAction(editEdgeColorAct);
    editEdgeMenu->addAction(editEdgeWeightAct);
    editEdgeMenu->addSeparator();
    editEdgeMenu->addAction(editEdgeColorAllAct);

    //   transformNodes2EdgesAct->addTo (editMenu);

    editMenu->addSeparator();
    filterMenu = new QMenu(tr("Filter..."));
    filterMenu->setIcon(QIcon(":/images/filter_list_48px.svg"));
    editMenu->addMenu(filterMenu);

    // — Node filters —
    filterMenu->addAction(filterNodesBySelectionAct);
    filterMenu->addAction(filterNodesByEgoNetworkAct);
    filterMenu->addAction(filterNodesByCentralityAct);
    filterMenu->addSeparator();
    // — Edge filters —
    filterMenu->addAction(editFilterEdgesByWeightAct);
    filterMenu->addSeparator();
    // — Attribute filter (nodes + edges) —
    filterMenu->addAction(filterNodesByAttributeAct);
    filterMenu->addAction(filterByQueryBuilderAct);
    filterMenu->addSeparator();
    // — Restore —
    filterMenu->addAction(filterNodesRestoreAllAct);
    filterMenu->addAction(editFilterEdgesRestoreAllAct);

    editMenu->addSeparator();
    subgraphMenu = new QMenu(tr("Subgraphs..."));
    subgraphMenu->setIcon(QIcon(":/images/filter_list_48px.svg"));
    editMenu->addMenu(subgraphMenu);
    subgraphMenu->addAction(editSubgraphExtractAct);
    subgraphMenu->addSeparator();
    subgraphMenu->addAction(editSubgraphExtractFromSelectionAct);

    editMenu->addSeparator();
    editMenu->addAction(viewDataTableAct);

    // ANALYZE MENU
    analysisMenu = menuBar()->addMenu(tr("&Analyze"));
    matrixMenu = new QMenu(tr("Adjacency Matrix and Matrices..."));
    matrixMenu->setIcon(QIcon(":/images/sociomatrix_48px.svg"));
    analysisMenu->addMenu(matrixMenu);
    matrixMenu->addAction(networkViewSociomatrixAct);
    matrixMenu->addAction(networkViewSociomatrixPlotAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixAdjInvertAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixAdjTransposeAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixAdjCocitationAct);
    matrixMenu->addSeparator();
    matrixMenu->addAction(analyzeMatrixDegreeAct);
    matrixMenu->addAction(analyzeMatrixLaplacianAct);
    //	analysisMenu->addAction (netDensity);

    analysisMenu->addSeparator();
    cohesionMenu = new QMenu(tr("Cohesion..."));
    cohesionMenu->setIcon(QIcon(":/images/assessment_48px.svg"));
    analysisMenu->addMenu(cohesionMenu);
    cohesionMenu->addAction(analyzeGraphReciprocityAct);
    cohesionMenu->addAction(analyzeGraphSymmetryAct);
    cohesionMenu->addSection("Graph distances");
    cohesionMenu->addAction(analyzeGraphDistanceAct);
    cohesionMenu->addAction(averGraphDistanceAct);
    cohesionMenu->addAction(analyzeGeodesicDistributionAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeMatrixDistancesGeodesicAct);
    cohesionMenu->addAction(analyzeMatrixGeodesicsAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeGraphEccentricityAct);
    cohesionMenu->addAction(analyzeGraphDiameterAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeGraphConnectednessAct);
    cohesionMenu->addAction(analyzeNodeConnectivityAct);
    cohesionMenu->addAction(analyzeConnectivityAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeGraphWalksAct);
    cohesionMenu->addAction(analyzeGraphWalksTotalAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(analyzeMatrixReachabilityAct);
    cohesionMenu->addSeparator();
    cohesionMenu->addAction(clusteringCoefAct);

    analysisMenu->addSeparator();

    // CENTRALITIES
    centrlMenu = new QMenu(tr("Centrality and Prestige indices..."));
    centrlMenu->setIcon(QIcon(":/images/centrality_48px.svg"));
    analysisMenu->addMenu(centrlMenu);

    centrlMenu->addAction(cDegreeAct);
    centrlMenu->addAction(cClosenessAct);
    centrlMenu->addAction(cInfluenceRangeClosenessAct);
    centrlMenu->addAction(cBetweennessAct);
    centrlMenu->addAction(cStressAct);
    centrlMenu->addAction(cEccentAct);
    centrlMenu->addAction(cPowerAct);
    centrlMenu->addAction(cInformationAct);
    centrlMenu->addAction(cEigenvectorAct);
    centrlMenu->addSeparator();
    centrlMenu->addAction(cInDegreeAct);
    centrlMenu->addAction(cPageRankAct);
    centrlMenu->addAction(cProximityPrestigeAct);

    analysisMenu->addSeparator();
    // COMMUNITIES & SUBGROUPS
    communitiesMenu = new QMenu(tr("Communities and Subgroups..."));
    communitiesMenu->setIcon(QIcon(":/images/communities_48px.svg"));
    analysisMenu->addMenu(communitiesMenu);
    communitiesMenu->addAction(analyzeCommunitiesCliquesAct);
    communitiesMenu->addSeparator();
    communitiesMenu->addAction(analyzeCommunitiesTriadCensusAct);

    analysisMenu->addSeparator();
    // STRUCTURAL EQUIVALENCE
    strEquivalenceMenu = new QMenu(tr("Structural Equivalence..."));
    strEquivalenceMenu->setIcon(QIcon(":/images/similarity.png"));
    analysisMenu->addMenu(strEquivalenceMenu);
    strEquivalenceMenu->addAction(analyzeStrEquivalencePearsonAct);
    strEquivalenceMenu->addAction(analyzeStrEquivalenceMatchesAct);
    strEquivalenceMenu->addSeparator();
    strEquivalenceMenu->addAction(analyzeStrEquivalenceTieProfileDissimilaritiesAct);
    strEquivalenceMenu->addSeparator();
    strEquivalenceMenu->addAction(analyzeStrEquivalenceClusteringHierarchicalAct);

    // LAYOUT MENU
    layoutMenu = menuBar()->addMenu(tr("&Layout"));
    //   colorationMenu = new QPopupMenu();
    //   layoutMenu->insertItem (tr("Colorization"), colorationMenu);
    //   strongColorationAct->addTo(colorationMenu);
    //   regularColorationAct->addTo(colorationMenu);
    //   layoutMenu->insertSeparator();
    randomLayoutMenu = new QMenu(tr("Random..."));
    randomLayoutMenu->setIcon(QIcon(":/images/random_48px.svg"));
    layoutMenu->addMenu(randomLayoutMenu);
    randomLayoutMenu->addAction(layoutRandomAct);
    randomLayoutMenu->addAction(layoutRandomRadialAct);
    layoutMenu->addSeparator();

    layoutRadialProminenceMenu = new QMenu(tr("Radial by prominence index..."));
    layoutRadialProminenceMenu->setIcon(QIcon(":/images/radial_layout_48px.svg"));
    layoutMenu->addMenu(layoutRadialProminenceMenu);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_DC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_CC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_IRCC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_BC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_SC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_EC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_PC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_IC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_EVC_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_DP_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_PRP_Act);
    layoutRadialProminenceMenu->addAction(layoutRadialProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutLevelProminenceMenu = new QMenu(tr("On Levels by prominence index..."));
    layoutLevelProminenceMenu->setIcon(QIcon(":/images/layout_levels_24px.svg"));
    layoutMenu->addMenu(layoutLevelProminenceMenu);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_DC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_CC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_IRCC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_BC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_SC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_EC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_PC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_IC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_EVC_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_DP_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_PRP_Act);
    layoutLevelProminenceMenu->addAction(layoutLevelProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeSizeProminenceMenu = new QMenu(tr("Node Size by prominence index..."));
    layoutNodeSizeProminenceMenu->setIcon(QIcon(":/images/node_size_48px.svg"));
    layoutMenu->addMenu(layoutNodeSizeProminenceMenu);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_DC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_CC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_IRCC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_BC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_SC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_EC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_PC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_IC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_EVC_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_DP_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_PRP_Act);
    layoutNodeSizeProminenceMenu->addAction(layoutNodeSizeProminence_PP_Act);

    layoutMenu->addSeparator();

    layoutNodeColorProminenceMenu = new QMenu(tr("Node Color by prominence index..."));
    layoutNodeColorProminenceMenu->setIcon(QIcon(":/images/color_layout_48px.svg"));
    layoutMenu->addMenu(layoutNodeColorProminenceMenu);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_DC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_CC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_IRCC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_BC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_SC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_EC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_PC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_IC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_EVC_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_DP_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_PRP_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_PP_Act);
    layoutNodeColorProminenceMenu->addAction(layoutNodeColorProminence_CLC_Act);

    layoutMenu->addSeparator();
    layoutMenu->addAction(layoutNodeColorByComponentAct);

    layoutMenu->addSeparator();

    layoutForceDirectedMenu = new QMenu(tr("Force-Directed Placement..."));
    layoutForceDirectedMenu->setIcon(QIcon(":/images/force.png"));
    layoutMenu->addMenu(layoutForceDirectedMenu);
    layoutForceDirectedMenu->addAction(layoutFDP_KamadaKawai_Act);
    layoutForceDirectedMenu->addAction(layoutFDP_FR_Act);
    layoutForceDirectedMenu->addAction(layoutFDP_Eades_Act);

    layoutMenu->addSeparator();
    layoutMenu->addAction(layoutEgoRadialAct);
    layoutMenu->addSeparator();
    layoutMenu->addAction(layoutGuidesAct);

    // OPTIONS MENU
    optionsMenu = menuBar()->addMenu(tr("&Options"));
    nodeOptionsMenu = new QMenu(tr("Nodes..."));
    nodeOptionsMenu->setIcon(QIcon(":/images/node_48px.svg"));

    optionsMenu->addMenu(nodeOptionsMenu);
    nodeOptionsMenu->addAction(optionsNodeNumbersVisibilityAct);
    nodeOptionsMenu->addAction(optionsNodeLabelsVisibilityAct);
    nodeOptionsMenu->addAction(optionsNodeNumbersInsideAct);

    edgeOptionsMenu = new QMenu(tr("Edges..."));
    edgeOptionsMenu->setIcon(QIcon(":/images/edges_48px.svg"));

    optionsMenu->addMenu(edgeOptionsMenu);
    edgeOptionsMenu->addAction(optionsEdgesVisibilityAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction(optionsEdgeWeightNumbersAct);
    edgeOptionsMenu->addAction(optionsEdgeWeightConsiderAct);
    edgeOptionsMenu->addAction(optionsEdgeThicknessPerWeightAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction(optionsEdgeLabelsAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction(optionsEdgeArrowsAct);
    edgeOptionsMenu->addSeparator();
    edgeOptionsMenu->addAction(drawEdgesBezier);

    viewOptionsMenu = new QMenu(tr("&Canvas..."));
    viewOptionsMenu->setIcon(QIcon(":/images/view.png"));
    optionsMenu->addMenu(viewOptionsMenu);
    viewOptionsMenu->addAction(changeBackColorAct);
    viewOptionsMenu->addAction(backgroundImageAct);

    optionsMenu->addSeparator();
    optionsMenu->addAction(fullScreenModeAct);
    optionsMenu->addAction(viewDataTableAct);

    optionsMenu->addSeparator();
    optionsMenu->addAction(openSettingsAct);

    // HELP MENU
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(helpApp);
    helpMenu->addAction(tipsApp);
    helpMenu->addSeparator();
    helpMenu->addAction(helpCheckUpdatesApp);
    helpMenu->addSeparator();
    helpMenu->addAction(helpSystemInfoAct);
    helpMenu->addAction(helpAboutApp);
    helpMenu->addAction(helpAboutQt);

    qCDebug(lcMainWindow) << "Finished menu bar init.";
}

/**
 * @brief Initializes the toolbar
 */
void MainWindow::initToolBar()
{

    qCDebug(lcMainWindow) << "Initializing toolbar...";

    toolBar = addToolBar("operations");

    toolBar->addAction(networkNewAct);
    toolBar->addAction(networkOpenAct);
    toolBar->addAction(networkSaveAct);
    toolBar->addAction(networkPrintAct);

    toolBar->addSeparator();

    toolBar->addAction(editMouseModeInteractiveAct);
    toolBar->addAction(editMouseModeScrollAct);

    toolBar->addSeparator();

    // Create relation select widget
    //    QLabel *labelRelationSelect= new QLabel;
    //    labelRelationSelect->setText(tr("Relations:"));
    //    toolBar->addWidget (labelRelationSelect);
    toolBar->addAction(editRelationPreviousAct);
    editRelationChangeCombo = new QComboBox;
    editRelationChangeCombo->setEditable(true);
    editRelationChangeCombo->setInsertPolicy(QComboBox::InsertAtCurrent);
    editRelationChangeCombo->setMinimumWidth(180);
    editRelationChangeCombo->setCurrentIndex(0);
    editRelationChangeCombo->setToolTip(
        tr("<p><b>Current relation<b></p>"
           "<p>To rename the current relation, click here, enter new name and press Enter.</p>"));
    editRelationChangeCombo->setStatusTip(
        tr("Name of the current relation. "
           "To rename it, enter a new name and press Enter. To select another relation, click the Down arrow (on the right)."));
    editRelationChangeCombo->setWhatsThis(
        tr("<p><b>Relations combo</b></p>"
           "<p>This displays the currently selected relation of the network. </p>"
           "<p>To rename the current relation, click on the name, enter a new name and press Enter. </p>"
           "<p>To select another relation (if any), click the Down arrow (on the right).</p>"));

    toolBar->addWidget(editRelationChangeCombo);
    toolBar->addAction(editRelationNextAct);
    toolBar->addAction(editRelationAddAct);

    toolBar->addSeparator();

    //    QLabel *labelEditNodes= new QLabel;
    //    labelEditNodes->setText(tr("Nodes:"));
    //    toolBar->addWidget (labelEditNodes);
    toolBar->addAction(editNodeAddAct);
    toolBar->addAction(editNodeRemoveAct);
    toolBar->addAction(editNodeFindAct);
    toolBar->addAction(editNodePropertiesAct);
    toolBar->addAction(filterNodesByCentralityAct);
    toolBar->addAction(filterNodesByAttributeAct);
    toolBar->addAction(filterNodesRestoreAllAct);

    toolBar->addSeparator();

    toolBar->addAction(editEdgeAddAct);
    toolBar->addAction(editEdgeRemoveAct);
    toolBar->addAction(editEdgePropertiesAct);
    toolBar->addAction(editFilterEdgesByWeightAct);
    toolBar->addAction(editFilterEdgesRestoreAllAct);

    toolBar->addSeparator();

    //    QLabel *labelApplicationIcons = new QLabel;
    //    labelApplicationIcons->setText(tr("Settings:"));
    //    toolBar->addWidget(labelApplicationIcons);
    toolBar->addAction(openSettingsAct);
    toolBar->addSeparator();
    toolBar->addAction(QWhatsThis::createAction(this));
    toolBar->setIconSize(QSize(16, 16));

    qCDebug(lcMainWindow) << "Finished toolbar init.";
}
