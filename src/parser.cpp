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
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QTextStream>
#include <QString>
#include <QtDebug> //used for qDebug messages
#include <QPointF>
#include <QTextCodec>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <list>  // used as list<int> listDummiesPajek
#include <queue> //for priority queue

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
 * @brief Signals to create either a single new node (numbered fixedNum) or multiple new nodes (numbered from 1 to to newNodes)
 * @param fixedNum
 * @param label
 * @param newNodes
 */
void Parser::createRandomNodes(const int &fixedNum,
                               const QString &label,
                               const int &newNodes)
{
    if (newNodes != 1)
    {
        for (int i = 0; i < newNodes; i++)
        {
            qDebug() << "Signaling to create multiple nodes. Now signaling for node:" << i + 1;
            if (m_parseSink)
            {
                m_parseSink->createNodeAtPosRandom(false);
            }
        }
    }
    else
    {
        qDebug() << "Signaling to create a single node:" << fixedNum << "with label:" << label;
        if (m_parseSink)
        {
            m_parseSink->createNodeAtPosRandomWithLabel(fixedNum, label, false);
        }
    }
}

/**
 * @brief Normalizes a quoted identifier from external network formats.
 *
 * Some formats (e.g. Pajek) use quotes in headers as syntactic delimiters, such as:
 * ```
 *   *Matrix 9: "star"
 *   *Matrix :9 "star"
 *   *Matrix :9 'star'
 * ```
 *
 * Quotes are part of the file syntax and must not become part of the internal
 * relation name. This function:
 *
 *  - trims surrounding whitespace
 *  - removes a single pair of wrapping double quotes OR single quotes, if present
 *  - normalizes doubled quotes inside quoted strings:
 *      ""  -> "
 *      ''  -> '
 *
 * It does NOT collapse internal whitespace.
 *
 * @param s Raw identifier substring extracted from the file.
 * @return Clean identifier suitable for internal storage.
 */
QString Parser::normalizeQuotedIdentifier(const QString &s)
{
    QString out = s.trimmed();

    // Strip one pair of wrapping quotes (format delimiters)
    if (out.size() >= 2)
    {
        const QChar first = out.front();
        const QChar last = out.back();

        if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
            out = out.mid(1, out.size() - 2);
    }

    // Normalize doubled quotes inside quoted strings
    out.replace("\"\"", "\"");
    out.replace("''", "'");

    return out.trimmed();
}

/**
 * @brief Parse a Pajek-formatted network from raw bytes.
 *
 * Supported constructs include (depending on the file contents):
 *  - `*Network` header
 *  - `*Vertices N` (node definitions with optional attributes)
 *  - Tie sections such as `*Arcs`, `*Edges`
 *  - Matrix-based relations:
 *      - `*Matrix :k`
 *      - `*Matrix :k "Label"`
 *      - `*Matrix k: "Label"`
 *      - empty-label cases like `*Matrix :k` / `*Matrix k:`
 *
 * Behavior:
 *  - Creates nodes/edges by emitting the standard Parser signals.
 *  - Detects directedness and relation structure according to Pajek sections.
 *  - Populates totals (nodes/links) used by the loader and CLI harness.
 *  - On failure, sets @c errorMessage and returns false.
 *
 * @param rawData Entire file contents (as read from disk).
 * @return true if parsing succeeds, false otherwise.
 */
