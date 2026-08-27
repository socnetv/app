/**
 * @file mainwindow_init_settings.cpp
 * @brief Implements MainWindow settings load/save and last-used-path bookkeeping.
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
#include "forms/dialogpreviewfile.h"

#include <QtWidgets>
#include <QtCharts>
#include <QTextCodec>

/**
 * @brief Reads user-defined settings (or uses defaults) and initializes some app settings
 */
QMap<QString, QString> MainWindow::initSettings(const int &debugLevel, const bool &forceProgress)
{

    qCDebug(lcMainWindow) << "Initializing settings - debugLevel" << debugLevel;

    //
    // Read used-defined settings or use defaults
    //

    // App settings are always saved to this folder.
    settingsDir = QDir::homePath() + QDir::separator() + "socnetv-data" + QDir::separator();
    settingsFilePath = settingsDir + "settings.conf";

    // dataDir is where our built-in datasets and reports are saved by default
    // initially dataDir and settingsDir are the same, but dataDir may be
    // changed by the user through Settings...
    QString dataDir = settingsDir;

    // hard-coded default settings to use only on first app load,
    // when there are no user defined values
    appSettings["initNodesEstimatedSize"] = "5000";
    appSettings["initEdgesPerNodeEstimatedSize"] = "500";
    appSettings["initNodeSize"] = "10";
    appSettings["initNodeColor"] = "red";
    appSettings["initNodeShape"] = "circle";
    appSettings["initNodeIconPath"] = "";

    appSettings["initNodeNumbersVisibility"] = "true";
    appSettings["initNodeNumberSize"] = "0";
    appSettings["initNodeNumberColor"] = "#333";
    appSettings["initNodeNumbersInside"] = "true";
    appSettings["initNodeNumberDistance"] = "2";

    appSettings["initNodeLabelsVisibility"] = "false";
    appSettings["initNodeLabelSize"] = "8";
    appSettings["initNodeLabelColor"] = "#8d8d8d";
    appSettings["initNodeLabelDistance"] = "6";

    appSettings["initEdgesVisibility"] = "true";
    appSettings["initEdgeShape"] = "line"; // bezier
    appSettings["initEdgeColor"] = "#666666";
    appSettings["initEdgeColorNegative"] = "red";
    appSettings["initEdgeColorZero"] = "blue";
    appSettings["showZeroWeightEdges"] = "true"; // #30: show by default; user can disable in Settings
    appSettings["initEdgeArrows"] = "true";
    appSettings["initEdgeArrowSize"] = "6";
    appSettings["initEdgeOffsetFromNode"] = "7";
    appSettings["initEdgeThicknessPerWeight"] = "true";
    appSettings["initEdgeWeightNumbersVisibility"] = "false";
    appSettings["initEdgeWeightNumberSize"] = "7";
    appSettings["initEdgeWeightNumberColor"] = "#00aa00";
    appSettings["initEdgeLabelsVisibility"] = "false";

    appSettings["initBackgroundColor"] = "white"; //"gainsboro";
    appSettings["initBackgroundImage"] = "";
    appSettings["printDebug"] = "false";
    appSettings["viewReportsInSystemBrowser"] = "true";
    appSettings["showProgressBar"] = "true";
    appSettings["showToolBar"] = "true";
    appSettings["showStatusBar"] = "true";
    appSettings["useCustomStyleSheet"] = "true";
    appSettings["opengl"] = "true";
    appSettings["antialiasing"] = "true";
    appSettings["canvasAntialiasingAutoAdjustment"] = "true";
    appSettings["canvasSmoothPixmapTransform"] = "true";
    appSettings["canvasPainterStateSave"] = "false";
    appSettings["canvasCacheBackground"] = "false";
    appSettings["canvasUpdateMode"] = "Full";
    appSettings["canvasIndexMethod"] = "NoIndex";
    appSettings["canvasEdgeHighlighting"] = "true";
    appSettings["canvasNodeHighlighting"] = "true";
    appSettings["dataDir"] = dataDir;
    appSettings["lastUsedDirPath"] = dataDir;
    appSettings["showRightPanel"] = "true";
    appSettings["showLeftPanel"] = "true";
    appSettings["printLogo"] = "true";
    appSettings["initStatusBarDuration"] = "5000";
    appSettings["randomErdosEdgeProbability"] = "0.04";
    appSettings["initReportsRealNumberPrecision"] = "6";
    appSettings["initReportsLabelsLength"] = "16";
    appSettings["initReportsChartType"] = "0";
    appSettings["initReportsOutputFormat"] = "0";

    appSettings["saveZeroWeightEdges"] = "false";

    // Try to load settings from previously-saved file
    // First check if our settings folder exist
    QDir socnetvDir(settingsDir);
    if (!socnetvDir.exists())
    {
        qCDebug(lcMainWindow) << "socnetv settings dir does not exist. Creating it...";
        socnetvDir.mkdir(settingsDir);
    }
    // Then check if the settings file exists inside the folder
    if (!socnetvDir.exists(settingsFilePath))
    {
        qCDebug(lcMainWindow) << "Settings file does not exist. Creating it with defaults at: "
                 << settingsFilePath;
        saveSettings();
    }
    else
    {
        qCDebug(lcMainWindow) << "Settings file exist. Reading it...";
        QFile file(settingsFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qCDebug(lcMainWindow) << "Could not open (for reading) file:" << settingsFilePath;
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error loading settings file"),
                                  tr("Error loading settings"),
                                  tr("Error! \n"
                                     "I cannot read the settings file "
                                     "in \n %1 \n"
                                     "You can continue using SocNetV with default "
                                     "settings but any changes to them will not "
                                     " be saved for future sessions \n"
                                     "Please, check permissions in your home folder "
                                     " and contact the developer team.")
                                      .arg(settingsFilePath.toLocal8Bit()));
            return appSettings;
        }
        // Read the previously-stored settings from the file and update appSettings
        QTextStream in(&file);
        QStringList setting;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (!line.isEmpty())
            {
                setting = line.simplified().split('=');
                if (setting[0].simplified().startsWith("recentFile_"))
                {
                    recentFiles += setting[1].simplified();
                }
                else
                {
                    appSettings.insert(setting[0].simplified(), setting[1].simplified());
                }
            }
        }
        file.close();

        // Migration 2: switch old BspTreeIndex default → NoIndex (better for large dynamic scenes)
        if (appSettings.value("settingsMigration", "0").toInt() < 2)
        {
            if (appSettings["canvasIndexMethod"] == "BspTreeIndex")
                appSettings["canvasIndexMethod"] = "NoIndex";
            appSettings["settingsMigration"] = "2";
        }

        // Migration 3: force-enable OpenGL once for existing users with a stale "opengl = false"
        // setting. setOptionsOpenGL() already no-ops safely if the Qt build has no OpenGL support,
        // so this is safe even when OpenGL isn't actually available. Users can still turn it back
        // off afterward; this only overrides the setting once.
        if (appSettings.value("settingsMigration", "0").toInt() < 3)
        {
            appSettings["opengl"] = "true";
            appSettings["settingsMigration"] = "3";
        }
    }

    // Override progress bar setting if the user has requested it (through a command-line parameter)
    appSettings["showProgressBar"] = forceProgress ? "true" : appSettings["showProgressBar"];

    // Override debug messages setting if the user has requested it (through a command-line parameter)
    if (debugLevel > 0)
    {
        appSettings["printDebug"] = "true";
    }
    else
    {
        // debugLevel == 0 (-d 0) or -1 (no -d flag at all): WS14 L1 -- a no-flag launch must
        // come up quiet regardless of whatever "print debug messages" persisted to
        // settings.conf from a previous Settings-dialog session. Without this, a single past
        // toggle-on silently makes every future no-flag launch pay the full qDebug() cost
        // forever, which is exactly the release-build-hygiene bug this milestone exists to
        // close. The Settings dialog checkbox still works fine as a live, in-session toggle.
        appSettings["printDebug"] = "false";
        slotOptionsDebugMessages(false);
    }

    if (appSettings["printDebug"] == "true")
    {
        slotOptionsDebugMessages(true);
    }
    else
    {
        slotOptionsDebugMessages(false);
    }

    //
    // Create fortune cookies and tips
    //
    createFortuneCookies();
    slotHelpCreateTips();

    //
    // Populate icons and shapes lists
    //
    // Note: When you add a new shape and icon, you must also:
    // 1. Add a new enum in NodeShape (global.h)
    // 2. Add a new branch in GraphicsNode::setShape() and paint()
    // 3. Add a new branch in DialogNodeEdit: getNodeShape() and getUserChoices()
    nodeShapeList << "box"
                  << "circle"
                  << "diamond"
                  << "ellipse"
                  << "triangle"
                  << "star"
                  << "person"
                  << "person-b"
                  << "bugs"
                  << "heart"
                  << "dice"
                  << "custom";

    iconPathList << ":/images/box.png"
                 << ":/images/circle.png"
                 << ":/images/diamond.png"
                 << ":/images/ellipse.png"
                 << ":/images/triangle.png"
                 << ":/images/star.png"
                 << ":/images/person.svg"
                 << ":/images/person-bw.svg"
                 << ":/images/bugs.png"
                 << ":/images/heart.svg"
                 << ":/images/random.png"
                 << ":/images/export_photo_48px.svg";

    // Max nodes used by createRandomNetwork dialogues
    maxRandomlyCreatedNodes = 5000;

    //
    // Initialize list of supported text codecs and prepare the preview file dialog
    //
    qCDebug(lcMainWindow) << "initializing text codecs list..";
    initNetworkAvailableTextCodecs();

    qCDebug(lcMainWindow) << "creating preview file dialog and passing the codecs list: " << codecs;
    m_dialogPreviewFile = new DialogPreviewFile(this);
    m_dialogPreviewFile->setCodecList(codecs);

    connect(m_dialogPreviewFile, &DialogPreviewFile::loadNetworkFileWithCodec,
            this, &MainWindow::slotNetworkFileLoad);

    // return the setting
    return appSettings;
}

