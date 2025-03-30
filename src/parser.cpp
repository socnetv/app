/**
 * @file parser.cpp
 * @brief Implements the Parser class for reading and interpreting various network data formats, including adjacency matrices and sociomatrices.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
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

    qDebug() << "Parser loading file:" << fileName
             << "codecName" << codecName
             << "- Running On thread " << this->thread();

    initNodeSize = defNodeSize;
    initNodeColor = defNodeColor;
    initNodeShape = defNodeShape;
    initNodeNumberColor = defNodeNumberColor;
    initNodeNumberSize = defNodeNumberSize;
    initNodeLabelColor = defNodeLabelColor;
    initNodeLabelSize = defNodeLabelSize;

    initEdgeColor = defEdgeColor;

    edgeDirType = EdgeType::Directed;
    arrows = true;
    bezier = false;
    m_textCodecName = codecName;
    networkName = (fileName.split("/")).last();
    gwWidth = canvasWidth;
    gwHeight = canvasHeight;
    randX = 0;
    randY = 0;
    fileFormat = format;
    two_sm_mode = sm_mode;
    fileLoaded = false;

    if (!delim.isNull() && !delim.isEmpty())
    {
        delimiter = delim;
    }
    else
    {
        delimiter = " ";
    }

    xml = 0;

    qDebug() << "Initial networkName:" << networkName
             << "requested fileFormat: " << fileFormat
             << "delim:" << delim << "delimiter" << delimiter;

    errorMessage = QString();

    // Start a timer.
    QElapsedTimer computationTimer;
    computationTimer.start();

    // Try to open the file
    qDebug() << "Opening file...";
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qint64 elapsedTime = computationTimer.elapsed();
        qDebug() << "Cannot open file" << fileName;
        errorMessage = tr("Cannot open file: %1").arg(fileName);
        emit signalFileLoaded(FileType::UNRECOGNIZED,
                              QString(),
                              QString(),
                              0,
                              0,
                              false,
                              elapsedTime,
                              errorMessage);
        return;
    }

    // Get the canonical path of the file to load (only the path)
    fileDirPath = QFileInfo(fileName).canonicalPath();

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
        if (parseAsAdjacency(rawData, delimiter, sm_has_labels))
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
        emit signalFileLoaded(fileFormat,
                              fileName,
                              networkName,
                              totalNodes,
                              totalLinks,
                              edgeDirType,
                              elapsedTime);
    }
    else if (errorMessage != QString())
    {
        emit signalFileLoaded(FileType::UNRECOGNIZED,
                              QString(),
                              QString(),
                              0,
                              0,
                              false,
                              elapsedTime,
                              errorMessage);
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
            emit signalCreateNodeAtPosRandom(false);
        }
    }
    else
    {
        qDebug() << "Signaling to create a single node:" << fixedNum << "with label:" << label;
        emit signalCreateNodeAtPosRandomWithLabel(fixedNum, label, false);
    }
}

/**
 * @brief Parses the given raw data as DL formatted (UCINET) data.
 *
 * This function reads and interprets a DL formatted file, which is a format used by UCINET.
 * It processes the file line by line, extracting relevant information such as node labels,
 * edge weights, and network properties. The function supports both fullmatrix and edgelist1
 * formats, and can handle two-mode networks.
 *
 * @param rawData The raw data to be parsed, provided as a QByteArray.
 * @return true if the parsing is successful, false otherwise.
 *
 * The function performs the following steps:
 * - Reads the raw data and decodes it using the specified text codec.
 * - Checks if the first non-comment line starts with "DL".
 * - Extracts keywords such as N, NM, NR, NC, and FORMAT from the file.
 * - Reads row and column labels if present.
 * - Creates nodes based on the labels or the declared number of nodes.
 * - Reads and processes the data section to create edges between nodes.
 * - Emits signals to create nodes and edges in the network.
 * - Handles errors and inconsistencies in the file format.
 *
 * @note The function emits several signals during the parsing process:
 * - signalAddNewRelation(const QString &relation): Emitted when a new relation is found.
 * - signalSetRelation(int relationIndex): Emitted to set the current relation.
 * - signalCreateEdge(int source, int target, double weight, const QColor &color, EdgeType type, bool arrows, bool bezier): Emitted to create a new edge.
 *
 * @warning The function assumes that the input data is correctly formatted according to the DL specification.
 * Any deviations or errors in the format may result in parsing failures.
 */
