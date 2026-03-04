/**
 * @file parser_dl.cpp
 * @brief UCINET DL parsers for SocNetV
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
#include "global.h"

SOCNETV_USE_NAMESPACE

#include <QTextCodec>
#include <QRegularExpression>

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
    bool diagonalPresent = false; // Flag to handle diagonal elements (self-loops)

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

                    // Check if DIAGONAL PRESENT is specified
                    if (value.contains("DIAGONAL", Qt::CaseInsensitive))
                    {
                        diagonalPresent = true;
                        qDebug() << "✅ FORMAT: Found standalone DIAGONAL token";
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
        // Check for standalone DIAGONAL line
        else if (str.compare("DIAGONAL", Qt::CaseInsensitive) == 0)
        {
            qDebug() << "✅ Found standalone DIAGONAL line in main parser loop";
            diagonalPresent = true;
            continue;
        }
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
                if (m_parseSink)
                {
                    m_parseSink->addNewRelation(relation);
                }
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

                    // FIX FOR ISSUE #174: Handle wrapped matrix rows
                    // Accumulate wrapped lines until we get totalNodes values
                    QString accumulatedLine = str;
                    myRegExp.setPattern("\\s+");
                    lineElement = accumulatedLine.split(myRegExp, Qt::SkipEmptyParts);
                    qDebug() << "line elements " << lineElement.size();

                    while (lineElement.size() < totalNodes && !ts.atEnd())
                    {
                        QString nextLine = ts.readLine().simplified();
                        if (!nextLine.isEmpty() && !isComment(nextLine))
                        {
                            accumulatedLine += " " + nextLine;
                            lineElement = accumulatedLine.split(myRegExp, Qt::SkipEmptyParts);
                            fileLineNumber++; // Advance line count to keep error reporting consistent
                        }
                    }

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
                        if (m_parseSink)
                        {
                            m_parseSink->setRelation(relationCounter);
                        }
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
                        if (m_parseSink)
                        {
                            m_parseSink->setRelation(relationCounter);
                        }
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

                        // FIX FOR ISSUE #173: Properly handle diagonal elements based on diagonalPresent flag
                        if (source == target)
                        {
                            // This is a diagonal element (self-loop)
                            qDebug() << "Diagonal element at (" << source << "," << target
                                     << ") with value " << edgeWeight;

                            if (diagonalPresent && edgeWeight > 0)
                            {
                                // Create self-loop only if DIAGONAL PRESENT and value is non-zero
                                qDebug() << "Creating self-loop for node " << source;
                                if (m_parseSink)
                                {
                                    m_parseSink->createEdge(source, target, edgeWeight, initEdgeColor,
                                                            EdgeType::Directed, arrows, bezier);
                                }
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
                                if (m_parseSink)
                                {
                                    m_parseSink->createEdge(source, target, edgeWeight, initEdgeColor,
                                                            EdgeType::Directed, arrows, bezier);
                                }

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
                            if (m_parseSink)
                            {
                                m_parseSink->createEdge(source, target, edgeWeight, initEdgeColor, EdgeType::Directed, arrows, bezier);
                            }

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
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, initEdgeColor, EdgeType::Directed,
                                            arrows, bezier);
                }

                totalLinks++;
            } // END edgelist1 format reading.
        } // end if data_flag
    } // end while there are more lines

    if (relationsList.isEmpty())
    {
        if (m_parseSink)
        {
            m_parseSink->addNewRelation("unnamed");
        }
    }

    // The network has been loaded. Change to the first relation
    if (m_parseSink)
    {
        m_parseSink->setRelation(0);
    }

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

        // Check for standalone DIAGONAL token
        if (tempStr.compare("DIAGONAL", Qt::CaseInsensitive) == 0)
        {
            qDebug() << "✅ Found standalone DIAGONAL token";
            diagonalPresent = true;
            continue;
        }

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
                    if (value.contains("DIAGONAL", Qt::CaseInsensitive))
                    {
                        diagonalPresent = true;
                        qDebug() << "✅ FORMAT: DIAGONAL token found in format value";
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
                // We'll be more lenient here - if we encounter unknown tokens without =
                // we'll just ignore them rather than returning false
                qDebug() << "Ignoring unknown token without = sign:" << tempStr;
            }
        } // end if > 0
    } // end for lineElement
    return true;
}

