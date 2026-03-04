/**
 * @file parser.cpp
 * @brief Implements the Parser class for reading and interpreting various network data formats, including adjacency matrices and sociomatrices.
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

#include "parser.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QString>
#include <QtDebug> //used for qDebug messages
#include <QPointF>
#include <QTextCodec>
#include <QElapsedTimer>
#include <QXmlStreamReader>

#include "graph.h" //needed for setParent

using namespace std;

Parser::Parser()
{
    qDebug() << "Parser constructor, on thread:" << this->thread();
}

Parser::~Parser()
{
    qDebug() << "**** Parser destructor on thread:" << this->thread()
             << " clearing hashes... ";
    nodeHash.clear();
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    edgesMissingNodesHash.clear();
    edgeMissingNodesList.clear();
    edgeMissingNodesListData.clear();
    firstModeMultiMap.clear();
    secondModeMultiMap.clear();
    if (xml != 0)
    {
        qDebug() << "**** clearing xml reader object ";
        xml->clear();
        delete xml;
        xml = 0;
    }
}

void Parser::setParseSink(SocNetV::IO::IGraphParseSink *sink)
{
    m_parseSink = sink;
}
void Parser::setOwnedParseSink(std::unique_ptr<SocNetV::IO::IGraphParseSink> sink)
{
    m_ownedParseSink = std::move(sink);
    m_parseSink = m_ownedParseSink.get();
}
/**
 * @brief Loads the data of the given network file, and calls the relevant method to parse it.
 *
 * @param fileName
 * @param codecName
 * @param defNodeSize
 * @param defNodeColor
 * @param defNodeShape
 * @param defNodeNumberColor
 * @param defNodeNumberSize
 * @param defNodeLabelColor
 * @param defNodeLabelSize
 * @param defEdgeColor
 * @param width
 * @param height
 * @param format
 * @param sm_mode
 * @param delim
 */
