// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Distance/Centrality kernel (schema v1) for socnetv-cli.
// Extracted mechanically from former monolithic socnetv_cli.cpp.
// No behavior changes intended.

#include "kernel_distance_v1.h"


#include "graph.h"
#include "graphvertex.h"
#include "tools/headless_graph_loader.h"
#include "tools/cli/cli_common.h"


#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace cli {

// ---- local helpers (schema v1 only) ----

static QJsonArray buildPerNodeArray(Graph &g)
{
    QJsonArray arr;

    // Deterministic order: vertex numbers ascending
    const QList<int> verts = g.verticesList();
    for (int v : verts)
    {
        GraphVertex *gv = g.vertexPtr(v);
        if (!gv)
            continue;

        QJsonObject o;
        o["id"] = v;
        o["label"] = gv->label();

        // Core outputs requested: CC/BC/SC/EC/PC + standardized variants
        o["CC"] = d2s(gv->CC());
        o["SCC"] = d2s(gv->SCC());

        o["BC"] = d2s(gv->BC());
        o["SBC"] = d2s(gv->SBC());

        o["SC"] = d2s(gv->SC());
        o["SSC"] = d2s(gv->SSC());

        o["EC"] = d2s(gv->EC());
        o["SEC"] = d2s(gv->SEC());

        o["PC"] = d2s(gv->PC());
        o["SPC"] = d2s(gv->SPC());

        // Useful distance-kernel side values (helps catch subtle regressions)
        o["distance_sum"] = d2s(gv->distanceSum());

        const qreal ecc = gv->eccentricity();
        o["eccentricity"] = d2s(ecc);
        o["eccentricity_inf"] = (ecc >= 2147483647.0);

        arr.append(o);
    }

    return arr;
}

static QJsonObject buildGoldenJsonV1(
    const QString &inputPath,
    int fileFormat,
    const HeadlessLoadResult &load,
    Graph &g,
    bool computeCentralities,
    bool considerWeights,
    bool inverseWeights,
    bool dropIsolates,
    double avgDist,
    int diameter)
{
    QJsonObject root;
    root["schema_version"] = 1;

    QJsonObject dataset;
    dataset["path"] = inputPath;
    dataset["name"] = QFileInfo(inputPath).fileName();
    dataset["filetype"] = fileFormat;
    root["dataset"] = dataset;

    QJsonObject run;
    run["computeCentralities"] = computeCentralities;
    run["considerWeights"] = considerWeights;
    run["inverseWeights"] = inverseWeights;
    run["dropIsolates"] = dropIsolates;
    root["run"] = run;

    QJsonObject counts;
    counts["nodes"] = load.totalNodes;
    counts["links_sna"] = load.totalLinks;   // SNA links as reported by GraphLoaded
    counts["ties_graph"] = g.edgesEnabled(); // canonical model ties: edges if undirected, arcs if directed
    root["counts"] = counts;

    QJsonObject graph;
    graph["directed"] = g.isDirected();
    graph["weighted"] = g.isWeighted();
    root["graph"] = graph;

    QJsonObject metrics;
    metrics["avg_distance"] = d2s(avgDist);
    metrics["diameter"] = diameter;
    metrics["disconnected_pairs"] = g.notConnectedPairsSize();
    metrics["connected"] = g.isConnectedCached();
    root["metrics"] = metrics;

    // Per-node vectors (only meaningful if centralities computed)
    if (computeCentralities)
        root["per_node"] = buildPerNodeArray(g);

    QJsonObject loadReport;
    loadReport["ok"] = load.ok;
    loadReport["fileType_signal"] = load.fileType;
    loadReport["load_ms"] = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"] = load.message;
    loadReport["net_name"] = load.netName;
    root["load_report"] = loadReport;

    return root;
}

