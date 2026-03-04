/**
 * @file parser_dot.cpp
 * @brief GraphViz DOT parsers for SocNetV
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
 * @brief Parses the data as GraphViz (DOT) formatted network.
 *
 * IMPORTANT CONTRACTS / INVARIANTS
 * - DOT has two graph “kinds”:
 *     1) digraph  => default directed, edges use "->"
 *     2) graph    => default undirected, edges use "--"
 *
 * - SocNetV must report the *graph* directedness reliably via Parser::signalFileLoaded
 *   (the Graph::graphFileLoaded slot uses this to set directedness before calling
 *   edgesEnabled()).
 *
 * - Therefore we must NOT use a single variable for two meanings:
 *     (a) the overall graph type (graph/digraph), AND
 *     (b) the last edge’s operator ("--"/"->").
 *
 * Previous bug:
 * - The parser used edgeDirType as both (a) and (b) and also treated any '-' as
 *   an edge indicator. In some cases, this left edgeDirType as Directed even for
 *   undirected files, so Graph::graphFileLoaded computed ties while the graph was
 *   still considered directed, doubling counts in the CLI (issue #187).
 *
 * Fix:
 * - Track graph-level directedness separately (graphDirType) and never overwrite it.
 * - Parse edges only when the line contains "--" or "->" (not merely '-').
 *
 * @param rawData Raw file bytes.
 * @return true on successful parse; false on format errors.
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

    QString label, node, nodeLabel, fontName, fontColor;
    QString edgeShape, edgeColor, edgeLabel, networkLabel;
    QString str, temp, prop, value;
    QStringList lineElement;

    nodeColor = "red";
    edgeColor = "black";
    nodeShape = "";
    edgeWeight = 1.0;
    initEdgeWeight = 1.0;
    qreal nodeValue = 1.0;

    bool netProperties = false;

    QList<QString> nodeSequence;    // holds nodes in an edge chain: a -- b -- c ...
    QList<QString> nodesDiscovered; // holds *raw* node identifiers encountered

    relationsList.clear();

    // Graph-level directedness. Set ONCE from header (graph/digraph) and never overwritten.
    int graphDirType = EdgeType::Directed;

    // Per-edge directedness (derived from operator in each edge statement).
    int edgeTypeThisLine = EdgeType::Directed;

    arrows = true;
    bezier = false;
    source = 0;
    target = 0;

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData).trimmed();

    // Abort early if file does not even contain dot keywords.
    if (!decodedData.contains("digraph", Qt::CaseInsensitive) &&
        !decodedData.contains("graph", Qt::CaseInsensitive))
    {
        qDebug() << "Not a valid GraphViz (dot) file. Aborting!";
        errorMessage = tr("Invalid GraphViz (dot) file. The file does not contain 'digraph' or 'graph'.");
        return false;
    }

    // Normalize to line-based format (single-line DOT etc).
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

        // Header: determine graph kind and optional graph name.
        if (actualLineNumber == 1)
        {
            // Reject obvious non-DOT formats
            if (str.contains("vertices", Qt::CaseInsensitive) ||                           // Pajek
                str.contains("network", Qt::CaseInsensitive) ||                            // Pajek?
                str.contains("[", Qt::CaseInsensitive) ||                                  // GML
                str.contains("DL n", Qt::CaseInsensitive) ||                               // UCINET DL
                str == "DL" || str == "dl" || str.contains("list", Qt::CaseInsensitive) || // list
                str.startsWith("<graphml", Qt::CaseInsensitive) ||                         // GraphML
                str.startsWith("<?xml", Qt::CaseInsensitive))
            {
                qDebug() << "*** Not a GraphViz-formatted file. Aborting";
                errorMessage = tr("Not a GraphViz-formatted file. "
                                  "First non-comment line includes keywords reserved by other file formats (i.e vertices, graphml, network, DL, xml).");
                return false;
            }

            lineElement = str.split(" ", Qt::SkipEmptyParts);

            if (str.contains("digraph", Qt::CaseInsensitive))
            {
                graphDirType = EdgeType::Directed;
                if (lineElement.size() > 1 && lineElement[1] != "{")
                    networkName = lineElement[1];
                qDebug() << "This is a DOT DIGRAPH named " << networkName;
                continue;
            }
            else if (str.contains("graph", Qt::CaseInsensitive))
            {
                graphDirType = EdgeType::Undirected;
                if (lineElement.size() > 1 && lineElement[1] != "{")
                    networkName = lineElement[1];
                qDebug() << "This is a DOT GRAPH named " << networkName;
                continue;
            }
            else
            {
                qDebug() << "*** Not a GraphViz file. "
                            "Abort: dot format can only start with \"(di)graph netname {\"";
                errorMessage = tr("Not properly GraphViz-formatted file. "
                                  "First non-comment line should start with \"(di)graph netname {\"");
                return false;
            }
        }

        // Global graph settings block starts
        if (str.contains("graph [", Qt::CaseInsensitive))
        {
            netProperties = true;
            qDebug() << "🔵 Detected global graph settings. Skipping...";
            continue;
        }

        // Global graph settings (simple key=value)
        if (str.startsWith("label", Qt::CaseInsensitive) ||
            str.startsWith("mincross", Qt::CaseInsensitive) ||
            str.startsWith("ratio", Qt::CaseInsensitive) ||
            str.startsWith("name", Qt::CaseInsensitive) ||
            str.startsWith("type", Qt::CaseInsensitive) ||
            str.startsWith("loops", Qt::CaseInsensitive) ||
            str.startsWith("rankdir", Qt::CaseInsensitive) ||
            str.startsWith("splines", Qt::CaseInsensitive) ||
            str.startsWith("overlap", Qt::CaseInsensitive) ||
            str.startsWith("nodesep", Qt::CaseInsensitive) ||
            str.startsWith("ranksep", Qt::CaseInsensitive) ||
            str.startsWith("size", Qt::CaseInsensitive))
        {
            qDebug() << "🔵 Detected global graph settings. Parsing...";
            next = str.indexOf('=', 1);
            prop = str.mid(0, next).simplified();
            value = str.right(str.size() - next - 1).simplified();

            if (prop == "label" || prop == "name")
                networkLabel = value;
            else if (prop == "size")
                qDebug() << "⚠️ Ignoring 'size' attribute:" << value;

            continue;
        }

        // End of global graph settings block
        if (netProperties && str.contains("]", Qt::CaseInsensitive))
        {
            netProperties = false;
            continue;
        }

        // Global node defaults
        if (str.startsWith("node [", Qt::CaseInsensitive))
        {
            qDebug() << "🔵 Detected global node settings...";
            readDotProperties(
                str.mid(str.indexOf('[') + 1, str.indexOf(']') - str.indexOf('[') - 1),
                nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor);
            qDebug() << "✅ Default node color set to:" << initNodeColor;
            continue;
        }

        // Global edge defaults
        if (str.startsWith("edge [", Qt::CaseInsensitive))
        {
            qDebug() << "🔵 Detected global edge settings...";
            readDotProperties(
                str.mid(str.indexOf('[') + 1, str.indexOf(']') - str.indexOf('[') - 1),
                edgeWeight, edgeLabel, edgeShape, initEdgeColor, fontName, fontColor);
            qDebug() << "✅ Default edge color set to:" << initEdgeColor;
            continue;
        }

        // Node with properties:  nodeName [ ... ]
        if (!str.startsWith('[', Qt::CaseInsensitive) &&
            !str.contains("--", Qt::CaseInsensitive) &&
            !str.contains("->", Qt::CaseInsensitive) &&
            str.contains("[", Qt::CaseInsensitive) &&
            !netProperties)
        {
            qDebug() << "🔵 A single node definition must be here:\n"
                     << str;

            start = str.indexOf('[');
            if (start == -1)
            {
                errorMessage = tr("Not properly GraphViz-formatted file. Node definition without opening [");
                return false;
            }

            node = str.left(start).simplified().remove('\"');
            qDebug() << "node named" << node;

            end = str.lastIndexOf(']');
            if (end == -1)
            {
                errorMessage = tr("Not properly GraphViz-formatted file. Node definition without closing ]");
                return false;
            }

            temp = str.mid(start + 1, end - start - 1);
            nodeLabel = node; // default label

            readDotProperties(temp, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor);
            if (nodeLabel.isEmpty())
                nodeLabel = node;

            totalNodes++;
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink)
            {
                m_parseSink->createNode(
                    totalNodes, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    nodeLabel, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
            }

            nodesDiscovered.push_back(node);
            target = totalNodes;
            continue;
        }

        // Node without properties: nodeName;
        if (!str.contains('[', Qt::CaseInsensitive) &&
            !str.contains("node", Qt::CaseInsensitive) &&
            !str.contains(']', Qt::CaseInsensitive) &&
            !str.contains("--", Qt::CaseInsensitive) &&
            !str.contains("->", Qt::CaseInsensitive) &&
            !str.contains("=", Qt::CaseInsensitive) &&
            !netProperties)
        {
            qDebug() << "🔵 A node definition without properties must be here ..." << str;

            end = str.indexOf(';');
            if (end == -1)
                continue;

            node = str.left(end).remove(']').remove(';').remove('\"').trimmed();
            nodeLabel = node;

            totalNodes++;
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink)
            {
                m_parseSink->createNode(
                    totalNodes, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    nodeLabel, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
            }

            nodesDiscovered.push_back(node);
            target = totalNodes;
            continue;
        }

        // Edge statement: MUST contain either "--" (undirected) or "->" (directed).
        // We do NOT treat any '-' as an edge indicator.
        if (str.contains("--", Qt::CaseInsensitive) || str.contains("->", Qt::CaseInsensitive))
        {
            netProperties = false;
            qDebug() << "🔵 Edge definition found ...";

            // Parse optional attribute block [...]
            end = str.indexOf('[');
            if (end != -1)
            {
                temp = str.right(str.size() - end - 1);
                temp = temp.remove(']').remove(';');
                readDotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor);
                initEdgeColor = edgeColor;
            }
            else
            {
                edgeLabel = "";
                edgeColor = initEdgeColor;
                edgeWeight = initEdgeWeight;
                end = str.indexOf(';');
                if (end == -1)
                    end = str.size();
            }

            // Keep only the edge chain part ("a -- b -- c" or "a -> b -> c")
            // NOTE: Cannot parse nodes named with '-' character (existing FIXME)
            QString edgeChain = str.mid(0, end).remove('\"').trimmed();

            const bool isDirectedEdge = edgeChain.contains("->", Qt::CaseInsensitive);
            const bool isUndirectedEdge = edgeChain.contains("--", Qt::CaseInsensitive);

            if (isDirectedEdge && isUndirectedEdge)
            {
                // Mixed operator on same line is unusual; treat as parse error rather than guessing.
                errorMessage = tr("Invalid DOT edge statement: mixed '->' and '--' operators on the same line.");
                return false;
            }

            if (isUndirectedEdge)
            {
                nodeSequence = edgeChain.split("--", Qt::SkipEmptyParts);
                edgeTypeThisLine = EdgeType::Undirected;
            }
            else
            {
                nodeSequence = edgeChain.split("->", Qt::SkipEmptyParts);
                edgeTypeThisLine = EdgeType::Directed;
            }

            // Create all nodes defined in nodeSequence (and edges between consecutive nodes)
            for (auto it = nodeSequence.begin(); it != nodeSequence.end(); ++it)
            {
                node = (*it).simplified();

                // Create node if unseen
                if ((aNum = nodesDiscovered.indexOf(node)) == -1)
                {
                    totalNodes++;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    if (m_parseSink)
                    {
                        m_parseSink->createNode(
                            totalNodes, initNodeSize, nodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            node, initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX, randY),
                            initNodeShape, QString());
                    }

                    nodesDiscovered.push_back(node);
                    target = totalNodes;
                }
                else
                {
                    target = aNum + 1;
                }

                // Emit edge between previous node (source) and current node (target)
                if (it != nodeSequence.begin())
                {
                    totalLinks++;
                    if (m_parseSink)
                    {
                        m_parseSink->createEdge(source, target, edgeWeight, edgeColor,
                                                edgeTypeThisLine, arrows, bezier);
                    }
                }

                source = target;
            }

            nodeSequence.clear();
            continue;
        }

        // Default node properties - no node keyword
        if (str.contains("[", Qt::CaseInsensitive) && str.contains("=") && !netProperties)
        {
            start = str.indexOf('[');
            end = str.indexOf(']');
            if (start == -1 || end == -1 || end <= start)
                continue;

            temp = str.mid(start + 1, end - start - 1).simplified();
            readDotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor);

            if (start > 2)
            {
                node = str.left(start).remove('\"').simplified();
                if (!nodesDiscovered.contains(node))
                {
                    totalNodes++;
                    randX = rand() % gwWidth;
                    randY = rand() % gwHeight;
                    if (m_parseSink)
                    {
                        m_parseSink->createNode(
                            totalNodes, initNodeSize, nodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            label, initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX, randY),
                            nodeShape, QString());
                    }

                    nodesDiscovered.push_back(node);
                }
            }

            continue;
        }

        qDebug() << "  Redundant data:" << str;
    }

    // Ensure at least one relation exists.
    if (relationsList.size() == 0)
    {
        QString relName = (!networkName.isEmpty()) ? networkName : "unnamed";
        if (m_parseSink)
        {
            m_parseSink->addNewRelation(relName);
        }
    }

    // IMPORTANT: Preserve graph-level directedness for the file.
    // Do NOT leak "last edge operator" into the graph kind.
    edgeDirType = graphDirType;

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
QString Parser::preprocessDotContent(const QString &dotContent)
{
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

    for (const QString &line : lines)
    {
        QString currentLine = line.trimmed();

        if (currentLine.isEmpty())
        {
            continue; // Skip empty lines
        }

        // Check if this line contains only attributes (starts with '[' but not a node/edge definition)
        if (currentLine.startsWith('[') &&
            !currentLine.contains("->") &&
            !currentLine.contains("--") &&
            !currentLine.startsWith("node") &&
            !currentLine.startsWith("edge") &&
            previousLineWasEdge)
        {

            // Merge edge attributes with previous edge definition
            QString combinedLine = previousLine;
            if (combinedLine.endsWith(';'))
            {
                combinedLine.chop(1);
            }

            combinedLine += " " + currentLine;

            // Replace the previous line with the combined line
            processedLines.removeLast();
            processedLines.append(combinedLine);

            previousLineWasEdge = false;
        }
        else
        {
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

