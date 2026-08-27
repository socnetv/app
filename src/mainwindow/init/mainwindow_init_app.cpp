/**
 * @file mainwindow_init_app.cpp
 * @brief Implements remaining MainWindow startup initialization: canvas/graph/signal-slot wiring, app defaults, combo boxes, and text codec setup.
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
#include <QTextCodec>

/**
 * @brief Initializes our graphics widget, the canvas where we draw networks
 *
 * The widget is a QGraphicsView, with a scene, and is the 'main' widget of the application.
 */
void MainWindow::initView()
{

    qCDebug(lcMainWindow) << "Creating graphics widget...";

    // Create our scene
    scene = new QGraphicsScene();

    // Create a view widget and pass the scene and the our object as parent
    graphicsWidget = new GraphicsWidget(scene, this);
    graphicsWidget->setObjectName("graphicsWidget");

    bool toggle = false;

    toggle = (appSettings["opengl"] == "true") ? true : false;
    graphicsWidget->setOptionsOpenGL(toggle);

    toggle = (appSettings["antialiasing"] == "true") ? true : false;
    graphicsWidget->setOptionsAntialiasing(toggle);

    // Disables QGraphicsView's antialiasing auto-adjustment of exposed areas.
    toggle = (appSettings["canvasAntialiasingAutoAdjustment"] == "true") ? false : true;
    graphicsWidget->setOptionsNoAntialiasingAutoAdjust(toggle);

    toggle = (appSettings["canvasSmoothPixmapTransform"] == "true") ? true : false;
    graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);

    // if items do restore their state, it's not needed for graphicsWidget to do the same...
    toggle = (appSettings["canvasPainterStateSave"] == "true") ? false : true;
    graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, toggle);

    if (appSettings["canvasUpdateMode"] == "Full")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
    else if (appSettings["canvasUpdateMode"] == "Minimal")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }
    else if (appSettings["canvasUpdateMode"] == "Smart")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    }
    else if (appSettings["canvasUpdateMode"] == "Bounding Rectangle")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    }
    else if (appSettings["canvasUpdateMode"] == "None")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }
    else
    { //
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }

    // QGraphicsView can cache pre-rendered content in a QPixmap, which is then drawn onto the viewport.
    if (appSettings["canvasCacheBackground"] == "true")
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
    }
    else
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheNone);
    }

    graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // graphicsWidget->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    // graphicsWidget->setTransformationAnchor(QGraphicsView::NoAnchor);
    graphicsWidget->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // sets dragging the mouse over the scene while the left mouse button is pressed.
    graphicsWidget->setDragMode(QGraphicsView::RubberBandDrag);

    graphicsWidget->setFocusPolicy(Qt::StrongFocus);
    graphicsWidget->setFocus();

    graphicsWidget->setWhatsThis(tr("<p><b>The canvas of SocNetV</b></p>"
                                    "<p>Inside this area you create and edit networks, "
                                    "load networks from files and visualize them "
                                    "according to the selected metrics. </p>"
                                    "<p>To create a new node, <em>double-click</em> anywhere.</p>"
                                    "<p>To add an edge between two nodes, <em>double-click</em>"
                                    " on the first node (source) then double-click on the second (target) .</p>"
                                    "<p>To move around the canvas, use the keyboard arrows.</p>"
                                    "<p>To change network appearance, <em>right click on empty space</em>. </p>"
                                    "<p>To edit the properties of a node, <em>right-click</em> on it. </p>"
                                    "<p>To edit the properties of an edge, <em>right-click</em> on it.</p>"));

    qCDebug(lcMainWindow) << "Finished initialization of graphics widget. Dimensions:"
             << graphicsWidget->width() << "x" << graphicsWidget->height();
}

/**
 * @brief Initializes the Graph
 */
void MainWindow::initGraph()
{

    qCDebug(lcMainWindow) << "creating activeGraph object...";

    bool ok1;
    nodesEstimatedSize = (appSettings["initNodesEstimatedSize"]).toInt(&ok1, 10);
    if (!ok1)
    {
        nodesEstimatedSize = 0;
    }

    bool ok2;
    edgesPerNodeEstimatedSize = (appSettings["initEdgesPerNodeEstimatedSize"]).toInt(&ok2, 10);
    if (!ok2)
    {
        edgesPerNodeEstimatedSize = 0;
    }

    activeGraph = new Graph(nodesEstimatedSize, edgesPerNodeEstimatedSize);

    qCDebug(lcMainWindow) << "activeGraph created on thread:" << activeGraph->getThread()
             << "moving it to new thread ";

    activeGraph->moveToThreadFacade(&graphThread);

    qCDebug(lcMainWindow) << "activeGraph moved to thread:" << activeGraph->getThread()
             << "starting new activeGraph thread...";

    graphThread.start();

    qCDebug(lcMainWindow) << "activeGraph thread now:" << activeGraph->getThread()
             << "Finished initialization of graph.";
}

