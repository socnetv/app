/**
 * @file parser_adjacency.cpp
 * @brief Adjacency matrix parsers for SocNetV
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
#include <QRandomGenerator>


/**
 * Main function to parse adjacency-formatted data.
 *
 * Validates the format, resets internal counters, and processes the file
 * to create nodes and edges from an adjacency matrix representation.
 *
 * If `cfg.sm_has_labels` is true, the first comment line is treated as
 * node labels.
 *
 * NOTE: Parsing is aborted if any invalid data is encountered.
 *
 * Example of a supported adjacency matrix file with node labels:
 *
 * ```
 * # Alice, Bob, Charlie
 * 0, 1, 1
 * 1, 0, 0
 * 1, 0, 0
 * ```
 *
 * In this example:
 * - The first line is a comment containing the node labels: Alice, Bob, Charlie.
 * - The remaining lines form a 3x3 adjacency matrix where:
 *   - Row 1 corresponds to Alice
 *   - Row 2 corresponds to Bob
 *   - Row 3 corresponds to Charlie
 * - A "1" indicates an edge (e.g., Alice is connected to Bob and Charlie).
 * - A "0" indicates no edge (e.g., Bob is not connected to Charlie).
 *
 * @param rawData Raw input data as QByteArray.
 * @param cfg Parser configuration (contains format flags and defaults,
 *            including `sm_has_labels`).
 * @param delimiter Delimiter used to split rows and columns.
 * @return true if parsing succeeds, false otherwise.
 */
