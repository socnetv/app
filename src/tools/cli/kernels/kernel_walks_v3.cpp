// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Walks matrix kernel (schema v3) for socnetv-cli.
// Exact-length K walks using Graph::walksBetween().
// No behavior changes intended elsewhere.

#include "kernel_walks_v3.h"

#include "tools/cli/cli_common.h"

#include "graph.h"
#include "tools/headless_graph_loader.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

#include <algorithm>

namespace cli {

// ---- schema v3 builder ----

static QJsonObject buildGoldenJsonV3WalksMatrix(
    const QString &inputPath,
    int fileFormat,
    const HeadlessLoadResult &load,
    Graph &g,
    bool considerWeights,
    bool inverseWeights,
    bool dropIsolates,
    int walksLength,
    const QList<int> &order,
    const QJsonArray &matrix,
    const QString &totalWalksStr)
{
    QJsonObject root;
    root["schema_version"] = 3;
    root["kernel"] = "walks_matrix";

    QJsonObject dataset;
    dataset["path"] = inputPath;
    dataset["name"] = QFileInfo(inputPath).fileName();
    dataset["filetype"] = fileFormat;
    root["dataset"] = dataset;

    QJsonObject run;
    run["considerWeights"] = considerWeights;
    run["inverseWeights"] = inverseWeights;
    run["dropIsolates"] = dropIsolates;
    run["operation"] = "walks_matrix_exact_length";
    run["walks_length"] = walksLength;
    root["run"] = run;

    QJsonObject counts;
    counts["nodes"] = load.totalNodes;
    counts["links_sna"] = load.totalLinks;
    counts["ties_graph"] = g.edgesEnabled();
    root["counts"] = counts;

    QJsonObject graph;
    graph["directed"] = g.isDirected();
    graph["weighted"] = g.isWeighted();
    root["graph"] = graph;

    QJsonObject walks;
    QJsonArray ord;
    for (int id : order)
        ord.append(id);
    walks["nodes"] = ord;
    walks["matrix"] = matrix;
    walks["total_walks"] = totalWalksStr; // integer as string
    root["walks"] = walks;

    QJsonObject loadReport;
    loadReport["ok"] = load.ok;
    loadReport["fileType_signal"] = load.fileType;
    loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"] = load.message;
    loadReport["net_name"] = load.netName;
    root["load_report"] = loadReport;

    return root;
}

// ---- schema v3 comparator ----

static int compareGoldenV3WalksMatrix(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 3 || actual.value("schema_version").toInt() != 3)
    {
        err << "ERROR: schema_version mismatch or unsupported\n";
        return 2;
    }
    if (expected.value("kernel").toString() != "walks_matrix" || actual.value("kernel").toString() != "walks_matrix")
    {
        err << "ERROR: kernel mismatch or unsupported\n";
        return 2;
    }

    bool ok = true;

    ok &= cmpInt(expected.value("dataset").toObject(), actual.value("dataset").toObject(), "filetype", err);
    ok &= cmpStr(expected.value("dataset").toObject(), actual.value("dataset").toObject(), "name", err);

    const QJsonObject eRun = expected.value("run").toObject();
    const QJsonObject aRun = actual.value("run").toObject();
    ok &= cmpBool(eRun, aRun, "considerWeights", err);
    ok &= cmpBool(eRun, aRun, "inverseWeights", err);
    ok &= cmpBool(eRun, aRun, "dropIsolates", err);
    ok &= cmpStr(eRun, aRun, "operation", err);
    ok &= cmpInt(eRun, aRun, "walks_length", err);

    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "nodes", err);
    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "links_sna", err);
    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "ties_graph", err);

    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "directed", err);
    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "weighted", err);

    const QJsonObject eW = expected.value("walks").toObject();
    const QJsonObject aW = actual.value("walks").toObject();

    ok &= cmpIntArray(eW.value("nodes").toArray(), aW.value("nodes").toArray(), err, "walks.nodes");
    ok &= cmpStr(eW, aW, "total_walks", err);

    const QJsonArray eM = eW.value("matrix").toArray();
    const QJsonArray aM = aW.value("matrix").toArray();

    if (eM.size() != aM.size())
    {
        err << "MISMATCH walks.matrix.rows expected=" << eM.size() << " got=" << aM.size() << "\n";
        ok = false;
    }
    else
    {
        for (int r = 0; r < eM.size(); ++r)
        {
            const QJsonArray eRow = eM.at(r).toArray();
            const QJsonArray aRow = aM.at(r).toArray();
            if (!cmpStrArray(eRow, aRow, err, QString("walks.matrix[%1]").arg(r)))
                ok = false;
        }
    }

    if (!ok)
        return 1;

    err << "OK: baseline match\n";
    return 0;
}

// ---- exported runner ----

int runKernelWalksV3(const CliConfig &cfg,
                     const HeadlessLoadResult &load,
                     Graph &g,
                     int walksLength)
{
    if (cfg.computeCentralities)
    {
        QTextStream(stderr) << "ERROR: --centralities is not applicable to --kernel walks_matrix\n";
        return 2;
    }
    if (cfg.benchRuns > 0)
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
        return 2;
    }
    if (walksLength < 1)
    {
        QTextStream(stderr) << "ERROR: --kernel walks_matrix requires --walks-length K (K>=1)\n";
        return 2;
    }

    // Deterministic vertex order
    QList<int> order = g.verticesList();
    std::sort(order.begin(), order.end());

    QElapsedTimer t;
    t.start();

    QJsonArray matrix;
    quint64 totalWalks = 0;

    for (int src : order)
    {
        QJsonArray row;

        for (int dst : order)
        {
            const int w = g.walksBetween(src, dst, walksLength);
            if (w < 0)
            {
                QTextStream(stderr) << "ERROR: walksBetween(" << src << "," << dst
                                    << "," << walksLength << ") returned " << w << "\n";
                return 2;
            }
            totalWalks += static_cast<quint64>(w);
            row.append(QString::number(w)); // integers as strings (deterministic)
        }
        matrix.append(row);
    }

    const qint64 computeMs = t.elapsed();

    printKV("COMPUTE_MS", computeMs);
    printKV("WALKS_NODES", order.size());
    printKV("WALKS_LENGTH", walksLength);
    printKV("WALKS_TOTAL", QString::number(totalWalks));

    const QJsonObject actual = buildGoldenJsonV3WalksMatrix(
        cfg.inputPath, cfg.fileFormat, load, g,
        cfg.considerWeights, cfg.inverseWeights, cfg.dropIsolates,
        walksLength,
        order, matrix, QString::number(totalWalks));

    if (!cfg.dumpJsonPath.isEmpty())
    {
        QString err;
        if (!writeJsonFile(cfg.dumpJsonPath, actual, &err))
        {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        QTextStream(stderr) << "WROTE_JSON=" << cfg.dumpJsonPath << "\n";
    }

    if (!cfg.compareJsonPath.isEmpty())
    {
        QJsonObject expected;
        QString err;
        if (!readJsonFile(cfg.compareJsonPath, &expected, &err))
        {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        return compareGoldenV3WalksMatrix(expected, actual);
    }

    return 0;
}

} // namespace cli