/**
 * @brief Connects signals & slots between various parts of the app
 *
 * Signal/slots between:
 * - the GraphicsWidget and the Graph
 * - the GraphicsWidget and the MainWindow
 * This must be called after all widgets have been created.
 *
 */
void MainWindow::initSignalSlots()
{
    qCDebug(lcMainWindow) << "setting up signals/slots between widgets (graphicsWidget, activeGraph and MW)...";

    // Signals between graphicsWidget and MainWindow

    connect(graphicsWidget, &GraphicsWidget::setCursor,
            this, &MainWindow::setCursor);

    connect(graphicsWidget, &GraphicsWidget::userMiddleClicked,
            this, &MainWindow::slotEditEdgeCreate);

    connect(graphicsWidget, SIGNAL(openNodeMenu()),
            this, SLOT(slotEditNodeOpenContextMenu()));

    connect(graphicsWidget, &GraphicsWidget::openContextMenu,
            this, &MainWindow::slotEditOpenContextMenu);

    connect(graphicsWidget, SIGNAL(userNodeMoved(const int &, const int &, const int &)),
            this, SLOT(slotEditNodePosition(const int &, const int &, const int &)));

    connect(graphicsWidget, SIGNAL(zoomChanged(const int &)),
            zoomSlider, SLOT(setValue(const int &)));

    connect(zoomSlider, SIGNAL(sliderMoved(const int &)),
            graphicsWidget, SLOT(changeMatrixScale(const int &)));

    connect(zoomInBtn, SIGNAL(clicked()), graphicsWidget, SLOT(zoomIn()));
    connect(zoomOutBtn, SIGNAL(clicked()), graphicsWidget, SLOT(zoomOut()));

    // rotateSlider::valueChanged (unlike zoomSlider::sliderMoved above) fires on
    // programmatic setValue() too, so updating it here would otherwise bounce straight
    // back into changeMatrixRotation() with the same angle. QSignalBlocker keeps this a
    // display-only update.
    connect(graphicsWidget, &GraphicsWidget::rotationChanged,
            this, [this](const int angle) {
                const QSignalBlocker blocker(rotateSlider);
                rotateSlider->setValue(angle);
            });

    connect(rotateSlider, SIGNAL(valueChanged(const int &)),
            graphicsWidget, SLOT(changeMatrixRotation(const int &)));

    connect(rotateLeftBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateLeft()));
    connect(rotateRightBtn, SIGNAL(clicked()), graphicsWidget, SLOT(rotateRight()));

    connect(resetSlidersBtn, SIGNAL(clicked()), graphicsWidget, SLOT(reset()));

    //
    // SIGNALS BETWEEN ACTIVEGRAPH AND MAINWINDOW
    //
    connect(activeGraph, &Graph::signalSelectionChanged,
            this, &MainWindow::slotEditSelectionChanged);

    connect(activeGraph, &Graph::signalNodeClickedInfo,
            this, &MainWindow::slotEditNodeInfoStatusBar);

    connect(activeGraph, &Graph::signalEdgeClicked,
            this, &MainWindow::slotEditEdgeClicked);

    connect(activeGraph, &Graph::signalGraphModified,
            this, &MainWindow::slotNetworkChanged);

    connect(activeGraph, &Graph::signalGraphLoaded,
            this, &MainWindow::slotNetworkFileLoaded);

    connect(activeGraph, &Graph::signalGraphLoaded,
            graphicsWidget, &GraphicsWidget::zoomToFit);

    connect(m_tableWidget, &GraphTableWidget::nodeSelected,
            this, [this](int number)
            { statusMessage(tr("Node %1 selected from data table").arg(number)); });

    connect(m_tableWidget, &GraphTableWidget::edgeSelected,
            this, [this](int source, int target)
            { statusMessage(tr("Edge %1→%2 selected from data table").arg(source).arg(target)); });

    connect(m_tableWidget, &GraphTableWidget::exportStatusMessage,
            this, &MainWindow::statusMessage);

    connect(m_tableWidget, &GraphTableWidget::importStatusMessage,
            this, &MainWindow::statusMessage);

    connect(activeGraph, &Graph::signalGraphSavedStatus,
            this, &MainWindow::slotNetworkSavedStatus);

    connect(activeGraph, SIGNAL(statusMessage(QString)),
            this, SLOT(statusMessage(QString)));

    connect(activeGraph, SIGNAL(signalDatasetDescription(QString)),
            this, SLOT(slotHelpMessageToUserInfo(QString)));

    connect(editRelationNextAct, &QAction::triggered,
            activeGraph, &Graph::relationNext);

    connect(editRelationPreviousAct, &QAction::triggered,
            activeGraph, &Graph::relationPrev);

    connect(editRelationChangeCombo, SIGNAL(activated(int)),
            activeGraph, SLOT(relationSet(int)));

    connect(editRelationChangeCombo, SIGNAL(currentTextChanged(const QString &)),
            activeGraph, SLOT(relationCurrentRename(const QString &)));

    //    connect( editRelationChangeCombo, &QComboBox::currentTextChanged,
    //             activeGraph, QOverload<const QString &>::of(&Graph::relationCurrentRename));

    connect(this, &MainWindow::signalRelationAddAndChange,
            activeGraph, &Graph::relationAdd);

    connect(activeGraph, &Graph::signalRelationChangedToMW,
            this, &MainWindow::slotEditRelationChange);

    connect(activeGraph, &Graph::signalGraphDirectedChanged,
            this, &MainWindow::slotEditGraphDirectedChanged);

    connect(activeGraph, &Graph::signalRelationsClear,
            this, &MainWindow::slotEditRelationsClear);

    connect(activeGraph, &Graph::signalRelationAddToMW,
            this, &MainWindow::slotEditRelationAdd);

    connect(activeGraph, &Graph::signalRelationRenamedToMW,
            editRelationChangeCombo, &QComboBox::setCurrentText);

    connect(activeGraph, &Graph::signalPromininenceDistributionChartUpdate,
            this, &MainWindow::slotAnalyzeProminenceDistributionChartUpdate);

    connect(activeGraph, &Graph::signalNetworkManagerRequest,
            this, &MainWindow::slotNetworkManagerRequest);

    //
    // Signals between activeGraph and graphicsWidget
    //

    connect(activeGraph, &Graph::addGuideCircle,
            graphicsWidget, &GraphicsWidget::addGuideCircle);

    connect(activeGraph, &Graph::addGuideHLine,
            graphicsWidget, &GraphicsWidget::addGuideHLine);

    connect(activeGraph, &Graph::setNodePos,
            graphicsWidget, &GraphicsWidget::moveNode);

    connect(activeGraph, &Graph::signalLayoutFinished,
            graphicsWidget, &GraphicsWidget::zoomToFit);

    connect(activeGraph, &Graph::signalNodesFound,
            graphicsWidget, &GraphicsWidget::setSelectedNodes);

    connect(activeGraph, &Graph::signalDrawNode,
            graphicsWidget, &GraphicsWidget::drawNode);

    connect(activeGraph, &Graph::signalRemoveNode,
            graphicsWidget, &GraphicsWidget::removeNode);

    connect(activeGraph, &Graph::setVertexVisibility,
            graphicsWidget, &GraphicsWidget::setNodeVisibility);

    connect(activeGraph, &Graph::setNodeSize,
            graphicsWidget, &GraphicsWidget::setNodeSize);

    connect(activeGraph, &Graph::setNodeColor,
            graphicsWidget, &GraphicsWidget::setNodeColor);

    connect(activeGraph, &Graph::setNodeShape,
            graphicsWidget, &GraphicsWidget::setNodeShape);

    connect(activeGraph, &Graph::setNodeNumberColor,
            graphicsWidget, &GraphicsWidget::setNodeNumberColor);

    connect(activeGraph, &Graph::setNodeNumberSize,
            graphicsWidget, &GraphicsWidget::setNodeNumberSize);

    connect(activeGraph, &Graph::setNodeNumberDistance,
            graphicsWidget, &GraphicsWidget::setNodeNumberDistance);

    connect(activeGraph, &Graph::setNodeLabel,
            graphicsWidget, &GraphicsWidget::setNodeLabel);

    connect(activeGraph, &Graph::setNodeLabelColor,
            graphicsWidget, &GraphicsWidget::setNodeLabelColor);

    connect(activeGraph, &Graph::setNodeLabelSize,
            graphicsWidget, &GraphicsWidget::setNodeLabelSize);

    connect(activeGraph, &Graph::setNodeLabelDistance,
            graphicsWidget, &GraphicsWidget::setNodeLabelDistance);

    connect(activeGraph, &Graph::signalRemoveEdge,
            graphicsWidget, &GraphicsWidget::removeEdge);

    connect(activeGraph, &Graph::signalDrawEdge,
            graphicsWidget, &GraphicsWidget::drawEdge);

    connect(activeGraph, &Graph::setEdgeWeight,
            graphicsWidget, &GraphicsWidget::setEdgeWeight);

    connect(activeGraph, &Graph::signalEdgeType,
            graphicsWidget, &GraphicsWidget::setEdgeDirectionType);

    connect(activeGraph, &Graph::setEdgeColor,
            graphicsWidget, &GraphicsWidget::setEdgeColor);

    connect(activeGraph, &Graph::setEdgeLabel,
            graphicsWidget, &GraphicsWidget::setEdgeLabel);

    connect(activeGraph, &Graph::signalSetEdgeVisibility,
            graphicsWidget, &GraphicsWidget::setEdgeVisibility);

    connect(activeGraph, &Graph::signalSetEdgesVisibilityBatch,
            graphicsWidget, &GraphicsWidget::setEdgesVisibilityBatch);

    connect(activeGraph, &Graph::signalRelationChangedToGW,
            graphicsWidget, &GraphicsWidget::relationSet);

    connect(graphicsWidget, &GraphicsWidget::userClickOnEmptySpace,
            activeGraph, &Graph::graphClickedEmptySpace);

    connect(graphicsWidget, &GraphicsWidget::resized,
            activeGraph, &Graph::canvasSizeSet);

    connect(graphicsWidget, &GraphicsWidget::userDoubleClickNewNode,
            activeGraph, &Graph::vertexCreateAtPos);

    connect(graphicsWidget, &GraphicsWidget::userSelectedItems,
            activeGraph, &Graph::setSelectionChanged);

    connect(graphicsWidget, &GraphicsWidget::userSelectedItems,
            this, &MainWindow::slotCacheSelection);

    connect(graphicsWidget, &GraphicsWidget::userClickedNode,
            activeGraph, &Graph::vertexClickedSet);

    connect(graphicsWidget, &GraphicsWidget::userClickedEdge,
            activeGraph, &Graph::edgeClickedSet);

    //
    // Signals and slots inside MainWindow
    //

