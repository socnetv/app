/**
 * @file parser_pajek.cpp
 * @brief Pajek parsers for SocNetV
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

#include <list>

using namespace std;

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
                    label = normalizeQuotedIdentifier(s); // strip wrapping quotes etc

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