bool Parser::parseAsDL(const QByteArray &rawData)
{

    qDebug() << "Parsing data as DL formatted (UCINET)...";

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str = QString();
    QString relation = QString();
    QString prevLineStr = QString();
    QString label = QString();
    QString value = QString();
    QString dlFormat = QString();
    QString edgeStr;

    unsigned long int fileLineNumber = 0;
    unsigned long int actualLineNumber = 0;
    int source = 1;
    int target = 1;
    int NM = 0;
    int NR = 0;
    int NC = 0;
    int nodeSum = 0;
    int relationCounter = 0;

    bool rowLabels_flag = false;
    bool colLabels_flag = false;
    bool data_flag = false;
    bool relation_flag = false;
    bool nodesCreated_flag = false;
    bool twoMode_flag = false;

    bool fullmatrixFormat = false;
    bool edgelist1Format = false;
    bool diagonalPresent = false;

    bool intOK = false;
    bool conversionOK = false;

    QStringList lineElement;
    QStringList tempList;
    QStringList rowLabels;
    QStringList colLabels;

    QRegularExpression myRegExp;

    relationsList.clear();

    totalLinks = 0;
    arrows = true;
    bezier = false;
    edgeWeight = 0;
    edgeDirType = EdgeType::Directed;

    while (!ts.atEnd())
    {

        fileLineNumber++;

        str = ts.readLine();
        str = str.simplified();

        if (isComment(str))
            continue;

        actualLineNumber++;

        qDebug() << "actualLineNumber " << actualLineNumber
                 << "str.simplified: \n"
                 << str;

        if (actualLineNumber == 1)
        {
            if (!str.startsWith("DL", Qt::CaseInsensitive))
            {
                qDebug() << "Not a DL file. Aborting!";
                errorMessage = tr("Invalid UCINET-formatted file. The file does not start with DL in first non-comment line %1").arg(fileLineNumber);
                return false;
            }
        } // end if actualLineNumber == 1

        //
        // This is a DL file.
        // Check if the line contains DL and comma
        // or we are still in search for N,NM, and FORMAT keywords
        //

        if (str.startsWith("DL", Qt::CaseInsensitive))
        {

            if (str.contains(","))
            {
                qDebug() << "DL starting line contains a comma";
                // If it is a DL file and contains a comma in the first line,
                // then the line might declare some keywords (N, NM, FORMAT)
                // this happens in R's sna output files
                lineElement = str.split(",", Qt::SkipEmptyParts);
                readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format, diagonalPresent);

            } //  end if str.contains(",")

            // if the line contains DL, does not contain any comma
            // but contains at least one "=" then we have keywords space separated.
            else if (str.contains("="))
            {
                qDebug() << "DL starting line contains a = but not a comma";
                // this is space separated
                lineElement = str.split(" ", Qt::SkipEmptyParts);
                readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format, diagonalPresent);

            } // end else if contains =

        } // end if startsWith("DL")

        //
        // Check if keywords are given in other lines, which do not start with DL
        //
        if (!str.contains("DL", Qt::CaseInsensitive) &&
            (str.contains("n =", Qt::CaseInsensitive) ||
             str.contains("n=", Qt::CaseInsensitive) ||
             str.contains("nm=", Qt::CaseInsensitive) ||
             str.contains("nm =", Qt::CaseInsensitive) ||
             str.contains("nr=", Qt::CaseInsensitive) ||
             str.contains("nr =", Qt::CaseInsensitive) ||
             str.contains("nc=", Qt::CaseInsensitive) ||
             str.contains("nc =", Qt::CaseInsensitive) ||
             str.contains("format =", Qt::CaseInsensitive) ||
             str.contains("format=", Qt::CaseInsensitive)))
        {

            // check if this line contains precisely one "="
            if (str.count("=", Qt::CaseInsensitive) == 1)
            {
                qDebug() << "Line contains just one = ";
                // then one of the above keywords is declared here
                tempList = str.split("=", Qt::SkipEmptyParts);

                label = tempList[0].simplified();
                value = tempList[1].simplified();

                if (label == "n" || label == "N")
                {
                    qDebug() << "N is declared to be : "
                             << value;
                    totalNodes = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "N conversion error...";
                        // emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert N value to integer at line %1.").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (label == "nm" || label == "NM")
                {
                    qDebug() << "NM is declared to be : "
                             << value;
                    NM = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NM conversion error...";
                        // emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NM value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (label == "nr" || label == "NR")
                {
                    qDebug() << "NR is declared to be : "
                             << value;
                    NR = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NR conversion error...";
                        // emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NR value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (label == "nc" || label == "NC")
                {
                    qDebug() << "NC is declared to be : "
                             << value;
                    NC = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NC conversion error...";
                        // emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NC value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (label == "format" || label == "FORMAT")
                {
                    qDebug() << "FORMAT is declared to be : "
                             << value;
                    if (value.contains("FULLMATRIX", Qt::CaseInsensitive))
                    {
                        fullmatrixFormat = true;
                        edgelist1Format = false;
                        qDebug() << "✅ FORMAT: FullMatrix detected";
                    }
                    else if (value.contains("edgelist", Qt::CaseInsensitive))
                    {
                        edgelist1Format = true;
                        fullmatrixFormat = false;
                        qDebug() << "✅ FORMAT: EdgeList detected";
                    }
                    else
                    {
                        qDebug() << "❌ ERROR: Unknown DL format. Expected 'FULLMATRIX' or 'edgelist'.";
                        errorMessage = tr("Invalid UCINET format declaration. Expected 'FULLMATRIX' or 'edgelist' but found: %1").arg(value);
                        return false;
                    }
                }
            } // end if count 1 "=" in line (network properties)

            // check if this line contains more than one "="
            else if (str.count("=", Qt::CaseInsensitive) > 1)
            {
                qDebug() << "Line contains multiple = ";
                if (str.contains(","))
                {
                    // this is comma separated
                    lineElement = str.split(",", Qt::SkipEmptyParts);
                    readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format, diagonalPresent);
                } // end else if contains comma

                // check if line contains space i.e. "NR=18 NC=14"
                else if (str.contains(" "))
                {
                    // this is space separated
                    lineElement = str.split(" ", Qt::SkipEmptyParts);
                    readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format, diagonalPresent);
                } // end else if contains space

            } // end if str.count("=") > 1 in line (network properties)

        } // end if str contains keywords

        else if (str.startsWith("labels", Qt::CaseInsensitive) || str.startsWith("row labels", Qt::CaseInsensitive))
        {
            rowLabels_flag = true;
            colLabels_flag = false;
            data_flag = false;
            relation_flag = false;
            qDebug() << "START LABELS RECOGNITION "
                        "AND NODE CREATION";
            continue;
        }
        else if (str.startsWith("COLUMN LABELS", Qt::CaseInsensitive))
        {
            colLabels_flag = true;
            rowLabels_flag = false;
            data_flag = false;
            relation_flag = false;
            qDebug() << "START COLUMN LABELS RECOGNITION "
                        "AND NODE CREATION";
            continue;
        }
        else if (str.startsWith("data:", Qt::CaseInsensitive) || str.startsWith("data :", Qt::CaseInsensitive))
        {
            data_flag = true;
            rowLabels_flag = false;
            colLabels_flag = false;
            relation_flag = false;
            qDebug() << "START DATA RECOGNITION "
                        "AND EDGE CREATION";
            continue;
        }
        else if (str.startsWith("LEVEL LABELS", Qt::CaseInsensitive))
        {
            relation_flag = true;
            data_flag = false;
            rowLabels_flag = false;
            colLabels_flag = false;
            qDebug() << "START RELATIONS RECOGNITION";
            continue;
        }
        else if (str.startsWith("matrix labels:", Qt::CaseInsensitive) || str.startsWith("matrix labels :", Qt::CaseInsensitive))
        {
            data_flag = false;
            rowLabels_flag = false;
            colLabels_flag = false;
            relation_flag = false;
            qDebug() << "matrix labels not supported";
            continue;
        }

        else if (str.isEmpty())
        {
            qDebug() << "EMPTY STRING - CONTINUE";
            continue;
        }

        if (rowLabels_flag)
        {
            // try to read row labels

            label = str.trimmed().simplified();

            if (rowLabels.contains(label))
            {
                qDebug() << "⚠ Warning: Duplicate row label '" << label << "' found. Ignoring.";
                continue;
            }
            else
            {
                qDebug() << "Adding label " << label
                         << " to rowLabels, list size: " << rowLabels.size();
                rowLabels << label;
            }
        }

        else if (colLabels_flag)
        {
            // try to read col labels

            label = str.trimmed().simplified();

            if (colLabels.contains(label))
            {
                qDebug() << "col label exists. CONTINUE";
                continue;
            }
            else
            {
                qDebug() << "Adding col label " << label
                         << " to colLabels";
                colLabels << label;
            }
        }
        else if (relation_flag)
        {
            relation = str;
            if (relationsList.contains(relation))
            {
                qDebug() << "relation exists. CONTINUE";
                continue;
            }
            else
            {
                qDebug() << "adding new relation" << relation
                         << "to relationsList and signaling to create new relation";
                relationsList << relation;
                emit signalAddNewRelation(relation);
            }
        }

        else if (data_flag)
        {

            // check if we haven't created any nodes...
            if (!nodesCreated_flag)
            {

                // check if there were NR and NC declared (then this is two-mode)
                qDebug() << "check if NR != 0 (two mode net).";
                if (NR != 0 && NC != 0)
                {
                    twoMode_flag = true;
                    qDebug() << "this is a two-mode net.";
                    // TODO: Check two-mode networks...
                }

                // check if we have found row labels
                if (rowLabels.size() == 0)
                {
                    // no labels found
                    qDebug() << "Nodes have not been created yet."
                             << "No node labels found."
                             << "Calling createRandomNodes(N) for all";
                    createRandomNodes(1, QString(), totalNodes);
                    nodeSum = totalNodes;
                }
                else if (rowLabels.size() == 1)
                {
                    // only one label line was found
                    // probably contains a comma to separate labels
                    // split it
                    qDebug() << "Nodes have not been created yet."
                             << "One row for labels found."
                             << "Splitting at a comma and calling createRandomNodes(1) for each label";
                    tempList = rowLabels[0].split(",", Qt::SkipEmptyParts);
                    for (QStringList::Iterator it1 = tempList.begin(); it1 != tempList.end(); ++it1)
                    {
                        label = (*it1);
                        nodeSum++;
                        createRandomNodes(nodeSum, label, 1);
                    }
                }
                else
                {
                    // multiple label lines were found

                    qDebug() << "Nodes have not been created yet."
                             << "Multiple label lines were found: " << rowLabels.size()
                             << "Calling createRandomNodes() for each label";
                    for (QStringList::Iterator it1 = rowLabels.begin(); it1 != rowLabels.end(); ++it1)
                    {
                        label = (*it1);
                        nodeSum++;
                        createRandomNodes(nodeSum, label, 1);
                    }
                }

                if (twoMode_flag)
                {

                    // check if we have found col labels
                    if (colLabels.size() == 0)
                    {
                        // no  col labels found
                        qDebug() << "Nodes have not been created yet."
                                 << "No node labels found."
                                 << "Calling createRandomNodes(NC) for all columns";
                        createRandomNodes(totalNodes, QString(), NC);
                    }
                    else if (colLabels.size() == 1)
                    {
                        // only one col label line was found
                        // probably contains a comma to separate labels
                        // split it
                        qDebug() << "Nodes have not been created yet."
                                 << "One line for col label found."
                                 << "Splitting at a comma and calling createRandomNodes(1) for each label";
                        tempList = colLabels[0].split(",", Qt::SkipEmptyParts);
                        for (QStringList::Iterator it1 = tempList.begin(); it1 != tempList.end(); ++it1)
                        {
                            label = (*it1);
                            nodeSum++;
                            createRandomNodes(nodeSum, label, 1);
                        }
                    }
                    else
                    {
                        // multiple  col label lines were found
                        qDebug() << "Nodes have not been created yet."
                                 << "Multiple col label lines were found."
                                 << "Calling createRandomNodes(1) for each label";
                        for (QStringList::Iterator it1 = colLabels.begin(); it1 != colLabels.end(); ++it1)
                        {
                            label = (*it1);
                            nodeSum++;
                            createRandomNodes(nodeSum, label, 1);
                        }
                    }
                }

                // sanity check
                if (!twoMode_flag && nodeSum != totalNodes)
                {

                    qDebug() << "❌ ERROR: Number of nodes processed (" << nodeSum
                             << ") does not match declared N=" << totalNodes;
                    errorMessage = tr("Error reading UCINET-formatted file: Number of nodes found (%1) does not match declared N=%2")
                                       .arg(nodeSum)
                                       .arg(totalNodes);
                    return false;
                }

                nodesCreated_flag = true;

            } // endif nodesCreated

            if (fullmatrixFormat)
            {

                if (!twoMode_flag)
                {

                    qDebug() << "reading edges in fullmatrix format";

                    // SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS)
                    if (!prevLineStr.isEmpty())
                    {
                        str = (prevLineStr.append(" ")).append(str);
                        qDebug() << "prevLineStr not empty - "
                                    "prepending it to str - new str: \n"
                                 << str;
                        str = str.simplified();
                    }
                    qDebug() << "splitting str to elements ";
                    myRegExp.setPattern("\\s+");
                    lineElement = str.split(myRegExp, Qt::SkipEmptyParts);
                    qDebug() << "line elements " << lineElement.size();
                    if (lineElement.size() != totalNodes)
                    {
                        qDebug() << "❌ ERROR: Mismatch in matrix row size at line" << fileLineNumber
                                 << ". Expected" << totalNodes << "columns but found" << lineElement.size();
                        qDebug() << "🔍 Full row content: " << str;

                        errorMessage = tr("Matrix row size mismatch. Expected %1 but got %2 at line %3.")
                                           .arg(totalNodes)
                                           .arg(lineElement.size())
                                           .arg(fileLineNumber);
                        return false;
                    }
                    prevLineStr.clear();
                    target = 1;
                    if (source == 1 && relationCounter > 0)
                    {
                        qDebug() << "we are at source 1. "
                                    "Checking relationList";
                        relation = relationsList[relationCounter];
                        qDebug() << "WE ARE THE FIRST DATASET/MATRIX"
                                 << "source node counter is" << source
                                 << "and relation to:" << relation << "index:"
                                 << relationCounter << "signaling to change to that relation...";
                        emit signalSetRelation(relationCounter);
                    }
                    else if (source > totalNodes)
                    {
                        source = 1;
                        relationCounter++;
                        relation = relationsList[relationCounter];
                        qDebug() << "LOOKS LIKE WE ENTERED A NEW DATASET/MATRIX "
                                 << " init source node counter to" << source
                                 << " and relation to" << relation << ": "
                                 << relationCounter << "signaling to change to that relation...";
                        emit signalSetRelation(relationCounter);
                    }
                    else
                    {
                        qDebug() << "source node counter is " << source;
                    }

                    for (QStringList::Iterator it1 = lineElement.begin(); it1 != lineElement.end(); ++it1)
                    {

                        edgeStr = (*it1);
                        edgeWeight = (*it1).toDouble(&conversionOK);
                        if (!conversionOK)
                        {
                            errorMessage = tr("Problem interpreting UCINET fullmatrix-formatted file. "
                                              "In edge (%1->%2), the weight (%3) could not be converted to number, at line %4.")
                                               .arg(source)
                                               .arg(target)
                                               .arg(edgeWeight)
                                               .arg(fileLineNumber);
                            return false;
                        }

                        // Properly handle diagonal elements based on diagonalPresent flag
                        if (source == target)
                        {
                            // This is a diagonal element (self-loop)
                            qDebug() << "Diagonal element at (" << source << "," << target
                                     << ") with value " << edgeWeight;

                            if (diagonalPresent && edgeWeight > 0)
                            {
                                // Create self-loop only if DIAGONAL PRESENT and value is non-zero
                                qDebug() << "Creating self-loop for node " << source;
                                emit signalCreateEdge(source, target, edgeWeight, initEdgeColor,
                                                      EdgeType::Directed, arrows, bezier);
                                totalLinks++;
                            }
                        }
                        else
                        {
                            // Non-diagonal element - normal edge
                            if (edgeWeight > 0)
                            {
                                qDebug() << "relation" << relationCounter
                                         << "Adding edge from " << source << " to " << target
                                         << " with weight " << edgeWeight;
                                emit signalCreateEdge(source, target, edgeWeight, initEdgeColor,
                                                      EdgeType::Directed, arrows, bezier);
                                totalLinks++;
                                qDebug() << "TotalLinks= " << totalLinks;
                            }
                        }
                        target++;
                    } // end for

                    source++;
                }
                else
                {
                    // two-mode
                    target = NR + 1;
                    qDebug() << "this is a two-mode fullmatrix file. "
                                "Splitting str to elements:";
                    myRegExp.setPattern("\\s+");
                    lineElement = str.split(myRegExp, Qt::SkipEmptyParts);
                    qDebug() << "lineElement:" << lineElement;
                    if (lineElement.size() != NC)
                    {
                        qDebug() << "Not a two-mode fullmatrix UCINET "
                                    "formatted file. Aborting!!";
                        // emit something...
                        errorMessage = tr("Problem interpreting UCINET two-mode fullmatrix-formatted file. The file declared %1 columns initially, "
                                          "but I found a different number %2 of matrix columns, at line %3.")
                                           .arg(QString::number(NC))
                                           .arg(QString::number(lineElement.size()))
                                           .arg(fileLineNumber);
                        return false;
                    }
                    for (QStringList::Iterator it1 = lineElement.begin(); it1 != lineElement.end(); ++it1)
                    {

                        edgeStr = (*it1);
                        edgeWeight = (*it1).toDouble(&conversionOK);
                        if (!conversionOK)
                        {
                            errorMessage = tr("Problem interpreting UCINET two-mode file. "
                                              "In edge (%1->%2), the weight (%3) cannot be converted to number, at line %4.")
                                               .arg(source)
                                               .arg(target)
                                               .arg(edgeWeight)
                                               .arg(fileLineNumber);
                            return false;
                        }

                        if (edgeWeight)
                        {

                            qDebug() << "relation "
                                     << relationCounter
                                     << "found edge from "
                                     << source << " to " << target
                                     << "weight " << edgeWeight
                                     << "signaling to create new edge";

                            emit signalCreateEdge(source, target, edgeWeight, initEdgeColor, EdgeType::Directed, arrows, bezier);

                            totalLinks++;
                            qDebug() << "TotalLinks= " << totalLinks;
                        }
                        target++;
                    } // end for

                    source++;
                }

            } // END FULLMATRIX FORMAT READING

            if (edgelist1Format)
            {
                // read edges in edgelist1 format
                myRegExp.setPattern("\\s+");
                lineElement = str.split(myRegExp, Qt::SkipEmptyParts);
                qDebug() << "edgelist str line:" << str;
                qDebug() << "edgelist data element:" << lineElement;
                if (lineElement.size() != 3)
                {
                    qDebug() << "Not an edgelist1 UCINET "
                                "formatted file. Aborting!!";
                    // emit something...
                    errorMessage = tr("Problem interpreting UCINET-formatted file. "
                                      "The file was declared as edgelist but I found "
                                      "a line which did not have 3 elements (source, target, weight), at line %1")
                                       .arg(fileLineNumber);
                    return false;
                }

                source = (lineElement[0]).toInt(&intOK);
                target = (lineElement[1]).toInt(&intOK);

                qDebug() << "source node "
                         << source << " target node " << target;

                edgeWeight = (lineElement[2]).toDouble(&conversionOK);

                if (conversionOK)
                {
                    qDebug() << "list file declares edge weight: "
                             << edgeWeight;
                }
                else
                {
                    edgeWeight = 1.0;
                    qDebug() << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
                }

                qDebug() << "Signaling to create new edge"
                         << source << "->" << target << " weight= " << edgeWeight
                         << " TotalLinks=  " << totalLinks + 1;
                emit signalCreateEdge(source, target, edgeWeight, initEdgeColor, EdgeType::Directed,
                                      arrows, bezier);
                totalLinks++;
            } // END edgelist1 format reading.

        } // end if data_flag

    } // end while there are more lines

    if (relationsList.isEmpty())
    {
        // QString defaultRelation = "DefaultRelation_" + QString::number(QDateTime::currentSecsSinceEpoch());
        // relationsList << defaultRelation;
        // emit signalAddNewRelation(defaultRelation);
        emit signalAddNewRelation("unnamed");
    }

    // The network has been loaded. Change to the first relation
    emit signalSetRelation(0);

    // Clear temp arrays
    lineElement.clear();
    tempList.clear();
    rowLabels.clear();
    colLabels.clear();
    relationsList.clear();

    qDebug() << "Finished OK. Returning.";
    return true;
}