void Parser::load(const QString &fileName,
                  const QString &codecName,
                  const int &defNodeSize,
                  const QString &defNodeColor,
                  const QString &defNodeShape,
                  const QString &defNodeNumberColor,
                  const int &defNodeNumberSize,
                  const QString &defNodeLabelColor,
                  const int &defNodeLabelSize,
                  const QString &defEdgeColor,
                  const int &canvasWidth,
                  const int &canvasHeight,
                  const int &format,
                  const QString &delim,
                  const int &sm_mode,
                  const bool &sm_has_labels)
{

    ParseConfig cfg{
        /* fileName */ fileName,
        /* codecName */ codecName,
        /* fileFormat */ format,
        /* delim */ delim,
        /* sm_mode */ sm_mode,
        /* sm_has_labels */ sm_has_labels,
        /* initNodeSize */ defNodeSize,
        /* initNodeColor */ defNodeColor,
        /* initNodeShape */ defNodeShape,
        /* initNodeNumberColor */ defNodeNumberColor,
        /* initNodeNumberSize */ defNodeNumberSize,
        /* initNodeLabelColor */ defNodeLabelColor,
        /* initNodeLabelSize */ defNodeLabelSize,
        /* initEdgeColor */ defEdgeColor,
        /* gwWidth */ canvasWidth,
        /* gwHeight */ canvasHeight};

    qDebug() << "Parser::load() current thread:" << QThread::currentThread()
             << " affinity thread:" << this->thread()
             << "loading file:" << cfg.fileName
             << "codecName" << cfg.codecName;

    initNodeSize = cfg.initNodeSize;
    initNodeColor = cfg.initNodeColor;
    initNodeShape = cfg.initNodeShape;
    initNodeNumberColor = cfg.initNodeNumberColor;
    initNodeNumberSize = cfg.initNodeNumberSize;
    initNodeLabelColor = cfg.initNodeLabelColor;
    initNodeLabelSize = cfg.initNodeLabelSize;

    initEdgeColor = cfg.initEdgeColor;

    m_textCodecName = cfg.codecName;
    networkName = (cfg.fileName.split("/")).last();
    gwWidth = cfg.gwWidth;
    gwHeight = cfg.gwHeight;
    fileFormat = cfg.fileFormat;
    two_sm_mode = cfg.sm_mode;

    edgeDirType = EdgeType::Directed;
    arrows = true;
    bezier = false;
    randX = 0;
    randY = 0;
    fileLoaded = false;

    if (!cfg.delim.isNull() && !cfg.delim.isEmpty())
    {
        delimiter = cfg.delim;
    }
    else
    {
        delimiter = " ";
    }

    xml = 0;

    qDebug() << "Initial networkName:" << networkName
             << "requested fileFormat: " << fileFormat
             << "delim:" << cfg.delim << "delimiter" << delimiter;

    errorMessage = QString();

    // Start a timer.
    QElapsedTimer computationTimer;
    computationTimer.start();

    // Try to open the file
    qDebug() << "Opening file...";
    QFile file(cfg.fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qint64 elapsedTime = computationTimer.elapsed();
        qDebug() << "Cannot open file" << cfg.fileName;
        errorMessage = tr("Cannot open file: %1").arg(cfg.fileName);
        if (m_parseSink)
        {
            m_parseSink->fileLoaded(FileType::UNRECOGNIZED,
                                    QString(),
                                    QString(),
                                    0,
                                    0,
                                    false,
                                    elapsedTime,
                                    errorMessage);
        }

        return;
    }

    // Get the canonical path of the file to load (only the path)
    fileDirPath = QFileInfo(cfg.fileName).canonicalPath();

    // Read the file into a byte array
    qDebug() << "Reading the whole file into a byte array...";
    QByteArray rawData = file.readAll();

    // Close the file
    file.close();

    switch (fileFormat)
    {
    case FileType::GRAPHML:
        if (parseAsGraphML(rawData))
        {
            fileLoaded = true;
        }
        break;
    case FileType::PAJEK:
        if (parseAsPajek(rawData))
        {
            fileLoaded = true;
        }
        break;
    case FileType::ADJACENCY:
        if (parseAsAdjacency(rawData, cfg, delimiter))
        {
            fileLoaded = true;
        }
        break;
    case FileType::GRAPHVIZ:
        if (parseAsDot(rawData))
        {
            fileLoaded = true;
        }
        break;
    case FileType::UCINET:
        if (parseAsDL(rawData))
        {
            fileLoaded = true;
        }
        break;
    case FileType::GML:
        if (parseAsGML(rawData))
        {
            fileLoaded = true;
        }
        break;
    case FileType::EDGELIST_WEIGHTED:
        if (parseAsEdgeListWeighted(rawData, delimiter))
        {
            fileLoaded = true;
        }
        break;
    case FileType::EDGELIST_SIMPLE:
        if (parseAsEdgeListSimple(rawData, delimiter))
        {
            fileLoaded = true;
        }
        break;
    case FileType::TWOMODE:
        if (parseAsTwoModeSociomatrix(rawData))
        {
            fileLoaded = true;
        }
        break;
    default: // GraphML
        if (parseAsGraphML(rawData))
        {
            fileLoaded = true;
        }
        break;
    }

    // Store computation time
    qint64 elapsedTime = computationTimer.elapsed();

    if (fileLoaded)
    {
        qDebug() << "[PARSER] emit signal fileLoaded:"
                 << "fileFormat" << fileFormat
                 << "edgeDirType" << edgeDirType
                 << "totalLinks(parser)" << totalLinks
                 << "totalNodes" << totalNodes;
        if (m_parseSink)
        {
            m_parseSink->fileLoaded(fileFormat,
                                    cfg.fileName,
                                    networkName,
                                    totalNodes,
                                    totalLinks,
                                    edgeDirType,
                                    elapsedTime);
        }
    }
    else if (errorMessage != QString())
    {
        if (m_parseSink)
        {
            m_parseSink->fileLoaded(FileType::UNRECOGNIZED,
                                    QString(),
                                    QString(),
                                    0,
                                    0,
                                    false,
                                    elapsedTime,
                                    errorMessage);
        }

        return;
    }

    qDebug() << "**** Parser finished. Emitting finished() signal. ";

    emit finished("Parser::load() - reach end");
}


