/**
 * @file table_import.h
 * @brief Declares TableImport — CSV and JSON parsers that return a ParsedTable
 *        suitable for attribute-import workflows (#227).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

#include <QStringList>
#include <QVector>

/**
 * @brief Free functions that parse tabular files (CSV / JSON) into an
 *        in-memory ParsedTable that can be fed to Graph::vertexAttributesImport()
 *        or Graph::edgeAttributesImport().
 *
 * Constraint: QtCore only — no widgets, no UI signals.
 */
namespace TableImport {

/**
 * @brief In-memory representation of a parsed tabular file.
 *
 * @p headers  Column names from the first CSV row or JSON object keys.
 * @p rows     Data rows; each inner list has the same length as @p headers
 *             (shorter rows are right-padded with empty strings during parsing).
 * @p ok       True if parsing succeeded and at least one header was found.
 * @p errorString  Human-readable failure reason when @p ok is false.
 */
struct ParsedTable {
    QStringList         headers;
    QVector<QStringList> rows;
    bool                ok          = false;
    QString             errorString;
};

/**
 * @brief Parses a CSV file at @p filePath into a ParsedTable.
 *
 * The first non-empty line is treated as the header row.
 * Quoted fields (RFC 4180) are handled: embedded double-quotes are written
 * as two consecutive double-quotes ("").
 */
ParsedTable fromCSV(const QString &filePath);

/**
 * @brief Parses a JSON file at @p filePath into a ParsedTable.
 *
 * Expects a top-level JSON array of objects — the format produced by
 * TableExport::toJSON().  Column order follows the key order of the
 * first object in the array.
 */
ParsedTable fromJSON(const QString &filePath);

} // namespace TableImport
