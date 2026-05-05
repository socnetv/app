/**
 * @file table_export.cpp
 * @brief Implements TableExport::toCSV() and TableExport::toJSON() (#226).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "table_export.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTextStream>

namespace TableExport {

// Returns a CSV-safe quoted field: wraps in "" if the value contains a
// comma, double-quote, or newline; escapes embedded double-quotes as "".
static QString csvQuote(const QString &value)
{
    if (!value.contains(QLatin1Char(','))
        && !value.contains(QLatin1Char('"'))
        && !value.contains(QLatin1Char('\n'))
        && !value.contains(QLatin1Char('\r')))
    {
        return value;
    }
    QString escaped = value;
    escaped.replace(QLatin1String("\""), QLatin1String("\"\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

bool toCSV(QAbstractItemModel *model, const QString &filePath)
{
    if (!model)
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);

    const int cols = model->columnCount();
    const int rows = model->rowCount();

    // Header row
    QStringList headerFields;
    headerFields.reserve(cols);
    for (int c = 0; c < cols; ++c) {
        headerFields << csvQuote(
            model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString());
    }
    out << headerFields.join(QLatin1Char(',')) << QLatin1Char('\n');

    // Data rows
    for (int r = 0; r < rows; ++r) {
        QStringList rowFields;
        rowFields.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            rowFields << csvQuote(
                model->data(model->index(r, c), Qt::DisplayRole).toString());
        }
        out << rowFields.join(QLatin1Char(',')) << QLatin1Char('\n');
    }

    return true;
}

bool toJSON(QAbstractItemModel *model, const QString &filePath)
{
    if (!model)
        return false;

    const int cols = model->columnCount();
    const int rows = model->rowCount();

    // Collect column headers once
    QStringList headers;
    headers.reserve(cols);
    for (int c = 0; c < cols; ++c) {
        headers << model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
    }

    QJsonArray array;
    for (int r = 0; r < rows; ++r) {
        QJsonObject obj;
        for (int c = 0; c < cols; ++c) {
            obj.insert(headers.at(c),
                       model->data(model->index(r, c), Qt::DisplayRole).toString());
        }
        array.append(obj);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace TableExport