/**
 * @brief Reads and parses DL keywords from a given QStringList.
 *
 * This function processes a list of strings to extract and interpret DL keywords.
 * It updates the provided references with the parsed values.
 *
 * @param strList A reference to a QStringList containing the DL keywords.
 * @param N A reference to an integer to store the parsed value of 'N'.
 * @param NM A reference to an integer to store the parsed value of 'NM'.
 * @param NR A reference to an integer to store the parsed value of 'NR'.
 * @param NC A reference to an integer to store the parsed value of 'NC'.
 * @param fullmatrixFormat A reference to a boolean to indicate if the format is 'FULLMATRIX'.
 * @param edgelist1Format A reference to a boolean to indicate if the format is 'edgelist'.
 * @return true if all keywords are successfully parsed and valid, false otherwise.
 */
bool Parser::readDLKeywords(QStringList &strList,
                            int &N,
                            int &NM,
                            int &NR,
                            int &NC,
                            bool &fullmatrixFormat,
                            bool &edgelist1Format,
                            bool &diagonalPresent)
{
    QStringList tempList;
    QString tempStr = QString();
    QString label = QString();
    QString value = QString();
    bool intOK = false;

    for (QStringList::Iterator it1 = strList.begin(); it1 != strList.end(); ++it1)
    {
        tempStr = (*it1);
        qDebug() << "element:" << tempStr.toLatin1();

        if (tempStr.startsWith("DL", Qt::CaseInsensitive))
        {
            // remove DL
            tempStr.remove("DL", Qt::CaseInsensitive);
            tempStr = tempStr.simplified();
            qDebug() << "element contained DL. Removed it:"
                     << tempStr;
        }

        // check if this element contains a "="
        if (tempStr.size() > 0)
        {

            if (tempStr.contains("=", Qt::CaseInsensitive))
            {
                qDebug() << "splitting element at = sign";

                tempList = tempStr.split("=", Qt::SkipEmptyParts);

                label = tempList[0].simplified();
                value = tempList[1].simplified();

                if (label == "n" || label == "N")
                {
                    qDebug() << "N is declared to be : "
                             << value;
                    N = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "N conversion error...";
                        // emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert N value to integer. ");
                        return false;
                    }
                }
                else if (label == "nm" || label == "NM")
                {
                    qDebug() << "NM is declared to be : "
                             << value;
                    NM = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NM conversion error...";
                        // emit something here...
                        errorMessage = tr("Problem interpreting UCINET file. Cannot convert NM value to integer. ");
                        return false;
                    }
                }
                else if (label == "nr" || label == "NR")
                {
                    qDebug() << "NR is declared to be : "
                             << value;
                    NR = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NR conversion error...";
                        // emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert NR value to integer.");
                        return false;
                    }
                }
                else if (label == "nc" || label == "NC")
                {
                    qDebug() << "NC is declared to be : "
                             << value;
                    NC = value.toInt(&intOK, 10);
                    if (!intOK)
                    {
                        qDebug() << "NC conversion error...";
                        // emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert NC value to integer. ");
                        return false;
                    }
                }
                else if (label == "format" || label == "FORMAT")
                {
                    qDebug() << "FORMAT is declared to be : "
                             << value;

                    // Check if DIAGONAL PRESENT is specified in the format
                    if (value.contains("DIAGONAL PRESENT", Qt::CaseInsensitive))
                    {
                        diagonalPresent = true;
                        qDebug() << "✅ DIAGONAL PRESENT detected in format";
                    }
                    if (value.contains("FULLMATRIX", Qt::CaseInsensitive))
                    {
                        fullmatrixFormat = true;
                        edgelist1Format = false;
                        qDebug() << "✅ FORMAT: FullMatrix detected";
                    }
                    else if (value.contains("edgelist", Qt::CaseInsensitive))
                    {
                        edgelist1Format = true;
                        fullmatrixFormat = false;
                        qDebug() << "✅ FORMAT: EdgeList detected";
                    }
                    else
                    {
                        qDebug() << "❌ ERROR: Unknown DL format. Expected 'FULLMATRIX' or 'edgelist'.";
                        errorMessage = tr("Invalid UCINET format declaration. Expected 'FULLMATRIX' or 'edgelist' but found: %1").arg(value);
                        return false;
                    }

                } // end format
            } // end if contains =
            else
            {
                return false;
            }

        } // end if > 0

    } // end for lineElement
    return true;
}

/**
 * @brief Parses the data as Pajek-formatted
 *
 * @param rawData
 * @return
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

        if (isComment(str)) {
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
                relation = relation.simplified();
                relationsList << relation;
                qDebug() << "added new relation" << relation
                         << "to relationsList - signaling to add new relation";
                emit signalAddNewRelation(relation);
                lastRelationIndex = relationsList.size() - 1;
                if (lastRelationIndex > 0)
                {
                    qDebug() << "last relation index:"
                             << lastRelationIndex
                             << "signaling to change to the last relation...";
                    emit signalSetRelation(lastRelationIndex);
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
            qDebug() << str;
            arcs_flag = false;
            edges_flag = false;
            arcslist_flag = false;
            matrix_flag = true;
            // check if row has label for matrix data,
            //  and use it as relation name
            if ((pos = str.indexOf(":")) != -1)
            {
                relation = str.right(str.size() - pos - 1);
                relation = relation.simplified();
                relationsList << relation;
                qDebug() << "added new relation" << relation
                         << "to relationsList - signaling to add new relation";
                emit signalAddNewRelation(relation);
                lastRelationIndex = relationsList.size() - 1;
                if (lastRelationIndex > 0)
                {
                    qDebug() << "last relation index:"
                             << lastRelationIndex
                             << "signaling to change to the last relation...";
                    emit signalSetRelation(lastRelationIndex);
                    i = 0; // reset the source node index
                }
            }
            continue;
        }

        /** READING NODES */
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
                    emit signalCreateNode(num,
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
            emit signalCreateNode(
                nodeNum, initNodeSize, nodeColor,
                initNodeNumberColor, initNodeNumberSize,
                label, initNodeLabelColor, initNodeLabelSize,
                QPointF(randX, randY),
                nodeShape, QString());
            initNodeColor = nodeColor;
        }
        // NODES CREATED. CREATE EDGES/ARCS NOW.
        // first check that all nodes are already created
        else
        {
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
                    emit signalCreateNode(
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
                emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                      EdgeType::Undirected, arrows, bezier, edgeLabel);
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
                emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                      EdgeType::Directed, arrows, bezier, edgeLabel);
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
                    emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                          EdgeType::Directed, arrows, bezier);
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
                        emit signalCreateEdge(source, target + 1, edgeWeight, edgeColor,
                                              EdgeType::Directed, arrows, bezier);
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
            emit removeDummyNode(*it);
        }
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation(networkName);
    }

    qDebug() << "Clearing temporary dummies and relations list";
    listDummiesPajek.clear();
    relationsList.clear();

    qDebug() << "signaling to change to the first relation...";
    emit signalSetRelation(0);

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
 * Main function to parse adjacency-formatted data.
 * Validates the format, resets counters, and processes the file to create nodes and edges.
 * If `sm_has_labels` is true, the first comment line is treated as node labels.
 * NOTE: Parsing is aborted if any invalid data is encountered.
 *
 * Example of a supported adjacency matrix file with node labels:
 * 
 *```
 * # Alice, Bob, Charlie
 * 0, 1, 1
 * 1, 0, 0
 * 1, 0, 0
 *```
 * In this example:
 * - The first line is a comment containing the node labels: Alice, Bob, Charlie.
 * - The remaining lines form a 3x3 adjacency matrix where:
 *     - Row 1 corresponds to Alice
 *     - Row 2 corresponds to Bob
 *     - Row 3 corresponds to Charlie
 * - A "1" indicates an edge (e.g., Alice is connected to Bob and Charlie).
 * - A "0" indicates no edge (e.g., Bob is not connected to Charlie).
 *
 * @param rawData Raw input data as QByteArray.
 * @param delimiter Delimiter used to split rows and columns.
 * @param sm_has_labels Indicates if the sociomatrix has labels in the first comment line.
 * @return true if parsing succeeds, false otherwise.
 */
