// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Reachability kernel (schema v2) for socnetv-cli.
// Extracted mechanically from former monolithic socnetv_cli.cpp.
// No behavior changes intended.

#include "kernel_reachability_v2.h"
#include "graph.h"
#include "graphvertex.h"
#include "tools/cli/cli_common.h"
#include "tools/headless_graph_loader.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

#include <algorithm>

namespace cli {

// ---- schema v2 builder ----

static QJsonObject buildGoldenJsonV2Reachability(
    const QString &inputPath,
    int fileFormat,
    const HeadlessLoadResult &load,
    Graph &g,
    bool considerWeights,
    bool inverseWeights,
    bool dropIsolates,
    const QList<int> &order,
    const QJsonArray &matrix,
    int onesCount)
{
    QJsonObject root;
    root["schema_version"] = 2;
    root["kernel"] = "reachability";

    QJsonObject dataset;
    dataset["path"] = inputPath;
    dataset["name"] = QFileInfo(inputPath).fileName();
    dataset["filetype"] = fileFormat;
    root["dataset"] = dataset;

    QJsonObject run;
    run["considerWeights"] = considerWeights;
    run["inverseWeights"] = inverseWeights;
    run["dropIsolates"] = dropIsolates;
    run["operation"] = "reachability_matrix";
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

    QJsonObject reach;
    QJsonArray ord;
    for (int id : order)
        ord.append(id);
    reach["nodes"] = ord;
    reach["reachable_pairs"] = onesCount;
    reach["matrix"] = matrix;

    const int n = order.size();
    const double density = (n > 0) ? double(onesCount) / double(n * n) : 0.0;
    reach["reachable_density"] = d2s(density); // as string
    root["reachability"] = reach;

    QJsonObject loadReport;
    loadReport["ok"] = load.ok;
    loadReport["fileType_signal"] = load.fileType;
    loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"] = load.message;
    loadReport["net_name"] = load.netName;
    root["load_report"] = loadReport;

    return root;
}

// ---- schema v2 comparator ----

static int compareGoldenV2Reachability(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 2 || actual.value("schema_version").toInt() != 2)
    {
        err << "ERROR: schema_version mismatch or unsupported\n";
        return 2;
    }
    if (expected.value("kernel").toString() != "reachability" || actual.value("kernel").toString() != "reachability")
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

    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "nodes", err);
    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "links_sna", err);
    ok &= cmpInt(expected.value("counts").toObject(), actual.value("counts").toObject(), "ties_graph", err);

    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "directed", err);
    ok &= cmpBool(expected.value("graph").toObject(), actual.value("graph").toObject(), "weighted", err);

    const QJsonObject eR = expected.value("reachability").toObject();
    const QJsonObject aR = actual.value("reachability").toObject();

    ok &= cmpInt(eR, aR, "reachable_pairs", err);

    const QJsonArray eOrder = eR.value("nodes").toArray();
    const QJsonArray aOrder = aR.value("nodes").toArray();
    ok &= cmpIntArray(eOrder, aOrder, err, "reachability.nodes");

    // NOTE: this preserves your current behavior: exact compare via cmpNumStrTol with rel=0 abs=0
    ok &= cmpNumStrTol(eR, aR, "reachable_density", err, 0.0, 0.0);

    const QJsonArray eM = eR.value("matrix").toArray();
    const QJsonArray aM = aR.value("matrix").toArray();
    if (eM.size() != aM.size())
    {
        err << "MISMATCH reachability.matrix.rows expected=" << eM.size() << " got=" << aM.size() << "\n";
        ok = false;
    }
    else
    {
        for (int r = 0; r < eM.size(); ++r)
        {
            const QJsonArray eRow = eM.at(r).toArray();
            const QJsonArray aRow = aM.at(r).toArray();
            if (!cmpIntArray(eRow, aRow, err, QString("reachability.matrix[%1]").arg(r)))
                ok = false;
        }
    }

    if (!ok)
        return 1;

    err << "OK: baseline match\n";
    return 0;
}

// ---- exported runner ----

int runKernelReachabilityV2(const CliConfig &cfg,
                           const HeadlessLoadResult &load,
                           Graph &g)
{
    if (cfg.computeCentralities)
    {
        QTextStream(stderr) << "ERROR: --centralities is not applicable to --kernel reachability\n";
        return 2;
    }
    if (cfg.benchRuns > 0)
    {
        QTextStream(stderr) << "ERROR: --bench is only supported with --kernel distance\n";
        return 2;
    }

    // Compute geodesic distances once (centralities must be false here)
    g.resetDistanceCentralityCacheFlags();
    QElapsedTimer t;
    t.start();
    g.graphDistancesGeodesic(false, cfg.considerWeights, cfg.inverseWeights, cfg.dropIsolates);
    const qint64 computeMs = t.elapsed();

    // Deterministic vertex order
    QList<int> order = g.verticesList();
    std::sort(order.begin(), order.end());

    // Build reachability matrix (0/1) using the same semantics as graphReachable()
    QJsonArray matrix;
    int ones = 0;

    for (int src : order)
    {
        GraphVertex *gv = g.vertexPtr(src);
        if (!gv)
        {
            QTextStream(stderr) << "ERROR: null vertex for id=" << src << "\n";
            return 2;
        }

        QJsonArray row;
        for (int dst : order)
        {
            const bool reachable = (gv->distance(dst) != RAND_MAX);
            row.append(reachable ? 1 : 0);
            if (reachable)
                ++ones;
        }
        matrix.append(row);
    }

    printKV("COMPUTE_MS", computeMs);
    printKV("REACH_NODES", order.size());
    printKV("REACHABLE_PAIRS", ones);
    const int n = order.size();
    const double density = (n > 0) ? (double)ones / (double)(n * n) : 0.0;
    printKV("REACHABLE_DENSITY", QString::number(density, 'g', 17));

    const QJsonObject actual = buildGoldenJsonV2Reachability(
        cfg.inputPath, cfg.fileFormat, load, g,
        cfg.considerWeights, cfg.inverseWeights, cfg.dropIsolates,
        order, matrix, ones);

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
        return compareGoldenV2Reachability(expected, actual);
    }

    return 0;
}

} // namespace cli