bool Parser::parseAsAdjacency(const QByteArray &rawData, const ParseConfig &cfg, const QString &delimiter)
{
    qDebug() << "Parsing data as adjacency formatted... delimiter: " << delimiter;

    // Decode the data and prepare a QTextStream for processing.
    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QStringList nodeLabels; // Stores node labels if `cfg.sm_has_labels` is true.
    QString str;

    // Validate the input data.
    if (!validateAndInitialize(rawData, delimiter, cfg.sm_has_labels, nodeLabels))
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
        if (m_parseSink)
        {
            m_parseSink->addNewRelation("unnamed");
        }
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
    if (m_parseSink)
    {
        m_parseSink->createNode(nodeIndex, initNodeSize, initNodeColor, initNodeNumberColor, initNodeNumberSize,
                                label, initNodeLabelColor, initNodeLabelSize, randomPosition, initNodeShape, QString());
    }
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
 * @brief Parses a two-mode (bipartite) sociomatrix file (.2sm / .aff).
 *
 * The file contains a rectangular NR × NC binary (or weighted) matrix where
 * rows = Mode 1 actors (persons, CEOs, …) and columns = Mode 2 actors
 * (events, clubs, …).
 *
 * Behaviour is controlled by two_sm_mode (set from ParseConfig::sm_mode,
 * default = 1):
 *
 *   two_sm_mode == 1  →  Bipartite graph (default)
 *       Creates NR + NC nodes and one undirected edge per non-zero cell.
 *       Mode 1 nodes: numbers 1..NR,       labels "p1".."pNR", initNodeColor / initNodeShape
 *       Mode 2 nodes: numbers NR+1..NR+NC, labels "e1".."eNC", "SkyBlue"   / "diamond"
 *
 *   two_sm_mode == 2  →  Person (Mode-1) projection  [B × Bᵀ]
 *       Creates NR nodes. Connects person i and person k with an undirected
 *       edge whenever they share at least one event (co-membership).
 *
 *   two_sm_mode == 3  →  Event (Mode-2) projection  [Bᵀ × B]
 *       Creates NC nodes. Connects event j and event l with an undirected
 *       edge whenever they share at least one person.
 *
 * @param rawData  Raw bytes of the file.
 * @return true on success, false on parse error.
 */
bool Parser::parseAsTwoModeSociomatrix(const QByteArray &rawData)
{
    qDebug() << "Parsing data as two-mode sociomatrix, two_sm_mode =" << two_sm_mode;

    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName.toLatin1());
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    // ------------------------------------------------------------------ //
    //  Pass 1 — read all data rows into memory, validate structure         //
    // ------------------------------------------------------------------ //
    QVector<QStringList> matrix;
    int fileLine = 0;
    int NC = 0;   // column count fixed by first data row

    while (!ts.atEnd()) {
        fileLine++;
        QString str = ts.readLine().simplified();

        if (isComment(str) || str.isEmpty())
            continue;

        // Reject files that look like another format
        if (str.contains("vertices",  Qt::CaseInsensitive) ||
            str.contains("network",   Qt::CaseInsensitive) ||
            str.contains("graph",     Qt::CaseInsensitive) ||
            str.contains("digraph",   Qt::CaseInsensitive) ||
            str.contains("DL n",      Qt::CaseInsensitive) ||
            str == "DL" || str == "dl" ||
            str.contains("list",      Qt::CaseInsensitive) ||
            str.contains("graphml",   Qt::CaseInsensitive) ||
            str.contains("xml",       Qt::CaseInsensitive))
        {
            qDebug() << "*** Not a two-mode sociomatrix file. Aborting!";
            errorMessage = tr("Invalid two-mode sociomatrix file. "
                              "Non-comment line %1 includes keywords reserved by "
                              "other file formats (vertices, graphml, network, "
                              "graph, digraph, DL, xml)").arg(fileLine);
            return false;
        }

        QStringList row = str.contains(',') ? str.split(',') : str.split(' ');

        if (NC == 0) {
            NC = row.size();
        } else if (row.size() != NC) {
            qDebug() << "*** Ragged matrix at line" << fileLine;
            errorMessage = tr("Invalid two-mode sociomatrix file. "
                              "Row %1 has a different number of elements than "
                              "the first data row.").arg(fileLine);
            return false;
        }

        matrix.append(row);
    }

    const int NR = matrix.size();

    if (NR == 0 || NC == 0) {
        qDebug() << "*** Empty matrix, aborting.";
        errorMessage = tr("Invalid two-mode sociomatrix file: no data rows found.");
        return false;
    }

    qDebug() << "Matrix dimensions: NR =" << NR << "NC =" << NC
             << "two_sm_mode =" << two_sm_mode;

    // ------------------------------------------------------------------ //
    //  Common setup                                                        //
    // ------------------------------------------------------------------ //
    edgeDirType = EdgeType::Undirected;
    arrows      = true;
    bezier      = false;
    totalLinks  = 0;
    totalNodes  = 0;
    firstModeMultiMap.clear();
    secondModeMultiMap.clear();
    relationsList.clear();

    if (m_parseSink)
        m_parseSink->addNewRelation("unnamed");

    // ------------------------------------------------------------------ //
    //  Mode 1 — Bipartite graph                                           //
    // ------------------------------------------------------------------ //
    if (two_sm_mode == 1) {

        // Create Mode-1 nodes (persons / actors)
        for (int i = 1; i <= NR; ++i) {
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink) {
                m_parseSink->createNode(
                    i, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    QString("p%1").arg(i),
                    initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
            }
        }

        // Create Mode-2 nodes (events / groups)
        const QString mode2Color = "SkyBlue";
        const QString mode2Shape = "diamond";
        for (int j = 1; j <= NC; ++j) {
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink) {
                m_parseSink->createNode(
                    NR + j, initNodeSize, mode2Color,
                    initNodeNumberColor, initNodeNumberSize,
                    QString("e%1").arg(j),
                    initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    mode2Shape, QString());
            }
        }

        totalNodes = NR + NC;

        // Create cross-edges for every non-zero cell
        for (int i = 0; i < NR; ++i) {
            for (int j = 0; j < NC; ++j) {
                const QString &cell = matrix[i][j].trimmed();
                if (cell != "0") {
                    bool ok = false;
                    qreal w = cell.toDouble(&ok);
                    edgeWeight = ok ? w : 1.0;
                    qDebug() << "Bipartite edge:" << (i + 1) << "->" << (NR + j + 1)
                             << "weight" << edgeWeight;
                    if (m_parseSink) {
                        m_parseSink->createEdge(
                            i + 1, NR + j + 1, edgeWeight,
                            initEdgeColor, EdgeType::Undirected,
                            arrows, bezier);
                    }
                    totalLinks++;
                }
            }
        }

    // ------------------------------------------------------------------ //
    //  Mode 2 — Person projection  (B × Bᵀ)                              //
    // ------------------------------------------------------------------ //
    } else if (two_sm_mode == 2) {

        // Build firstModeMultiMap: person i (1-indexed) → events attended
        for (int i = 0; i < NR; ++i) {
            for (int j = 0; j < NC; ++j) {
                if (matrix[i][j].trimmed() != "0")
                    firstModeMultiMap.insert(i + 1, j + 1);
            }
        }

        // Create person nodes
        for (int i = 1; i <= NR; ++i) {
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink) {
                m_parseSink->createNode(
                    i, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    QString::number(i),
                    initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
            }
        }

        totalNodes = NR;

        // Connect persons sharing at least one event
        for (int i = 1; i <= NR; ++i) {
            for (int k = 1; k < i; ++k) {
                const QList<int> eventsI = firstModeMultiMap.values(i);
                bool connected = false;
                for (int ev : eventsI) {
                    if (firstModeMultiMap.contains(k, ev)) {
                        connected = true;
                        break;
                    }
                }
                if (connected) {
                    qDebug() << "Person projection: edge" << i << "-" << k;
                    if (m_parseSink) {
                        m_parseSink->createEdge(
                            i, k, 1.0,
                            initEdgeColor, EdgeType::Undirected,
                            arrows, bezier);
                    }
                    totalLinks++;
                }
            }
        }

    // ------------------------------------------------------------------ //
    //  Mode 3 — Event projection  (Bᵀ × B)                               //
    // ------------------------------------------------------------------ //
    } else if (two_sm_mode == 3) {

        // Build secondModeMultiMap: event j (1-indexed) → persons attending
        for (int i = 0; i < NR; ++i) {
            for (int j = 0; j < NC; ++j) {
                if (matrix[i][j].trimmed() != "0")
                    secondModeMultiMap.insert(j + 1, i + 1);
            }
        }

        // Create event nodes
        for (int j = 1; j <= NC; ++j) {
            randX = rand() % gwWidth;
            randY = rand() % gwHeight;
            if (m_parseSink) {
                m_parseSink->createNode(
                    j, initNodeSize, initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    QString::number(j),
                    initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, QString());
            }
        }

        totalNodes = NC;

        // Connect events sharing at least one person
        for (int j = 1; j <= NC; ++j) {
            for (int l = 1; l < j; ++l) {
                const QList<int> personsJ = secondModeMultiMap.values(j);
                bool connected = false;
                for (int p : personsJ) {
                    if (secondModeMultiMap.contains(l, p)) {
                        connected = true;
                        break;
                    }
                }
                if (connected) {
                    qDebug() << "Event projection: edge" << j << "-" << l;
                    if (m_parseSink) {
                        m_parseSink->createEdge(
                            j, l, 1.0,
                            initEdgeColor, EdgeType::Undirected,
                            arrows, bezier);
                    }
                    totalLinks++;
                }
            }
        }

    } else {
        qWarning() << "parseAsTwoModeSociomatrix: unknown two_sm_mode" << two_sm_mode
                   << "- falling back to bipartite (mode 1)";
        two_sm_mode = 1;
        return parseAsTwoModeSociomatrix(rawData);
    }

    qDebug() << "parseAsTwoModeSociomatrix done."
             << "totalNodes" << totalNodes << "totalLinks" << totalLinks;
    return true;
}