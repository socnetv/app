/**
 * @file table_export.h
 * @brief Declares TableExport — CSV and JSON serialisers for QAbstractItemModel
 *        (#226).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#pragma once

class QAbstractItemModel;
class QString;

/**
 * @brief Free functions that serialise any QAbstractItemModel to CSV or JSON.
 *
 * Both functions operate on whatever rows and columns the model exposes —
 * pass a QSortFilterProxyModel to export only the currently visible rows, or
 * the raw source model to export everything.
 *
 * Constraint: QtCore only — no widgets, no UI signals.
 */
namespace TableExport {

/**
 * @brief Writes @p model to an RFC 4180 CSV file at @p filePath.
 *
 * The first row is the column headers from headerData(). Each subsequent row
 * is one model row. Fields containing commas, double-quotes, or newlines are
 * enclosed in double-quotes; embedded double-quotes are escaped as "".
 *
 * @return true on success, false if the file could not be opened/written.
 */
bool toCSV(QAbstractItemModel *model, const QString &filePath);

/**
 * @brief Writes @p model to a JSON file at @p filePath.
 *
 * Produces a top-level JSON array of objects. Each object has one key per
 * column, taken from headerData(), with the display-role cell value as the
 * string value.
 *
 * @return true on success, false if the file could not be opened/written.
 */
bool toJSON(QAbstractItemModel *model, const QString &filePath);

} // namespace TableExport