bool Parser::parseAsAdjacency(const QByteArray &rawData, const QString &delimiter, const bool &sm_has_labels)
{
    qDebug() << "Parsing data as adjacency formatted... delimiter: " << delimiter;

    // Decode the data and prepare a QTextStream for processing.
    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QStringList nodeLabels; // Stores node labels if `sm_has_labels` is true.
    QString str;

    // Validate the input data.
    if (!validateAndInitialize(rawData, delimiter, sm_has_labels, nodeLabels))
    {
        return false;
    }

    // Reset the QTextStream for processing the adjacency matrix.
    ts.seek(0);

    // Reset internal counters and data structures.
    resetCounters();

    // Process the file to create nodes and edges.
    if (!doParseAdjacency(ts, delimiter, nodeLabels))
    {
        return false; // Abort if any error occurs during node or edge creation.
    }

    // If no relations are defined, add a default unnamed relation.
    if (relationsList.empty())
    {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}

/**
 * Validates the adjacency matrix file format and, optionally, gets node labels from first line (if it is a comment line).
 * Checks for reserved keywords, row consistency, and appropriate delimiters in the first 11 rows.
 * Parsing is aborted immediately if any issue is encountered.
 * @param rawData Raw input data as QByteArray.
 * @param delimiter Delimiter used to split rows and columns.
 * @return true if the file format is valid, false otherwise.
 */
bool Parser::validateAndInitialize(const QByteArray &rawData, const QString &delimiter, const bool &sm_has_labels, QStringList &nodeLabels)
{
    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str;

    int fileLine = 0, actualLineNumber = 0, lastCount = 0;

    while (actualLineNumber < 11 && !ts.atEnd())
    {
        fileLine++;
        str = ts.readLine().simplified().trimmed();

        // Check for reserved keywords to ensure the file is adjacency-formatted.
        if (containsReservedKeywords(str))
        {
            errorMessage = tr("Invalid adjacency-formatted file. "
                              "Non-comment line %1 includes reserved keywords ('%2'). Parsing aborted.")
                               .arg(fileLine)
                               .arg(str);
            return false;
        }

        if (isComment(str))
        {

            // Get node labels from first line, if `sm_has_labels` is true.
            if (fileLine == 1 && sm_has_labels)
            {
                str.remove(QRegularExpression("^#\\s*")); // Removes '#' and any trailing spaces
                nodeLabels = str.split(delimiter);
                if (nodeLabels.isEmpty())
                {
                    errorMessage = tr("Invalid Adjacency-formatted file. "
                                      "Node labels line is empty or improperly formatted. Parsing aborted.");
                    return false;
                }
                qDebug() << "Parsed node labels:" << nodeLabels;
                break;
            }

            continue;
        }

        actualLineNumber++;
        int colCount = str.split(delimiter).size();

        // Ensure that all rows have consistent column counts.
        if ((colCount != lastCount && actualLineNumber > 1) || (colCount < actualLineNumber))
        {
            errorMessage = tr("Invalid Adjacency-formatted file. "
                              "Row %1 at line %2 has a different number of elements (%3) than expected (%4). Parsing aborted.")
                               .arg(actualLineNumber)
                               .arg(fileLine)
                               .arg(colCount)
                               .arg(lastCount);
            return false;
        }

        lastCount = colCount;
    }

    qDebug() << "Validation successful. Proceeding.";
    return true;
}

/**
 * Resets counters and data structures used during parsing.
 * Clears relations and resets node and edge counters to ensure a clean state.
 */
void Parser::resetCounters()
{
    relationsList.clear();
    totalNodes = 0;
    edgeWeight = 1.0;
    totalLinks = 0;
    edgeDirType = EdgeType::Directed;
}

/**
 * Processes the adjacency matrix file to create nodes and edges.
 * Reads each line of the matrix, creates nodes for the first row, and creates edges for subsequent rows.
 * Uses `nodeLabels` to assign labels to nodes if provided.
 * Parsing is aborted immediately if any issue is encountered.
 * @param ts QTextStream of the decoded adjacency matrix file.
 * @param delimiter Delimiter used to split rows and columns.
 * @param nodeLabels List of node labels (optional). If empty, numeric labels are used.
 * @return true if the nodes and edges are successfully created, false otherwise.
 */
bool Parser::doParseAdjacency(QTextStream &ts, const QString &delimiter, const QStringList &nodeLabels)
{
    QString str;
    QStringList currentRow;
    int fileLine = 0, actualLineNumber = 0;

    while (!ts.atEnd())
    {
        fileLine++;
        str = ts.readLine().simplified().trimmed();

        // Skip comment lines but count them for accurate line tracking.
        if (isComment(str))
        {
            qDebug() << tr("fileLine: %1 is a comment...").arg(fileLine);
            continue;
        }

        actualLineNumber++;
        currentRow = str.split(delimiter);

        if (actualLineNumber == 1)
        {
            // Initialize nodes based on the first row of the adjacency matrix.
            totalNodes = currentRow.size();
            qDebug() << "Nodes to be created:" << totalNodes;

            // Create each node, assigning random positions and labels.
            for (int j = 1; j <= totalNodes; ++j)
            {
                QString label = (j <= nodeLabels.size()) ? nodeLabels[j - 1] : QString::number(j);
                createNodeWithDefaults(j, label);
            }

            qDebug() << "Finished creating nodes";
        }

        // If more rows exist than declared nodes, create additional nodes.
        if (actualLineNumber > totalNodes)
        {
            createNodeWithDefaults(actualLineNumber, QString::number(actualLineNumber));
        }

        // Abort if a row contains more columns than the expected NxN matrix format.
        if (currentRow.size() > totalNodes)
        {
            errorMessage = tr("Invalid Adjacency-formatted file. "
                              "Not a NxN matrix. Row %1 declares %2 edges. Expected: %3. Parsing aborted.")
                               .arg(actualLineNumber)
                               .arg(currentRow.size())
                               .arg(totalNodes);
            return false;
        }

        // Create edges for the current row of the matrix.
        if (!createEdgesForRow(currentRow, actualLineNumber))
        {
            return false; // Abort if edge creation fails.
        }
    }

    return true;
}

/**
 * Emits a signal to create a node with the specified index and label, and default node properties.
 * Assigns a random position for the node within the graph dimensions.
 * @param nodeIndex Index of the node to create.
 * @param label Label for the node (numerical or custom).
 */
void Parser::createNodeWithDefaults(int nodeIndex, const QString &label)
{
    // Assign a random position for the node within the graph dimensions.
    // QPointF randomPosition(rand() % gwWidth, rand() % gwHeight);
    QPointF randomPosition(QRandomGenerator::global()->bounded(gwWidth), QRandomGenerator::global()->bounded(gwHeight));

    // Emit a signal to create the node with the given properties.
    emit signalCreateNode(nodeIndex, initNodeSize, initNodeColor, initNodeNumberColor, initNodeNumberSize,
                          label, initNodeLabelColor, initNodeLabelSize, randomPosition, initNodeShape, QString());
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

            emit signalCreateEdge(rowIndex, colIndex, edgeWeight, initEdgeColor, EdgeType::Directed, true, false);
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
        emit signalCreateNode(i, initNodeSize, initNodeColor,
                              initNodeNumberColor, initNodeNumberSize,
                              QString::number(i), initNodeLabelColor, initNodeLabelSize,
                              QPointF(randX, randY),
                              initNodeShape, QString());
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
                        emit signalCreateEdge(i, k, edgeWeight, initEdgeColor, EdgeType::Undirected, arrows, bezier);
                        totalLinks++;
                    }
                }
            }
            j++;
        }
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation("unnamed");
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
    initNodeCustomIcon="";
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
    emit signalSetRelation(0);

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
    emit signalAddNewRelation(networkName);
    int lastRelationIndex = relationsList.size() - 1;
    if (lastRelationIndex > 0)
    {
        totalNodes = 0;
        qDebug() << "last relation index:"
                 << lastRelationIndex
                 << "signaling to change to the new relation";
        emit signalSetRelation(lastRelationIndex);
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
    keyDefaultValue[key_id] = key_value; // key_id is already stored

    qDebug() << "Reading default key values - key default value is"
             << key_value;

    if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node")
    {
        qDebug() << "key default value" << key_value << "is for node size";
        conv_OK = false;
        initNodeSize = key_value.toInt(&conv_OK);
        if (!conv_OK)
            initNodeSize = 8;
    }
    if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node")
    {
        qDebug() << "key default value" << key_value << "is for nodes shape";
        initNodeShape = key_value;
    }
    if (keyName.value(key_id) == "custom-icon" && keyFor.value(key_id) == "node")
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
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node")
    {
        qDebug() << "key default value" << key_value << "is for nodes color";
        initNodeColor = key_value;
    }
    if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node")
    {
        qDebug() << "key default value" << key_value << "is for node labels color";
        initNodeLabelColor = key_value;
    }
    if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node")
    {
        qDebug() << "key default value" << key_value << "is for node labels size";
        conv_OK = false;
        initNodeLabelSize = key_value.toInt(&conv_OK);
        if (!conv_OK)
            initNodeLabelSize = 8;
    }
    if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge")
    {
        qDebug() << "key default value" << key_value << "is for edges weight";
        conv_OK = false;
        initEdgeWeight = key_value.toDouble(&conv_OK);
        if (!conv_OK)
            initEdgeWeight = 1;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge")
    {
        qDebug() << "key default value" << key_value << "is for edges color";
        initEdgeColor = key_value;
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
        emit signalCreateNode(totalNodes,
                              nodeSize,
                              nodeColor,
                              nodeNumberColor,
                              nodeNumberSize,
                              nodeLabel,
                              nodeLabelColor,
                              nodeLabelSize,
                              QPointF(randX, randY),
                              nodeShape,
                              (nodeIconPath.isEmpty() ? initNodeCustomIcon : nodeIconPath));
    }
    else
    {
        emit signalCreateNode(totalNodes,
                              nodeSize,
                              nodeColor,
                              nodeNumberColor,
                              nodeNumberSize,
                              nodeLabel,
                              nodeLabelColor,
                              nodeLabelSize,
                              QPointF(randX, randY),
                              nodeShape,
                              QString());
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
    emit signalCreateEdge(source, target, edgeWeight, edgeColor, edgeDirType,
                          arrows, bezier, edgeLabel);
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

    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node color: "
                 << key_value << " for this node";
        nodeColor = key_value;
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node label: "
                    ""
                 << key_value << " for this node";
        nodeLabel = key_value;
    }
    else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node")
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
    else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node")
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
    else if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node size: "
                 << key_value << " for this node";
        conv_OK = false;
        nodeSize = key_value.toInt(&conv_OK);
        if (!conv_OK)
            nodeSize = initNodeSize;
        qDebug() << "Using: " << nodeSize;
    }
    else if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node label size: "
                 << key_value << " for this node";
        conv_OK = false;
        nodeLabelSize = key_value.toInt(&conv_OK);
        if (!conv_OK)
            nodeLabelSize = initNodeLabelSize;
        qDebug() << "Using: " << nodeSize;
    }
    else if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node label Color: "
                 << key_value << " for this node";
        nodeLabelColor = key_value;
    }
    else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node shape: "
                 << key_value << " for this node";
        nodeShape = key_value;
    }
    else if (keyName.value(key_id) == "custom-icon" && keyFor.value(key_id) == "node")
    {
        qDebug() << "Data found. Node custom-icon path: "
                 << key_value << " for this node";
        nodeIconPath = key_value;
        nodeIconPath = fileDirPath + ("/") + nodeIconPath;
        qDebug() << "full node custom-icon path: "
                 << nodeIconPath;
    }
    else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge")
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
    else if ((keyName.value(key_id) == "value" || keyName.value(key_id) == "weight") && keyFor.value(key_id) == "edge")
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
    else if (keyName.value(key_id) == "size of arrow" && keyFor.value(key_id) == "edge")
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
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "edge")
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

                emit signalCreateEdge(source, target, edgeWeight, edgeColor, edgeDirType, arrows, bezier, edgeLabel);
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
 * @param rawData
 * @return bool
 */
