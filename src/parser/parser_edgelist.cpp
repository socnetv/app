/**
 * @file parser_edgelist.cpp
 * @brief Edge-list parsers for SocNetV
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

#include <queue>
#include <vector>

using namespace std;

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
            if (m_parseSink)
            {
                m_parseSink->createNode(node.value,
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
        else
        {

            qDebug() << "signaling to create new node" << node.key.toInt()
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            if (m_parseSink)
            {
                m_parseSink->createNode(node.key.toInt(),
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
        if (m_parseSink)
        {
            m_parseSink->createEdge(source,
                                    target,
                                    edgeWeight,
                                    initEdgeColor,
                                    edgeDirType,
                                    arrows,
                                    bezier);
        }

        ++edge;
    }

    if (relationsList.size() == 0)
    {
        if (m_parseSink)
        {
            m_parseSink->addNewRelation("unnamed");
        }
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
            if (m_parseSink)
            {
                m_parseSink->createNode(node.value,
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
        else
        {

            qDebug() << "signaling to create new node"
                     << node.key.toInt()
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            if (m_parseSink)
            {
                m_parseSink->createNode(node.key.toInt(),
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
        if (m_parseSink)
        {
            m_parseSink->createEdge(source,
                                    target,
                                    edgeWeight,
                                    initEdgeColor,
                                    edgeDirType,
                                    arrows,
                                    bezier);
        }

        ++edge;
    }

    if (relationsList.size() == 0)
    {
        if (m_parseSink)
        {
            m_parseSink->addNewRelation("unnamed");
        }
    }

    qDebug() << " Finished OK. Returning.";
    return true;
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
