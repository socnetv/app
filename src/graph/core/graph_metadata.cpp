/**
 * @file graph_metadata.cpp
 * @brief Implements graph metadata and modification state methods for the Graph
 *        class (name/filename/format, modified/loaded/saved flags).
 * @author Dimitris B. Kalamaras
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

#include "graph.h"

/**
 * @brief Returns the name of the current graph
 *
 * If graph name is empty, then returns current relation name.
 * If no relation exists, returns "noname"
 *
 * @return QString
 */
QString Graph::getName() const
{
    if (m_graphName.isEmpty())
    {
        if (!(relationCurrentName().isEmpty()))
        {
            return relationCurrentName();
        }
        else
        {
            return "noname";
        }
    }
    return m_graphName;
}

/**
 * @brief Sets the name of the current graph
 *
 * @param graphName
 */
void Graph::setName(const QString &graphName)
{
    qDebug() << "Setting graph name to:" << graphName;
    m_graphName = graphName;
}

/**
 * @brief Returns the file name of the current graph, if any.
 *
 * @return QString
 */
QString Graph::getFileName() const
{
    return m_fileName;
}

/**
 * @brief Sets the file name of the current graph
 *
 * @param fileName
 */
void Graph::setFileName(const QString &fileName)
{
    qDebug() << "Setting graph filename to:" << fileName;
    m_fileName = fileName;
}

/**
 * @brief Returns the format of the last file opened
 *
 * @return int
 */
int Graph::getFileFormat() const
{
    return m_fileFormat;
}

void Graph::setFileFormat(const int &fileFormat)
{
    qDebug() << "Setting graph file format to:" << fileFormat;
    m_fileFormat = fileFormat;
}

/**
 * @brief Returns true if the fileFormat is supported for saving
 * @param fileFormat
 * @return
 */
bool Graph::isFileFormatExportSupported(const int &fileFormat) const
{
    if (m_graphFileFormatExportSupported.contains(fileFormat))
    {
        return true;
    }
    return false;
}

/**
 * @brief Sets the graph modification status.
 *
 * If there are major changes or new network, it signals to MW to update the UI.
 *
 * @param int graphNewStatus
 * @param bool signalMW
 */
void Graph::setModStatus(const int &graphNewStatus, const bool &signalMW)
{

    if (m_graphModStatus == ModStatus::NewNet && isEmpty())
    {
        // New network, no vertices. Don't change status.

        qDebug() << "This is a empty new network. Will not change status.";

        emit signalGraphModified(isDirected(),
                                 0,
                                 0,
                                 0,
                                 false);

        return;
    }

    else if (graphNewStatus == ModStatus::NewNet)
    {

        qDebug() << "This is a new network. Setting graph as new...";

        m_graphModStatus = graphNewStatus;

        emit signalGraphModified(isDirected(),
                                 m_totalVertices,
                                 edgesEnabled(),
                                 graphDensity(),
                                 false);

        return;
    }
    else if (graphNewStatus == ModStatus::SavedUnchanged)
    {

        // this is called after loading or saving a file

        qDebug() << "Setting graph as saved/unchanged...";

        m_graphModStatus = graphNewStatus;

        emit signalGraphSavedStatus(true);

        return;
    }
    else if (graphNewStatus > ModStatus::MajorChanges)
    {

        // This is called from any method that alters the graph structure,
        // thus all prior computations are invalidated

        //        qDebug()<<"Major changes, invalidating computations, setting graph as changed...";

        m_graphModStatus = graphNewStatus;

        // Init all calculated* flags to false,
        // to force all relevant methods to recompute
        calculatedGraphReciprocity = false;
        calculatedGraphSymmetry = false;
        calculatedGraphWeighted = false;
        calculatedGraphDensity = false;
        calculatedEdges = false;
        calculatedVertices = false;
        calculatedVerticesList = false;
        calculatedVerticesSet = false;
        calculatedIsolates = false;
        calculatedTriad = false;
        calculatedAdjacencyMatrix = false;
        calculatedDistances = false;
        calculatedCentralities = false;
        calculatedDP = false;
        calculatedDC = false;
        calculatedPP = false;
        calculatedIRCC = false;
        calculatedIC = false;
        calculatedEVC = false;
        calculatedPRP = false;

        if (signalMW)
        {

            //            qDebug() << "signaling to MW that the graph is modified...";

            emit signalGraphModified(isDirected(),
                                     m_totalVertices,
                                     edgesEnabled(),
                                     graphDensity(),
                                     true);
            return;
        }
    }
    else if (graphNewStatus > ModStatus::MinorOptions)
    {

        // this is called from Graph methods that inflict minor changes,
        // i.e. changing vertex positions, labels, etc

        if (m_graphModStatus < ModStatus::MajorChanges)
        {
            //  Do not change status if current status is > MajorChanges
            m_graphModStatus = graphNewStatus;
        }
        //        qDebug()<<"minor changes but needs saving...";
        emit signalGraphSavedStatus(false);
        return;
    }
    else
    {
        qCritical() << "Strange. I should not reach this code...";
        m_graphModStatus = graphNewStatus;
    }
}

/**
 * @brief Returns true of graph is modified (edges/vertices added/removed)
 * @return
 */
bool Graph::isModified() const
{
    if (m_graphModStatus > ModStatus::MajorChanges)
    {
        qDebug() << "Graph::isModified() - isModified: true";
        return true;
    }
    qDebug() << "Graph::isModified() - isModified: false";
    return false;
}

/**
 * @brief Returns true if a graph has been loaded from a file.
 * @return
 */
bool Graph::isLoaded() const
{
    if (!getFileName().isEmpty() && getFileFormat() != FileType::UNRECOGNIZED)
    {
        qDebug() << "isLoaded: true ";
        return true;
    }
    qDebug() << "isLoaded: false ";
    return false;
}

/**
 * @brief Returns true if the graph is saved.
 * @return
 */
bool Graph::isSaved() const
{
    if (m_graphModStatus == ModStatus::NewNet)
    {
        qDebug() << "isSaved: true (new net)";
        return true;
    }
    else if (m_graphModStatus == ModStatus::SavedUnchanged)
    {
        qDebug() << "isSaved: true";
        return true;
    }
    qDebug() << "isSaved: false";
    return false;
}