/**
 * @brief Saves default (or user-defined) app settings
 */
void MainWindow::saveSettings()
{
    qCDebug(lcMainWindow) << "Saving app settings to file: " << settingsFilePath;
    QFile file(settingsFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCDebug(lcMainWindow) << "Could not open (for writing) file:" << settingsFilePath;
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Error writing settings file"),
                              tr("Error writing settings"),
                              tr("I cannot write the settings file "
                                 "in \n %1 \n"
                                 "You can continue using SocNetV with default "
                                 "settings but any changes to them will not "
                                 " be saved for future sessions \n"
                                 "Please, check permissions in your home folder "
                                 " and contact the developer team.")
                                  .arg(settingsFilePath.toLocal8Bit()));
        return;
    }

    QTextStream out(&file);
    qCDebug(lcMainWindow) << "Writing settings to settings file first ";
    QMap<QString, QString>::const_iterator it = appSettings.constBegin();
    while (it != appSettings.constEnd())
    {
        out << it.key() << " = " << it.value() << "\n";
        ++it;
    }

    // save recent files
    for (int i = 0; i < recentFiles.size(); ++i)
    {
        out << "recentFile_" + QString::number(i + 1)
            << " = "
            << recentFiles.at(i) << "\n";
    }

    file.close();
}

/**
 * @brief Returns the last path used by user to open/save something
 */