bool Parser::parseAsPajek(const QByteArray &rawData)
{

    qDebug() << "Parsing data as pajek formatted...";

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str, label, temp;
    nodeColor = "";
    edgeColor = "";
    nodeShape = "";
    initEdgeLabel = QString();
    QStringList lineElement;
    bool ok = false, intOk = false, check1 = false, check2 = false;
    bool has_arcs = false;
    bool nodes_flag = false, edges_flag = false, arcs_flag = false, arcslist_flag = false, matrix_flag = false;
    fileContainsNodeColors = false;
    fileContainsNodeCoords = false;
    fileContainsLinkColors = false;
    fileContainsLinkLabels = false;
    bool zero_flag = false;
    int i = 0, j = 0, miss = 0, source = -1, target = -1, nodeNum, colorIndex = -1;
    int coordIndex = -1, labelIndex = -1;
    unsigned long int fileLineNumber = 0;
    unsigned long int actualLineNumber = 0;
    int pos = -1, lastRelationIndex = 0;
    qreal weight = 1;
    QString relation;
    list<int> listDummiesPajek;
    totalLinks = 0;
    totalNodes = 0;
    j = 0;    // counts how many real nodes exist in the file
    miss = 0; // counts missing nodeNumbers.
    // if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
    relationsList.clear();

    QRegularExpression myRegExp;

    while (!ts.atEnd())
    {

        fileLineNumber++;

        str = ts.readLine();
        str = str.simplified();

        if (isComment(str))
        {
            continue;
        }

        actualLineNumber++;

        qDebug() << "*** str:" << str;

        if (actualLineNumber == 1)
        {
            if (str.startsWith("graph", Qt::CaseInsensitive) || str.startsWith("digraph", Qt::CaseInsensitive) || str.startsWith("DL", Qt::CaseInsensitive) || str.startsWith("list", Qt::CaseInsensitive) || str.startsWith("graphml", Qt::CaseInsensitive) || str.startsWith("<?xml", Qt::CaseInsensitive) || str.startsWith("LEDA.GRAPH", Qt::CaseInsensitive) || (!str.startsWith("*network", Qt::CaseInsensitive) && !str.startsWith("*vertices", Qt::CaseInsensitive)))
            {
                qDebug() << "*** Not a Pajek-formatted file. Aborting!!";
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line %1 (at file line %2) does not start with "
                                  "Network or Vertices")
                                   .arg(actualLineNumber)
                                   .arg(fileLineNumber);
                return false;
            }
        }

        if (!edges_flag && !arcs_flag && !nodes_flag && !arcslist_flag && !matrix_flag)
        {
            // qDebug("reading headlines");
            if ((actualLineNumber == 1) &&
                (!str.contains("network", Qt::CaseInsensitive) && !str.contains("vertices", Qt::CaseInsensitive)))
            {
                qDebug("*** Not a Pajek file. Aborting!");
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line does not start with "
                                  "Network or Vertices");
                return false;
            }
            else if (str.startsWith("*network", Qt::CaseInsensitive))
            { // NETWORK NAME
                networkName = (str.right(str.size() - 8)).simplified();
                if (!networkName.isEmpty())
                {
                    qDebug() << "networkName: "
                             << networkName;
                }
                else
                {
                    qDebug() << "set networkName to unnamed.";
                    networkName = "unnamed";
                }
                continue;
            }
            if (str.contains("vertices", Qt::CaseInsensitive))
            {
                myRegExp.setPattern("\\s+");
                lineElement = str.split(myRegExp);
                if (!lineElement[1].isEmpty())
                    totalNodes = lineElement[1].toInt(&intOk, 10);
                qDebug("Vertices %i.", totalNodes);
                continue;
            }
            qDebug("headlines end here");
        }
        /**SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) IN SEVERAL ELEMENTS*/
        myRegExp.setPattern("\\s+");
        lineElement = str.split(myRegExp, Qt::SkipEmptyParts);

        if (str.contains("*edges", Qt::CaseInsensitive))
        {
            edges_flag = true;
            arcs_flag = false;
            arcslist_flag = false;
            matrix_flag = false;
            continue;
        }
        else if (str.contains("*arcs", Qt::CaseInsensitive))
        {
            arcs_flag = true;
            edges_flag = false;
            arcslist_flag = false;
            matrix_flag = false;
            // check if row has label for arcs data,
            //  and use it as relation name
            if ((pos = str.indexOf(":")) != -1)
            {
                relation = str.right(str.size() - pos - 1);
                relation = normalizeQuotedIdentifier(relation);
                relationsList << relation;
                qDebug() << "added new relation" << relation
                         << "to relationsList - signaling to add new relation";
                if (m_parseSink)
                {
                    m_parseSink->addNewRelation(relation);
                }
                lastRelationIndex = relationsList.size() - 1;
                if (lastRelationIndex > 0)
                {
                    qDebug() << "last relation index:"
                             << lastRelationIndex
                             << "signaling to change to the last relation...";
                    if (m_parseSink)
                    {
                        m_parseSink->setRelation(lastRelationIndex);
                    }

                    i = 0; // reset the source node index
                }
            }

            continue;
        }
        else if (str.contains("*arcslist", Qt::CaseInsensitive))
        {
            arcs_flag = false;
            edges_flag = false;
            arcslist_flag = true;
            matrix_flag = false;
            continue;
        }
        else if (str.contains("*matrix", Qt::CaseInsensitive))
        {
            /*
             * Pajek *Matrix Header Import Policy
             *
             * IMPORT IS TOLERANT.
             *
             * The parser accepts common real-world variants, including:
             *
             *   *Matrix :1
             *   *Matrix :1 "Label"
             *   *Matrix 1:
             *   *Matrix 1: "Label"
             *
             * The parser normalizes the relation name so that:
             *   - Leading ":k", "k", or "k:" tokens do not pollute the label.
             *   - Wrapping quotes are removed.
             *
             * Export canonicalization is handled in Graph::saveToPajekFormat().
             *
             * Do NOT make the importer stricter to enforce canonical form.
             * Canonicalization belongs to export, not import.
             */

            auto parsePajekMatrixHeader = [](const QString &line,
                                             int *outK,
                                             QString *outLabel) -> bool
            {
                // Accept:
                //   *Matrix :1
                //   *Matrix :1 "Label"
                //   *Matrix 1:
                //   *Matrix 1: "Label"
                //
                // Output:
                //   outK = 1-based matrix index
                //   outLabel = cleaned label (no surrounding quotes), may be empty

                QString s = line.trimmed();

                // Remove leading "*Matrix" (case-insensitive)
                if (!s.toLower().startsWith("*matrix"))
                    return false;

                s = s.mid(QString("*matrix").size()).trimmed();
                if (s.isEmpty())
                    return false;

                // Optional ":" before k
                if (s.startsWith(':'))
                    s = s.mid(1).trimmed();

                // Read integer k
                int pos = 0;
                while (pos < s.size() && s.at(pos).isDigit())
                    ++pos;

                if (pos == 0)
                    return false;

                bool ok = false;
                const int k = s.left(pos).toInt(&ok);
                if (!ok || k <= 0)
                    return false;

                s = s.mid(pos).trimmed();

                // Optional ":" after k
                if (s.startsWith(':'))
                    s = s.mid(1).trimmed();

                // Whatever remains is the optional label
                QString label;
                if (!s.isEmpty())
                    label = normalizeQuotedIdentifier(s); // your existing helper (strips wrapping quotes etc)

                *outK = k;
                *outLabel = label.trimmed();
                return true;
            };

            qDebug() << str;
            arcs_flag = false;
            edges_flag = false;
            arcslist_flag = false;
            matrix_flag = true;
            // check if row has label for matrix data,
            //  and use it as relation name
            int k = 0;
            QString label;
            if (parsePajekMatrixHeader(str, &k, &label))
            {
                // If label missing, use temporary name = index (for UI)
                relation = label.isEmpty() ? QString::number(k) : label;

                relationsList << relation;
                qDebug() << "added new relation" << relation
                         << "to relationsList - signaling to add new relation";
                if (m_parseSink)
                {
                    m_parseSink->addNewRelation(relation);
                }
                lastRelationIndex = relationsList.size() - 1;
                if (lastRelationIndex > 0)
                {
                    qDebug() << "last relation index:"
                             << lastRelationIndex
                             << "signaling to change to the last relation...";
                    if (m_parseSink)
                    {
                        m_parseSink->setRelation(lastRelationIndex);
                    }
                }
                i = 0; // reset the source node index
            }

            continue;
        }

        /** READING NODES, THEN EDGES/ARCS */
        if (!edges_flag && !arcs_flag && !arcslist_flag && !matrix_flag)
        {
            // qDebug("=== Reading nodes ===");
            nodes_flag = true;
            nodeNum = lineElement[0].toInt(&intOk, 10);
            // qDebug()<<"node number: "<<nodeNum;
            if (nodeNum == 0)
            {
                qDebug("Node is zero numbered! Raising zero-start-flag - increasing nodenum");
                zero_flag = true;
            }
            if (zero_flag)
            {
                nodeNum++;
            }
            if (lineElement.size() < 2)
            {
                label = lineElement[0];
                randX = rand() % gwWidth;
                randY = rand() % gwHeight;
                nodeColor = initNodeColor;
                nodeShape = initNodeShape;
            }
            else
            { /** NODELABEL */
                label = lineElement[1];
                // qDebug()<< "node label: " << lineElement[1].toLatin1();
                str.remove(0, str.lastIndexOf(label) + label.size());
                // qDebug()<<"cropped str: "<< str.toLatin1();
                if (label.contains('"', Qt::CaseInsensitive))
                    label = label.remove('\"');
                // qDebug()<<"node label now: " << label.toLatin1();

                /** NODESHAPE: There are five possible . */
                if (str.contains("Ellipse", Qt::CaseInsensitive))
                    nodeShape = "ellipse";
                else if (str.contains("circle", Qt::CaseInsensitive))
                    nodeShape = "circle";
                else if (str.contains("box", Qt::CaseInsensitive))
                    nodeShape = "box";
                else if (str.contains("star", Qt::CaseInsensitive))
                    nodeShape = "star";
                else if (str.contains("triangle", Qt::CaseInsensitive))
                    nodeShape = "triangle";
                else
                    nodeShape = "diamond";
                /** NODECOLORS */
                // if there is an "ic" tag, a specific NodeColor for this node follows...
                if (str.contains("ic", Qt::CaseInsensitive))
                {
                    for (int c = 0; c < lineElement.size(); c++)
                    {
                        if (lineElement[c] == "ic")
                        {
                            // the colourname is at c+1 position.
                            nodeColor = lineElement[c + 1];
                            fileContainsNodeColors = true;
                            break;
                        }
                    }
                    // qDebug()<<"nodeColor:" << nodeColor;
                    if (nodeColor.contains("."))
                        nodeColor = initNodeColor;
                    if (nodeColor.startsWith("RGB"))
                        nodeColor.replace(0, 3, "#");
                    qDebug() << " \n\n PAJEK color " << nodeColor;
                }
                else
                { // there is no nodeColor. Use the default
                    // qDebug("No nodeColor");
                    fileContainsNodeColors = false;
                    nodeColor = initNodeColor;
                }
                /**READ NODE COORDINATES */
                if (str.contains(".", Qt::CaseInsensitive))
                {
                    for (int c = 0; c < lineElement.size(); c++)
                    {
                        temp = lineElement.at(c);
                        //		qDebug()<< temp.toLatin1();
                        if ((coordIndex = temp.indexOf(".", Qt::CaseInsensitive)) != -1)
                        {
                            if (lineElement.at(c - 1) == "ic")
                                continue; // pajek declares colors with numbers!
                            if (!temp[coordIndex - 1].isDigit())
                                continue; // needs 0.XX
                            if (c + 1 == lineElement.size())
                            { // first coord zero, i.e: 0  0.455
                                // qDebug ()<<"coords: " <<lineElement.at(c-1).toLatin1() << " " <<temp.toLatin1() ;
                                randX = lineElement.at(c - 1).toDouble(&check1);
                                randY = temp.toDouble(&check2);
                            }
                            else
                            {
                                // qDebug ()<<"coords: " << temp.toLatin1() << " " <<lineElement[c+1].toLatin1();
                                randX = temp.toDouble(&check1);
                                randY = lineElement[c + 1].toDouble(&check2);
                            }

                            if (check1 && check2)
                            {
                                randX = randX * gwWidth;
                                randY = randY * gwHeight;
                                fileContainsNodeCoords = true;
                            }
                            if (randX <= 0.0 || randY <= 0.0)
                            {
                                randX = rand() % gwWidth;
                                randY = rand() % gwHeight;
                            }
                            break;
                        }
                    }
                    // qDebug()<<"Coords: "<<randX << randY<< gwHeight;
                }
                else
                {
                    fileContainsNodeCoords = false;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    // qDebug()<<"No coords. Using random "<<randX << randY;
                }
            }
            // START NODE CREATION
            qDebug() << "Creating node numbered " << nodeNum << " Real nodes count (j)= " << j + 1;
            j++; // Controls the real number of nodes.
            // If the file misses some nodenumbers then we create dummies and delete them afterwards!
            if (j + miss < nodeNum)
            {
                qDebug() << "There are " << j << " nodes but this node has number" << nodeNum;
                for (int num = j; num < nodeNum; num++)
                {
                    qDebug() << "Signaling to create new dummy node" << num
                             << "at" << QPointF(randX, randY);
                    if (m_parseSink)
                    {
                        m_parseSink->createNode(num,
                                                initNodeSize,
                                                nodeColor,
                                                initNodeNumberColor,
                                                initNodeNumberSize,
                                                label,
                                                lineElement[3],
                                                initNodeLabelSize,
                                                QPointF(randX, randY),
                                                nodeShape,
                                                QString());
                    }

                    listDummiesPajek.push_back(num);
                    miss++;
                }
            }
            else if (j > nodeNum)
            {
                qDebug("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
                errorMessage = tr("Invalid Pajek-formatted file. It declares a node with "
                                  "nodeNumber smaller than previous nodes.");
                return false;
            }
            qDebug() << "Signaling to create new node" << nodeNum
                     << "at" << QPointF(randX, randY);
            if (m_parseSink)
            {
                m_parseSink->createNode(
                    nodeNum, initNodeSize, nodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    label, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    nodeShape, QString());
            }

            initNodeColor = nodeColor;
        }
        else
        {
            // NODES CREATED. CREATE EDGES/ARCS NOW.
            // first check that all nodes are already created
            if (j && j != totalNodes)
            { // if there were more or less nodes than the file declared
                qDebug() << "*** WARNING ***: The Pajek file declares " << totalNodes << "  nodes, but I found " << j << " nodes....";
                totalNodes = j;
            }
            else if (j == 0)
            { // if there were no nodes at all, we need to create them now.
                qDebug() << "The Pajek file declares " << totalNodes << " but I didn't found any nodes. I will create them....";
                for (int num = j + 1; num <= totalNodes; num++)
                {
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    qDebug() << "Signaling to create new node" << num
                             << "at random pos:" << QPointF(randX, randY);
                    if (m_parseSink)
                    {
                        m_parseSink->createNode(
                            num,
                            initNodeSize,
                            initNodeColor,
                            initNodeNumberColor,
                            initNodeNumberSize,
                            QString::number(i),
                            initNodeLabelColor,
                            initNodeLabelSize,
                            QPointF(randX, randY),
                            initNodeShape,
                            QString(),
                            false);
                    }
                }
                j = totalNodes;
            }
            if (edges_flag && !arcs_flag)
            { /**EDGES */

                qDebug("==== Reading edges ====");
                qDebug() << lineElement;

                source = lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok, 10);

                if (source == 0 || target == 0)
                {
                    errorMessage = tr("Invalid Pajek-formatted file. The file declares an edge "
                                      "with a zero source or target nodeNumber. "
                                      "However, each node should have a nodeNumber > 0.");
                    return false;
                }
                else if (source < 0 && target > 0)
                { // weights come first...

                    edgeWeight = lineElement[0].toDouble(&ok);

                    source = lineElement[1].toInt(&ok, 10);

                    if (lineElement.size() > 2)
                    {
                        target = lineElement[2].toInt(&ok, 10);
                    }
                    else
                    {
                        target = lineElement[1].toInt(&ok, 10); // self link
                    }
                }
                else if (lineElement.size() > 2)
                {
                    edgeWeight = lineElement[2].toDouble(&ok);
                }
                else
                {
                    edgeWeight = 1.0;
                }

                // qDebug()<<"weight "<< weight;

                if (lineElement.contains("c", Qt::CaseSensitive))
                {
                    // qDebug("file with link colours");
                    fileContainsLinkColors = true;
                    myRegExp.setPattern("[c]");
                    colorIndex = lineElement.indexOf(myRegExp, 0) + 1;
                    if (colorIndex >= lineElement.size())
                        edgeColor = initEdgeColor;
                    else
                        edgeColor = lineElement[colorIndex];
                    if (edgeColor.contains("."))
                        edgeColor = initEdgeColor;
                    // qDebug()<< " current color "<< edgeColor;
                }
                else
                {
                    // qDebug("file with no link colours");
                    edgeColor = initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive))
                {
                    qDebug("file with link labels");
                    fileContainsLinkLabels = true;
                    myRegExp.setPattern("[l]");
                    labelIndex = lineElement.indexOf(myRegExp, 0) + 1;
                    if (labelIndex >= lineElement.size())
                        edgeLabel = initEdgeLabel;
                    else
                        edgeLabel = lineElement[labelIndex];
                    if (edgeLabel.contains("."))
                        edgeLabel = initEdgeLabel;
                    qDebug() << " edge label " << edgeLabel;
                }
                else
                {
                    // qDebug("file with no link labels");
                    edgeLabel = initEdgeLabel;
                }

                arrows = false;
                bezier = false;
                qDebug() << "EDGES: signaling to create new edge:" << source << " - " << target;
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, edgeColor,
                                            EdgeType::Undirected, arrows, bezier, edgeLabel);
                }

                totalLinks = totalLinks + 2;

            } // end if EDGES
            else if (!edges_flag && arcs_flag)
            { /** ARCS */

                // qDebug("=== Reading arcs ===");
                source = lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok, 10);

                if (source == 0 || target == 0)
                {
                    errorMessage = tr("Invalid Pajek-formatted file. The file declares arc "
                                      "with a zero source or target nodeNumber. "
                                      "However, each node should have a nodeNumber > 0.");
                    return false; //  i -->(i-1)   internally
                }
                else if (source < 0 && target > 0)
                { // weights come first...

                    edgeWeight = lineElement[0].toDouble(&ok);
                    source = lineElement[1].toInt(&ok, 10);

                    if (lineElement.size() > 2)
                    {
                        target = lineElement[2].toInt(&ok, 10);
                    }
                    else
                    {
                        target = lineElement[1].toInt(&ok, 10); // self link
                    }
                }
                else if (lineElement.size() > 2)
                {
                    edgeWeight = lineElement[2].toDouble(&ok);
                }
                else
                {
                    edgeWeight = 1.0;
                }

                if (lineElement.contains("c", Qt::CaseSensitive))
                {
                    // qDebug("file with link colours");
                    myRegExp.setPattern("[c]");
                    edgeColor = lineElement.at(lineElement.indexOf(myRegExp, 0) + 1);
                    fileContainsLinkColors = true;
                }
                else
                {
                    // qDebug("file with no link colours");
                    edgeColor = initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive))
                {
                    qDebug("file with link labels");
                    fileContainsLinkLabels = true;
                    myRegExp.setPattern("[l]");
                    labelIndex = lineElement.indexOf(myRegExp, 0) + 1;
                    if (labelIndex >= lineElement.size())
                        edgeLabel = initEdgeLabel;
                    else
                        edgeLabel = lineElement.at(labelIndex);
                    // if (edgeLabel.contains (".") )  edgeLabel=initEdgeLabel;
                    qDebug() << " edge label " << edgeLabel;
                }
                else
                {
                    // qDebug("file with no link labels");
                    edgeLabel = initEdgeLabel;
                }
                arrows = true;
                bezier = false;
                has_arcs = true;
                qDebug() << "ARCS: signaling to create new arc:" << source << "->" << target << "with weight " << weight;
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, edgeColor,
                                            EdgeType::Directed, arrows, bezier, edgeLabel);
                }
                totalLinks++;
            } // else if ARCS
            else if (arcslist_flag)
            { /** ARCSlist */
                // qDebug("=== Reading arcs list===");
                if (lineElement[0].startsWith("-"))
                    lineElement[0].remove(0, 1);
                source = lineElement[0].toInt(&ok, 10);
                fileContainsLinkColors = false;
                edgeColor = initEdgeColor;
                has_arcs = true;
                arrows = true;
                bezier = false;
                for (int index = 1; index < lineElement.size(); index++)
                {
                    target = lineElement.at(index).toInt(&ok, 10);
                    qDebug() << "ARCS LIST: signaling to create new arc:" << source << "->" << target << "with weight " << weight;
                    if (m_parseSink)
                    {
                        m_parseSink->createEdge(source, target, edgeWeight, edgeColor,
                                                EdgeType::Directed, arrows, bezier);
                    }

                    totalLinks++;
                }
            } // else if ARCSLIST
            else if (matrix_flag)
            { /** matrix */
                // qDebug("=== Reading matrix of edges===");
                i++;
                source = i;
                fileContainsLinkColors = false;
                edgeColor = initEdgeColor;
                has_arcs = true;
                arrows = true;
                bezier = false;
                for (target = 0; target < lineElement.size(); target++)
                {
                    if (lineElement.at(target) != "0")
                    {
                        edgeWeight = lineElement.at(target).toFloat(&ok);
                        qDebug() << " MATRIX: signaling to create new arc"
                                 << source << "->" << target + 1
                                 << "with weight" << weight;
                        if (m_parseSink)
                        {
                            m_parseSink->createEdge(source, target + 1, edgeWeight, edgeColor,
                                                    EdgeType::Directed, arrows, bezier);
                        }

                        totalLinks++;
                    }
                }
            } // else if matrix
        } // end if BOTH ARCS AND EDGES
    } // end WHILE

    if (j == 0)
    {
        errorMessage = tr("Invalid Pajek-formatted file. Could not find node declarations in this file.");
        return false;
    }

    qDebug("Removing all dummy nodes, if any");
    if (listDummiesPajek.size() > 0)
    {
        qDebug("Trying to delete the dummies now");
        for (list<int>::iterator it = listDummiesPajek.begin(); it != listDummiesPajek.end(); it++)
        {
            if (m_parseSink)
            {
                m_parseSink->removeDummyNode(*it);
            }
        }
    }

    if (relationsList.size() == 0)
    {
        if (m_parseSink)
        {
            m_parseSink->addNewRelation(networkName);
        }
    }

    qDebug() << "Clearing temporary dummies and relations list";
    listDummiesPajek.clear();
    relationsList.clear();

    qDebug() << "signaling to change to the first relation...";
    if (m_parseSink)
    {
        m_parseSink->setRelation(0);
    }

    if (has_arcs)
    {
        edgeDirType = EdgeType::Directed;
    }
    else
    {
        edgeDirType = EdgeType::Undirected;
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}



