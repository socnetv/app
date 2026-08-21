/**
 * @file mainwindow_options.cpp
 * @brief Implements MainWindow Options/Settings menu: the Settings dialog launcher and every node/edge/canvas/window appearance toggle it wires.
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
#include "graphicsedge.h"
#include "texteditor.h"
#include "forms/dialogsettings.h"

#include <QtWidgets>

/**
 * @brief Opens the Settings dialog
 */
void MainWindow::slotOpenSettingsDialog()
{

    // build dialog

    m_settingsDialog = new DialogSettings(appSettings, nodeShapeList, iconPathList, this);

    connect(m_settingsDialog, &DialogSettings::saveSettings,
            this, &MainWindow::saveSettings);

    connect(m_settingsDialog, &DialogSettings::setReportsDataDir,
            activeGraph, &Graph::setReportsDataDir);

    connect(m_settingsDialog, &DialogSettings::setReportsRealNumberPrecision,
            activeGraph, &Graph::setReportsRealNumberPrecision);

    connect(m_settingsDialog, &DialogSettings::setReportsLabelLength,
            activeGraph, &Graph::setReportsLabelLength);

    connect(m_settingsDialog, &DialogSettings::setReportsChartType,
            activeGraph, &Graph::setReportsChartType);

    connect(m_settingsDialog, &DialogSettings::setReportsOutputFormat,
            activeGraph, &Graph::setReportsOutputFormat);

    connect(m_settingsDialog, &DialogSettings::setDebugMsgs,
            this, &MainWindow::slotOptionsDebugMessages);

    connect(m_settingsDialog, &DialogSettings::setProgressDialog,
            this, &MainWindow::slotOptionsProgressDialogVisibility);

    connect(m_settingsDialog, &DialogSettings::setPrintLogo,
            this, &MainWindow::slotOptionsEmbedLogoExporting);

    connect(m_settingsDialog, &DialogSettings::setCustomStylesheet,
            this, &MainWindow::slotOptionsCustomStylesheet);

    connect(m_settingsDialog, &DialogSettings::setToolBar,
            this, &MainWindow::slotOptionsWindowToolbarVisibility);

    connect(m_settingsDialog, &DialogSettings::setStatusBar,
            this, &MainWindow::slotOptionsWindowStatusbarVisibility);

    connect(m_settingsDialog, &DialogSettings::setLeftPanel,
            this, &MainWindow::slotOptionsWindowLeftPanelVisibility);

    connect(m_settingsDialog, &DialogSettings::setRightPanel,
            this, &MainWindow::slotOptionsWindowRightPanelVisibility);

    connect(m_settingsDialog, &DialogSettings::setCanvasBgColor,
            this, &MainWindow::slotOptionsBackgroundColor);

    connect(m_settingsDialog, &DialogSettings::setCanvasBgImage,
            this, &MainWindow::slotOptionsBackgroundImage);

    connect(m_settingsDialog, &DialogSettings::setCanvasOpenGL,
            this, &MainWindow::slotOptionsCanvasOpenGL);

    connect(m_settingsDialog, &DialogSettings::setCanvasAntialiasing,
            this, &MainWindow::slotOptionsCanvasAntialiasing);

    connect(m_settingsDialog, &DialogSettings::setCanvasAntialiasingAutoAdjust,
            this, &MainWindow::slotOptionsCanvasAntialiasingAutoAdjust);

    connect(m_settingsDialog, &DialogSettings::setCanvasSmoothPixmapTransform,
            this, &MainWindow::slotOptionsCanvasSmoothPixmapTransform);

    connect(m_settingsDialog, &DialogSettings::setCanvasSavePainterState,
            this, &MainWindow::slotOptionsCanvasSavePainterState);

    connect(m_settingsDialog, &DialogSettings::setCanvasCacheBackground,
            this, &MainWindow::slotOptionsCanvasCacheBackground);

    connect(m_settingsDialog, &DialogSettings::setCanvasEdgeHighlighting,
            this, &MainWindow::slotOptionsCanvasEdgeHighlighting);

    connect(m_settingsDialog, &DialogSettings::setCanvasUpdateMode,
            this, &MainWindow::slotOptionsCanvasUpdateMode);

    connect(m_settingsDialog, &DialogSettings::setCanvasIndexMethod,
            this, &MainWindow::slotOptionsCanvasIndexMethod);

    connect(m_settingsDialog, SIGNAL(setNodeColor(QColor)),
            this, SLOT(slotEditNodeColorAll(QColor)));

    connect(m_settingsDialog, &DialogSettings::setNodeShape,
            this, &MainWindow::slotEditNodeShape);

    connect(m_settingsDialog, &DialogSettings::setNodeSize,
            this, &MainWindow::slotEditNodeSizeAll);

    connect(m_settingsDialog, &DialogSettings::setNodeNumbersVisibility,
            this, &MainWindow::slotOptionsNodeNumbersVisibility);

    connect(m_settingsDialog, &DialogSettings::setNodeNumbersInside,
            this, &MainWindow::slotOptionsNodeNumbersInside);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberColor,
            this, &MainWindow::slotEditNodeNumbersColor);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberSize,
            this, &MainWindow::slotEditNodeNumberSize);

    connect(m_settingsDialog, &DialogSettings::setNodeNumberDistance,
            this, &MainWindow::slotEditNodeNumberDistance);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelsVisibility,
            this, &MainWindow::slotOptionsNodeLabelsVisibility);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelSize,
            this, &MainWindow::slotEditNodeLabelSize);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelColor,
            this, &MainWindow::slotEditNodeLabelsColor);

    connect(m_settingsDialog, &DialogSettings::setNodeLabelDistance,
            this, &MainWindow::slotEditNodeLabelDistance);

    connect(m_settingsDialog, &DialogSettings::setEdgesVisibility,
            this, &MainWindow::slotOptionsEdgesVisibility);

    connect(m_settingsDialog, &DialogSettings::setEdgesBezier,
            this, &MainWindow::slotOptionsEdgesBezier);

    connect(m_settingsDialog, &DialogSettings::setEdgeArrowsVisibility,
            this, &MainWindow::slotOptionsEdgeArrowsVisibility);
    connect(m_settingsDialog, &DialogSettings::setEdgeArrowSize,
            this, &MainWindow::slotOptionsEdgeArrowSize);

    connect(m_settingsDialog, &DialogSettings::setEdgeOffsetFromNode,
            this, &MainWindow::slotOptionsEdgeOffsetFromNode);

    connect(m_settingsDialog, &DialogSettings::setEdgeColor,
            this, &MainWindow::slotEditEdgeColorAll);

    connect(m_settingsDialog, &DialogSettings::setEdgeWeightNumbersVisibility,
            this, &MainWindow::slotOptionsEdgeWeightNumbersVisibility);

    connect(m_settingsDialog, &DialogSettings::setEdgeLabelsVisibility,
            this, &MainWindow::slotOptionsEdgeLabelsVisibility);

    connect(m_settingsDialog, &DialogSettings::setSaveZeroWeightEdges,
            this, &MainWindow::slotOptionsSaveZeroWeightEdges);

    connect(m_settingsDialog, &DialogSettings::setShowZeroWeightEdges, // #30
            this, &MainWindow::slotOptionsShowZeroWeightEdges);

    // show settings dialog
    m_settingsDialog->exec();
}