QString MainWindow::getLastPath()
{
    if (appSettings["lastUsedDirPath"] == "socnetv-initial-none")
    {
        appSettings["lastUsedDirPath"] = appSettings["dataDir"];
    }
    qCDebug(lcMainWindow) << "Last path used: " << appSettings["lastUsedDirPath"];
    return appSettings["lastUsedDirPath"];
}

/**
 * @brief Sets the last path used by user to open/save a network and adds the file
 * to recent files...
 * @param filePath
 */
void MainWindow::setLastPath(const QString &filePath)
{
    qCDebug(lcMainWindow) << "Setting last path and adding to recent files:" << filePath;
    QString currentPath = QFileInfo(filePath).dir().absolutePath();
    QDir::setCurrent(currentPath);
    appSettings["lastUsedDirPath"] = currentPath;

    if (!QFileInfo(filePath).completeSuffix().toLower().contains("bmp") &&
        !QFileInfo(filePath).completeSuffix().toLower().contains("jpg") &&
        !QFileInfo(filePath).completeSuffix().toLower().contains("png") &&
        !QFileInfo(filePath).completeSuffix().toLower().contains("pdf"))
    {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > MaxRecentFiles)
            recentFiles.removeLast();
    }
    slotNetworkFileRecentUpdateActions();
    saveSettings();
}