/**
 * @brief Parses the data as two-mode sociomatrix formatted network.
 * @param rawData
 * @return
 */
bool Parser::parseAsTwoModeSociomatrix(const QByteArray &rawData)
{

    qDebug() << "Parsing data as two-mode sociomatrix formatted...";

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str;
    QStringList lineElement;
    int fileLine = 0;
    int i = 0, j = 0, newCount = 0, lastCount = 0;
    totalNodes = 0;
    edgeWeight = 1.0;
    edgeDirType = EdgeType::Undirected;
    relationsList.clear();

    while (!ts.atEnd())
    {
        i++;
        fileLine++;
        str = ts.readLine().simplified();
        if (isComment(str))
            continue;
        if (str.contains("vertices", Qt::CaseInsensitive) || str.contains("network", Qt::CaseInsensitive) || str.contains("graph", Qt::CaseInsensitive) || str.contains("digraph", Qt::CaseInsensitive) || str.contains("DL n", Qt::CaseInsensitive) || str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) || str.contains("graphml", Qt::CaseInsensitive) || str.contains("xml", Qt::CaseInsensitive))
        {
            qDebug() << "*** Not a two mode sociomatrix-formatted file. Aborting!!";

            errorMessage = tr("Invalid two-mode sociomatrix file. "
                              "Non-comment line %1 includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)")
                               .arg(fileLine);
            return false;
        }
        if (str.contains(","))
        {
            lineElement = str.split(",");
            newCount = lineElement.size();
        }
        else
        {
            lineElement = str.split(" ");
            newCount = lineElement.size();
        }
        qDebug() << str;
        qDebug() << "newCount " << newCount << " nodes. We are at i = " << i;
        if ((newCount != lastCount && i > 1))
        { // line element count differ
            qDebug() << "*** Not a Sociomatrix-formatted file. Aborting!!";

            errorMessage = tr("Invalid two-mode sociomatrix file. "
                              "Row %1 has fewer or more elements than previous line.")
                               .arg(i);
            return false;
        }
        lastCount = newCount;
        randX = rand() % gwWidth;
        randY = rand() % gwHeight;
        qDebug() << "Signaling to create new node"
                 << i << "at random pos:" << randX << "x" << randY;
        if (m_parseSink)
        {
            m_parseSink->createNode(i, initNodeSize, initNodeColor,
                                    initNodeNumberColor, initNodeNumberSize,
                                    QString::number(i), initNodeLabelColor, initNodeLabelSize,
                                    QPointF(randX, randY),
                                    initNodeShape, QString());
        }

        j = 1;

        qDebug() << "reading actor affiliations...";
        for (QStringList::Iterator it1 = lineElement.begin(); it1 != lineElement.end(); ++it1)
        {
            if ((*it1) != "0")
            {
                qDebug() << "there is an 1 from " << i << " to " << j;
                firstModeMultiMap.insert(i, j);
                secondModeMultiMap.insert(j, i);
                for (int k = 1; k < i; ++k)
                {
                    qDebug() << "Checking earlier discovered actor k = " << k;
                    if (firstModeMultiMap.contains(k, j))
                    {
                        arrows = true;
                        bezier = false;
                        edgeWeight = 1;
                        qDebug() << "Actor" << i << " on the same event as actor " << k << ". signaling to create new edge";
                        if (m_parseSink)
                        {
                            m_parseSink->createEdge(i, k, edgeWeight, initEdgeColor, EdgeType::Undirected, arrows, bezier);
                        }

                        totalLinks++;
                    }
                }
            }
            j++;
        }
    }

    if (relationsList.size() == 0)
    {
        if (m_parseSink)
        {
            m_parseSink->addNewRelation("unnamed");
        }
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}