static bool cmpPerNodeArray(const QJsonArray &eArr, const QJsonArray &aArr, QTextStream &err)
{
    if (eArr.size() != aArr.size())
    {
        err << "MISMATCH per_node.size expected=" << eArr.size() << " got=" << aArr.size() << "\n";
        return false;
    }

    bool ok = true;

    auto cmpNodeFieldStrictStr = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id)
    {
        const QString ev = e.value(k).toString();
        const QString av = a.value(k).toString();
        if (ev != av)
        {
            err << "MISMATCH per_node id=" << id << " field=" << k << " expected=" << ev << " got=" << av << "\n";
            ok = false;
        }
    };

    auto cmpNodeFieldNumStrTol = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id)
    {
        const QString es = e.value(k).toString();
        const QString as = a.value(k).toString();
        bool ok1 = false, ok2 = false;
        const double ev = es.toDouble(&ok1);
        const double av = as.toDouble(&ok2);

        if (!ok1 || !ok2)
        {
            err << "MISMATCH per_node id=" << id << " field=" << k
                << " non-numeric expected=" << es << " got=" << as << "\n";
            ok = false;
            return;
        }
        if (!almostEqual(ev, av, 1e-15))
        {
            err << "MISMATCH per_node id=" << id << " field=" << k
                << " expected=" << es << " got=" << as
                << " (diff=" << d2s(std::abs(ev - av)) << ")\n";
            ok = false;
        }
    };

    auto cmpNodeFieldInt = [&](const QJsonObject &e, const QJsonObject &a, const QString &k, int id)
    {
        const int ev = e.value(k).toInt();
        const int av = a.value(k).toInt();
        if (ev != av)
        {
            err << "MISMATCH per_node id=" << id << " field=" << k << " expected=" << ev << " got=" << av << "\n";
            ok = false;
        }
    };

    for (int i = 0; i < eArr.size(); ++i)
    {
        const QJsonObject e = eArr.at(i).toObject();
        const QJsonObject a = aArr.at(i).toObject();

        const int eid = e.value("id").toInt();
        const int aid = a.value("id").toInt();
        if (eid != aid)
        {
            err << "MISMATCH per_node ordering at index=" << i
                << " expected_id=" << eid << " got_id=" << aid << "\n";
            ok = false;
            continue;
        }

        cmpNodeFieldInt(e, a, "id", eid);
        cmpNodeFieldStrictStr(e, a, "label", eid);

        const QStringList fields = {
            "CC", "SCC", "BC", "SBC", "SC", "SSC", "EC", "SEC", "PC", "SPC",
            "distance_sum", "eccentricity"};
        for (const QString &f : fields)
            cmpNodeFieldNumStrTol(e, a, f, eid);
    }

    return ok;
}

static int compareGoldenV1(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 1 || actual.value("schema_version").toInt() != 1)
    {
        err << "ERROR: schema_version mismatch or unsupported\n";
        return 2;
    }

    bool ok = true;

    const QJsonObject eDataset = expected.value("dataset").toObject();
    const QJsonObject aDataset = actual.value("dataset").toObject();
    ok &= cmpInt(eDataset, aDataset, "filetype", err);
    ok &= cmpStr(eDataset, aDataset, "name", err);

    const QJsonObject eRun = expected.value("run").toObject();
    const QJsonObject aRun = actual.value("run").toObject();
    ok &= cmpBool(eRun, aRun, "computeCentralities", err);
    ok &= cmpBool(eRun, aRun, "considerWeights", err);
    ok &= cmpBool(eRun, aRun, "inverseWeights", err);
    ok &= cmpBool(eRun, aRun, "dropIsolates", err);

    const QJsonObject eCounts = expected.value("counts").toObject();
    const QJsonObject aCounts = actual.value("counts").toObject();
    ok &= cmpInt(eCounts, aCounts, "nodes", err);
    ok &= cmpInt(eCounts, aCounts, "links_sna", err);
    ok &= cmpInt(eCounts, aCounts, "ties_graph", err);

    const QJsonObject eGraph = expected.value("graph").toObject();
    const QJsonObject aGraph = actual.value("graph").toObject();
    ok &= cmpBool(eGraph, aGraph, "directed", err);
    ok &= cmpBool(eGraph, aGraph, "weighted", err);

    const QJsonObject eMetrics = expected.value("metrics").toObject();
    const QJsonObject aMetrics = actual.value("metrics").toObject();
    ok &= cmpNumStrTol(eMetrics, aMetrics, "avg_distance", err, 1e-15);
    ok &= cmpInt(eMetrics, aMetrics, "diameter", err);
    ok &= cmpInt(eMetrics, aMetrics, "disconnected_pairs", err);
    ok &= cmpBool(eMetrics, aMetrics, "connected", err);

    const bool wantPerNode = expected.value("run").toObject().value("computeCentralities").toBool();
    if (wantPerNode)
    {
        const QJsonArray ePN = expected.value("per_node").toArray();
        const QJsonArray aPN = actual.value("per_node").toArray();
        ok &= cmpPerNodeArray(ePN, aPN, err);
    }

    if (!ok)
        return 1;

    err << "OK: baseline match\n";
    return 0;
}

