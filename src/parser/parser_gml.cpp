/**
 * @file parser_gml.cpp
 * @brief GML parsers for SocNetV
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

