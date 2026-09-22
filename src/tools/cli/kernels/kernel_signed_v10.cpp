// SPDX-License-Identifier: GPL-3.0-or-later
// SocNetV - Social Network Visualizer
//
// Signed-network analysis kernel (schema v10) for socnetv-cli.
// Computes Johnson's-algorithm potentials h(v) via Graph::graphComputePotentials() and reports
// negative-cycle detection, so both can be covered by golden-baseline regression testing.
// Designed to grow: later signed-network measures (PN centrality, structural balance ratio) are
// expected to add new JSON sections to this same kernel rather than spawning new ones, since they
// all describe the same "signed-network analysis of this dataset" concept.

#include "kernel_signed_v10.h"

#include "graph.h"
#include "graphvertex.h"
#include "tools/headless_graph_loader.h"
#include "tools/cli/cli_common.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

namespace cli {

static QJsonObject buildGoldenJsonV10(
    const QString     &inputPath,
    int                fileFormat,
    const HeadlessLoadResult &load,
    Graph             &g,
    bool               negativeCycleDetected,
    const QVector<qreal> &potentials)
{
    QJsonObject root;
    root["schema_version"] = 10;
    root["kernel"] = "signed";

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

    QJsonObject potentialsObj;
    potentialsObj["negative_cycle_detected"] = negativeCycleDetected;

    QJsonArray perNode;
    const QList<int> verts = g.verticesList();
    for (int i = 0; i < verts.size(); ++i) {
        const int v = verts.at(i);
        GraphVertex *gv = g.vertexPtr(v);
        if (!gv) continue;
        QJsonObject o;
        o["id"]    = v;
        o["label"] = gv->label();
        // Not meaningful when negativeCycleDetected is true (Bellman-Ford's own contract) -
        // still emitted so the JSON shape is uniform across both outcomes, but callers must
        // check negative_cycle_detected before trusting any value here.
        o["potential"] = negativeCycleDetected ? 0.0 : static_cast<double>(potentials.value(i, 0.0));
        perNode.append(o);
    }
    potentialsObj["per_node"] = perNode;
    root["potentials"] = potentialsObj;

    QJsonObject loadReport;
    loadReport["ok"]               = load.ok;
    loadReport["fileType_signal"]  = load.fileType;
    loadReport["load_ms"]          = static_cast<qint64>(load.elapsedTime);
    loadReport["load_msg"]         = load.message;
    loadReport["net_name"]         = load.netName;
    root["load_report"] = loadReport;

    return root;
}

static int compareGoldenV10(const QJsonObject &expected, const QJsonObject &actual)
{
    QTextStream err(stderr);

    if (expected.value("schema_version").toInt() != 10 ||
        actual.value("schema_version").toInt()   != 10)
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

    const QJsonObject ePot = expected.value("potentials").toObject();
    const QJsonObject aPot = actual.value("potentials").toObject();
    ok &= cmpBool(ePot, aPot, "negative_cycle_detected", err);

    const QJsonArray ePN = ePot.value("per_node").toArray();
    const QJsonArray aPN = aPot.value("per_node").toArray();
    if (ePN.size() != aPN.size()) {
        err << "MISMATCH potentials.per_node.size expected=" << ePN.size()
            << " got=" << aPN.size() << "\n";
        ok = false;
    } else {
        for (int i = 0; i < ePN.size(); ++i) {
            const QJsonObject e = ePN.at(i).toObject();
            const QJsonObject a = aPN.at(i).toObject();
            const int eid = e.value("id").toInt();
            const int aid = a.value("id").toInt();
            if (eid != aid) {
                err << "MISMATCH potentials.per_node ordering at index=" << i
                    << " expected_id=" << eid << " got_id=" << aid << "\n";
                ok = false;
                continue;
            }
            if (!almostEqual(e.value("potential").toDouble(), a.value("potential").toDouble())) {
                err << "MISMATCH potentials.per_node id=" << eid
                    << " potential expected=" << e.value("potential").toDouble()
                    << " got=" << a.value("potential").toDouble() << "\n";
                ok = false;
            }
        }
    }

    if (!ok) return 1;

    err << "OK: baseline match\n";
    return 0;
}

int runKernelSignedV10(const CliConfig &cfg,
                       const HeadlessLoadResult &load,
                       Graph &g)
{
    QVector<qreal> potentials;
    const bool ok = g.graphComputePotentials(cfg.inverseWeights, potentials);
    const bool negativeCycleDetected = !ok;

    printKV("NEGATIVE_CYCLE_DETECTED", negativeCycleDetected ? 1 : 0);

    const QJsonObject actual = buildGoldenJsonV10(
        cfg.inputPath, cfg.fileFormat, load, g, negativeCycleDetected, potentials);

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
        return compareGoldenV10(expected, actual);
    }

    return 0;
}

} // namespace cli