bool Parser::parseAsGML(const QByteArray &rawData)
{

    qDebug() << "Parsing data as GML formatted...";

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
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

        qDebug() << "line" << fileLine << ":"
                 << str;

        if (isComment(str))
            continue;

        actualLineNumber++;

        if (actualLineNumber == 1 &&
            (str.contains("vertices", Qt::CaseInsensitive) || str.contains("network", Qt::CaseInsensitive) || str.contains("digraph", Qt::CaseInsensitive) || str.contains("DL n", Qt::CaseInsensitive) || str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) || str.contains("graphml", Qt::CaseInsensitive) || str.contains("xml", Qt::CaseInsensitive)))
        {
            qDebug() << "*** Not a GML-formatted file. Aborting!!";
            errorMessage = tr("Not an GML-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats  (i.e vertices, graphml, network, digraph, DL, xml)")
                               .arg(fileLine);

            return false;
        }

        if (str.startsWith("comment", Qt::CaseInsensitive))
        {
            qDebug() << "This is a comment. Continue.";
            continue;
        }
        if (str.startsWith("creator", Qt::CaseInsensitive))
        {
            qDebug() << "This is a creator description. Continue.";
            continue;
        }
        else if (str.startsWith("graph", Qt::CaseInsensitive))
        {
            // describe a graph
            qDebug() << "graph description list start";
            graphKey = true;
        }
        else if (str.startsWith("directed", Qt::CaseInsensitive))
        {
            // graph attribute declarations
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
        }
        else if (str.startsWith("isPlanar", Qt::CaseInsensitive))
        {
            // key declarations
            if (graphKey)
            {
                if (str.contains("1"))
                {
                    qDebug() << "graph isPlanar 1. Planar graph.";
                    isPlanar = true;
                }
                else
                {
                    isPlanar = false;
                }
            }
        }

        else if (str.startsWith("node", Qt::CaseInsensitive))
        {
            // node declarations
            qDebug() << "node description list starts";
            nodeKey = true;
        }
        else if (str.startsWith("id", Qt::CaseInsensitive))
        {
            // describes identification number for an object
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
        }

        else if (str.startsWith("label ", Qt::CaseInsensitive))
        {
            // describes label
            if (nodeKey)
            {
                nodeLabel = str.split(" ", Qt::SkipEmptyParts).last().remove("\"");
                qDebug() << "node label definition"
                         << "node" << totalNodes
                         << "id" << node_id
                         << "label" << nodeLabel;

                // FIXME REMOVE ANY "
            }
            else if (edgeKey)
            {
                edgeLabel = str.split(" ", Qt::SkipEmptyParts).last();
                qDebug() << "edge label definition"
                         << "edge" << totalLinks
                         << "label" << edgeLabel;
            }
        }

        else if (str.startsWith("edge ", Qt::CaseInsensitive))
        {
            // edge declarations
            qDebug() << "edge description list start";
            edgeKey = true;
            totalLinks++;
            // Reset edge attributes
            edgeWeight = 1.0; // Default weight
            edgeColor = "black";
            edgeLabel.clear();
        }
        else if (str.startsWith("source ", Qt::CaseInsensitive))
        {
            if (edgeKey)
            {
                edge_source = str.split(" ", Qt::SkipEmptyParts).last();
                // if edge_source
                if (!edge_source.contains(onlyDigitsExp))
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge source tag at line %1 has non-arithmetic value.")
                                       .arg(fileLine);
                    return false;
                }
                source = edge_source.toInt(0);
                qDebug() << "edge source definition"
                         << "edge source" << edge_source;
            }
        }
        else if (str.startsWith("target ", Qt::CaseInsensitive))
        {
            if (edgeKey)
            {
                edge_target = str.split(" ", Qt::SkipEmptyParts).last();
                if (!edge_source.contains(onlyDigitsExp))
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge target tag at line %1 has non-arithmetic value.")
                                       .arg(fileLine);
                    return false;
                }

                target = edge_target.toInt(0);
                qDebug() << "edge target definition"
                         << "edge target" << edge_target;
            }
        }

        else if (str.startsWith("weight ", Qt::CaseInsensitive))
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
        }

        else if (str.startsWith("graphics", Qt::CaseInsensitive))
        {
            // Describes graphics which are used to draw a particular object.
            if (nodeKey)
            {
                qDebug() << "node graphics description list start";
            }
            else if (edgeKey)
            {
                qDebug() << "edge graphics description list start";
            }
            graphicsKey = true;
        }
        else if (str.startsWith("center", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                qDebug() << "node graphics center start";
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
                        qDebug() << "node graphics center"
                                 << "x" << randX
                                 << "y" << randY;
                        fileContainsNodeCoords = true;
                    }
                    else
                    {
                        graphicsCenterKey = true;
                    }
                }
            }
        }
        else if (str.startsWith("center", Qt::CaseInsensitive) &&
                 nodeKey && graphicsKey && graphicsCenterKey)
        {
            // this is the case where the bracker [ is below the center tag
        }
        else if (str.startsWith("type", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                qDebug() << "node graphics type start";
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
        }
        else if (str.startsWith("fill", Qt::CaseInsensitive))
        {
            if (graphicsKey && nodeKey)
            {
                qDebug() << "node graphics fill start";
                nodeColor = str.split(" ", Qt::SkipEmptyParts).last();
                if (nodeColor.isNull() || nodeColor.isEmpty())
                {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node fill tag at line %1 has no value.")
                                       .arg(fileLine);
                    return false;
                }
            }
        }
        else if (str.startsWith("]", Qt::CaseInsensitive))
        {
            if (nodeKey && graphicsKey && graphicsCenterKey)
            {
                qDebug() << "node graphics center ends";
                graphicsCenterKey = false;
            }
            else if (graphicsKey)
            {
                qDebug() << "graphics list ends";
                graphicsKey = false;
            }
            else if (nodeKey && !graphicsKey)
            {
                qDebug() << "node description list ends";
                nodeKey = false;
                if (!fileContainsNodeCoords)
                {
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                }
                qDebug() << " *** Signaling to create new node " << node_id
                         << " at " << randX << "," << randY
                         << " label " << nodeLabel;
                emit signalCreateNode(
                    node_id.toInt(0), initNodeSize, nodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    nodeLabel, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    nodeShape, QString());
            }
            else if (edgeKey && !graphicsKey)
            {
                qDebug() << "edge description list ends. signaling to create new edge.";
                edgeKey = false;

                if (edgeLabel == QString())
                {
                    edgeLabel = edge_source + "->" + edge_target;
                }
                emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                      edgeDirType, arrows, bezier, edgeLabel);
            }

            else if (graphKey)
            {
                qDebug() << "graph description list ends";
                graphKey = false;
            }
        }
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}

/**
 * @brief Parses the data as GraphViz (dot) formatted network.
 *
 * @param rawData
 * @return
 */