#ifndef QT_NO_SSL
    connect(networkManager, &QNetworkAccessManager::sslErrors,
            this, &MainWindow::slotNetworkManagerSslErrors);
#endif

    connect(editMouseModeInteractiveAct, &QAction::triggered,
            this, &MainWindow::slotEditDragModeSelection);

    connect(editMouseModeScrollAct, &QAction::triggered,
            this, &MainWindow::slotEditDragModeScroll);

    connect(editRelationAddAct, SIGNAL(triggered()),
            this, SLOT(slotEditRelationAddPrompt()));

    connect(editRelationRenameAct, SIGNAL(triggered()),
            this, SLOT(slotEditRelationRename()));

    connect(zoomInAct, SIGNAL(triggered()), graphicsWidget, SLOT(zoomIn()));
    connect(zoomOutAct, SIGNAL(triggered()), graphicsWidget, SLOT(zoomOut()));
    connect(editRotateLeftAct, SIGNAL(triggered()), graphicsWidget, SLOT(rotateLeft()));
    connect(editRotateRightAct, SIGNAL(triggered()), graphicsWidget, SLOT(rotateRight()));
    connect(editResetSlidersAct, SIGNAL(triggered()), graphicsWidget, SLOT(reset()));

    connect(layoutGuidesAct, SIGNAL(triggered(bool)),
            this, SLOT(slotLayoutGuides(bool)));

    connect(toolBoxNetworkAutoCreateSelect, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::toolBoxNetworkAutoCreateSelectChanged);

    connect(toolBoxEditNodeSubgraphSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxEditNodeSubgraphSelectChanged(int)));

    connect(toolBoxEditEdgeModeSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotEditEdgeMode(int)));

    connect(toolBoxEditEdgeTransformSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxEditEdgeTransformSelectChanged(int)));

    connect(toolBoxFilterSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxFilterSelectChanged(int)));

    // Filter bar chip signals
    connect(m_filterBar, &FilterBarWidget::chipCloseRequested,
            this, [this](int barIndex, FilterCondition::Scope /*scope*/)
            {
        // barIndex == stackIndex: every filter (node or edge) pushes exactly one snapshot.
        runGraphOperationAsync(
            [this, barIndex]() { activeGraph->vertexFilterRemoveAt(barIndex); },
            tr("Removing filter..."),
            [this, barIndex]() {
                m_filterChips.removeAt(barIndex);
                // Rebuild bar from remaining chips (preserves original application order).
                m_filterBar->clearAllChips();
                for (const auto &chip : std::as_const(m_filterChips))
                    m_filterBar->addChip(chip.first, chip.second);
                // Update restore-action states.
                bool hasNodeFilters = false, hasEdgeFilters = false;
                for (const auto &chip : std::as_const(m_filterChips)) {
                    if (chip.second != FilterCondition::Scope::Edges) hasNodeFilters = true;
                    else                                               hasEdgeFilters = true;
                }
                filterNodesRestoreAllAct->setEnabled(hasNodeFilters);
                editFilterEdgesRestoreAllAct->setEnabled(hasEdgeFilters);
            }); });
    connect(m_filterBar, &FilterBarWidget::clearAllRequested,
            this, [this]()
            {
        runGraphOperationAsync(
            [this]() {
                while (!activeGraph->visibilityHistoryEmpty())
                    activeGraph->vertexFilterRestoreAll();
            },
            tr("Clearing all filters..."),
            [this]() {
                m_filterChips.clear();
                filterNodesRestoreAllAct->setEnabled(false);
                editFilterEdgesRestoreAllAct->setEnabled(false);
                m_filterBar->clearAllChips();
            }); });

    connect(toolBoxAnalysisMatricesSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxAnalysisMatricesSelectChanged(int)));

    connect(toolBoxAnalysisCohesionSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxAnalysisCohesionSelectChanged(int)));

    connect(toolBoxAnalysisStrEquivalenceSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxAnalysisStrEquivalenceSelectChanged(int)));

    connect(toolBoxAnalysisCommunitiesSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxAnalysisCommunitiesSelectChanged(int)));

    connect(toolBoxAnalysisProminenceSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxAnalysisProminenceSelectChanged(int)));

    connect(toolBoxLayoutByIndexSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxLayoutByIndexApplyBtnPressed()));

    connect(toolBoxLayoutByIndexTypeSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxLayoutByIndexApplyBtnPressed()));

    connect(toolBoxLayoutForceDirectedSelect, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toolBoxLayoutForceDirectedApplyBtnPressed()));
}