/**
 * @brief Toggles visibility of node numbers.
 * Persists the choice in appSettings["initNodeNumbersVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeNumbersVisibility()" << toggle;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Nodes Numbers. Please wait..."));
    appSettings["initNodeNumbersVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setNodeNumberVisibility(toggle);
    optionsNodeNumbersVisibilityAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Node Numbers are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Node Numbers are visible again..."));
    }
    QApplication::restoreOverrideCursor();
    return;
}

/**
 * @brief Toggles whether node numbers are drawn inside or outside nodes.
 * Shows node numbers first if they are currently hidden.
 * Persists the choice in appSettings["initNodeNumbersInside"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeNumbersInside(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeNumbersInside()" << toggle;

    statusMessage(tr("Toggle Numbers inside nodes. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    // if node numbers are hidden, show them first.
    if (toggle && appSettings["initNodeNumbersVisibility"] != "true")
        slotOptionsNodeNumbersVisibility(true);

    appSettings["initNodeNumbersInside"] = (toggle) ? "true" : "false";
    graphicsWidget->setNumbersInsideNodes(toggle);
    optionsNodeNumbersVisibilityAct->setChecked(toggle);

    if (toggle)
    {
        statusMessage(tr("Numbers inside nodes..."));
    }
    else
    {
        statusMessage(tr("Numbers outside nodes..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of node labels.
 * Persists the choice in appSettings["initNodeLabelsVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsNodeLabelsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsNodeLabelsVisibility()" << toggle;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    statusMessage(tr("Toggle Nodes Labels. Please wait..."));

    appSettings["initNodeLabelsVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setNodeLabelsVisibility(toggle);
    optionsNodeLabelsVisibilityAct->setChecked(toggle);

    if (!toggle)
    {
        statusMessage(tr("Node Labels are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Node Labels are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of all edges.
 * Persists the choice in appSettings["initEdgesVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgesVisibility(bool toggle)
{
    if (!activeEdges())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges. Please wait..."));
    appSettings["initEdgesVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setAllItemsVisibility(TypeEdge, toggle);
    if (!toggle)
    {
        statusMessage(tr("Edges are invisible now. Click again the same menu to display them."));
    }
    else
    {
        statusMessage(tr("Edges visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of directional arrows on edges.
 * Persists the choice in appSettings["initEdgeArrows"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeArrowsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "Request to toggle edges arrows to:" << toggle;

    statusMessage(tr("Toggling Edges' Arrows. Please wait..."));
    appSettings["initEdgeArrows"] = (toggle) ? "true" : "false";

    graphicsWidget->setEdgeArrowsVisibility(toggle);
    if (toggle)
    {
        statusMessage(tr("Arrows in edges: on."));
    }
    else
    {
        statusMessage(tr("Arrows in edges: off."));
    }
}

/**
 * @brief Applies a new arrow size to all edges and persists the setting.
 * @param size  Arrow size in pixels (2–20).
 */