bool Parser::parseAsDot(const QByteArray &rawData)
{

    qDebug() << "Parsing data as dot (Graphviz) formatted...";

    int fileLine = 0;
    int actualLineNumber = 0;
    int aNum = -1;
    int start = 0;
    int end = 0;
    int next = 0;
    QString label, node, nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel, networkLabel;
    QString str, temp, prop, value;
    QStringList lineElement;
    nodeColor = "red";
    edgeColor = "black";
    nodeShape = "";
    edgeWeight = 1.0;
    qreal nodeValue = 1.0;
    bool netProperties = false;
    QStringList labels;
    QList<QString> nodeSequence;    // holds edges
    QList<QString> nodesDiscovered; // holds nodes;

    relationsList.clear();

    edgeDirType = EdgeType::Directed;
    arrows = true;
    bezier = false;
    source = 0;
    target = 0;

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData).trimmed();

    // Abort if the file does not contain any valid GraphViz (dot) data
    if (!decodedData.contains("digraph", Qt::CaseInsensitive) && !decodedData.contains("graph", Qt::CaseInsensitive))
    {
        qDebug() << "Not a valid GraphViz (dot) file. Aborting!";
        errorMessage = tr("Invalid GraphViz (dot) file. The file does not contain 'digraph' or 'graph'.");
        return false;
    }
    
    // Apply preprocessing to handle single-line DOT files properly
    decodedData = preprocessDotContent(decodedData);

    QTextStream ts(&decodedData);

    totalNodes = 0;
    totalLinks = 0;
    
    while (!ts.atEnd())
    {

        fileLine++;
        qDebug() << "🔎 Reading fileLine " << fileLine;

        str = ts.readLine().simplified().trimmed();

        qDebug() << str;

        if (isComment(str))
            continue;

        actualLineNumber++;

        // Verify that:
        // a) There is no reserved keyword from other formats in the first non-comment line
        // b) The first non-comment line starts with "digraph" or "graph"
        if (actualLineNumber == 1)
        {
            if (str.contains("vertices", Qt::CaseInsensitive)                              // Pajek
                || str.contains("network", Qt::CaseInsensitive)                            // Pajek?
                || str.contains("[", Qt::CaseInsensitive)                                  // GML
                || str.contains("DL n", Qt::CaseInsensitive)                               // DL format
                || str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) // list
                || str.startsWith("<graphml", Qt::CaseInsensitive)                         // GraphML
                || str.startsWith("<?xml", Qt::CaseInsensitive))
            {
                qDebug() << "*** Not an GraphViz -formatted file. Aborting";

                errorMessage = tr("Not a GraphViz-formatted file. "
                                  "First non-comment line includes keywords reserved by other file formats  (i.e vertices, graphml, network, DL, xml).");
                return false;
            }
            
            if (str.contains("digraph", Qt::CaseInsensitive))
            {
                lineElement = str.split(" ");
                edgeDirType = EdgeType::Directed;
                if (lineElement[1] != "{")
                    networkName = lineElement[1];
                qDebug() << "This is a DOT DIGRAPH named " << networkName;
                continue;
            }
            else if (str.contains("graph", Qt::CaseInsensitive))
            {
                lineElement = str.split(" ");
                edgeDirType = EdgeType::Undirected;
                if (lineElement[1] != "{")
                    networkName = lineElement[1];
                qDebug() << "This is a DOT GRAPH named " << networkName;
                continue;
            }
            else
            {
                qDebug() << " *** Not a GraphViz file. "
                            "Abort: dot format can only start with \" (di)graph netname {\"";
                errorMessage = tr("Not properly GraphViz-formatted file. "
                                  "First non-comment line should start with \" (di)graph netname {\"");
                return false;
            }
        }

        if (str.contains("graph [", Qt::CaseInsensitive))
        {
            netProperties = true;
            qDebug() << "🔵 Detected global graph settings. Skipping...";
            Q_UNUSED(netProperties);
        }
        else if (
            str.startsWith("label", Qt::CaseInsensitive) 
            || str.startsWith("mincross", Qt::CaseInsensitive) 
            || str.startsWith("ratio", Qt::CaseInsensitive) 
            || str.startsWith("name", Qt::CaseInsensitive) 
            || str.startsWith("type", Qt::CaseInsensitive) 
            || str.startsWith("loops", Qt::CaseInsensitive) 
            || str.startsWith("rankdir", Qt::CaseInsensitive) 
            || str.startsWith("splines", Qt::CaseInsensitive) 
            || str.startsWith("overlap", Qt::CaseInsensitive) 
            || str.startsWith("nodesep", Qt::CaseInsensitive) 
            || str.startsWith("ranksep", Qt::CaseInsensitive) 
            || str.startsWith("size", Qt::CaseInsensitive) // Handle size attribute
        )
        { 
            qDebug() << "🔵 Detected global graph settings. Parsing...";
            next = str.indexOf('=', 1);
            qDebug("Found next = at %i. Start is at %i", next, 1);
            prop = str.mid(0, next).simplified();
            qDebug() << "Prop: " << prop.toLatin1();
            value = str.right(str.size() - next - 1).simplified();
            qDebug() << "Value " << value;
            if (prop == "label" || prop == "name")
            {
                networkLabel = value;
            }
            else if (prop == "ratio")
            {
            }
            else if (prop == "size")
            {
                qDebug() << "⚠️ Ignoring 'size' attribute: " << value;
                // Graphviz size= controls graph scaling, but in SocNetV we don't use it.
            }
            else if (prop == "mincross")
            {
            }
        }
        else if (netProperties && str.contains("]", Qt::CaseInsensitive))
        {
            netProperties = false;
        }
        else if (str.startsWith("node [", Qt::CaseInsensitive))
        {
            qDebug() << "🔵 Detected global node settings...";
            readDotProperties(str.mid(str.indexOf('[') + 1, str.indexOf(']') - str.indexOf('[') - 1),
                              nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor);
            qDebug() << "✅ Default node color set to: " << initNodeColor;
        }
        else if (str.startsWith("edge [", Qt::CaseInsensitive))
        {
            qDebug() << "🔵 Detected global edge settings...";
            readDotProperties(str.mid(str.indexOf('[') + 1, str.indexOf(']') - str.indexOf('[') - 1),
                              edgeWeight, edgeLabel, edgeShape, initEdgeColor, fontName, fontColor);
            qDebug() << "✅ Default edge color set to: " << initEdgeColor;
        }
        else if (!str.startsWith('[', Qt::CaseInsensitive) &&
                 !str.contains("--", Qt::CaseInsensitive) &&
                 !str.contains("->", Qt::CaseInsensitive) &&
                 str.contains("[", Qt::CaseInsensitive) &&
                 !netProperties)
        {
            qDebug() << "🔵 A single node definition must be here: \n"
                     << str;
            // Node definitions in the format: nodeName [properties]
            int start = str.indexOf('[');
            if (start != -1)
            {
                // Extract node name and properties
                QString node = str.left(start).simplified();
                node = node.remove('\"');
                qDebug() << "node named " << node.toLatin1();

                // Extract properties
                int end = str.lastIndexOf(']');
                QString temp;
                if (end != -1)
                {
                    temp = str.mid(start + 1, end - start - 1);
                    qDebug() << "node properties " << temp.toLatin1();

                    // Set default label to node name
                    nodeLabel = node;

                    // Process properties
                    readDotProperties(temp, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor);

                    // If no label was specified, use the node name
                    if (nodeLabel.isEmpty())
                        nodeLabel = node;

                    totalNodes++;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    qDebug() << "🌟 Signaling to create new node " << totalNodes
                             << "at " << randX << "," << randY
                             << " label " << node.toLatin1()
                             << " colored " << initNodeColor
                             << "initNodeSize " << initNodeSize
                             << "initNodeNumberColor " << initNodeNumberColor
                             << "initNodeNumberSize " << initNodeNumberSize
                             << "initNodeLabelColor " << initNodeLabelColor
                             << "nodeShape" << initNodeShape;
                    emit signalCreateNode(
                        totalNodes, initNodeSize, initNodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        nodeLabel, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        initNodeShape, QString());
                    // Note that we push the numbered nodelabel whereas we create
                    // the node with its file specified node label.
                    nodesDiscovered.push_back(node);
                    qDebug() << " * Total totalNodes " << totalNodes
                             << " nodesDiscovered  " << nodesDiscovered.size();
                    target = totalNodes;
                }
                else
                {
                    qDebug("* ERROR!");
                    errorMessage = tr("Not properly GraphViz-formatted file. "
                                      "Node definition without closing ]");
                    return false;
                }
            }
            else
            {
                qDebug("* ERROR!");
                errorMessage = tr("Not properly GraphViz-formatted file. "
                                  "Node definition without opening [");
                return false;
            }
        }
        else if (
            !str.contains('[', Qt::CaseInsensitive) && 
            !str.contains("node", Qt::CaseInsensitive) && 
            !str.contains(']', Qt::CaseInsensitive) && 
            !str.contains("--", Qt::CaseInsensitive) && 
            !str.contains("->", Qt::CaseInsensitive) && 
            !str.contains("=", Qt::CaseInsensitive) && !netProperties)
        {
            qDebug() << "🔵 A node definition without properties must be here ..." << str;
            end = str.indexOf(';');
            if (end != -1)
            {
                node = str.left(str.size() - end); // keep the properties
                qDebug() << "node named " << node.toLatin1();
                node = node.remove(']').remove(';').remove('\"');
                qDebug() << "node named " << node.toLatin1();
                nodeLabel = node;
                totalNodes++;
                randX = rand() % gwWidth;
                randY = rand() % gwHeight;
                qDebug() << " 🌟 Signaling to create new node " << totalNodes
                         << " at " << randX << "," << randY
                         << " label " << node.toLatin1()
                         << " colored " << initNodeColor
                         << "initNodeSize " << initNodeSize
                         << "initNodeNumberColor " << initNodeNumberColor
                         << "initNodeNumberSize " << initNodeNumberSize
                         << "initNodeLabelColor " << initNodeLabelColor
                         << "nodeShape" << initNodeShape;
                emit signalCreateNode(
                    totalNodes, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    nodeLabel, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
                nodesDiscovered.push_back(node); // Note that we push the numbered nodelabel whereas we create the node with its file specified node label.
                qDebug() << " * Total nodes" << totalNodes << " nodesDiscovered  " << nodesDiscovered.size();
                target = totalNodes;
            }
            else
                end = str.indexOf(';');
            qDebug("* Finished node!");
        }

        else if (str.contains('-', Qt::CaseInsensitive))
        {
            netProperties = false;
            qDebug("🔵 Edge definition found ...");
            end = str.indexOf('[');
            if (end != -1)
            {
                temp = str.right(str.size() - end - 1); // keep the properties
                qDebug("  Edge with properties - reading properties...");
                temp = temp.remove(']');
                temp = temp.remove(';');
                qDebug() << "edge properties " << temp.toLatin1();
                readDotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor);
                initEdgeColor = edgeColor;
            }
            else
            {
                qDebug("  Edge without properties...");
                edgeLabel = "";
                edgeColor = initEdgeColor;
                edgeWeight = initEdgeWeight;
                end = str.indexOf(';');
            }

            // FIXME Cannot parse nodes named with '-' character
            str = str.mid(0, end).remove('\"'); // keep only edges

            qDebug() << "edge " << str.toLatin1();

            if (!str.contains("->", Qt::CaseInsensitive))
            { // non directed = symmetric links
                if (str.contains("--", Qt::CaseInsensitive))
                    nodeSequence = str.split("--");
                else
                    nodeSequence = str.split("-");
                edgeDirType = EdgeType::Undirected;
            }
            else
            { // is directed
                nodeSequence = str.split("->");
                edgeDirType = EdgeType::Directed;
            }
            // Create all nodes defined in nodeSequence
            for (QList<QString>::iterator it = nodeSequence.begin(); it != nodeSequence.end(); it++)
            {
                node = (*it).simplified();
                qDebug() << " nodeSequence node " << node;
                if ((aNum = nodesDiscovered.indexOf(node)) == -1)
                {
                    // node not discovered before
                    totalNodes++;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    qDebug() << "🌟 Signaling to create new node" << totalNodes
                             << "at" << QPointF(randX, randY)
                             << "label" << node.toLatin1()
                             << "colored " << nodeColor
                             << "initNodeSize " << initNodeSize
                             << "initNodeNumberColor " << initNodeNumberColor
                             << "initNodeNumberSize " << initNodeNumberSize
                             << "initNodeLabelColor " << initNodeLabelColor
                             << "nodeShape" << initNodeShape;
                    emit signalCreateNode(
                        totalNodes, initNodeSize, nodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        node, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        initNodeShape, QString());
                    nodesDiscovered.push_back(node);
                    qDebug() << " * Total totalNodes "
                             << totalNodes << " nodesDiscovered  " << nodesDiscovered.size();
                    target = totalNodes;
                    if (it != nodeSequence.begin())
                    {
                        totalLinks++;
                        qDebug() << "🔗 signaling to create new edge:"
                                 << source << "->" << target << "totalLinks:" << totalLinks;
                        emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                              edgeDirType, arrows, bezier);
                    }
                }
                else
                {
                    // node discovered before
                    target = aNum + 1;
                    qDebug("# Node already exists. Vector num: %i ", target);
                    if (it != nodeSequence.begin())
                    {
                        totalLinks++;
                        qDebug() << "🔗 signaling to create new edge"
                                 << source << "->" << target << "totalLinks:" << totalLinks;
                        emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                              edgeDirType, arrows, bezier);
                        
                    }
                }
                source = target;
            }
            nodeSequence.clear();
            qDebug("Finished reading fileLine %i ", fileLine);
        }
        else if (str.contains("[", Qt::CaseInsensitive) && str.contains("=") && !netProperties)
        { // Default node properties - no node keyword

            qDebug("* Node properties found but with no Node keyword in the beginning!");
            start = str.indexOf('[');
            end = str.indexOf(']');
            temp = str.mid(start + 1, end - start - 1); // keep whatever is inside [ and ]
            qDebug() << "Properties start at " << start << " and end at " << end;
            temp = temp.simplified();
            qDebug() << temp.toLatin1();
            readDotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor);
            qDebug("Finished the properties!");

            if (start > 2)
            { // there is a node definition here
                node = str.left(start).remove('\"').simplified();
                qDebug() << "node label: " << node.toLatin1() << ".";
                if (!nodesDiscovered.contains(node))
                {
                    qDebug("not discovered node");
                    totalNodes++;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    qDebug() << "🌟 Signaling to create new node at"
                             << randX << "," << randY << "label" << node.toLatin1()
                             << " colored " << nodeColor;
                    emit signalCreateNode(
                        totalNodes, initNodeSize, nodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        label, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        nodeShape, QString());
                    aNum = totalNodes;
                    nodesDiscovered.push_back(node);
                    qDebug() << " Total totalNodes: " << totalNodes << " nodesDiscovered = " << nodesDiscovered.size();
                }
                else
                {
                    qDebug("discovered node - skipping it!");
                }
            }
        }

        else
        {
            qDebug() << "  Redudant data: " << str.toLatin1();
        }
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation((!networkName.isEmpty()) ? networkName : "unnamed");
    }

    qDebug() << "Parser::parseAsDot() - Finished OK. Returning.";
    return true;
}


/**
 * @brief Preprocesses the content of a DOT file to normalize its formatting, improve parsing and readability.
 *
 * This function performs several transformations on the input DOT content:
 * - Converts escaped newlines (&#92;n) into actual newlines.
 * - Adds newlines after opening braces `{` and before closing braces `}`.
 * - Adds newlines after each closing bracket `]` and semicolon `;`.
 * - Normalizes brackets by adding spaces around them.
 * - Ensures there is a semicolon `;` between consecutive node definitions.
 * - Adds spaces around edge definitions (`->` and `--`).
 * - Handles node and edge attribute blocks more carefully by adding newlines before them.
 * - Merges edge attributes written on separate lines with their respective edge definitions.
 *
 * @param dotContent The original content of the DOT file as a QString.
 * @return A QString containing the preprocessed DOT content.
 */
