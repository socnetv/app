// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Common CLI utilities shared by socnetv-cli kernels.
// Keep this file logic-free and deterministic.

#include "cli_common.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

#include <algorithm>
#include <cmath>

namespace cli {

// ---------------- printing ----------------

void printKV(const QString &k, double v)
{
    QTextStream(stdout) << k << "=" << QString::number(v, 'f', 3) << "\n";
}

void printKV(const QString &k, const QString &v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}

void printKV(const QString &k, int v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}

void printKV(const QString &k, qint64 v)
{
    QTextStream(stdout) << k << "=" << v << "\n";
}

// ---------------- deterministic formatting ----------------

QString d2s(double v)
{
    // Deterministic string for golden compare (avoid float parse/format differences).
    return QString::number(v, 'g', 17);
}

// ---------------- JSON I/O ----------------

bool writeJsonFile(const QString &path, const QJsonObject &obj, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (err)
            *err = QString("Could not open for write: %1").arg(path);
        return false;
    }
    const QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool readJsonFile(const QString &path, QJsonObject *outObj, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (err)
            *err = QString("Could not open for read: %1").arg(path);
        return false;
    }

    const QByteArray data = f.readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (doc.isNull() || !doc.isObject())
    {
        if (err)
            *err = QString("Invalid JSON (%1) in %2").arg(pe.errorString(), path);
        return false;
    }

    *outObj = doc.object();
    return true;
}

// ---------------- compare helpers (generic) ----------------

bool cmpStr(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const QString ev = e.value(k).toString();
    const QString av = a.value(k).toString();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << ev << " got=" << av << "\n";
        return false;
    }
    return true;
}

bool cmpInt(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const int ev = e.value(k).toInt();
    const int av = a.value(k).toInt();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << ev << " got=" << av << "\n";
        return false;
    }
    return true;
}

bool cmpBool(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err)
{
    const bool ev = e.value(k).toBool();
    const bool av = a.value(k).toBool();
    if (ev != av)
    {
        err << "MISMATCH " << k << " expected=" << (ev ? "true" : "false")
            << " got=" << (av ? "true" : "false") << "\n";
        return false;
    }
    return true;
}

bool almostEqual(double a, double b, double rel, double abs)
{
    // Treat NaN==NaN as equal for regression purposes
    if (std::isnan(a) && std::isnan(b))
        return true;

    // Treat +Inf==+Inf and -Inf==-Inf as equal
    if (std::isinf(a) || std::isinf(b))
        return a == b;

    const double diff = std::abs(a - b);
    if (diff <= abs)
        return true;

    const double scale = std::max(std::abs(a), std::abs(b));
    return diff <= rel * scale;
}

bool cmpNumStrTol(const QJsonObject &e, const QJsonObject &a,
                  const QString &k, QTextStream &err,
                  double rel, double abs)
{
    const QString es = e.value(k).toString();
    const QString as = a.value(k).toString();

    bool ok1 = false, ok2 = false;
    const double ev = es.toDouble(&ok1);
    const double av = as.toDouble(&ok2);

    if (!ok1 || !ok2)
    {
        err << "MISMATCH " << k << " non-numeric expected=" << es << " got=" << as << "\n";
        return false;
    }

    if (!almostEqual(ev, av, rel, abs))
    {
        err << "MISMATCH " << k << " expected=" << es << " got=" << as
            << " (diff=" << d2s(std::abs(ev - av)) << ")\n";
        return false;
    }

    return true;
}

bool cmpIntArray(const QJsonArray &e, const QJsonArray &a, QTextStream &err, const QString &what)
{
    if (e.size() != a.size())
    {
        err << "MISMATCH " << what << ".size expected=" << e.size() << " got=" << a.size() << "\n";
        return false;
    }
    for (int i = 0; i < e.size(); ++i)
    {
        const int ev = e.at(i).toInt();
        const int av = a.at(i).toInt();
        if (ev != av)
        {
            err << "MISMATCH " << what << "[" << i << "] expected=" << ev << " got=" << av << "\n";
            return false;
        }
    }
    return true;
}

bool cmpStrArray(const QJsonArray &e, const QJsonArray &a, QTextStream &err, const QString &what)
{
    if (e.size() != a.size())
    {
        err << "MISMATCH " << what << ".size expected=" << e.size() << " got=" << a.size() << "\n";
        return false;
    }
    for (int i = 0; i < e.size(); ++i)
    {
        const QString ev = e.at(i).toString();
        const QString av = a.at(i).toString();
        if (ev != av)
        {
            err << "MISMATCH " << what << "[" << i << "] expected=" << ev << " got=" << av << "\n";
            return false;
        }
    }
    return true;
}

} // namespace cli
