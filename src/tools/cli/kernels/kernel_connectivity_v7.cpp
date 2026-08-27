// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Connectivity kernel (schema v7) for socnetv-cli.
// Computes weakly or strongly connected components (--connectivity-type weak|strong, weak is the
// default and the only option that means anything on an undirected graph) via
// Graph::graphWeaklyConnectedComponents() / Graph::graphStronglyConnectedComponents(), and emits a
// JSON report suitable for golden-baseline regression testing.

#include "kernel_connectivity_v7.h"

#include "graph.h"
#include "graphvertex.h"
#include "tools/headless_graph_loader.h"
#include "tools/cli/cli_common.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

namespace cli {

static QJsonObject buildGoldenJsonV7(
    const QString     &inputPath,
    int                fileFormat,
    const HeadlessLoadResult &load,
    Graph             &g,
    int                componentCount,
    const QString     &connectivityTypeLabel)
{
    QJsonObject root;
    root["schema_version"] = 7;
    root["kernel"] = "connectivity";

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

    QJsonObject conn;
    conn["connected"]       = (componentCount <= 1);
    conn["component_count"] = componentCount;
    conn["type"] = connectivityTypeLabel;
    root["connectivity"] = conn;

    // Per-node component IDs (deterministic: BFS visits vertices in list order). Only meaningful
    // for weak/undirected mode - Graph::vertexComponentId() is populated by
    // graphWeaklyConnectedComponents() only; graphStronglyConnectedComponents() reports just a
    // count, not per-vertex SCC membership (see its doc comment), so emitting it here for strong
    // mode would silently show weak IDs mislabeled as strong ones.
    const bool showComponentIds = (connectivityTypeLabel != "strong");
    const QHash<int,int> &compMap = g.vertexComponentId();
    QJsonArray perNode;
    const QList<int> verts = g.verticesList();
    for (int v : verts) {
        GraphVertex *gv = g.vertexPtr(v);
        if (!gv) continue;
        QJsonObject o;
        o["id"]    = v;
        o["label"] = gv->label();
        if (showComponentIds)
            o["component_id"] = compMap.value(v, 0);
        perNode.append(o);
    }
    root["per_node"] = perNode;

    QJsonObject loadReport;
    loadReport["ok"]               = load.ok;
    loadReport["fileType_signal"]  = load.fileType;
    loadReport["load_ms"]          = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"]         = load.message;
    loadReport["net_name"]         = load.netName;
    root["load_report"] = loadReport;

    return root;
}

static int compareGoldenV7(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 7 ||
        actual.value("schema_version").toInt()   != 7)
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

    const QJsonObject eConn = expected.value("connectivity").toObject();
    const QJsonObject aConn = actual.value("connectivity").toObject();
    ok &= cmpBool(eConn, aConn, "connected",       err);
    ok &= cmpInt (eConn, aConn, "component_count", err);
    ok &= cmpStr (eConn, aConn, "type",            err);

    // Per-node component IDs
    const QJsonArray ePN = expected.value("per_node").toArray();
    const QJsonArray aPN = actual.value("per_node").toArray();
    if (ePN.size() != aPN.size()) {
        err << "MISMATCH per_node.size expected=" << ePN.size()
            << " got=" << aPN.size() << "\n";
        ok = false;
    } else {
        for (int i = 0; i < ePN.size(); ++i) {
            const QJsonObject e = ePN.at(i).toObject();
            const QJsonObject a = aPN.at(i).toObject();
            const int eid = e.value("id").toInt();
            const int aid = a.value("id").toInt();
            if (eid != aid) {
                err << "MISMATCH per_node ordering at index=" << i
                    << " expected_id=" << eid << " got_id=" << aid << "\n";
                ok = false;
                continue;
            }
            const int ec = e.value("component_id").toInt();
            const int ac = a.value("component_id").toInt();
            if (ec != ac) {
                err << "MISMATCH per_node id=" << eid
                    << " component_id expected=" << ec << " got=" << ac << "\n";
                ok = false;
            }
        }
    }

    if (!ok) return 1;

    err << "OK: baseline match\n";
    return 0;
}

int runKernelConnectivityV7(const CliConfig &cfg,
                            const HeadlessLoadResult &load,
                            Graph &g)
{
    // Strong connectivity only means something distinct from weak on a directed graph - on an
    // undirected graph the two notions coincide, so --connectivity-type is ignored there (same
    // as the GUI's Connectedness action, which only asks weak-vs-strong for directed networks).
    const bool useStrong = g.isDirected() && (cfg.connectivityType == "strong");

    const int components = useStrong
        ? g.graphStronglyConnectedComponents()
        : g.graphWeaklyConnectedComponents();
    const bool connected = (components <= 1);
    const QString typeLabel = useStrong ? "strong" : (g.isDirected() ? "weak" : "connected");

    printKV("COMPONENTS", components);
    printKV("CONNECTED",  connected ? 1 : 0);
    printKV("TYPE", typeLabel);

    const QJsonObject actual = buildGoldenJsonV7(
        cfg.inputPath, cfg.fileFormat, load, g, components, typeLabel);

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
        return compareGoldenV7(expected, actual);
    }

    return 0;
}

} // namespace cli