QString Parser::preprocessDotContent(const QString &dotContent) {
    QString processedData = dotContent;
    
    // Convert escaped newlines (&#92;n) into actual newlines
    processedData.replace("&#92;n", "\\n");
    
    // Add newline after opening brace `{`
    processedData.replace(QRegularExpression("\\{\\s*"), "{\n  ");
    
    // Add newline after each closing bracket `]` (but don't add semicolon if one already exists)
    // This effectively splits node definitions into separate lines and ensures they end with a semicolon
    processedData.replace(QRegularExpression("\\](\\s*)(?!;)"), "];\n  ");
    processedData.replace(QRegularExpression("\\];(\\s*)"), "];\n  ");
    
    // Add newline after each semicolon `;`
    processedData.replace(";", ";\n  ");
    
    // Add newline before closing brace `}`
    processedData.replace(QRegularExpression("\\s*\\}"), "\n}");
    
    // Normalize brackets - add spaces around them for better parsing
    processedData.replace("[", " [ ");
    processedData.replace("]", " ] ");
    
    // Ensure there is a semicolon `;` between consecutive node definitions
    processedData.replace(QRegularExpression("(\\]\\s*)(struct\\d+)"), "];\n  \\2");
    
    // Add spaces around edge definitions
    processedData.replace("->", " -> ");
    processedData.replace("--", " -- ");
    
    // Handle node and edge attribute blocks more carefully
    processedData.replace(QRegularExpression("\\bnode\\s*\\["), "\nnode [");
    processedData.replace(QRegularExpression("\\bedge\\s*\\["), "\nedge [");
    
    // Handle cases where an edge attribute is written on a separate line
    QStringList lines = processedData.split('\n');
    QStringList processedLines;
    bool previousLineWasEdge = false;
    QString previousLine;
    
    for (const QString &line : lines) {
        QString currentLine = line.trimmed();
        
        if (currentLine.isEmpty()) {
            continue; // Skip empty lines
        }
        
        // Check if this line contains only attributes (starts with '[' but not a node/edge definition)
        if (currentLine.startsWith('[') && 
            !currentLine.contains("->") && 
            !currentLine.contains("--") && 
            !currentLine.startsWith("node") && 
            !currentLine.startsWith("edge") &&
            previousLineWasEdge) {
            
            // Merge edge attributes with previous edge definition
            QString combinedLine = previousLine;
            if (combinedLine.endsWith(';')) {
                combinedLine.chop(1);
            }
            
            combinedLine += " " + currentLine;
            
            // Replace the previous line with the combined line
            processedLines.removeLast();
            processedLines.append(combinedLine);
            
            previousLineWasEdge = false;
        } else {
            // Check if this is an edge definition line
            previousLineWasEdge = (currentLine.contains("->") || currentLine.contains("--"));
            previousLine = currentLine;
            
            // Add the current line as-is
            processedLines.append(currentLine);
        }
    }
    
    // Return the processed data
    return processedLines.join('\n');
}



/**
 * @brief Reads the properties of a dot element with improved handling of quoted values
 * @param str String containing properties (format: "prop1=value1, prop2=value2, ...")
 * @param nValue Output variable for numeric value property
 * @param label Output variable for label property
 * @param shape Output variable for shape property
 * @param color Output variable for color property
 * @param fontName Output variable for font name property
 * @param fontColor Output variable for font color property
 */
void Parser::readDotProperties(QString str, qreal &nValue, QString &label,
                               QString &shape, QString &color, QString &fontName,
                               QString &fontColor)
{
    qDebug() << "Reading DOT properties from:" << str;

    str = str.simplified();

    // Process properties one by one
    while (!str.isEmpty())
    {
        int equalPos = str.indexOf('=');
        if (equalPos == -1)
        {
            break; // No more properties
        }

        // Extract property name
        QString prop = str.left(equalPos).simplified();
        str = str.mid(equalPos + 1).simplified();

        // Check for quoted value
        QString value;
        if (str.startsWith('\"'))
        {
            // Handle quoted value (which might contain commas)
            int endQuotePos = -1;
            bool escaped = false;

            // Find the matching end quote, accounting for escaped quotes
            for (int i = 1; i < str.length(); i++)
            {
                if (str.at(i) == '\\')
                {
                    escaped = !escaped;
                }
                else if (str.at(i) == '\"' && !escaped)
                {
                    endQuotePos = i;
                    break;
                }
                else
                {
                    escaped = false;
                }
            }

            if (endQuotePos != -1)
            {
                value = str.mid(1, endQuotePos - 1); // Extract without quotes
                str = str.mid(endQuotePos + 1).simplified();
            }
            else
            {
                // Malformed - no closing quote
                qDebug() << "Warning: No closing quote found in property value";
                value = str.mid(1);
                str.clear();
            }

            // Handle unescape if needed
            value = value.replace("\\\"", "\"");
        }
        else
        {
            // Handle unquoted value (which ends at next comma or end of string)
            int commaPos = str.indexOf(',');
            if (commaPos != -1)
            {
                value = str.left(commaPos).simplified();
                str = str.mid(commaPos + 1).simplified();
            }
            else
            {
                value = str.simplified();
                str.clear();
            }
        }

        // Skip any leading comma in remaining string
        if (str.startsWith(','))
        {
            str = str.mid(1).simplified();
        }

        qDebug() << "Parsed property:" << prop << "=" << value;

        // Process the property
        bool ok = false;
        if (prop == "label")
        {
            label = value;
            qDebug() << "Set label to:" << label;
        }
        else if (prop == "fontname")
        {
            fontName = value;
            qDebug() << "Set fontName to:" << fontName;
        }
        else if (prop == "value")
        {
            nValue = value.toFloat(&ok);
            if (ok)
            {
                qDebug() << "Set value to:" << nValue;
            }
            else
            {
                qDebug() << "Error converting value:" << value;
            }
        }
        else if (prop == "color" || prop == "fillcolor")
        {
            color = value;
            qDebug() << "Set color to:" << color;
        }
        else if (prop == "fontcolor")
        {
            fontColor = value;
            qDebug() << "Set fontColor to:" << fontColor;
        }
        else if (prop == "shape")
        {
            shape = value;
            qDebug() << "Set shape to:" << shape;
        }
        else if (prop == "weight")
        {
            nValue = value.toFloat(&ok);
            if (ok)
            {
                qDebug() << "Set weight to:" << nValue;
            }
            else
            {
                qDebug() << "Error converting weight:" << value;
            }
        }
        else if (prop == "style")
        {
            qDebug() << "Style property:" << value << "(currently not used)";
        }
        else
        {
            qDebug() << "Ignoring unknown property:" << prop << "=" << value;
        }
    }
}

/**
 * Debugging only - Used while parsing weighted edge lists
 */
template <typename T>
void print_queue(T &q)
{
    qDebug() << "print_queue() ";
    while (!q.empty())
    {
        qDebug() << q.top().key << " value: " << q.top().value << " ";
        q.pop();
    }
    qDebug() << "\n";
}

/**
 * @brief Parses the data as weighted edgelist formatted network
 *
 *  This method can read and parse edgelist formated files
 * where edge source and target are either named with numbers or with labels
 * That is the following formats can be parsed:
 * # edgelist with node numbers
 * 1 2 1
 * 1 3 2
 * 1 6 2
 * 1 8 2
 * ...
 *
 * # edgelist with node labels
 * actor1 actor2 1
 * actor2 actor4 2
 * actor1 actor3 1
 * actorX actorY 3
 * name othername 1
 * othername somename 2
 * ...

 * @param rawData
 * @param delimiter
 * @return
 */