// ---- exported runner ----

int runKernelDistanceV1(const CliConfig &cfg,
                        const HeadlessLoadResult &load,
                        Graph &g)
{
    // Ensure centralities are actually computed before we read per-node values.
    auto run_compute_once_ms = [&]() -> qint64
    {
        // Prevent early-return in DistanceEngine::compute()
        g.resetDistanceCentralityCacheFlags();

        QElapsedTimer t;
        t.start();

        // Canonical entry point (refactor target)
        g.graphDistancesGeodesic(cfg.computeCentralities,
                                 cfg.considerWeights,
                                 cfg.inverseWeights,
                                 cfg.dropIsolates);

        return t.elapsed();
    };

    if (cfg.benchRuns > 0)
    {
        // warmup (not measured)
        (void)run_compute_once_ms();

        std::vector<qint64> ms;
        ms.reserve(static_cast<size_t>(cfg.benchRuns));

        for (int r = 0; r < cfg.benchRuns; ++r)
            ms.push_back(run_compute_once_ms());

        std::sort(ms.begin(), ms.end());

        const qint64 minMs = ms.front();
        const qint64 maxMs = ms.back();

        const double meanMs =
            std::accumulate(ms.begin(), ms.end(), 0.0) / static_cast<double>(ms.size());

        const qint64 medianMs =
            (ms.size() % 2 == 1)
                ? ms[ms.size() / 2]
                : (ms[ms.size() / 2 - 1] + ms[ms.size() / 2]) / 2;

        printKV("COMPUTE_RUNS", cfg.benchRuns);
        printKV("COMPUTE_MS_MIN", minMs);
        printKV("COMPUTE_MS_MEDIAN", medianMs);
        printKV("COMPUTE_MS_MEAN", meanMs);
        printKV("COMPUTE_MS_MAX", maxMs);

        return 0;
    }

    const qint64 computeMs = run_compute_once_ms();
    printKV("COMPUTE_MS", computeMs);

    const qreal avgDist = g.graphDistanceGeodesicAverageCached();
    const int diameter = g.graphDiameterCached();
    const int discPairs = g.notConnectedPairsSize();
    const bool connected = g.isConnectedCached();

    printKV("AVG_DIST", QString::number(avgDist, 'g', 12));
    printKV("DIAMETER", diameter);
    printKV("DISC_PAIRS", discPairs);
    printKV("CONNECTED", connected ? 1 : 0);

    if (cfg.computeCentralities)
        printKV("PER_NODE", g.verticesList().size());

    const QJsonObject actual = buildGoldenJsonV1(
        cfg.inputPath,
        cfg.fileFormat,
        load,
        g,
        cfg.computeCentralities,
        cfg.considerWeights,
        cfg.inverseWeights,
        cfg.dropIsolates,
        static_cast<double>(avgDist),
        diameter);

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
        return compareGoldenV1(expected, actual);
    }

    return 0;
}

} // namespace cli