/**
 * Iterates through a row of the adjacency matrix to create edges.
 * Emits a signal for each non-zero weight to create an edge between nodes.
 * Parsing is aborted immediately if any invalid data is encountered.
 * @param currentRow The adjacency matrix row being processed.
 * @param rowIndex The index of the row (source node for edges).
 * @return true if edges are successfully created, false otherwise.
 */
bool Parser::createEdgesForRow(const QStringList &currentRow, int rowIndex)
{
    int colIndex = 0;
    bool conversionOK = false;

    for (const QString &edgeStr : currentRow)
    {
        colIndex++;
        edgeWeight = edgeStr.toDouble(&conversionOK);

        // Abort if a matrix element cannot be converted to a number.
        if (!conversionOK)
        {
            errorMessage = tr("Error reading Adjacency-formatted file. "
                              "Element (%1, %2) contains invalid data ('%3'). Parsing aborted.")
                               .arg(rowIndex)
                               .arg(colIndex)
                               .arg(edgeStr);
            return false;
        }

        // If the weight is greater than 0, create a directed edge.
        if (edgeWeight > 0)
        {
            qDebug() << "Signaling to create new edge:" << rowIndex << "->" << colIndex
                     << "weight:" << edgeWeight << "TotalLinks:" << totalLinks + 1;
            if (m_parseSink)
            {
                m_parseSink->createEdge(rowIndex, colIndex, edgeWeight, initEdgeColor, EdgeType::Directed, true, false);
            }

            totalLinks++;
        }
    }

    return true;
}