bool Parser::parseAsEdgeListWeighted(const QByteArray &rawData, const QString &delimiter)
{

    qDebug() << "Parsing data as weighted edgelist formatted..." << "column delimiter" << delimiter;

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, qreal> edgeList;
    QString str, edgeKey, edgeKeyDelimiter = "====>";
    QStringList lineElement, edgeElement;
    // one or more digits
    QRegularExpression onlyDigitsExp("^\\d+$");

    bool nodesWithLabels = false;
    bool conversionOK = false;
    int fileLine = 0, actualLineNumber = 0;
    totalNodes = 0;
    edgeWeight = 1.0;
    edgeDirType = EdgeType::Directed;
    arrows = true;
    bezier = false;

    relationsList.clear();

    qDebug() << "***  Initial file parsing "
                "to test integrity and edge naming scheme";
    while (!ts.atEnd())
    {

        fileLine++;

        str = ts.readLine().simplified().trimmed();

        qDebug() << " simplified str" << str;

        if (isComment(str))
            continue;

        actualLineNumber++;

        if (
            actualLineNumber == 1 &&

            (str.contains("vertices", Qt::CaseInsensitive) || str.contains("network", Qt::CaseInsensitive) || str.contains("graph", Qt::CaseInsensitive) || str.contains("digraph", Qt::CaseInsensitive) || str.contains("DL n", Qt::CaseInsensitive) // DL format
             || str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) || str.contains("graphml", Qt::CaseInsensitive) || str.contains("xml", Qt::CaseInsensitive)))
        {
            qDebug() << "Not a Weighted list-formatted file. Aborting!!";

            errorMessage = tr("Not an EdgeList-formatted file. "
                              "A non-comment line includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)");
            return false;
        }

        lineElement = str.split(delimiter);

        if (lineElement.size() != 3)
        {
            qDebug() << "*** Not a Weighted list-formatted file. Aborting!!";

            errorMessage = tr("Not a properly EdgeList-formatted file. "
                              "Row %1 has not 3 elements as expected (i.e. source, target, weight)")
                               .arg(fileLine);
            return false;
        }

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug() << " Dissecting line - "
                    "source:"
                 << edge_source
                 << "target:"
                 << edge_target
                 << "weight:"
                 << edge_weight;

        if (!edge_source.contains(onlyDigitsExp))
        {
            qDebug() << " node named by non-digit only string. "
                        "nodesWithLabels = true";
            nodesWithLabels = true;
        }

        if (!edge_target.contains(onlyDigitsExp))
        {
            qDebug() << " node named by non-digit only string. "
                        "nodesWithLabels = true";
            nodesWithLabels = true;
        }
    }

    ts.seek(0);
    fileLine = 0;

    qDebug() << "***  Initial file parsing finished. "
                "This is really a weighted edge list. Proceed to main parsing";

    while (!ts.atEnd())
    {
        str = ts.readLine();

        qDebug() << " str" << str;

        str = str.simplified();
        qDebug() << " simplified str" << str;

        if (isComment(str))
            continue;

        lineElement = str.split(delimiter);

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug() << " Dissecting line - "
                    "source:"
                 << edge_source
                 << "target:"
                 << edge_target
                 << "weight:"
                 << edge_weight;

        if (!nodeMap.contains(edge_source))
        {
            totalNodes++;
            Actor sourceActor;
            sourceActor.key = edge_source;
            if (nodesWithLabels)
            {
                sourceActor.value = totalNodes;
                // order by an increasing totalNodes index
                nodeQ.push(sourceActor);
                nodeMap.insert(edge_source, totalNodes);
            }
            else
            {
                sourceActor.value = edge_source.toInt();
                // order by the actual actor number in the file
                nodeQ.push(sourceActor);
                nodeMap.insert(edge_source, edge_source.toInt());
            }
            qDebug() << " source, new node named"
                     << edge_source
                     << "totalNodes" << totalNodes
                     << "nodeMap.count"
                     << nodeMap.size();
        }
        else
        {
            qDebug() << " source already found, continue";
        }
        if (!nodeMap.contains(edge_target))
        {
            totalNodes++;
            Actor targetActor;
            targetActor.key = edge_target;
            if (nodesWithLabels)
            {
                targetActor.value = totalNodes;
                // order by an increasing totalNodes index
                nodeQ.push(targetActor);
                nodeMap.insert(edge_target, totalNodes);
            }
            else
            {
                targetActor.value = edge_target.toInt();
                // order by the actual actor number in the file
                nodeQ.push(targetActor);
                nodeMap.insert(edge_target, edge_target.toInt());
            }
            qDebug() << " target, new node named"
                     << edge_target
                     << "totalNodes" << totalNodes
                     << "nodeMap.count"
                     << nodeMap.size();
        }
        else
        {
            qDebug() << " target already found, continue";
        }

        edgeWeight = edge_weight.toDouble(&conversionOK);
        if (conversionOK)
        {
            qDebug() << " read edge weight"
                     << edgeWeight;
        }
        else
        {
            edgeWeight = 1.0;
            qDebug() << " error reading edge weight."
                        "Using default edgeWeight"
                     << edgeWeight;
        }
        edgeKey = edge_source + edgeKeyDelimiter + edge_target;
        if (!edgeList.contains(edgeKey))
        {
            qDebug() << " inserting edgeKey"
                     << edgeKey
                     << "in edgeList with weight" << edgeWeight;
            edgeList.insert(edgeKey, edgeWeight);
            totalLinks++;
        }

    } // end ts.stream while here

    qDebug() << " finished reading file, "
                "start creating nodes and edges";

    // print_queue(nodeQ);

    // create nodes one by one
    while (!nodeQ.empty())
    {

        Actor node = nodeQ.top();
        nodeQ.pop();
        randX = rand() % gwWidth;
        randY = rand() % gwHeight;

        if (nodesWithLabels)
        {
            qDebug() << "signaling to create new node" << node.value
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode(node.value,
                                  initNodeSize,
                                  initNodeColor,
                                  initNodeNumberColor,
                                  initNodeNumberSize,
                                  node.key,
                                  initNodeLabelColor, initNodeLabelSize,
                                  QPointF(randX, randY),
                                  initNodeShape, QString());
        }
        else
        {

            qDebug() << "signaling to create new node" << node.key.toInt()
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode(node.key.toInt(),
                                  initNodeSize,
                                  initNodeColor,
                                  initNodeNumberColor,
                                  initNodeNumberSize,
                                  node.key,
                                  initNodeLabelColor, initNodeLabelSize,
                                  QPointF(randX, randY),
                                  initNodeShape, QString());
        }
    }

    // create edges one by one
    QHash<QString, qreal>::const_iterator edge = edgeList.constBegin();
    while (edge != edgeList.constEnd())
    {

        qDebug() << " creating edge named"
                 << edge.key() << " weight " << edge.value();

        edgeElement = edge.key().split(edgeKeyDelimiter);
        if (nodesWithLabels)
        {
            source = nodeMap.value(edgeElement[0]);
            target = nodeMap.value(edgeElement[1]);
        }
        else
        {
            source = edgeElement[0].toInt();
            target = edgeElement[1].toInt();
        }
        edgeWeight = edge.value();
        emit signalCreateEdge(source,
                              target,
                              edgeWeight,
                              initEdgeColor,
                              edgeDirType,
                              arrows,
                              bezier);

        ++edge;
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << " END. Returning.";
    return true;
}

/**
 * @brief Parses the data as simple edgelist formatted
 *
 * @param rawData
 * @param delimiter
 * @return bool
 */
bool Parser::parseAsEdgeListSimple(const QByteArray &rawData, const QString &delimiter)
{

    qDebug() << "Parsing data as simple edgelist formatted..." << "column delimiter" << delimiter;

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str, edgeKey, edgeKeyDelimiter = "====>";
    QStringList lineElement, edgeElement;
    int columnCount = 0;
    int fileLine = 0, actualLineNumber = 0;
    bool nodesWithLabels = false;
    //@TODO Always use nodesWithLabels= true

    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, qreal> edgeList;

    QRegularExpression onlyDigitsExp("^\\d+$");

    totalNodes = 0;
    initEdgeWeight = 1.0;

    edgeDirType = EdgeType::Directed;
    arrows = true;
    bezier = false;

    relationsList.clear();

    qDebug() << "***  Initial file parsing "
                "to test integrity and edge naming scheme";

    while (!ts.atEnd())
    {

        fileLine++;

        str = ts.readLine().simplified().trimmed();

        qDebug() << " line " << fileLine
                 << "\n"
                 << str;

        if (isComment(str))
        {
            continue;
        }

        actualLineNumber++;

        if (actualLineNumber == 1 &&
            (str.contains("vertices", Qt::CaseInsensitive) || str.contains("network", Qt::CaseInsensitive) || str.contains("graph", Qt::CaseInsensitive) || str.contains("digraph", Qt::CaseInsensitive) || str.contains("DL n", Qt::CaseInsensitive) || str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) || str.contains("graphml", Qt::CaseInsensitive) || str.contains("xml", Qt::CaseInsensitive)))
        {
            qDebug() << "*** Not an EdgeList-formatted file. Aborting!!";
            errorMessage = tr("Not an EdgeList-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)")
                               .arg(fileLine);

            return false;
        }

        lineElement = str.split(delimiter);

        for (QStringList::Iterator it1 = lineElement.begin(); it1 != lineElement.end(); ++it1)
        {
            edge_source = (*it1);
            if (!edge_source.contains(onlyDigitsExp))
            {
                qDebug() << " node named by non-digit only string. "
                            "nodesWithLabels = true";
                nodesWithLabels = true;
            }
            if (edge_source == "0")
            {
                nodesWithLabels = true;
            }
        }
    }

    ts.seek(0);
    fileLine = 0;

    qDebug() << " Reset and read lines. nodesWithLabels"
             << nodesWithLabels;

    while (!ts.atEnd())
    {
        fileLine++;
        str = ts.readLine();

        str = str.simplified();

        qDebug() << " line" << fileLine
                 << "\n"
                 << str;

        if (isComment(str))
            continue;

        lineElement = str.split(delimiter);

        columnCount = 0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1 != lineElement.end(); ++it1)
        {
            columnCount++;
            if (columnCount == 1)
            { // source node
                edge_source = (*it1);
                qDebug() << " Dissecting line - "
                            "source node:"
                         << edge_source;

                if (!nodeMap.contains(edge_source))
                {

                    totalNodes++;
                    Actor sourceActor;
                    sourceActor.key = edge_source;
                    if (nodesWithLabels)
                    {
                        sourceActor.value = totalNodes;
                        // order by an increasing totalNodes index
                        nodeQ.push(sourceActor);
                        nodeMap.insert(edge_source, totalNodes);
                    }
                    else
                    {
                        sourceActor.value = edge_source.toInt();
                        // order by the actual actor number in the file
                        nodeQ.push(sourceActor);
                        nodeMap.insert(edge_source, edge_source.toInt());
                    }
                    qDebug() << " source, new node named"
                             << edge_source
                             << "totalNodes" << totalNodes
                             << "nodeMap.count"
                             << nodeMap.size();
                }
                else
                {
                    qDebug() << " source already found, continue";
                }
            }
            else
            { // target nodes
                edge_target = (*it1);
                qDebug() << " Dissecting line - "
                            "target node:"
                         << edge_target;

                if (!nodeMap.contains(edge_target))
                {

                    totalNodes++;
                    Actor targetActor;
                    targetActor.key = edge_target;
                    if (nodesWithLabels)
                    {
                        targetActor.value = totalNodes;
                        // order by an increasing totalNodes index
                        nodeQ.push(targetActor);
                        nodeMap.insert(edge_target, totalNodes);
                    }
                    else
                    {
                        targetActor.value = edge_target.toInt();
                        // order by the actual actor number in the file
                        nodeQ.push(targetActor);
                        nodeMap.insert(edge_target, edge_target.toInt());
                    }
                    qDebug() << " target, new node named"
                             << edge_target
                             << "totalNodes" << totalNodes
                             << "nodeMap.count"
                             << nodeMap.size();
                }
                else
                {
                    qDebug() << " target already found, continue";
                }
            }

            if (columnCount > 1)
            {
                edgeKey = edge_source + edgeKeyDelimiter + edge_target;
                if (!edgeList.contains(edgeKey))
                {
                    qDebug() << " inserting edgeKey"
                             << edgeKey
                             << "in edgeList with initial weight" << initEdgeWeight;
                    edgeList.insert(edgeKey, initEdgeWeight);
                    totalLinks++;
                }
                else
                { // if edge already discovered, then increase its weight by 1
                    edgeWeight = edgeList.value(edgeKey);
                    edgeWeight = edgeWeight + 1;
                    qDebug() << " edgeKey"
                             << edgeKey
                             << "found before, adding in edgeList with increased weight"
                             << edgeWeight;

                    edgeList.insert(edgeKey, edgeWeight);
                }
            }

        } // end for QStringList::Iterator

    } // end ts.stream while here

    // create nodes one by one
    while (!nodeQ.empty())
    {

        Actor node = nodeQ.top();
        nodeQ.pop();
        randX = rand() % gwWidth;
        randY = rand() % gwHeight;

        if (nodesWithLabels)
        {
            qDebug() << "signaling to create new node" << node.value
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode(node.value,
                                  initNodeSize,
                                  initNodeColor,
                                  initNodeNumberColor,
                                  initNodeNumberSize,
                                  node.key,
                                  initNodeLabelColor, initNodeLabelSize,
                                  QPointF(randX, randY),
                                  initNodeShape, QString());
        }
        else
        {

            qDebug() << "signaling to create new node"
                     << node.key.toInt()
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode(node.key.toInt(),
                                  initNodeSize,
                                  initNodeColor,
                                  initNodeNumberColor,
                                  initNodeNumberSize,
                                  node.key,
                                  initNodeLabelColor, initNodeLabelSize,
                                  QPointF(randX, randY),
                                  initNodeShape, QString());
        }
    }

    // create edges one by one
    QHash<QString, qreal>::const_iterator edge = edgeList.constBegin();
    while (edge != edgeList.constEnd())
    {

        qDebug() << " creating edge named"
                 << edge.key() << " weight " << edge.value();

        edgeElement = edge.key().split(edgeKeyDelimiter);
        if (nodesWithLabels)
        {
            source = nodeMap.value(edgeElement[0]);
            target = nodeMap.value(edgeElement[1]);
        }
        else
        {
            source = edgeElement[0].toInt();
            target = edgeElement[1].toInt();
        }
        edgeWeight = edge.value();
        emit signalCreateEdge(source,
                              target,
                              edgeWeight,
                              initEdgeColor,
                              edgeDirType,
                              arrows,
                              bezier);

        ++edge;
    }

    if (relationsList.size() == 0)
    {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << " Finished OK. Returning.";
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