void MainWindow::slotOptionsEdgeArrowSize(const int &size)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeArrowSize - new size" << size;
    appSettings["initEdgeArrowSize"] = QString::number(size);
    graphicsWidget->setEdgeArrowSize(size);
    statusMessage(tr("Changed edge arrow size to %1.").arg(size));
}

/**
 * @brief Toggles edge weights during computations
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightsDuringComputation(bool toggle)
{
    askedAboutWeights = false;
    askAboutEdgeWeights(toggle);
    activeGraph->setModStatus(activeGraph->ModStatus::EdgeCount);
}

/**
 * @brief Toggles drawing edges as Bezier curves or straight lines.
 * Persists the choice in appSettings["initEdgeShape"] ("bezier" or "line").
 * @param toggle
 */
void MainWindow::slotOptionsEdgesBezier(bool toggle)
{
    if (!activeNodes())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL_NO_NETWORK);
        return;
    }
    statusMessage(tr("Toggle edges bezier. Please wait..."));
    appSettings["initEdgeShape"] = toggle ? "bezier" : "line";
    graphicsWidget->setEdgesBezier(toggle);
    statusMessage(tr("Edges drawn as %1.").arg(toggle ? tr("Bezier curves") : tr("straight lines")));
}

/**
 * @brief MainWindow::slotOptionsEdgeThicknessPerWeight
 * @param toggle
 */
void MainWindow::slotOptionsEdgeThicknessPerWeight(bool toggle)
{
    if (toggle)
    {
    }
    else
    {
    }
}

/**
 * @brief Changes the distance of edge arrows from nodes
 * Called from Edit menu option and DialogSettings
 * if offset=0, asks the user to enter a new offset
 * if v1=0 and v2=0, it changes all edges
 * @param v1
 * @param v2
 * @param offset
 */