/**
 * Checks if the given string contains any reserved keywords.
 * Reserved keywords suggest the file is not adjacency-formatted but in another graph format.
 * Parsing is aborted if a reserved keyword is found.
 * @param str The string to check for keywords.
 * @return true if a reserved keyword is found, false otherwise.
 */
bool Parser::containsReservedKeywords(const QString &str) const
{
    // List of keywords reserved by other file formats.
    static const QStringList reservedKeywords = {
        "*Vertices", "*Arcs", "*Edges", "*Network", "graph", "digraph", "DL n", "DL", "dl", "list", "<graphml", "<?xml"};

    for (const QString &keyword : reservedKeywords)
    {
        if (str.trimmed().startsWith(keyword, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
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

/**
 * @brief Parses the data as GraphML (not GML) formatted network.
 *
 * @param rawData
 * @return bool
 */
bool Parser::parseAsGraphML(const QByteArray &rawData)
{

    qDebug() << "Parsing data as GraphML formatted...";

    totalNodes = 0;
    totalLinks = 0;
    nodeHash.clear();
    relationsList.clear();

    bool_key = false;
    bool_node = false;
    bool_edge = false;
    key_id = "";
    key_name = "";
    key_type = "";
    key_value = "";
    initNodeCustomIcon = "";
    initEdgeWeight = 1;
    edgeWeight = 1;
    edgeColor = "black";
    arrows = true;
    edgeDirType = EdgeType::Directed;

    // Create a xml parser
    QXmlStreamReader xml;

    // Prepare the user selected codec, if needed
    QByteArray userSelectedCodec = m_textCodecName.toLatin1();

    // Add raw data into xml parser
    xml.addData(rawData);

    qDebug() << "Testing if XML document encoding is the same as the userSelectedCodec:" << userSelectedCodec;

    xml.readNext();
    if (xml.isStartDocument())
    {
        qDebug() << "XML document version"
                 << xml.documentVersion()
                 << "encoding" << xml.documentEncoding()
                 << "userSelectedCodec"
                 << m_textCodecName;
        if (xml.documentEncoding().toString() != m_textCodecName)
        {
            qDebug() << "Conflicting encodings. "
                     << " Re-reading data with userSelectedCodec" << userSelectedCodec;
            xml.clear();

            QTextCodec *codec = QTextCodec::codecForName(userSelectedCodec);
            QString decodedData = codec->toUnicode(rawData);
            xml.addData(decodedData);

            //                QTextStream in(&rawData);
            //                in.setAutoDetectUnicode(false);
            //                QString decodedData = in.readAll();
            //                // QTextStream no longer supports setCodec
            //                in.setEncoding()
            //                QTextStream in(&rawData);
        }
        else
        {
            qDebug() << "Testing XML: OK";
            xml.clear();
            xml.addData(rawData);
        }
    }

    while (!xml.atEnd())
    {
        xml.readNext();
        qDebug() << "xml.token " << xml.tokenString();
        if (xml.isStartDocument())
        {
            qDebug() << "xml startDocument" << " version "
                     << xml.documentVersion()
                     << " encoding " << xml.documentEncoding();
        }

        if (xml.isStartElement())
        {
            qDebug() << "element name " << xml.name().toString();

            if (xml.name().toString() == "graphml")
            {
                qDebug() << "GraphML start. NamespaceUri is "
                         << xml.namespaceUri().toString()
                         << "Calling readGraphML()";
                if (!readGraphML(xml))
                {
                    // return false;
                    break;
                }
            }
            else
            { // not a GraphML doc, return false.
                xml.raiseError(
                    QObject::tr("not a GraphML file."));
                qDebug() << "### Error in startElement "
                         << " The file is not an GraphML version 1.0 file ";
                errorMessage = tr("Invalid GraphML file. "
                                  "XML at startElement but element name not graphml.");
                break;
            }
        }
        else if (xml.tokenString() == "Invalid")
        {
            xml.raiseError(
                QObject::tr("invalid GraphML or encoding."));
            qDebug() << "### Cannot find startElement"
                     << " The file is not valid GraphML or has invalid encoding";
            errorMessage = tr("Invalid GraphML file. "
                              "XML tokenString at line %1 invalid.")
                               .arg(xml.lineNumber());
            break;
        }
    } // end while

    // clear our mess - remove every hash element...
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    nodeHash.clear();
    edgeMissingNodesList.clear();

    // if there was an error return false with error string
    if (xml.hasError())
    {
        qDebug() << "### xmls has error! "
                    "Returning false with errorString"
                 << xml.errorString();
        errorMessage =
            tr("Invalid GraphML file. "
               "XML has error at line %1, token name %2:\n\n%3")
                .arg(xml.lineNumber())
                .arg(xml.name().toString())
                .arg(xml.errorString());
        xml.clear();
        return false;
    }

    xml.clear();

    qDebug() << "signaling to change to the first relation...";
    if (m_parseSink)
    {
        m_parseSink->setRelation(0);
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}

/**
 * @brief Checks the xml token name and calls the appropriate function.
 *
 * @param xml
 * @return bool
 */
bool Parser::readGraphML(QXmlStreamReader &xml)
{
    qDebug() << "Reading graphml token/element...";
    bool_node = false;
    bool_edge = false;
    bool_key = false;
    // Q_ASSERT(xml.isStartElement() && xml.name().toString() == "graph");

    while (!xml.atEnd())
    { // start reading until QXmlStreamReader end().

        xml.readNext(); // read next token

        qDebug() << "line:" << xml.lineNumber();

        if (xml.isStartElement())
        { // new token (graph, node, or edge) here
            qDebug() << "isStartElement() : "
                     << xml.name().toString();
            if (xml.name().toString() == "graph") // graph definition token
                readGraphMLElementGraph(xml);

            else if (xml.name().toString() == "key")
            { // key definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementKey(xmlStreamAttr);
            }
            else if (xml.name().toString() == "default") // default key value token
                readGraphMLElementDefaultValue(xml);

            else if (xml.name().toString() == "node") // graph definition token
                readGraphMLElementNode(xml);

            else if (xml.name().toString() == "data") // data definition token
                readGraphMLElementData(xml);

            else if (xml.name().toString() == "ShapeNode")
            {
                bool_node = true;
            }
            else if ((xml.name().toString() == "Geometry" || xml.name().toString() == "Fill" || xml.name().toString() == "BorderStyle" || xml.name().toString() == "NodeLabel" || xml.name().toString() == "Shape") && bool_node)
            {
                readGraphMLElementNodeGraphics(xml);
            }

            else if (xml.name().toString() == "edge")
            { // edge definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementEdge(xmlStreamAttr);
            }

            else if (xml.name().toString() == "BezierEdge")
            {
                bool_edge = true;
            }

            else if ((xml.name().toString() == "Path" || xml.name().toString() == "LineStyle" || xml.name().toString() == "Arrows" || xml.name().toString() == "EdgeLabel") && bool_edge)
            {
                readGraphMLElementEdgeGraphics(xml);
            }

            else
                readGraphMLElementUnknown(xml);
        }

        if (xml.isEndElement())
        { // token ends here
            qDebug() << " element ends here: "
                     << xml.name().toString();
            if (xml.name().toString() == "node") // node definition end
                endGraphMLElementNode(xml);
            else if (xml.name().toString() == "edge") // edge definition end
                endGraphMLElementEdge(xml);
        }

        if (xml.hasError())
        {
            qDebug() << "xml has error:" << xml.errorString();
            return false;
        }
    }

    // Check if we need to create any edges with missing nodes
    createMissingNodeEdges();

    return true;
}

/**
 * @brief Reads a graph definition
 *
 * Called at Graph element
 *
 * @param xml
 */
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml)
{
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
    qDebug() << "Parsing graph element - edgedefault "
             << defaultDirection;
    if (defaultDirection == "undirected")
    {
        qDebug() << "this is an undirected graph ";
        edgeDirType = EdgeType::Undirected;
        arrows = false;
    }
    else
    {
        qDebug() << "this is a directed graph ";
        edgeDirType = EdgeType::Directed;
        arrows = true;
    }
    // store graph id
    networkName = xmlStreamAttr.value("id").toString();
    // add it as relation
    relationsList << networkName;
    qDebug() << "Signaling to add new relation:" << networkName;
    if (m_parseSink)
    {
        m_parseSink->addNewRelation(networkName);
    }
    int lastRelationIndex = relationsList.size() - 1;
    if (lastRelationIndex > 0)
    {
        totalNodes = 0;
        qDebug() << "last relation index:"
                 << lastRelationIndex
                 << "signaling to change to the new relation";
        if (m_parseSink)
        {
            m_parseSink->setRelation(lastRelationIndex);
        }
    }
    qDebug() << "graph id:" << networkName;
}

/**
 * @brief Reads a key definition
 *
 * called at key element
 *
 * @param xmlStreamAttr
 */
void Parser::readGraphMLElementKey(QXmlStreamAttributes &xmlStreamAttr)
{
    key_id = xmlStreamAttr.value("id").toString();
    qDebug() << "Reading key element - key id" << key_id;
    key_what = xmlStreamAttr.value("for").toString();
    keyFor[key_id] = key_what;
    qDebug() << "key for " << key_what;

    if (xmlStreamAttr.hasAttribute("attr.name"))
    { // to be enabled in later versions..
        key_name = xmlStreamAttr.value("attr.name").toString();
        keyName[key_id] = key_name;
        qDebug() << "key attr.name" << key_name;
    }
    if (xmlStreamAttr.hasAttribute("attr.type"))
    {
        key_type = xmlStreamAttr.value("attr.type").toString();
        keyType[key_id] = key_type;
        qDebug() << "key attr.type" << key_type;
    }
    else if (xmlStreamAttr.hasAttribute("yfiles.type"))
    {
        key_type = xmlStreamAttr.value("yfiles.type").toString();
        keyType[key_id] = key_type;
        qDebug() << "key yfiles.type" << key_type;
    }
}

/**
 * @brief Reads default key values
 *
 * Called at a default element (usually nested inside key element)
 *
 * @param xml
 */
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml)
{

    key_value = xml.readElementText();
    key_name = keyName.value(key_id);
    keyDefaultValue[key_id] = key_value; // key_id is already stored

    qDebug() << "Reading default key values - key default value is"
             << key_value;

    if (keyFor.value(key_id) == "node")
    {
        if (key_name == "size")
        {
            qDebug() << "key default value" << key_value << "is for node size";
            conv_OK = false;
            initNodeSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                initNodeSize = 8;
        }
        if (key_name == "shape")
        {
            qDebug() << "key default value" << key_value << "is for nodes shape";
            initNodeShape = key_value;
        }
        if (key_name == "custom-icon")
        {
            qDebug() << "key default value" << key_value << "is for node custom-icon path";
            initNodeCustomIcon = key_value;
            initNodeCustomIcon = fileDirPath + "/" + initNodeCustomIcon;
            qDebug() << "initNodeCustomIcon full path:" << initNodeCustomIcon;
            if (QFileInfo::exists(initNodeCustomIcon))
            {
                qDebug() << "custom icon file exists!";
            }
            else
            {
                qDebug() << "custom icon file does not exists!";
                xml.raiseError(
                    QObject::tr(" Default custom icon for nodes does not exist in the filesystem. \nThe declared icon file was: \n%1").arg(initNodeCustomIcon));
            }
        }
        if (key_name == "color")
        {
            qDebug() << "key default value" << key_value << "is for nodes color";
            initNodeColor = key_value;
        }
        if (key_name == "label.color")
        {
            qDebug() << "key default value" << key_value << "is for node labels color";
            initNodeLabelColor = key_value;
        }
        if (key_name == "label.size")
        {
            qDebug() << "key default value" << key_value << "is for node labels size";
            conv_OK = false;
            initNodeLabelSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                initNodeLabelSize = 8;
        }
    }
    else if (keyFor.value(key_id) == "edge")
    {
        if (key_name == "weight")
        {
            qDebug() << "key default value" << key_value << "is for edges weight";
            conv_OK = false;
            initEdgeWeight = key_value.toDouble(&conv_OK);
            if (!conv_OK)
                initEdgeWeight = 1;
        }
        if (key_name == "color")
        {
            qDebug() << "key default value" << key_value << "is for edges color";
            initEdgeColor = key_value;
        }
    }
}

/**
 * @brief Reads basic node attributes and sets the nodeNumber.
 *
 * called at the start of a node element
 *
 * @param xml
 */
void Parser::readGraphMLElementNode(QXmlStreamReader &xml)
{
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    node_id = (xmlStreamAttr.value("id")).toString();
    totalNodes++;

    //    qDebug()<< "reading node id"<<  node_id
    //           << "index" << totalNodes
    //           << "added to nodeHash"
    //           << "gwWidth, gwHeight "<< gwWidth<< "," <<gwHeight;

    nodeHash[node_id] = totalNodes;

    // copy default node attribute values.
    // Some might change when reading element data, some will stay the same...
    nodeColor = initNodeColor;
    nodeShape = initNodeShape;
    nodeIconPath = initNodeCustomIcon;
    nodeSize = initNodeSize;
    nodeNumberSize = initNodeNumberSize;
    nodeNumberColor = initNodeNumberColor;
    nodeLabel = node_id;
    nodeLabelSize = initNodeLabelSize;
    nodeLabelColor = initNodeLabelColor;
    nodeCustomAttributes = initNodeCustomAttributes;
    bool_node = true;
    randX = rand() % gwWidth;
    randY = rand() % gwHeight;
}

/**
 * @brief Signals to create a new node
 *
 * called at the end of a node element
 *
 * @param xml
 */
void Parser::endGraphMLElementNode(QXmlStreamReader &xml)
{
    Q_UNUSED(xml);
    //@todo this check means we cannot have different nodes between relations.
    if (relationsList.size() > 1)
    {
        qDebug() << "multirelational data"
                    "skipping node creation. Node should have been created in earlier relation";
        bool_node = false;
        return;
    }

    qDebug() << "signaling to create a new node"
             << totalNodes << "id " << node_id
             << " label " << nodeLabel << "at pos:" << QPointF(randX, randY);

    if (nodeShape == "custom")
    {
        if (m_parseSink)
        {
            m_parseSink->createNode(totalNodes,
                                    nodeSize,
                                    nodeColor,
                                    nodeNumberColor,
                                    nodeNumberSize,
                                    nodeLabel,
                                    nodeLabelColor,
                                    nodeLabelSize,
                                    QPointF(randX, randY),
                                    nodeShape,
                                    (nodeIconPath.isEmpty() ? initNodeCustomIcon : nodeIconPath),
                                    false,
                                    nodeCustomAttributes);
        }
    }
    else
    {
        if (m_parseSink)
        {
            m_parseSink->createNode(totalNodes,
                                    nodeSize,
                                    nodeColor,
                                    nodeNumberColor,
                                    nodeNumberSize,
                                    nodeLabel,
                                    nodeLabelColor,
                                    nodeLabelSize,
                                    QPointF(randX, randY),
                                    nodeShape,
                                    QString(),
                                    false,
                                    nodeCustomAttributes);
        }
    }

    bool_node = false;
}

/**
 * @brief Reads basic edge creation properties.
 *
 * called at the start of an edge element
 *
 * @param xmlStreamAttr
 */
void Parser::readGraphMLElementEdge(QXmlStreamAttributes &xmlStreamAttr)
{

    edge_source = xmlStreamAttr.value("source").toString();
    edge_target = xmlStreamAttr.value("target").toString();
    edge_directed = xmlStreamAttr.value("directed").toString();

    //    qDebug()<< "Parsing edge id: "
    //            <<	xmlStreamAttr.value("id").toString()
    //                << "edge_source " << edge_source
    //                << "edge_target " << edge_target
    //                << "directed " << edge_directed;

    missingNode = false;
    edgeWeight = initEdgeWeight;
    edgeColor = initEdgeColor;
    edgeLabel = "";
    bool_edge = true;

    if (edge_directed == "false" || edge_directed.contains("false", Qt::CaseInsensitive))
    {
        edgeDirType = EdgeType::Undirected;
        qDebug() << "Edge is UNDIRECTED";
    }
    else
    {
        edgeDirType = EdgeType::Directed;
        qDebug() << "Edge is DIRECTED";
    }
    if (!nodeHash.contains(edge_source))
    {
        qDebug() << "source node id "
                 << edge_source
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                     QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
        missingNode = true;
    }
    if (!nodeHash.contains(edge_target))
    {
        qDebug() << "target node id "
                 << edge_target
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                     QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
        missingNode = true;
    }

    if (missingNode)
    {
        return;
    }

    source = nodeHash[edge_source];
    target = nodeHash[edge_target];
    qDebug() << "source " << edge_source
             << " num " << source
             << " - target " << edge_target << " num " << target
             << " edgeDirType " << edgeDirType;
}

/**
 * @brief Signals for a new edge to be created/added
 *
 * Called at the end of edge element
 *
 * @param xml
 */
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml)
{
    Q_UNUSED(xml);
    if (missingNode)
    {
        qDebug() << "missingNode true "
                 << " postponing edge creation signal";
        return;
    }
    qDebug() << "signaling to create new edge"
             << source << "->" << target << " edgeDirType value " << edgeDirType;
    if (m_parseSink)
    {
        m_parseSink->createEdge(source, target, edgeWeight, edgeColor, edgeDirType,
                                arrows, bezier, edgeLabel);
    }

    totalLinks++;
    bool_edge = false;
}

/**
 * @brief Reads data for edges and nodes
 *
 * called at a data element (usually nested inside a node or an edge element)
 *
 * @param xml
 */
void Parser::readGraphMLElementData(QXmlStreamReader &xml)
{

    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    key_id = xmlStreamAttr.value("key").toString();
    key_name = keyName.value(key_id);
    key_value = xml.text().toString();

    qDebug() << "parding data for key_id: "
             << key_id << "key_value " << key_value;

    if (key_value.trimmed() == "")
    {
        qDebug() << "empty key_value: "
                 << key_value
                 << "reading more xml.text()...";

        xml.readNext();

        key_value = xml.text().toString();

        qDebug() << "now key_value: " << key_value;

        if (key_value.trimmed() != "")
        {
            // if there's simple text after the StartElement,
            qDebug() << "key_id " << key_id
                     << " value is simple text " << key_value;
        }
        else
        { // no text, probably more tags. Return...
            qDebug() << "key_id " << key_id
                     << " for " << keyFor.value(key_id)
                     << ". More elements nested here. Returning";
            return;
        }
    }
    if (keyFor.value(key_id) == "node")
    {

        if (key_name == "color")
        {
            qDebug() << "Data found. Node color: "
                     << key_value << " for this node";
            nodeColor = key_value;
        }
        else if (key_name == "label")
        {
            qDebug() << "Data found. Node label: "
                        ""
                     << key_value << " for this node";
            nodeLabel = key_value;
        }
        else if (key_name == "x_coordinate")
        {
            qDebug() << "Data found. Node x: "
                     << key_value << " for this node";
            conv_OK = false;
            randX = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                randX = 0;
            else
                randX = randX * gwWidth;
            qDebug() << "Using: " << randX;
        }
        else if (key_name == "y_coordinate")
        {
            qDebug() << "Data found. Node y: "
                     << key_value << " for this node";
            conv_OK = false;
            randY = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                randY = 0;
            else
                randY = randY * gwHeight;
            qDebug() << "Using: " << randY;
        }
        else if (key_name == "size")
        {
            qDebug() << "Data found. Node size: "
                     << key_value << " for this node";
            conv_OK = false;
            nodeSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                nodeSize = initNodeSize;
            qDebug() << "Using: " << nodeSize;
        }
        else if (key_name == "label.size")
        {
            qDebug() << "Data found. Node label size: "
                     << key_value << " for this node";
            conv_OK = false;
            nodeLabelSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                nodeLabelSize = initNodeLabelSize;
            qDebug() << "Using: " << nodeSize;
        }
        else if (key_name == "label.color")
        {
            qDebug() << "Data found. Node label Color: "
                     << key_value << " for this node";
            nodeLabelColor = key_value;
        }
        else if (key_name == "shape")
        {
            qDebug() << "Data found. Node shape: "
                     << key_value << " for this node";
            nodeShape = key_value;
        }
        else if (key_name == "custom-icon")
        {
            qDebug() << "Data found. Node custom-icon path: "
                     << key_value << " for this node";
            nodeIconPath = key_value;
            nodeIconPath = fileDirPath + ("/") + nodeIconPath;
            qDebug() << "full node custom-icon path: "
                     << nodeIconPath;
        }
        else
        {
            qDebug() << "Data found for custom attribute: "
                     << key_name << " of this node. Data: " << key_value;
            nodeCustomAttributes.insert(key_name, key_value);
        }
    }
    else if (keyFor.value(key_id) == "edge")
    {
        if (key_name == "color")
        {
            qDebug() << "Data found. Edge color: "
                     << key_value << " for this edge";
            edgeColor = key_value;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
        }
        else if ((key_name == "value" || key_name == "weight"))
        {
            conv_OK = false;
            edgeWeight = key_value.toDouble(&conv_OK);
            if (!conv_OK)
                edgeWeight = 1.0;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
            qDebug() << "Data found. Edge value: "
                     << key_value << " Using " << edgeWeight << " for this edge";
        }
        else if (key_name == "size of arrow")
        {
            conv_OK = false;
            qreal temp = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                arrowSize = 1;
            else
                arrowSize = temp;
            qDebug() << "Data found. Edge arrow size: "
                     << key_value << " Using  " << arrowSize << " for this edge";
        }
        else if (key_name == "label")
        {
            edgeLabel = key_value;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
            qDebug() << "Data found. Edge label: "
                     << edgeLabel << " for this edge";
        }
    }
}

/**
 * @brief Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 * @param xml
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml)
{
    qDebug() << "reading node graphics/properties, element name"
             << xml.name().toString();
    qreal tempX = -1, tempY = -1, temp = -1;
    QString color;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if (xml.name().toString() == "Geometry")
    {
        if (xmlStreamAttr.hasAttribute("x"))
        {
            conv_OK = false;
            tempX = xml.attributes().value("x").toString().toFloat(&conv_OK);
            if (conv_OK)
                randX = tempX;
        }
        if (xmlStreamAttr.hasAttribute("y"))
        {
            conv_OK = false;
            tempY = xml.attributes().value("y").toString().toFloat(&conv_OK);
            if (conv_OK)
                randY = tempY;
        }
        qDebug() << "Node Coordinates: "
                 << tempX << " " << tempY << " Using coordinates" << randX << " " << randY;
        if (xmlStreamAttr.hasAttribute("width"))
        {
            conv_OK = false;
            temp = xmlStreamAttr.value("width").toString().toFloat(&conv_OK);
            if (conv_OK)
                nodeSize = temp;
            qDebug() << "Node Size: "
                     << temp << " Using nodesize" << nodeSize;
        }
        if (xmlStreamAttr.hasAttribute("shape"))
        {
            nodeShape = xmlStreamAttr.value("shape").toString();
            qDebug() << "Node Shape: "
                     << nodeShape;
        }
    }
    else if (xml.name().toString() == "Fill")
    {
        if (xmlStreamAttr.hasAttribute("color"))
        {
            nodeColor = xmlStreamAttr.value("color").toString();
            qDebug() << "Node color: "
                     << nodeColor;
        }
    }
    else if (xml.name().toString() == "BorderStyle")
    {
    }
    else if (xml.name().toString() == "NodeLabel")
    {
        key_value = xml.readElementText(); // see if there's simple text after the StartElement
        if (!xml.hasError())
        {
            qDebug() << "Node Label "
                     << key_value;
            nodeLabel = key_value;
        }
        else
        {
            qDebug() << "Cannot read Node Label. There must be more elements nested here, continuing";
        }
    }
    else if (xml.name().toString() == "Shape")
    {
        if (xmlStreamAttr.hasAttribute("type"))
        {
            nodeShape = xmlStreamAttr.value("type").toString();
            qDebug() << "Node shape: "
                     << nodeShape;
        }
    }
}

/**
 * @brief Reads edge graphics data and properties: path, linestyle,width, arrows, etc
 * @param xml
 */
void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml)
{
    qDebug() << "reading edge graphics/props, element name"
             << xml.name().toString();

    qreal tempX = -1, tempY = -1, temp = -1;
    QString color, tempString;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if (xml.name().toString() == "Path")
    {
        if (xmlStreamAttr.hasAttribute("sx"))
        {
            conv_OK = false;
            tempX = xmlStreamAttr.value("sx").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p1_x = tempX;
            else
                bez_p1_x = 0;
        }
        if (xmlStreamAttr.hasAttribute("sy"))
        {
            conv_OK = false;
            tempY = xmlStreamAttr.value("sy").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p1_y = tempY;
            else
                bez_p1_y = 0;
        }
        if (xmlStreamAttr.hasAttribute("tx"))
        {
            conv_OK = false;
            tempX = xmlStreamAttr.value("tx").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p2_x = tempX;
            else
                bez_p2_x = 0;
        }
        if (xmlStreamAttr.hasAttribute("ty"))
        {
            conv_OK = false;
            tempY = xmlStreamAttr.value("ty").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p2_y = tempY;
            else
                bez_p2_y = 0;
        }
        qDebug() << "Edge Path control points: "
                 << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
    }
    else if (xml.name().toString() == "LineStyle")
    {
        if (xmlStreamAttr.hasAttribute("color"))
        {
            edgeColor = xmlStreamAttr.value("color").toString();
            qDebug() << "Edge color: "
                     << edgeColor;
        }
        if (xmlStreamAttr.hasAttribute("type"))
        {
            edgeType = xmlStreamAttr.value("type").toString();
            qDebug() << "Edge type: "
                     << edgeType;
        }
        if (xmlStreamAttr.hasAttribute("width"))
        {
            temp = xmlStreamAttr.value("width").toString().toFloat(&conv_OK);
            if (conv_OK)
                edgeWeight = temp;
            else
                edgeWeight = 1.0;
            qDebug() << "Edge width: "
                     << edgeWeight;
        }
    }
    else if (xml.name().toString() == "Arrows")
    {
        if (xmlStreamAttr.hasAttribute("source"))
        {
            tempString = xmlStreamAttr.value("source").toString();
            qDebug() << "Edge source arrow type: "
                     << tempString;
        }
        if (xmlStreamAttr.hasAttribute("target"))
        {
            tempString = xmlStreamAttr.value("target").toString();
            qDebug() << "Edge target arrow type: "
                     << tempString;
        }
    }
    else if (xml.name().toString() == "EdgeLabel")
    {
        key_value = xml.readElementText(); // see if there's simple text after the StartElement
        if (!xml.hasError())
        {
            qDebug() << "Edge Label "
                     << key_value;
            // probably there's more than simple text after StartElement
            edgeLabel = key_value;
        }
        else
        {
            qDebug() << "Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
            edgeLabel = "";
        }
    }
}

/**
 * @brief Trivial call for unknown elements
 * @param xml
 */
void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement());
    qDebug() << "unknown element found:" << xml.name().toString();
}

