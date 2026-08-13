// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Vertex connectivity kernel (schema v9) for socnetv-cli.
// Computes local (Menger's-theorem/max-flow) or global (pairwise-minimum) vertex connectivity via
// Graph::graphNodeConnectivity() / Graph::graphConnectivity(), and emits a JSON report suitable
// for golden-baseline regression testing.

#include "kernel_vertex_connectivity_v9.h"

#include "graph.h"
#include "tools/headless_graph_loader.h"
#include "tools/cli/cli_common.h"

#include <QFileInfo>
#include <QTextStream>

namespace cli {

static QString nodeConnectivityStatusStr(Graph::NodeConnectivityStatus s)
{
    switch (s) {
        case Graph::NodeConnectivityStatus::Ok:       return "ok";
        case Graph::NodeConnectivityStatus::Adjacent: return "adjacent";
        case Graph::NodeConnectivityStatus::Invalid:  return "invalid";
    }
    return "invalid";
}

static QJsonObject buildGoldenJsonV9(
    const QString     &inputPath,
    int                fileFormat,
    const HeadlessLoadResult &load,
    Graph             &g,
    const QString     &mode,
    const QString     &connectivityTypeLabel,
    int                connSource,
    int                connTarget,
    const Graph::NodeConnectivityResult &localResult,
    int                globalValue)
{
    QJsonObject root;
    root["schema_version"] = 9;
    root["kernel"] = "vertex_connectivity";

    QJsonObject dataset;
    dataset["path"]     = inputPath;
    dataset["name"]     = QFileInfo(inputPath).fileName();
    dataset["filetype"] = fileFormat;
    root["dataset"] = dataset;

    const int ties_graph = load.tiesGraph;
    const int links_sna  = g.isDirected() ? ties_graph : (2 * ties_graph);

    QJsonObject counts;
    counts["nodes"]      = load.totalNodes;
    counts["links_sna"]  = links_sna;
    counts["ties_graph"] = ties_graph;
    root["counts"] = counts;

    QJsonObject graph;
    graph["directed"] = g.isDirected();
    graph["weighted"] = g.isWeighted();
    root["graph"] = graph;

    root["mode"] = mode;
    root["connectivity_type"] = connectivityTypeLabel;

    if (mode == "local") {
        QJsonObject local;
        local["source"] = connSource;
        local["target"] = connTarget;
        local["status"] = nodeConnectivityStatusStr(localResult.status);
        if (localResult.status == Graph::NodeConnectivityStatus::Ok)
            local["value"] = localResult.value;
        root["local"] = local;
    } else {
        QJsonObject global;
        global["value"] = globalValue;
        root["global"] = global;
    }

    QJsonObject loadReport;
    loadReport["ok"]               = load.ok;
    loadReport["fileType_signal"]  = load.fileType;
    loadReport["load_ms"]          = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"]         = load.message;
    loadReport["net_name"]         = load.netName;
    root["load_report"] = loadReport;

    return root;
}

static int compareGoldenV9(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 9 ||
        actual.value("schema_version").toInt()   != 9)
    {
        err << "ERROR: schema_version mismatch or unsupported\n";
        return 2;
    }

    bool ok = true;

    ok &= cmpInt(expected.value("dataset").toObject(),
                 actual.value("dataset").toObject(),   "filetype", err);
    ok &= cmpStr(expected.value("dataset").toObject(),
                 actual.value("dataset").toObject(),   "name",     err);

    ok &= cmpInt(expected.value("counts").toObject(),
                 actual.value("counts").toObject(),    "nodes",      err);
    ok &= cmpInt(expected.value("counts").toObject(),
                 actual.value("counts").toObject(),    "ties_graph", err);

    ok &= cmpBool(expected.value("graph").toObject(),
                  actual.value("graph").toObject(),    "directed", err);

    ok &= cmpStr(expected, actual, "mode", err);
    ok &= cmpStr(expected, actual, "connectivity_type", err);

    const QString mode = actual.value("mode").toString();
    if (mode == "local") {
        const QJsonObject eLocal = expected.value("local").toObject();
        const QJsonObject aLocal = actual.value("local").toObject();
        ok &= cmpInt(eLocal, aLocal, "source", err);
        ok &= cmpInt(eLocal, aLocal, "target", err);
        ok &= cmpStr(eLocal, aLocal, "status", err);
        if (aLocal.value("status").toString() == "ok")
            ok &= cmpInt(eLocal, aLocal, "value", err);
    } else {
        ok &= cmpInt(expected.value("global").toObject(),
                     actual.value("global").toObject(), "value", err);
    }

    if (!ok) return 1;

    err << "OK: baseline match\n";
    return 0;
}

int runKernelVertexConnectivityV9(const CliConfig &cfg,
                                  const HeadlessLoadResult &load,
                                  Graph &g)
{
    const bool useStrong = g.isDirected() && (cfg.connectivityType == "strong");
    const QString typeLabel = useStrong ? "strong" : (g.isDirected() ? "weak" : "undirected");
    const bool respectDirection = useStrong;

    const QString mode = (cfg.connMode == "local") ? "local" : "global";

    Graph::NodeConnectivityResult localResult;
    int globalValue = 0;

    if (mode == "local") {
        localResult = g.graphNodeConnectivity(cfg.connSource, cfg.connTarget, respectDirection);
        printKV("STATUS", nodeConnectivityStatusStr(localResult.status));
        if (localResult.status == Graph::NodeConnectivityStatus::Ok)
            printKV("VALUE", localResult.value);
    } else {
        globalValue = g.graphConnectivity(respectDirection);
        printKV("VALUE", globalValue);
    }
    printKV("TYPE", typeLabel);

    const QJsonObject actual = buildGoldenJsonV9(
        cfg.inputPath, cfg.fileFormat, load, g, mode, typeLabel,
        cfg.connSource, cfg.connTarget, localResult, globalValue);

    if (!cfg.dumpJsonPath.isEmpty()) {
        QString err;
        if (!writeJsonFile(cfg.dumpJsonPath, actual, &err)) {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        QTextStream(stderr) << "WROTE_JSON=" << cfg.dumpJsonPath << "\n";
    }

    if (!cfg.compareJsonPath.isEmpty()) {
        QJsonObject expected;
        QString err;
        if (!readJsonFile(cfg.compareJsonPath, &expected, &err)) {
            QTextStream(stderr) << "ERROR: " << err << "\n";
            return 2;
        }
        return compareGoldenV9(expected, actual);
    }

    return 0;
}

} // namespace cli