void MainWindow::slotOptionsEdgeOffsetFromNode(const int &offset, const int &v1, const int &v2)
{
    bool ok = false;
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeOffsetFromNode - new offset " << offset;
    int newOffset = offset;

    if (!newOffset)
    {
        newOffset = QInputDialog::getInt(
            this, "Change edge offset",
            tr("Change all edges offset from their nodes to: (1-16)"),
            appSettings["initNodeLabelDistance"].toInt(0, 10), 1, 16, 1, &ok);
        if (!ok)
        {
            statusMessage(tr("Change edge offset aborted."));
            return;
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (v1 && v2)
    { // change one edge offset only
        graphicsWidget->setEdgeOffsetFromNode(v1, v2, newOffset);
    }
    else
    { // change all
        appSettings["initEdgeOffsetFromNode"] = QString::number(newOffset);
        graphicsWidget->setEdgeOffsetFromNode(v1, v2, newOffset);
    }

    QApplication::restoreOverrideCursor();

    statusMessage(tr("Changed edge offset from nodes."));
}

/**
 * @brief Toggles visibility of edge weight numbers.
 * Persists the choice in appSettings["initEdgeWeightNumbersVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeWeightNumbersVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeWeightNumbersVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges Weights. Please wait..."));
    appSettings["initEdgeWeightNumbersVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setEdgeWeightNumbersVisibility(toggle);
    activeGraph->edgeWeightNumbersVisibilitySet(toggle);
    optionsEdgeWeightNumbersAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Edge weights are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Edge weights are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Toggles visibility of edge labels.
 * Persists the choice in appSettings["initEdgeLabelsVisibility"].
 * @param toggle
 */
void MainWindow::slotOptionsEdgeLabelsVisibility(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsEdgeLabelsVisibility - Toggling Edges Weights";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle Edges Labels. Please wait..."));

    appSettings["initEdgeLabelsVisibility"] = (toggle) ? "true" : "false";
    graphicsWidget->setEdgeLabelsVisibility(toggle);
    activeGraph->edgeLabelsVisibilitySet(toggle);
    optionsEdgeLabelsAct->setChecked(toggle);
    if (!toggle)
    {
        statusMessage(tr("Edge labels are invisible now. "
                         "Click the same option again to display them."));
    }
    else
    {
        statusMessage(tr("Edge labels are visible again..."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns on/off saving zero-edge edge weights (only for GraphML at the moment)
 * @param toggle
 */
void MainWindow::slotOptionsSaveZeroWeightEdges(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsSaveZeroWeightEdges - Toggling saving zero weight edges";
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusMessage(tr("Toggle zero-weight edges saving. Please wait..."));

    appSettings["saveZeroWeightEdges"] = (toggle) ? "true" : "false";

    if (toggle)
    {
        statusMessage(tr("Zero-weight edges will be saved to graphml files. "));
    }
    else
    {
        statusMessage(tr("Zero-weight edges will NOT be saved to graphml files."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns on/off drawing of zero-weight edges on the canvas (#30)
 * @param toggle
 */
void MainWindow::slotOptionsShowZeroWeightEdges(bool toggle)
{
    qCDebug(lcMainWindow) << "MW::slotOptionsShowZeroWeightEdges - toggle:" << toggle;
    appSettings["showZeroWeightEdges"] = (toggle) ? "true" : "false";
    activeGraph->showZeroWeightEdgesSet(toggle);
    statusMessage(toggle
                      ? tr("Zero-weight edges will be drawn on the canvas.")
                      : tr("Zero-weight edges will not be drawn on the canvas."));
}

/**
 * @brief Turns opengl on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasOpenGL(const bool &toggle)
{
    statusMessage(tr("Toggle openGL. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Inform graphicsWidget about the change
    graphicsWidget->setOptionsOpenGL(toggle);

    if (!toggle)
    {
        appSettings["opengl"] = "false";
        statusMessage(tr("Using openGL off."));
    }
    else
    {
        appSettings["opengl"] = "true";
        statusMessage(tr("Using OpenGL on."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns antialiasing on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasing(bool toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage(tr("Toggle anti-aliasing. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    graphicsWidget->setOptionsAntialiasing(toggle);

    if (!toggle)
    {
        appSettings["antialiasing"] = "false";
        statusMessage(tr("Anti-aliasing off."));
    }
    else
    {
        appSettings["antialiasing"] = "true";
        statusMessage(tr("Anti-aliasing on."));
    }
    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns antialiasing auto-adjustment on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasAntialiasingAutoAdjust(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasAntialiasingAutoAdjust() " << toggle;

    statusMessage(tr("Toggle anti-aliasing auto adjust. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    graphicsWidget->setOptionsNoAntialiasingAutoAdjust(toggle);

    if (!toggle)
    {
        appSettings["canvasAntialiasingAutoAdjustment"] = "false";
        statusMessage(tr("Antialiasing auto-adjustment off."));
    }
    else
    {
        appSettings["canvasAntialiasingAutoAdjustment"] = "true";
        statusMessage(tr("Antialiasing auto-adjustment on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns smooth pixmap transformations on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSmoothPixmapTransform(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasSmoothPixmapTransform() " << toggle;

    statusMessage(tr("Toggle smooth pixmap transformations. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "false";
        statusMessage(tr("Smooth pixmap transformations off."));
    }
    else
    {
        graphicsWidget->setRenderHint(QPainter::SmoothPixmapTransform, toggle);
        appSettings["canvasSmoothPixmapTransform"] = "true";
        statusMessage(tr("Smooth pixmap transformations on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns saving painter state on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasSavePainterState(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasSavePainterState() " << toggle;

    statusMessage(tr("Toggle saving painter state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
        appSettings["canvasPainterStateSave"] = "false";
        statusMessage(tr("Saving painter state off."));
    }
    else
    {
        graphicsWidget->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
        appSettings["canvasPainterStateSave"] = "true";
        statusMessage(tr("Saving painter state on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns caching of canvas background on or off
 * @param toggle
 */
void MainWindow::slotOptionsCanvasCacheBackground(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasCacheBackground() " << toggle;

    statusMessage(tr("Toggle canvas background caching state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheNone);
        appSettings["canvasCacheBackground"] = "false";
        statusMessage(tr("Canvas background caching  off."));
    }
    else
    {
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        appSettings["canvasCacheBackground"] = "true";
        statusMessage(tr("Canvas background caching  on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Turns selected edge highlighting
 * @param toggle
 */
void MainWindow::slotOptionsCanvasEdgeHighlighting(const bool &toggle)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasEdgeHighlighting() " << toggle;

    statusMessage(tr("Toggle edge highlighting state. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (!toggle)
    {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "false";
        statusMessage(tr("Edge highlighting off."));
    }
    else
    {
        graphicsWidget->setEdgeHighlighting(toggle);
        appSettings["canvasEdgeHighlighting"] = "true";
        statusMessage(tr("Edge highlighting on."));
    }

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Sets canvas update mode
 * @param toggle
 */
void MainWindow::slotOptionsCanvasUpdateMode(const QString &mode)
{

    qCDebug(lcMainWindow) << "MW::slotOptionsCanvasUpdateMode() " << mode;

    statusMessage(tr("Setting canvas update mode. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (mode == "Full")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
    else if (mode == "Minimal")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }
    else if (mode == "Smart")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    }
    else if (mode == "Bounding Rectangle")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    }
    else if (mode == "None")
    {
        graphicsWidget->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }
    else
    { //
        graphicsWidget->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    }

    appSettings["canvasUpdateMode"] = mode;

    statusMessage(tr("Canvas update mode: ") + mode);

    QApplication::restoreOverrideCursor();
}

/**
 * @brief Changes the indexing method of the graphics scene.
 *
 * Called from Settings dialog.
 *
 * @param method
 */
void MainWindow::slotOptionsCanvasIndexMethod(const QString &method)
{

    qCDebug(lcMainWindow) << "Changing graphics scene index method to:" << method;

    statusMessage(tr("Setting canvas index method. Please wait..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (method == "BspTreeIndex")
    { // Qt default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }
    else if (method == "NoIndex")
    { // for animated scenes
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    }
    else
    { // default
        graphicsWidget->scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }

    appSettings["canvasIndexMethod"] = method;

    statusMessage(tr("Canvas index method: ") + method);

    QApplication::restoreOverrideCursor();
}

/**
 * @brief MainWindow::slotOptionsEmbedLogoExporting
 *
 * @param toggle
 */
void MainWindow::slotOptionsEmbedLogoExporting(bool toggle)
{
    if (!toggle)
    {
        statusMessage(tr("SocNetV logo print off."));
        appSettings["printLogo"] = "false";
    }
    else
    {
        appSettings["printLogo"] = "true";
        statusMessage(tr("SocNetV logo print on."));
    }
}

/**
 * @brief Turns progress dialogs on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsProgressDialogVisibility(bool toggle)
{
    statusMessage(tr("Toggle progressbar..."));
    if (!toggle)
    {
        appSettings["showProgressBar"] = "false";
        statusMessage(tr("Progress bars off."));
    }
    else
    {
        appSettings["showProgressBar"] = "true";
        statusMessage(tr("Progress bars on."));
    }
}

/**
 * @brief
 * Turns debugging messages on or off
 * @param toggle
 */
void MainWindow::slotOptionsDebugMessages(bool toggle)
{
    if (!toggle)
    {
        qCDebug(lcMainWindow) << "Disabling debugging messages";
        appSettings["printDebug"] = "false";
        QLoggingCategory::setFilterRules("default.debug=false\n"
                                         "socnetv.*.debug=false");
        statusMessage(tr("Debug messages off."));
    }
    else
    {
        appSettings["printDebug"] = "true";
        QLoggingCategory::setFilterRules("default.debug=true\n"
                                         "socnetv.*.debug=true");
        qCDebug(lcMainWindow) << "Enabled debugging messages";
        statusMessage(tr("Debug messages on."));
    }
}

/**
 * @brief
 * Called from Options menu and Settings dialog
 * @param color QColor
 */
void MainWindow::slotOptionsBackgroundColor(QColor color)
{

    if (!color.isValid())
    {
        color = QColorDialog::getColor(QColor(appSettings["initBackgroundColor"]),
                                       this,
                                       "Change the background color");
    }
    if (color.isValid())
    {
        appSettings["initBackgroundColor"] = color.name();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
        QApplication::restoreOverrideCursor();
        statusMessage(tr("Background changed."));
    }
    else
    {
        // user pressed Cancel
        statusMessage(tr("Invalid color. "));
    }
}

/**
 * @brief Toggles a custom background image on the canvas.
 * When enabled, opens a file dialog to pick the image.
 * Persists the path in appSettings["initBackgroundImage"].
 * @param toggle
 */
void MainWindow::slotOptionsBackgroundImageSelect(bool toggle)
{
    statusMessage(tr("Toggle BackgroundImage..."));
    QString m_fileName;
    if (toggle == false)
    {
        statusMessage(tr("BackgroundImage off."));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
    }
    else
    {
        m_fileName = QFileDialog::getOpenFileName(
            this, tr("Select one image"), getLastPath(),
            tr("Images (*.png *.jpg *.jpeg);;All (*.*)"));
        if (m_fileName.isNull())
            appSettings["initBackgroundImage"] = "";
        appSettings["initBackgroundImage"] = m_fileName;
        slotOptionsBackgroundImage();
    }
}

/**
 * @brief
 * Enables/disables displaying a user-defined custom image in the background
 * Called from Settings Dialog and
 */
void MainWindow::slotOptionsBackgroundImage()
{
    statusMessage(tr("Toggle BackgroundImage..."));
    if (appSettings["initBackgroundImage"].isEmpty())
    {
        statusMessage(tr("BackgroundImage off."));
        graphicsWidget->setBackgroundBrush(
            QBrush(QColor(appSettings["initBackgroundColor"])));
    }
    else
    {
        setLastPath(appSettings["initBackgroundImage"]);
        graphicsWidget->setBackgroundBrush(QImage(appSettings["initBackgroundImage"]));
        graphicsWidget->setCacheMode(QGraphicsView::CacheBackground);
        statusMessage(tr("BackgroundImage on."));
    }
}

/**
 * @brief Toggles full screen mode (F11)
 * @param toggle
 */
void MainWindow::slotOptionsWindowFullScreen(bool toggle)
{
    if (toggle == false)
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage(tr("Full screen mode off. Press F11 again to enter full screen."));
    }
    else
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        statusMessage(tr("Full screen mode on. Press F11 again to exit full screen."));
    }
}

/**
 * @brief Turns Toolbar on or off
 * @param toggle
 *
 */
void MainWindow::slotOptionsWindowToolbarVisibility(bool toggle)
{
    statusMessage(tr("Toggle toolbar..."));
    if (toggle == false)
    {
        toolBar->hide();
        appSettings["showToolBar"] = "false";
        statusMessage(tr("Toolbar off."));
    }
    else
    {
        toolBar->show();
        appSettings["showToolBar"] = "true";
        statusMessage(tr("Toolbar on."));
    }
}

/**
 * @brief Turns window statusbar on or off
 * @param toggle
 */
void MainWindow::slotOptionsWindowStatusbarVisibility(bool toggle)
{
    statusMessage(tr("Toggle statusbar..."));

    if (toggle == false)
    {
        statusBar()->hide();
        appSettings["showStatusBar"] = "false";
        statusMessage(tr("Status bar off."));
    }
    else
    {
        statusBar()->show();
        appSettings["showStatusBar"] = "true";
        statusMessage(tr("Status bar on."));
    }
}

/**
 * @brief Toggles left panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowLeftPanelVisibility(bool toggle)
{
    statusMessage(tr("Toggle left panel..."));

    if (toggle == false)
    {
        m_leftScrollArea->hide();
        appSettings["showLeftPanel"] = "false";
        statusMessage(tr("Left Panel off."));
    }
    else
    {
        m_leftScrollArea->show();
        appSettings["showLeftPanel"] = "true";
        statusMessage(tr("Left Panel on."));
    }
}

/**
 * @brief Toggles right panel
 * @param toggle
 */
void MainWindow::slotOptionsWindowRightPanelVisibility(bool toggle)
{
    statusMessage(tr("Toggle left panel..."));

    if (toggle == false)
    {
        rightPanel->hide();
        appSettings["showRightPanel"] = "false";
        statusMessage(tr("Right Panel off."));
    }
    else
    {
        rightPanel->show();
        appSettings["showRightPanel"] = "true";
        statusMessage(tr("Right Panel on."));
    }
}

/**
 * @brief Toggles the use of our own Qt StyleSheet
 *
 * The .qss file is defined in project resources
 *
 * @param checked
 */
void MainWindow::slotOptionsCustomStylesheet(const bool checked = true)
{
    if (checked)
    {
        slotStyleSheetByName(":/qss/default.qss");
        appSettings["useCustomStyleSheet"] = "true";
    }
    else
    {
        slotStyleSheetByName("");
        appSettings["useCustomStyleSheet"] = "false";
    }
}

/**
 * @brief Loads a custom Qt StyleSheet (.qss file)
 *
 * If sheetFileName is empty, the app uses platform-specific Qt style
 *
 * @param sheetFileName
 */
void MainWindow::slotStyleSheetByName(const QString &sheetFileName)
{

    qCDebug(lcMainWindow) << "Opening stylesheet file: " << sheetFileName;

    QString styleSheet = "";

    if (!sheetFileName.isEmpty())
    {

        QFile file(sheetFileName);

        if (!file.open(QFile::ReadOnly))
        {
            qCDebug(lcMainWindow) << "Could not open (for reading) file:" << sheetFileName;
            slotHelpMessageToUserError(
                tr("Cannot read stylesheet file %1:\n%2")
                    .arg(sheetFileName)
                    .arg(file.errorString()));
            return;
        }
        styleSheet = QString::fromLatin1(file.readAll());
    }
    qApp->setStyleSheet(styleSheet);
}
