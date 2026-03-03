#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QString>

namespace cli
{

    struct CliConfig
    {
        bool verbose = false;

        QString inputPath;
        int fileFormat = 0;
        QString delimiter;
        int twoMode = 0;
        bool hasLabels = false;

        bool computeCentralities = true;
        bool considerWeights = false;
        bool inverseWeights = true;
        bool dropIsolates = false;

        QString dumpJsonPath;
        QString compareJsonPath;

        int benchRuns = 0; // 0 = off
        QString kernel;    // "distance", etc.
        bool strict = false; // if true, timing regressions fail (exit non-zero)
    };

    // ---------------- printing ----------------

    void printKV(const QString &k, double v);
    void printKV(const QString &k, const QString &v);
    void printKV(const QString &k, int v);
    void printKV(const QString &k, qint64 v);

    // ---------------- deterministic formatting ----------------

    QString d2s(double v);

    // ---------------- JSON I/O ----------------

    bool writeJsonFile(const QString &path, const QJsonObject &obj, QString *err);
    bool readJsonFile(const QString &path, QJsonObject *outObj, QString *err);

    // ---------------- compare helpers ----------------

    bool cmpStr(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);
    bool cmpInt(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);
    bool cmpBool(const QJsonObject &e, const QJsonObject &a, const QString &k, QTextStream &err);

    bool almostEqual(double a, double b, double rel = 1e-15, double abs = 0.0);

    bool cmpNumStrTol(const QJsonObject &e, const QJsonObject &a,
                      const QString &k, QTextStream &err,
                      double rel = 1e-15, double abs = 0.0);

    bool cmpIntArray(const QJsonArray &e, const QJsonArray &a,
                     QTextStream &err, const QString &what);

    bool cmpStrArray(const QJsonArray &e, const QJsonArray &a,
                     QTextStream &err, const QString &what);

} // namespace cli