/**
 * @brief Initializes the default app parameters.
 *
 * Used on app start and when erasing a network to start a new one
 */
void MainWindow::initApp()
{

    qCDebug(lcMainWindow) << "### Application initialization starts, on thread" << thread();

    statusMessage(tr("Application initialization. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // first select none
    graphicsWidget->selectNone();

    // Init basic variables
    inverseWeights = false;
    askedAboutWeights = false;

    previous_fileName = fileName;
    fileName = "";

    initTextCodecName = "UTF-8";

    networkSaveAct->setIcon(QIcon(":/images/file_download_48px.svg"));
    networkSaveAct->setEnabled(false);

    /** Clear previous network data and reset user-selected settings */
    qCDebug(lcMainWindow) << "### Clearing current graph. Please wait...";
    activeGraph->clear();

    activeGraph->vertexShapeSetDefault(appSettings["initNodeShape"], appSettings["initNodeIconPath"]);
    activeGraph->vertexSizeInit(appSettings["initNodeSize"].toInt(0, 10));
    activeGraph->vertexColorInit(appSettings["initNodeColor"]);

    activeGraph->vertexNumberSizeInit(appSettings["initNodeNumberSize"].toInt(0, 10));
    activeGraph->vertexNumberColorInit(appSettings["initNodeNumberColor"]);
    activeGraph->vertexNumberDistanceInit(appSettings["initNodeNumberDistance"].toInt(0, 10));

    activeGraph->vertexLabelColorInit(appSettings["initNodeLabelColor"]);
    activeGraph->vertexLabelSizeInit(appSettings["initNodeLabelSize"].toInt(0, 10));
    activeGraph->vertexLabelDistanceInit(appSettings["initNodeLabelDistance"].toInt(0, 10));

    activeGraph->edgeColorInit(appSettings["initEdgeColor"]);
    activeGraph->edgeColorZeroInit(appSettings["initEdgeColorZero"]); // #30
    activeGraph->showZeroWeightEdgesSet(
        appSettings["showZeroWeightEdges"] == "true"); // #30

    activeGraph->edgeWeightNumbersVisibilitySet(
        (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true : false);
    activeGraph->setReportsRealNumberPrecision(appSettings["initReportsRealNumberPrecision"].toInt());

    activeGraph->setReportsLabelLength(appSettings["initReportsLabelsLength"].toInt());
    activeGraph->setReportsChartType(appSettings["initReportsChartType"].toInt());
    activeGraph->setReportsOutputFormat(appSettings["initReportsOutputFormat"].toInt());

    emit signalSetReportsDataDir(appSettings["dataDir"]);

    /** Clear graphicsWidget and reset settings and transformations **/
    qCDebug(lcMainWindow) << "### Clearing graphicsWidget and resetting transformations. Please wait...";
    graphicsWidget->clear();
    rotateSlider->setValue(0);
    zoomSlider->setValue((int)maxZoomIndex / 2.0);
    //    graphicsWidget->setInitZoomIndex((int) maxZoomIndex/2.0);
    graphicsWidget->setMaxZoomIndex(maxZoomIndex);

    graphicsWidget->setInitNodeSize(appSettings["initNodeSize"].toInt(0, 10));
    graphicsWidget->setNodeNumberVisibility(
        (appSettings["initNodeNumbersVisibility"] == "true") ? true : false);
    graphicsWidget->setNodeLabelsVisibility(
        (appSettings["initNodeLabelsVisibility"] == "true") ? true : false);

    graphicsWidget->setNumbersInsideNodes(
        (appSettings["initNodeNumbersInside"] == "true") ? true : false);
    graphicsWidget->setEdgeHighlighting(
        (appSettings["canvasEdgeHighlighting"] == "true") ? true : false);
    graphicsWidget->setEdgeArrowSize(appSettings["initEdgeArrowSize"].toInt(nullptr, 10));
    graphicsWidget->setEdgesBezier(appSettings["initEdgeShape"] == "bezier");
    drawEdgesBezier->setChecked(appSettings["initEdgeShape"] == "bezier");

    if (appSettings["initBackgroundImage"] != "" && QFileInfo::exists(appSettings["initBackgroundImage"]))
    {
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage(tr("BackgroundImage on."));
    }
    else
    {
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
    }

    slotOptionsCanvasIndexMethod(appSettings["canvasIndexMethod"]);

    /** Clear Chart */
    miniChart->resetToTrivial();

    /** Clear LCDs **/
    qCDebug(lcMainWindow) << "### Clearing Statistics panel LCDs. Please wait...";

    rightPanelClickedNodeLCD->setText("-");
    rightPanelClickedNodeInDegreeLabel->setVisible(false);
    rightPanelClickedNodeInDegreeLCD->setVisible(false);
    rightPanelClickedNodeOutDegreeLabel->setVisible(false);
    rightPanelClickedNodeOutDegreeLCD->setVisible(false);
    rightPanelClickedEdgeNameLCD->setText("-");
    rightPanelClickedEdgeWeightLabel->setVisible(false);
    rightPanelClickedEdgeWeightLCD->setVisible(false);
    rightPanelClickedEdgeReciprocalWeightLabel->setVisible(false);
    rightPanelClickedEdgeReciprocalWeightLCD->setVisible(false);

    /** Clear toolbox and menu checkboxes **/
    qCDebug(lcMainWindow) << "### Resetting toolbox. Please wait...";
    toolBoxEditEdgeTransformSelect->setCurrentIndex(0);
    toolBoxEditEdgeModeSelect->setCurrentIndex(0);

    initComboBoxes();

    toolBoxLayoutByIndexSelect->blockSignals(true);
    toolBoxLayoutByIndexSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexSelect->blockSignals(false);
    toolBoxLayoutByIndexTypeSelect->blockSignals(true);
    toolBoxLayoutByIndexTypeSelect->setCurrentIndex(0);
    toolBoxLayoutByIndexTypeSelect->blockSignals(false);
    toolBoxLayoutForceDirectedSelect->blockSignals(true);
    toolBoxLayoutForceDirectedSelect->setCurrentIndex(0);
    toolBoxLayoutForceDirectedSelect->blockSignals(false);

    optionsEdgeWeightNumbersAct->setChecked(
        (appSettings["initEdgeWeightNumbersVisibility"] == "true") ? true : false);
    optionsEdgeWeightConsiderAct->setChecked(false);

    optionsEdgeArrowsAct->setChecked(
        (appSettings["initEdgeArrows"] == "true") ? true : false);

    optionsEdgeLabelsAct->setChecked(
        (appSettings["initEdgeLabelsVisibility"] == "true") ? true : false);
    editFilterNodesIsolatesAct->setChecked(false); // re-init orphan nodes menu item

    editFilterEdgesUnilateralAct->setChecked(false);

    // editRelationChangeCombo->clear();

    qCDebug(lcMainWindow) << "### Clearing textEditors. Current count: " << m_textEditors.size() << "textEditors";
    foreach (TextEditor *ed, m_textEditors)
    {
        ed->close();
        delete ed;
    }
    m_textEditors.clear();

    QApplication::restoreOverrideCursor();
    // Do it again, to catch any older overriden cursor
    QApplication::restoreOverrideCursor();

    setCursor(Qt::ArrowCursor);

    setWindowTitle("SocNetV");

    filterNodesRestoreAllAct->setEnabled(false);
    editSubgraphExtractFromSelectionAct->setEnabled(false);
    m_filterChips.clear();
    m_filterBar->clearAllChips();

    // Always reset the data table so it shows an empty graph when next opened
    if (m_tableWidget)
        m_tableWidget->refresh(activeGraph);

    statusMessage(tr("Ready"));

    qCDebug(lcMainWindow) << "#### APP INITIALISATION FINISHED, ON THREAD" << thread();
}

/**
 * @brief Initializes combo boxes in the MW
 */
void MainWindow::initComboBoxes()
{
    toolBoxAnalysisCommunitiesSelect->setCurrentIndex(0);
    toolBoxAnalysisStrEquivalenceSelect->setCurrentIndex(0);
    toolBoxAnalysisCohesionSelect->setCurrentIndex(0);
    toolBoxAnalysisProminenceSelect->setCurrentIndex(0);
    toolBoxAnalysisMatricesSelect->setCurrentIndex(0);
    toolBoxNetworkAutoCreateSelect->setCurrentIndex(0);
    toolBoxEditNodeSubgraphSelect->setCurrentIndex(0);
}

/**
 * @brief Setup a list of all text codecs supported by OS
 */
void MainWindow::initNetworkAvailableTextCodecs()
{
    qCDebug(lcMainWindow) << "Checking which text codecs are supported and storing them to a list";
    QMap<QString, QTextCodec *> codecMap;
    QRegularExpression iso8859RegExp("ISO[- ]8859-([0-9]+).*");
    QRegularExpressionMatch match;

    foreach (int mib, QTextCodec::availableMibs())
    {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

        //    // FOR FUTURE REFERENCE (IF QTextCodec Class GETS REMOVED FROM QT6 QT5 CORE COMPAT MODULE)
        // Verify that Codec/Encoding is supported by QStringConverter,
        // Otherwise skip it.
        //        std::optional<QStringConverter::Encoding> test_support = QStringConverter::encodingForName(codec->name());
        //        if ( ! test_support.has_value()) {
        //            continue;
        //        }

        QString sortKey = codec->name().toUpper();
        match = iso8859RegExp.match(sortKey);

        int rank;

        if (sortKey.startsWith("UTF-8"))
        {
            rank = 1;
        }
        else if (sortKey.startsWith("UTF-16"))
        {
            rank = 2;
        }
        else if (match.hasMatch())
        {
            if (match.captured(1).size() == 1)
                rank = 3;
            else
                rank = 4;
        }
        else
        {
            rank = 5;
        }
        sortKey.prepend(QChar('0' + rank));

        codecMap.insert(sortKey, codec);
    }
    codecs = codecMap.values();
}
