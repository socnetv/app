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
 *
 * Node attributes example — ID column is "#", remaining columns become attributes:
 * @code
 * #,Label,type,year_founded
 * 1,Alice,investor,2010
 * 2,Bob,founder,2018
 * 3,Carol,advisor,2015
 * @endcode
 *
 * Edge attributes example — Source/Target identify the edge, rest become attributes:
 * @code
 * Source,Target,relationship,weight
 * 1,2,invested_in,0.8
 * 2,3,mentors,0.5
 * 1,3,co_founded,1.0
 * @endcode
 */
ParsedTable fromCSV(const QString &filePath);

/**
 * @brief Parses a JSON file at @p filePath into a ParsedTable.
 *
 * Expects a top-level JSON array of objects — the format produced by
 * TableExport::toJSON().  Column order follows the key order of the
 * first object in the array.
 *
 * Node attributes example:
 * @code
 * [
 *   { "#": "1", "Label": "Alice", "type": "investor", "year_founded": "2010" },
 *   { "#": "2", "Label": "Bob",   "type": "founder",  "year_founded": "2018" }
 * ]
 * @endcode
 *
 * Edge attributes example:
 * @code
 * [
 *   { "Source": "1", "Target": "2", "relationship": "invested_in", "weight": "0.8" },
 *   { "Source": "2", "Target": "3", "relationship": "mentors",     "weight": "0.5" }
 * ]
 * @endcode
 */
ParsedTable fromJSON(const QString &filePath);

} // namespace TableImport
