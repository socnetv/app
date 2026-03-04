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