/**
 * @brief Creates any missing node edges
 */
void Parser::createMissingNodeEdges()
{
    qDebug() << "Creating missing node edges... ";
    int count = 0;
    if ((count = edgesMissingNodesHash.size()) > 0)
    {

        bool ok;
        edgeWeight = initEdgeWeight;
        edgeColor = initEdgeColor;
        edgeDirType = EdgeType::Directed;
        qDebug() << "edges to create " << count;
        QHash<QString, QString>::const_iterator it =
            edgesMissingNodesHash.constBegin();
        while (it != edgesMissingNodesHash.constEnd())
        {
            qDebug() << "creating missing edge "
                     << it.key() << " data " << it.value();
            edgeMissingNodesList = (it.key()).split("===>");
            if (!((edgeMissingNodesList[0]).isEmpty()) && !((edgeMissingNodesList[1]).isEmpty()))
            {
                source = nodeHash.value(edgeMissingNodesList[0], -666);
                target = nodeHash.value(edgeMissingNodesList[1], -666);
                if (source == -666 || target == -666)
                {
                    // emit something that this node has not been declared
                    continue;
                }
                edgeMissingNodesListData = (it.value()).split("|");
                if (!edgeMissingNodesListData[0].isEmpty())
                {
                    edgeWeight = edgeMissingNodesListData[0].toInt(&ok, 10);
                }
                if (!edgeMissingNodesListData[1].isEmpty())
                {
                    edgeColor = edgeMissingNodesListData[1];
                }
                if (!edgeMissingNodesListData[2].isEmpty())
                {
                    if ((edgeMissingNodesListData[2]).contains("2"))
                        edgeDirType = EdgeType::Undirected;
                }
                qDebug() << "signaling to create new edge:"
                         << source << "->" << target << " edgeDirType value " << edgeDirType;
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, edgeColor, edgeDirType, arrows, bezier, edgeLabel);
                }
            }
            ++it;
        }
    }
    else
    {
        qDebug() << "nothing to do";
    }
}

