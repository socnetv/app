/**
 * @file table_import.cpp
 * @brief Implements TableImport::fromCSV() and TableImport::fromJSON() (#227).
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   GNU GPL v3 or later. See <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "table_import.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTextStream>

namespace TableImport {

// Parses one CSV line, respecting RFC 4180 double-quote escaping.
static QStringList parseCSVLine(const QString &line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                fields << field;
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields << field;
    return fields;
}

ParsedTable fromCSV(const QString &filePath)
{
    ParsedTable result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorString = QStringLiteral("Cannot open file: %1").arg(filePath);
        return result;
    }

    QTextStream in(&file);
    bool headerRead = false;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty())
            continue;

        const QStringList fields = parseCSVLine(line);

        if (!headerRead) {
            result.headers = fields;
            headerRead = true;
        } else {
            // Pad short rows so every row has the same column count
            QStringList row = fields;
            while (row.size() < result.headers.size())
                row << QString();
            result.rows << row;
        }
    }

    if (!headerRead || result.headers.isEmpty()) {
        result.errorString = QStringLiteral("Empty or malformed CSV file");
        return result;
    }

    result.ok = true;
    return result;
}

ParsedTable fromJSON(const QString &filePath)
{
    ParsedTable result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorString = QStringLiteral("Cannot open file: %1").arg(filePath);
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.errorString = parseError.errorString();
        return result;
    }

    if (!doc.isArray()) {
        result.errorString = QStringLiteral("Expected a JSON array of objects");
        return result;
    }

    const QJsonArray array = doc.array();
    if (array.isEmpty()) {
        result.errorString = QStringLiteral("Empty JSON array");
        return result;
    }

    // Derive column order from the first object's keys
    result.headers = array.first().toObject().keys();

    for (const QJsonValue &val : array) {
        const QJsonObject obj = val.toObject();
        QStringList row;
        row.reserve(result.headers.size());
        for (const QString &key : std::as_const(result.headers))
            row << obj.value(key).toString();
        result.rows << row;
    }

    result.ok = true;
    return result;
}

} // namespace TableImport