/**
 * @brief Parses the data as GML formatted network.
 *
 * This parser is line/state based. Many GML files are "compact" and place
 * multiple attributes on the same line, e.g.:
 *
 *   node [ id 1 label "1" ]
 *
 * while others use the expanded form:
 *
 *   node [
 *     id 1
 *     label "1"
 *   ]
 *
 * To support both forms, we preprocess the decoded input into a normalized
 * stream where:
 *   - '[' and ']' become standalone lines (outside quoted strings)
 *   - each attribute becomes its own "key value" line (outside quoted strings),
 *     e.g. 'id 1 label "1"' -> 'id 1' + 'label "1"'
 *
 * We then run the existing state machine unchanged (but now it receives
 * one attribute per line).
 *
 * Also accepts both "weight" and "value" as edge weight keys.
 *
 * @param rawData Raw file bytes.
 * @return bool True on success, false on parse error.
 */
bool Parser::parseAsGML(const QByteArray &rawData)
{
    qDebug() << "Parsing data as GML formatted...";

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);

    /**
     * @brief Returns true if ch is whitespace (space/tab/etc).
     */
    auto isWS = [](QChar ch) -> bool
    {
        return ch.isSpace();
    };

    /**
     * @brief Tokenizes a line into tokens while respecting quoted strings.
     *
     * Example:
     *   id 1 label "John Doe"
     * becomes tokens:
     *   ["id","1","label","\"John Doe\""]
     *
     * Quotes are preserved as part of the token.
     */
    auto tokenizeRespectQuotes = [&](const QString &line) -> QStringList
    {
        QStringList tokens;
        QString cur;
        bool inQuotes = false;

        auto flush = [&]()
        {
            const QString t = cur.trimmed();
            if (!t.isEmpty())
                tokens << t;
            cur.clear();
        };

        for (int i = 0; i < line.size(); ++i)
        {
            const QChar ch = line.at(i);

            if (ch == '"')
            {
                inQuotes = !inQuotes;
                cur.append(ch);
                continue;
            }

            if (!inQuotes && isWS(ch))
            {
                flush();
                continue;
            }

            cur.append(ch);
        }
        flush();
        return tokens;
    };

    /**
     * @brief Normalizes GML text for a line-based parser.
     *
     * 1) Makes '[' and ']' standalone lines (outside quoted strings).
     * 2) Splits compact attribute runs into "key value" lines using a known-key set.
     *
     * This is intentionally conservative: we only split on keys we know how
     * to parse. Unknown tokens are left as-is.
     */
    auto normalizeGmlForLineParser = [&](const QString &in) -> QString
    {
        // Keys we want to split into separate "key value" lines.
        const QSet<QString> keys = {
            "directed", "isplanar",
            "id", "label",
            "source", "target",
            "weight", "value",
            "graphics", "center", "type", "fill",
            "graph", "node", "edge"};

        QString out;
        out.reserve(in.size() + 256);

        // Step 1: split brackets into their own lines (outside quotes).
        {
            bool inQuotes = false;
            for (int i = 0; i < in.size(); ++i)
            {
                const QChar ch = in.at(i);

                if (ch == '"')
                {
                    inQuotes = !inQuotes;
                    out.append(ch);
                    continue;
                }

                if (!inQuotes && (ch == '[' || ch == ']'))
                {
                    out.append('\n');
                    out.append(ch);
                    out.append('\n');
                    continue;
                }

                out.append(ch);
            }
        }

        // Step 2: for each line, split "id 1 label "1"" into "id 1" + "label "1"" etc.
        QString normalized;
        normalized.reserve(out.size() + 256);

        QTextStream ts(&out);
        while (!ts.atEnd())
        {
            QString line = ts.readLine().trimmed();
            if (line.isEmpty())
            {
                normalized.append('\n');
                continue;
            }

            // Keep standalone bracket/section lines intact.
            if (line == "[" || line == "]")
            {
                normalized.append(line);
                normalized.append('\n');
                continue;
            }

            // Tokenize with quote awareness.
            const QStringList toks = tokenizeRespectQuotes(line);
            if (toks.size() <= 2)
            {
                // already "key value" or single keyword
                normalized.append(line);
                normalized.append('\n');
                continue;
            }

            // If the line is a known section keyword (graph/node/edge/graphics), keep it alone.
            const QString firstLower = toks.first().toLower();
            if (firstLower == "graph" || firstLower == "node" || firstLower == "edge" || firstLower == "graphics")
            {
                normalized.append(toks.first());
                normalized.append('\n');

                // Remaining tokens may contain attributes on same line: split them.
                int i = 1;
                while (i < toks.size())
                {
                    const QString k = toks.at(i);
                    const QString kLower = k.toLower();

                    if (!keys.contains(kLower))
                    {
                        // Unknown token: keep as raw remainder and stop splitting.
                        QString rest;
                        for (int j = i; j < toks.size(); ++j)
                        {
                            if (!rest.isEmpty())
                                rest += " ";
                            rest += toks.at(j);
                        }
                        normalized.append(rest);
                        normalized.append('\n');
                        break;
                    }

                    // We expect "key value" pairs for attributes.
                    if (i + 1 < toks.size())
                    {
                        normalized.append(k);
                        normalized.append(' ');
                        normalized.append(toks.at(i + 1));
                        normalized.append('\n');
                        i += 2;
                    }
                    else
                    {
                        // Dangling key with no value: keep as-is.
                        normalized.append(k);
                        normalized.append('\n');
                        break;
                    }
                }
                continue;
            }

            // Otherwise interpret as a sequence of key/value pairs: k v k v ...
            int i = 0;
            while (i < toks.size())
            {
                const QString k = toks.at(i);
                const QString kLower = k.toLower();

                if (!keys.contains(kLower))
                {
                    // Unknown token: keep remainder on one line.
                    QString rest;
                    for (int j = i; j < toks.size(); ++j)
                    {
                        if (!rest.isEmpty())
                            rest += " ";
                        rest += toks.at(j);
                    }
                    normalized.append(rest);
                    normalized.append('\n');
                    break;
                }

                if (i + 1 < toks.size())
                {
                    normalized.append(k);
                    normalized.append(' ');
                    normalized.append(toks.at(i + 1));
                    normalized.append('\n');
                    i += 2;
                }
                else
                {
                    normalized.append(k);
                    normalized.append('\n');
                    break;
                }
            }
        }

        return normalized;
    };

    decodedData = normalizeGmlForLineParser(decodedData);
    QTextStream ts(&decodedData);

    QRegularExpression onlyDigitsExp("^\\d+$");
    QStringList tempList;
    QString str;
    int fileLine = 0, actualLineNumber = 0;
    bool floatOK = false;
    bool isPlanar = false, graphKey = false, graphicsKey = false,
         edgeKey = false, nodeKey = false, graphicsCenterKey = false;
    Q_UNUSED(isPlanar);

    relationsList.clear();

    node_id = QString();
    arrows = true;
    bezier = false;
    edgeDirType = EdgeType::Undirected;
    totalNodes = 0;

    while (!ts.atEnd())
    {
        floatOK = false;
        fileContainsNodeCoords = false;
        nodeShape = initNodeShape;
        nodeColor = initNodeColor;

        fileLine++;
        str = ts.readLine().simplified();

        qDebug() << "line" << fileLine << ":" << str;

        if (isComment(str))
            continue;

        actualLineNumber++;

        if (actualLineNumber == 1 &&
            (str.contains("vertices", Qt::CaseInsensitive) ||
             str.contains("network", Qt::CaseInsensitive) ||
             str.contains("digraph", Qt::CaseInsensitive) ||
             str.contains("DL n", Qt::CaseInsensitive) ||
             str == "DL" || str == "dl" ||
             str.contains("list", Qt::CaseInsensitive) ||
             str.contains("graphml", Qt::CaseInsensitive) ||
             str.contains("xml", Qt::CaseInsensitive)))
        {
            qDebug() << "*** Not a GML-formatted file. Aborting!!";
            errorMessage = tr("Not an GML-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats  "
                              "(i.e vertices, graphml, network, digraph, DL, xml)")
                               .arg(fileLine);
            return false;
        }

        if (str.startsWith("comment", Qt::CaseInsensitive))
            continue;

        if (str.startsWith("creator", Qt::CaseInsensitive))
            continue;

        if (str.startsWith("graph", Qt::CaseInsensitive))
        {
            qDebug() << "graph description list start";
            graphKey = true;
            continue;
        }

        if (str.startsWith("directed", Qt::CaseInsensitive))
        {
            if (graphKey)
            {
                if (str.contains("1"))
                {
                    qDebug() << "graph directed 1. A directed graph.";
                    edgeDirType = EdgeType::Directed;
                }
                else
                {
                    qDebug() << "graph directed 0. An undirected graph.";
                }
            }
            continue;
        }

        if (str.startsWith("isPlanar", Qt::CaseInsensitive))
        {
            if (graphKey)
                isPlanar = str.contains("1");
            continue;
        }

        if (str.startsWith("node", Qt::CaseInsensitive))
        {
            qDebug() << "node description list starts";
            nodeKey = true;
            continue;
        }

        if (str.startsWith("id", Qt::CaseInsensitive))
        {
            if (nodeKey)
            {
                totalNodes++;
                node_id = str.split(" ", Qt::SkipEmptyParts).last();
                if (!node_id.contains(onlyDigitsExp))
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node id tag at line %1 has non-arithmetic value.")
                                       .arg(fileLine);
                    return false;
                }
                qDebug() << "id description "
                         << "This node" << totalNodes
                         << "id" << node_id;
            }
            continue;
        }

        if (str.startsWith("label ", Qt::CaseInsensitive))
        {
            if (nodeKey)
            {
                nodeLabel = str.split(" ", Qt::SkipEmptyParts).last().remove("\"");
                qDebug() << "node label definition"
                         << "node" << totalNodes
                         << "id" << node_id
                         << "label" << nodeLabel;
            }
            else if (edgeKey)
            {
                edgeLabel = str.split(" ", Qt::SkipEmptyParts).last();
                qDebug() << "edge label definition"
                         << "edge" << totalLinks
                         << "label" << edgeLabel;
            }
            continue;
        }

        if (str.startsWith("edge ", Qt::CaseInsensitive) || str == "edge")
        {
            qDebug() << "edge description list start";
            edgeKey = true;
            totalLinks++;
            edgeWeight = 1.0;
            edgeColor = "black";
            edgeLabel.clear();
            continue;
        }

        if (str.startsWith("source ", Qt::CaseInsensitive))
        {
            if (edgeKey)
            {
                edge_source = str.split(" ", Qt::SkipEmptyParts).last();
                if (!edge_source.contains(onlyDigitsExp))
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge source tag at line %1 has non-arithmetic value.")
                                       .arg(fileLine);
                    return false;
                }
                source = edge_source.toInt(nullptr, 10);
                qDebug() << "edge source definition"
                         << "edge source" << edge_source;
            }
            continue;
        }

        if (str.startsWith("target ", Qt::CaseInsensitive))
        {
            if (edgeKey)
            {
                edge_target = str.split(" ", Qt::SkipEmptyParts).last();

                // BUGFIX: validate edge_target, not edge_source
                if (!edge_target.contains(onlyDigitsExp))
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge target tag at line %1 has non-arithmetic value.")
                                       .arg(fileLine);
                    return false;
                }

                target = edge_target.toInt(nullptr, 10);
                qDebug() << "edge target definition"
                         << "edge target" << edge_target;
            }
            continue;
        }

        if (str.startsWith("weight ", Qt::CaseInsensitive) ||
            str.startsWith("value ", Qt::CaseInsensitive))
        {
            if (edgeKey)
            {
                edgeWeight = str.split(" ", Qt::SkipEmptyParts).last().toDouble(&floatOK);
                if (!floatOK)
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge weight tag at line %1 has an invalid value.")
                                       .arg(fileLine);
                    return false;
                }
                qDebug() << "edge weight definition"
                         << "edge weight" << edgeWeight;
            }
            continue;
        }

        if (str.startsWith("graphics", Qt::CaseInsensitive))
        {
            graphicsKey = true;
            continue;
        }

        if (str.startsWith("center", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                if (str.contains("[", Qt::CaseInsensitive))
                {
                    if (str.contains("]", Qt::CaseInsensitive) &&
                        str.contains("x", Qt::CaseInsensitive) &&
                        str.contains("y", Qt::CaseInsensitive))
                    {
                        str.remove("center");
                        str.remove("[");
                        str.remove("]");
                        str = str.simplified();
                        tempList = str.split(" ", Qt::SkipEmptyParts);
                        randX = (tempList.at(1)).toFloat(&floatOK);
                        if (!floatOK)
                        {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to qreal.")
                                               .arg(fileLine);
                            return false;
                        }
                        randY = tempList.at(3).toFloat(&floatOK);
                        if (!floatOK)
                        {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to qreal.")
                                               .arg(fileLine);
                            return false;
                        }
                        fileContainsNodeCoords = true;
                    }
                    else
                    {
                        graphicsCenterKey = true;
                    }
                }
            }
            continue;
        }

        if (str.startsWith("type", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                nodeShape = str.split(" ", Qt::SkipEmptyParts).last();
                if (nodeShape.isNull() || nodeShape.isEmpty())
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node type tag at line %1 has no value.")
                                       .arg(fileLine);
                    return false;
                }
                nodeShape.remove("\"");
            }
            continue;
        }

        if (str.startsWith("fill", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                nodeColor = str.split(" ", Qt::SkipEmptyParts).last();
                if (nodeColor.isNull() || nodeColor.isEmpty())
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node fill tag at line %1 has no value.")
                                       .arg(fileLine);
                    return false;
                }
            }
            continue;
        }

        if (str.startsWith("]", Qt::CaseInsensitive))
        {
            if (nodeKey && graphicsKey && graphicsCenterKey)
            {
                graphicsCenterKey = false;
                continue;
            }
            else if (graphicsKey)
            {
                graphicsKey = false;
                continue;
            }
            else if (nodeKey && !graphicsKey)
            {
                nodeKey = false;
                if (!fileContainsNodeCoords)
                {
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                }
                if (m_parseSink)
                {
                    m_parseSink->createNode(
                        node_id.toInt(nullptr, 10),
                        initNodeSize, nodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        nodeLabel, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        nodeShape, QString());
                }

                continue;
            }
            else if (edgeKey && !graphicsKey)
            {
                edgeKey = false;

                if (edgeLabel == QString())
                    edgeLabel = edge_source + "->" + edge_target;
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, edgeColor,
                                            edgeDirType, arrows, bezier, edgeLabel);
                }

                continue;
            }
            else if (graphKey)
            {
                graphKey = false;
                continue;
            }
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

/**
 * @brief Helper. Checks if the string parameter is a comment (starts with a known char, i.e #).
 *
 * @param str
 * @return  bool
 */
bool Parser::isComment(QString str)
{
    if (str.startsWith("#", Qt::CaseInsensitive) || str.startsWith("/*", Qt::CaseInsensitive) || str.startsWith("%", Qt::CaseInsensitive) || str.startsWith("/*", Qt::CaseInsensitive) || str.startsWith("//", Qt::CaseInsensitive) || str.isEmpty())
    {
        qDebug() << "Parser::isComment() - Comment or an empty line was found. "
                    "Skipping...";
        return true;
    }
    return false;
}
